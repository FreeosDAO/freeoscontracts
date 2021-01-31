#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include "../common/freeoscommon.hpp"
#include "freeos.hpp"

using namespace eosio;

// versions
// 301 - with vestaccounts migration to userext_table
// 302 - changed schema of usercount table
// 303 - with scheduled actions and functions
// 304 - with scheduled actions and functions compared against 'earliest time the process could have been run'
// 305 - a stake transfer must contain 'freeos stake' in the memo field
// 306 - fixed stake function to work when memo == "freeos stake"
// 307 - enabled the schedulelog parameter in parameters table to switch schedule logging on or off
//     - added user-driver ticks to stake, unstake, claim and transfer actions

const std::string VERSION = "0.307 XPR";

[[eosio::action]]
void freeos::version() {
  print("Version = ", VERSION);
}


[[eosio::action]]
void freeos::tick(std::string trigger) {

  // flags to indicate if a time period has elapsed - default values
  bool  hour_elapsed = false;
  bool  day_elapsed = false;
  bool  week_elapsed = false;

  // current time
  uint32_t now = current_time_point().sec_since_epoch();

  // work out if any time periods have elapsed
  tickers ticks(get_self(), get_self().value);
  auto iterator = ticks.begin();

  // check if the ticks record exists in the table (there is only one record)
  if (iterator == ticks.end() ) {
      // no record in the table, so insert initialised values
      ticks.emplace(_self, [&](auto & row) {
         row.tickly = now;
         row.hourly = 0;
         row.daily  = 0;
         row.weekly = 0;
      });

      return; // do nothing in this 'initialisation' tick

  } else {
      uint32_t previous_hourly = iterator->hourly;
      uint32_t previous_daily  = iterator->daily;
      uint32_t previous_weekly = iterator->weekly;

      // default new values if a period has not been elapsed
      uint32_t new_hourly = previous_hourly;
      uint32_t new_daily  = previous_daily;
      uint32_t new_weekly = previous_weekly;

      // Have we crossed an hourly boundary?
      if (now >= (previous_hourly + HOUR_SECONDS)) {
        new_hourly = now - (now % HOUR_SECONDS);  // the earliest time the new hour could have started
        hour_elapsed = true;
      }

      // Have we crossed a daily boundary?
      if (now >= (previous_daily + DAY_SECONDS)) {
        new_daily = now - (now % DAY_SECONDS);    // the earliest time the new day could have started
        day_elapsed = true;
      }

      // Have we crossed a weekly boundary?
      if (now >= (previous_weekly + WEEK_SECONDS)) {
        new_weekly = now - (now % WEEK_SECONDS);      // the earliest time the new week could have started
        week_elapsed = true;
      }

      // the record is in the table, so update
      ticks.modify(iterator, _self, [&](auto& row) {
          row.tickly = now;
          row.hourly = new_hourly;
          row.daily  = new_daily;
          row.weekly = new_weekly;
      });

      // run the tick process
      tick_process(trigger);

      // run the hourly process
      if (hour_elapsed == true) {
        hourly_process(trigger);
      }

      // run the daily process
      if (day_elapsed == true) {
        daily_process(trigger);
      }

      // run the weekly process
      if (week_elapsed == true) {
        weekly_process(trigger);
      }

    }

}


[[eosio::action]]
void freeos::cron() {
  require_auth("cron"_n);

  tick("P");  // "A" = runscheduled action, "U" = User driven, "P" = Proton CRON, "S" = Server CRON
}


[[eosio::action]]
void freeos::clearlog() {
  // clear schedule log
  schedulelog_index log(get_self(), get_self().value);

  auto iterator = log.begin();

  while (iterator != log.end()) {
    auto next_iterator = iterator;
    next_iterator++;
    log.erase(iterator);
    iterator = next_iterator;
  }

  // clear ticker table
  tickers ticks(get_self(), get_self().value);

  auto itick = ticks.begin();

  while (itick != ticks.end()) {
    auto next_itick = itick;
    next_itick++;
    ticks.erase(itick);
    itick = next_itick;
  }

}


// These actions allow the scheduled processes to be invoked explicitly by the freeosticker account

