#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include "../common/freeoscommon.hpp"
#include "freeos.hpp"
#include <cmath>

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
// 308 - userext_table migrated back to vestaccounts
// 309 - dummy 'unvest' action
// 310 - single record 'counters' table replaces old singleton
// 311 - unvest action completed
// 312 - FREEOS holding requirement for a claim also considers vested FREEOS
//     - getuser action upgraded to show user's staking and various balances
// 313 - stakes table contains the staking requirements for all types of users - it is subject to change rather set at registration
//     - stake_requirement field removed from user record
// 314 - added 'stake' action
// 315 - capped vested proportion at 0.9
// 316 - weeks table renamed to iterations
//       claims table - field: week_number renamed to iteration_number
//       unvests table - field: week_number renamed to iteration_number
//       counters table contains a field called 'iteration' which equals the current claim iteration
// 317 - added the unclaim action which removes user's records from claim history table and sets liquid and freeos balance to zero.
// 318 - put in a fix in eligibilty to change how we check if the user has staked
// 319 - added deposits table of accrued transfers to the freedao account - per iteration (used by the dividend contract)
//       action (depositclear) to clear a deposit record from the deposits table
//       verification table used to calculate the user account_type
// 320 - unstake action modified to put unstake request into an unstake queue (the 'unstakes' table)
//       unstakecncl (unstake cancel) action added
// 321 - added new field 'failsafecounter' to the counters table
//       added failsafe unvesting feature
// 322 - Added reverify action - re-checks whether user is verified and changes their account_type accordingly.
//       update unvest percentage driven by user 'unvest' action rather than CRON
//       hourly process refunds stakes
//       failsafe_frequency determined by 'failsafefreq' parameter in freeosconfig
// 323 - fixed bug in the unvest function. Tested and working ok.
// 324 - fixed bug whereby vested freeos was not being decremented after transfer of FREEOS to user.
// 325 - removed reference to the 'weeks' table in the abi
// 326 - tables cleared and representative configuration deployed
// 327 - fixed issue with unstaking where the stake release time was calculated as staked_time + holding time. It should
//       of course be current_time + holding time.
// 328 - If the user has a zero stake requirement then we consider them to have staked at registration time i.e. user.staked_time is set
//       Related to above - if user has 0 XPR staked then we don't need to do a transfer in order to unstake
// 329 - Removed unneccesary table definitions from the hpp file and abi
// 330 - All error checking performed by check function
//       User can only request to unstake once i.e. cannot create multiple unstake requests
//       Iterations table can now be accessed via secondary index (by start time)
// 331 - Changed unvest action to not allow unvesting in iteration 0
//       Corrected problem of advancing unvestperecentiteration more often than required
// 332 - Improved performance of getclaimiteration function - it uses secondary index based on iteration start
//       Improved performance of getthreshold function - it uses the primary index based on threshold
// 333 - User registration record now tracks the number of FREEOS issuances (claims) and the iteration of last issuance.
//       User registration record no longer records staked_time. It now records the iteration in which the stake was made.
//       Unstake requests are processed within the next iteration i.e. the 'stake hold time' is no longer used.
//       There is a fallback 'default' unvestpercent in case the figure can't be calculated. Specified in the 'unvestpercnt' parameter.
// 334 - Replaced the 'counters' table with the 'statistics' table to solve a record corruption problem
//       The freeos::stakes table has been deleted - stake requirement code now refers to the freeosconfig::stakereqs table instead

const std::string VERSION = "0.334";

[[eosio::action]]
void freeos::version() {
  iteration this_iteration = getclaimiteration();

  uint32_t sr_e = get_stake_requirement('e');

  if (DEBUG) print("Version = ", VERSION, " - it is currently iteration ", this_iteration.iteration_number, " stake requirement_e = ", sr_e);
}


[[eosio::action]]
void freeos::currentiter() {
  iteration this_iteration = getclaimiteration();

  if (DEBUG) print("It is currently iteration ", this_iteration.iteration_number);
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

  tick(TRIGGERED_BY_PROTON);
}


[[eosio::action]]
void freeos::clearlog() {
  // clear schedule log
  schedulelog_index log(get_self(), get_self().value);

  auto iterator = log.begin();

  while (iterator != log.end()) {
    iterator = log.erase(iterator);
  }

  // clear ticker table
  tickers ticks(get_self(), get_self().value);

  auto itick = ticks.begin();

  while (itick != ticks.end()) {
    itick = ticks.erase(itick);
  }

}


