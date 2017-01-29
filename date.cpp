#include "StdAfx.h"
#include "date.h"
#include <sstream>
#include <iomanip>
#include <climits>

using namespace std;

// Year for which tm::tm_year is zero:
const int TM_START_YEAR = 1900;

// Seconds to milliseconds quotient
const int TIME_MULTIPLIER = 1000;

const int MONTH_COUNT = 12;
const int SECS_IN_MINUTE = 60;
const int SECS_IN_HOUR = SECS_IN_MINUTE * 60;
const int SECS_IN_DAY = SECS_IN_HOUR * 24;
const long long MILLISECS_IN_DAY = SECS_IN_DAY * TIME_MULTIPLIER;

// Displacement of time_t ticks related to DateTime::m_time
const long long TIME_T_ZERO = 62167132800000;

const int MONTH_LENGTHS[]      =     {31, 28, 31, 30,   31,  30,  31,  31,  30,  31,  30,  31};
const int MONTH_LENGTHS_LEAP[] =     {31, 29, 31, 30,   31,  30,  31,  31,  30,  31,  30,  31};

const int MONTH_STARTS[]       = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};

// Spare day of the leap year counted in the leap-year-days
// So it's compensated after February but lacks in Jan & Feb
const int MONTH_STARTS_LEAP[]  = {-1, 30, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};

/** Get difference between local time and UTC time in milliseconds
 */
long long getTimezone(void) {
  static bool inited(false);
  static long long tz(0);

  if (!inited) {
    // Point is converting current time_t to UTC in struct tm, then back to localtime
    time_t t = time(nullptr);
    tm utc;
    if (gmtime_s(&utc, &t) == 0) {
      // Unknown state of daylight saving (supposing the state is not
      // changing while the application works):
      utc.tm_isdst = -1;

      time_t tu = mktime(&utc);
      tz = (tu - t) * TIME_MULTIPLIER;
    }
    inited = true;
  }

  return tz;
}

/** Check if the year is leap
 */
inline bool isLeap (int year) {
  // Gregorian
  return (year % 4 == 0)
    && ((year % 100 != 0) || (year % 400 == 0));
}

/** Count leap days in the years
 */
inline int getLeapDays(int years) {
  // Gregorian
  return years / 4 - years / 100 + years / 400;
}

/** Quick and dirty replacement for struct tm with
 *  additional functionality
 */
class STime {
  int year;
  int month;  // 0 - 11
  int day;    // 1 - xx
  int hour;
  int minute;
  int second;
  int millisecond;
  bool valid;

  // Extract month and day from given monthTable (MONTH_STARTS or MONTH_STARTS_LEAP)
  void setYearsDay(const int *monthTable, const int *lengths, int yearDay);

public:
  STime();
  STime(long long time);
  STime(const tm *time);
  STime(const string &value);
  
  // Get amount of milliseconds from J1n, 1 of the 1'th year
  long long get(void);

  // Returns day-of-year for the date
  int dayOfYear(void) const;

  // Add months to the date-time
  void incMonth(int months) { month += months; }

  // Add years to the date-time
  void incYear(int years) { year += years; }

  // Get date formatted as yyyy-MM-dd
  string formatDate(void) const;

  // Get date and time formatted as yyyy-MM-dd hh:mm:ss
  string formatDateTime(void) const;

  // Get amount of months since a previous date
  // (this date-time MUST be before or equal to the "from" one)
  int monthsAfter(const STime &from);
};

STime::STime():
  year(0),
  month(0),
  day(0),
  hour(0),
  minute(0),
  second(0),
  millisecond(0),
  valid(true)
{}

STime::STime(const tm *time)
{
  if (time) {
    year = time->tm_year + TM_START_YEAR;
    month = time->tm_mon;
    day = time->tm_mday;
    hour = time->tm_hour;
    minute = time->tm_min;
    second = time->tm_sec;
    millisecond = 0;

    valid = year >= TM_START_YEAR
      && day > 0
      && day <= MONTH_LENGTHS[month]
      && hour >= 0
      && minute >= 0
      && second >= 0;
  } else {
    memset(this, 0, sizeof(STime));
    valid = false;
  }
}

STime::STime(const string &value) {
  // Read the fields from a string
  memset (this, 0, sizeof(STime));
  char dummy;
  istringstream str(value);

  str >> year;
  valid = !str.fail();

  str >> dummy >> month;
  valid &= !str.fail();

  str >> dummy >> day;
  valid &= !str.fail();

  if (!str.eof()) {
    // The string includes time portion
    if (!(str >> hour)
      || !(str >> dummy >> minute)
      || !(str >> dummy >> second)) 
        valid = false;

    str >> dummy >> millisecond;
  }

  // Month starts from zero:
  month--;
}

