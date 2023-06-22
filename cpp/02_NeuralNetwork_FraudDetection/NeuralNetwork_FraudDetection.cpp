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

#include "helayers/ai/AiGlobals.h"
#include "helayers/ai/DatasetPlain.h"
#include "helayers/ai/HeProfileOptimizer.h"
#include "helayers/ai/nn/NeuralNetPlain.h"
#include "helayers/ai/nn/NeuralNet.h"
#include "helayers/hebase/hebase.h"
#include "helayers/hebase/HebaseGlobals.h"
#include "helayers/hebase/seal/SealCkksContext.h"
#include "helayers/hebase/utils/MemoryUtils.h"
#include "helayers/math/DoubleTensor.h"
#include "helayers/math/MathGlobals.h"
#include "helayers/math/TensorUtils.h"
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
using namespace helayers;

const std::string outDir = getExamplesOutputDir();
const string nnFile = outDir + "/nnFile.bin";
const string samplesFile = outDir + "/samples.bin";
const string contextFile = outDir + "/context.bin";
const string predictionsFile = outDir + "/predictions.bin";

void assesResults(const DoubleTensor& predictedLabels,
                  const DoubleTensor& origLabels)
{
  DimInt batchSize = predictedLabels.getDimSize(0);
  int truePositives = 0, falsePositives = 0, trueNegatives = 0,
      falseNegatives = 0;
  for (DimInt i = 0; i < batchSize; i++) {
    int predicted = (round(predictedLabels.at(i)) > 0.5);
    int orig = round(origLabels.at(i));

    truePositives += predicted * orig;
    falsePositives += predicted * (1 - orig);
    trueNegatives += (1 - predicted) * (1 - orig);
    falseNegatives += (1 - predicted) * orig;
  }

  double precision = ((double)truePositives / (truePositives + falsePositives));
  double recall = ((double)truePositives / (truePositives + falseNegatives));
  double f1Score = (2 * precision * recall) / (precision + recall);

  cout << endl;
  cout << "|---------------------------------------------|" << endl;
  cout << "|                       |    True condition   |" << endl;
  cout << "|                       ----------------------|" << endl;
  cout << "|                       | Positive | Negative |" << endl;
  cout << "|---------------------------------------------|" << endl;
  cout << "| Predicted  | Positive |" << setw(8) << truePositives << "  |"
       << setw(8) << falsePositives << "  |" << endl;
  cout << "|            |--------------------------------|" << endl;
  cout << "| condition  | Negative |" << setw(8) << falseNegatives << "  |"
       << setw(8) << trueNegatives << "  |" << endl;
  cout << "|---------------------------------------------|" << endl;
  cout << endl;
  cout << "Precision: " << precision << endl;
  cout << "Recall: " << recall << endl;
  cout << "F1 score: " << f1Score << endl;
}

// -- Neural Network Inference for Fraud Detection Using FHE --

// -- Introduction --

// This example demonstrates a use case in the finance domain as well as
// demonstrating encrypted machine learning. We will demonstrate how we can use
// FHE along with neural networks (NN) to carry out predictions for fraud
// detection while keeping the data, the NN model and the prediction results
// encrypted at all times. The neural network and dataset determine fraudulent
// activities based on anonymized transactions.

// This example showcases how you are able to utilize the processing power of an
// untrusted environment while preserving the privacy of your sensitive data.
// The demonstration is split into a privileged client that has access to
// unencrypted data and models, and an unprivileged server that only performs
// homomorphic computation in a completely encrypted fashion. The data and the
// NN model are encrypted in a trusted client environment and then are used to
// carry out predictions in an untrusted or public environment. Finally, the
// prediction results return encrypted and can only be decrypted by the data
// owner in the trusted environment. The concept of providing fully outsourced,
// but fully encrypted computation to a cloud provider is a major motivating
// factor in the field of FHE. This use case example shows the capability of the
// SDK to build such applications.

// **NOTE: while the client and server are not literally separated (nor
// demonstrating true remote cloud computation), the concepts generalize. One
// can imagine running the trusted code on local environment and the prediction
// code on a less trusted environment like the cloud. Additionally, we are
// working on FHE cloud that simplifies a lot of this.**

