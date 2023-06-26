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

#include <iostream>
#include <fstream>
#include <chrono>
#include <assert.h>

#include "helayers/hebase/hebase.h"
#include "helayers/hebase/seal/SealCkksContext.h"
#include "helayers/db/Table.h"

using namespace std;
using namespace helayers;

string compTypeToStr(ComparisonType comparisonType)
{
  switch (comparisonType) {
  case IS_EQUAL:
    return "IS_EQUAL";
  case IS_GREATER:
    return "IS_GREATER";
  case IS_SMALLER:
    return "IS_SMALLER";
  case IS_GREATER_EQUAL:
    return "IS_GREATER_EQUAL";
  case IS_SMALLER_EQUAL:
    return "IS_SMALLER_EQUAL";
  default:
    assert(false);
    return "";
  }
}

double countQuery(const Table& t,
                  const string& compareCol,
                  int compareValPlain,
                  const Field& compareField,
                  ComparisonType comparisonType)
{
  cout << "COUNT * WHERE " << compareCol << " " << compTypeToStr(comparisonType)
       << " " << compareValPlain << endl;
  HELAYERS_TIMER_PUSH("count query " + compTypeToStr(comparisonType));
  Field res = t.countQuery(compareCol, compareField, comparisonType);
  HELAYERS_TIMER_POP();
  HELAYERS_TIMER_PUSH("decrypt count query " + compTypeToStr(comparisonType));
  double count = t.postProcessCountQuery(res);
  HELAYERS_TIMER_POP();
  cout << fixed << "result: " << count << endl;

  return count;
}

double sumQuery(const Table& t,
                const string& sumCol,
                const string& compareCol,
                int compareValPlain,
                const Field& compareField,
                ComparisonType comparisonType)
{
  cout << "SUM " << sumCol << " WHERE " << compareCol << " "
       << compTypeToStr(comparisonType) << " " << compareValPlain << endl;
  HELAYERS_TIMER_PUSH("sum query " + compTypeToStr(comparisonType));
  Field res = t.sumQuery(sumCol, compareCol, compareField, comparisonType);
  HELAYERS_TIMER_POP();
  HELAYERS_TIMER_PUSH("decrypt sum query " + compTypeToStr(comparisonType));
  double sum = t.postProcessSumQuery(res);
  HELAYERS_TIMER_POP();
  cout << fixed << "result: " << sum << endl;

  return sum;
}

double averageQuery(const Table& t,
                    const string& avgCol,
                    const string& compareCol,
                    int compareValPlain,
                    const Field& compareField,
                    ComparisonType comparisonType)
{
  cout << "AVG " << avgCol << " WHERE " << compareCol << " "
       << compTypeToStr(comparisonType) << " " << compareValPlain << endl;
  HELAYERS_TIMER_PUSH("average query " + compTypeToStr(comparisonType));
  vector<Field> res =
      t.averageQuery(avgCol, compareCol, compareField, comparisonType);
  HELAYERS_TIMER_POP();
  HELAYERS_TIMER_PUSH("decrypt average query " + compTypeToStr(comparisonType));
  double avg = t.postProcessAverageQuery(res);
  HELAYERS_TIMER_POP();
  cout << fixed << "result: " << avg << endl;

  return avg;
}

double stdDevQuery(const Table& t,
                   const string& stdDevCol,
                   const string& compareCol,
                   int compareValPlain,
                   const Field& compareField,
                   ComparisonType comparisonType)
{
  cout << "STDDEV " << stdDevCol << " WHERE " << compareCol << " "
       << compTypeToStr(comparisonType) << " " << compareValPlain << endl;
  HELAYERS_TIMER_PUSH("standard deviation query " +
                      compTypeToStr(comparisonType));
  vector<Field> res = t.standardDeviationQuery(
      stdDevCol, compareCol, compareField, comparisonType);
  HELAYERS_TIMER_POP();
  HELAYERS_TIMER_PUSH("decrypt standard deviation query " +
                      compTypeToStr(comparisonType));
  double stdDev = t.postProcessStdDevQuery(res);
  HELAYERS_TIMER_POP();
  cout << fixed << "result: " << stdDev << endl;

  return stdDev;
}

