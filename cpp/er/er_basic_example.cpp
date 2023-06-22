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
pair<int, int> runProtocolIteration(RecordLinkageManager& alice,
                                    RecordLinkageManager& bob,
                                    RecordLinkageRule& rule);

/*
  This file implements the Privacy Preserving Record Linkage (PPRL) Protocol
  between two parties, called Alice and Bob.
  Alice and Bob both have their own DB with ~500,000 private records, and the
  objective here is for the two sides to learn which of their records match
  similar records in the other side's DB but nothing else (beyond also
  incidentally the number of records in the other side's DB). So for example,
  they don't get to learn anything about the non-similar records of the other
  side, nor the similar but possibly different fields of the identified similar
  record of the other side.

  The protocol is similar to the Private-Set-Intersection (PSI) protocol,
  except that it allowes for similarities rather than insisting on exact
  equivalence of the reported candidate pairs. The similarity of the records is
  measured in terms of the jaccard similarity index of the two records (see
  https://en.wikipedia.org/wiki/Jaccard_index) and uses min-hashing to estimate
  this similarity measure. For performance reasons, the Record-Linkage algorithm
  is statistical so that matched pairs of records probably have a high Jaccard
  similarity index.
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
  // Read args from cmd
  // numRecords is an optional integer flag that sets the number of records
  // to read from Alice and Bob's tables (by default 1000 records).
  // To compare all the records use --num_records -1
  // verbosity is an optional flag that sets the verbosity level.
  // For example, to run the protocol on just 10000 out of the 500000 records,
  // run
  // ./er_basic_example --num_records 10000
  // to run the protocol with high verbosity, run
  // ./er_basic_example --verbose
  // to run the protocol with no verbosity, run
  // ./er_basic_example --quiet
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

  HELAYERS_TIMER_PUSH("PPRL: total");

  RecordLinkageConfig config;

  // Here we define the Record-Linkage configuration shared by both parties.
  // This includes the list of record field names and some
  // tuning of the Record-Linkage algorithm and heuristics.
  // See implementation of initSharedConfig below.
  initSharedConfig(config);

  config.setVerbosity(verbosity);
  printHeader(numRecords, config);

  // Here we define the rules by which we will consider two records as linked.
  // See implementation of initRules below.
  vector<RecordLinkageRule> rules = initRules(config);

  // Construct the RecordLinkageManager which manages the PPRL protocol
  RecordLinkageManager alice(config), bob(config);

  // Read the Alice and Bob's tables from the pair of csv files.
  // This step also processes the records (creates shingles and computes the
  // min-hashes) and encrypts the processed information (thus plaing the part of
  // the 1st party in a Diffie-Hellman like protocol).
  alice.initRecordsFromFile(getDataSetsDir() + "/er/out1.csv", numRecords);
  bob.initRecordsFromFile(getDataSetsDir() + "/er/out2.csv", numRecords);

  // Here we run the protocol. Each iteration we apply different rule to find
  // linked records. See implementation of runProtocolIteration below.
  vector<pair<int, int>> resultsForIteration;
  for (RecordLinkageRule& rule : rules) {
    resultsForIteration.push_back(runProtocolIteration(alice, bob, rule));
  }

  // Finally, we're ready to Compare the two sets of doubly encrypted PPRL
  // information for the records of the two sides and report the matching pairs
  // of records.
  //
  // For every record of Alice that has a duplicate in Bob's table, the report
  // includes the number of shared "bands" for the pair. This is a tehcnical
  // term that relates to the min-hash algorithm that is used to compare the
  // records, but in general at least one shared band is required and sufficient
  // to warrent a report of a probable candidate pair of records. More shared
  // bands indicate a higher probability that the pair of records indeed
  // describe the same entity.
  //
  // Thus the following example indicates that the reported record from Alice's
  // DB has an identical record in Bob's DB (with all the bands matching).
  // RecordA: Fredericka, Martin, Fredericka.Martin, stanford.edu, 28598, 150
  // Dewey St,
  //   Mountain Home,ID,83647, Suite 68325, Bozoo, OH, USA, 81783, 5338, 458,
  //   845, 8312
  // RecordB: Fredericka, Martin, Fredericka.Martin, stanford.edu, 28598, 150
  // Dewey St,
  //   Mountain Home,ID,83647, Suite 68325, Bozoo, OH, USA, 81783, 5338, 458,
  //   845, 8312
  // Number of matching bands: 110
  //
  // The following example indicates that the reported record from Alice's
  // DB has a similar record in Bob's DB (with just one matching band).
  // RecordA: Rudy, Chari, Rudy.Chari, technion.ac.il , 86152, 4101 Mc Innis St,
  // San Antonio,TX,
  //   78222, Suite 39868, Nackawic, NM, USA, 25976, 6561, 952, 229, 3276
  // RecordB: Jodi, Flatt, Jodi.Flatt, technion.ac.il , 80993, 4101 Mc Innis St.
  // San Antonio,TX,
  //   78222, Box 95836, Typo, MN, USA, 68335, 5160, 562, 853, 2044
  // Number of matching bands: 1
  //
  // Note that the printout produced here prints Alice's record and also Bob's
  // matching record. This is posisble here because this program has access both
  // to Alice and Bob's records and the print method called by alice below
  // indeed receives Bob's table information. This is for debuging purposes
  // only, and a more realistic application would call
  //   int numMatches = alice.reportMatchedRecords();
  // which would only print the records of Alice that have a similar record in
  // Bob's table but not actually print Bob's record.
  pair<int, int> res;
  if (verbosity > VERBOSITY_NONE) {
    res = alice.reportMatchedRecordsAlongWithOtherSideRecords(bob, true);
  } else {
    res = alice.getNumMatchedRecords(true);
  }
  int numMatches = res.first;
  int numBlocked = res.second;

  HELAYERS_TIMER_POP();
  if (verbosity > VERBOSITY_NONE) {
    // Print timing statistics
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
  if ((numRecords == -1) || (numRecords >= 10000)) {
    always_assert_msg(numMatches >= 10, "expecting at least 10 matches");
  }

  cout << "Finished successfully" << endl;

  return 0;
}

void initSharedConfig(RecordLinkageConfig& config)
{
  // Here we define the Record-Linkage configuration shared by both parties.
  // This includes the list of record field names and some
  // tuning of the Record-Linkage algorithm and heuristics.
  // The list of field names in Alice and Bob's tables' schema
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

  // The min-hash algorithm will use a total of 560 hashes
  // arranged in 40 bands with 14 hashes in each band
  config.setNumBandsAndSizeBands(40, 14);

  // Set the configuration with the field names and groups defined above.
  // The field that gives the person name is specifically indicated
  // in the last parameter. This is later used in some name related heuristics.
  config.setRecordsFields(fieldNames, "first_name");
}

vector<RecordLinkageRule> initRules(RecordLinkageConfig& config)
{
  // Here we define the rules by which we will consider two records as linked.
  // Each rule defines for each field a rule type - either RL_RULE_EQUAL,
  // RL_RULE_SIMILAR or RL_RULE_NONE (which is the default rule type).
  //
  // Fields with RL_RULE_EQUAL rule type implies that two records to be
  // considered linked if their content of these fields is exactly the same.
  //
  // Fields with RL_RULE_SIMILAR rule type implies that records to be
  // considered linked if their content of these fields have high Jaccard
  // simmilarity. We can also optionaly set the weight and size of shingles
  // generated for every such field.
  //
  // Fields with RL_RULE_NONE rule type are not taken into account in the record
  // linkage process
  //
  // For two records to be considered linked, ALL the conditions in the specific
  // rule must apply. For example, if first_name is set to RL_RULE_EQUAL and
  // address_location is set to RL_RULE_SIMILAR then two records considered
  // linked if their first_name content is equal AND their address_location
  // content is similar.
  //
  // We can run the protocol with a number of rules iteratively. Two records are
  // considered linked if at least one of the rules applies for them. The order
  // of the rules matter - After a record has been matched, it will not be
  // considered as a candidate at the following iterations.

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

pair<int, int> runProtocolIteration(RecordLinkageManager& alice,
                                    RecordLinkageManager& bob,
                                    RecordLinkageRule& rule)
{
  // At each iteration we need to set the rule we want to run.
  alice.setCurrentRule(rule);
  bob.setCurrentRule(rule);

  // Both sides exchange their encrypted data. At an iteration that runs with a
  // rule that combines RL_RULE_EQUAL rule type fields and RL_RULE_SIMILAR rule
  // type field, we must run the RL_RULE_EQUAL related functions first.
  RecordLinkagePackage packageAlice = alice.encryptFieldsForEqualRule();
  RecordLinkagePackage packageBob = bob.encryptFieldsForEqualRule();

  // Both sides receive the encrypted information from the other side,
  // and then add their own encryption layer (thus playing the part of the 2nd
  // party in a Diffie-Hellman like protocol).
  alice.applySecretKeyToRecords(packageBob);
  bob.applySecretKeyToRecords(packageAlice);

  // Both parties receives the doubly encrypted PPRL information of their own
  // records from the other party. They then processes this together with the
  // doubly encrypted PPRL information they computed for the other party's
  // records.
  alice.matchRecordsByEqualRule(packageAlice, packageBob);
  bob.matchRecordsByEqualRule(packageBob, packageAlice);

  // We now run the RL_RULE_SIMILAR related functions
  packageAlice = alice.encryptFieldsForSimilarRule();
  packageBob = bob.encryptFieldsForSimilarRule();

  alice.applySecretKeyToRecords(packageBob);
  bob.applySecretKeyToRecords(packageAlice);

  alice.matchRecordsBySimilarRule(packageAlice, packageBob);
  bob.matchRecordsBySimilarRule(packageBob, packageAlice);

  // At this point we finished the iteration and ready to run the next one
  // with the next rule. We return the number of matches found after running the
  // rule.
  return alice.getNumMatchedRecords(true);
}