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

#include <iostream>
#include <memory>
#include "helayers/hebase/hebase.h"
#include "helayers/hebase/mockup/MockupContext.h"
#include "helayers/hebase/seal/SealCkksContext.h"

#include "tutorial_circuit.h"

using namespace std;
using namespace helayers;

shared_ptr<HeContext> he2;

int main(int argc, char** argv)
{
  string scheme = "";
  string arg = "";

  if (argc == 3) {
    arg = argv[1];
    scheme = argv[2];
  } else if (argc == 2) {
    arg = argv[1];
    scheme = "mockup";
  } else
    help(argv[0]);

  int slotCount = 4 * 1024;
  if (scheme == "mockup") {
    he2 = make_shared<MockupContext>();
    he2->init(HeConfigRequirement::insecure(slotCount));
  } else if (scheme == "seal") {
    HeConfigRequirement requirement;
    requirement.numSlots = slotCount;
    requirement.multiplicationDepth = 2;
    requirement.fractionalPartPrecision = 40;
    requirement.integerPartPrecision = 20;
    requirement.securityLevel = 128;
    he2 = make_shared<SealCkksContext>();
    he2->init(requirement);
  } else {
    throw runtime_error("Unknown scheme " + scheme);
  }

  if (arg == "1")
    tut_1_basics_log();
  else if (arg == "2")
    tut_2_basics_run();
  else if (arg == "3")
    tut_3_run_plaintext();
  else if (arg == "4")
    tut_4_run_param();
  else if (arg == "5")
    tut_5_run_tile_tensors();
  else {
    throw runtime_error("Unknown tutorial " + arg);
  }
}

void help(const char* cmd)
{
  cout << "Usage: " << cmd << " <test number> <scheme>" << endl;
  cout << "Supported schemes:" << endl;
  cout << "\tmockup\ta fast mockup scheme that does not involve encryption"
       << endl;
  cout << "\tseal\tSeal's implementation for CKKS" << endl;
  cout << endl;
  cout << "\t1\tbasic example that logs a circuit" << endl;
  cout << "\t2\tbasic example that creates a circuit and then runs it" << endl;
  cout << "\t3\tbasic example that creates a circuit that uses a plaintext"
       << endl;
  cout << "\t4\tbasic example that creates a circuit that uses a encrypted "
          "parameters"
       << endl;
  cout << "\t5\tbasic example that creates a circuit that uses tile tensors"
       << endl;
  exit(1);
}
