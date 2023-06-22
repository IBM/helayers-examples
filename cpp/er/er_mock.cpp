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

#include "er/RecordLinkageManager.h"
#include "helayers/hebase/AlwaysAssert.h"
#include "helayers/hebase/HelayersTimer.h"
#include "helayers/hebase/utils/HelayersConfig.h"

using namespace std;
using namespace helayers;
using namespace er;

void initSharedConfig(RecordLinkageConfig& config);
vector<RecordLinkageRule> initRules(RecordLinkageConfig& config);
pair<int, int> runProtocolIteration(RecordLinkageMockManager& alice,
                                    RecordLinkageMockManager& bob,
                                    RecordLinkageRule& rule);

/*
This file implements a mock version of the PPRL protocol implemented in
er_basic_example. The functionality is exactly the same, but there is no
security. meaning, the records are not encrypted. Use this for basic testing and
debugging.
*/

void help()
{
  cout << "Usage: ./er_basic_example [--num_records n] [--verbose]" << endl;
  cout << "--num_records n\tAn optional integer parameter indicating the "
          "number of records to analyze from both tables. "
       << endl
       << "\t\tThe default is to analyze 1000 records from each side." << endl
       << "\t\tUse -1 to analyze all the records, but take into account that "
          "this may take some time to complete."
       << endl;
  cout << "--verbose\tAn optional flag sets the verbosity level to high "
          "verbosity."
       << endl;
  cout << "--quiet\tAn optional flag sets the verbosity level to no verbosity."
       << endl;
  exit(1);
}

void printHeader(int numRecords, const RecordLinkageConfig& config)
{
  cout << std::string(70, '=') << endl;
  if (numRecords == -1)
    cout << "Comparing all the records. This may take some time to complete."
         << endl;
  else
    cout << "Comparing " << numRecords << " records from each side." << endl;
  cout << "Number of bands: " << config.getNumBands() << endl;
  cout << "Size of bands  : " << config.getSizeBands() << endl;
}

int main(int argc, char* argv[])
{
  int numRecords = 1000;
  Verbosity verbosity = VERBOSITY_REGULAR;

  int i = 1;
  while (i < argc) {
    string arg = argv[i++];
    if (arg == "--num_records")
      numRecords = stoi(argv[i++]);
    else if (arg == "--verbose")
      verbosity = VERBOSITY_DETAILED;
    else if (arg == "--quiet")
      verbosity = VERBOSITY_NONE;
    else
      help();
  }

  HELAYERS_TIMER_PUSH("Mock PPRL: total");

  RecordLinkageConfig config;

  initSharedConfig(config);

  config.setVerbosity(verbosity);
  printHeader(numRecords, config);

  vector<RecordLinkageRule> rules = initRules(config);

  RecordLinkageMockManager alice(config), bob(config);

  alice.initRecordsFromFile(getDataSetsDir() + "/er/out1.csv", numRecords);
  bob.initRecordsFromFile(getDataSetsDir() + "/er/out2.csv", numRecords);

  vector<pair<int, int>> resultsForIteration;
  for (RecordLinkageRule& rule : rules)
    resultsForIteration.push_back(runProtocolIteration(alice, bob, rule));

  auto res = alice.reportMatchedRecordsAlongWithOtherSideRecords(bob, true);
  int numMatches = res.first;
  int numBlocked = res.second;

  HELAYERS_TIMER_POP();
  if (verbosity > VERBOSITY_NONE) {
    cout << std::string(70, '=') << endl;
    cout << "Timing statistics overview:" << endl;
    HELAYERS_TIMER_PRINT_MEASURES_SUMMARY();

    // Print some meta data
    cout << std::string(70, '=') << endl;
    cout << "Number of records analyzed from Alice's side    : "
         << alice.getNumOfRecords() << endl;
    cout << "Number of records analyzed from Bob's side      : "
         << bob.getNumOfRecords() << endl
         << endl;
    for (size_t i = 0; i < resultsForIteration.size(); i++) {
      cout << "Number of matching records after running rule #" << i + 1 << ": "
           << resultsForIteration[i].first << endl;
      cout << "Number of blocked records after running rule #" << i + 1 << " : "
           << resultsForIteration[i].second << endl
           << endl;
    }
    cout << "Total number of matching records                : " << numMatches
         << endl;
    cout << "Total number of blocked records                 : " << numBlocked
         << endl;
    cout << std::string(70, '=') << endl;
  }

  // Sanity checks
  if ((numRecords == -1) || (numRecords >= 10000))
    always_assert_msg(numMatches >= 10, "expecting at least 10 matches");

  return 0;
}