// This demo uses the Credit Card Fraud Detection dataset, originally taken
// from: https://www.kaggle.com/mlg-ulb/creditcardfraud.
// This dataset contains actual anonymized transactions made by credit card
// holders from September 2013 and is labeled for transactions being fraudulent
// or genuine. See references at the bottom of the page.

// -- Use case --

// Global credit card fraud is expected to reach $35B by 2025 (Nilson Report,
// 2020) and since the beginning of the COVID-19 pandemic, 40% of financial
// services firms saw an increase in fraudulent activity (LIMRA, 2020). As well
// as volume effects, COVID-19 has worsened the false positive issue for over
// two-thirds of institutions (69%). A key challenge for many institutions is
// that significant changes in consumer behavior have often resulted in existing
// fraud detection systems wrongly identifying legitimate behavior as suspected
// fraud (Omdia, 2021).

// With FHE, you are now able to unlock the value of regulated and sensitive PII
// data in the context of a less trusted cloud environment by performing AI,
// machine learning, and data analytic computations without ever having to
// decrypt. By training your AI models with additional sensitive data, you are
// able to achieve higher accuracy in fraud detection and reduce the false
// positive rate while also utilizing the many benefits of cloud computing.

// FHE can also help to support a zero trust strategy and can implement strong
// access control measures by keeping the data, the models that process the data
// and the results generated encrypted and confidential; only the data owner has
// access to the private key and has the privilege to decrypt the results.