// These actions allow the scheduled processes to be invoked explicitly by the freeosticker account

// run scheduled process action
void freeos::runscheduled(std::string process_specifier, bool schedule_override) {

  require_auth(permission_level("freeosticker"_n, "active"_n));

  // validate the parameters - prepare error message first
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

  if (DEBUG) print("schedule timers have been initialised, process is not yet scheduled to run");
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
      hourly_process(TRIGGERED_BY_ACTION);

      process_ran = true;
      if (DEBUG) print("hourly process ran");
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
      if (DEBUG) print("daily process ran");
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
      if (DEBUG) print("weekly process ran");
    }
  }

  if (process_ran == false) {
    if (DEBUG) print(process_specifier, " process has not run (did not override schedule)");
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
  set_iteration_number(); // advance the iteration number if required (used by the dividend contract)
  refund_stakes();

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


void freeos::set_iteration_number() {
  uint32_t  iteration_number = getclaimiteration().iteration_number;

  // set it in the statistics table
  statistic_index statistics(get_self(), get_self().value);
  auto iterator = statistics.begin();

  if (iterator != statistics.end()) {
    statistics.modify(iterator, _self, [&](auto& s) {
        s.iteration = iteration_number;
    });
  }

}


void freeos::update_unvest_percentage() {

  uint32_t current_unvest_percentage;
  uint32_t new_unvest_percentage;

  // find the current vested proprotion. If 0.0f it means that the exchange rate is favourable
  float vested_proportion = get_vested_proportion();

  // get the statistics record
  statistic_index statistics(get_self(), get_self().value);
  auto iterator = statistics.begin();


  // Decide whether we are above target or below target price
  if (vested_proportion == 0.0f) {
    // favourable exchange rate, so implement the 'good times' strategy - calculate the new unvest_percentage
    current_unvest_percentage = iterator->unvestpercent;

    // move the unvest_percentage on to next level
    switch (current_unvest_percentage) {
        case 0 :  new_unvest_percentage = 1;
                  break;
        case 1  : new_unvest_percentage = 2;
                  break;
        case 2  : new_unvest_percentage = 3;
                  break;
        case 3 :  new_unvest_percentage = 5;
                  break;
        case 5 :  new_unvest_percentage = 8;
                  break;
        case 8 :  new_unvest_percentage = 13;
                  break;
        case 13:  new_unvest_percentage = 21;
                  break;
        case 21:  new_unvest_percentage = 21;
                  break;
      }

      // modify the statistics table with the new percentage. Also ensure the failsafe counter is set to 0.
      statistics.modify(iterator, _self, [&](auto& s) {
          s.unvestpercent = new_unvest_percentage;
          s.unvestpercentiteration = getclaimiteration().iteration_number;
          s.failsafecounter = 0;
      });

  } else {
    // unfavourable exchange rate, so implement the 'bad times' strategy
    // calculate failsafe unvest percentage - every Xth week of unfavourable rate, set unvest percentage to 15%

    // get the unvest failsafe frequency - default is 24
    uint8_t failsafe_frequency = 24;

    // read the frequency from the freeosconfig 'parameters' table
    parameter_index parameters(name(freeosconfig_acct), name(freeosconfig_acct).value);
    auto p_iterator = parameters.find(name("failsafefreq").value);

    if (p_iterator != parameters.end()) {
      failsafe_frequency = stoi(p_iterator->value);
    }

    // increment the failsafecounter and determine if we are at 24 weeks
    uint32_t failsafecounter = iterator->failsafecounter;
    failsafecounter++;

    // Store the new failsafecounter and unvestpercent
    statistics.modify(iterator, _self, [&](auto& s) {
        s.failsafecounter = failsafecounter % failsafe_frequency;
        s.unvestpercent = failsafecounter == failsafe_frequency ? 15 : 0;
    });
  }

}


[[eosio::action]]
void freeos::reguser(const name& user) {
  require_auth( user );

  // check that system is operational (global masterswitch parameter set to "1")
  check(checkmasterswitch(), msg_freeos_system_not_available);

  // is this a real account?
  check(is_account(user), "user does not have an account on the network");

  // perform the registration
  registration_status result = register_user(user);

  // give feedback
  if (DEBUG) {
    if (result == registered_already) {
      print("user is already registered");
    } else if (result == registered_success) {
      // prepare the success message
      print(user.to_string() + std::string(" successfully registered"));
    }
  }

}

// register_user is a function available to other actions. This is to enable auto-registration i.e. user is automatically registered whenever they stake or claim.
// return values are defined by enum registration_status.
// N.B. This function is 'silent' - errors and user notifications are handled by the calling actions.
// All prerequisities must be handled by the calling action.

registration_status freeos::register_user(const name& user) {

  // get the current iteration
  iteration current_iteration = getclaimiteration();

  // is the user already registered?
  // find the account in the user table
  user_index usertable( get_self(), user.value );
  auto u = usertable.find( symbol_code(CURRENCY_SYMBOL_CODE).raw() );

  if( u != usertable.end() ) {
    return registered_already;
  }

  // determine account type
  char account_type = get_account_type(user);

  // update the user count in the 'counters' record
  uint32_t numusers;

  statistic_index statistics(get_self(), get_self().value);
  auto iterator = statistics.begin();
  if (iterator == statistics.end())  {
    // emplace
    statistics.emplace( get_self(), [&]( auto& s ) {
      s.usercount = numusers = 1;
      });

  } else {
    // modify
    statistics.modify(iterator, _self, [&](auto& s) {
        s.usercount = numusers = s.usercount + 1;
    });
  }

  // examine the staking requirement for the user - if their staking requirement is 0 then we will consider them to have already staked
  int64_t stake_requirement_amount = get_stake_requirement(account_type);
  asset stake_requirement = asset(stake_requirement_amount, symbol(CURRENCY_SYMBOL_CODE, 4));

  // register the user
  time_point_sec now = time_point_sec(current_time_point().sec_since_epoch());

  usertable.emplace( get_self(), [&]( auto& u ) {
    u.stake = asset(0, symbol(CURRENCY_SYMBOL_CODE, 4));
    u.account_type = account_type;
    u.registered_time = now;
    u.staked_iteration = stake_requirement.amount == 0 ? current_iteration.iteration_number : 0;
    });

   // add the user to the vested accounts table
   vestaccounts v_accounts( get_self(), user.value );
   auto usr = v_accounts.find( symbol_code("FREEOS").raw() );
   if( usr == v_accounts.end() ) {
      v_accounts.emplace(get_self(), [&]( auto& a ){
        a.balance = asset(0, symbol("FREEOS",4));
      });
   }

   return registered_success;
}

// action to allow user to reverify their account_type
[[eosio::action]]
void freeos::reverify(name user) {
  require_auth(user);

  // set the account type
  user_index users(get_self(), user.value);
  auto iterator = users.begin();

  // check if the user has a user registration record
  check (iterator != users.end(), "user is not registered with freeos");

  // get the account type
  char account_type = get_account_type(user);

  // set the user account type
  users.modify(iterator, _self, [&]( auto& u) {
    u.account_type = account_type;
  });

}

// determine the user account type from the Proton verification table
char freeos::get_account_type(name user) {

  // default result
  char user_account_type = 'e';

  // first determine which contract we consult - if we have set an alternative contract then use that one
  name verification_contract;

  parameter_index parameters(name(freeosconfig_acct), name(freeosconfig_acct).value);
  auto p_iterator = parameters.find(name("altverifyacc").value);

  if (p_iterator == parameters.end()) {
    verification_contract = name(verification_contract);  // no alternative contract configured, so use the 'production' contract (for Proton mainnet)
  } else {
    verification_contract = name(p_iterator->value);  // alternative contract is configured, so use that contract instead (for Proton testnet)
  }

  // access the verification table
  usersinfo verify_table(name(verification_contract), name(verification_contract).value);
  auto v_iterator = verify_table.find(user.value);

  if (v_iterator != verify_table.end()) {
    // record found, so default account_type is 'd', unless we find a verification
    user_account_type = 'd';

    auto kyc_prov = v_iterator->kyc;

    for (int i = 0; i < kyc_prov.size(); i++) {
      size_t fn_pos = kyc_prov[0].kyc_level.find("firstname");
      size_t ln_pos = kyc_prov[0].kyc_level.find("lastname");

      if (v_iterator->verified == true && fn_pos != std::string::npos && ln_pos != std::string::npos) {
        user_account_type = 'v';
        break;
      }
    }

  }

  return user_account_type;
}


// for deregistering user
[[eosio::action]]
void freeos::dereg(const name& user) {
  require_auth( get_self() );

  user_index usertable( get_self(), user.value );
  auto u = usertable.find( symbol_code(CURRENCY_SYMBOL_CODE).raw() );

  check(u != usertable.end(), "user is not registered with freeos");
  check(u->stake.amount == 0, "user has staked and cannot be deregistered");

  // erase the user record
  usertable.erase(u);

  // decrement number of users in the statistics table
  statistic_index statistics(get_self(), get_self().value);
  auto iterator = statistics.begin();

  // decrement user count
  statistics.modify( iterator, _self, [&]( auto& s ) {
    s.usercount = s.usercount - 1;
    });

  // feedback
  if (DEBUG) print("account ", user, " removed from user register");
}


// stake confirmation
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
    registration_status result = register_user(user);

    check(result == registered_success, "user registration did not succeed");

    //****************************************************

    // which iteration are we in?
    iteration current_iteration = getclaimiteration();

    // get the user record - the amount of the stake requirement and the amount staked
    // find the account in the user table
    user_index usertable( get_self(), user.value );
    auto u = usertable.begin();

    // check if the user is registered
    check(u != usertable.end(), "user is not registered");

    // check that user isn't already staked
    check(u->staked_iteration == 0, "the account is already staked");


    uint32_t stake_requirement_amount = get_stake_requirement(u->account_type);
    asset stake_requirement = asset(stake_requirement_amount * 10000, symbol(CURRENCY_SYMBOL_CODE, 4));
    check(stake_requirement == quantity, "the stake amount is not what is required");

    // update the user record
    usertable.modify(u, _self, [&](auto& row) {
      row.stake = quantity;
      row.staked_iteration = current_iteration.iteration_number;
    });

    // feedback
    if (DEBUG) print(quantity.to_string(), " stake received for account ", user);
  }

  tick(TRIGGERED_BY_USER);

}