void initSharedConfig(RecordLinkageConfig& config)
{
  vector<string> fieldNames = {"first_name",
                               "last_name",
                               "email",
                               "email_domain",
                               "address_number",
                               "address_location",
                               "address_line2",
                               "city",
                               "state",
                               "country",
                               "zip_base",
                               "zip_ext",
                               "phone_area_code",
                               "phone_exchange_code",
                               "phone_line_number"};

  config.setNumBandsAndSizeBands(40, 14);

  config.setRecordsFields(fieldNames, "first_name");
}

vector<RecordLinkageRule> initRules(RecordLinkageConfig& config)
{
  RecordLinkageRule rule1(config);
  rule1.setField("first_name", RL_RULE_EQUAL);
  rule1.setField("last_name", RL_RULE_EQUAL);
  rule1.setField("email", RL_RULE_EQUAL);
  rule1.setField("email_domain", RL_RULE_EQUAL);

  RecordLinkageRule rule2(config);
  rule2.setField("first_name", RL_RULE_SIMILAR, 2, 4);
  rule2.setField("last_name", RL_RULE_SIMILAR, 2, 4);
  rule2.setField("email", RL_RULE_SIMILAR, 2, 4);
  rule2.setField("phone_line_number", RL_RULE_SIMILAR, 2, 4);
  rule2.setField("address_location", RL_RULE_SIMILAR, 1, 5);
  rule2.setField("address_number", RL_RULE_SIMILAR, 1, 5);
  rule2.setField("city", RL_RULE_SIMILAR, 1, 5);

  RecordLinkageRule rule3(config);
  rule3.setField("address_number", RL_RULE_EQUAL);
  rule3.setField("city", RL_RULE_EQUAL);
  rule3.setField("state", RL_RULE_EQUAL);
  rule3.setField("country", RL_RULE_EQUAL);
  rule3.setField("email_domain", RL_RULE_EQUAL);
  rule3.setField("first_name", RL_RULE_SIMILAR, 1, 3);
  rule3.setField("last_name", RL_RULE_SIMILAR, 1, 3);
  rule3.setField("email", RL_RULE_SIMILAR, 1, 3);
  rule3.setField("address_location", RL_RULE_SIMILAR, 1, 3);

  return {rule1, rule2, rule3};
}

pair<int, int> runProtocolIteration(RecordLinkageMockManager& alice,
                                    RecordLinkageMockManager& bob,
                                    RecordLinkageRule& rule)
{
  alice.setCurrentRule(rule);
  bob.setCurrentRule(rule);

  RecordLinkageMockPackage packageAlice = alice.mockEncryptFieldsForEqualRule();
  RecordLinkageMockPackage packageBob = bob.mockEncryptFieldsForEqualRule();

  alice.mockMatchRecordsByEqualRule(packageAlice, packageBob);
  bob.mockMatchRecordsByEqualRule(packageBob, packageAlice);

  packageAlice = alice.mockEncryptFieldsForSimilarRule();
  packageBob = bob.mockEncryptFieldsForSimilarRule();

  alice.mockMatchRecordsBySimilarRule(packageAlice, packageBob);
  bob.mockMatchRecordsBySimilarRule(packageBob, packageAlice);

  return alice.reportMatchedRecordsAlongWithOtherSideRecords(bob, true);
}