// run scheduled process action
void freeos::runscheduled(std::string process_specifier, bool schedule_override) {

  require_auth(permission_level("freeosticker"_n, "active"_n));

  // validate the parameters - prepare error message get_first
  std::string validation_error_msg = "valid process specifiers are '" + HOURLY + "', '" + DAILY + "', '" + WEEKLY + "'";
  check(process_specifier == HOURLY || process_specifier == DAILY || process_specifier == WEEKLY, validation_error_msg);

  // get the current time
  uint32_t now = current_time_point().sec_since_epoch();

  // work out if any time periods have elapsed
  tickers ticks(get_self(), get_self().value);
  auto iterator = ticks.begin();

  // check if the ticks record exists in the table (there is only one record)
  // if the record doesn't exist then create/initialise it and do nothing
  if (iterator == ticks.end() ) {
      // no record in the table, so insert initialised values
      ticks.emplace(_self, [&](auto & row) {
         row.tickly = now;
         row.hourly = 0;
         row.daily  = 0;
         row.weekly = 0;
      });

  print("schedule timers have been initialised, process is not yet scheduled to run");
  return;
  }

  // get previous run times
  uint32_t previous_hourly = iterator->hourly;
  uint32_t previous_daily  = iterator->daily;
  uint32_t previous_weekly = iterator->weekly;

  // check whether the scheduled process is eligible to run
  bool process_ran = false;

  // Have we crossed an hourly boundary?
  if (process_specifier == HOURLY) {
    if (schedule_override == true || now >= (previous_hourly + HOUR_SECONDS)) {
      // record the run
      ticks.modify(iterator, _self, [&](auto& row) {
          row.hourly = now - (now % HOUR_SECONDS);    // the earliest time the new hour could have started
      });

      // run the process
      hourly_process("A");  // "A" = runscheduled action, "U" = User driven, "P" = Proton CRON, "S" = Server CRON

      process_ran = true;
      print("hourly process ran");
    }
  }


  // Have we crossed a daily boundary?
  if (process_specifier == DAILY) {
    if (schedule_override == true || now >= (previous_daily + DAY_SECONDS)) {
      // record the run
      ticks.modify(iterator, _self, [&](auto& row) {
          row.daily  = now - (now % DAY_SECONDS);    // the earliest time the new day could have started
      });

      // run the process
      daily_process("A");   // "A" = runscheduled action, "U" = User driven, "P" = Proton CRON, "S" = Server CRON

      process_ran = true;
      print("daily process ran");
    }
  }


  // Have we crossed a weekly boundary?
  if (process_specifier == WEEKLY) {
    if (schedule_override == true || now >= (previous_weekly + WEEK_SECONDS)) {
      // record the run
      ticks.modify(iterator, _self, [&](auto& row) {
          row.weekly = now - (now % WEEK_SECONDS);    // the earliest time the new week could have started
      });

      // run the process
      weekly_process("A");  // "A" = runscheduled action, "U" = User driven, "P" = Proton CRON, "S" = Server CRON

      process_ran = true;
      print("weekly process ran");
    }
  }

  if (process_ran == false) {
    print(process_specifier, " process has not run (did not override schedule)");
  }

}


// process to run every tick
void freeos::tick_process(std::string trigger) {

}


// process to run every hour
void freeos::hourly_process(std::string trigger) {
  if (checkschedulelogging()) {
    // log the run
    schedulelog_index log(get_self(), get_self().value);
    log.emplace(_self, [&](auto & row) {
       row.task = "H";
       row.trigger = trigger;
       row.time = current_time_point().sec_since_epoch() + 10000000000; // 64 bit number added to provide primary-key uniqueness
    });
  }

  // do whatever...

}

// process to run every day
void freeos::daily_process(std::string trigger) {
  if (checkschedulelogging()) {
    // log the run
    schedulelog_index log(get_self(), get_self().value);
    log.emplace(_self, [&](auto & row) {
       row.task = "D";
       row.trigger = trigger;
       row.time = current_time_point().sec_since_epoch() + 20000000000; // 64 bit number added to provide primary-key uniqueness
    });
  }

  // do whatever...

}

// process to run every week
void freeos::weekly_process(std::string trigger) {
  if (checkschedulelogging()) {
    // log the run
    schedulelog_index log(get_self(), get_self().value);
    log.emplace(_self, [&](auto & row) {
       row.task = "W";
       row.trigger = trigger;
       row.time = current_time_point().sec_since_epoch() + 30000000000; // 64 bit number added to provide primary-key uniqueness
    });
  }

  // do whatever...

}


void freeos::payalan() {
  // ??? THIS IS TEST CODE - TO BE REMOVED BEFORE DEPLOYMENT

  asset one_tick = asset(1, symbol("FREEOS",4));

  action transfer = action(
    permission_level{get_self(),"active"_n},
    name(freeos_acct),
    "transfer"_n,
    std::make_tuple(get_self(), "alanappleton"_n, one_tick, std::string("tick test transfer"))
  );

  transfer.send();
}


