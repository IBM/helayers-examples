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

#include "helayers/math/TTConvolutionInterleaved.h"
#include "helayers/math/TTEncoder.h"
#include "helayers/math/TTFunctionEvaluator.h"
#include "helayers/hebase/heaan/HeaanContext.h"

using namespace std;
using namespace helayers;

const int N = 6;
const int M = 6; // board size is N*M

const int DEFAULT_ITERATIONS = 4;

DoubleTensor plainBoard({2, M, N});

shared_ptr<CTileTensor> tensorBoard; // true board
TTShape inputShape;

struct InterleavedConvolutionParameters // for the neighbour convolution
{
  int numChannels;
  int numRows;
  int numCols;
  int numFilters;
  int numBatches;

  int filtersNumRows;
  int filtersNumCols;

  int strideRows;
  int strideCols;

  int externalRows{-1};
  int externalCols{-1};

  Padding2d padding;

  void setImageData(int c, int r, int col, int b)
  {
    numChannels = c;
    numRows = r;
    numCols = col;
    numBatches = b;
  }

  void setFilterData(int f, int r, int c, int sr, int sc)
  {
    numFilters = f;
    filtersNumRows = r;
    filtersNumCols = c;
    strideRows = sr;
    strideCols = sc;
  }

  void setExternalSizes(int rows, int cols)
  {
    externalRows = rows;
    externalCols = cols;
  }

  void setSamePadding()
  {
    padding = Padding2d::same(numRows,
                              numCols,
                              filtersNumRows,
                              filtersNumCols,
                              strideRows,
                              strideCols);
  }

  void setValidPadding() { padding = Padding2d(); }

  HeContext* he;
};

InterleavedConvolutionParameters p; // global convolution parameters val

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

void checkEqualTensorInPlace(
    CTileTensor& x,
    int a,
    TTEncoder& enc,
    helayers::HeContext& he,
    bool useLagrange =
        true) // write in res 1 if dec(x)==a and 0 otherwise (for every slot)
{
  if (useLagrange) {
    if (a < 0 || a > 8) {
      std::cout << "we only support integers between 0 and 8" << std::endl;
    }
    TTFunctionEvaluator fe(he);
    std::vector<double> range = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    fe.computeLagrangeBasis(x, x, range, a);
    return;
  }
  TTFunctionEvaluator fe(he);
  CTileTensor tmp(he);
  DoubleTensor plainDT;
  plainDT.init(inputShape.getOriginalSizes(), static_cast<double>(a));
  enc.encodeEncrypt(tmp, inputShape, plainDT);

  x.sub(tmp); // x==a is the same as x-a ==0
  x.multiplyScalar(
      1.0 / 9.0); // scaling, since sign in place works when diff is at max 1
  fe.signInPlace(x, 8, 3);

  plainDT.init(inputShape.getOriginalSizes(), 1);
  enc.encodeEncrypt(tmp, inputShape, plainDT);

  x.add(tmp);
  x.multiplyScalar(0.5); // from [-1.1] to [0,1]

  tmp.sub(x);
  x.multiply(tmp);
  x.multiplyScalar(4);
}

// initializg the board
// starting pos is a binary vector for the starting position
// the ordering for starting pos can be wierd: the indices {x_1,x_2,x_3,x_4} for
// a 2X2 board will create the following board:

// x_1 x_3
// x_2 x_4

// where x_i should be 0 or 1
// for example the vector {1,0,0,1} will create the board
// 1 0
// 0 1

// and the vector {0,1,0,0} will create the board
// 0 0
// 1 0