STime::STime (long long time): valid(true) {
  millisecond = time % TIME_MULTIPLIER;
  // Milliseconds ingnored
  time /= TIME_MULTIPLIER;

  // Seconds since the day start
  second = time % SECS_IN_DAY;

  hour = second / SECS_IN_HOUR;
  second %= SECS_IN_HOUR; 

  minute = second / SECS_IN_MINUTE;
  second %= SECS_IN_MINUTE;
  // ... now everything within the day is correct

  // Amount of days since 1.01.01
  time /= SECS_IN_DAY;

  // Maximum possible years count for this amount of days:
  year = time / 365;
  year -= getLeapDays(year) / 365;

  // The year changed, leap days count may be changed also:
  int leap = getLeapDays(year);

  // Day of the year for these year and leap days count:
  long long days = time - (long long)year * 365 - leap;
  
  // Adjusting the year to achieve days being in [0; 365]
  // Down:
  while (days < 0) {
    year--;
    leap = getLeapDays(year);
    days = time - (long long)year * 365 - leap;
  }

  // Up:
  while (days > 365) {
    year++;
    leap = getLeapDays(year);
    days = time - (long long)year * 365 - leap;
  }

  // Split day-of-the year onto month and day
  if (isLeap(year))
    setYearsDay(MONTH_STARTS_LEAP, MONTH_LENGTHS_LEAP, days);
  else
    setYearsDay(MONTH_STARTS, MONTH_LENGTHS, days);
}

void STime::setYearsDay(const int *monthTable, const int *lengths, int yearDay) {
  for (int i = 1; i < 13; i++) {
    if (yearDay <= monthTable[i]) {
      month = i-1;
      day = yearDay - monthTable[i-1] + 1;
      break;
    }
  }

  // Lame date correction for the cases like Jan, 32
  if (day > lengths[month]) {
    day = 1;
    month++;
    if (month > 11) {
      year++;
      month = 0;
    }
  }
}

long long STime::get(void) {
  if (!valid) return LLONG_MIN;

  // By chance, month may be more than 11...
  year += month / MONTH_COUNT;
  month %= MONTH_COUNT;

  int mdays;
  if (isLeap(year))
    mdays = MONTH_STARTS_LEAP[month];
  else
    mdays = MONTH_STARTS[month];

  // Days amount from Jan, 1 of the 1'st year
  long long result = year * 365 // in non-leap years
    + getLeapDays(year)         // days in leap years
    + mdays                     // the day on which the month starts
    + day                       // month's day
    - 1;                        // since month starts with day 1 (not 0)

  result *= SECS_IN_DAY;
  result += hour * SECS_IN_HOUR;
  result += minute * SECS_IN_MINUTE;
  result += second;
  result *= TIME_MULTIPLIER;
  result += millisecond;

  return result;
}

string STime::formatDate(void) const {
  ostringstream res;
  res << setfill('0') << year
    << '-' << setw(2) << (month + 1)
    << '-' << setw(2) << day;
  return res.str();
}

string STime::formatDateTime(void) const {
  ostringstream res;
  res << setfill('0') << year
    << '-' << setw(2) << (month + 1)
    << '-' << setw(2) << day
    << ' ' << setw(2) << hour
    << ':' << setw(2) << minute
    << ':' << setw(2) << second;

  if (millisecond) res << '.' << setw(3) << millisecond;

  return res.str();
}

int STime::dayOfYear(void) const {
  if (isLeap(year))
    return MONTH_STARTS_LEAP[month] + day;
  else
    return MONTH_STARTS[month] + day;
}

int STime::monthsAfter(const STime &from) {
  // * * * > *
  // * * * * *
  // * < * * *
  int result = (year - from.year) * MONTH_COUNT;
  result -= from.month - month;

  if (day < from.day)
    return result - 1;

  if (day == from.day) {
    int time = (hour * SECS_IN_HOUR + minute * SECS_IN_MINUTE + second) * TIME_MULTIPLIER + millisecond;
    int ftime = (from.hour * SECS_IN_HOUR + from.minute * SECS_IN_MINUTE + from.second) * TIME_MULTIPLIER + from.millisecond;
    if (ftime > time)
      return result - 1;
  }

  return result;
}

//******************************
DateTime::DateTime () {
  // Invalid date-time by default
  m_time = LLONG_MIN;
}

