#include <eosio/eosio.hpp>
#include "freeosconfig.hpp"
#include "../paramtable.hpp"

using namespace eosio;

[[eosio::action]]
void freeosconfig::upsert(
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
void freeosconfig::erase ( name paramname ) {
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

// Required for testing
// Reads the tables and prints out a config parameter value and a stake requirements value
[[eosio::action]]
void freeosconfig::configcheck(uint64_t threshold, name paramname) {
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