uint32_t freeos::get_stake_requirement(char account_type) {

  // default return value
  uint32_t stake_requirement = 0;

  // get the number of users
  statistic_index statistics(get_self(), get_self().value);
   auto stat = statistics.begin();
   check(stat != statistics.end(), "the statistics record is not found");
   uint32_t numusers = stat->usercount;

  // look up the freeosconfig stakereqs table
  stakereq_index stakereqs(name(freeosconfig_acct), name(freeosconfig_acct).value);
  auto sr = stakereqs.upper_bound(numusers);
  sr--;

  if (account_type == 'v') {
    stake_requirement = sr->requirement_v;
  } else if (account_type == 'd') {
    stake_requirement = sr->requirement_d;
  } else {
    stake_requirement = sr->requirement_e;
  }

  return stake_requirement;
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

  // check if there is already an unstake in progress
  unstakereq_index unstakes(get_self(), get_self().value);
  auto idx = unstakes.get_index<"staker"_n>();
  auto iterator = idx.find(user.value);

  check(iterator == idx.end(), "user has already requested to unstake");
  check(u->stake.amount > 0, "user does not have a staked amount");

  request_stake_refund(user, u->stake);

  // feedback
  if (DEBUG) print("unstake requested");

  tick(TRIGGERED_BY_USER);

}


