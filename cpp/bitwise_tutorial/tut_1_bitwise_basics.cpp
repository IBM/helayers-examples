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

#include <cassert>
#include <random>
#include <bitset>

#include "helayers/hebase/helib/HelibBitwiseBgvContext.h"
#include "helayers/hebase/hebase.h"

// This tutorial shows a basic usage example of using the hebase layer.
// For our basic example, we'll configure HElib as the underlying HE library
// and BGV as the scheme.

using namespace std;
using namespace helayers;

void tut_1_run(HeContext& he);

// Specifies the number of bits used when representing a ciphertext as a
// bitwise number
const int bitSize = 16;
// Specifies the number of bits the resulting output ciphertexts should contain
const int outSize = 32;

void tut_1_bitwise_basics()
{
  // First, we initialize the library
  // As before, this performs all the required setup.
  // NOTE: this also generates a public-private key pair,
  // so it's ready now for use.
  long m = 4095; // Cyclotomic polynomial - defines \phi(m)
  long r = 1;    // Hensel lifting
  long L = 500;  // Number of bits of the modulus chain
  long c = 2;    // Max circuit depth

  // Factorisation of m required for bootstrapping
  std::vector<long> mvec = {7, 5, 9, 13};

  // Generating set of Zm* group.
  std::vector<long> gens = {2341, 3277, 911};

  // Orders of the previous generators.
  std::vector<long> ords = {6, 4, 6};
  HelibBitwiseBgvContext helib;
  helib.initWithBootStrapping(m, r, L, c, mvec, gens, ords);

  // Scale applied to encrypted numbers. Affects the precision of the
  // calculations. Because in this tutorial we only perform calculations on
  // integers, we set the scale to 1.
  double scale = 1;

  helib.setDefaultScale(scale);
  helib.setNumBits(bitSize);

  // This will print the details of the underlying scheme:
  // name, configuration params, and security level.
  cout << "Using scheme: " << endl;
  helib.printSignature(cout);

  // This configuration is actually not secure.
  // But we use it only for demo purposes.
  // always_assert(helib.getSecurityLevel() >= 128);

  // we'll send our initialized context as input to this run()
  // function to do some work with it.
  tut_1_run(helib);
}

void tut_1_run(HeContext& he)
{
  // Note that this function receives a class of type HeContext.
  // It's an abstract class that HelibBitwiseBgvContext is one of the many
  // classes that inherit from it.
  // This allows writing run() in a scheme oblivious way.

  // Throughout this tutorial, we will generate three random numbers a, b, c,
  // and encrypt them under BGV.
  // Then, we will calculate a * b + c , and decrypt the result.
  // Next, calculate a + b + c, and decrypt the result.
  // Finally, calculate the hamming weight of a, and decrypt the result.  Note
  // that the hamming weight is defined as the count of non-zero bits.

  // Let's create three bitwise ciphertexts.
  // We'll first pcik three random integers in the range [0, 10000].

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distribution(0, 10000);

  int a = distribution(gen);
  int b = distribution(gen);
  int c = distribution(gen);

  std::cout << "Pre-encryption data:" << std::endl;
  std::cout << "a = " << a << std::endl;
  std::cout << "b = " << b << std::endl;
  std::cout << "c = " << c << std::endl;

  // To encrypt the three numbers, we need an encoder . . .
  Encoder encoder(he);

  // And three CTile objects - these are our ciphertext objects.
  CTile ca(he), cb(he), cc(he);

  // We'll now encrypt "a", "b" and "c" into "ca", "cb" and "cc", respectively.
  // In HE, encryption actually involves two steps: encode, then encrypt.
  // encoder.encodeEncrypt() method does both.
  // Note: several numbers can be encoded across the slots of each ciphertext
  // which would result in several parallel slot-wise operations.
  // For simplicity we place the same data into each slot of each ciphertext,
  // printing out only the back of each vector.
  encoder.encodeEncrypt(ca, vector<int>(he.slotCount(), a));
  encoder.encodeEncrypt(cb, vector<int>(he.slotCount(), b));
  encoder.encodeEncrypt(cc, vector<int>(he.slotCount(), c));

  // To perform operations on bitwise ciphertexts, we need a
  // "BitwiseEvaluator" . . .
  BitwiseEvaluator evaluator(he);

  // Each ciphertext has slotCount slots where each operation is applied in SIMD
  int slotCount = he.slotCount();
  cout << "Each ciphertext has " << slotCount << " slots" << endl;

  // Now, we will set the three bitwise ciphertexts, "ca", "cb" and "cc", just
  // created, to be considered as unsigned binary numbers
  evaluator.setIsSigned(ca, false);
  evaluator.setIsSigned(cb, false);
  evaluator.setIsSigned(cc, false);

  // Now, let's multiply "ca" with "cb", and store the results into "cres1"
  CTile cres1(he);
  cres1 = evaluator.multiply(ca, cb, outSize);

  // Let's add "cc" to "cres1", just computed.
  cres1 = evaluator.add(cres1, cc, outSize);

  // Let's decrypt and verify that cres1 is equal to ((a * b) + c).
  // Decryption involves two steps: decryption and decoding.
  // encoder.decryptDecodeInt() function does both.
  vector<int> p_res1 = encoder.decryptDecodeInt(cres1);
  cout << "computing (a * b) + c homomorphically" << endl;
  cout << p_res1[0] << " = (" << a << " * " << b << ") + " << c << endl;

  cout << endl;

  // let's compute ("ca" + "cb" + "cc"), and store the result into "cres2".
  CTile cres2(he);
  cres2 = evaluator.add(ca, cb, bitSize + 1);
  cres2 = evaluator.add(cres2, cc, bitSize + 2);

  // Let's decrypt and verify that cres2 is equal to (a + b + c).
  // Decryption involves two steps: decryption and decoding.
  // encoder.decryptDecodeInt() function does both.
  vector<int> p_res2 = encoder.decryptDecodeInt(cres2);
  cout << "computing a + b + c homomorphically" << endl;
  cout << p_res2[0] << " = " << a << " + " << b << " + " << c << endl;

  // Let's calculate hamming("ca"), and store the result into "cres3".
  // Note that hamming("ca"), returns the hamming weight of "ca", which is
  // the count of non-zero bits.
  CTile cres3(he);
  cres3 = evaluator.hamming(ca);

  // Let's decrypt and verify that cres3 is equal to poopcnt(a).
  // Decryption involves two steps: decryption and decoding.
  // encoder.decryptDecodeInt() function does both.
  vector<int> p_res3 = encoder.decryptDecodeInt(cres3);
  cout << "computing hamming(a)" << endl;
  cout << p_res3[0] << " = hamming(" << std::bitset<bitSize>(a) << ")" << endl;
}