// also initializing the convolution parameters
void initBoard(TTEncoder& enc, HeContext& he, std::vector<double> startingPos)
{
  p.he = (&he);
  p.setImageData(1, M, N, 1);
  p.setFilterData(1, 3, 3, 1, 1);
  p.setSamePadding();
  TTShape baseShape{1, 4, p.he->slotCount() / 4, 1, 1};

  const int imageRowDim = 1;
  const int imageColDim = 2;
  const int imageFilterDim = 3; // working cxyfb

  int numRowsWithPadding = p.numRows;
  int numColsWithPadding = p.numCols;

  inputShape = baseShape.getWithDuplicatedDim(imageFilterDim);
  inputShape.setOriginalSizes(
      {p.numChannels, numRowsWithPadding, numColsWithPadding, 1, p.numBatches});

  inputShape.getDim(imageRowDim).setInterleaved(true, p.strideRows);
  inputShape.getDim(imageColDim).setInterleaved(true, p.strideCols);

  if (p.externalRows != -1)
    inputShape.getDim(imageRowDim).setInterleavedExternalSize(p.externalRows);
  if (p.externalCols != -1)
    inputShape.getDim(imageColDim).setInterleavedExternalSize(p.externalCols);

  std::vector<double> matrix(M * N);
  for (int i = 0; i < M * N; ++i) {
    matrix[i] = startingPos[i];
  }
  DoubleTensor heInput({M, N});

  heInput.init(inputShape.getOriginalSizes(), matrix);
  tensorBoard = make_shared<CTileTensor>(he);
  enc.encodeEncrypt(*tensorBoard, inputShape, heInput);

  for (int x = 0; x < M; x++) {
    for (int y = 0; y < N; y++) {
      plainBoard.at(0, x, y) = startingPos[N * y + x];
    }
  }
}

// compute plain step (for comparison)
// in order to not overwrite the current board with the computations results
// for the next iteration, we have a "current" board and a "next" board
// we switch between the boards along the iterations
static inline void computePlainStep(int iterationCounter, int x, int y)
{
  int currentBoardIndex = iterationCounter % 2;
  double plainNeighbours = 0;
  for (int xpos = max(0, x - 1); xpos < min(M, x + 2); xpos++) {
    for (int ypos = max(0, y - 1); ypos < min(N, y + 2); ypos++) {
      plainNeighbours += plainBoard.at(currentBoardIndex, xpos, ypos);
    }
  }

  plainNeighbours -= (plainBoard.at(currentBoardIndex, x, y));
  double plainThreeCheck = int(plainNeighbours == 3);
  double plainTwoCheck = int(plainNeighbours == 2);
  plainTwoCheck *= (plainBoard.at(currentBoardIndex, x, y));
  plainThreeCheck += (plainTwoCheck);
  plainBoard.at(1 - currentBoardIndex, x, y) = plainThreeCheck;
}

// compute one step in the tensor board
static void tensorStep(TTEncoder& enc, HeContext& he, int step)
{
  HELAYERS_TIMER_SECTION("iteration");

  cout << "  - Compute plain step" << endl;
  for (int x = 0; x < M; x++) {
    for (int y = 0; y < N; y++) {
      computePlainStep(step, x, y); // for plain comparison
    }
  }

  cout << "  - Bootstrap" << endl;
  (*tensorBoard).bootstrap(); // the best bootsrapping point is here, where
                              // there is only one CTileTensor

  cout << "  - Get number of neighbours" << endl;
  CTileTensor neighbours(he);
  TTConvConfig cc(tensorBoard->getHeContext(),
                  tensorBoard->getShape(),
                  p.filtersNumRows,
                  p.filtersNumCols);
  cc.setStrides(p.strideRows, p.strideCols);
  cc.setDefaultDims(true);
  cc.setPadding(p.padding);
  TTConvFilters ff(cc);
  TTConvolutionInterleaved sumPoolEngine(tensorBoard, ff);
  neighbours = sumPoolEngine.getConvolution(); // get the number of neighbours
                                               // with the convolution

  neighbours.sub(*tensorBoard); // sum pooling returns the sum of the
                                // neighbours with the cell, and we don't want
                                // the cell status to be included in the sum

  cout << "  - Check for 2/3 neighbours" << endl;
  CTileTensor threeCheck(
      neighbours); // if a cell has 3 neighbours, then the corresponding tile in
                   // threeCheck will have a value of 1
  CTileTensor twoCheck(
      neighbours); // if a cell has 2 neighbours, then the corresponding tile in
                   // twoCheck will have a value of 1

  checkEqualTensorInPlace(threeCheck, 3, enc, he);
  checkEqualTensorInPlace(twoCheck, 2, enc, he);

  twoCheck.multiply(*tensorBoard);
  threeCheck.add(twoCheck);
  // the new state of a cell is (N==3) or ((N==2) and oldState==1) where N is
  // the number of alive neighbours, and oldState is the previous state

  cout << "  - Cleanup" << endl;
  cleanTensor(threeCheck);

  *tensorBoard = threeCheck;
}

