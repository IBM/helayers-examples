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

#include <iostream>
#include <vector>
#include <cassert>
#include "helayers/hebase/Encoder.h"
#include "helayers/hebase/lattigo/LattigoContext.h"

using namespace std;
using namespace helayers;

HeConfigRequirement defaultKeysConfigReq()
{
  HeConfigRequirement req;
  req.securityLevel = 128;
  req.numSlots = pow(2, 12);
  req.multiplicationDepth = 3;
  req.fractionalPartPrecision = 30;
  req.integerPartPrecision = 10;
  req.publicFunctions.hierarchicalKeys = true;
  req.publicFunctions.hierarchicalRatio = 0.7; // reduce network trafic by ~70%
  return req;
}

HeConfigRequirement customKeysConfigReq()
{
  HeConfigRequirement req;
  req.securityLevel = 128;
  req.numSlots = pow(2, 12);
  req.multiplicationDepth = 3;
  req.fractionalPartPrecision = 30;
  req.integerPartPrecision = 10;
  req.publicFunctions.hierarchicalKeys = true;
  req.publicFunctions.hierarchicalRatio = 0.8; // reduce network trafic by ~80%
  req.publicFunctions.rotate = CUSTOM_ROTATIONS;
  req.publicFunctions.rotationSteps = {
      1, 2, 3}; // generates only custom rotations with rotation-steps 1,2,3
  return req;
}

HeConfigRequirement bootstrapKeysConfigReq()
{
  HeConfigRequirement req;
  req.securityLevel = 128;
  req.numSlots = pow(2, 16);
  req.multiplicationDepth = 3;
  req.fractionalPartPrecision = 30;
  req.integerPartPrecision = 10;
  req.bootstrappable = true;
  req.publicFunctions.hierarchicalKeys = true;
  return req;
}

HeConfigRequirement bootstrapAndCustomKeysConfigReq()
{
  HeConfigRequirement req;
  req.securityLevel = 128;
  req.numSlots = pow(2, 16);
  req.multiplicationDepth = 3;
  req.fractionalPartPrecision = 30;
  req.integerPartPrecision = 10;
  req.bootstrappable = true;
  req.publicFunctions.hierarchicalKeys = true;
  req.publicFunctions.hierarchicalRatio = 0.8;
  req.publicFunctions.rotate = CUSTOM_ROTATIONS;
  req.publicFunctions.rotationSteps = {
      1, 2, 3}; // generates only custom rotations with rotation-steps 1,2,3
  return req;
}

void run(HeConfigRequirement req)
{
  ///////////////////// client side /////////////////////
  shared_ptr<HeContext> he = make_shared<LattigoContext>();
  cout << "Init context" << endl;
  he->init(req);
  Encoder enc(*he);
  int minChainIndex = he->getMinChainIndexForBootstrapping();

  cout << "Assert that rotation key for 2 steps does not exist." << endl;
  assert(!he->isRotationExist(2));

  cout << "Generate ciphertexts." << endl;
  vector<int> vecRot(he->slotCount());
  for (int i = 0; i < vecRot.size(); i++) {
    vecRot.at(i) = i % 100;
  }
  CTile ctRot(*he);
  enc.encodeEncrypt(ctRot, vecRot);
  CTile ctBts(*he);
  vector<double> vecBts(he->slotCount(), 0.5); //
  enc.encodeEncrypt(ctBts, vecBts, minChainIndex);
  cout << "ctRot original values: " << vecRot[0] << ", " << vecRot[1] << ", "
       << vecRot[2] << ", " << vecRot[3] << ", " << vecRot[4] << endl;

  cout << "Serializeing context and ciphertexts to disk" << endl;
  string contextFile = "./context.tmp";
  string ctRotFile = "./ctRot.tmp";
  string ctBtsFile = "./ctBts.tmp";
  he->saveToFile(contextFile);
  ctRot.saveToFile(ctRotFile);
  ctBts.saveToFile(ctBtsFile);

  ///////////////////// server side /////////////////////
  cout << "Deserialized context (without secret key) " << endl;
  shared_ptr<HeContext> he2 = make_shared<LattigoContext>();
  he2->loadFromFile(contextFile);
  if (he2->getPublicFunctions().hierarchicalKeys) {
    cout << "Deserialized ciphertext" << endl;
    CTile ctRot2(*he2);
    ctRot2.loadFromFile(ctRotFile);

    cout << "Generating rotations keys homomorphicaly" << endl;
    he2->genRotKeysFromHierarchicalKeys();

    assert(he2->isRotationExist(2));
    ctRot2.rotate(2);
    vector<double> vecRot = enc.decryptDecodeDouble(ctRot2);
    cout << "rotated values: " << vecRot[0] << ", " << vecRot[1] << ", "
         << vecRot[2] << endl;
  }

  if ((*he2).getBootstrappable()) {
    cout << "Read ciphertext from disk" << endl;
    CTile ctBts2(*he2);
    ctBts2.loadFromFile(ctBtsFile);

    vector<double> vecBts2 = enc.decryptDecodeDouble(ctBts2);
    cout << "original values: " << vecBts2[0] << ", " << vecBts2[1] << ", "
         << vecBts2[2] << endl;
    assert(ctBts2.getChainIndex() == minChainIndex);

    cout << "Run bootstrapping" << endl;
    ctBts2.bootstrap();
    vecBts2 = enc.decryptDecodeDouble(ctBts2);

    assert(ctBts2.getChainIndex() == he->getChainIndexAfterBootstrapping());
    cout << "After bootstrapping values: " << vecBts2[0] << ", " << vecBts2[1]
         << ", " << vecBts2[2] << endl;
  }
}

int main()
{
  int choice;
  do {
    cout << "Menu:\n";
    cout << "1. Generating default powers of 2 rotation keys homomorphicaly by "
            "ratio.\n";
    cout << "2. Generating custom rotation keys homomorphicaly by "
            "ratio.\n";
    cout << "3. Generating bootstrap and default rotation keys.\n";
    cout << "4. Generating bootstrap and custom rotation keys.\n";
    cout << "0. Exit\n";

    std::cout << "Enter your choice: ";
    std::cin >> choice;

    switch (choice) {
    case 1:
      run(defaultKeysConfigReq());
      break;
    case 2:
      run(customKeysConfigReq());
      break;
    case 3:
      run(bootstrapKeysConfigReq());
      break;
    case 4:
      run(bootstrapAndCustomKeysConfigReq());
      break;
    case 0:
      std::cout << "Exiting the program.\n";
      break;
    default:
      std::cout << "Invalid choice. Please try again.\n";
    }

  } while (choice != 0);
}
