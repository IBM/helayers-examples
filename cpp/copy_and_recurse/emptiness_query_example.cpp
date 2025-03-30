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

#include "helayers/db/CopyAndRecurseDatabase.h"
#include "helayers/hebase/openfhe/OpenFheCkksContext.h"
#include "helayers/hebase/mockup/MockupContext.h"
#include "helayers/hebase/mockup/EmptyContext.h"
#include "helayers/hebase/utils/PrintUtils.h"
#include "helayers/math/FunctionEvaluator.h"

using namespace std;
using namespace helayers;

/*
The example demonstrates the use of CopyAndRecurseDatabase class for encrypted
database querying of emptiness.
*/

/// Verbosity options
Verbosity verbosity = VERBOSITY_NONE;
bool reportAllTimers = false;

// Emptiness query benchmark options
bool naive = false;
size_t numElements = 16;
size_t rangeSize = 100;
int numChildren = 3;
int repeats = 1;
int gRep = 4;
int fRep = 1;

// Context options
bool mockupContext = false;
bool emptyContext = false;

int numSlots = pow(2, 15);
int multiplicationDepth = 30;
int fractionalPartPrecision = 40;
int integerPartPrecision = 7;
bool gpu = false;

void help()
{
  cout << "Usage: ./emptiness_query_example [ additional optional parameters ]"
       << endl;
  cout << endl;

  cout << "Verbosity options:" << endl;
  cout << "--verbose \tsets the verbosity level to regular (default is none)."
       << endl;
  cout << "--timers \tprints all timers at the end of the example." << endl;
  cout << endl;

  cout << "Emptiness query benchmark options:" << endl;
  cout << "--elements n\tan integer parameter that sets the number of database "
          "elements to generate for the example."
       << endl;
  cout << "--range n\tan integer parameter that sets the range of the elements "
          "in the generated database for the example (elements will be chosen "
          "randomly from range [0, range])."
       << endl;
  cout << "--children n\tan integer parameter that sets the number of "
          "children for each node in the partition tree data structure of "
          "CopyAndRecurseDatabase."
       << endl;
  cout << "--repeats n\tan integer parameter that sets the number of "
          "repetitions of the benchmark run."
       << endl;
  cout << "--f_rep n\tan integer parameter that controls the accuracy (and "
          "depth) of the comparison method under encryption."
       << endl;
  cout << "--g_rep n\tan integer parameter that controls the accuracy (and "
          "depth) of the comparison method under encryption."
       << endl;
  cout << endl;

  cout << "Context options:" << endl;
  cout << "--mockup\truns the example with a mockup context for simulation."
       << endl;
  cout << "--empty\truns the example with an empty context for counting number "
          "of operations."
       << endl;
  cout << "--slots n\tsets the number of slots in the HE context." << endl;
  cout << "--depth n\tsets the multiplication depth in the HE context." << endl;
  cout << "--frac n\tsets the fractional precision in the HE context." << endl;
  cout << "--int n\tsets the integer precision in the HE context." << endl;
  cout << "--gpu\tWhether to run on a GPU (if one is available)." << endl;
  exit(1);
}

void printHeader(shared_ptr<const HeContext> he)
{
  cout << "*** Starting emptiness query demo ***" << endl;
  cout << "Generating random database with " << numElements
       << " elements (integers) in range [0, " << rangeSize << "]." << endl;
  cout << "Number of children in partition tree: " << numChildren << endl;
  cout << "Verbose: " << PrintUtils::boolToString(verbosity > VERBOSITY_NONE)
       << endl;
  cout << endl;
  cout << "HeContext info:" << endl;
  he->printSignature(cout);
}

vector<uint16_t> getElementsInRange(const vector<uint16_t>& database,
                                    double queryStart,
                                    double queryEnd);
shared_ptr<HeContext> initContext();
CTile compare(const FunctionEvaluator& fe, const CTile& a, const CTile& b);
bool copyAndRecurseEmptinessQuery(shared_ptr<HeContext> he,
                                  const vector<uint16_t>& database,
                                  double queryStart,
                                  double queryEnd);
bool naiveEmptinessQuery(shared_ptr<HeContext> he,
                         const vector<uint16_t>& database,
                         double queryStart,
                         double queryEnd);

