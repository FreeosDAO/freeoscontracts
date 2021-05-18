#pragma once

#include "eosio.proton.hpp"
#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <string>

using namespace eosio;

#define _STRINGIZE(x) #x
#define STRINGIZE(x) _STRINGIZE(x)

std::string freeos_acct = STRINGIZE(FREEOS);
std::string freeosconfig_acct = STRINGIZE(FREEOSCONFIG);
std::string freeostokens_acct = STRINGIZE(FREEOSTOKENS);
std::string freedao_acct = STRINGIZE(DIVIDEND);

const std::string verification_contract =
    "eosio.proton"; // contains the usersinfo table

// currency symbol for network
const std::string SYSTEM_CURRENCY_CODE = "XPR";
const std::string NON_EXCHANGEABLE_CURRENCY_CODE = "OPTION";
const std::string EXCHANGEABLE_CURRENCY_CODE = "FREEOS";
const std::string AIRKEY_CURRENCY_CODE = "AIRKEY";

// hard floor for the target exchange rate - it can never go below this
const double HARD_EXCHANGE_RATE_FLOOR = 0.0167;

// common error/notification messages
const std::string msg_freeos_system_not_available =
    "Freeos system is not currently operating. Please try later";
const std::string msg_account_not_registered =
    "Account is not registered with freeos";

namespace freedao {

// Table definitions

// freeos contract
// liquid OPTION ledger
struct[[ eosio::table("accounts"), eosio::contract("freeos") ]] account {
  asset balance;

  uint64_t primary_key() const { return balance.symbol.code().raw(); }
};
typedef eosio::multi_index<"accounts"_n, account> accounts;

// vested OPTION ledger
struct[
    [ eosio::table("vestaccounts"), eosio::contract("freeos") ]] vestaccount {
  asset balance;

  uint64_t primary_key() const { return balance.symbol.code().raw(); }
};
typedef eosio::multi_index<"vestaccounts"_n, vestaccount> vestaccounts_index;

// currency stats
struct[[ eosio::table("stat"), eosio::contract("freeos") ]] currency_stats {
  asset supply;
  asset max_supply;
  asset conditional_supply;
  name issuer;

  uint64_t primary_key() const { return supply.symbol.code().raw(); }
};
typedef eosio::multi_index<"stat"_n, currency_stats> stats;

// the registered user table
struct[[ eosio::table("users"), eosio::contract("freeos") ]] user {
  asset stake;                   // how many XPR tokens staked
  char account_type;             // user's verification level
  uint32_t registered_iteration; // when the user was registered
  uint32_t
      staked_iteration;   // the iteration in which the user staked their tokens
  uint32_t votes;         // how many votes the user has made
  uint32_t issuances;     // total number of times the user has been issued with
                          // OPTIONs
  uint32_t last_issuance; // the last iteration in which the user was issued
                          // with OPTIONs

  uint64_t primary_key() const { return stake.symbol.code().raw(); }
};
using users_index = eosio::multi_index<"users"_n, user>;

// new statistics table - to replace counters
struct[[ eosio::table("statistics"), eosio::contract("freeos") ]] statistic {
  uint32_t usercount;
  uint32_t claimevents;
  uint32_t unvestpercent;
  uint32_t unvestpercentiteration;
  uint32_t iteration;
  uint32_t failsafecounter;

  uint64_t primary_key() const {
    return 0;
  } // return a constant (0 in this case) to ensure a single-row table
};
using statistic_index = eosio::multi_index<"statistics"_n, statistic>;

// unvest history table - scoped on user account name
struct[[ eosio::table("unvests"), eosio::contract("freeos") ]] unvestevent {
  uint64_t iteration_number;

  uint64_t primary_key() const { return iteration_number; }
};
using unvest_index = eosio::multi_index<"unvests"_n, unvestevent>;

// freedao deposits table
struct[[ eosio::table("deposits"), eosio::contract("freeos") ]] deposit {
  uint64_t iteration;
  asset accrued;