[[eosio::action]]
void freeos::reguser(const name& user, const std::string account_type) {
  require_auth( user );

  // check that system is operational (global masterswitch parameter set to "1")
  check(checkmasterswitch(), msg_freeos_system_not_available);

  // check that the account type is correct
  if (account_type.length() != 1) {
    print("account type must be 1 character");
    return;
  }

  if (account_type[0] != 'e' && account_type[0] != 'd' && account_type[0] != 'v') {
    print("account type specified incorrectly");
    return;
  }

  // is this a real account?
  if (!is_account(user)) {
    std::string account_error_msg = std::string("account ") + user.to_string() + " is not a valid account";
    // display the error message
    print(account_error_msg);
    return;
  }

  // perform the registration
  registration_status result = register_user(user, account_type);

  // give feedback to user
  if (result == registered_already) {
    print("user is already registered");
  } else if (result == registered_success) {
    // get the user record to display the stake requirement
    user_index usertable( get_self(), user.value );
    auto u = usertable.find( symbol_code(CURRENCY_SYMBOL_CODE).raw() );

    // prepare the success message
    std::string account_success_msg = user.to_string() + std::string(" successfully registered. Stake requirement is ") + u->stake_requirement.to_string();

    // display the success message including the stake requirement
    print(account_success_msg);
  }

  return;
}

// register_user is a function available to other actions. This is to enable auto-registration i.e. user is automatically registered whenever they stake or claim.
// return values are defined by enum registration_status.
// N.B. This function is 'silent' - errors and user notifications are handled by the calling actions.
// All prerequisities must be handled by the calling action.

registration_status freeos::register_user(const name& user, const std::string account_type) {

  // is the user already registered?
  // find the account in the user table
  user_index usertable( get_self(), user.value );
  auto u = usertable.find( symbol_code(CURRENCY_SYMBOL_CODE).raw() );

  if( u != usertable.end() ) {
    return registered_already;
  }


  // calculate the user number when registered
  user_singleton user_counter(get_self(), get_self().value);
  if (!user_counter.exists()) {
    user_counter.get_or_create(get_self(), ct);
  }
  auto entry = user_counter.get();
  entry.usercount += 1;   // new user number

  // get the required stake for the user number
  uint32_t stake = getthreshold(entry.usercount, account_type);
  asset stake_requirement = asset(stake * 10000, symbol(CURRENCY_SYMBOL_CODE,4));

  // register the user
  usertable.emplace( get_self(), [&]( auto& u ) {
    u.stake = asset(0, symbol(CURRENCY_SYMBOL_CODE, 4));
    u.account_type = account_type.at(0);
    u.stake_requirement = stake_requirement;
    u.registered_time = time_point_sec(current_time_point().sec_since_epoch());
    u.staked_time = time_point_sec(0);
    });

   // add the user to the user extension table
   userext_index usr_ext_table( get_self(), get_self().value );
   auto usr = usr_ext_table.find( user.value );
   if( usr == usr_ext_table.end() ) {
      usr_ext_table.emplace(get_self(), [&]( auto& u ){
        u.user = user;
        u.vested = asset(0, symbol("FREEOS",4));
      });
   }


   // update number of users in record_count singleton
   user_counter.set(entry, get_self());

   // set the stake requirement for an unregistered user i.e. a user who has not staked or claimed and therefore does not have a user record
   uint32_t next_user_stake = getthreshold(entry.usercount + 1, account_type);  // using usercount+1 to calculate for the next user
   asset next_user_stake_requirement = asset(next_user_stake * 10000, symbol(CURRENCY_SYMBOL_CODE,4));
   store_unregistered_stake(next_user_stake_requirement);

   return registered_success;
}


// store the unregistered user stake requirement
void freeos::store_unregistered_stake(asset next_user_stake_requirement) {
  stake_index stake_table(get_self(), get_self().value);

  auto stake = stake_table.find( 0 ); // using 0 because this is a single row table

  if( stake == stake_table.end() ) {
     stake_table.emplace(get_self(), [&]( auto& s ){
       s.default_stake = next_user_stake_requirement;
     });
  } else {
     stake_table.modify( stake, same_payer, [&]( auto& s ) {
       s.default_stake = next_user_stake_requirement;
     });
  }
}

