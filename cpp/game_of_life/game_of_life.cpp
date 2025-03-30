/*
 * MIT License
 *
 * Copyright (c) 2020 International Business Machines
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <chrono>
#include "helayers/math/TTConvolutionInterleaved.h"
#include "helayers/math/TTEncoder.h"
#include "helayers/math/TTFunctionEvaluator.h"
#include "helayers/hebase/openfhe/OpenFheCkksContext.h"
#include "helayers/hebase/mockup/MockupContext.h"

using namespace std;
using namespace chrono;
using namespace helayers;

int N = 256;
int M = 256; // board size is N*M
int iterations = 1;
bool verbose = false;
bool mockup = false;
bool useLagrange = false;

DoubleTensor* plainBoard;

shared_ptr<CTileTensor> tensorBoard; // true board
TTShape inputShape;

void printBoard(const string& name, const DoubleTensor& board);
void printEncryptedBoard(CTileTensor& board);

void cleanTensor(CTileTensor& x) // cleaning the tensor (inplace)
{
  CTileTensor square = x.getSquare();
  CTileTensor cube = square;
  cube.multiply(x);

  // computation of 3x^2-2x^3 which is a rough-but-good-enough approximation of
  // the sign fucntion
  x = square;
  x.add(square);
  x.add(square);
  x.sub(cube);
  x.sub(cube);

  return;
}

// Each slot of x becomes 1 if it equals to a, to 0 otherwise
void checkEqualTensorInPlace(CTileTensor& x, int a, helayers::HeContext& he)
{
  TTEncoder enc(he);

  if (useLagrange) {
    if (a < 0 || a > 8)
      throw runtime_error("we only support integers between 0 and 8");
    TTFunctionEvaluator fe(he);
    std::vector<double> range = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    fe.computeLagrangeBasis(x, x, range, a);
    return;
  }
  TTFunctionEvaluator fe(he);

  x.addScalar(-a);
  // elements are in the range [0,9], call signInPlace with maxValue=9.
  // Set the number of function steps to 8 and 3.
  fe.signInPlace(x, 8, 3, 9);

  // Compute (x-1)(x+1)
  x.square();
  x.negate();
  x.addScalar(+1);
}

// Initialize the encrypted board (tensorBoard) and the plaintext board
// (plainBoard).
//
// startingPos is a binary 2D tensor holding the initial setup,
// where 1 indicates an occupied cell and 0 indicates a free cell.
void initBoard(TTEncoder& enc, HeContext& he, DoubleTensor& startingPos)
{
  *plainBoard = startingPos;

  // Counting the the neighbors is done efficiently using sum-pooling.
  // The sum-pooling mechanism is shared with convolution mechanism which
  // assumes filter dimensions. These are degenerate dims in our case.
  startingPos.reshape({1, M, N, 1, 1});

  TTShape baseShape{1, 256, he.slotCount() / 256, 1, 1};
  inputShape = baseShape.getWithDuplicatedDim(3);
  inputShape.setOriginalSizes({1, M, N, 1, 1});

  // Set the dimensions associated with the X and Y dimensions of the board to
  // be interleaved. This improves the efficiency of computing the number of
  // live neighbors of cells
  inputShape.getDim(1).setInterleaved(true, 1);
  inputShape.getDim(2).setInterleaved(true, 1);

  tensorBoard = make_shared<CTileTensor>(he);
  enc.encodeEncrypt(*tensorBoard, inputShape, startingPos);
}

// compute plain step (for comparison)
// in order to not overwrite the current board with the computations results
// for the next iteration, we have a "current" board and a "next" board
// we switch between the boards along the iterations
static inline double computePlainStep(int x, int y)
{
  double plainNeighbours = 0;
  for (int xpos = max(0, x - 1); xpos < min(M, x + 2); xpos++) {
    for (int ypos = max(0, y - 1); ypos < min(N, y + 2); ypos++) {
      plainNeighbours += plainBoard->at(xpos, ypos);
    }
  }

  plainNeighbours -= (plainBoard->at(x, y));
  double plainThreeCheck = (plainNeighbours == 3) ? 1 : 0;
  double plainTwoCheck = (plainNeighbours == 2) ? 1 : 0;
  plainTwoCheck *= plainBoard->at(x, y);
  plainThreeCheck += (plainTwoCheck);
  return plainThreeCheck;
}

// compute one step in the tensor board
static void tensorStep(HeContext& he)
{
  HELAYERS_TIMER_SECTION("iteration");

  const int strideRows = 1;
  const int strideCols = 1;

  cout << "  - Compute plain step" << endl;
  DoubleTensor newPlainBoard({M, N});
  for (int x = 0; x < M; x++) {
    for (int y = 0; y < N; y++) {
      newPlainBoard.at(x, y) = computePlainStep(x, y); // for plain comparison
    }
  }
  *plainBoard = newPlainBoard;

  TTConvConfig cc(he, tensorBoard->getShape(), 3, 3, 0, false);

  cc.setStrides(strideRows, strideCols);
  cc.setDefaultDims(true); // cxyfb = true

  // To add all neighbors of all cells we use SumPooling with on a 3x3 window.
  // The padding policy in this case padds the board with non-occupied cells.
  Padding2d padding = Padding2d::same(M, N, 3, 3, 3, 3);
  cc.setPadding(padding);

  // will store the number of neighbours each cell has
  CTileTensor neighbours(he);
  TTConvFilters cf(cc);
  TTConvolutionInterleaved sumPoolEngine(tensorBoard, cf);
  neighbours = sumPoolEngine.getConvolution(); // get the number of neighbours
                                               // with the convolution

  neighbours.sub(*tensorBoard); // sum pooling returns the sum of the
                                // neighbours with the cell, and we don't want
                                // the cell status to be included in the sum

  cout << "  - Check for 2/3 neighbours" << endl;
  // if a cell has 3 neighbours, then the corresponding tile in threeCheck will
  // have a value of 1
  CTileTensor threeCheck(neighbours);
  // if a cell has 2 neighbours, then the corresponding tile in twoCheck will
  // have a value of 1
  CTileTensor twoCheck(neighbours);

  checkEqualTensorInPlace(threeCheck, 3, he);
  checkEqualTensorInPlace(twoCheck, 2, he);

  twoCheck.multiply(*tensorBoard);
  threeCheck.add(twoCheck);
  // the new state of a cell is (N==3) or ((N==2) and oldState==1) where N is
  // the number of alive neighbours, and oldState is the previous state

  cout << "  - Cleanup" << endl;
  cleanTensor(threeCheck);

  *tensorBoard = threeCheck;
}

void printBoard(const string& name, const DoubleTensor& board)
{
  std::cout << name << ":" << std::endl;
  for (int x = 0; x < M; x++) {
    for (int y = 0; y < N; y++) {
      double val = board.at(x, y);

      if (val >= 0.5) {
        std::cout << "⬜";
      } else {
        std::cout << "⬛";
      }
    }
    std::cout << std::endl;
  }
}

void printEncryptedBoard(CTileTensor& board)
{
  TTEncoder enc(board.getHeContext());
  DoubleTensor decryptedBoard = enc.decryptDecodeDouble(board);
  decryptedBoard.reshape({M, N});
  double max = 0;
  double avg = 0;
  printBoard("Ciphertext board", decryptedBoard);
  for (size_t i = 0; i < M; ++i) {
    for (size_t j = 0; j < N; ++j) {
      double dif = abs(plainBoard->at(i, j) - decryptedBoard.at(i, j));
      if (dif > max) {
        max = dif;
      }
      avg += dif;
    }
  }

  // printBoard("Plaintext board", *plainBoard);

  std::cout << "max difference from plaintext is " << max
            << " and the average difference is " << avg / (M * N) << std::endl;
}

void benchmarkGameOfLife()
{
  shared_ptr<HeContext> he;
  HeConfigRequirement reqGameOfLife;
  if (mockup) {
    he = make_shared<MockupContext>();
    reqGameOfLife = HeConfigRequirement::insecure(pow(2, 16), // numSlots
                                                  13, // multiplicationDepth
                                                  24, // fractionalPartPrecision
                                                  5   // integerPartPrecision
    );
    reqGameOfLife.bootstrappable = true;
    reqGameOfLife.bootstrapConfig = BootstrapConfig();
    reqGameOfLife.bootstrapConfig->targetChainIndex = 12;
    reqGameOfLife.bootstrapConfig->minChainIndexForBootstrapping = 3;
  } else {
    he = make_shared<OpenFheCkksContext>();

    reqGameOfLife = HeConfigRequirement(pow(2, 14), // numSlots
                                        20,         // multiplicationDepth
                                        29,         // fractionalPartPrecision
                                        5           // integerPartPrecision
    );
    reqGameOfLife.bootstrappable = true;
  }

  std::cout << "Initializing HeContext . . ." << std::endl;
  he->init(reqGameOfLife);
  if (!useLagrange)
    he->setAutomaticBootstrapping(true);

  TTEncoder ttencoder(*he);

  DoubleTensor startingPos({M, N});
  // starting position a glider
  startingPos.at(0, 0) = 1;
  startingPos.at(1, 1) = 1;
  startingPos.at(1, 2) = 1;
  startingPos.at(0, 2) = 1;
  startingPos.at(2, 1) = 1;
  initBoard(ttencoder, *he, startingPos);

  for (int i = 0; i < iterations; ++i) {

    if (verbose)
      printEncryptedBoard(*tensorBoard);

    auto start = high_resolution_clock::now();

    if ((i != 0) && (useLagrange)) {
      tensorBoard->bootstrap(); // the best bootsrapping point is here, where
      // there is only one CTileTensor
    }

    tensorStep(*he);

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<seconds>(end - start);

    std::cout << "tensor step took " << duration.count() << std::endl;

    // HELAYERS_TIMER_PRINT_MEASURES_SUMMARY();
    // cout << "Exiting after one iteration" << endl;
  }
}

int main(int argc, char* argv[])
{
  int i = 1;
  while (i < argc) {
    string arg = argv[i++];
    if (arg == "--iterations" && i < argc)
      iterations = stoi(argv[i++]);
    else if (arg == "--size" && i < argc)
      N = M = stoi(argv[i++]);
    else if (arg == "--lagrange")
      useLagrange = true;
    else if (arg == "--verbose")
      verbose = true;
    else if (arg == "--mockup")
      mockup = true;
    else {
      cerr << "Usage:" << endl;
      cerr << "     --iterations ITER" << endl;
      cerr << "     --verbose" << endl;
      cerr << "     --mockup" << endl;
      cerr << "     --lagrange" << endl;
      cerr << "     --size N" << endl;
      throw runtime_error(string("Unknown argument ") + arg);
    }
  }
  plainBoard = new DoubleTensor({M, N});

  benchmarkGameOfLife();
}