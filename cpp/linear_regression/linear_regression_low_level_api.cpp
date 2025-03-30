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

#include <iostream>
#include "helayers/hebase/hebase.h"
#include "helayers/hebase/seal/SealCkksContext.h"
#include "helayers/hebase/utils/TextIoUtils.h"
#include "helayers/math/MathUtils.h"
#include "helayers/ai/logistic_regression/LogisticRegressionPlain.h"
#include "helayers/ai/HeModel.h"
#include "helayers/ai/AiGlobals.h"
#include "helayers/math/MathGlobals.h"

using namespace std;
using namespace helayers;

/*
This demo uses the low level HElayers HeModel API that allows finer control than
the regular API demonstrated in other demos.
*/

int main()
{
  cout << "*** Starting Linear Regression demo ***" << endl;

  string dataDir = getDataSetsDir() + "/linear_regression/";

  // === CLIENT SIDE ===
  // build plain model.
  DoubleTensor weights =
      TextIoUtils::readMatrixFromCsvFile(dataDir + "logModelWeights.csv");
  weights.transpose();
  weights.reshape({(int)weights.size(), 1, 1});

  DoubleTensor biases =
      TextIoUtils::readMatrixFromCsvFile(dataDir + "bias.csv");

  PlainModelHyperParams hp;
  hp.logisticRegressionActivation(LR_ACTIVATION_NONE);
  LogisticRegressionPlain lrPlain;
  lrPlain.initFromTensor(hp, weights, biases.at(0));

  // define HE LR variable. it represents a model that's encoded, and
  // possible encrypted, under FHE, and supports fit and/or predict.
  // this variable does not contin anything yet.
  shared_ptr<HeModel> lr;

  // define HE run requirements
  HeRunRequirements heRunReq;
  heRunReq.setHeContextOptions({make_shared<SealCkksContext>()});

  // compile the plain model and HE run requirements into HE profile
  optional<HeProfile> profile = HeModel::compile(lrPlain, heRunReq);
  always_assert(profile.has_value());

  // init HE context given profile
  shared_ptr<HeContext> he = HeModel::createContext(*profile);

  // build HE model
  lr = lrPlain.getEmptyHeModel(*he);
  lr->encodeEncrypt(lrPlain, *profile);

  // get IO encoder from the HE model
  ModelIoEncoder modelIoEncoder(*lr);

  // encrypt inputs for predict
  shared_ptr<DoubleTensor> inputs = make_shared<DoubleTensor>();
  *inputs = TextIoUtils::readMatrixFromCsvFile(dataDir + "testData.csv");

  shared_ptr<EncryptedData> cinputs = make_shared<EncryptedData>(*he);
  {
    HELAYERS_TIMER_SECTION("encrypt");
    modelIoEncoder.encodeEncrypt(*cinputs, {inputs});
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

  // decrypt prediction
  {
    DoubleTensorCPtr results = modelIoEncoder.decryptDecodeOutput(*cresults);
    cout << results->getShapeAsString() << endl;

    for (int p = 0; p < results->size(); p++) {
      double sum = 0.0;
      for (int w = 0; w < inputs->getDimSize(1); w++) {
        sum += inputs->at(p, w) * weights.at(w);
      }

      sum += biases.at(0, 0);

      cout << "[" << p << "] ground truth = " << sum
           << ", HE result = " << results->at(p) << endl;

      if (fabs(results->at(p) - sum) > 1e-4) {
        cout << "ERROR: results " << results->at(p) << " should be " << sum
             << endl;
      }
    }
  }

  HELAYERS_TIMER_PRINT_MEASURE_SUMMARY("encrypt");
  HELAYERS_TIMER_PRINT_MEASURE_SUMMARY("predict");

  // Uncomment this to get lower level breakdown
  //  HELAYERS_TIMER_PRINT_MEASURES_SUMMARY();
}
