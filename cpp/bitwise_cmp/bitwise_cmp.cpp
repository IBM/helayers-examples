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

#include "helayers/hebase/Encoder.h"
#include "helayers/hebase/openfhe/OpenFheCkksContext.h"
#include "helayers/hebase/mockup/MockupContext.h"
#include "helayers/hebase/BitwiseEvaluator.h"

using namespace std;
using namespace helayers;

shared_ptr<HeContext> he;

void help()
{
  cout << "Usage:  [--mockup]" << endl;
  cout << "--mockup\tuse mockup" << endl;
  exit(1);
}

void encodeBinNumber(vector<shared_ptr<CTile>>& c, int a, int bits)
{
  Encoder encoder(*he);

  for (int b = 0; b < bits; ++b) {
    shared_ptr<CTile> ctxt = make_shared<CTile>(*he);
    encoder.encodeEncrypt(*ctxt, a & 1);
    c.push_back(ctxt);
    a = a >> 1;
  }
}

int main(int argc, char* argv[])
{
  bool mockup = false;
  int i = 1;
  while (i < argc) {
    string arg = argv[i++];
    if (arg == "--mockup")
      mockup = true;
    else
      help();
    ++i;
  }

  HeConfigRequirement req;
  if (mockup) {
    he = make_shared<MockupContext>();
    req = HeConfigRequirement::insecure(pow(2, 16), // numSlots
                                        13,         // multiplicationDepth
                                        24,         // fractionalPartPrecision
                                        5           // integerPartPrecision
    );
    req.bootstrappable = true;
    req.bootstrapConfig = BootstrapConfig();
    req.bootstrapConfig->targetChainIndex = 12;
    req.bootstrapConfig->minChainIndexForBootstrapping = 3;
  } else {
    he = make_shared<OpenFheCkksContext>();
    req = HeConfigRequirement(pow(2, 15), // numSlots
                              20,         // multiplicationDepth
                              28,         // fractionalPartPrecision
                              6           // integerPartPrecision
    );
    req.bootstrappable = true;
    req.automaticBootstrapping = true;
  }
  std::cout << "Initializing HeContext . . ." << std::endl;
  he->init(req);
  std::cout << "Finished context initialization" << std::endl;

  bool error = false;
  int bitNum = 4;
  for (int a = 0; a < (1 << 4); ++a) {
    vector<shared_ptr<CTile>> aEnc;
    encodeBinNumber(aEnc, a, bitNum);
    for (int b = 0; b < (1 << 4); ++b) {
      cout << "Comparing " << a << " and " << b << endl;
      vector<shared_ptr<CTile>> bEnc;
      encodeBinNumber(bEnc, b, bitNum);

      CTile eqEnc(*he);
      CTile lessEnc(*he);
      BitwiseEvaluator::compareBitArrays(aEnc, bEnc, eqEnc, lessEnc);

      Encoder encoder(*he);

      double eq = encoder.decryptDecodeDouble(eqEnc)[0];
      double less = encoder.decryptDecodeDouble(lessEnc)[0];
      if ((a == b) ^ (abs(eq) > 0.1)) {
        cout << "Error isEq(" << a << ", " << b << ") = " << eq << endl;
        error = true;
      }
      if ((a < b) ^ (abs(less) > 0.1)) {
        cout << "Error isLess(" << a << ", " << b << ") = " << less << endl;
        error = true;
      }
    }
  }

  if (!error)
    cout << "No errors" << endl;
}