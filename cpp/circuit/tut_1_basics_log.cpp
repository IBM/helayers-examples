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

using namespace std;
using namespace helayers;
using namespace circuit;

void buildCircuit1(CircuitContext& he)
{
  Encoder encoder(he);

  // Let's create our 2 ciphertexts
  cout << "Creating Ctxt1" << endl;
  vector<double> vals1{1.0, 2.0, 3.0};
  CTile c1(he);
  encoder.encodeEncrypt(c1, vals1);
  // since this is an input to the circuit we label to be able to attach a value
  // to it later we label this ctxt as input1
  he.labelCtxt(c1, "input1");

  // now let's create a second ciphertext:
  vector<double> vals2{4.0, 5.0, 6.0};
  CTile c2(he);
  encoder.encodeEncrypt(c2, vals2);
  // we label this ctxt as input2
  he.labelCtxt(c2, "input2");

  // now let's add them together:
  c1.add(c2);
  // although c1 is the same variable from the circuit's point of view it is
  // different because now it correspond to a different node.

  // Copy c2 to c3.
  CTile c3(c2);
  // Now c3 is a different variable than c2, but they both refer to the same
  // node in the circuit.

  // perform some more computation
  c3.rotate(-2);
  c2.multiply(c3);

  // Let's reencrypt c1.
  encoder.encodeEncrypt(c1, vals1);
  // Now c1 is a different input node in the circuit. We label it.
  he.labelCtxt(c1, "input3");

  c1.square();
  c1.negate();

  c1.add(c2);

  // c1 is the output of the circuit, so we label it as such to be able to read
  // it later
  he.labelCtxt(c1, "output");
  he.flush();
}

void tut_1_basics_log()
{
  cout << "We start by creating a CircuitContext object." << endl
       << "HE computations running with this context are not executed, but "
       << endl
       << "are recorded as a circuit instead." << endl;
  CircuitContext he;
  he.init(HeConfigRequirement::insecure(1024));

  cout << "Now we'll record a circuit by executing a code with this "
          "CircuitContext "
          "context"
       << endl;
  buildCircuit1(he);

  cout << "Get the circuit object" << endl;
  Circuit circ = he.getCircuit();

  cout << "Write the circuit in GateList output format:" << endl << endl;
  circ.writeGateList(cout);
  cout << endl;
}
