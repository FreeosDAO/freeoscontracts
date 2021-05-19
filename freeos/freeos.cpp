#include "freeos.hpp"
#include <cmath>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>

namespace freedao {

using namespace eosio;

const std::string VERSION = "0.345";

// ACTION
void freeos::version() {
  iteration this_iteration = get_claim_iteration();

  std::string version_message = freeos_acct + "/" + freeosconfig_acct + "/" +
                                freeostokens_acct + "/" + freedao_acct +
                                " version = " + VERSION + " - iteration " +
                                std::to_string(this_iteration.iteration_number);
  
   

  check(false, version_message);
}

// ACTION
void freeos::tick() {
  // what iteration is in the statistics table?
  statistic_index statistic_table(get_self(), get_self().value);
  auto statistic_iterator = statistic_table.begin();
  check(statistic_iterator != statistic_table.end(),
        "statistics record is not found");

  uint32_t old_iteration = statistic_iterator->iteration;
  uint32_t new_iteration = get_claim_iteration().iteration_number;
  uint32_t previous_unvest_iteration =
      statistic_iterator->unvestpercentiteration;

  if (new_iteration != old_iteration) {
    // a change in iteration has occurred

    // update iteration in statistics table
    statistic_table.modify(statistic_iterator, _self,
                           [&](auto &stat) { stat.iteration = new_iteration; });

    // update unvest percent if required
    if (new_iteration > previous_unvest_iteration) {
      update_unvest_percentage();
    }

    if (new_iteration != 0) {
      uint32_t previous_iteration = new_iteration - 1;

      // delete the expired iteration record
      action delete_action = action(permission_level{get_self(), "active"_n},
                                    name(freeosconfig_acct), "iterclear"_n,
                                    std::make_tuple(previous_iteration));

      delete_action.send();
    }

  } else {
    // no change to iteration - do some unstaking if we are in a valid iteration
    if (new_iteration > 0)
      refund_stakes();
  }
}

uint32_t freeos::get_cached_iteration() {
  statistic_index statistic_table(get_self(), get_self().value);
  auto statistic_iterator = statistic_table.begin();

  check(statistic_iterator != statistic_table.end(),
        "statistics record is not found");

  return statistic_iterator->iteration;
}

// ACTION
void freeos::cron() {
  require_auth("cron"_n);

  tick();
}

// this is only ever called by tick() when a switch to a new iteration is
// detected
void freeos::update_unvest_percentage() {
  uint32_t current_unvest_percentage;
  uint32_t new_unvest_percentage;

  // get the statistics record
  statistic_index statistic_table(get_self(), get_self().value);
  auto statistic_iterator = statistic_table.begin();
  check(statistic_iterator != statistic_table.end(),
        "statistics record is not found");

  // find the current vested proportion. If 0.0f it means that the exchange rate
  // is favourable
  float vested_proportion = get_vested_proportion();

  // Decide whether we are above target or below target price
  if (vested_proportion == 0.0f) {
    // favourable exchange rate, so implement the 'good times' strategy -
    // calculate the new unvest_percentage
    current_unvest_percentage = statistic_iterator->unvestpercent;

    // move the unvest_percentage on to next level if we have reached a new
    // 'good times' iteration
    switch (current_unvest_percentage) {
    case 0:
      new_unvest_percentage = 1;
      break;
    case 1:
      new_unvest_percentage = 2;
      break;
    case 2:
      new_unvest_percentage = 3;
      break;
    case 3:
      new_unvest_percentage = 5;
      break;
    case 5:
      new_unvest_percentage = 8;
      break;
    case 8:
      new_unvest_percentage = 13;
      break;
    case 13:
      new_unvest_percentage = 21;
      break;
    case 21:
      new_unvest_percentage = 21;
      break;
    }

    // modify the statistics table with the new percentage. Also ensure the
    // failsafe counter is set to 0.
    statistic_table.modify(statistic_iterator, _self, [&](auto &stat) {
      stat.unvestpercent = new_unvest_percentage;
      stat.unvestpercentiteration = get_cached_iteration();
      stat.failsafecounter = 0;
    });

  } else {
    // unfavourable exchange rate, so implement the 'bad times' strategy
    // calculate failsafe unvest percentage - every Xth week of unfavourable
    // rate, set unvest percentage to 15%

    // get the unvest failsafe frequency - default is 24
    uint8_t failsafe_frequency = 24;

    // read the frequency from the freeosconfig 'parameters' table
    parameters_index parameters_table(name(freeosconfig_acct),
                                      name(freeosconfig_acct).value);
    auto parameter_iterator = parameters_table.find(name("failsafefreq").value);

    if (parameter_iterator != parameters_table.end()) {
      failsafe_frequency = stoi(parameter_iterator->value);
    }

    // increment the failsafe_counter
    uint32_t failsafe_counter = statistic_iterator->failsafecounter;
    failsafe_counter++;

    // Store the new failsafecounter and unvestpercent
    statistic_table.modify(statistic_iterator, _self, [&](auto &stat) {
      stat.failsafecounter = failsafe_counter % failsafe_frequency;
      stat.unvestpercent = (failsafe_counter == failsafe_frequency ? 15 : 0);
      stat.unvestpercentiteration = get_cached_iteration();
    });
  }
}

// ACTION
void freeos::reguser(const name &user) {
  require_auth(user);

  // check that system is operational (global masterswitch parameter set to "1")
  check(check_master_switch(), msg_freeos_system_not_available);

  // get the current iteration
  iteration current_iteration = get_claim_iteration();

  // nothing is allowed when not in a valid claim period (when iteration is 0)
  check(current_iteration.iteration_number != 0,
        "freeos is not in a claim period");

  // perform the registration
  registration_status result = register_user(user);
}

// register_user is a function available to other actions. This is to enable
// auto-registration i.e. user is automatically registered whenever they stake
// or claim. return values are defined by enum registration_status. N.B. This
// function is 'silent' - errors and user notifications are handled by the
// calling actions. All prerequisities must be handled by the calling action.

registration_status freeos::register_user(const name &user) {
  // is the user already registered?
  // find the account in the user table
  users_index users_table(get_self(), user.value);
  auto user_iterator =
      users_table.find(symbol_code(SYSTEM_CURRENCY_CODE).raw());

  if (user_iterator != users_table.end()) {
    return registered_already;
  }

  // determine account type
  char account_type = get_account_type(user);

  // update the user count in the 'counters' record
  uint32_t number_of_users;

  statistic_index statistic_table(get_self(), get_self().value);
  auto statistic_iterator = statistic_table.begin();
  if (statistic_iterator == statistic_table.end()) {
    // emplace
    statistic_table.emplace(
        get_self(), [&](auto &stat) { stat.usercount = number_of_users = 1; });

  } else {
    // modify
    statistic_table.modify(statistic_iterator, _self, [&](auto &stat) {
      stat.usercount = number_of_users = stat.usercount + 1;
    });
  }

  // get the current iteration
  iteration current_iteration = get_claim_iteration();

  // examine the staking requirement for the user - if their staking requirement
  // is 0 then we will consider them to have already staked
  int64_t stake_requirement_amount = get_stake_requirement(account_type);
  asset stake_requirement =
      asset(stake_requirement_amount, symbol(SYSTEM_CURRENCY_CODE, 4));

  // register the user
  users_table.emplace(get_self(), [&](auto &user) {
    user.stake = asset(0, symbol(SYSTEM_CURRENCY_CODE, 4));
    user.account_type = account_type;
    user.registered_iteration = current_iteration.iteration_number;
    user.staked_iteration =
        stake_requirement.amount == 0 ? current_iteration.iteration_number : 0;
  });

  // add the user to the vested accounts table
  vestaccounts_index vestaccounts_table(get_self(), user.value);
  auto user_account = vestaccounts_table.find(
      symbol_code(NON_EXCHANGEABLE_CURRENCY_CODE).raw());
  if (user_account == vestaccounts_table.end()) {
    vestaccounts_table.emplace(get_self(), [&](auto &account) {
      account.balance = asset(0, symbol(NON_EXCHANGEABLE_CURRENCY_CODE, 4));
    });
  }

  return registered_success;
}

// action to allow user to reverify their account_type
// ACTION
void freeos::reverify(name user) {
  require_auth(user);

  // get the current iteration
  iteration current_iteration = get_claim_iteration();

  // nothing is allowed when not in a valid claim period (when iteration is 0)
  check(current_iteration.iteration_number != 0,
        "freeos is not in a claim period");

  // set the account type
  users_index users_table(get_self(), user.value);
  auto user_iterator = users_table.begin();

  // check if the user has a user registration record
  check(user_iterator != users_table.end(),
        "user is not registered with freeos");

  // get the account type
  char account_type = get_account_type(user);

  // examine the staking requirement for the user - if their staking requirement
  // is 0 then we will consider them to have already staked
  int64_t stake_requirement_amount = get_stake_requirement(account_type);
  asset stake_requirement =
      asset(stake_requirement_amount, symbol(SYSTEM_CURRENCY_CODE, 4));

  // set the user account type
  users_table.modify(user_iterator, _self, [&](auto &u) {
    u.account_type = account_type;

    // if user not already staked and stake requirement is 0, then consider the
    // user to have staked
    if (u.staked_iteration == 0 && stake_requirement.amount == 0) {
      u.staked_iteration = current_iteration.iteration_number;
    }
  });
}

// determine the user account type from the Proton verification table
char freeos::get_account_type(name user) {
  // default result
  char user_account_type = 'e';

  // first determine which contract we consult - if we have set an alternative
  // contract then use that one
  name verification_contract;

  parameters_index parameters_table(name(freeosconfig_acct),
                                    name(freeosconfig_acct).value);
  auto parameter_iterator = parameters_table.find(name("altverifyacc").value);

  if (parameter_iterator == parameters_table.end()) {
    verification_contract =
        name(verification_contract); // no alternative contract configured, so
                                     // use the 'production' contract (for
                                     // Proton mainnet)
  } else {
    verification_contract =
        name(parameter_iterator
                 ->value); // alternative contract is configured, so use
                           // that contract instead (for Proton testnet)
  }

  // access the verification table
  usersinfo verification_table(name(verification_contract),
                               name(verification_contract).value);
  auto verification_iterator = verification_table.find(user.value);

  if (verification_iterator != verification_table.end()) {
    // record found, so default account_type is 'd', unless we find a
    // verification
    user_account_type = 'd';

    auto kyc_prov = verification_iterator->kyc;

    for (int i = 0; i < kyc_prov.size(); i++) {
      size_t fn_pos = kyc_prov[0].kyc_level.find("firstname");
      size_t ln_pos = kyc_prov[0].kyc_level.find("lastname");

      if (verification_iterator->verified == true &&
          fn_pos != std::string::npos && ln_pos != std::string::npos) {
        user_account_type = 'v';
        break;
      }
    }
  }

  return user_account_type;
}

// stake confirmation
[[eosio::on_notify("eosio.token::transfer")]] void
freeos::stake(name user, name to, asset quantity, std::string memo) {
  if (memo == "freeos stake") {
    if (user == get_self()) {
      return;
    }

    // user-activity-driven background process
    tick();

    // check that system is operational (global masterswitch parameter set to
    // "1")
    check(check_master_switch(), msg_freeos_system_not_available);

    // which iteration are we in?
    uint32_t current_iteration = get_cached_iteration();

    check(current_iteration != 0, "Staking not allowed in iteration 0");

    //****************************************************

    // auto-register the user - if user is already registered then that is ok,
    // the register_user function responds silently
    registration_status result = register_user(user);

    check(result == registered_success || result == registered_already,
          "user registration did not succeed");

    //****************************************************

    // get the user record - the amount of the stake requirement and the amount
    // staked find the account in the user table
    users_index users_table(get_self(), user.value);
    auto user_iterator = users_table.begin();

    // check if the user is registered
    check(user_iterator != users_table.end(), "user is not registered");

    // check that user isn't already staked
    check(user_iterator->staked_iteration == 0,
          "the account is already staked");

    uint32_t stake_requirement_amount =
        get_stake_requirement(user_iterator->account_type);
    asset stake_requirement = asset(stake_requirement_amount * 10000,
                                    symbol(SYSTEM_CURRENCY_CODE, 4));
    check(stake_requirement == quantity,
          "the stake amount is not what is required");

    // update the user record
    users_table.modify(user_iterator, _self, [&](auto &usr) {
      usr.stake = quantity;
      usr.staked_iteration = current_iteration;
    });
  }
}

uint32_t freeos::get_stake_requirement(char account_type) {
  // default return value
  uint32_t stake_requirement = 0;

  // get the number of users
  statistic_index statistic_table(get_self(), get_self().value);
  auto statistic_iterator = statistic_table.begin();
  check(statistic_iterator != statistic_table.end(),
        "the statistics record is not found");
  uint32_t number_of_users = statistic_iterator->usercount;

  // look up the freeosconfig stakereqs table
  stakereq_index stakereqs_table(name(freeosconfig_acct),
                                 name(freeosconfig_acct).value);
  auto sr_iterator = stakereqs_table.upper_bound(number_of_users);
  sr_iterator--;

  check(sr_iterator != stakereqs_table.end(),
        "stake requirements cannot be determined");

  if (account_type == 'v') {
    stake_requirement = sr_iterator->requirement_v;
  } else if (account_type == 'd') {
    stake_requirement = sr_iterator->requirement_d;
  } else {
    stake_requirement = sr_iterator->requirement_e;
  }

  return stake_requirement;
}

// ACTION
void freeos::unstake(const name &user) {
  require_auth(user);

  // user-activity-driven background process
  tick();

  // check that system is operational (global masterswitch parameter set to "1")
  check(check_master_switch(), msg_freeos_system_not_available);

  uint32_t current_iteration = get_cached_iteration();
  check(current_iteration != 0, "Unstaking is not allowed in iteration 0");

  // find user record
  users_index users_table(get_self(), user.value);
  auto user_iterator =
      users_table.find(symbol_code(SYSTEM_CURRENCY_CODE).raw());

  // check if the user is registered
  check(user_iterator != users_table.end(), msg_account_not_registered);

  // check if there is already an unstake in progress
  unstakerequest_index unstakes_table(get_self(), get_self().value);
  auto unstake_iterator = unstakes_table.find(user.value);

  check(unstake_iterator == unstakes_table.end(),
        "user has already requested to unstake");
  check(user_iterator->stake.amount > 0, "user does not have a staked amount");

  request_stake_refund(user, user_iterator->stake);
}

// unstaking functions

// request stake refund - add to stake refund queue
void freeos::request_stake_refund(name user, asset amount) {
  uint32_t current_iteration = get_cached_iteration();

  // add to the unstake requests queue
  unstakerequest_index unstakes_table(get_self(), get_self().value);
  unstakes_table.emplace(get_self(), [&](auto &unstake) {
    unstake.staker = user;
    unstake.iteration = current_iteration;
    unstake.amount = amount;
  });
}

// refund stakes
void freeos::refund_stakes() {
  // read the number of unstakes to release - from the freeosconfig 'parameters'
  // table
  uint16_t number_to_release = 3; // default (safe) value if parameter not set
  parameters_index parameters_table(name(freeosconfig_acct),
                                    name(freeosconfig_acct).value);
  auto parameter_iterator = parameters_table.find(name("unstakesnum").value);

  if (parameter_iterator != parameters_table.end()) {
    number_to_release = stoi(parameter_iterator->value);
  }

  uint32_t current_iteration = get_cached_iteration();

  unstakerequest_index unstakes_table(get_self(), get_self().value);
  auto iteration_index = unstakes_table.get_index<"iteration"_n>();
  auto unstake_iterator = iteration_index.begin();

  for (uint16_t i = 0;
       i < number_to_release && unstake_iterator != iteration_index.end();
       i++) {
    if (unstake_iterator->iteration < current_iteration) {
      // process the unstake request
      refund_stake(unstake_iterator->staker, unstake_iterator->amount);
      unstake_iterator = iteration_index.erase(unstake_iterator);
    } else {
      // we've reached stakes to be released in the future
      break;
    }
  }
}

// refund a stake
void freeos::refund_stake(name user, asset amount) {
  // find user record
  users_index users_table(get_self(), user.value);
  auto user_iterator =
      users_table.find(symbol_code(SYSTEM_CURRENCY_CODE).raw());

  // transfer stake from freeos to user account using the eosio.token contract
  action transfer = action(
      permission_level{get_self(), "active"_n}, "eosio.token"_n, "allocate"_n,
      std::make_tuple(get_self(), user, amount,
                      std::string("refund of freeos stake")));

  transfer.send();

  // update the user record
  users_table.modify(user_iterator, _self, [&](auto &usr) {
    usr.stake = asset(0, symbol(SYSTEM_CURRENCY_CODE, 4));
    usr.staked_iteration = 0;
  });
}

// ACTION
void freeos::unstakecncl(const name &user) {
  require_auth(user);

  unstakerequest_index unstakes_table(get_self(), get_self().value);
  auto unstake_iterator = unstakes_table.find(user.value);

  check(unstake_iterator != unstakes_table.end(),
        "user does not have an unstake request");

  // cancel the unstake - erase the unstake record
  unstakes_table.erase(unstake_iterator);
}

bool freeos::check_master_switch() {
  parameters_index parameters_table(name(freeosconfig_acct),
                                    name(freeosconfig_acct).value);
  auto parameter_iterator = parameters_table.find("masterswitch"_n.value);

  // check if the parameter is in the table or not
  if (parameter_iterator == parameters_table.end()) {
    // the parameter is not in the table, or table not found, return false
    // because it should be accessible (failsafe)
    return false;
  } else {
    // the parameter is in the table
    if (parameter_iterator->value.compare("1") == 0) {
      return true;
    } else {
      return false;
    }
  }
}

// ACTION
void freeos::create(const name &issuer, const asset &maximum_supply) {
  require_auth(get_self());

  auto sym = maximum_supply.symbol;
  check(sym.is_valid(), "invalid symbol name");
  check(maximum_supply.is_valid(), "invalid supply");
  check(maximum_supply.amount > 0, "max-supply must be positive");

  stats statstable(get_self(), sym.code().raw());
  auto existing = statstable.find(sym.code().raw());
  check(existing == statstable.end(), "token with symbol already exists");

  statstable.emplace(get_self(), [&](auto &s) {
    s.supply.symbol = maximum_supply.symbol;
    s.conditional_supply.symbol = maximum_supply.symbol;
    s.max_supply = maximum_supply;
    s.issuer = issuer;
  });
}

void freeos::issue(const name &to, const asset &quantity, const string &memo) {
  auto sym = quantity.symbol;
  check(sym.is_valid(), "invalid symbol name");
  check(memo.size() <= 256, "memo has more than 256 bytes");

  stats statstable(get_self(), sym.code().raw());
  auto existing = statstable.find(sym.code().raw());
  check(existing != statstable.end(),
        "token with symbol does not exist, create token before issue");
  const auto &st = *existing;
  check(to == st.issuer, "tokens can only be issued to issuer account");

  check(quantity.is_valid(), "invalid quantity");
  check(quantity.amount > 0, "must issue positive quantity");

  check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
  check(quantity.amount <= st.max_supply.amount - st.supply.amount,
        "quantity exceeds available supply");

  statstable.modify(st, same_payer, [&](auto &s) { s.supply += quantity; });

  add_balance(st.issuer, quantity, st.issuer);
}

void freeos::retire(const asset &quantity, const string &memo) {
  auto sym = quantity.symbol;
  check(sym.is_valid(), "invalid symbol name");
  check(memo.size() <= 256, "memo has more than 256 bytes");

  stats statstable(get_self(), sym.code().raw());
  auto existing = statstable.find(sym.code().raw());
  check(existing != statstable.end(), "token with symbol does not exist");
  const auto &st = *existing;

  check(quantity.is_valid(), "invalid quantity");
  check(quantity.amount > 0, "must retire positive quantity");

  check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");

  statstable.modify(st, same_payer, [&](auto &s) { s.supply -= quantity; });

  sub_balance(st.issuer, quantity);
}

// Replacement for the transfer action - 'allocate' enforces a whitelist of
// those who can transfer
// ACTION
void freeos::allocate(const name &from, const name &to, const asset &quantity,
                      const string &memo) {
  // check if the 'from' account is in the transferer whitelist
  transferers_index transferers_table(name(freeosconfig_acct),
                                      name(freeosconfig_acct).value);
  auto transferer_iterator = transferers_table.find(from.value);

  check(transferer_iterator != transferers_table.end(),
        "the allocate action is protected");

  // if the 'from' user is in the transferers table then call the transfer
  // function
  transfer(from, to, quantity, memo);
}

// Replacement for the issue action - 'mint' enforces a whitelist of who can
// issue OPTIONs ACTION
void freeos::mint(const name &minter, const name &to, const asset &quantity,
                  const string &memo) {
  // check if the 'to' account is in the minter whitelist
  minters_index minters_table(name(freeosconfig_acct),
                              name(freeosconfig_acct).value);
  auto minter_iterator = minters_table.find(to.value);

  check(minter_iterator != minters_table.end(), "the mint action is protected");

  require_auth(minter);

  // if the 'to' user is in the minters table then call the issue function
  issue(to, quantity, memo);
}

// Replacement for the retire action - 'burn' enforces a whitelist of who can
// retire OPTIONs ACTION
void freeos::burn(const name &burner, const asset &quantity,
                  const string &memo) {
  // check if the 'burner' account is in the burner whitelist
  burners_index burners_table(name(freeosconfig_acct),
                              name(freeosconfig_acct).value);
  auto burner_iterator = burners_table.find(burner.value);

  check(burner_iterator != burners_table.end(), "the burn action is protected");

  require_auth(burner);

  // if the 'to' user is in the burners table then call the retire function
  retire(quantity, memo);
}

void freeos::transfer(const name &from, const name &to, const asset &quantity,
                      const string &memo) {
  check(from != to, "cannot transfer to self");
  require_auth(from);
  check(is_account(to), "to account does not exist");

  // AIRKEY tokens are non-transferable, except by the freeostokens account
  // check(quantity.symbol.code().to_string().compare("AIRKEY") != 0 || from ==
  // name(freeos_acct), "AIRKEY tokens are non-transferable");

  auto sym = quantity.symbol.code();
  stats statstable(get_self(), sym.raw());
  const auto &st = statstable.get(sym.raw());

  require_recipient(from);
  require_recipient(to);

  check(quantity.is_valid(), "invalid quantity");
  check(quantity.amount > 0, "must transfer positive quantity");
  check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
  check(memo.size() <= 256, "memo has more than 256 bytes");

  auto payer = has_auth(to) ? to : from;

  sub_balance(from, quantity);
  add_balance(to, quantity, payer);
}

// convert non-exchangeable currency for exchangeable currency
// ACTION
void freeos::convert(const name &owner, const asset &quantity) {
  require_auth(owner);

  auto sym = quantity.symbol;
  check(sym == symbol(NON_EXCHANGEABLE_CURRENCY_CODE, 4),
        "invalid symbol name");

  stats statstable(get_self(), sym.code().raw());
  auto existing = statstable.find(sym.code().raw());
  check(existing != statstable.end(), "token with symbol does not exist");
  const auto &st = *existing;

  check(quantity.is_valid(), "invalid quantity");
  check(quantity.amount > 0, "must convert positive quantity");

  statstable.modify(st, same_payer, [&](auto &s) {
    s.supply -= quantity;
    s.conditional_supply -= quantity;
  });

  // decrease owner's balance of non-exchangeable tokens
  sub_balance(owner, quantity);

  // Issue exchangeable tokens
  asset exchangeable_amount =
      asset(quantity.amount, symbol(EXCHANGEABLE_CURRENCY_CODE, 4));
  std::string memo = std::string("conversion");

  // issue an equivalent amount of exchangeable tokens to the freeos account
  action issue_action = action(
      permission_level{get_self(), "active"_n}, name(freeostokens_acct),
      "issue"_n, std::make_tuple(name(freeos_acct), exchangeable_amount, memo));

  issue_action.send();

  // transfer exchangeable tokens to the owner
  action transfer_action = action(
      permission_level{get_self(), "active"_n}, name(freeostokens_acct),
      "transfer"_n,
      std::make_tuple(name(freeos_acct), owner, exchangeable_amount, memo));

  transfer_action.send();
}

void freeos::sub_balance(const name &owner, const asset &value) {
  accounts from_acnts(get_self(), owner.value);

  const auto &from =
      from_acnts.get(value.symbol.code().raw(), "no balance object found");
  check(from.balance.amount >= value.amount, "overdrawn balance");

  from_acnts.modify(from, owner, [&](auto &a) { a.balance -= value; });
}

void freeos::add_balance(const name &owner, const asset &value,
                         const name &ram_payer) {
  accounts to_acnts(get_self(), owner.value);
  auto to = to_acnts.find(value.symbol.code().raw());
  if (to == to_acnts.end()) {
    to_acnts.emplace(ram_payer, [&](auto &a) { a.balance = value; });
  } else {
    to_acnts.modify(to, same_payer, [&](auto &a) { a.balance += value; });
  }
}

// ACTION
void freeos::claim(const name &user) {
  require_auth(user);

  // user-activity-driven background process
  tick();

  // check that system is operational (global masterswitch parameter set to "1")
  check(check_master_switch(), msg_freeos_system_not_available);

  // is this a real account?
  check(is_account(user), "user does not have an account on the network");

  // what iteration are we in?
  iteration this_iteration = get_claim_iteration();
  check(this_iteration.iteration_number != 0,
        "freeos is not in a claim period");

  // auto-register the user - if user is already registered then that is ok, the
  // register_user function responds silently
  registration_status result = register_user(user);

  // check user eligibility to claim
  check(eligible_to_claim(user, this_iteration),
        "user is not eligible to claim in this iteration");

  // update the number of claimevents
  uint32_t claim_event_count = update_claim_event_count();

  // get freedao multiplier
  uint16_t freedao_multiplier = get_freedao_multiplier(claim_event_count);

  // calculate amounts to be transferred to user and FreeDAO
  // first get the proportion that is vested
  float vested_proportion = get_vested_proportion();

  // work out the vested proportion and liquid proportion of OPTION to be
  // claimed
  uint16_t claim_tokens = this_iteration.claim_amount;
  asset claim_amount =
      asset(claim_tokens * 10000, symbol(NON_EXCHANGEABLE_CURRENCY_CODE, 4));

  uint16_t vested_tokens = claim_tokens * vested_proportion;
  asset vested_amount =
      asset(vested_tokens * 10000, symbol(NON_EXCHANGEABLE_CURRENCY_CODE, 4));

  uint16_t liquid_tokens = claim_tokens - vested_tokens;
  asset liquid_amount =
      asset(liquid_tokens * 10000, symbol(NON_EXCHANGEABLE_CURRENCY_CODE, 4));

  uint16_t freedao_tokens = claim_tokens * freedao_multiplier;
  asset freedao_amount =
      asset(freedao_tokens * 10000, symbol(NON_EXCHANGEABLE_CURRENCY_CODE, 4));

  uint16_t minted_tokens = liquid_tokens + freedao_tokens;
  asset minted_amount =
      asset(minted_tokens * 10000, symbol(NON_EXCHANGEABLE_CURRENCY_CODE, 4));

  // prepare the memo string
  std::string memo = std::string("claim by ") + user.to_string();

  // conditionally limited supply - increment the conditional_supply by total
  // amount of issue
  stats statstable(get_self(),
                   symbol_code(NON_EXCHANGEABLE_CURRENCY_CODE).raw());
  auto currency_iterator =
      statstable.find(symbol_code(NON_EXCHANGEABLE_CURRENCY_CODE).raw());

  check(currency_iterator != statstable.end(),
        "token with symbol does not exist");
  const auto &st = *currency_iterator;

  statstable.modify(st, same_payer,
                    [&](auto &s) { s.conditional_supply += minted_amount; });

  // Issue the required minted amount to the freeos account
  action mint_action = action(
      permission_level{get_self(), "active"_n}, name(freeos_acct), "mint"_n,
      std::make_tuple(get_self(), get_self(), minted_amount, memo));

  mint_action.send();

  // transfer liquid OPTION to user
  action user_transfer = action(
      permission_level{get_self(), "active"_n}, name(freeos_acct), "allocate"_n,
      std::make_tuple(get_self(), user, liquid_amount, memo));

  user_transfer.send();

  // transfer OPTION to freedao_acct
  action freedao_transfer = action(
      permission_level{get_self(), "active"_n}, name(freeos_acct), "allocate"_n,
      std::make_tuple(get_self(), name(freedao_acct), freedao_amount, memo));

  freedao_transfer.send();

  // record the deposit to the freedao account
  record_deposit(this_iteration.iteration_number, freedao_amount);

  // update the user's vested OPTION balance
  if (vested_tokens > 0) {
    vestaccounts_index to_acnts(get_self(), user.value);
    auto to = to_acnts.find(vested_amount.symbol.code().raw());
    if (to == to_acnts.end()) {
      to_acnts.emplace(get_self(), [&](auto &a) { a.balance = vested_amount; });
    } else {
      to_acnts.modify(to, _self, [&](auto &a) { a.balance += vested_amount; });
    }
  }

  // update the user's issuance stats in their registration record
  users_index users(get_self(), user.value);
  auto user_iterator = users.begin();

  if (user_iterator != users.end()) {
    users.modify(user_iterator, _self, [&](auto &u) {
      u.issuances += 1;
      u.last_issuance = this_iteration.iteration_number;
    });
  }
}

// record a deposit to the freedao account
void freeos::record_deposit(uint64_t iteration_number, asset amount) {
  deposits_index deposits_table(get_self(), get_self().value);

  // find the record for the iteration
  auto deposit_iterator = deposits_table.find(iteration_number);

  if (deposit_iterator == deposits_table.end()) {
    // insert record and initialise
    deposits_table.emplace(get_self(), [&](auto &d) {
      d.iteration = iteration_number;
      d.accrued = amount;
    });
  } else {
    // modify record
    deposits_table.modify(deposit_iterator, _self,
                          [&](auto &d) { d.accrued += amount; });
  }
}

// action to clear (remove) a deposit record from the deposit table
// ACTION
void freeos::depositclear(uint64_t iteration_number) {
  require_auth(name(freedao_acct));

  deposits_index deposits_table(get_self(), get_self().value);

  // find the record for the iteration
  auto deposit_iterator = deposits_table.find(iteration_number);

  check(deposit_iterator != deposits_table.end(),
        "a deposit record for the requested iteration does not exist");

  deposits_table.erase(deposit_iterator);
}

// ACTION
void freeos::unvest(const name &user) {
  require_auth(user);

  // user-activity-driven background process
  tick();

  // check that system is operational (global masterswitch parameter set to "1")
  check(check_master_switch(), msg_freeos_system_not_available);

  // check that the user exists
  check(is_account(user), "User does not have an account");

  // get the current iteration
  uint32_t this_iteration = get_cached_iteration();
  check(this_iteration > 0, "Not in a valid iteration");

  // calculate the amount to be unvested - get the percentage for the iteration
  statistic_index statistic_table(get_self(), get_self().value);
  auto statistic_iterator = statistic_table.begin();
  check(statistic_iterator != statistic_table.end(),
        "statistics record is not found");
  uint32_t unvest_percent = statistic_iterator->unvestpercent;

  // check that the unvest percentage is within limits
  check(unvest_percent > 0 && unvest_percent <= 100,
        "vested OPTIONs cannot be unvested in this claim period. Please try "
        "during next claim period.");

  // has the user unvested this iteration? - consult the unvest history table
  unvest_index unvest_table(get_self(), user.value);
  auto unvest_iterator = unvest_table.find(this_iteration);
  // if the unvest record exists for the iteration then the user has unvested,
  // so is not eligible to unvest again
  check(unvest_iterator == unvest_table.end(),
        "user has already unvested in this iteration");

  // do the unvesting
  // get the user's unvested OPTION balance
  asset user_vbalance = asset(0, symbol(NON_EXCHANGEABLE_CURRENCY_CODE, 4));
  vestaccounts_index vestaccounts_table(get_self(), user.value);
  auto vestaccount_iterator = vestaccounts_table.begin();

  if (vestaccount_iterator != vestaccounts_table.end()) {
    user_vbalance = vestaccount_iterator->balance;
  }

  // if user's vested balance is 0 then nothing to do, so return
  if (user_vbalance.amount == 0) {
    return;
  }

  // calculate the amount of vested OPTIONs to convert to liquid OPTIONs
  // Warning: these calculations use mixed-type arithmetic. Any changes need to
  // be thoroughly tested.

  uint64_t vested_units =
      user_vbalance.amount; // in currency units (i.e. number of 0.0001 OPTION)

  double percentage = unvest_percent / 100.0; // required to be a double

  uint64_t converted_units =
      vested_units *
      percentage; // in currency units (i.e. number of 0.0001 OPTION)

  uint32_t rounded_up_options = (uint32_t)ceil(
      converted_units /
      10000.0); // ceil rounds up to the next whole number of OPTION

  asset converted_options =
      asset(rounded_up_options * 10000,
            symbol(NON_EXCHANGEABLE_CURRENCY_CODE,
                   4)); // express the roundedupoptions as an asset

  // conditionally limited supply - increment the conditional_supply by total
  // amount of issue
  stats statstable(get_self(),
                   symbol_code(NON_EXCHANGEABLE_CURRENCY_CODE).raw());

  auto currency_iterator =
      statstable.find(symbol_code(NON_EXCHANGEABLE_CURRENCY_CODE).raw());

  check(currency_iterator != statstable.end(),
        "token with symbol does not exist");
  const auto &st = *currency_iterator;

  statstable.modify(st, same_payer, [&](auto &s) {
    s.conditional_supply += converted_options;
  });

  std::string memo = std::string("unvesting OPTIONs by ");
  memo.append(user.to_string());

  // Issue the required amount to the freeos account
  action mint_action = action(
      permission_level{get_self(), "active"_n}, name(freeos_acct), "mint"_n,
      std::make_tuple(get_self(), get_self(), converted_options, memo));

  mint_action.send();

  // transfer liquid OPTIONs to user
  action user_transfer = action(
      permission_level{get_self(), "active"_n}, name(freeos_acct), "allocate"_n,
      std::make_tuple(get_self(), user, converted_options, memo));

  user_transfer.send();

  // subtract the amount transferred from the unvested record
  vestaccounts_table.modify(vestaccount_iterator, _self,
                            [&](auto &v) { v.balance -= converted_options; });

  // write the unvest event to the unvest history table
  unvest_iterator = unvest_table.begin();
  if (unvest_iterator == unvest_table.end()) {
    unvest_table.emplace(get_self(), [&](auto &unvest) {
      unvest.iteration_number = this_iteration;
    });
  } else {
    unvest_table.modify(unvest_iterator, same_payer, [&](auto &unvest) {
      unvest.iteration_number = this_iteration;
    });
  }
}

float freeos::get_vested_proportion() {
  // default rate if exchange rate record not found, or if current price >=
  // target price (so no need to vest)
  float proportion = 0.0f;

  exchange_index exchangerate_table(name(freeosconfig_acct),
                                    name(freeosconfig_acct).value);

  // there is a single record
  auto exchangerate_iterator = exchangerate_table.begin();

  // if the exchange rate exists in the table
  if (exchangerate_iterator != exchangerate_table.end()) {
    // get current and target rates
    double currentprice = exchangerate_iterator->currentprice;
    double targetprice = exchangerate_iterator->targetprice;

    if (targetprice > 0 && currentprice < targetprice) {
      proportion = 1.0f - (currentprice / targetprice);
    }
  } else {
    // use the default proportion specified in the 'vestpercent' parameter
    parameters_index parameters_table(name(freeosconfig_acct),
                                      name(freeosconfig_acct).value);
    auto parameter_iterator = parameters_table.find(name("vestpercent").value);

    if (parameter_iterator != parameters_table.end()) {
      uint8_t int_percent = stoi(parameter_iterator->value);
      proportion = ((float)int_percent) / 100.0f;
    }
  }

  // apply a cap of 0.9
  if (proportion > 0.9f) {
    proportion = 0.9f;
  }

  return proportion;
}

// look up the required threshold for the number of users
uint64_t freeos::get_threshold(uint32_t number_of_users) {
  // look up the config stakereqs table
  stakereq_index stakereqs_table(name(freeosconfig_acct),
                                 name(freeosconfig_acct).value);
  auto stakereqs_iterator = stakereqs_table.upper_bound(number_of_users);
  stakereqs_iterator--;

  return stakereqs_iterator->threshold;
}

// return the current iteration record
iteration freeos::get_claim_iteration() {
  iteration this_iteration =
      iteration{0, time_point(), time_point(), 0,
                0}; // default null iteration value if outside of a claim period

  uint64_t now = current_time_point().time_since_epoch()._count;

  // find iteration that matches current time
  iterations_index iterations_table(name(freeosconfig_acct),
                                    name(freeosconfig_acct).value);
  auto start_index = iterations_table.get_index<"start"_n>();
  auto iteration_iterator = start_index.upper_bound(now);

  if (iteration_iterator != start_index.begin()) {
    iteration_iterator--;
  }

  // check we are within the period of the iteration
  if (iteration_iterator != start_index.end() &&
      now >= iteration_iterator->start.time_since_epoch()._count &&
      now <= iteration_iterator->end.time_since_epoch()._count) {
    this_iteration = *iteration_iterator;
  }

  return this_iteration;
}

// calculate if user is eligible to claim in this iteration
bool freeos::eligible_to_claim(const name &claimant, iteration this_iteration) {
  // get the user record - if there is no record then user is not registered
  users_index users_table(get_self(), claimant.value);
  auto user_iterator = users_table.begin();

  check(user_iterator != users_table.end(), "user is not registered in freeos");

  // has the user claimed this iteration - consult the last_issuance field in
  // the user record
  if (user_iterator->last_issuance == this_iteration.iteration_number) {
    return false;
  }

  // how many airkey tokens does the user have?
  asset user_airkey_balance =
      asset(0, symbol(AIRKEY_CURRENCY_CODE, 0)); // default = 0 AIRKEY

  accounts user_accounts(get_self(), claimant.value);
  symbol_code airkey = symbol_code(AIRKEY_CURRENCY_CODE);
  auto user_airkey_account = user_accounts.find(airkey.raw());

  if (user_airkey_account != user_accounts.end()) {
    user_airkey_balance = user_airkey_account->balance;
  }

  // Possession of an AIRKEY allows the user to bypass the staking and holding
  // requirements
  if (user_airkey_balance.amount == 0) {
    // has the user staked?
    if (user_iterator->staked_iteration == 0) {
      return false;
    }

    // check that the user has the required balance of OPTION
    int64_t liquid_option_balance_amount = 0;

    auto user_option_account =
        user_accounts.find(symbol_code(NON_EXCHANGEABLE_CURRENCY_CODE).raw());
    if (user_option_account != user_accounts.end()) {
      liquid_option_balance_amount = user_option_account->balance.amount;
    }

    // check the user's vested OPTION balance
    int64_t vested_option_balance_amount = 0;

    vestaccounts_index vestaccounts_table(get_self(), claimant.value);
    const auto &vaccount_iterator = vestaccounts_table.find(
        symbol_code(NON_EXCHANGEABLE_CURRENCY_CODE).raw());
    if (vaccount_iterator != vestaccounts_table.end()) {
      vested_option_balance_amount = vaccount_iterator->balance.amount;
    }

    // check the user's exchangeable balance
    int64_t exchangeable_balance_amount = 0;

    accounts accounts_table(name(freeostokens_acct), claimant.value);
    const auto &account_iterator =
        accounts_table.find(symbol_code(EXCHANGEABLE_CURRENCY_CODE).raw());
    if (account_iterator != accounts_table.end()) {
      exchangeable_balance_amount = account_iterator->balance.amount;
    }

    // user's total OPTION balance is liquid-OPTION plus vested-OPTION
    int64_t total_option_balance_amount = liquid_option_balance_amount +
                                          vested_option_balance_amount +
                                          exchangeable_balance_amount;

    // the 'holding' balance requirement for this iteration's claim
    int64_t iteration_holding_requirement =
        this_iteration.tokens_required * 10000;

    if (total_option_balance_amount < iteration_holding_requirement) {
      return false;
    }
  }

  // if the user passes all these checks then they are eligible
  return true;
}

uint32_t freeos::update_claim_event_count() {
  uint32_t claimevents;

  statistic_index statistic_table(get_self(), get_self().value);
  auto statistic_record = statistic_table.begin();
  check(statistic_record != statistic_table.end(),
        "statistics record is not found");

  // modify
  statistic_table.modify(statistic_record, _self, [&](auto &s) {
    s.claimevents = claimevents = s.claimevents + 1;
  });

  return claimevents;
}

uint16_t freeos::get_freedao_multiplier(uint32_t claimevents) {
#ifdef TEST_BUILD
  if (claimevents <= 5) {
    return 55;
  } else if (claimevents <= 10) {
    return 34;
  } else if (claimevents <= 20) {
    return 21;
  } else if (claimevents <= 30) {
    return 13;
  } else if (claimevents <= 50) {
    return 8;
  } else if (claimevents <= 80) {
    return 5;
  } else if (claimevents <= 130) {
    return 3;
  } else if (claimevents <= 210) {
    return 2;
  } else {
    return 1;
  }
#else
  if (claimevents <= 100) {
    return 233;
  } else if (claimevents <= 200) {
    return 144;
  } else if (claimevents <= 300) {
    return 89;
  } else if (claimevents <= 500) {
    return 55;
  } else if (claimevents <= 800) {
    return 34;
  } else if (claimevents <= 1300) {
    return 21;
  } else if (claimevents <= 2100) {
    return 13;
  } else if (claimevents <= 3400) {
    return 8;
  } else if (claimevents <= 5500) {
    return 5;
  } else if (claimevents <= 8900) {
    return 3;
  } else if (claimevents <= 14400) {
    return 2;
  } else {
    return 1;
  }
#endif
}

} // namespace freedao