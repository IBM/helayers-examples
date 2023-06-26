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
  // build plain model.
  PlainModelHyperParams hp;
  hp.logisticRegressionActivation = activation;
  hp.numberOfFeatures = inputSize;

  shared_ptr<PlainModel> lrp = PlainModel::create(hp, {weightsH5File});

  // define HE LR variable. it represents a model that's encoded, and
  // possible encrypted, under FHE, and supports fit and/or predict.
  // this variable does not contin anything yet.
  shared_ptr<HeModel> lr;

  // define HE run requirements
  HeRunRequirements heRunReq;
  heRunReq.optimizeForBatchSize(batchSize);
  heRunReq.setHeContextOptions({make_shared<SealCkksContext>()});

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

  // encrypt inputs for predict
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
    iop->encodeEncryptInputsForPredict(*cinputs, {inputs});
  }

  // save HE model
  stringstream stream;
  lr->save(stream);
  cinputs->save(stream);

  // === SERVER SIDE ===
  // load HE model
  lr = loadHeModel(*he, stream);
  cinputs = loadEncryptedData(*he, stream);

  shared_ptr<EncryptedData> cresults = make_shared<EncryptedData>(*he);
  {
    HELAYERS_TIMER_SECTION("predict");
    lr->predict(*cresults, *cinputs);
  }

  // save predictions
  stringstream stream2;
  cresults->save(stream2);

  // === CLIENT SIDE ===
  // load predictions
  cresults = loadEncryptedData(*he, stream2);

  DoubleTensorCPtr res = iop->decryptDecodeOutput(*cresults);

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

  // Uncomment this to get lower level breakdown
  //  HELAYERS_TIMER_PRINT_MEASURES_SUMMARY();
}
