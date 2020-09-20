#include <eosio/eosio.hpp>
#include "freeosconfig.hpp"


using namespace eosio;

[[eosio::action]]
void freeosconfig::paramupsert(
        name virtualtable,
        name paramname,
        std::string value
        ) {

    require_auth(_self);
    parameter_index parameters(get_self(), get_first_receiver().value);
    auto iterator = parameters.find(paramname.value);

    // check if the parameter is in the table or not
    if (iterator == parameters.end() ) {
        // the parameter is not in the table, so insert
        parameters.emplace(_self, [&](auto & row) {  // first argument was "freeosconfig"_n
           row.virtualtable = virtualtable;
           row.paramname = paramname;
           row.value = value;
        });

    } else {
        // the parameter is in the table, so update
        parameters.modify(iterator, _self, [&](auto& row) {   // second argument was "freeosconfig"_n
          row.virtualtable = virtualtable;
          row.paramname = paramname;
          row.value = value;
        });
    }
}

// erase parameter from the table
[[eosio::action]]
void freeosconfig::paramerase ( name paramname ) {
    require_auth(_self);

    parameter_index parameters(get_self(), get_first_receiver().value);
    auto iterator = parameters.find(paramname.value);

    // check if the parameter is in the table or not
    check(iterator != parameters.end(), "config parameter does not exist");

    // the parameter is in the table, so delete
    parameters.erase(iterator);
}


// stake requirements table actions

[[eosio::action]]
void freeosconfig::stakeupsert(
        uint64_t threshold,
        uint32_t  value_e,
        uint32_t  value_d,
        uint32_t  value_v,
        uint32_t  value_x
        ) {

    require_auth(_self);
    stakereq_index stakereqs(get_self(), get_first_receiver().value);
    auto iterator = stakereqs.find(threshold);

    // check if the threshold is in the table or not
    if (iterator == stakereqs.end() ) {
        // the threshold is not in the table, so insert
        stakereqs.emplace(_self, [&](auto & row) {  // first argument was "freeosconfig"_n
           row.threshold = threshold;
           row.requirement_e = value_e;
           row.requirement_d = value_d;
           row.requirement_v = value_v;
           row.requirement_x = value_x;
        });

    } else {
        // the threshold is in the table, so update
        stakereqs.modify(iterator, _self, [&](auto& row) {   // second argument was "freeosconfig"_n
        row.threshold = threshold;
        row.requirement_e = value_e;
        row.requirement_d = value_d;
        row.requirement_v = value_v;
        row.requirement_x = value_x;
        });
    }
}

// erase stake requirement from the table
[[eosio::action]]
void freeosconfig::stakeerase (uint64_t threshold) {
    require_auth(_self);

    stakereq_index stakereqs(get_self(), get_first_receiver().value);
    auto iterator = stakereqs.find(threshold);

    // check if the parameter is in the table or not
    check(iterator != stakereqs.end(), "stake requirement record does not exist");

    // the parameter is in the table, so delete
    stakereqs.erase(iterator);
}


// upsert a week into the weeks table
[[eosio::action]]
void freeosconfig::weekupsert(uint64_t week_number, std::string start, std::string end) {

  require_auth(_self);

  // parse the week start and end strings
  uint32_t nstart = parsetime(start);
  if (nstart == 0) {
    print("start date ", start, " cannot be parsed; must be in YYYY-MM-DD HH:MM:SS format");
    return;
  }

  uint32_t nend = parsetime(end);
  if (nend == 0) {
    print("end date ", end, " cannot be parsed; must be in YYYY-MM-DD HH:MM:SS format");
    return;
  }

  week_index freeosweeks(get_self(), get_self().value);
  auto iterator = freeosweeks.find(week_number);

  // check if the week is in the table or not
  if (iterator == freeosweeks.end() ) {
      // the week is not in the table, so insert
      freeosweeks.emplace(_self, [&](auto & row) {
         row.week_number = week_number;
         row.start = nstart;
         row.start_date = start;
         row.end = nend;
         row.end_date = end;
      });

      print("week: ", week_number, " start: ", start, ", end: ", end, " added to the weeks table");

  } else {
      // the week is in the table, so update
      freeosweeks.modify(iterator, _self, [&](auto& row) {
        row.week_number = week_number;
        row.start = nstart;
        row.start_date = start;
        row.end = nend;
        row.end_date = end;
      });

      print("week: ", week_number, " start: ", start, ", end: ", end, " modified in the weeks table");
  }
}

// erase a week record from the weeks table
[[eosio::action]]
void freeosconfig::weekerase(uint64_t week_number) {

  week_index freeosweeks(get_self(), get_self().value);
  auto iterator = freeosweeks.find(week_number);

  // check if the parameter is in the table or not
  check(iterator != freeosweeks.end(), "week record does not exist in the weeks table");

  // the parameter is in the table, so delete
  freeosweeks.erase(iterator);
}

// for testing - get a week from the weeks table
[[eosio::action]]
void freeosconfig::getweek(uint64_t week_number) {

  week_index freeosweeks(get_self(), get_self().value);
  auto iterator = freeosweeks.find(week_number);

  // check if the week is in the table or not
  if (iterator == freeosweeks.end() ) {
    print("a record for week ", week_number, " does not exist in the weeks table" );
  } else {
    print("week ", week_number, " start: ", iterator->start_date, " (", iterator->start, "), end: ", iterator->end_date, " (", iterator->end, ")");
  }
}



