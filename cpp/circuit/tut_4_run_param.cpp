
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
#include "helayers/hebase/mockup/CircuitContext.h"
#include "helayers/circuit/Runner.h"

using namespace std;
using namespace helayers;
using namespace circuit;

void buildCircuit4(CircuitContext& he)
{
  Encoder encoder(he);

  // Let's create our 2 ciphertexts

  // Create the first ciphertext
  cout << "Creating the first ciphertext that is the input to the circuit"
       << endl;
  vector<double> vals1{1.0, 2.0, 3.0};
  CTile c1(he);
  encoder.encodeEncrypt(c1, vals1);
  // since this is an input to the circuit we label to be able to attach a value
  // to it later we label this ctxt as input1
  cout << "Label the ciphertext as an input so we can attach ciphertexts to it "
          "when we run it later"
       << endl;
  he.labelCtxt(c1, "input1");

  // now let's create a second ciphertext:
  cout << "Create a second ciphertext. That will be a secret parameter to the "
          "circuit"
       << endl;
  vector<double> vals2{4.0, 5.0, 6.0};
  CTile c2(he);
  encoder.encodeEncrypt(c2, vals2);
  // we do not label this ctxt as input2 this is an encrypted parameter of the
  // circuit
  cout << "This ciphertext is not labeled" << endl;

  // now let's add them together:
  cout << "Add the 2 ciphertexts creating an addition gate" << endl;
  c1.add(c2);

  // c1 is the output of the circuit, so we label it as such to be able to read
  // it later
  cout << "Label the output so we'll be able to retrieve the output later by "
          "the label"
       << endl;
  he.labelCtxt(c1, "output");
  he.flush();
}

void runCircuit4(const HeContext& he,
                 const Circuit& circ,
                 const CtxtCacheMem& constants)
{
  Encoder encoder(he);

  cout << "Encode and encrypt a ciphertext" << endl;
  vector<double> vals1{2.0, 3.0, 4.0};
  CTile c1(he);
  encoder.encodeEncrypt(c1, vals1);

  CtxtCacheMem inputs;
  cout << "Attach the ciphertext to the label to be input for the circuit"
       << endl;
  inputs.setByLabel("input1", c1);

  Runner runner(he, circ);
  cout << "Add the input to the circuit runner" << endl;
  runner.addWritableCache(&inputs);

  cout << "Add the constants to the circuit runner" << endl;
  runner.addReadOnlyCache(&constants);

  cout << "Run the circuit" << endl;
  runner.run();

  cout << "Get the output from the circuit using the output label" << endl;
  CTile output = runner.getCTileByLabel("output");

  vector<double> outputDec;
  outputDec = encoder.decryptDecodeDouble(output);

  cout << "(" << vals1[0] << ", " << vals1[1] << ", " << vals1[2] << ")"
       << " + (4, 5, 6) = "
       << " = "
       << "(" << outputDec[0] << ", " << outputDec[1] << ", " << outputDec[2]
       << ")" << endl;
}

extern shared_ptr<HeContext> he2;

void tut_4_run_param()
{
  cout << "Create a circuit that computes c1+c2. Then attach 2 ciphertexts to "
          "the input of the circuit, run the the circuit and read the output"
       << endl;
  CircuitContext he;
  auto req = HeConfigRequirement::insecure(he2->slotCount());
  req.multiplicationDepth = he2->getTopChainIndex();
  he.init(req);

  cout << "Create a circuit structure by executing a code with CircuitContext "
          "context"
       << endl;
  buildCircuit4(he);

  cout << "Get the circuit object" << endl;
  Circuit circ = he.getCircuit();
  cout << "Get the parameters of the circuit" << endl;
  circuit::CtxtCacheMem constants;
  circ.saveUnlabeledInputs(constants);
  constants.setReadOnly();

  cout << "Encrypt the constants with the context we will run with" << endl;
  constants.encrypt(*he2);

  cout << "Run the circuit with a different context" << endl;
  runCircuit4(*he2, circ, constants);
}