// This action for maintenance purposes
[[eosio::action]]
void freeos::maintain(std::string option) {
  require_auth( get_self() );

  if (option == "counter remove") {
    user_singleton user_counter(get_self(), get_self().value);  // owner was originally get_self()
    if (!user_counter.exists()) {
      user_counter.get_or_create(get_self(), ct);
    }

    user_counter.remove();
  }

  if (option == "counter initialise") {
    user_singleton user_counter(get_self(), get_self().value);  // owner was originally get_self()
    if (!user_counter.exists()) {
      user_counter.get_or_create(get_self(), ct);
    }

    auto entry = user_counter.get();
    entry.usercount = 6;
    entry.claimevents = 4;
    entry.unvestweek = 0;

    user_counter.set(entry, get_self());
  }

  /* OBSOLETE
  if (option == "migrate vested") {
    symbol freeos = symbol("FREEOS",4);

    userext_index userext_table(get_self(), get_self().value);
    name thisuser = ""_n;

    // array of users
    name candidates[5] = {"alanappleton"_n, "billbeaumont"_n, "dennisedolan"_n, "ethanedwards"_n, "frankyfellon"_n};

    for(int i = 0; i < 5; i++) {
      thisuser = candidates[i];

      vestaccounts to_acnts( get_self(), thisuser.value );
      auto to = to_acnts.find( freeos.code().raw() );
      if( to != to_acnts.end() ) {
         userext_table.emplace(get_self(), [&]( auto& a ){
           a.user = thisuser;
           a.vested = asset(to->balance);
         });

         // erase the user record from vestaccounts
         to_acnts.erase(to);
       }
    }

  }

  if (option == "set vested values") {
    userext_index userext_table(get_self(), get_self().value);

    // ethanedwards
    name thisuser = "ethanedwards"_n;

    auto u = userext_table.find(thisuser.value);
    if (u != userext_table.end()) {
      userext_table.modify(u, same_payer, [&]( auto& a ) {
        a.vested = asset(210 * 10000, symbol("FREEOS",4));
      });
    }

    // frankyfellon
    thisuser = "frankyfellon"_n;

    auto v = userext_table.find(thisuser.value);
    if (v != userext_table.end()) {
      userext_table.modify(v, same_payer, [&]( auto& a ) {
        a.vested = asset(40 * 10000, symbol("FREEOS",4));
      });
    }
  }
  */


  /*
  if (option == "increment") {
      entry.usercount += 1;
  } else if (option == "reset") {
      entry.usercount = 1;
  } else if (option == "remove") {
      user_counter.remove();
      return;
  } */

  if (option == "initialise") {
      // initialise the default stake value

      // get the value from the config 'stakereqs' table
      stakereq_index stakereqs(name(freeosconfig_acct), name(freeosconfig_acct).value);
      // get the stake-requirements record for threshold 0
      auto sr = stakereqs.find(0);
      if (sr == stakereqs.end()) {
        print("the stake requirements table does not have a record for threshold 0");
        return;
      }

      // the default stake is calculated below
      uint32_t req = sr->requirement_e;
      asset next_user_stake_requirement = asset(req * 10000, symbol(CURRENCY_SYMBOL_CODE,4));

      stake_index stake_table(get_self(), get_self().value);
      auto stake = stake_table.find( 0 ); // using 0 because this is a single row table

      if( stake == stake_table.end() ) {
         stake_table.emplace(get_self(), [&]( auto& s ){
           s.default_stake = next_user_stake_requirement;
         });
      } else {
         stake_table.modify( stake, same_payer, [&]( auto& s ) {
           s.default_stake = next_user_stake_requirement;
         });
      }

      print("default stake set to ", next_user_stake_requirement.to_string());
  }

  if (option == "vested") {
      float vested_proportion = get_vested_proportion();

      uint16_t claim_amount = 100;
      uint16_t vested_amount = claim_amount * 0.009f;

      print("vested amount is ", vested_amount);
      return;
  }

}


// for deregistering user
[[eosio::action]]
void freeos::dereg(const name& user) {
  require_auth( get_self() );

  user_index usertable( get_self(), user.value );
  auto u = usertable.find( symbol_code(CURRENCY_SYMBOL_CODE).raw() );

  if(u != usertable.end()) {
    if (u->stake.amount == 0) {
      // erase the user record
      usertable.erase(u);

      // decrement number of users in record_count singleton
      user_singleton user_counter(get_self(), get_self().value);
      if (!user_counter.exists()) {
        user_counter.get_or_create(get_self(), ct);
      }
      auto entry = user_counter.get();
      entry.usercount -= 1;
      user_counter.set(entry, get_self());
    } else {
      print("account ", user, " has staked amount and cannot be deregistered");
    }

    print("account ", user, " removed from user register");
  } else {
    print("account ", user, " not in register");
  }

}


// stake action
[[eosio::on_notify("eosio.token::transfer")]]
void freeos::stake(name user, name to, asset quantity, std::string memo) {

  if (memo == "freeos stake") {
    if (user == get_self()) {
      return;
    }

    // check that system is operational (global masterswitch parameter set to "1")
    check(checkmasterswitch(), msg_freeos_system_not_available);

    //****************************************************

    // auto-register the user - if user is already registered then that is ok, the register_user function responds silently
    registration_status result = register_user(user, "e");

    //****************************************************


    // get the user record - the amount of the stake requirement and the amount staked
    // find the account in the user table
    user_index usertable( get_self(), user.value );
    auto u = usertable.find( symbol_code(CURRENCY_SYMBOL_CODE).raw() );

    // check if the user is registered
    check(u != usertable.end(), "user is not registered");

    // print("req: ", u->stake_requirement.to_string(), " quantity: ", quantity.to_string());

    // check that user isn't already staked
    check(u->staked_time == time_point_sec(0), "the account is already staked");

    // check that the required stake has been transferred
    check(u->stake_requirement == quantity, "the stake amount is not what is required");

    // update the user record
    usertable.modify(u, to, [&](auto& row) {   // second argument is scope
      row.stake += quantity;
      row.staked_time = time_point_sec(current_time_point().sec_since_epoch());
    });

    print(quantity.to_string(), " stake received for account ", user);
  }

  tick("U");   // User-driven tick

}