  uint64_t primary_key() const { return iteration; }
};
using deposits_index = eosio::multi_index<"deposits"_n, deposit>;

// unstake requests queue
struct[
    [ eosio::table("unstakereqs"), eosio::contract("freeos") ]] unstakerequest {
  name staker;
  uint32_t iteration;
  asset amount;

  uint64_t primary_key() const { return staker.value; }
  uint64_t get_secondary() const { return iteration; }
};
using unstakerequest_index = eosio::multi_index<
    "unstakereqs"_n, unstakerequest,
    indexed_by<"iteration"_n, const_mem_fun<unstakerequest, uint64_t,
                                            &unstakerequest::get_secondary>>>;

// freeosconfig contract
// CONFIG stake requirements table - code: freeosconfig, scope: freeosconfig
struct[[
  eosio::table("stakereqs"), eosio::contract("freeosconfig")
]] stakerequire {
  uint64_t threshold;
  uint32_t requirement_a;
  uint32_t requirement_b;
  uint32_t requirement_c;
  uint32_t requirement_d;
  uint32_t requirement_e;
  uint32_t requirement_u;
  uint32_t requirement_v;
  uint32_t requirement_w;
  uint32_t requirement_x;
  uint32_t requirement_y;

  uint64_t primary_key() const { return threshold; }
};
using stakereq_index = eosio::multi_index<"stakereqs"_n, stakerequire>;

// exchangerate table
struct[
    [ eosio::table("exchangerate"), eosio::contract("freeosconfig") ]] price {
  double currentprice;
  double targetprice;

  uint64_t primary_key() const {
    return 0;
  } // return a constant (0 in this case) to ensure a single-row table
};
using exchange_index = eosio::multi_index<"exchangerate"_n, price>;

// iteration calendar table
struct[
    [ eosio::table("iterations"), eosio::contract("freeosconfig") ]] iteration {
  uint32_t iteration_number;
  time_point start;
  time_point end;
  uint16_t claim_amount;
  uint16_t tokens_required;

  uint64_t primary_key() const { return iteration_number; }
  uint64_t get_secondary() const { return start.time_since_epoch()._count; }
};
using iterations_index = eosio::multi_index<
    "iterations"_n, iteration,
    indexed_by<"start"_n,
               const_mem_fun<iteration, uint64_t, &iteration::get_secondary>>>;

// transferers table - a whitelist of who can call the transfer function
struct[[
  eosio::table("transferers"),
  eosio::contract("freeosconfig")
]] transfer_whitelist {
  name account;

  uint64_t primary_key() const { return account.value; }
};
using transferers_index =
    eosio::multi_index<"transferers"_n, transfer_whitelist>;

// minters table - a whitelist of who can call the issue function
struct[[
  eosio::table("minters"), eosio::contract("freeosconfig")
]] minter_whitelist {
  name account;

  uint64_t primary_key() const { return account.value; }
};
using minters_index = eosio::multi_index<"minters"_n, minter_whitelist>;

// burners table - a whitelist of who can call the retire function
struct[[
  eosio::table("burners"), eosio::contract("freeosconfig")
]] burner_whitelist {
  name account;

  uint64_t primary_key() const { return account.value; }
};
using burners_index = eosio::multi_index<"burners"_n, burner_whitelist>;

// miscellaneous parameters table
struct[
    [ eosio::table("parameters"), eosio::contract("freeosconfig") ]] parameter {
  name virtualtable;
  name paramname;
  std::string value;

  uint64_t primary_key() const { return paramname.value; }
  uint64_t get_secondary() const { return virtualtable.value; }
};
using parameters_index = eosio::multi_index<
    "parameters"_n, parameter,
    indexed_by<"virtualtable"_n,
               const_mem_fun<parameter, uint64_t, &parameter::get_secondary>>>;

#ifdef TEST_BUILD
// Verification table - a mockup of the verification table on Proton - so that
// we can test how to determine a user's account_type Taken from
// https://github.com/ProtonProtocol/proton.contracts/blob/master/contracts/eosio.proton/include/eosio.proton/eosio.proton.hpp
struct[
    [ eosio::table("usersinfo"), eosio::contract("freeosconfig") ]] userinfo {
  name acc;
  std::string name;
  std::string avatar;
  bool verified;
  uint64_t date;
  uint64_t verifiedon;
  eosio::name verifier;

  std::vector<eosio::name> raccs;
  std::vector<std::tuple<eosio::name, eosio::name>> aacts;
  std::vector<std::tuple<eosio::name, std::string>> ac;

  std::vector<kyc_prov> kyc;

  uint64_t primary_key() const { return acc.value; }
};
typedef eosio::multi_index<"usersinfo"_n, userinfo> usersinfo;
#endif

} // namespace freedao