// unstaking functions

// request stake refund - add to stake refund queue
void freeos::request_stake_refund(name user, asset amount) {
  iteration current_iteration = getclaimiteration();

  // add to the unstake requests queue
  unstakereq_index unstakes(get_self(), get_self().value);
  unstakes.emplace( get_self(), [&]( auto& u ) {
     u.staker = user;
     u.iteration = current_iteration.iteration_number;
     u.amount = amount;
  });
}

// refund stakes
void freeos::refund_stakes() {

  // read the number of unstakes to release - from the freeosconfig 'parameters' table
  uint16_t number_to_release = 3; // default (safe) value if parameter not set
  parameter_index parameters(name(freeosconfig_acct), name(freeosconfig_acct).value);
  auto p_iterator = parameters.find(name("unstakesnum").value);

  if (p_iterator != parameters.end()) {
    number_to_release = stoi(p_iterator->value);
  }

  iteration current_iteration = getclaimiteration();

  unstakereq_index unstakes(get_self(), get_self().value);
  auto iterator = unstakes.begin();

  for (uint16_t i = 0; i < number_to_release && iterator != unstakes.end(); i++) {

    if (iterator->iteration < current_iteration.iteration_number) {
      // process the unstake request
      refund_stake(iterator->staker, iterator->amount);
      iterator = unstakes.erase(iterator);
    } else {
      // we've reached stakes to be released in the future
      break;
    }
  }
}

