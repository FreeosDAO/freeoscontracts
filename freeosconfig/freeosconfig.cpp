#include <eosio/eosio.hpp>
#include "../common/freeoscommon.hpp"
#include "freeosconfig.hpp"


using namespace eosio;

// versions
// 100 - refactored the stakereqs table to have 10 columns a-e and u-y
// 101 - weeks table renamed to iterations - with iteration_number as primary key
// 102 - added the usersinfo verification table, with actions for upserting and erasing records

const std::string VERSION = "0.102";

[[eosio::action]]
void freeosconfig::version() {
  print("Version = ", VERSION);
}

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


[[eosio::action]]
void freeosconfig::currentrate(double price) {

    require_auth(_self);

    exchange_index rate(get_self(), get_self().value);
    auto iterator = rate.begin();

    // check if the rate exists in the table
    if (iterator == rate.end() ) {
        // the rate is not in the table, so insert
        rate.emplace(_self, [&](auto & row) {  // first argument was "freeosconfig"_n
           row.currentprice = price;
        });

    } else {
        // the rate is in the table, so update
        rate.modify(iterator, _self, [&](auto& row) {   // second argument was "freeosconfig"_n
          row.currentprice = price;
        });
    }
}

[[eosio::action]]
void freeosconfig::targetrate(double price) {

    require_auth(_self);

    exchange_index rate(get_self(), get_self().value);
    auto iterator = rate.begin();

    // check if the rate exists in the table
    if (iterator == rate.end() ) {
        // the rate is not in the table, so insert
        rate.emplace(_self, [&](auto & row) {  // first argument was "freeosconfig"_n
           row.targetprice = price;
        });

    } else {
        // the rate is in the table, so update
        rate.modify(iterator, _self, [&](auto& row) {   // second argument was "freeosconfig"_n
          row.targetprice = price;
        });
    }
}

// erase rate from the table
[[eosio::action]]
void freeosconfig::rateerase () {
    require_auth(_self);

    exchange_index rate(get_self(), get_self().value);
    auto iterator = rate.begin();

    // check if the rate is in the table
    check(iterator != rate.end(), "rate record does not exist");

    // the rate record is in the table, so delete
    rate.erase(iterator);
}


// stake requirements table actions

