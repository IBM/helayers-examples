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

// See more information about this demo in the readme file.

#include "helayers/hebase/seal/SealCkksContext.h"
#include "helayers/hebase/hebase.h"

using namespace std;
using namespace helayers;

void tut_3_clientEncrypt();
void tut_3_serverRun();
void tut_3_clientDecrypt();

const std::string outDir = getExamplesOutputDir();

// This tutorial demonstrates I/O operations
void tut_3_io()
{

  // As in tut_1_basics, let's choose the same HE CKKS setup
  HeConfigRequirement requirement;
  requirement.numSlots = 4096;
  requirement.multiplicationDepth = 2;
  requirement.fractionalPartPrecision = 40;
  requirement.integerPartPrecision = 20;
  requirement.securityLevel = 128;
  shared_ptr<HeContext> hePtr = make_shared<SealCkksContext>();
  hePtr->init(requirement);

  // Let's also make sure we have enough security
  always_assert(hePtr->getSecurityLevel() >= 128);

  // Let's create a clean directory for this tutorial's output files:
  FileUtils::createCleanDir(outDir);

  // This call bundles all required info into a given file.
  // ** execpt the secret key **.
  cout << "Saving context . . ." << endl;
  hePtr->saveToFile(outDir + "/context.bin");

  // Save the secret key into a separate file
  cout << "Saving  secret key . . ." << endl;
  hePtr->saveSecretKeyToFile(outDir + "/secretKey.bin");

  // All the above methods have a version that writes to an ostream object.

  // We'll now simulate client preparing some encrypted data:
  tut_3_clientEncrypt();

  // The server process it without seeing the encrypted data:
  tut_3_serverRun();

  // And client decrypting the results:
  tut_3_clientDecrypt();
}

void tut_3_clientEncrypt()
{

  // First we'll load the context (which only includes the public key).
  cout << "Loading client context . . ." << endl;
  shared_ptr<HeContext> hePtr = loadHeContextFromFile(outDir + "/context.bin");

  // For convenience
  HeContext& he = *hePtr;

  // we'll encrypt the vector (1,2,3).
  vector<double> data{1, 2, 3};
  cout << "Encrypting:";
  for (int i = 0; i < 3; ++i)
    cout << " " << data[i];
  cout << endl;

  Encoder encoder(he);
  CTile c(he);
  encoder.encodeEncrypt(c, data);

  // And save it to file:
  cout << "Saving encrypted data . . ." << endl;
  c.saveToFile(outDir + "/data.bin");
}

void tut_3_serverRun()
{
  // Now we'll load the server file, not having the secret key
  cout << "Loading server context . . ." << endl;
  shared_ptr<HeContext> hePtr = loadHeContextFromFile(outDir + "/context.bin");

  // For convenience
  HeContext& he = *hePtr;

  // Let's load our data
  CTile c(he);
  cout << "Loading encrypted data . . ." << endl;
  c.loadFromFile(outDir + "/data.bin");

  cout << "Processing . . ." << endl;
  // square it
  c.square();

  // And save it to file:
  cout << "Saving encrypted data . . ." << endl;
  c.saveToFile(outDir + "/data2.bin");

  // Note that having no secret key, the server cannot decrypt:
  Encoder encoder(he);
  try {
    vector<double> res = encoder.decryptDecodeDouble(c);
  } catch (exception& e) {
    cout << "Expected exception occurred: " << e.what() << endl;
  }

  // We can however add the secret key now if we want to do some debugging
  he.loadSecretKeyFromFile(outDir + "/secretKey.bin");
  // we can now decrypt, or do asserts:
  encoder.assertEquals(c, "test results", vector<double>{1, 4, 9});
  cout << "Assert passed!" << endl;
}

void tut_3_clientDecrypt()
{
  // Finally, let's load client side context again
  cout << "Loading client context . . ." << endl;
  shared_ptr<HeContext> hePtr = loadHeContextFromFile(outDir + "/context.bin");
  // And we load the secret key separately, to be able to decrypt.
  cout << "Loading secret key . . ." << endl;
  hePtr->loadSecretKeyFromFile(outDir + "/secretKey.bin");

  // For convenience
  HeContext& he = *hePtr;

  // Let's load our processed data
  CTile c(he);
  cout << "Loading processed encrypted data . . ." << endl;
  c.loadFromFile(outDir + "/data2.bin");

  Encoder encoder(he);
  vector<double> res = encoder.decryptDecodeDouble(c);
  cout << "Results:";
  for (int i = 0; i < 3; ++i)
    cout << " " << res[i];
  cout << endl;
}