// refund a stake
void freeos::refund_stake(name user, asset amount) {
  // find user record
  user_index usertable( get_self(), user.value );
  auto u = usertable.find( symbol_code(CURRENCY_SYMBOL_CODE).raw() );

  // transfer stake from freeos to user account using the eosio.token contract
  action transfer = action(
    permission_level{get_self(),"active"_n},
    "eosio.token"_n,
    "transfer"_n,
    std::make_tuple(get_self(), user, amount, std::string("refund of freeos stake"))
  );

  transfer.send();

  // update the user record
  usertable.modify(u, _self, [&](auto& row) {
    row.stake = asset(0, symbol(CURRENCY_SYMBOL_CODE, 4));
    row.staked_iteration = 0;
  });
}


[[eosio::action]]
void freeos::unstakecncl(const name& user) {

  require_auth(user);

  unstakereq_index unstakes(get_self(), get_self().value);
  auto idx = unstakes.get_index<"staker"_n>();
  auto iterator = idx.find(user.value);

  check(iterator != idx.end(), "user does not have an unstake request");

  // cancel the unstake - erase the unstake record
  idx.erase(iterator);

  // feedback
  if (DEBUG) print("your unstake request has been cancelled");
}

// the getuser action exists to help testers get all relevant information for a user
[[eosio::action]]
void freeos::getuser(const name& user) {

  user_index usertable( get_self(), user.value );
  auto u = usertable.begin();

  // check if the user is registered
  check(u != usertable.end(), "user is not registered");

  // get the user's XPR balance
  asset xpr_balance = get_balance("eosio.token"_n, user, symbol("XPR",4));

  // get the user's liquid FREEOS balance
  asset liquid_freeos_balance = get_balance(get_self(), user, symbol("FREEOS",4));

  // get the user's vested FREEOS balance
  asset vested_freeos_balance = asset(0, symbol("FREEOS",4)); // default if account record not found
  vestaccounts v_accounts(get_self(), user.value );
  const auto& ac = v_accounts.find(symbol_code("FREEOS").raw());
  if (ac != v_accounts.end()) {
    vested_freeos_balance = ac->balance;;
  }

  // get the user's AIRKEY balance
  asset airkey_balance = get_balance(get_self(), user, symbol("AIRKEY",0));

  // has user claimed in the current iteration?
  // what iteration is it
  iteration this_iteration = getclaimiteration();

  bool claimed_flag = false;
  claim_index claims(get_self(), user.value);
  auto iterator = claims.find(this_iteration.iteration_number);
  // if the claim record exists for this iteration then the user has claimed
  if (iterator != claims.end()) {
    claimed_flag = true;
  }

  // feedback
  if (DEBUG) print("account: ", user, ", registered: ", u->registered_time.utc_seconds, ", type: ", u->account_type, ", stake: ", u->stake.to_string(),
        ", staked-iteration: ", u->staked_iteration, ", XPR: ", xpr_balance.to_string(), ", liquid: ", liquid_freeos_balance.to_string(), ", vested: ", vested_freeos_balance.to_string(),
         ", airkey: ", airkey_balance.to_string(), ", iteration: ", this_iteration.iteration_number, ", claimed: ", claimed_flag);

}


