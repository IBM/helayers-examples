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

#include "helayers/ai/DatasetPlain.h"
#include "helayers/ai/decision_tree/DecisionTree.h"
#include "helayers/ai/HeModel.h"
#include "helayers/hebase/hebase.h"
#include "helayers/hebase/heaan/HeaanContext.h"
#include "helayers/hebase/mockup/MockupContext.h"

#include <iomanip>
#include <iostream>

using namespace std;
using namespace helayers;

static void assessResults(const DoubleTensor& predictedLabels,
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

  always_assert(f1Score > 0.7);
}

int main(int argc, char** argv)
{
  int maxBatches = 3;
  bool mockup = false;

  for (int i = 0; i < argc; ++i) {
    // The maximum number of batches to predict over. Set to -1 to predict over
    // all batches.
    if (std::string(argv[i]) == "--max_batches") {
      maxBatches = atoi(argv[i + 1]);
    }

    // If this flag is set, a mockup context will be used
    if (std::string(argv[i]) == "--mockup") {
      mockup = true;
    }
  }

  string dtFilePath = getDataSetsDir() + "/decision_tree/creditfraud_dt.json";
  string hyperParamsFile =
      getDataSetsDir() + "/decision_tree/dtree_hyper_params.json";

  PlainModelHyperParams hyperParams;
  hyperParams.load(hyperParamsFile);

  shared_ptr<PlainModel> dtreePlain =
      PlainModel::create(hyperParams, {dtFilePath});

  HeRunRequirements heRunReq;
  shared_ptr<HeaanContext> heaanHe = make_shared<HeaanContext>();
  heRunReq.setHeContextOptions({heaanHe});

  optional<HeProfile> profile = HeModel::compile(*dtreePlain, heRunReq);
  always_assert(profile.has_value());

  shared_ptr<HeContext> he;
  if (mockup)
    profile->setNotSecureMockup();
  he = HeModel::createContext(*profile);

  DatasetPlain datasetPlain(he->slotCount());
  string inputFile = getDataSetsDir() + "/net_fraud/creditcard.csv";
  datasetPlain.loadFromCsv(
      inputFile, true /* ignoreFirstRow */, ',' /* delimeter */, maxBatches);

  DoubleTensor plainSamples = datasetPlain.getAllSamples();
  DoubleTensor plainLabels = datasetPlain.getAllLabels();

  shared_ptr<HeModel> dtree = dtreePlain->getEmptyHeModel(*he);
  dtree->encodeEncrypt(*dtreePlain, *profile);

  shared_ptr<ModelIoProcessor> iop = dtree->createIoProcessor();
  EncryptedData encryptedDataSamples(*he);
  iop->encodeEncryptInputsForPredict(encryptedDataSamples,
                                     {make_shared<DoubleTensor>(plainSamples)});

  EncryptedData encryptedPredictions(*he);
  {
    HELAYERS_TIMER_SECTION("predict");
    dtree->predict(encryptedPredictions, encryptedDataSamples);
  }

  DoubleTensorCPtr predictions = iop->decryptDecodeOutput(encryptedPredictions);

  HELAYERS_TIMER_PRINT_MEASURE_SUMMARY("predict");

  assessResults(*predictions, plainLabels);
}
