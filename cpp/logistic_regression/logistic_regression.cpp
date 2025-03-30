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
#include "helayers/hebase/seal/SealCkksContext.h"
#include "helayers/hebase/utils/TextIoUtils.h"
#include "helayers/ai/HeModel.h"
#include "helayers/ai/logistic_regression/LogisticRegression.h"
#include "helayers/ai/AiGlobals.h"
#include "helayers/math/MathGlobals.h"

using namespace std;
using namespace std::chrono;
using namespace helayers;

int main()
{
  DimInt inputSize = 30;
  DimInt batchSize = 984;
  LRActivationType activation = LR_ACTIVATION_SIGMOID_POLY_3_APPROXIMATION;
  string weightsH5File =
      getDataSetsDir() + "/logistic_regression/creditcard_weights_bias.h5";
  string dataFile = getDataSetsDir() +
                    "/logistic_regression/"
                    "processed_creditcard_balanced_sample.csv";

  cout << "*** Starting LR demo ***" << endl;

  // === CLIENT SIDE ===
  // Define hyper parameters.
  PlainModelHyperParams hp;
  hp.logisticRegressionActivation(activation);
  hp.numberOfFeatures(inputSize);

  // Define HE run requirements.
  HeRunRequirements heRunReq;
  heRunReq.optimizeForBatchSize(batchSize);
  heRunReq.setHeContextOptions({make_shared<SealCkksContext>()});

  // Build HE model.
  shared_ptr<HeModel> lr = make_shared<LogisticRegression>();
  lr->encodeEncrypt({weightsH5File}, heRunReq, hp);

  // Get the HE context.
  shared_ptr<HeContext> he = lr->getCreatedHeContext();

  // Get Model IO encoder for the HE model.
  ModelIoEncoder modelIoEncoder(*lr);

  // Encrypt inputs for predict.
  shared_ptr<DoubleTensor> inputs = make_shared<DoubleTensor>();
  *inputs = TextIoUtils::readMatrixFromCsvFile(dataFile);
  shared_ptr<DoubleTensor> labels =
      make_shared<DoubleTensor>(inputs->getSlice(1, inputSize));
  inputs->removeSlice(1, inputSize);
  for (size_t i = 0; i < labels->size(); i++) {
    labels->at(i) = 1 - labels->at(i);
  }

  shared_ptr<EncryptedData> cinputs = make_shared<EncryptedData>(*he);
  {
    HELAYERS_TIMER_SECTION("encrypt");
    modelIoEncoder.encodeEncrypt(*cinputs, {inputs});
  }

  // Save HE model.
  stringstream stream;
  lr->save(stream);
  cinputs->save(stream);

  // === SERVER SIDE ===
  // Load HE model.
  lr = loadHeModel(*he, stream);
  cinputs = loadEncryptedData(*he, stream);

  shared_ptr<EncryptedData> cresults = make_shared<EncryptedData>(*he);
  {
    HELAYERS_TIMER_SECTION("predict");
    lr->predict(*cresults, *cinputs);
  }

  // Save predictions.
  stringstream stream2;
  cresults->save(stream2);

  // === CLIENT SIDE ===
  // Load predictions.
  cresults = loadEncryptedData(*he, stream2);

  DoubleTensorCPtr res = modelIoEncoder.decryptDecodeOutput(*cresults);

  res->assertSameShape(*labels);
  int correct = 0;
  for (int i = 0; i < batchSize; i++) {
    if (fabs(res->at(i) - labels->at(i)) < 0.5) {
      correct++;
    }
  }

  double accuracy = (double)correct / batchSize;
  cout << "Accuracy = " << accuracy << endl;
  always_assert(accuracy > 0.75);

  HELAYERS_TIMER_PRINT_MEASURE_SUMMARY("encrypt");
  HELAYERS_TIMER_PRINT_MEASURE_SUMMARY("predict");

  // Uncomment this to get lower level breakdown:
  //  HELAYERS_TIMER_PRINT_MEASURES_SUMMARY();
}