[[eosio::action]]
void freeos::unstake(const name& user) {

  require_auth(user);

  // check that system is operational (global masterswitch parameter set to "1")
  check(checkmasterswitch(), msg_freeos_system_not_available);

  // find user record
  user_index usertable( get_self(), user.value );
  auto u = usertable.find( symbol_code(CURRENCY_SYMBOL_CODE).raw() );

  // check if the user is registered
  check(u != usertable.end(), msg_account_not_registered);

  // check if the user has an amount staked
  check(u->stake.amount > 0, "account has not staked");

  // if enough time elapsed then refund amount
  check((u->staked_time.utc_seconds + STAKE_HOLD_TIME_SECONDS) <= current_time_point().sec_since_epoch(), "stake has not yet been held for one week and cannot be refunded");

  // refund the stake tokens
  // transfer stake from freeos to user account using the eosio.token contract
  action transfer = action(
    permission_level{get_self(),"active"_n},
    "eosio.token"_n,
    "transfer"_n,
    std::make_tuple(get_self(), user, u->stake, std::string("refund of freeos stake"))
  );

  transfer.send();

  // update the user record
  usertable.modify(u, user, [&](auto& row) {   // second argument is scope
    row.stake = asset(0, symbol(CURRENCY_SYMBOL_CODE, 4));
    row.staked_time = time_point_sec(0);
  });

  print("stake successfully refunded");

  tick("U");   // User-driven tick

}


[[eosio::action]]
void freeos::getuser(const name& user) {
  user_index usertable( get_self(), user.value );
  auto u = usertable.find( symbol_code(CURRENCY_SYMBOL_CODE).raw() );

  // check if the user is registered
  check(u != usertable.end(), "user is not registered");

  print("account: ", user, " account type ", u->account_type, ", stake req = ", u->stake_requirement.to_string(), ", stake = ", u->stake.to_string(), ", staked on ", u->staked_time.utc_seconds);
}


bool freeos::checkmasterswitch() {
  parameter_index parameters("freeosconfig"_n, "freeosconfig"_n.value);
  auto iterator = parameters.find("masterswitch"_n.value);

  // check if the parameter is in the table or not
  if (iterator == parameters.end() ) {
      // the parameter is not in the table, or table not found, return false because it should be accessible (failsafe)
      return false;
  } else {
      // the parameter is in the table
      const auto& parameter = *iterator;

      if (parameter.value.compare("1") == 0) {
        return true;
      } else {
        return false;
      }
  }
}

bool freeos::checkschedulelogging() {
  parameter_index parameters("freeosconfig"_n, "freeosconfig"_n.value);
  auto iterator = parameters.find("schedulelog"_n.value);

  // check if the parameter is in the table or not
  if (iterator == parameters.end() ) {
      // the parameter is not in the table, or table not found, return false because it should be accessible (failsafe)
      return false;
  } else {
      // the parameter is in the table
      const auto& parameter = *iterator;

      if (parameter.value.compare("1") == 0) {
        return true;
      } else {
        return false;
      }
  }
}


