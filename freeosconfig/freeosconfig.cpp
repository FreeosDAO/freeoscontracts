//#include <eosio/eosio.hpp>
#include "../common/freeoscommon.hpp"
#include "freeosconfig.hpp"


using namespace eosio;

// versions
// 100 - refactored the stakereqs table to have 10 columns a-e and u-y
// 101 - weeks table renamed to iterations - with iteration_number as primary key
// 102 - added the usersinfo verification table, with actions for upserting and erasing records
// 103 - implemented a hardcoded floor for the target exchange rate (floor)
// 104 - All error checking performed by check function
//     - Replaced all incorrect references to get_first_receiver() with get_self()
//     - Added secondary index to the iterations table
// 105 - additeration action added - creates a new iteration following on from the last for x number of hours duration
// 106 - additeration action changed to remove the price parameter
// 107 - added checks when setting current and target exchange rates that values must be positive
// 108 - added transferers table and actions transfadd and transferase for maintaining the table
// 109 - iterupsert changed to accept uint32_t iteration number
//       new iterclear action - called by freeos contract to delete expired iteration record


const std::string VERSION = "0.109";

#ifdef TEST_BUILD
[[eosio::action]]
void freeosconfig::version() {
  std::string version_message = freeos_acct + "/" + freeosconfig_acct + "/" + freeostokens_acct + "/" + freedao_acct + " version = " + VERSION;

  check(false, version_message);
}
#endif

[[eosio::action]]
void freeosconfig::paramupsert(
        name virtualtable,
        name paramname,
        std::string value
        ) {

    require_auth(_self);
    parameter_index parameters(get_self(), get_self().value);
    auto iterator = parameters.find(paramname.value);

    // check if the parameter is in the table or not
    if (iterator == parameters.end() ) {
        // the parameter is not in the table, so insert
        parameters.emplace(_self, [&](auto & row) {
           row.virtualtable = virtualtable;
           row.paramname = paramname;
           row.value = value;
        });

    } else {
        // the parameter is in the table, so update
        parameters.modify(iterator, _self, [&](auto& row) {
          row.virtualtable = virtualtable;
          row.value = value;
        });
    }
}

// erase parameter from the table
[[eosio::action]]
void freeosconfig::paramerase ( name paramname ) {
    require_auth(_self);

    parameter_index parameters(get_self(), get_self().value);
    auto iterator = parameters.find(paramname.value);

    // check if the parameter is in the table or not
    check(iterator != parameters.end(), "config parameter does not exist");

    // the parameter is in the table, so delete
    parameters.erase(iterator);
}


[[eosio::action]]
void freeosconfig::currentrate(double price) {

    require_auth(_self);

    check(price > 0.0, "current rate must be positive");

    exchange_index rate(get_self(), get_self().value);
    auto iterator = rate.begin();

    // check if the rate exists in the table
    if (iterator == rate.end() ) {
        // the rate is not in the table, so insert
        rate.emplace(_self, [&](auto & row) {
           row.currentprice = price;
        });

    } else {
        // the rate is in the table, so update
        rate.modify(iterator, _self, [&](auto& row) {
          row.currentprice = price;
        });
    }

#ifdef TEST_BUILD
    print("Current exchange rate set to ", price);
#endif

}

[[eosio::action]]
void freeosconfig::targetrate(double exchangerate) {

    require_auth(_self);

    check(exchangerate > 0.0, "target rate must be positive");

    double new_exchangerate = exchangerate;

    // ensure it is not set below the hardcoded floor
    if (new_exchangerate < HARD_EXCHANGE_RATE_FLOOR) {
      new_exchangerate = HARD_EXCHANGE_RATE_FLOOR;
    }

    exchange_index rate(get_self(), get_self().value);
    auto iterator = rate.begin();

    // check if the rate exists in the table
    if (iterator == rate.end() ) {
        // the rate is not in the table, so insert
        rate.emplace(_self, [&](auto & row) {
           row.targetprice = new_exchangerate;
        });

    } else {
        // the rate is in the table, so update
        rate.modify(iterator, _self, [&](auto& row) {
          row.targetprice = new_exchangerate;
        });
    }

#ifdef TEST_BUILD
    print("Exchange rate floor set to ", new_exchangerate);
#endif

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
    stakereq_index stakereqs(get_self(), get_self().value);
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

    stakereq_index stakereqs(get_self(), get_self().value);
    auto iterator = stakereqs.find(threshold);

    // check if the parameter is in the table
    check(iterator != stakereqs.end(), "stake requirement record does not exist");

    // the parameter is in the table, so delete
    stakereqs.erase(iterator);
}