DateTime::DateTime (const DateTime &value):
  m_time(value.m_time)
{}

DateTime::DateTime (const std::string &value) {
  set(value);
}

DateTime::DateTime (const time_t &time) {
  set(time);
}

DateTime::DateTime (const tm *time) {
  set(time);
}

void DateTime::setNow(void) {
  set(time(nullptr));
}

bool DateTime::isValid(void) const {
  return m_time != LLONG_MIN;
}

long long DateTime::getRaw(void) const {
  return m_time;
}

void DateTime::toUTC(void) {
  if (m_time != LLONG_MIN)
    m_time += getTimezone();
}

void DateTime::fromUTC(void) {
  if (m_time != LLONG_MIN)
    m_time -= getTimezone();
}

std::string DateTime::formatDate(void) const {
  if (m_time == LLONG_MIN) return string();
  STime time(m_time);
  return time.formatDate();
}

std::string DateTime::formatDateTime(void) const {
  if (m_time == LLONG_MIN) return string();
  STime time(m_time);
  return time.formatDateTime();
}

time_t DateTime::asUnixTime(void) const {
  if (m_time == LLONG_MIN) return -1;
  time_t result = (m_time - TIME_T_ZERO) / TIME_MULTIPLIER;
  if (result < 0) return -1;
  return result;
}

void DateTime::set (const std::string &value) {
  STime time(value);
  m_time = time.get();
}

void DateTime::set (const time_t &time) {
  if (time > 0) {
    m_time = (long long) time * TIME_MULTIPLIER + TIME_T_ZERO;
  } else {
    m_time = LLONG_MIN;
  }
}

void DateTime::set (const tm *time) {
  STime t(time);
  m_time = t.get();
}

DateTime& DateTime::incSecond(int seconds) {
  if (m_time != LLONG_MIN)
    m_time += (long long) seconds * TIME_MULTIPLIER;
  return *this;
}

DateTime& DateTime::incMinute(int minutes) {
  if (m_time != LLONG_MIN)
    m_time += (long long) minutes * SECS_IN_MINUTE * TIME_MULTIPLIER;
  return *this;
}

DateTime& DateTime::incHour(int hours) {
  if (m_time != LLONG_MIN)
    m_time += (long long) hours * SECS_IN_HOUR * TIME_MULTIPLIER;
  return *this;
}

DateTime& DateTime::incDay(int days) {
  if (m_time != LLONG_MIN)
    m_time += (long long) days * MILLISECS_IN_DAY;
  return *this;
}

DateTime& DateTime::incMonth(int months) {
  if (m_time != LLONG_MIN) {
    STime time(m_time);
    time.incMonth(months);
    m_time = time.get();
  }
  return *this;
}

DateTime& DateTime::incYear(int years) {
  if (m_time != LLONG_MIN) {
    STime time(m_time);
    time.incYear(years);
    m_time = time.get();
  }
  return *this;
}

int DateTime::getWeekDay(void) const {
  if (m_time != LLONG_MIN)
    return (m_time / MILLISECS_IN_DAY + 6) % 7;
  else
    return -1;
}

int DateTime::getDayOfYear(void) const {
  if (m_time == LLONG_MIN) return -1;
  STime time(m_time);
  return time.dayOfYear();
}

int DateTime::daysBetween(const DateTime &date1, const DateTime &date2) {
  if (date1.m_time == LLONG_MIN || date2.m_time == LLONG_MIN) return -1;
  long long diff = date1.m_time - date2.m_time;
  if (diff < 0) diff = -diff;
  return diff / MILLISECS_IN_DAY;
}

int DateTime::monthsBetween(const DateTime &date1, const DateTime &date2) {
  if (date1.m_time == LLONG_MIN || date2.m_time == LLONG_MIN) return -1;
  if (date1 == date2) return 0;

  STime d1(date1.m_time);
  STime d2(date2.m_time);

  if (date1 < date2)
    return d2.monthsAfter(d1);
  else
    return d1.monthsAfter(d2);
}

int DateTime::yearsBetween(const DateTime &date1, const DateTime &date2) {
  int months = monthsBetween(date1, date2);
  if (months <= 0)
    return months;
  else
    return months / MONTH_COUNT;
}

const DateTime& DateTime::operator = (const DateTime &date) {
  m_time = date.m_time;
  return *this;
}

bool DateTime::hasTime(void) const {
	return m_time != LLONG_MIN
		&& (m_time % MILLISECS_IN_DAY) != 0;
}
