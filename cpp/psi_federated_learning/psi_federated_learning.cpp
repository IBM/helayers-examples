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

#include <string>
#include <random>
#include <vector>
#include <algorithm>
#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include "helayers/hebase/OmpWrapper.h"

#include "helayers/hebase/hebase.h"
#include "helayers/hebase/openfhe/OpenFheCkksContext.h"
#include "helayers/hebase/HelayersTimer.h"
#include "helayers/ai/psi_federated_learning/RtsPsiManager.h"
#include "helayers/ai/psi_federated_learning/AggregatorPsiManager.h"

using namespace std;
using namespace helayers;

/*
 This example demonstrates a Private Set Intersection (PSI) protocol between two
 parties, called Alice and Bob, using a third party, called the Aggregator, to
 be used for Federated Learning (FL). Alice and Bob both have their own DB with
 private samples. Each sample contains a unique identifier (UID) and number of
 features, each is a real number in range [-1,1]. The objective is that Alice
 will get by the end of the protocol a CTileTensor encrypted under the
 aggregator's key, contains only the samples which are in the intersection, in a
 way such that no party learns anything about the other party's samples or about
 the intersection. For example, Alice shouldn't learn whether a specific sample
 is in the intersection or not, and the aggregator shouldn't learn anything
 about Alice's and Bob's samples. The CTileTensor then can be further used by
 Alice in some learning algorithm. The result then can be sent to the aggregator
 who can decrypt it and send it to Alice and Bob.
*/

// A secret that is shared between all the RTSs, and unknown by the aggregator.
// The means by which this secret is shared is out of scope of this protocol.

const vector<uint64_t> SHARED_SECRET = {1234, 5678};

const int DEFAULT_NUM_SAMPLES = 3;

void help()
{
  cout << "--num_samples is an optional integer flag that sets the number of "
          "samples to read from Alice and Bob's tables (by default 10 samples, "
          "and must be at least 2 and at most 351).\n"
          "For example, to run the protocol on just 100 records, run"
          "./psi_federated_learning --num_samples 100"
       << endl;
  cout << "--verbose\tAn optional flag sets the verbosity level to high "
          "verbosity."
       << endl;
  exit(1);
}

void printHeader(int numSamples)
{
  cout << std::string(70, '=') << endl;
  cout << "Comparing " << numSamples << " samples from each side"
       << (numSamples == DEFAULT_NUM_SAMPLES
               ? ""
               : " (run with --num_samples to configure number "
                 "of samples to compare)")
       << "." << endl;
}

void cleanData(DoubleTensor& data)
{
  for (size_t i = 0; i < data.size(); i++) {
    if (abs(data.at(i)) < 1e-4) {
      data.at(i) = 0;
    }
  }
}

