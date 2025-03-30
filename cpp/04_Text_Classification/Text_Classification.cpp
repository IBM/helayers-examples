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

#include "helayers/ai/DatasetPlain.h"
#include "helayers/ai/nn/NeuralNet.h"
#include "helayers/hebase/hebase.h"
#include "helayers/hebase/seal/SealCkksContext.h"
#include "helayers/hebase/utils/MemoryUtils.h"
#include "helayers/math/DoubleTensor.h"
#include "helayers/math/TensorUtils.h"
#include <iostream>
#include <string>
#include <vector>

using namespace std;
using namespace helayers;

void assessResults(const DoubleTensor& predictedLabels,
                   const DoubleTensor& origLabels)
{
  int numSamples = predictedLabels.getDimSize(0);
  int numClasses = predictedLabels.getDimSize(1);

  // Initialize the confusion matrix with zeros
  std::vector<std::vector<int>> confusionMatrix(
      numClasses, std::vector<int>(numClasses, 0));

  // Iterate through each sample to fill the confusion matrix
  for (int sample = 0; sample < numSamples; ++sample) {
    int predictedClass = -1, origClass = -1;
    double maxPred = -1.0, maxOrig = -1.0;

    // Find the index of the maximum value in the predicted and original label
    // row
    for (int cls = 0; cls < numClasses; ++cls) {
      double predVal = predictedLabels.at(sample, cls);
      double origVal = origLabels.at(sample, cls);

      if (predVal > maxPred) {
        maxPred = predVal;
        predictedClass = cls;
      }
      if (origVal > maxOrig) {
        maxOrig = origVal;
        origClass = cls;
      }
    }

    // Increment the corresponding cell in the confusion matrix
    if (origClass != -1 && predictedClass != -1) {
      confusionMatrix[origClass][predictedClass]++;
    }
  }

  // Print the confusion matrix
  std::cout << "Confusion Matrix:" << std::endl;
  for (const auto& row : confusionMatrix) {
    for (int count : row) {
      std::cout << count << " ";
    }
    std::cout << std::endl;
  }
}

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
  // used in this demo correspond to the 20-newsgroups. For
  // convenience, the model has been pre-trained and is available in
  // examples/python/notebooks/data_gen folder.

  // 1.1 load the model and data.
  string inputPath = getDataSetsDir() + "/text_classification";
  string archFile = inputPath + "/model.json";
  string weightsFile = inputPath + "/model.h5";
  int batchSize = 8;
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
  assessResults(*plainPredictions, labels);
  HELAYERS_TIMER_PRINT_MEASURE_SUMMARY("predict");
  cout << "used RAM = " << MemoryUtils::getUsedRam() << " (MB)" << endl;
}