// Required for testing
// Reads the tables and prints out a config parameter value and a stake requirements value
[[eosio::action]]
void freeosconfig::getconfig(uint64_t threshold, name paramname) {
  // stakereqs
  stakereq_index stakereqs(get_self(), get_first_receiver().value);
  auto iterator = stakereqs.find(threshold);

  if (iterator == stakereqs.end()) {
    print("threshold does not exist - ");
  } else {
    const auto& s = *iterator;
    print(s.threshold, ": ", s.requirement_e, " ", s.requirement_d, " ", s.requirement_v, " ", s.requirement_x, " ");
  }

  // config
  parameter_index parameters(get_self(), get_first_receiver().value);
  auto it = parameters.find(paramname.value);

  // check if the parameter is in the table or not
  if (it == parameters.end()) {
    print("config parameter does not exist");
  } else {
    const auto& c = *it;
    print(paramname, ": ", c.value);
  }

}

// Required for testing
[[eosio::action]]
void freeosconfig::getthreshold(uint64_t numusers, std::string account_type) {
  int required_stake;

  stakereq_index stakereqs(get_self(), get_first_receiver().value);
  auto iterator = stakereqs.end();

  // find which band to apply
  do {
    iterator--;
    if (numusers >= iterator->threshold) break;
  } while (iterator  != stakereqs.begin());

  // which value to look up depends on the type of account
  switch (account_type[0]) {
    case 'e':
      required_stake = iterator->requirement_e;
      break;
    case 'd':
      required_stake = iterator->requirement_d;
      break;
    case 'v':
      required_stake = iterator->requirement_v;
      break;
    default:
      required_stake = 9999;
      break;
  }

  print("In band ", iterator->threshold, ", required_stake: ", required_stake);

}

// helper functions

uint32_t freeosconfig::parsetime(const std::string sdatetime) {

      // create a character array for the date string
      char date[sdatetime.length() + 1];
      strcpy(date, sdatetime.c_str());

      // define a tm structure to contain the parsed string values
      // tm *ltm = new tm;
      tm ltm;
      char delims[] = " ,.-:/";

      char* pchar;
      pchar = strtok(date, delims);
      ltm.tm_year = atoi(pchar);                 // year
      ltm.tm_mon = atoi(strtok(NULL, delims));   // month
      ltm.tm_mday = atoi(strtok(NULL, delims));  // day
      ltm.tm_hour = atoi(strtok(NULL, delims));  // hour
      ltm.tm_min = atoi(strtok(NULL, delims));   // minute
      ltm.tm_sec = atoi(strtok(NULL, delims));   // seconds

      if (ltm.tm_year < 2020) return 0;
      if (ltm.tm_mon < 1 || ltm.tm_mon > 12) return 0;
      if (ltm.tm_mday < 1 || ltm.tm_mday > 31) return 0;
      if (ltm.tm_hour < 0 || ltm.tm_hour > 23) return 0;
      if (ltm.tm_min < 0 || ltm.tm_min > 59) return 0;
      if (ltm.tm_sec < 0 || ltm.tm_sec > 59) return 0;

      uint32_t epoch_seconds = GetTimeStamp(ltm.tm_year, ltm.tm_mon, ltm.tm_mday, ltm.tm_hour, ltm.tm_min, ltm.tm_sec);

      // print various components of tm structure.
      // print("Date: ", ltm.tm_year, "-", ltm.tm_mon, "-", ltm.tm_mday, " ", ltm.tm_hour, ":", ltm.tm_min, ":", ltm.tm_sec, " UTC: ", epoch_seconds);

      return epoch_seconds;

}

bool IsLeapYear(int year)
{
    if ((year % 4) != 0)
        return false;

    if ((year % 100) == 0)
        return ((year % 400) == 0);

    return true;
}

uint32_t DateToSeconds(int year, int month, int day)
{
  int DaysToMonth365[] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
  int DaysToMonth366[] = { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 };

    if (((year >= 1) && (year <= 9999)) && ((month >= 1) && (month <= 12)))
    {
        int *daysToMonth = IsLeapYear(year) ? DaysToMonth366 : DaysToMonth365;
        if ((day >= 1) && (day <= (daysToMonth[month] - daysToMonth[month - 1])))
        {
            int previousYear = year - 1;
            int daysInPreviousYears = ((((previousYear * 365) + (previousYear / 4)) - (previousYear / 100)) + (previousYear / 400));

            int totalDays = ((daysInPreviousYears + daysToMonth[month - 1]) + day) - 1;
            return (totalDays * 86400);
        }
    }
    return 0;
}


uint32_t TimeToSeconds(int hour, int minute, int second)
{
    long totalSeconds = ((hour * 3600) + (minute * 60)) + second;

    return (totalSeconds);
}


uint32_t freeosconfig::GetTimeStamp(int year, int month, int day,
                        int hour, int minute, int second)
{
    const uint32_t start_of_epoch = 2006054656;   // seconds elapsed to 1st Jan 1970

    uint32_t timestamp = DateToSeconds(year, month, day)
        + TimeToSeconds(hour, minute, second);

    return timestamp - start_of_epoch;
}