int main()
{
  int availableMemory = MemoryUtils::getAvailableMemory();
  if (availableMemory == -1) {
    cerr << "WARNING: computing the amount of available memory failed. "
            "Assuming there is enough memory to run the demo ..."
         << endl;
  } else {
    // Make sure there is enough available memory to run this demo.
    // This demo requires about 4 GB of available memory.
    always_assert(MemoryUtils::getAvailableMemory() >= 4000);
  }

  // Step 1. Load the existing model and dataset into the trusted environment
  // In this step we are loading a pre-trained model and a dataset while
  // operating on a trusted client environment. The model and data used in this
  // demo correspond to a credit card fraud dataset.
  // For convenience, the model has been pre-trained and is available in
  // examples/python/notebooks/data_gen folder.

  string inputPath = getDataSetsDir() + "/net_fraud";
  int batchSize = 4096;
  DatasetPlain ds(batchSize);
  ds.loadFromH5(
      inputPath + "/x_test.h5", "x_test", inputPath + "/y_test.h5", "y_test");
  DoubleTensor plainSamples = ds.getSamples(0 /* batch */);
  DoubleTensor labels = ds.getLabels(0 /* batch */);
  cout << "loaded samples of shape: "
       << TensorUtils::shapeToString(plainSamples.getShape()) << endl;

  // Step 2. Encrypt the neural network in the trusted environment
  // The next set of steps include the following:

  // 2.1 Load NN architecture and weights using the FHE library
  string archFile = inputPath + "/model.json";
  string weightsFile = inputPath + "/model.h5";
  PlainModelHyperParams hyperParams;
  NeuralNetPlain neuralNetPlain;
  neuralNetPlain.initFromFiles(hyperParams, {archFile, weightsFile});
  cout << "neuralNetPlain created and initialized" << endl;

  // 2.2 Define He run requirements
  // These requirements specify how the HE encryption should be configured.
  HeRunRequirements heRunReq;
  // Encryption is as strong as a 192-bit encryption at least.
  heRunReq.setSecurityLevel(192);
  // Largest number we'll be able to safely process under encryption is
  // 2^30=1,073,741,824.
  heRunReq.setIntegerPartPrecision(30);
  // Our numbers are theoretically stored with precision of about 2^-30.
  heRunReq.setFractionalPartPrecision(30);
  // Batch size for NN. Large batch sizes should be used to optimize for
  // throughput while small batch sizes should be uesd to optimize for latency.
  heRunReq.optimizeForBatchSize(batchSize);
  // Use SEAL CKKS encryption library
  shared_ptr<SealCkksContext> sealHe = make_shared<SealCkksContext>();
  heRunReq.setHeContextOptions({sealHe});

  // 2.3  Compile the plain model and HE run requirements into HE profile
  // This HE profile holds encryption-specific parameters.
  optional<HeProfile> profile = HeModel::compile(neuralNetPlain, heRunReq);
  always_assert(profile.has_value());

  // 2.4 Encrypt the NN
  shared_ptr<HeContext> clientContext = HeModel::createContext(*profile);
  shared_ptr<HeModel> clientNn = neuralNetPlain.getEmptyHeModel(*clientContext);
  clientNn->encodeEncrypt(neuralNetPlain, *profile);
  cout << "clientNn initialized" << endl;

  // 2.5 Get a "ModelIoProcessor" from the HE model
  // The IoProcessor object will be used to encrypt and decrypt the input and
  // output of the prediction.
  shared_ptr<ModelIoProcessor> iop = clientNn->createIoProcessor();

  // 2.6 Encrypt the data in the trusted environment
  // Here we encrypt the samples that we'll later perform inference on. Note
  // that the encryption is done by the above created ModelIoProcessor object,
  // since some pre-processing of the data may be required to adjust it to this
  // praticular network.
  EncryptedData encryptedDataSamples(*clientContext);
  iop->encodeEncryptInputsForPredict(encryptedDataSamples,
                                     {make_shared<DoubleTensor>(plainSamples)});

  // 2.7 Save the NN and data so they can be loaded on the cloud/remote server
  FileUtils::createCleanDir(outDir);
  clientNn->saveToFile(nnFile);
  encryptedDataSamples.saveToFile(samplesFile);
  // Save the context. Note that this saves all the HE library information,
  // including the public key, allowing the server to perform HE computations.
  // The secret key is not saved here, so the server won't be able to decrypt.
  // The secret key is never stored unless explicitly requested by the user
  // using the designated method.
  clientContext->saveToFile(contextFile);

  // Step 3. Perform predictions in the untrusted server using encrypted data
  // and neural network

  // 3.1 Load the neural network, samples and context in the server
  // Now we're on the server side. We will load the encrypted NN, the encrypted
  // samples and the free-of-secret-key context into empty variables, using the
  // loadFromFile() method.
  shared_ptr<HeContext> serverContext = loadHeContextFromFile(contextFile);
  shared_ptr<HeModel> serverNn = loadHeModelFromFile(*serverContext, nnFile);
  shared_ptr<EncryptedData> serverSamples =
      loadEncryptedDataFromFile(*serverContext, samplesFile);

  // 3.2 Perform inference in cloud/server using encrypted data and encrypted NN
  // We can now run the inference of the encrypted data and encrypted NN to
  // obtain encrypted results. This computation does not use the secret key and
  // acts on completely encrypted values.
  // **NOTE: the data, the NN and the results always remain in encyrpted state,
  // even during computation.**
  EncryptedData serverPredictions(*serverContext);
  HELAYERS_TIMER_PUSH("predict");
  serverNn->predict(serverPredictions, *serverSamples);
  HELAYERS_TIMER_POP();

  // 3.3 Save the encrypted prediction so they can loaded and decrypted in the
  // trusted environment
  serverPredictions.saveToFile(predictionsFile);

  // Step 4. Decrypt the prediction results in the trusted environment

  // 4.1  Load the encrypted predictions:
  shared_ptr<EncryptedData> clientPredictions =
      loadEncryptedDataFromFile(*clientContext, predictionsFile);
  cout << "predictions loaded" << endl;

  // 4.2 Decrypt and decode the predictions
  // The client's side context also has the secret key, so we are able to
  // perform decryption. Here, the decrypt decode operation is done by the
  // ModelIoProcessor object, as some minor post-processing may be required
  // (e.g. transpose).
  DoubleTensorCPtr plainPredictions =
      iop->decryptDecodeOutput(*clientPredictions);

  // Step 5. Assess the results - precision, recall, F1 score

  // As this classification problem is a binary one, we will assess the results
  // by comparing the positive and negative classifications with the true
  // labels.
  assesResults(*plainPredictions, labels);
  HELAYERS_TIMER_PRINT_MEASURE_SUMMARY("predict");
  cout << "used RAM = " << MemoryUtils::getUsedRam() << " (MB)" << endl;

  FileUtils::removeDir(outDir);
}