int main()
{
  HeConfigRequirement req;
  req.multiplicationDepth = 9;
  req.numSlots = 16384;
  req.fractionalPartPrecision = 50;
  req.integerPartPrecision = 10;
  SealCkksContext he;
  he.init(req);
  he.printSignature(cout);
  Encoder enc(he);
  string tablePath = getDataSetsDir() + "/db/txsmillion11Bits.csv";
  ifstream ifs(tablePath);
  string opCol = "tx_sum";
  string compareCol = "client_id";
  HELAYERS_TIMER_PUSH("table encryption");
  Table t(he, ifs);
  HELAYERS_TIMER_POP();

  int compareValIsEq = 9;
  HELAYERS_TIMER_PUSH("creating compare value1");
  Field compareFieldIsEq = t.createCompareValue(compareValIsEq, compareCol);
  HELAYERS_TIMER_POP();

  int compareValIsGr = 50;
  HELAYERS_TIMER_PUSH("creating compare value2");
  Field compareFieldIsGr = t.createCompareValue(compareValIsGr, compareCol);
  HELAYERS_TIMER_POP();

  double countIsEqual =
      countQuery(t, compareCol, compareValIsEq, compareFieldIsEq, IS_EQUAL);
  double countIsGreater =
      countQuery(t, compareCol, compareValIsGr, compareFieldIsGr, IS_GREATER);

  double sumIsEqual = sumQuery(
      t, opCol, compareCol, compareValIsEq, compareFieldIsEq, IS_EQUAL);
  double sumIsGreater = sumQuery(
      t, opCol, compareCol, compareValIsGr, compareFieldIsGr, IS_GREATER);

  double avgIsEqual = averageQuery(
      t, opCol, compareCol, compareValIsEq, compareFieldIsEq, IS_EQUAL);
  double avgIsGreater = averageQuery(
      t, opCol, compareCol, compareValIsGr, compareFieldIsGr, IS_GREATER);

  double stdDevIsEqual = stdDevQuery(
      t, opCol, compareCol, compareValIsEq, compareFieldIsEq, IS_EQUAL);
  double stdDevIsGreater = stdDevQuery(
      t, opCol, compareCol, compareValIsGr, compareFieldIsGr, IS_GREATER);

  always_assert(round(countIsEqual) == 9964);
  always_assert(round(countIsGreater) == 490726);
  always_assert(round(sumIsEqual) == 246748572);
  always_assert(round(sumIsGreater) == 12276999495);
  always_assert(fabs(avgIsEqual - 24764.007627458854) < 0.01);
  always_assert(fabs(avgIsGreater - 25018.0334748923) < 0.01);
  always_assert(fabs(stdDevIsEqual - 14360.362655088153) < 0.01);
  always_assert(fabs(stdDevIsGreater - 14429.498357990333) < 0.01);

  HELAYERS_TIMER_PRINT_MEASURE_SUMMARY("table encryption");
  HELAYERS_TIMER_PRINT_MEASURE_SUMMARY("count query IS_EQUAL");
  HELAYERS_TIMER_PRINT_MEASURE_SUMMARY("count query IS_GREATER");
  HELAYERS_TIMER_PRINT_MEASURE_SUMMARY("sum query IS_EQUAL");
  HELAYERS_TIMER_PRINT_MEASURE_SUMMARY("sum query IS_GREATER");
  HELAYERS_TIMER_PRINT_MEASURE_SUMMARY("average query IS_EQUAL");
  HELAYERS_TIMER_PRINT_MEASURE_SUMMARY("average query IS_GREATER");
  HELAYERS_TIMER_PRINT_MEASURE_SUMMARY("standard deviation query IS_EQUAL");
  HELAYERS_TIMER_PRINT_MEASURE_SUMMARY("standard deviation query IS_GREATER");
}