void freeos::create( const name&   issuer,
                    const asset&  maximum_supply )
{
    require_auth( get_self() );

    auto sym = maximum_supply.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( maximum_supply.is_valid(), "invalid supply");
    check( maximum_supply.amount > 0, "max-supply must be positive");

    stats statstable( get_self(), sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    check( existing == statstable.end(), "token with symbol already exists" );

    statstable.emplace( get_self(), [&]( auto& s ) {
       s.supply.symbol = maximum_supply.symbol;
       s.conditional_supply.symbol = maximum_supply.symbol;
       s.max_supply    = maximum_supply;
       s.issuer        = issuer;
    });
}


void freeos::issue( const name& to, const asset& quantity, const string& memo )
{
    auto sym = quantity.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    stats statstable( get_self(), sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    check( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
    const auto& st = *existing;
    check( to == st.issuer, "tokens can only be issued to issuer account" );

    require_auth( st.issuer );
    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must issue positive quantity" );

    check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    check( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply += quantity;
    });

    add_balance( st.issuer, quantity, st.issuer );
}


void freeos::retire( const asset& quantity, const string& memo )
{
    auto sym = quantity.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    stats statstable( get_self(), sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    check( existing != statstable.end(), "token with symbol does not exist" );
    const auto& st = *existing;

    require_auth( st.issuer );
    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must retire positive quantity" );

    check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply -= quantity;
    });

    sub_balance( st.issuer, quantity );
}


void freeos::transfer( const name&    from,
                      const name&    to,
                      const asset&   quantity,
                      const string&  memo )
{
    check( from != to, "cannot transfer to self" );
    require_auth( from );
    check( is_account( to ), "to account does not exist");

    // AIRKEY tokens are non-transferable, except by the freeostokens account
    check(quantity.symbol.code().to_string().compare("AIRKEY") != 0 || from == name(freeos_acct), "AIRKEY tokens are non-transferable");

    auto sym = quantity.symbol.code();
    stats statstable( get_self(), sym.raw() );
    const auto& st = statstable.get( sym.raw() );

    require_recipient( from );
    require_recipient( to );

    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must transfer positive quantity" );
    check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    auto payer = has_auth( to ) ? to : from;

    sub_balance( from, quantity );
    add_balance( to, quantity, payer );

    tick("U");   // User-driven tick
}


void freeos::sub_balance( const name& owner, const asset& value ) {
   accounts from_acnts( get_self(), owner.value );

   const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
   check( from.balance.amount >= value.amount, "overdrawn balance" );

   from_acnts.modify( from, owner, [&]( auto& a ) {
         a.balance -= value;
      });
}


void freeos::add_balance( const name& owner, const asset& value, const name& ram_payer )
{
   accounts to_acnts( get_self(), owner.value );
   auto to = to_acnts.find( value.symbol.code().raw() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
      });
   } else {
      to_acnts.modify( to, same_payer, [&]( auto& a ) {
        a.balance += value;
      });
   }
}


void freeos::add_stake( const name& owner, const asset& value, const name& ram_payer )
{
   user_index to_acnts( get_self(), owner.value );
   auto to = to_acnts.find( value.symbol.code().raw() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.stake = value;
      });
   } else {
      to_acnts.modify( to, same_payer, [&]( auto& a ) {
        a.stake += value;
      });
   }
}


void freeos::sub_stake( const name& owner, const asset& value ) {
   user_index from_acnts( get_self(), owner.value );

   const auto& from = from_acnts.get( value.symbol.code().raw(), "no stake balance found" );
   check( from.stake.amount >= value.amount, "overdrawn balance" );

   from_acnts.modify( from, owner, [&]( auto& a ) {
         a.stake -= value;
      });
}


void freeos::open( const name& owner, const symbol& symbol, const name& ram_payer )
{
   require_auth( ram_payer );

   check( is_account( owner ), "owner account does not exist" );

   auto sym_code_raw = symbol.code().raw();
   stats statstable( get_self(), sym_code_raw );
   const auto& st = statstable.get( sym_code_raw, "symbol does not exist" );
   check( st.supply.symbol == symbol, "symbol precision mismatch" );

   accounts acnts( get_self(), owner.value );
   auto it = acnts.find( sym_code_raw );
   if( it == acnts.end() ) {
      acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = asset{0, symbol};
      });
   }
}


