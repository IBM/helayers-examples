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

#include <vector>
#include <random>
#include <numeric>

#include "helayers/db/QueryDatabase.h"
#include "helayers/hebase/heaan/HeaanContext.h"
#include "helayers/hebase/mockup/EmptyContext.h"

using namespace std;
using namespace helayers;

int plaintextRangeQuery(const vector<uint16_t>& database,
                        double queryStart,
                        double queryEnd);
shared_ptr<HeContext> initContext(bool empty);
int rangeQuery(shared_ptr<HeContext> he,
               const vector<uint16_t>& database,
               const CTile& start,
               const CTile& end);
int naiveRangeQuery(shared_ptr<HeContext> he,
                    const vector<uint16_t>& database,
                    const CTile& start,
                    const CTile& end);

bool reportAllTimers = false;
bool naive = false;
int numElements = 16;
int rangeSize = 100;
int numChildren = 3;
Verbosity verbosity = VERBOSITY_NONE;

/*
The example demonstrates the use of QueryDatabase class for encrypted database
querying.
*/

void help()
{
  cout << "Usage: ./range_query_example [--verbose] [--all_timers] [--naive] "
          "[--empty] [--num_elements n] [--range_size n] [--num_children n]"
       << endl;
  cout << "--num_records n\tan integer parameter that sets the number of "
          "database elements to generate for the example."
       << endl;
  cout << "--range_size n\tan integer parameter that sets the range of the "
          "elements in the generated database for the example."
       << endl;
  cout << "--num_children n\tan integer parameter that sets the number of "
          "children for each node in the partition tree data structure of "
          "QueryDatabase."
       << endl;
  cout << "--verbose n\tsets the verbosity level to regular (default is none)."
       << endl;
  cout << "--all_timers n\tprints all timers at the end of the example."
       << endl;
  cout << "--naive n\truns the naive secure range query algorithm." << endl;
  cout << "--empty n\truns the example with an empty context for simulation."
       << endl;
  exit(1);
}

void printHeader(shared_ptr<HeContext> he)
{
  cout << "*** Starting range query demo [" << numElements
       << " elements in range 0.." << rangeSize - 1 << "] ***" << endl;
  cout << "Naive mode: " << (naive ? "yes" : "no") << endl;
  if (!naive) {
    cout << "Number of children in tree: " << numChildren << endl;
  }
  cout << "Verbose: " << (verbosity > VERBOSITY_NONE ? "yes" : "no") << endl;
  cout << endl;
  cout << "HeContext info:" << endl;
  he->printSignature(cout);
}

int main(int argc, char** argv)
{
  bool empty = false;

  // Read optional args from cmd. See instructions in help() function.
  // For example, to run the demo in maximal verbosity on a 1000
  // elements database in range [0,234] using a partition tree with 5
  // children for every node, run
  // ./range_query_example --verbose --all_timers --num_elements 1000
  // --range_size 234 --num_children 5
  for (int i = 0; i < argc; ++i) {
    if (std::string(argv[i]) == "--num_elements")
      numElements = atoi(argv[++i]);
    if (std::string(argv[i]) == "--range_size")
      rangeSize = atoi(argv[++i]);
    if (std::string(argv[i]) == "--num_children")
      numChildren = atoi(argv[++i]);
    if (std::string(argv[i]) == "--verbose")
      verbosity = VERBOSITY_REGULAR;
    if (std::string(argv[i]) == "--all_timers")
      reportAllTimers = true;
    if (std::string(argv[i]) == "--naive")
      naive = true;
    if (std::string(argv[i]) == "--empty")
      empty = true;
  }

  // Initialize the HeContext.
  shared_ptr<HeContext> he = initContext(empty);
  printHeader(he);

  // Generate a random database to encrypt.
  vector<uint16_t> database(numElements);
  double queryStart, queryEnd;
  uniform_int_distribution<> unif(0, rangeSize);
  default_random_engine re(time(0));
  for (size_t i = 0; i < database.size(); i++) {
    database[i] = unif(re);
  }

  // Generate random query range.
  queryStart = -0.5;
  queryEnd = unif(re) + 0.5;

  // Compute the expected result in plain.
  int expectedRes = plaintextRangeQuery(database, queryStart, queryEnd);

  // Encrypts the range to query.
  Encoder enc(*he);
  CTile cStart(*he), cEnd(*he);
  enc.encodeEncrypt(cStart, queryStart);
  enc.encodeEncrypt(cEnd, queryEnd);

  int res;

  // Performs the range query.
  if (naive) {
    res = naiveRangeQuery(he, database, cStart, cEnd);
  } else {
    res = rangeQuery(he, database, cStart, cEnd);
  }

  cout << endl << string(30, '=') << endl;

  if (reportAllTimers) {
    HELAYERS_TIMER_PRINT_MEASURES_SUMMARY();
  } else {
    HELAYERS_TIMER_PRINT_MEASURE_SUMMARY("encrypted-db-init");
    HELAYERS_TIMER_PRINT_MEASURE_SUMMARY("query");
  }

  cout << endl << string(30, '=') << endl;

  cout << "Queried [" << queryStart << ", " << queryEnd << "]" << endl;
  cout << "Query expected result:\t\t" << expectedRes << endl;
  cout << "Query result:\t\t\t" << res << endl;
}

