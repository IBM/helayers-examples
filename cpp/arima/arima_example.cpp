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

#include "helayers/hebase/heaan/HeaanContext.h"
#include "helayers/hebase/mockup/MockupContext.h"
#include "helayers/ai/HeModel.h"
#include "helayers/ai/AiGlobals.h"
#include "helayers/ai/arima/ArimaPlain.h"
#include "helayers/math/MathGlobals.h"
#include <iomanip>

using namespace std;
using namespace helayers;

void generateSeries(DoubleTensor& res,
                    DoubleTensor& errors,
                    double mu,
                    const vector<double>& phi,
                    double theta1,
                    double varw,
                    PlainModelHyperParams& hyperParams)
{
  int numSamples = hyperParams.numSamples;
  int p = hyperParams.p;
  res = DoubleTensor({numSamples, 1});
  errors = DoubleTensor({numSamples, 1});
  mt19937_64 rng;
  uint64_t timeSeed =
      chrono::high_resolution_clock::now().time_since_epoch().count();
  seed_seq ss{uint32_t(timeSeed & 0xffffffff), uint32_t(timeSeed >> 32)};

  rng.seed(ss);
  // sdw is the required standard deviation of the error
  double sdw = sqrt(varw);
  normal_distribution<double> dist(0, sdw);

  cout << "Generating random time series with " << numSamples << " elements"
       << endl;
  for (int i = 0; i < numSamples; i++) {
    errors.at(i) = dist(rng);
  }

  // mean = mu/(1-(sum(phi[i])))
  double sumPhi = 0.0;
  for (int i = 0; i < p; i++)
    sumPhi += phi[i];
  double mean = mu / (1.0 - sumPhi);
  for (int i = 0; i < p; i++)
    res.at(i) = mean;

  for (int i = p; i < numSamples; i++) {
    double val = mu;
    for (int j = 0; j < p; j++)
      val += phi[j] * res.at(i - 1 - j);
    val += theta1 * errors.at(i - 1) + errors.at(i);
    res.at(i) = val;
  }

  // The following commented-out code stores/loads the data
  // to enable re-running on the same data for debug purposes.

  // cout << "Storing series to file" << endl;
  // std::ofstream valsO("vals.txt");
  // for (int i = 0; i < numSamples; i++) {
  //   valsO << res.at(i) << endl;
  // }
  // valsO.close();

  // cout << "Loading series from file" << endl;
  // std::ifstream valsI("vals.txt");
  // for (int i = 0; i < numSamples; i++) {
  //   double val;
  //   valsI >> val;
  //   res.at(i) = val;
  // }
  // valsI.close();
}