[[eosio::action]]
void freeosconfig::stakeupsert(
        uint64_t threshold,
        uint32_t  value_a,
        uint32_t  value_b,
        uint32_t  value_c,
        uint32_t  value_d,
        uint32_t  value_e,
        uint32_t  value_u,
        uint32_t  value_v,
        uint32_t  value_w,
        uint32_t  value_x,
        uint32_t  value_y
        ) {

    require_auth(_self);
    stakereq_index stakereqs(get_self(), get_first_receiver().value);
    auto iterator = stakereqs.find(threshold);

    // check if the threshold is in the table or not
    if (iterator == stakereqs.end() ) {
        // the threshold is not in the table, so insert
        stakereqs.emplace(_self, [&](auto & row) {
           row.threshold = threshold;
           row.requirement_a = value_a;
           row.requirement_b = value_b;
           row.requirement_c = value_c;
           row.requirement_d = value_d;
           row.requirement_e = value_e;
           row.requirement_u = value_u;
           row.requirement_v = value_v;
           row.requirement_w = value_w;
           row.requirement_x = value_x;
           row.requirement_y = value_y;
        });

    } else {
        // the threshold is in the table, so update
        stakereqs.modify(iterator, _self, [&](auto& row) {   // second argument was "freeosconfig"_n
        row.threshold = threshold;
        row.requirement_a = value_a;
        row.requirement_b = value_b;
        row.requirement_c = value_c;
        row.requirement_d = value_d;
        row.requirement_e = value_e;
        row.requirement_u = value_u;
        row.requirement_v = value_v;
        row.requirement_w = value_w;
        row.requirement_x = value_x;
        row.requirement_y = value_y;
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


// upsert an iteration into the iterations table
[[eosio::action]]
void freeosconfig::iterupsert(uint64_t iteration_number, std::string start, std::string end, uint16_t claim_amount, uint16_t tokens_required) {

  require_auth(_self);

  // parse the iteration start and end strings
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

  iteration_index iterations(get_self(), get_self().value);
  auto iterator = iterations.find(iteration_number);

  // check if the iteration is in the table or not
  if (iterator == iterations.end() ) {
      // the iteration is not in the table, so insert
      iterations.emplace(_self, [&](auto & row) {
         row.iteration_number = iteration_number;
         row.start = nstart;
         row.start_date = start;
         row.end = nend;
         row.end_date = end;
         row.claim_amount = claim_amount;
         row.tokens_required = tokens_required;
      });

      print("iteration: ", iteration_number, " start: ", start, ", end: ", end, ", claim amount: ", claim_amount, ", tokens_required: ", tokens_required, " added to the iterations table");

  } else {
      // the iteration is in the table, so update
      iterations.modify(iterator, _self, [&](auto& row) {
        row.iteration_number = iteration_number;
        row.start = nstart;
        row.start_date = start;
        row.end = nend;
        row.end_date = end;
        row.claim_amount = claim_amount;
        row.tokens_required = tokens_required;
      });

      print("iteration: ", iteration_number, " start: ", start, ", end: ", end, ", claim amount: ", claim_amount, ", tokens_required: ", tokens_required, ", modified in the iterations table");
  }
}

// erase an iteration record from the iterations table
[[eosio::action]]
void freeosconfig::itererase(uint64_t iteration_number) {
  require_auth(_self);

  iteration_index iterations(get_self(), get_self().value);
  auto iterator = iterations.find(iteration_number);

  // check if the parameter is in the table or not
  check(iterator != iterations.end(), "iteration record does not exist in the iterations table");

  // the iteration is in the table, so delete
  iterations.erase(iterator);

  print("record for iteration ", iteration_number, " deleted from iterations table");
}

// for testing - get an iteration from the iterations table
[[eosio::action]]
void freeosconfig::getiter(uint64_t iteration_number) {

  iteration_index iterations(get_self(), get_self().value);
  auto iterator = iterations.find(iteration_number);

  // check if the iteration is in the table or not
  if (iterator == iterations.end() ) {
    print("a record for iteration ", iteration_number, " does not exist in the iterations table" );
  } else {
    print("iteration ", iteration_number, " start: ", iterator->start_date, " (", iterator->start, "), end: ", iterator->end_date,
    " (", iterator->end, "), claim amount: ", iterator->claim_amount, ", tokens required: ", iterator->tokens_required);
  }
}



// Required for testing
// Prints out a stake requirements value record
[[eosio::action]]
void freeosconfig::getstakes(uint64_t threshold) {
  // stakereqs
  stakereq_index stakereqs(get_self(), get_first_receiver().value);
  auto iterator = stakereqs.find(threshold);

  if (iterator == stakereqs.end()) {
    print("threshold does not exist");
  } else {
    const auto& s = *iterator;
    //print(s.threshold, ": ", s.requirement_e, " ", s.requirement_d, " ", s.requirement_v, " ", s.requirement_x, " ");
    print(s.threshold, ": a=", s.requirement_a, " b=", s.requirement_b, " c=", s.requirement_c, " d=", s.requirement_d, " e=", s.requirement_e,
                        " u=", s.requirement_u, " v=", s.requirement_v, " w=", s.requirement_w, " x=", s.requirement_x, " y=", s.requirement_y);
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

    case 'a':
      required_stake = iterator->requirement_a;
      break;
    case 'b':
      required_stake = iterator->requirement_b;
      break;
    case 'c':
      required_stake = iterator->requirement_c;
      break;
    case 'd':
      required_stake = iterator->requirement_d;
      break;
    case 'e':
      required_stake = iterator->requirement_e;
      break;
    case 'u':
      required_stake = iterator->requirement_u;
      break;
    case 'v':
      required_stake = iterator->requirement_v;
      break;
    case 'w':
      required_stake = iterator->requirement_w;
      break;
    case 'x':
      required_stake = iterator->requirement_x;
      break;
    case 'y':
      required_stake = iterator->requirement_y;
      break;
    default:
      required_stake = 9999;
      break;
  }

  print("In band ", iterator->threshold, ", required_stake: ", required_stake);

}


// ************************************************************************************
// ************* eosio.proton actions for populating usersinfo table ******************
// ************************************************************************************

void freeosconfig::userverify(name acc, name verifier, bool  verified) {

		check( is_account( acc ), "Account does not exist.");

		usersinfo usrinf( get_self(), get_self().value );
		auto existing = usrinf.find( acc.value );

		if ( existing != usrinf.end() ) {
			check (existing->verified != verified, "This status alredy set");
			usrinf.modify( existing, get_self(), [&]( auto& p ){
				p.verified = verified;
				if ( verified ) {
					p.verifiedon = eosio::current_time_point().sec_since_epoch();
					p.verifier = verifier;
				} else  {
					p.verifiedon = 0;
					p.verifier = ""_n;
				}
				p.date = eosio::current_time_point().sec_since_epoch();
			});
		} else {
			usrinf.emplace( get_self(), [&]( auto& p ){
				p.acc = acc;
				p.name = "";
				p.avatar = "";
				p.verified = verified;

				if ( verified ) {
					p.verifiedon = eosio::current_time_point().sec_since_epoch();
					p.verifier = verifier;
				} else  {
					p.verifiedon = 0;
					p.verifier = ""_n;
				}
				p.date = eosio::current_time_point().sec_since_epoch();
			});
		}

}  // end of userverify

void freeosconfig::addkyc( name acc, name kyc_provider, std::string kyc_level, uint64_t kyc_date) {

    kyc_prov kyc;
    kyc.kyc_provider = kyc_provider;
    kyc.kyc_level = kyc_level;
    kyc.kyc_date = kyc_date;

    usersinfo usrinf( get_self(), get_self().value );
		auto itr_usrs = usrinf.require_find( acc.value, string("User " + acc.to_string() + " not found").c_str() );

		for (auto i = 0; i < itr_usrs->kyc.size(); i++) {
			if ( itr_usrs->kyc[i].kyc_provider == kyc.kyc_provider) {
				check (false, string("There is already approval from " + kyc.kyc_provider.to_string()).c_str());
				break;
			}
		}

		usrinf.modify( itr_usrs, get_self(), [&]( auto& p ){
			p.kyc.push_back(kyc);
		});
	}

// ************************************************************************************
// ************************************************************************************
// ************************************************************************************

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
