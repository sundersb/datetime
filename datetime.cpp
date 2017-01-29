// datetime.cpp
//  Test unit for DateTime class

#include "stdafx.h"
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <fstream>
#include "date.h"

using namespace std;

const string SECOND_DATE("1726-05-01 21:03:15.040");
const string TEST_DATE  ("2100-04-01 21:03:15.050");

const int YEAR_FIRST = 1900;
const int YEAR_LAST  = 2000;
const int DAYS_COUNT = 365 * 200;
const string DATE_DIFF_FILE("dates.txt");


void testSingle(void) {
  cout << endl << "Test single day:" << endl;
  DateTime time(TEST_DATE);
  cout << TEST_DATE << endl;
  cout << time.formatDateTime() << endl;

  time.setNow();
  cout << "It's " << time.formatDateTime() << " now (UTC)" << endl;
}


bool checkDate(int year, int month) {
  ostringstream s;
  s << setfill('0') << year << "-" << setw(2) << month << "-29 21:12:15";
  string sample = s.str();

  DateTime time(sample);
  if (sample != time.formatDateTime()) {
    cout << sample << " != " << time.formatDateTime() << endl;
    return false;
  }
  return true;
}


void testMonts(void) {
  // Test increasing month from YEAR_FIRST till YEAR_LAST
  cout << endl << "Test months from " << YEAR_FIRST << " till " << YEAR_LAST << ':' << endl;
  
  bool result = true;
  for (int year = YEAR_FIRST; year <= YEAR_LAST; year++) {
    for (int month = 1; month < 13; month++)
      result &= checkDate(year, month);
    if (!result) break;
  }
  if (result) cout << "All dates match!" << endl;
}


void testDays(void) {
  // Increase dates by one day since TEST_DATE for DAYS_COUNT

  // The output file can be later analized in Excel to check that any
  //  later date is one day older than the former one.
  // But Excel considers 1900 a leap year which is wrong

  cout << endl << "Test " << DAYS_COUNT << " days since " << YEAR_FIRST << ':' << endl;
  // Save formatted dates to file
  ofstream stream(DATE_DIFF_FILE);
  if (!stream.good()) return;

  DateTime t(YEAR_FIRST);
  for (int i = 0; i < DAYS_COUNT; i++) {
    stream << t.formatDate() << endl;
    t.incDay(1);
  }

  cout << "Dates saved to " << DATE_DIFF_FILE << endl;
}


void testTimezone(void) {
  cout << endl << "Test timezone:" << endl;
  DateTime now;
  now.setNow();
  cout << left << setw(12) << "UTC: " << now.formatDateTime() << endl;
  now.fromUTC();
  cout << setw(12) << "Local: " << now.formatDateTime() << endl;
  now.toUTC();
  cout << setw(12) << "UTC back: " << now.formatDateTime() << endl;
}


void testDifference(void) {
  cout << endl << "Test days difference:" << endl;
  DateTime date1(TEST_DATE);
  DateTime date2(SECOND_DATE);

  cout << "Compare " << TEST_DATE << " and " << SECOND_DATE << endl;
  cout << "Months: " << DateTime::monthsBetween(date1, date2) << endl;
  cout << "Years: " << DateTime::yearsBetween(date1, date2) << endl;
}


void testUnixTime(void) {
  cout << endl << "Test UNIX time:" << endl;
  time_t t = time(nullptr);

  DateTime time;
  time.set(t);

  if (t == time.asUnixTime()) {
    cout << "Time matches!" << endl;
  } else {
    cout << "Time mismatch: " << time.asUnixTime() << " instead of " << t << endl;
  }
}


int _tmain(int argc, _TCHAR* argv[])
{
  testTimezone();
  testSingle();
  testUnixTime();
  testDifference();
  testMonts();
  testDays();

  cin.get();
	return 0;
}

