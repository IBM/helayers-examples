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
#include <fstream>
#include "kmeans_demo.h"
#include "helayers/ai/kmeans/KMeans.h"
#include "helayers/ai/kmeans/KMeansPlain.h"
#include "helayers/math/CTileTensor.h"
#include "helayers/math/TTEncoder.h"
#include "helayers/math/TensorUtils.h"
#include "helayers/math/MathGlobals.h"
#include "helayers/hebase/MemoryStorage.h"
#include "helayers/hebase/utils/TextIoUtils.h"

using namespace std;
using namespace helayers;

void run(HeContext& emptyHe)
{
  string dataDir = getDataSetsDir() + "/kmeans/";

  shared_ptr<Storage> storage1 = make_shared<MemoryStorage>();
  shared_ptr<Storage> storage2 = make_shared<MemoryStorage>();

  cout << "Loading plain model . . . " << endl;

  shared_ptr<KMeansPlain> kp = make_shared<KMeansPlain>();
  kp->initFromFiles(PlainModelHyperParams(), {dataDir + "kmeansCenters.csv"});

  cout << "Testing it with sample data . . ." << endl;
  shared_ptr<DoubleTensor> input = make_shared<DoubleTensor>();
  *input = TextIoUtils::readMatrixFromCsvFile(dataDir + "testData.csv");
  DoubleTensorCPtr plainRes = kp->predict({input}).at(0);

  cout << "Creating encrypted KMeans instance" << endl;
  HeRunRequirements heRunReq;
  heRunReq.setHeContextOptions({emptyHe.clone()});
  optional<HeProfile> profile = HeModel::compile(*kp, heRunReq);
  always_assert(profile.has_value());

  shared_ptr<HeContext> he;
  {
    HELAYERS_TIMER_SECTION("init");
    he = HeModel::createContext(*profile);
  }

  KMeans kmeans(*he);
  kmeans.encodeEncrypt(*kp, *profile);
  shared_ptr<ModelIoProcessor> iop = kmeans.createIoProcessor();

  EncryptedData cdata(*he);
  cdata.attachOutputStorage(storage1);
  {
    HELAYERS_TIMER_SECTION("encrypt");
    iop->encodeEncryptInputsForPredict(cdata, {input});
  }
  cdata.flushToStorage();

  // Now entering server side.
  // Note that we use here the same context.
  // In real life (and some other demos) we pass the context
  // to the server as well, without the secret key.
  {
    cout << "predicting" << endl;
    shared_ptr<EncryptedData> input = loadEncryptedData(*he, storage1);

    EncryptedData serverResults(*he);
    serverResults.attachOutputStorage(storage2);
    {
      HELAYERS_TIMER_SECTION("predict");
      kmeans.predict(serverResults, *input);
    }
    serverResults.flushToStorage();
  }

  shared_ptr<EncryptedData> cresults = loadEncryptedData(*he, storage2);

  cout << "decrypting results" << endl;
  DoubleTensorCPtr heRes;
  {
    HELAYERS_TIMER_SECTION("decrypt");
    heRes = iop->decryptDecodeOutput(*cresults);
  }

  cout << "Comparing with plain . . ." << endl;
  // note: no HE noise is expected, since argmin is computed in the plain after
  // decryption
  heRes->assertEquals(*plainRes, "he vs plain kmeans results", 1e-10);

  // server context will be mostly the same
  cout << "Encrypted input size (bytes): "
       << dynamic_cast<const MemoryStorage&>(*storage1).getSize() << endl;
  cout << "Encrypted output size (bytes): "
       << dynamic_cast<const MemoryStorage&>(*storage2).getSize() << endl;
  // model file size should be also reported

  HELAYERS_TIMER_PRINT_MEASURE_SUMMARY("init");
  HELAYERS_TIMER_PRINT_MEASURE_SUMMARY("encrypt");
  HELAYERS_TIMER_PRINT_MEASURE_SUMMARY("predict");
  HELAYERS_TIMER_PRINT_MEASURE_SUMMARY("decrypt");

  // Uncomment this to get lower level breakdown
  //  HELAYERS_TIMER_PRINT_MEASURES_SUMMARY();*/
}