bool freeos::checkmasterswitch() {
  parameter_index parameters(name(freeosconfig_acct), name(freeosconfig_acct).value);
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
  parameter_index parameters(name(freeosconfig_acct), name(freeosconfig_acct).value);
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

/*
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
*/

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
   check(is_account(user), "user does not have an account on the network");

   // auto-register the user - if user is already registered then that is ok, the register_user function responds silently
   registration_status result = register_user(user);


   // what iteration are we in?
   iteration this_iteration = getclaimiteration();
   check(this_iteration.iteration_number != 0, "freeos is not in a claim period");

   // check user eligibility to claim
   check(eligible_to_claim(user, this_iteration), "user is not eligible to claim in this iteration");

   // update the number of claimevents
   uint32_t claim_event_count = updateclaimeventcount();

   // get freedao multiplier
   uint16_t freedao_multiplier = getfreedaomultiplier(claim_event_count);


   // calculate amounts to be transferred to user and FreeDAO
   // first get the proportion that is vested
   float vested_proportion = get_vested_proportion();

   // work out the vested proportion and liquid proportion of FREEOS to be claimed
   uint16_t claim_tokens = this_iteration.claim_amount;
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

   // record the deposit to the freedao account
   record_deposit(this_iteration.iteration_number, freedao_amount);

   // update the user's vested FREEOS balance
   if (vested_tokens > 0) {
     vestaccounts to_acnts( get_self(), user.value );
     auto to = to_acnts.find( vested_amount.symbol.code().raw() );
     if( to == to_acnts.end() ) {
        to_acnts.emplace(get_self(), [&]( auto& a ){
          a.balance = vested_amount;
        });
     } else {
        to_acnts.modify(to, _self, [&]( auto& a ) {
          a.balance += vested_amount;
        });
     }
   }


   // write the claim event to the claim history table
   claim_index claims(get_self(), user.value);
   auto iterator = claims.find(this_iteration.iteration_number);

   if (iterator == claims.end()) {
     claims.emplace( get_self(), [&]( auto& claim ) {
     claim.iteration_number = this_iteration.iteration_number;
     claim.claim_time = current_time_point().sec_since_epoch();
     });
   }

   // update the user's issuance stats in their registration record
   user_index users(get_self(), user.value);
   auto user_record = users.begin();

   if (user_record != users.end()) {
     users.modify(user_record, _self, [&]( auto& u ) {
      u.issuances += 1;
      u.last_issuance = this_iteration.iteration_number;
    });
   }

  // feedback
   if (DEBUG) print(user, " claimed ", liquid_amount.to_string(), " and vested ", vested_amount.to_string(), " for iteration ", this_iteration.iteration_number); // " at ", current_time_point().sec_since_epoch());

   tick(TRIGGERED_BY_USER);
}

// record a deposit to the freedao account
void freeos::record_deposit(uint64_t iteration_number, asset amount) {
  deposit_index deposits(get_self(), get_self().value);

  // find the record for the iteration
  auto iterator = deposits.find(iteration_number);

  if (iterator == deposits.end()) {
    // insert record and initialise
    deposits.emplace(get_self(), [&]( auto& d ){
      d.iteration = iteration_number;
      d.accrued = amount;
    });
  } else {
    // modify record
    deposits.modify(iterator, _self, [&]( auto& d ) {
      d.accrued += amount;
    });
  }
}

// action to clear (remove) a deposit record from the deposit table
[[eosio::action]]
void freeos::depositclear(uint64_t iteration_number) {
  require_auth(name(freedao_acct));

  deposit_index deposits(get_self(), get_self().value);

  // find the record for the iteration
  auto iterator = deposits.find(iteration_number);

  check(iterator != deposits.end(), "a deposit record for the requested iteration does not exist");

  deposits.erase(iterator);

}

void freeos::unclaim( const name& user )
{
   require_auth (get_self());

   // remove the user's history from the claims table
   claim_index claims(get_self(), user.value);
   auto iterator = claims.begin();

   while (iterator != claims.end()) {
     iterator = claims.erase(iterator);
   }

   // set the user's liquid FREEOS balance to zero
   accounts user_liquid_accounts(get_self(), user.value);
   auto user_liquid_freeos = user_liquid_accounts.find(symbol_code("FREEOS").raw());
   if (user_liquid_freeos != user_liquid_accounts.end()) {
     user_liquid_accounts.modify(user_liquid_freeos, same_payer, [&]( auto& a ) {
       a.balance = asset(0, symbol("FREEOS",4));
     });
   }

   // set the user's vested FREEOS balance to zero
   vestaccounts user_vested_accounts(get_self(), user.value);
   auto user_vested_freeos = user_vested_accounts.find(symbol_code("FREEOS").raw());
   if (user_vested_freeos != user_vested_accounts.end()) {
     user_vested_accounts.modify(user_vested_freeos, same_payer, [&]( auto& a ) {
       a.balance = asset(0, symbol("FREEOS",4));
     });
   }

}