int main(int argc, char** argv)
{
  // Read optional args from cmd. See instructions in help() function.
  // For example, to run the demo in verbose mode on a database with 100
  // elements from range [0, 100] using a partition tree with 5 children for
  // every node, run
  // ./emptiness_query_example --verbose --elements 100 --range 100 --children 5
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "--elements")
      numElements = atoll(argv[++i]);
    else if (std::string(argv[i]) == "--range")
      rangeSize = atoll(argv[++i]);
    else if (std::string(argv[i]) == "--children")
      numChildren = atoi(argv[++i]);
    else if (std::string(argv[i]) == "--verbose")
      verbosity = VERBOSITY_REGULAR;
    else if (std::string(argv[i]) == "--gpu")
      gpu = true;
    else if (std::string(argv[i]) == "--timers")
      reportAllTimers = true;
    else if (std::string(argv[i]) == "--mockup")
      mockupContext = true;
    else if (std::string(argv[i]) == "--empty")
      emptyContext = true;
    else if (std::string(argv[i]) == "--repeats")
      repeats = atoi(argv[++i]);
    else if (std::string(argv[i]) == "--f_rep")
      fRep = atoi(argv[++i]);
    else if (std::string(argv[i]) == "--g_rep")
      gRep = atoi(argv[++i]);
    else if (std::string(argv[i]) == "--slots")
      numSlots = atoi(argv[++i]);
    else if (std::string(argv[i]) == "--depth")
      multiplicationDepth = atoi(argv[++i]);
    else if (std::string(argv[i]) == "--frac")
      fractionalPartPrecision = atoi(argv[++i]);
    else if (std::string(argv[i]) == "--int")
      integerPartPrecision = atoi(argv[++i]);
    else {
      cout << "Unsupported argument: " << argv[i] << endl;
      help();
    }
  }

  // Initialize the HeContext.
  shared_ptr<HeContext> he = initContext();
  printHeader(he);

  for (int repeat = 0; repeat < repeats; repeat++) {

    if (repeats > 1)
      cout << endl << "Repeat #" << repeat + 1 << endl;

    // Generate a random database to encrypt.
    vector<uint16_t> database(numElements);
    double queryStart, queryEnd;
    uniform_int_distribution<> unif(0, rangeSize + 1);
    default_random_engine re(time(0));
    for (size_t i = 0; i < database.size(); i++)
      database[i] = unif(re);

    // Generate random query range.
    queryStart = -0.5;
    queryEnd = unif(re) + 0.5;

    // Compute the expected result in plain.
    vector<uint16_t> elementsInRange =
        getElementsInRange(database, queryStart, queryEnd);
    bool expectedRes = elementsInRange.empty();

    // Performs the emptiness query.
    bool naiveRes = naiveEmptinessQuery(he, database, queryStart, queryEnd);
    bool copyAndRecurseRes =
        copyAndRecurseEmptinessQuery(he, database, queryStart, queryEnd);

    cout << endl << string(30, '=') << endl;

    if (reportAllTimers) {
      HELAYERS_TIMER_PRINT_MEASURES_SUMMARY();
    } else {
      HELAYERS_TIMER_PRINT_MEASURE_SUMMARY("copy-and-recurse-db-init");
      HELAYERS_TIMER_PRINT_MEASURE_SUMMARY("naive-db-init");
      HELAYERS_TIMER_PRINT_MEASURE_SUMMARY("copy-and-recurse-emptiness-query");
      HELAYERS_TIMER_PRINT_MEASURE_SUMMARY("naive-emptiness-query");
    }

    cout << endl << string(30, '=') << endl;

    cout << "Queried range [" << queryStart << ", " << queryEnd << "]" << endl;
    cout << "Query expected result (is range empty?):\t"
         << PrintUtils::boolToString(expectedRes)
         << (expectedRes || verbosity == VERBOSITY_NONE
                 ? ""
                 : " (elements in range: " +
                       PrintUtils::containerToString(elementsInRange) + ")")
         << endl;
    cout << "Query result Copy-And-Recurse:\t\t\t"
         << PrintUtils::boolToString(copyAndRecurseRes) << endl;
    cout << "Query result naive algorithm:\t\t\t"
         << PrintUtils::boolToString(naiveRes) << endl;

    if (expectedRes != copyAndRecurseRes)
      throw runtime_error("invalid results");
  }
}

vector<uint16_t> getElementsInRange(const vector<uint16_t>& database,
                                    double queryStart,
                                    double queryEnd)
{
  vector<uint16_t> res;
  for (auto& elem : database)
    if (elem >= queryStart && elem <= queryEnd)
      res.push_back(elem);

  return res;
}