int plaintextRangeQuery(const vector<uint16_t>& database,
                        double queryStart,
                        double queryEnd)
{
  int res = 0;
  for (auto& elem : database) {
    if (elem >= queryStart && elem <= queryEnd) {
      res++;
    }
  }

  return res;
}

shared_ptr<HeContext> initContext(bool empty)
{
  HeConfigRequirement req(pow(2, 15), // numSlots
                          12,         // multiplicationDepth
                          42,         // fractionalPartPrecision
                          18          // integerPartPrecision
  );

  req.bootstrappable = true;
  req.automaticBootstrapping = true;
  BootstrapConfig bsConfig;
  bsConfig.range = EXTENDED_RANGE;
  req.bootstrapConfig = bsConfig;

  shared_ptr<HeContext> he;
  if (empty) {
    he = make_shared<EmptyContext>();
    req.securityLevel = 0;
  } else {
    he = make_shared<HeaanContext>();
  }
  he->init(req);
  he->setAutomaticBootstrapping(true);
  return he;
}

int rangeQuery(shared_ptr<HeContext> he,
               const vector<uint16_t>& database,
               const CTile& start,
               const CTile& end)
{
  Encoder enc(*he);

  QueryDatabase qd(*he);
  qd.setVerbosity(verbosity);

  {
    HELAYERS_TIMER("encrypted-db-init");
    qd.init(database, numChildren);
  }

  CTile cRes(*he);
  {
    HELAYERS_TIMER("query");
    qd.countQuery(cRes, start, end, 4, 1, rangeSize);
  }

  auto res = enc.decryptDecodeInt(cRes);
  return accumulate(res.begin(), res.end(), 0);
}

int naiveRangeQuery(shared_ptr<HeContext> he,
                    const vector<uint16_t>& database,
                    const CTile& start,
                    const CTile& end)
{
  Encoder enc(*he);

  vector<CTile> encryptedDatabase(
      ceil((double)database.size() / (double)he->slotCount()), CTile(*he));

  {
    HELAYERS_TIMER("encrypted-db-init");
    for (size_t i = 0; i < database.size(); i += he->slotCount()) {
      vector<int> tmp(database.begin() + i,
                      i + he->slotCount() >= database.size()
                          ? database.end()
                          : database.begin() + i + he->slotCount());

      // fill unused slots with -1
      while (tmp.size() < he->slotCount()) {
        tmp.push_back(-1);
      }

      enc.encodeEncrypt(encryptedDatabase.at(i / he->slotCount()), tmp);
    }
  }

  FunctionEvaluator fe(*he);
  CTile cRes(*he);
  enc.encodeEncrypt(cRes, 0);
  {
    HELAYERS_TIMER("query");

    for (const auto& elem : encryptedDatabase) {
      CTile tmp = fe.compare(elem, start, 4, 1, database.size());
      tmp.multiply(fe.compare(end, elem, 4, 1, database.size()));
      cRes.add(tmp);
    }
  }

  auto res = enc.decryptDecodeInt(cRes);
  return accumulate(res.begin(), res.end(), 0);
}