void freeos::unvest(const name& user)
{
   require_auth ( user );

   // check that system is operational (global masterswitch parameter set to "1")
   check(checkmasterswitch(), msg_freeos_system_not_available);

   // check that the user exists
   check(is_account(user), "User does not have an account");

   // get the current iteration
   iteration this_iteration = getclaimiteration();

   check(this_iteration.iteration_number > 0, "Not in a valid iteration");

   // get the unvestpercentiteration - if different from current iteration then update it and update the unvestpercentage
   statistic_index statistics(get_self(), get_self().value);
   auto stat = statistics.begin();

   if (stat != statistics.end()) {
     if (this_iteration.iteration_number > stat->unvestpercentiteration) {
       update_unvest_percentage();
     }
   }


   // has the user unvested this iteration? - consult the unvests history table
   unvest_index unvests(get_self(), user.value);
   auto iterator = unvests.find(this_iteration.iteration_number);
   // if the unvest record exists for the iteration then the user has unvested, so is not eligible to unvest again
   check(iterator == unvests.end(), "user has already unvested in this iteration");

   // do the unvesting

   // calculate the amount to be unvested - get the percentage for the iteration
   uint32_t unvest_percent = 0;
   auto iter = statistics.begin();

   if (iter != statistics.end()) {
     unvest_percent = iter->unvestpercent;
   } else {
     unvest_percent = 50; // default if freeosconfig parameter not found

     // if no counters record then use the default unvestpercent
     parameter_index parameters(name(freeosconfig_acct), name(freeosconfig_acct).value);
     auto iterator = parameters.find("unvestpercnt"_n.value);

     if (iterator != parameters.end()) {
       unvest_percent = stoi(iterator->value);
     }     
   }
   
   if (unvest_percent == 0) {
     // nothing to unvest
     if (DEBUG) print("Vested FREEOS cannot be unvested in this claim period. Please try next claim period.");
     return;
   }

   // get the user's unvested FREEOS balance
   asset user_vbalance = asset(0, symbol("FREEOS",4));
   vestaccounts v_accounts(get_self(), user.value);
   auto v_it = v_accounts.begin();

   if (v_it != v_accounts.end()) {
     user_vbalance = v_it->balance;
   }

   // if user's vested balance is 0 then nothing to do, so return
   if (user_vbalance.amount == 0) {
     if (DEBUG) print("You have no vested FREEOS therefore nothing to unvest.");
     return;
   }


   // calculate the amount of vested FREEOS to convert to liquid FREEOS
   // Warning: these calculations use mixed-type arithmetic. Any changes need to be thoroughly tested.

   // check that the unvest percentage is within limits
   check(unvest_percent > 0 && unvest_percent <= 100, "The unvest percentage is incorrect. Please notify FreeDAO.");

   uint64_t vestedunits = user_vbalance.amount;   // in currency units (i.e. number of 0.0001 FREEOS)
   double percentage = unvest_percent / 100.0;    // required to be a double
   uint64_t convertedunits = vestedunits * percentage;  // in currency units (i.e. number of 0.0001 FREEOS)
   uint32_t roundedupfreeos = (uint32_t) ceil(convertedunits / 10000.0);  // ceil rounds up to the next whole number of FREEOS
   asset convertedfreeos = asset(roundedupfreeos * 10000, symbol("FREEOS", 4)); // express the roundedupfreeos as an asset


   // conditionally limited supply - increment the conditional_supply by total amount of issue
   stats statstable( get_self(), symbol_code("FREEOS").raw() );
   auto currency_record = statstable.find( symbol_code("FREEOS").raw() );

   check( currency_record != statstable.end(), "token with symbol does not exist" );
   const auto& st = *currency_record;

   statstable.modify( st, same_payer, [&]( auto& s ) {
      s.conditional_supply += convertedfreeos;
   });

   std::string memo = std::string("unvesting FREEOS by ");
   memo.append(user.to_string());

   // Issue the required amount to the freeos account
   action issue_action = action(
     permission_level{get_self(),"active"_n},
     name(freeos_acct),
     "issue"_n,
     std::make_tuple(get_self(), convertedfreeos, memo)
   );

   issue_action.send();

   // transfer liquid FREEOS to user
   action user_transfer = action(
     permission_level{get_self(),"active"_n},
     name(freeos_acct),
     "transfer"_n,
     std::make_tuple(get_self(), user, convertedfreeos, memo)
   );

   user_transfer.send();

   // subtract the amount transferred from the unvested record
   v_accounts.modify( v_it, _self, [&]( auto& v ) {
     v.balance -= convertedfreeos;
   });


   // write the unvest event to the unvests history table
   iterator = unvests.find(this_iteration.iteration_number);
   if (iterator == unvests.end()) {
     unvests.emplace( get_self(), [&]( auto& unvest ) {
       unvest.iteration_number = this_iteration.iteration_number;
       unvest.unvest_time = current_time_point().sec_since_epoch();
     });
   }

   // feedback
   if (DEBUG) print("Unvesting successful. You have gained another ", convertedfreeos.to_string());

   tick(TRIGGERED_BY_USER);
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

  // apply a cap of 0.9
  if (proportion > 0.9f) {
    proportion = 0.9f;
  }

  return proportion;
}


