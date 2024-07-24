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
  // and encrypt them. In this step we are loading a pre-trained model and a
  // dataset while operating on a trusted client environment. The model and data
  // used in this demo correspond to a credit card fraud dataset. For
  // convenience, the model has been pre-trained and is available in
  // examples/python/notebooks/data_gen folder.

  // 1.1 load the model and data.
  string inputPath = getDataSetsDir() + "/net_fraud";
  string archFile = inputPath + "/model.json";
  string weightsFile = inputPath + "/model.h5";
  int batchSize = 4096;
  DatasetPlain ds(batchSize);
  ds.loadFromH5(
      inputPath + "/x_test.h5", "x_test", inputPath + "/y_test.h5", "y_test");
  DoubleTensor plainSamples = ds.getSamples(0 /* batch */);
  DoubleTensor labels = ds.getLabels(0 /* batch */);
  cout << "loaded samples of shape: "
       << TensorUtils::shapeToString(plainSamples.getShape()) << endl;

  // 1.2 Encrypt the neural network in the trusted environment
  // The next step loads a model that was pre-trained in the clear.
  // The initialization processes involves internally an optimization step.
  // This step finds the best parameters for this model, and also gives us
  // estimations on the time it would take to predict using a single core, the
  // precision, the memory, the time it would take to encrypt/decrypt, etc.

  // The input to the compilation process are some preferences that we have,
  // specified in the `HeRunRequirements` object. In this demo we just specify
  // that we use SEAL as the underlying backend. We also specify the batch size,
  // how many samples we plan to provide when we perform inference.

  // There are many more parameters that can be specified to the optimizer.

  // These requirements specify how the HE encryption should be configured.
  HeRunRequirements heRunReq;
  // Use SEAL CKKS encryption library
  heRunReq.setHeContextOptions({make_shared<SealCkksContext>()});
  // Batch size for NN. Large batch sizes should be used to optimize for
  // throughput while small batch sizes should be used to optimize for latency.
  heRunReq.optimizeForBatchSize(batchSize);

  shared_ptr<HeModel> nn = make_shared<NeuralNet>();
  nn->encodeEncrypt({archFile, weightsFile}, heRunReq);

  // The above initialization processes also configured the HE encryption
  // scheme, and generated the keys. These can be accessed via the `he_context`
  // object.
  shared_ptr<HeContext> heContext = nn->getCreatedHeContext();

  // 1.3 Encrypt the data.
  // Create a "ModelIoEncoder" for the HE model. This object will be
  // used to encrypt and decrypt the input and output of the prediction.
  ModelIoEncoder modelIoEncoder(*nn);

  // Here we encrypt the samples that we'll later perform inference on. Note
  // that the encryption is done by the above created ModelIoEncoder object,
  // since some pre-processing of the data may be required to adjust it to this
  // particular network.
  EncryptedData encryptedDataSamples(*heContext);
  modelIoEncoder.encodeEncrypt(encryptedDataSamples,
                               {make_shared<DoubleTensor>(plainSamples)});

  // Step 2. Perform predictions in the untrusted server using encrypted data
  // and neural network

  // We assume the encrypted model and data were sent over to an untrusted
  // server (see next demos for examples how to do that).

  // 2.1 Perform inference in cloud/server using encrypted data and encrypted NN
  // We can now run the inference of the encrypted data and encrypted NN to
  // obtain encrypted results. This computation does not use the secret key and
  // acts on completely encrypted values.
  // **NOTE: the data, the NN and the results always remain in encrypted state,
  // even during computation.**
  EncryptedData predictions(*heContext);
  HELAYERS_TIMER_PUSH("predict");
  nn->predict(predictions, encryptedDataSamples);
  HELAYERS_TIMER_POP();

  // Step 3. Decrypt the prediction results in the trusted environment

  // The client's side context also has the secret key, so we are able to
  // perform decryption. Here, the decrypt decode operation is done by the
  // ModelIoEncoder object, as some minor post-processing may be required
  // (e.g. transpose).
  DoubleTensorCPtr plainPredictions =
      modelIoEncoder.decryptDecodeOutput(predictions);

  // Step 4. Assess the results - precision, recall, F1 score

  // As this classification problem is a binary one, we will assess the results
  // by comparing the positive and negative classifications with the true
  // labels.
  assesResults(*plainPredictions, labels);
  HELAYERS_TIMER_PRINT_MEASURE_SUMMARY("predict");
  cout << "used RAM = " << MemoryUtils::getUsedRam() << " (MB)" << endl;
}
