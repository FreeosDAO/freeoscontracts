#ifndef __USERTABLE_H_INCLUDED
#define __USERTABLE_H_INCLUDED

#include <eosio/singleton.hpp>


using namespace eosio;

// the registered user table
struct [[eosio::table]] user {
  asset stake;                    // how many EOS tokens staked
  char account_type;              // user has EOS-login account, Dapp Account-login or other
  asset stake_requirement;        // the number of tokens the user is required to stake
  time_point_sec registered_time; // when the user was registered
  time_point_sec staked_time;     // when the user staked their tokens (staked tokens have a time lock)

  uint64_t primary_key() const {return stake.symbol.code().raw();}
};

using user_index = eosio::multi_index<"users"_n, user>;

// the record counter table
struct [[eosio::table]] count {
  uint32_t  count;
} ct;

using user_singleton = eosio::singleton<"usercount"_n, count>;

#endif