int main(int argc, char* argv[])
{
  int numSamples = DEFAULT_NUM_SAMPLES;

  Verbosity verbosity = VERBOSITY_LOW;
  int i = 1;
  while (i < argc) {
    string arg = argv[i++];
    if (arg == "--num_samples")
      numSamples = stoi(argv[i++]);
    else if (arg == "--verbose")
      verbosity = VERBOSITY_REGULAR;
    else
      help();
  }

  printHeader(numSamples);

  // The aggregator creates Helayers context to be used in the Private Set
  // Intersection protocol. Then, it communicates with the clients Alice and Bob
  // (the means by which it does it are out of scope of this example). Finally,
  // it sends them the context object (in this example, we will just use the
  // same context object in Alice's and Bob's code).

  cout << "Run Aggregator's side..." << endl;

  OpenFheCkksContext he;
  HeConfigRequirement req(pow(2, 15), 20, 48, 7);
  req.bootstrappable = true;
  req.automaticBootstrapping = true;
  he.init(req);

  AggregatorPsiManager aggregator(he, he);

  // Alice initializes a RtsPsiManager object with the Helayers context
  // the aggregator sent, with her UIDs and data and with the shared
  // secret she agreed about with Bob. Then, she run the first step of
  // the PSI which is inserting her UIDs to an encrypted hash table.
  // Finally, she sends the result along with the mapping of the uids in the
  // hash table to the aggregator.

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distribution(1, 2147483647);

  vector<uint64_t> aliceUids(numSamples);
  for (size_t i = 0; i < aliceUids.size(); i++) {
    aliceUids[i] = distribution(gen);
  }

  // Here we initialize Alice's data with numSamples samples and 17 features.
  DoubleTensor aliceData({numSamples, 1});

  RtsPsiManager alicePsiManager(he, he, 1, aliceUids, aliceData, SHARED_SECRET);
  alicePsiManager.setVerbosity(verbosity);

  vector<uint64_t> bobUids(numSamples);
  for (size_t i = 0; i < bobUids.size(); i++) {
    bobUids[i] = i + 1;
  }

  // Here we initialize Bob's data with numSamples samples and 18 features.
  DoubleTensor bobData({numSamples, 1});

  RtsPsiManager bobPsiManager(he, he, 2, bobUids, bobData, SHARED_SECRET);
  bobPsiManager.setVerbosity(verbosity);

  HELAYERS_TIMER_PUSH("PSI for FL: total");

  cout << "Run Alice's side..." << endl;
  CTileTensor ctt1(he);
  alicePsiManager.insertToHash(ctt1);
  const vector<size_t>& mapping = alicePsiManager.getUidsMapping();

  // The aggregator receives the encrypted hash table from alice and sends it to
  // Bob. Also, it receives the mapping to be used later.

  cout << "Run Aggregator's side..." << endl;

  // Bob initializes a RtsPsiManager as well.
  // He receives the data from the aggregator, processes it, meaning generate an
  // indicators vector from it that indicates which samples are in the
  // intersection, and sends the result back to the aggregator.

  cout << "Run bob's side..." << endl;

  CTileTensor ctt2(he);
  bobPsiManager.generateIndicatorVector(1, ctt2, ctt1);

  // The aggregator rearranges the indicators vector using the mapping sent by
  // Alice, so that the order of the indicators will be the same as the original
  // order of the samples

  cout << "Run Aggregator's side..." << endl;
  CTileTensor ctt3(he);
  aggregator.rearrangeIndicatorVector(ctt3, ctt2, mapping);

  // Finally, Alice receives the encrypted indicators vector from the
  // aggregator. She uses it to privately sort her data, such that the
  // first rows are records that are in the intersection, and the rest of the
  // rows are encryptions of 0s (note that the relative order of the samples
  // that are in the intersection is the same as the relative order of them in
  // the DoubleTensor given to the c'tor). The resulted CTileTensor will be then
  // used in the learning algorithm.

  cout << "Run Alice's side..." << endl;
  CTileTensor ctt4(he);
  alicePsiManager.multiplyIndicatorVectors(ctt4, {2}, {ctt3});

  CTileTensor ctt5(he);
  alicePsiManager.compaction(ctt5, ctt4);

  HELAYERS_TIMER_POP();

  cout << std::string(70, '=') << endl << endl;
  cout << "Final Results" << endl;
  cout << std::string(70, '=') << endl << endl;
  cout << "Alice's UIDS:" << endl;
  cout << boost::join(aliceUids |
                          boost::adaptors::transformed(
                              [](uint64_t uid) { return to_string(uid); }),
                      ", ")
       << endl
       << endl;
  cout << std::string(70, '=') << endl;
  cout << "Bob's UIDS:" << endl;
  cout << boost::join(bobUids | boost::adaptors::transformed([](uint64_t uid) {
                        return to_string(uid);
                      }),
                      ", ")
       << endl
       << endl;
  cout << std::string(70, '=') << endl;
  aliceData.debugPrint("Alice's Data");
  cout << endl;
  cout << std::string(70, '=') << endl;
  TTEncoder enc(he);
  DoubleTensor res = enc.decryptDecodeDouble(ctt5);
  cleanData(res);
  res.debugPrint(
      "Protocol Output (In a real use of the protocol, Alice will not "
      "be able to see this content, and so will not learn "
      "anything about which samples are in the intersection)");
  cout << endl;

  return 0;
}