shared_ptr<HeContext> initContext()
{
  HeConfigRequirement req(numSlots,
                          multiplicationDepth,
                          fractionalPartPrecision,
                          integerPartPrecision);
  req.bootstrappable = true;
  req.automaticBootstrapping = true;
  BootstrapConfig bsConfig;
  bsConfig.range = EXTENDED_RANGE;
  req.bootstrapConfig = bsConfig;

  shared_ptr<HeContext> he = make_shared<OpenFheCkksContext>();

  if (mockupContext || emptyContext) {
    shared_ptr<TrackingContext> trackingContext =
        mockupContext
            ? shared_ptr<TrackingContext>(make_shared<MockupContext>())
            : shared_ptr<TrackingContext>(make_shared<EmptyContext>());
    trackingContext->setEstimatedMeasures(he->getEstimatedMeasures());
    he = trackingContext;

    req.securityLevel = 0;
    req.bootstrapConfig = BootstrapConfig();
    req.bootstrapConfig->targetChainIndex = 12;
    req.bootstrapConfig->minChainIndexForBootstrapping = 3;
  }

  he->init(req);
  he->setAutomaticBootstrapping(true);

  if (gpu)
    he->setDefaultDevice(DEVICE_GPU);

  return he;
}

bool copyAndRecurseEmptinessQuery(shared_ptr<HeContext> he,
                                  const vector<uint16_t>& database,
                                  double queryStart,
                                  double queryEnd)
{
  Encoder enc(*he);

  CopyAndRecurseDatabase qd(*he);
  qd.setVerbosity(verbosity);

  {
    HELAYERS_TIMER("copy-and-recurse-db-init");
    qd.init(database, numChildren);
  }

  FunctionEvaluator fe(*he);
  function<CTile(const CTile&, const CTile&)> lambda =
      [&fe](const CTile& a, const CTile& b) -> CTile {
    return compare(fe, a, b);
  };
  qd.setCompareMethod(lambda);

  CopyAndRecurseDatabase::Range range = qd.encryptRange(queryStart, queryEnd);
  CTile cRes(*he);

  if (mockupContext || emptyContext)
    dynamic_cast<TrackingContext&>(*he).startOperationCountTrack();

  {
    // Synchronize the GPU (if there is one) before the start of the run and
    // after it finishes. This is needed for accurate time measurement.
    he->cudaDeviceSynchronize();
    HELAYERS_TIMER("copy-and-recurse-emptiness-query");
    qd.emptinessQuery(cRes, range);
    he->cudaDeviceSynchronize();
  }

  if (mockupContext || emptyContext) {
    cout << "Copy-And-Recurse operation count:" << endl;
    dynamic_cast<const TrackingContext&>(*he).printStatsAndClear(cout);
  }

  return enc.decryptDecodeInt(cRes).at(0);
}

bool naiveEmptinessQuery(shared_ptr<HeContext> he,
                         const vector<uint16_t>& database,
                         double queryStart,
                         double queryEnd)
{
  Encoder enc(*he);

  // Encrypts the range to query.
  CTile cStart(*he), cEnd(*he);
  enc.encodeEncrypt(cStart, queryStart);
  enc.encodeEncrypt(cEnd, queryEnd);

  vector<CTile> encryptedDatabase(
      ceil((double)database.size() / (double)he->slotCount()), CTile(*he));

  {
    HELAYERS_TIMER("naive-db-init");
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
  CTile ones(*he);
  enc.encodeEncrypt(ones, 1);

  CTile cRes(ones);

  if (mockupContext || emptyContext)
    dynamic_cast<TrackingContext&>(*he).startOperationCountTrack();

  {
    // Synchronize the GPU (if there is one) before the start of the run and
    // after it finishes. This is needed for accurate time measurement.
    he->cudaDeviceSynchronize();
    HELAYERS_TIMER("naive-emptiness-query");

    for (const auto& elem : encryptedDatabase) {
      CTile tmp = compare(fe, elem, cStart);
      tmp.multiply(compare(fe, cEnd, elem));
      tmp.sub(ones);
      tmp.multiplyScalar(-1);
      cRes.multiply(tmp);
    }

    // Since we packed the database in SIMD fashion, we need to compute the big
    // AND of all bits in the resulted ciphertext, to get the final indicator.
    for (int rot = 1; rot < he->slotCount(); rot *= 2) {
      CTile tmp(cRes);
      tmp.rotate(rot);
      cRes.multiply(tmp);
    }
    he->cudaDeviceSynchronize();
  }

  if (mockupContext || emptyContext) {
    cout << "Naive algorithm operation count:" << endl;
    dynamic_cast<const TrackingContext&>(*he).printStatsAndClear(cout);
  }

  return enc.decryptDecodeInt(cRes).at(0);
}

CTile compare(const FunctionEvaluator& fe, const CTile& a, const CTile& b)
{
  return fe.compare(a, b, gRep, fRep, rangeSize);
}