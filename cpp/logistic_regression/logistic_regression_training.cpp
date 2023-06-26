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
#include <fstream>
#include <chrono>
#include "helayers/hebase/hebase.h"
#include "helayers/hebase/HelayersTimer.h"
#include "helayers/hebase/utils/HelayersConfig.h"
#include "helayers/hebase/utils/TextIoUtils.h"
#include "helayers/ai/logistic_regression/LogisticRegressionPlain.h"
#include "helayers/ai/HeModel.h"
#include "helayers/ai/AiGlobals.h"
#include "helayers/math/MathGlobals.h"
#include "helayers/hebase/heaan/HeaanContext.h"

using namespace std;
using namespace std::chrono;
using namespace helayers;

// read training data from input file
static void readInput(DoubleTensor& input,
                      DoubleTensor& labels,
                      const string& inputFile,
                      int inputSize,
                      int batchSize)
{
  input = TextIoUtils::readMatrixFromCsvFile(inputFile);
  labels = input.getSlice(1, inputSize);
  input.removeSlice(1, inputSize);
  input = input.getSlice(0, 0, batchSize);
  labels = labels.getSlice(0, 0, batchSize);
}

int main()
{
  /*
  This example demonstrates how an encrypted logistic regression (LR) model can
  be trained with encrypted data. Predictions are also carried for validation of
  the trained model. Prediction results are encrypted and sent back to the data
  owner to be decrypted and assessed.

  The training is done over creditcardfraud dataset.

  Note that in this example, we use the high-level objects provided for LR
  (`LogisticRegressionPlain` and `HeModel`).
  - the number of epochs to be used in training is referred to by
  `numEpochs`.
  - the learning rate for training is `learnRate`.
  - the activation used for training is a `sigmoid degree 3 polynomial` and is
  referenced by index 0.
  */

  int numEpochs = 6;
  double learnRate = 0.1;
  DimInt inputSize = 30;
  DimInt batchSize = 984;
  LRActivationType activation = LR_ACTIVATION_SIGMOID_POLY_3_APPROXIMATION;
  string dataFile =
      getDataSetsDir() +
      "/logistic_regression/processed_creditcard_balanced_sample.csv";

  // Load and prepare the credit card fraud dataset for encryption in a trusted
  // client environment.
  cout << "Reading input . . ." << endl;
  DoubleTensor input({batchSize, inputSize, 1}),
      labels({batchSize, 1}); // input {984,30,1}, labels {984,1}
  readInput(input, labels, dataFile, inputSize, batchSize);
  cout << "Done reading input." << endl;

  // initialise a plain model object
  cout << "Initialising Logistic Regression Training . . ." << endl;
  PlainModelHyperParams hp;
  hp.numberOfFeatures = inputSize;
  hp.fitHyperParams.numberOfEpochs = numEpochs;
  hp.fitHyperParams.numberOfIterations = 1;
  hp.fitHyperParams.learningRate = learnRate;
  hp.logisticRegressionActivation = activation;
  hp.trainable = true;
  hp.verbose = false;

  shared_ptr<PlainModel> lrp = PlainModel::create(hp);

  // define HE LR variable. it represents a model that's encoded, and
  // possible encrypted, under FHE, and supports fit and/or predict.
  // this variable does not contain anything yet.
  shared_ptr<HeModel> lr;

  // define HE run requirements
  HeRunRequirements heRunReq;
  cout << "Initializing HEaaN . . ." << endl;
  heRunReq.setHeContextOptions({make_shared<HeaanContext>()});
  heRunReq.setMaxContextMemory(10L * 1024L * 1024L * 1024L); // 10 GB
  heRunReq.optimizeForBatchSize(batchSize);

  // compile the plain model and HE run requirements into HE profile
  optional<HeProfile> profile = HeModel::compile(*lrp, heRunReq);
  always_assert(profile.has_value());

  // init HE context given profile
  shared_ptr<HeContext> he = HeModel::createContext(*profile);

  // build HE model
  lr = lrp->getEmptyHeModel(*he);
  lr->encodeEncrypt(*lrp, *profile);

  // get IO processor from the HE model
  shared_ptr<ModelIoProcessor> iop = lr->createIoProcessor();
  cout << "Done initialising Logistic Regression Training." << endl;

  // The plaintext samples and labels are encrypted:
  cout << "Encrypting input and labels for Logistic Regression Training."
       << endl;
  EncryptedData encryptedInputs(*he);

  {
    HELAYERS_TIMER("Encrypt Input and Labels");
    vector<DoubleTensorCPtr> xy = {make_shared<DoubleTensor>(input),
                                   make_shared<DoubleTensor>(labels)};
    iop->encodeEncryptInputsForFit(encryptedInputs, xy);
  }
  cout << "Done encrypting input and labels for Logistic Regression Training."
       << endl;
  /*
  Perform the model training in the cloud/server using encrypted data and
  encrypted labels. We can now run the training of the encrypted data to obtain
  encrypted trained weights and bias. This computation does not use the secret
  key and acts on completely encrypted values. NOTE : the data the LR model and
  the results always remain in an encrypted state, even during computation.
  */

  {
    HELAYERS_TIMER("Training on Encrypted Data");
    lr->fit(encryptedInputs);
  }

  // decode the encrypted model for comparison to a plain model and use model to
  // predict on the input.
  shared_ptr<PlainModel> tpp = lr->decryptDecode();
  LogisticRegressionPlain& trainedPlain =
      dynamic_cast<LogisticRegressionPlain&>(*tpp);

  DoubleTensor actualWeightRes, expectedWeightRes, expectedBias, actualBiasRes;
  actualWeightRes = trainedPlain.getWeights();
  actualBiasRes = trainedPlain.getBias();
  cout << "===================================================================="
       << endl;

  // Train a plain model to compare with encrypted one
  DoubleTensor y = labels;
  {
    HELAYERS_TIMER("Training on Plain Data");
    lrp->fit({make_shared<DoubleTensor>(input), make_shared<DoubleTensor>(y)});
  }
  expectedWeightRes = dynamic_cast<LogisticRegressionPlain&>(*lrp).getWeights();
  expectedBias = dynamic_cast<LogisticRegressionPlain&>(*lrp).getBias();

  // cout << "Expected weights: " << expectedWeightRes << endl;
  // cout << "Actual weights: " << actualWeightRes << endl;
  // cout << "Expected bias: " << expectedBias << endl;
  // cout << "Actual bias: " << actualBiasRes << endl;

  actualWeightRes.testMse(expectedWeightRes, "weights", 1e-5);
  actualBiasRes.testMse(expectedBias, "bias", 1e-5);

  HELAYERS_TIMER_PRINT_MEASURE_SUMMARY("Initialise Context");
  HELAYERS_TIMER_PRINT_MEASURE_SUMMARY("Training on Encrypted Data");
}