int main()
{
  // define plain ARIMA variable. it represents a plain model, that is used as
  // an intermediate object that is loaded from external parameters and/or
  // weights, to be used to build an HE model. this variable does not contain
  // anything yet.
  shared_ptr<PlainModel> plainArima;

  // define HE ARIMA variable. it represents a model that's encoded, and
  // possible encrypted, under FHE, and supports fit and/or predict. this
  // variable does not contain anything yet.
  shared_ptr<HeModel> heArima;

  // === CLIENT SIDE ===

  // load plain model hyper-parameters from stream.
  ifstream ifs =
      FileUtils::openIfstream(getDataSetsDir() + "/arima/model.json");
  PlainModelHyperParams hyperParams;
  hyperParams.load(ifs);

  // build plain model.
  // The "create" method also supports providing a vector of input file names,
  // or a vector of input streams, as a second parameter.
  plainArima = PlainModel::create(hyperParams);

  // define HE run requirements
  HeRunRequirements heRunReq;
  heRunReq.setHeContextOptions({make_shared<HeaanContext>()});

  // compile the plain model and HE run requirements into HE profile
  optional<HeProfile> profile = HeModel::compile(*plainArima, heRunReq);
  always_assert(profile.has_value());

  // init HE context given profile
  shared_ptr<HeContext> he;
  bool useMockup = false;
  if (useMockup)
    profile->setNotSecureMockup();
  he = HeModel::createContext(*profile);

  getPrintOptions().precision = 8;

  // build HE model
  heArima = plainArima->getEmptyHeModel(*he);
  heArima->encodeEncrypt(*plainArima, *profile);

  // Generate a plain ARIMA(p,0,1) time series for training
  int p = hyperParams.p;
  int d = hyperParams.d;
  double expectedMu = 50;
  vector<double> expectedPhi;
  expectedPhi = {0.6};
  if (p >= 2)
    expectedPhi.push_back(0.3);
  if (p >= 3)
    expectedPhi.push_back(-0.2);
  if (p >= 4) {
    for (int i = 4; i <= p; i++)
      expectedPhi.push_back(0.1);
  }
  double expectedTheta1 = -0.4;
  double expectedVarw = 1.0;

  string ARIMA_name = "ARIMA(" + to_string(p) + "," + to_string(d) + ",1)";
  cout << "Fitting and training an " << ARIMA_name << " model" << endl;
  cout << "expected " << ARIMA_name << " coefficients:" << endl;
  cout << "mu     = " << expectedMu << endl;
  for (int i = 0; i < p; i++)
    cout << "phi" << (i + 1) << "   = " << expectedPhi[i] << endl;
  cout << "theta1 = " << expectedTheta1 << endl;
  cout << "varw   = " << expectedVarw << endl << endl;

  DoubleTensor seriesPlain, errors;
  generateSeries(seriesPlain,
                 errors,
                 expectedMu,
                 expectedPhi,
                 expectedTheta1,
                 expectedVarw,
                 hyperParams);

  DoubleTensorCPtr plainInputsForFit =
      make_shared<const DoubleTensor>(seriesPlain);

  DoubleTensorCPtr plainInputsForPredict =
      make_shared<const DoubleTensor>(seriesPlain.getSlice(
          0,
          seriesPlain.size() - hyperParams.numValuesUsedForPrediction,
          hyperParams.numValuesUsedForPrediction));

  // get IO processor from the HE model
  shared_ptr<ModelIoProcessor> iop = heArima->createIoProcessor();

  // encrypt inputs for fit
  shared_ptr<EncryptedData> encInputs = make_shared<EncryptedData>(*he);
  iop->encodeEncryptInputsForFit(*encInputs, {plainInputsForFit});

  // save HE model
  stringstream stream;
  heArima->save(stream);
  encInputs->save(stream);

  // === SERVER SIDE ===

  // load HE model
  heArima = loadHeModel(*he, stream);
  encInputs = loadEncryptedData(*he, stream);

  // fit HE model
  cout << "Homomorphically computing the ARIMA coefficients . . ." << endl;
  heArima->fit(*encInputs);

  // save trained HE model
  stringstream stream2;
  heArima->save(stream2);

  // === CLIENT SIDE ===

  heArima = loadHeModel(*he, stream2);

  // decrypt trained HE model into trained plain model
  plainArima = heArima->decryptDecode();
  plainArima->debugPrint("Model Trained Parameters");

  // build new HE model from the trained plain model, for inference under FHE.
  // can define new HE run requirements, obtain new profile and init new HE
  // context, tailored for inference.
  // this time, for inference under FHE, we choose to not encrypt the model
  // weights
  HeRunRequirements heRunReq2;
  heRunReq2.setHeContextOptions({make_shared<HeaanContext>()});
  heRunReq2.setModelEncrypted(false);

  profile = HeModel::compile(*plainArima, heRunReq2);
  always_assert(profile.has_value());

  if (useMockup)
    profile->setNotSecureMockup();
  he = HeModel::createContext(*profile);

  heArima = plainArima->getEmptyHeModel(*he);
  heArima->encode(*plainArima, *profile);

  // get new IO processor from the HE model
  iop = heArima->createIoProcessor();

  // save IO processor to be used to encrypt inputs by some other entity that
  // has data to predict over
  stringstream stream3;
  iop->save(stream3);

  // load IO processor by the other entity else, encrypt inputs for predict
  iop = loadIoProcessor(*he, stream3);

  encInputs = make_shared<EncryptedData>(*he);
  iop->encodeEncryptInputsForPredict(*encInputs, {plainInputsForPredict});

  // save HE model
  stringstream stream4;
  heArima->save(stream4);
  encInputs->save(stream4);

  // === SERVER SIDE ===

  // load HE model
  heArima = loadHeModel(*he, stream4);
  encInputs = loadEncryptedData(*he, stream4);

  // predict over HE model
  cout << endl
       << "Homomorphically predicting the next value in the time series"
       << endl;
  EncryptedData encResults(*he);
  heArima->predict(encResults, *encInputs);

  // === CLIENT SIDE ===

  // decrypt prediction
  cout << endl << "Decrypting the prediction result" << endl << endl;
  DoubleTensorCPtr result = iop->decryptDecodeOutput(encResults);
  double fhePrediction = result->at(0);
  std::shared_ptr<ArimaPlain> plainArimaDerived =
      std::dynamic_pointer_cast<ArimaPlain>(plainArima);
  double varw = plainArimaDerived->getErrorVariance();

  // check prediction
  DoubleTensorCPtr plainResults =
      plainArima->predict({plainInputsForPredict}).at(0);
  double plainPrediction = plainResults->at(0);
  double plainVarw = plainArimaDerived->getErrorVariance();

  cout << setprecision(8);
  cout << "FHE   " << ARIMA_name << " prediction = " << fhePrediction;
  if (!isnan(varw) && (varw > 0)) {
    double sdw = sqrt(varw);
    cout << ", error standard-deviation = +-" << sdw;
  }
  cout << endl;

  cout << "plain " << ARIMA_name << " prediction = " << plainPrediction;
  if (!isnan(plainVarw) && (plainVarw > 0)) {
    double sdw = sqrt(plainVarw);
    cout << ", error standard-deviation = +-" << sdw;
  }
  cout << endl;
  double absoluteDiff = fabs(plainPrediction - fhePrediction);
  double relativeDiff = absoluteDiff / fabs(plainPrediction);
  cout << "absolute diff = " << absoluteDiff << endl;
  cout << "relative diff = " << relativeDiff << endl;
  always_assert(relativeDiff < 1e-2);
}