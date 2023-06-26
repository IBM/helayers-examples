
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
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// #include <iomanip>
#include <iostream>
#include <sstream>
#include "helayers/hebase/hebase.h"
#include "helayers/math/TTEncoder.h"
#include "helayers/hebase/mockup/CircuitContext.h"
#include "helayers/circuit/Runner.h"

using namespace std;
using namespace helayers;
using namespace circuit;

///@brief Returns a random tensor with the given shape, and elements in the
/// range [0,100]
///
///@param shape The tensor's shape
DoubleTensor createRandTensor(const std::vector<DimInt>& shape)
{
  DoubleTensor res(shape);
  std::uniform_int_distribution<> unif(0, 100);
  static std::default_random_engine re;
  for (int i = 0; i < res.size(); ++i)
    res.at(i) = unif(re);
  return res;
}

void buildCircuit5(CircuitContext& he)
{
  TTEncoder encoder(he);

  // In this circuit we'll add these two input tensors,
  // encrypted as tile tensors
  DoubleTensor input1 = createRandTensor({3, 3, 5});
  DoubleTensor input2 = createRandTensor({3, 3, 5});

  // Let's create our 2 ciphertexts
  cout << "Creating the first tile tensor that is an input to the circuit"
       << endl;
  TTShape shape({2, 2, he.slotCount() / 4});
  CTileTensor inputEnc1(he);
  encoder.encodeEncrypt(inputEnc1, shape, input1);
  cout << "Label the tile tensor as an input so we can attach a tile tensor to "
          "it "
          "when we run it later"
       << endl;
  he.labelCtxt(inputEnc1, "input1");

  // now let's create a second ciphertext:
  cout << "Creating the second tile tensor that is also an input to the circuit"
       << endl;
  CTileTensor inputEnc2(he);
  encoder.encodeEncrypt(inputEnc2, shape, input2);
  cout << "Label the tile tensor as an input so we can attach a tile tensor to "
          "it "
          "when we run it later"
       << endl;
  he.labelCtxt(inputEnc2, "input2");

  // now let's add them together:
  cout << "Add the 2 tile tensors creating addition gates that add the two "
          "tensors"
       << endl;
  inputEnc1.add(inputEnc2);

  // c1 is the output of the circuit, so we label it as such to be able to read
  // it later
  cout << "Label the output so we'll be able to retrieve the output later by "
          "the label. This also records the shape of the tensor"
       << endl;
  he.labelCtxt(inputEnc1, "output");
  he.flush();
}

void runCircuit5(const HeContext& he, const Circuit& circ)
{
  TTEncoder encoder(he);

  // We must choose the same shape for the tensors again,
  // So it will fit the recorded circuit.
  // The elements of the tensors can change of course.
  DoubleTensor input1 = createRandTensor({3, 3, 5});
  DoubleTensor input2 = createRandTensor({3, 3, 5});

  // The tile tensor shape should as well be the same as the one we used when we
  // recorded the circuit. Different shapes might change the circuit structure.
  TTShape shape({2, 2, he.slotCount() / 4});

  cout << "Encode and encrypt 2 ciphertexts" << endl;
  CTileTensor inputEnc1(he);
  encoder.encodeEncrypt(inputEnc1, shape, input1);
  CTileTensor inputEnc2(he);
  encoder.encodeEncrypt(inputEnc2, shape, input2);

  CtxtCacheMem inputs;
  cout << "Attach the 2 TileTensors to labels to be input for the circuit"
       << endl;
  inputs.setByLabel(string("input1"), inputEnc1);
  inputs.setByLabel(string("input2"), inputEnc2);

  Runner runner(he, circ);
  cout << "Add the inputs to the circuit runner" << endl;
  runner.addWritableCache(&inputs);

  cout << "Run the circuit" << endl;
  runner.run();

  cout << "Get the output from the circuit" << endl;
  CTileTensor output(he);
  runner.getCTileTensorByLabel("output", output);

  cout << "Shape of the resulting tile tensor: " << output.getShape() << endl;

  DoubleTensor outputDec;
  outputDec = encoder.decryptDecodeDouble(output);
  cout << "Shape of the resulting tensor: " << outputDec.getShape() << endl;
  cout << "Sample element (3,0,0) of the two inputs and output:" << endl;
  cout << input1.at(3, 0, 0) << " + " << input2.at(3, 0, 0) << " = "
       << outputDec.at(3, 0, 0) << endl;
}

extern shared_ptr<HeContext> he2;

void tut_5_run_tile_tensors()
{
  cout << "Create a circuit that computes c1+c2. Then attach 2 ciphertexts to "
          "the input of the circuit, run the the circuit and read the output"
       << endl;
  CircuitContext he;
  he.init(HeConfigRequirement::insecure(he2->slotCount()));

  cout << "Create a circuit structure by executing a code with CircuitContext "
          "context"
       << endl;
  buildCircuit5(he);

  cout << "Get the circuit object" << endl;
  Circuit circ = he.getCircuit();

  cout << "Run the circuit with a different context" << endl;
  runCircuit5(*he2, circ);
}
