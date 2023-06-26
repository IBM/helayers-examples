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
#include <omp.h>

#include "helayers/hebase/hebase.h"
#include "helayers/hebase/heaan/HeaanContext.h"
#include "helayers/hebase/HelayersTimer.h"
#include "helayers/ai/psi_federated_learning/RtsPsiManager.h"
#include "helayers/ai/psi_federated_learning/AggregatorPsiManager.h"

using namespace std;
using namespace helayers;

// See more information about this demo in the readme file.

const vector<uint64_t> SHARED_SECRET = {1234, 5678};

const int ALICE_RTS_ID = 1;
const int BOB_RTS_ID = 2;
const int CHARLIE_RTS_ID = 3;
const int DOUG_RTS_ID = 4;

const int DEFAULT_NUM_SAMPLES = 3;

void help()
{
  cout << "--num_samples is an optional integer flag that sets the number of "
          "samples to read from each participant (by default 4 samples, "
          "and must be at least 4).\n"
          "For example, to run the protocol on just 1000 records, run"
          "./psi_multiple_parties --num_samples 1000\n"
          "When running with less than 5 samples, the result will be printed "
          "in the end."
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

void initRandomUids(vector<u_int64_t>& uids)
{
  random_device dev;
  mt19937 rng(dev());
  uniform_int_distribution<mt19937::result_type> dist(1, 100);

  uids[0] = dist(rng);
  for (size_t i = 1; i < uids.size(); i++)
    uids[i] = uids[i - 1] + dist(rng);

  shuffle(uids.begin(), uids.end(), rng);
}

vector<uint64_t> runOtherParty(HeContext& he,
                               Verbosity& verbosity,
                               const CTileTensor& hashTable,
                               CTileTensor& res,
                               int numSamples,
                               int rtsId,
                               int oneOfaliceUids)
{
  vector<uint64_t> uids(numSamples);
  initRandomUids(uids);

  // To ensure that the intersection is not empty, we put one of Alice's UIDs
  uids[numSamples - 1] = oneOfaliceUids;

  DoubleTensor data({numSamples, 4});
  data.initRandom();

  RtsPsiManager psiManager(he, he, rtsId, uids, data, SHARED_SECRET);
  psiManager.setVerbosity(verbosity);

  psiManager.generateIndicatorVector(1, res, hashTable);

  return uids;
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

  HELAYERS_TIMER_PUSH("PSI for FL with multiple parties: total");

  cout << "Run Aggregator's side..." << endl;

  HeaanContext he;
  HeConfigRequirement req(pow(2, 16), 29, 51, 9);
  req.bootstrappable = true;
  req.automaticBootstrapping = true;
  he.init(req);

  AggregatorPsiManager aggregator(he, he);

  cout << "Run Alice's side..." << endl;
  vector<uint64_t> aliceUids(numSamples);
  initRandomUids(aliceUids);

  DoubleTensor aliceData({numSamples, 6});
  aliceData.initRandom();

  RtsPsiManager alicePsiManager(
      he, he, ALICE_RTS_ID, aliceUids, aliceData, SHARED_SECRET);
  alicePsiManager.setVerbosity(verbosity);

  CTileTensor hashTable(he);
  alicePsiManager.insertToHash(hashTable);
  const vector<size_t>& mapping = alicePsiManager.getUidsMapping();

  cout << "Run Aggregator's side..." << endl;
  // The aggregator receives the encrypted hash table from Alice, and pass it to
  // Bob, Charlie and Doug

  cout << "Run Bob's side..." << endl;
  CTileTensor bobIndicatorVector(he);
  auto bobUids = runOtherParty(he,
                               verbosity,
                               hashTable,
                               bobIndicatorVector,
                               numSamples,
                               BOB_RTS_ID,
                               aliceUids[1]);

  cout << "Run Charlie's side..." << endl;
  CTileTensor charlieIndicatorVector(he);
  auto charlieUids = runOtherParty(he,
                                   verbosity,
                                   hashTable,
                                   charlieIndicatorVector,
                                   numSamples,
                                   CHARLIE_RTS_ID,
                                   aliceUids[1]);

  cout << "Run Doug's side..." << endl;
  CTileTensor dougIndicatorVector(he);
  auto dougUids = runOtherParty(he,
                                verbosity,
                                hashTable,
                                dougIndicatorVector,
                                numSamples,
                                DOUG_RTS_ID,
                                aliceUids[1]);

  // The aggregator rearranges the indicators vectors using the mapping sent by
  // Alice, so that the order of the indicators will be the same as the original
  // order of the samples

  cout << "Run Aggregator's side..." << endl;
  CTileTensor bobIndicatorVectorRearranged(he),
      charlieIndicatorVectorRearranged(he), dougIndicatorVectorRearranged(he);
  aggregator.rearrangeIndicatorVector(
      bobIndicatorVectorRearranged, bobIndicatorVector, mapping);
  aggregator.rearrangeIndicatorVector(
      charlieIndicatorVectorRearranged, charlieIndicatorVector, mapping);
  aggregator.rearrangeIndicatorVector(
      dougIndicatorVectorRearranged, dougIndicatorVector, mapping);

  // Finally, Alice receives the encrypted indicators vector from the
  // aggregator. She uses it to privately sort her data, such that the
  // first rows are records that are in the intersection, and the rest of the
  // rows are encryptions of 0s (note that the relative order of the samples
  // that are in the intersection is the same as the relative order of them in
  // the DoubleTensor given to the c'tor). The resulted CTileTensor will be then
  // used in the learning algorithm.

  cout << "Run Alice's side..." << endl;
  CTileTensor finalIndicatorVector(he);
  alicePsiManager.multiplyIndicatorVectors(
      finalIndicatorVector,
      {BOB_RTS_ID, CHARLIE_RTS_ID, DOUG_RTS_ID},
      {bobIndicatorVectorRearranged,
       charlieIndicatorVectorRearranged,
       dougIndicatorVectorRearranged});

  CTileTensor res(he);
  alicePsiManager.compaction(res, finalIndicatorVector);

  HELAYERS_TIMER_POP();

  if (numSamples < 5) {
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
    cout << "Bob's UIDS:" << endl;
    cout << boost::join(bobUids |
                            boost::adaptors::transformed(
                                [](uint64_t uid) { return to_string(uid); }),
                        ", ")
         << endl
         << endl;
    cout << "Charlie's UIDS:" << endl;
    cout << boost::join(charlieUids |
                            boost::adaptors::transformed(
                                [](uint64_t uid) { return to_string(uid); }),
                        ", ")
         << endl
         << endl;
    cout << "Doug's UIDS:" << endl;
    cout << boost::join(dougUids |
                            boost::adaptors::transformed(
                                [](uint64_t uid) { return to_string(uid); }),
                        ", ")
         << endl
         << endl;
    aliceData.debugPrint("Alice's Data", 1);
    cout << endl;
    res.debugPrint(
        "Protocol Output (In a real use of the protocol, Alice will not "
        "be able to see this content, and so will not learn "
        "anything about which samples are in the intersection)");
    cout << endl;
  }

  return 0;
}