void freeos::close( const name& owner, const symbol& symbol )
{
   require_auth( owner );
   accounts acnts( get_self(), owner.value );
   auto it = acnts.find( symbol.code().raw() );
   check( it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect." );
   check( it->balance.amount == 0, "Cannot close because the balance is not zero." );
   acnts.erase( it );
}


void freeos::claim( const name& user )
{
   require_auth ( user );

   // check that system is operational (global masterswitch parameter set to "1")
   check(checkmasterswitch(), msg_freeos_system_not_available);

   // is this a real account?
   if (!is_account(user)) {
     std::string account_error_msg = std::string("account ") + user.to_string() + " is not a valid account";
     // display the error message
     print(account_error_msg);
     return;
   }

   // auto-register the user - if user is already registered then that is ok, the register_user function responds silently
   registration_status result = register_user(user, "e");


   // what week are we in?
   week this_week = getclaimweek();
   check(this_week.week_number != 0, "freeos is not in a claim period");

   // for debugging
   // print("this_week: ", this_week.week_number, " ", this_week.start, " ", this_week.start_date, " ", this_week.end, " ", this_week.end_date, " ", this_week.claim_amount, " ", this_week.tokens_required);

   // check user eligibility to claim
   if (!eligible_to_claim(user, this_week)) return;

   // update the number of claimevents
   uint32_t claim_event_count = updateclaimeventcount();

   // get freedao multiplier
   uint16_t freedao_multiplier = getfreedaomultiplier(claim_event_count);


   // calculate amounts to be transferred to user and FreeDAO
   // first get the proportion that is vested
   float vested_proportion = get_vested_proportion();

   // work out the vested proportion and liquid proportion of FREEOS to be claimed
   uint16_t claim_tokens = this_week.claim_amount;
   asset claim_amount = asset(claim_tokens * 10000, symbol("FREEOS",4));

   uint16_t vested_tokens = claim_tokens * vested_proportion;
   asset vested_amount = asset(vested_tokens * 10000, symbol("FREEOS",4));

   uint16_t liquid_tokens = claim_tokens - vested_tokens;
   asset liquid_amount = asset(liquid_tokens * 10000, symbol("FREEOS",4));

   uint16_t freedao_tokens = claim_tokens * freedao_multiplier;
   asset freedao_amount = asset(freedao_tokens * 10000, symbol("FREEOS",4));

   uint16_t minted_tokens = liquid_tokens + freedao_tokens;
   asset minted_amount = asset(minted_tokens * 10000, symbol("FREEOS",4));


   // prepare the memo string
   std::string memo = std::string("claim by ") + user.to_string();

   // conditionally limited supply - increment the conditional_supply by total amount of issue
   stats statstable( get_self(), symbol_code("FREEOS").raw() );
   auto currency_record = statstable.find( symbol_code("FREEOS").raw() );

   check( currency_record != statstable.end(), "token with symbol does not exist" );
   const auto& st = *currency_record;

   statstable.modify( st, same_payer, [&]( auto& s ) {
      s.conditional_supply += minted_amount;
   });


   // Issue the required minted amount to the freeos account
   action issue_action = action(
     permission_level{get_self(),"active"_n},
     name(freeos_acct),
     "issue"_n,
     std::make_tuple(get_self(), minted_amount, memo)
   );

   issue_action.send();


   // transfer liquid FREEOS to user
   action user_transfer = action(
     permission_level{get_self(),"active"_n},
     name(freeos_acct),
     "transfer"_n,
     std::make_tuple(get_self(), user, liquid_amount, memo)
   );

   user_transfer.send();


   // transfer FREEOS to freedao_acct
   action freedao_transfer = action(
     permission_level{get_self(),"active"_n},
     name(freeos_acct),
     "transfer"_n,
     std::make_tuple(get_self(), name(freedao_acct), freedao_amount, memo)
   );

   freedao_transfer.send();


   // update the user's vested FREEOS balance
   if (liquid_tokens > 0) {
     userext_index to_acnts( get_self(), get_self().value );
     auto to = to_acnts.find( user.value );
     if( to == to_acnts.end() ) {
        to_acnts.emplace(get_self(), [&]( auto& u ){
          u.user = user;
          u.vested = vested_amount;
        });
     } else {
        to_acnts.modify(to, same_payer, [&]( auto& u ) {
          u.vested += vested_amount;
        });
     }
   }


   // write the claim event to the claim history table
   claim_index claims(get_self(), user.value);
   auto iterator = claims.find(this_week.week_number);
   claims.emplace( get_self(), [&]( auto& claim ) {
     claim.week_number = this_week.week_number;
     claim.claim_time = current_time_point().sec_since_epoch();
   });

   print(user, " claimed ", claim_amount.to_string(), " for week ", this_week.week_number); // " at ", current_time_point().sec_since_epoch());

   tick("U");   // User-driven tick
}


float freeos::get_vested_proportion() {
  // default rate if exchange rate record not found, or if current price >= target price (so no need to vest)
  float proportion = 0.0f;

  exchange_index rate(name(freeosconfig_acct), name(freeosconfig_acct).value);

  // there is a single record so we can position iterator on first record
  auto iterator = rate.begin();

  // if the exchange rate exists in the table
  if (iterator != rate.end() ) {
      // get current and target rates
      double currentprice = iterator->currentprice;
      double targetprice = iterator->targetprice;

      if (targetprice > 0 && currentprice < targetprice) {
        proportion = 1.0f - (currentprice / targetprice);
      }
  }

  return proportion;
}

void freeos::getcounts() {
  user_singleton user_counter(get_self(), get_self().value);
  if (user_counter.exists()) {
    auto entry = user_counter.get();
    print("users registered: ", entry.usercount, ", claim events: ", entry.claimevents, " unvest week: ", entry.unvestweek);
  } else {
    print("the count singleton has not been initialised");
  }
}



// look up the required stake depending on number of users and account type
uint32_t freeos::getthreshold(uint32_t numusers, std::string account_type) {
  uint64_t required_stake;

  // require_auth(get_self()); -- read only, so will not require authentication

  stakereq_index stakereqs(name(freeosconfig_acct), name(freeosconfig_acct).value);
  auto iterator = stakereqs.end();

  // find which band to apply - iterate from the end of the table upwards until the matching threshold is found
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

  return required_stake;
}


// return the week record matching the current datetime
freeos::week freeos::getclaimweek() {

  freeos::week this_week = week {0, 0, "", 0, "", 0, 0};    // default null week value if outside of a claim period

  // current time in UTC seconds
  uint32_t now = current_time_point().sec_since_epoch();

  // iterate through week records and find one that matches current time
  week_index freeosweeks(name(freeosconfig_acct), name(freeosconfig_acct).value);
  auto iterator = freeosweeks.begin();

  while (iterator != freeosweeks.end()) {
    if ((now >= iterator->start) && (now <= iterator->end)) {
      this_week = *iterator;
      break;
    }
    // print(" week ", iterator->week_number, " ", iterator->start_date, "-", iterator->end_date, " >> ");
    iterator++;
  }

  //print ("week_number = ", week_number);
  return this_week;
}


// calculate if user is eligible to claim in a week
bool freeos::eligible_to_claim(const name& claimant, week this_week) {

  // get the user record - if there is no record then user is not registered
  user_index users(get_self(), claimant.value);
  auto user_record = users.begin();
  if (user_record == users.end()) {
    print("user ", claimant, " is not registered");
    return false;
  }

  // has the user claimed this week - consult the claims history table
  claim_index claims(get_self(), claimant.value);
  auto iterator = claims.find(this_week.week_number);
  // if the claim record exists for the week then the user has claimed, so is not eligible to claim again
  if (iterator != claims.end()) {
    print("user ", claimant, " has already claimed in week ", this_week.week_number);
    return false;
  }

  // has the user met their staking requirement
  if (user_record->stake != user_record->stake_requirement) {
    print("user ", claimant, " has not staked the required ", user_record->stake_requirement.to_string());
    return false;
  }

  // how many airkey tokens does the user have?
  asset user_airkey_balance = asset(0, symbol("AIRKEY",0));  // default = 0 AIRKEY

  accounts user_accounts(get_self(), claimant.value);
  symbol_code airkey = symbol_code("AIRKEY");
  auto user_airkey_account = user_accounts.find(airkey.raw());

  if (user_airkey_account != user_accounts.end()) {
    user_airkey_balance = user_airkey_account->balance;
  }

  // for debugging purposes
  // print("user ", claimant, " has FREEOS balance of ", user_freeos_balance.to_string());
  // print("user ", claimant, " has AIRKEY balance of ", user_airkey_balance.to_string());

  // only perform the FREEOS holding requirement check if the user does NOT have an AIRKEY token
  if (user_airkey_balance.amount == 0) {
    // check that the user has the required balance of FREEOS
    asset user_freeos_balance = asset(0, symbol("FREEOS",4));  // default holding = 0 FREEOS

    symbol_code freeos = symbol_code("FREEOS");
    auto user_freeos_account = user_accounts.find(freeos.raw());

    if (user_freeos_account != user_accounts.end()) {
      user_freeos_balance = user_freeos_account->balance;
    }

    // the 'holding' balance requirement for this week's claim
    asset week_holding_requirement = asset(this_week.tokens_required * 10000, symbol("FREEOS",4));
    // print("holding balance required for week ", this_week.week_number, " is ", holding_balance.to_string());

    if (user_freeos_balance < week_holding_requirement) {
      print("user ", claimant, " has ", user_freeos_balance.to_string(), " which is less than the holding requirement of ", week_holding_requirement.to_string());
      return false;
    }
  }

  // if the user passes all these checks then they are eligible
  return true;
}


uint32_t freeos::updateclaimeventcount() {
  user_singleton user_counter(get_self(), get_self().value);  // owner was originally get_self()

  if (!user_counter.exists()) {
    user_counter.get_or_create(get_self(), ct);
  }
  auto entry = user_counter.get();

  entry.claimevents += 1;
  user_counter.set(entry, get_self());

  return entry.claimevents;
}

uint16_t freeos::getfreedaomultiplier(uint32_t claimevents) {

/*  FOR PRODUCTION
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
    } */

    // FOR TEST PURPOSES
    if (claimevents <= 2) {
      return 233;
    } else if (claimevents <= 4) {
      return 144;
    } else if (claimevents <= 5) {
      return 89;
    } else if (claimevents <= 6) {
      return 55;
    } else if (claimevents <= 7) {
      return 34;
    } else if (claimevents <= 8) {
      return 21;
    } else if (claimevents <= 9) {
      return 13;
    } else if (claimevents <= 10) {
      return 8;
    } else if (claimevents <= 11) {
      return 5;
    } else if (claimevents <= 12) {
      return 3;
    } else if (claimevents <= 13) {
      return 2;
    } else {
      return 1;
    }

}