void printBoard(const string& name, int i, const DoubleTensor& board)
{
  std::cout << name << ":" << std::endl;
  for (int x = 0; x < M; x++) {
    for (int y = 0; y < N; y++) {
      double val = board.at(i, x, y);

      if (val >= 0.5) {
        std::cout << "⬜";
      } else {
        std::cout << "⬛";
      }
    }
    std::cout << std::endl;
  }
}

void printEncryptedBoard(TTEncoder& enc, int parity = 0)
{
  DoubleTensor decryptedBoard = enc.decryptDecodeDouble(*tensorBoard);
  double max = 0;
  double avg = 0;
  printBoard("Ciphertext board", 0, decryptedBoard);
  for (size_t i = 0; i < M; ++i) {
    for (size_t j = 0; j < N; ++j) {
      double dif =
          abs(plainBoard.at(parity, i, j) - decryptedBoard.at(0, i, j, 0, 0));
      if (dif > max) {
        max = dif;
      }
      avg += dif;
    }
  }
  std::cout << "max difference from plaintext is " << max
            << " and the average difference is " << avg / (M * N) << std::endl;
}

void help()
{
  cout << "Usage: ./game_of_life [--iterations n]" << endl;
  cout << "--iterations n\tNumber of iterations (default: "
       << DEFAULT_ITERATIONS << ")" << endl;
  exit(1);
}

int main(int argc, char* argv[])
{
  int iterations = DEFAULT_ITERATIONS;
  int i = 1;
  while (i < argc) {
    string arg = argv[i++];
    if (arg == "--iterations" && i < argc)
      iterations = stoi(argv[i++]);
    else
      help();
  }

  shared_ptr<HeContext> he = make_shared<HeaanContext>();
  HeConfigRequirement req(pow(2, 15), // numSlots
                          9,          // multiplicationDepth
                          24,         // fractionalPartPrecision
                          5           // integerPartPrecision
  );
  req.bootstrappable = true;
  req.automaticBootstrapping = true;
  std::cout << "Initializing HeContext . . ." << std::endl;
  he->init(req);
  std::cout << "Finished context initialization" << std::endl;

  TTEncoder ttencoder(*he);

  std::vector<double> startingPos(M * N); // starting position
  startingPos[0] = 1;
  startingPos[M + 1] = 1;
  startingPos[M + 2] = 1;
  startingPos[2 * M] = 1;
  startingPos[2 * M + 1] = 1; // glider
  initBoard(ttencoder, *he, startingPos);

  for (int i = 0; i < iterations; ++i) {
    std::cout << "Starting iteration " << i + 1 << "/" << iterations
              << std::endl;
    printBoard("Plain board", i % 2, plainBoard);
    printEncryptedBoard(ttencoder, i % 2);
    std::cout << "Computing next step" << std::endl;
    tensorStep(ttencoder, *he, i);
    std::cout << "Finished iteration " << i + 1 << "/" << iterations
              << std::endl
              << std::endl;
  }

  HELAYERS_TIMER_PRINT_MEASURE_SUMMARY("iteration");

  // explicitly clean before HEaaN own destructor
  tensorBoard.reset();
  he.reset();
}
