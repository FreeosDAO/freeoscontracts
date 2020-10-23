#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include "../common/freeoscommon.hpp"
#include "freeos.hpp"

using namespace eosio;


[[eosio::action]]
void freeos::reguser(const name& user, const std::string account_type) {
  require_auth( user );

  // check that system is operational (global masterswitch parameter set to "1")
  check(checkmasterswitch(), "freeos system is not operating");

  check( is_account( user ), "user account does not exist");
  check(account_type.length() == 1, "account type should be 1 character");

  // calculate the user number when registered
  user_singleton user_counter(get_self(), get_self().value);  // owner was originally get_self()
  if (!user_counter.exists()) {
    user_counter.get_or_create(get_self(), ct);
  }
  auto entry = user_counter.get();
  entry.count += 1;   // new user number

  // get the required stake for the user number
  uint32_t stake = getthreshold(entry.count, account_type);
  //uint32_t stake = 10;
  asset stake_requirement = asset(stake * 10000, symbol("EOS",4)); // TODO: look up from the config table

  // find the account in the user table
  user_index usertable( get_self(), user.value );
  auto u = usertable.find( symbol_code("EOS").raw() );

  // add the user if not already registered
  if( u == usertable.end() ) {
     usertable.emplace( get_self(), [&]( auto& u ) {
       u.stake = asset(0, symbol("EOS", 4));
       u.account_type = account_type.at(0);
       u.stake_requirement = stake_requirement;
       u.registered_time = time_point_sec(current_time_point().sec_since_epoch());
       u.staked_time = time_point_sec(0);

       // update number of users in record_count singleton
       user_counter.set(entry, get_self());

       print("account number ", entry.count, ": ", user, " registered with account type ", u.account_type, ", stake requirement = ", u.stake_requirement.to_string());
     });

  } else {
    print("account ", user, " is already registered");
  }
}


// for deregistering user
[[eosio::action]]
void freeos::dereg(const name& user) {
  require_auth( get_self() );

  user_index usertable( get_self(), user.value );
  auto u = usertable.find( symbol_code("EOS").raw() );

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
      entry.count -= 1;
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

  if (user == get_self()) {
    return;
  }

  // check that system is operational (global masterswitch parameter set to "1")
  check(checkmasterswitch(), "freeos system is not operating");

  // get the user record - the amount of the stake requirement and the amount staked
  // find the account in the user table
  user_index usertable( get_self(), user.value );
  auto u = usertable.find( symbol_code("EOS").raw() );

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


[[eosio::action]]
void freeos::unstake(const name& user) {

  require_auth(user);

  // check that system is operational (global masterswitch parameter set to "1")
  check(checkmasterswitch(), "freeos system is not operating");

  // find user record
  user_index usertable( get_self(), user.value );
  auto u = usertable.find( symbol_code("EOS").raw() );

  // check if the user is registered
  check(u != usertable.end(), "account is not registered");

  // check if the user has an amount staked
  check(u->stake.amount > 0, "account has not staked");

  // if enough time elapsed then refund amount
  check((u->staked_time.utc_seconds + 604800) <= current_time_point().sec_since_epoch(), "stake has not been held for required time and cannot be refunded");

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
    row.stake = asset(0, symbol("EOS", 4));
    row.staked_time = time_point_sec(0);
  });

  print("stake refunded");

}


[[eosio::action]]
void freeos::getuser(const name& user) {
  user_index usertable( get_self(), user.value );
  auto u = usertable.find( symbol_code("EOS").raw() );

  // check if the user is registered
  check(u != usertable.end(), "user is not registered");

  print("account: ", user, " account type ", u->account_type, ", stake req = ", u->stake_requirement.to_string(), ", stake = ", u->stake.to_string(), ", staked on ", u->staked_time.utc_seconds);
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
    check(!(quantity.symbol.code().to_string().compare("AIRKEY") != 0 && from != name(freeos_acct)), "AIRKEY tokens are non-transferable");

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
   check(checkmasterswitch(), "freeos system is not operating");

   // what week are we in?
   week this_week = getclaimweek();
   check(this_week.week_number != 0, "not in a claim period");

   // for debugging
   // print("this_week: ", this_week.week_number, " ", this_week.start, " ", this_week.start_date, " ", this_week.end, " ", this_week.end_date, " ", this_week.claim_amount, " ", this_week.tokens_required);

   // check user eligibility to claim
   if (!eligible_to_claim(user, this_week)) return;

   // calculate amounts to be transferred to user and FreeDAO
   asset claim_amount = asset(this_week.claim_amount * 10000, symbol("FREEOS",4));
   asset freedao_amount = asset(this_week.freedao_payment * 10000, symbol("FREEOS",4));
   asset total_amount = claim_amount + freedao_amount;

   // prepare the memo string
   std::string memo = std::string("claim by ") + user.to_string();

   // Issue the required total amount to the freeos account
   action issue_action = action(
     permission_level{get_self(),"active"_n},
     name(freeos_acct),
     "issue"_n,
     std::make_tuple(get_self(), total_amount, memo)
   );

   issue_action.send();


   // transfer FREEOS to user
   action user_transfer = action(
     permission_level{get_self(),"active"_n},
     name(freeos_acct),
     "transfer"_n,
     std::make_tuple(get_self(), user, claim_amount, memo)
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


   // write the claim event to the claim history table
   claim_index claims(get_self(), user.value);
   auto iterator = claims.find(this_week.week_number);
   claims.emplace( get_self(), [&]( auto& claim ) {
     claim.week_number = this_week.week_number;
     claim.claim_time = current_time_point().sec_since_epoch();
   });

   print("user ", user, " claimed ", claim_amount.to_string(), " for week ", this_week.week_number, " at ", current_time_point().sec_since_epoch());

}


// look up the required stake depending on number of users and account type
uint32_t freeos::getthreshold(uint32_t numusers, std::string account_type) {
  uint64_t required_stake;

  // require_auth(get_self()); -- read only, so will not require authentication

  stakereq_index stakereqs(name(freeosconfig_acct), name(freeosconfig_acct).value);
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