// look up the required threshold for the number of users
uint64_t freeos::getthreshold(uint32_t numusers) {

  // look up the config stakereqs table
  stakereq_index stakereqs(name(freeosconfig_acct), name(freeosconfig_acct).value);
  auto iterator = stakereqs.upper_bound(numusers);
  iterator--; 

  return iterator->threshold;
}


// return the current iteration record
freeos::iteration freeos::getclaimiteration() {

  freeos::iteration this_iteration = iteration {0, 0, "", 0, "", 0, 0};    // default null iteration value if outside of a claim period

  // current time in UTC seconds
  uint32_t now = current_time_point().sec_since_epoch();

  // iterate through iteration records and find one that matches current time
  iteration_index iterations(name(freeosconfig_acct), name(freeosconfig_acct).value);
  auto idx = iterations.get_index<"start"_n>();
  auto iterator = idx.upper_bound(now);

  if (iterator != idx.begin()) {
    iterator--;
  }

  // check we are within the period of the iteration
  if (iterator != idx.end() && now >= iterator->start && now <= iterator->end) {
    this_iteration = *iterator;
  }  
  
  return this_iteration;
}


// calculate if user is eligible to claim in this iteration
bool freeos::eligible_to_claim(const name& claimant, iteration this_iteration) {

  // get the user record - if there is no record then user is not registered
  user_index users(get_self(), claimant.value);
  auto user_record = users.begin();

  check(user_record != users.end(), "user is not registered in freeos");

  // has the user claimed this iteration - consult the claims history table
  claim_index claims(get_self(), claimant.value);
  auto iterator = claims.find(this_iteration.iteration_number);
  // if the claim record exists for the iteration then the user has claimed, so is not eligible to claim again
  if (iterator != claims.end()) {
    if (DEBUG) print("user ", claimant, " has already claimed in claim period ", this_iteration.iteration_number);
    return false;
  }

  // has the user staked?
  if (user_record->staked_iteration == 0) {
    if (DEBUG) print("user ", claimant, " has not staked");
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

  // only perform the FREEOS holding requirement check if the user does NOT have an AIRKEY token
  if (user_airkey_balance.amount == 0) {
    // check that the user has the required balance of FREEOS
    asset liquid_freeos_balance = asset(0, symbol("FREEOS",4));  // default holding = 0 FREEOS

    symbol_code freeos = symbol_code("FREEOS");
    auto user_freeos_account = user_accounts.find(freeos.raw());

    if (user_freeos_account != user_accounts.end()) {
      liquid_freeos_balance = user_freeos_account->balance;
    }

    // check the user's vested FREEOS balance
    asset vested_freeos_balance = asset(0, symbol("FREEOS",4)); // default if account record not found
    vestaccounts v_accounts(get_self(), claimant.value );
    const auto& ac = v_accounts.find(symbol_code("FREEOS").raw());
    if (ac != v_accounts.end()) {
      vested_freeos_balance = ac->balance;;
    }

    // user's total FREEOS balance is liquid-FREEOS plus vested-FREEOS
    asset total_freeos_balance = liquid_freeos_balance + vested_freeos_balance;

    // the 'holding' balance requirement for this iteration's claim
    asset iteration_holding_requirement = asset(this_iteration.tokens_required * 10000, symbol("FREEOS",4));
    
    if (total_freeos_balance < iteration_holding_requirement) {
      if (DEBUG) print("user ", claimant, " has ", total_freeos_balance.to_string(), " which is less than the holding requirement of ", iteration_holding_requirement.to_string());
      return false;
    }
  }

  // if the user passes all these checks then they are eligible
  return true;
}


uint32_t freeos::updateclaimeventcount() {

  uint32_t claimevents;

  statistic_index statistics(get_self(), get_self().value);
  auto iterator = statistics.begin();

  // modify
  statistics.modify(iterator, _self, [&](auto& s) {
        s.claimevents = claimevents = s.claimevents + 1;
  });

  return claimevents;
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

}
