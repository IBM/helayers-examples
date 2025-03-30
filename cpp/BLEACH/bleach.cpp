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
using namespace std::chrono;

#include "helayers/math/TTConvolutionInterleaved.h"
#include "helayers/math/TTEncoder.h"
#include "helayers/math/TTFunctionEvaluator.h"
#include "helayers/hebase/openfhe/OpenFheCkksContext.h"
#include "helayers/hebase/mockup/MockupContext.h"

using namespace helayers;
using namespace std;

std::string presetName = "fhe";
int numberOfBits = 8;

void benckmarkParseBits(HeContext& he)
{
  TTEncoder enc(he);
  TTFunctionEvaluator fe(he);

  int val = 0x45;
  cout << "Parsing bits of " << val << endl;

  // We assume the CKKS error is at most e_ckks
  double e_ckks = 0.000001;
  // We assume the input is at most (0.5-alpha) away from an integer
  double alpha = 0.4;
  // We want the output to be at most beta away from an integer
  double beta = 0.001;

  TTShape shape{1, he.slotCount()};
  shape.setOriginalSizes({1, 1});
  shape.getDim(1).setNumDuplicated(he.slotCount());
  CTileTensor c(he);
  DoubleTensor valDT;
  valDT.init(shape.getOriginalSizes(), static_cast<double>(val));
  enc.encodeEncrypt(c, shape, valDT);

  {
    for (int bit = numberOfBits - 1; bit >= 0; --bit) {
      auto start = high_resolution_clock::now();

      double alpha_i = alpha / (1 << (numberOfBits - bit)); // Alg3 Line4
      double beta_i = min(beta, alpha_i); // Alg3 Line5
      alpha_i = alpha_i / (1 << bit);     // Alg2 Line3
      beta_i = 2 * (beta_i - e_ckks);     // Alg2 Line4
      CTileTensor b(c);
      b.multiplyScalar(1.0 / (1 << bit));   // Alg2 Line2
      b.addScalar(-1.0 + 0.5 / (1 << bit)); // Alg2 Line2
      int giantStep = log(1.0 / alpha_i);
      int babyStep = log(1.0 / beta_i);
      // Use HElayer's sign implementation
      fe.signInPlace(b, giantStep, babyStep, 1, true);

      if (bit > 0) {
        // run the equivalent of b.multiply(1 << bit)
        for (int i = 0; i < bit; ++i)
          b.add(b);
        c.sub(b);
      }

      auto end = high_resolution_clock::now();
      auto duration = duration_cast<seconds>(end - start);

      int bitValue = 1 << bit;
      double plainBit = enc.decryptDecodeDouble(b).at(0);
      if ((abs(plainBit) > 0.5) && (abs(plainBit - bitValue) > 0.5))
        throw runtime_error("bit has an invalid value");

      int plainBitInt = (abs(plainBit) < 0.5) ? 0 : 1;

      cout << "Extracting bit " << bit << " (out of " << numberOfBits
           << ") took " << duration.count() << " seconds. Bit = " << plainBitInt
           << endl;
    }
  }
}

int main(int argc, char** argv)
{
  int i = 1;
  while (i < argc) {
    string arg = argv[i++];
    if (arg == "--bits")
      numberOfBits = atoi(argv[i++]);
    else if (arg == "--preset")
      presetName = argv[i++];
    else {
      cerr << "Usage:" << endl;
      cerr << "  --preset <preset name>" << endl;
      cerr << "  --bits <number of bits>" << endl;
      throw runtime_error(string("Unknown argument: ") + arg);
    }
  }

  shared_ptr<HeContext> he;
  HeConfigRequirement reqParseBits;
  if (presetName == "mockup") {
    he = make_shared<MockupContext>();
    reqParseBits = HeConfigRequirement::insecure(pow(2, 16), // numSlots
                                       35,         // multiplicationDepth
                                       53,         // fractionalPartPrecision
                                       5           // integerPartPrecision
    );

    reqParseBits.bootstrapConfig = BootstrapConfig();
    reqParseBits.bootstrapConfig->minChainIndexForBootstrapping = 0;
    reqParseBits.bootstrapConfig->targetChainIndex = 35;
  } else if (presetName == "fhe") {
    he = make_shared<OpenFheCkksContext>();
    reqParseBits = HeConfigRequirement(pow(2, 16), // numSlots
                                       35,         // multiplicationDepth
                                       53,         // fractionalPartPrecision
                                       5           // integerPartPrecision
    );
  } else {

    cerr << "Unsupported preset " << presetName << endl;
    cerr << "Supported presets:" << endl;
    cerr << "   mockup" << endl;
    cerr << "   fhe" << endl;

    exit(1);
  }

  reqParseBits.bootstrappable = true;
  std::cout << "Initializing HeContext . . ." << std::endl;
  he->init(reqParseBits);
  he->setAutomaticBootstrapping(true);

  benckmarkParseBits(*he);
}