// add an account to the whitelist transferers table
[[eosio::action]]
void freeosconfig::transfadd(name account) {
  require_auth(_self);

  transferer_index transferers_table(get_self(), get_self().value);
  transferers_table.emplace(_self, [&](auto & t) {
    t.account = account;
  });
}

// erase an account from the transferers table
void freeosconfig::transferase(name account) {
  require_auth(_self);

  transferer_index transferers_table(get_self(), get_self().value);
  auto iterator = transferers_table.find(account.value);

    // check if the account is in the table
    check(iterator != transferers_table.end(), "account is not in the transferers table");

    // the account is in the table, so delete
    transferers_table.erase(iterator);
}


// upsert an iteration into the iterations table
[[eosio::action]]
void freeosconfig::iterupsert(uint32_t iteration_number, time_point start, time_point end, uint16_t claim_amount, uint16_t tokens_required) {

  require_auth(_self);

  check(iteration_number != 0, "iteration number must not be 0");

  iteration_index iterations(get_self(), get_self().value);
  auto iterator = iterations.find(iteration_number);

  // check if the iteration is in the table or not
  if (iterator == iterations.end() ) {
      // the iteration is not in the table, so insert
      iterations.emplace(_self, [&](auto & row) {
         row.iteration_number = iteration_number;
         row.start = start;
         row.end = end;
         row.claim_amount = claim_amount;
         row.tokens_required = tokens_required;
      });
  } else {
      // the iteration is in the table, so update
      iterations.modify(iterator, _self, [&](auto& row) {
        row.iteration_number = iteration_number;
        row.start = start;
        row.end = end;
        row.claim_amount = claim_amount;
        row.tokens_required = tokens_required;
      });
  }
}

// erase an iteration record from the iterations table - contract action
[[eosio::action]]
void freeosconfig::itererase(uint32_t iteration_number) {
  require_auth(_self);

  iter_delete(iteration_number);
}

// erase an iteration record from the iterations table - called by freeos contract
[[eosio::action]]
void freeosconfig::iterclear(uint32_t iteration_number) {
  require_auth(name(freeos_acct));

  iter_delete(iteration_number);
}

// function to delete an iteration record
void freeosconfig::iter_delete(uint32_t iteration_number) {
  iteration_index iterations(get_self(), get_self().value);
  auto iterator = iterations.find(iteration_number);

  if (iterator != iterations.end()) {
    iterations.erase(iterator);
  }
}


#ifdef TEST_BUILD
// Required for testing
// Prints out a stake requirements value record
[[eosio::action]]
void freeosconfig::getstakes(uint64_t threshold) {
  // stakereqs
  stakereq_index stakereqs(get_self(), get_self().value);
  auto iterator = stakereqs.find(threshold);

  if (iterator == stakereqs.end()) {
    print("threshold does not exist");
  } else {
    const auto& s = *iterator;
    print(s.threshold, ": a=", s.requirement_a, " b=", s.requirement_b, " c=", s.requirement_c, " d=", s.requirement_d, " e=", s.requirement_e,
                        " u=", s.requirement_u, " v=", s.requirement_v, " w=", s.requirement_w, " x=", s.requirement_x, " y=", s.requirement_y);
  }

}
#endif


#ifdef TEST_BUILD
// Required for testing
[[eosio::action]]
void freeosconfig::getthreshold(uint64_t numusers, std::string account_type) {
  int required_stake;

  stakereq_index stakereqs(get_self(), get_self().value);
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
#endif


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
