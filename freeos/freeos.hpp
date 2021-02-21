#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>


namespace eosiosystem {
   class system_contract;
}

   using namespace eosio;
   using std::string;

enum registration_status{ registered_already,
                          registered_success,
                          };

const std::string HOURLY = "hourly";
const std::string DAILY  = "daily";
const std::string WEEKLY = "weekly";

// ??? test figures
const uint32_t  HOUR_SECONDS  = 180;    // 1/20 normal time
const uint32_t  DAY_SECONDS   = 4320;    // 1/20 normal time
const uint32_t  WEEK_SECONDS  = 30240;    // 1/20 normal time


   /**
    * @defgroup freeos freeos contract
    * @ingroup eosiocontracts
    *
    * freeos contract
    *
    * @details freeos contract defines the structures and actions that allow users to create, issue, and manage
    * tokens on eosio based blockchains.
    * @{
    */
   class [[eosio::contract("freeos")]] freeos : public contract {
      public:
         using contract::contract;

         /**
          * version action.
          *
          * @details Prints the version of this contract.
          */
         [[eosio::action]]
         void version();

         /**
          * runscheduled action.
          *
          * @details Explicitly run a scheduled process.
          * @param process_specifier - hourly, daily or weekly
          * @param schedule_override - true forces the process to run regardless of scheduled times.
          *
          * @pre Requires permission of the freeosticker account
          *
          */
         [[eosio::action]]
         void runscheduled(std::string process_specifier, bool schedule_override);


         /**
          * tick action.
          *
          * @details Triggers scheduled actions.
          * @param trigger - "A" = runscheduled action, "U" = User driven, "P" = Proton CRON, "S" = Server CRON
          */
         [[eosio::action]]
         void tick(std::string trigger);


         /**
          * cron action.
          *
          * @details This action is run by the Proton CRON service.
          */
         [[eosio::action]]
         void cron();


         /**
          * clearlog action.
          *
          * @details clears schedulelog table.
          */
         [[eosio::action]]
         void clearlog();


         /**
          * reguser action.
          *
          * @details Creates a record for the user in the 'stakereqs' table.
          * @param user - the account to be registered,
          *
          * @pre Requires permission of the contract account
          *
          * If validation is successful a new entry in 'stakereqs' table is created for the user account.
          */
         [[eosio::action]]
         void reguser(const name& user);

         /**
          * dereg action.
          *
          * @details Creates a record for the user in the 'stakereqs' table.
          * @param user - the account to be registered,
          * @param account_type - the type of account: "e" is EOS wallet user, "d" is Dapp Account user, "v" is Voice verified user.
          *
          * @pre Requires permission of the contract account
          * @pre The user account must be previously registered
          * @pre The account must not have any staked EOS recorded
          *
          * If validation is successful the record for the user account is deleted from the 'stakereqs' table.
          */
         [[eosio::action]]
         void dereg(const name& user);

         /**
          * stake action.
          *
          * @details Adds a record of staked EOS tokens in the user account record in the 'stakereqs' table.\n
          * @details This action is automatically invoked on the freeos account receiving EOS from the user.\n
          *
          * @param user - the account sending the EOS tokens to freeos,
          * @param to - the 'freeos' account,
          * @param quantity - the asset, i.e. an amount of EOS which is transferred to the freeos account,
          * @param memo - a memo describing the transaction
          *
          * @pre The user account must be previously registered
          * @pre The asset must be equal to the stake requirement for the user
          *
          * If validation is successful the record for the user account is updated to record the amount staked and the date/time of the receipt.
          */
         [[eosio::on_notify("eosio.token::transfer")]]
         void stake(name user, name to, asset quantity, std::string memo);


         /**
          * unstake action.
          *
          * @details Transfer staked EOS back to the user if the waiting time of <holding period> has elapsed since staking.
          * @details Updates the record of staked EOS tokens in the user account record in the 'stakereqs' table.\n
          *
          * @param user - the account to transfer the previously EOS tokens to,
          *
          * @pre The user account must be previously registered and has a non-zero balance of staked EOS
          *
          * If transfer is successful the record for the user account is updated to set the amount staked and stake time/date to zero.
          */
         [[eosio::action]]
         void unstake(const name& user);

         /**
          * getuser action.
          *
          * @details For testing purposes, prints the values stored in the user account record in the 'stakereqs' table.
          *
          * @param user - the user account
          *
          * @pre The user account must be previously registered
          *
          */
         [[eosio::action]]
         void getuser(const name& user);

         /**
          * Create action.
          *
          * @details Allows `issuer` account to create a token in supply of `maximum_supply`.
          * @param issuer - the account that creates the token,
          * @param maximum_supply - the maximum supply set for the token created.
          *
          * @pre Token symbol has to be valid,
          * @pre Token symbol must not be already created,
          * @pre maximum_supply has to be smaller than the maximum supply allowed by the system: 1^62 - 1.
          * @pre Maximum supply must be positive;
          *
          * If validation is successful a new entry in statstable for token symbol scope gets created.
          */
         [[eosio::action]]
         void create( const name&   issuer,
                      const asset&  maximum_supply);
         /**
          * Issue action.
          *
          * @details This action issues to `to` account a `quantity` of tokens.
          *
          * @param to - the account to issue tokens to, it must be the same as the issuer,
          * @param quntity - the amount of tokens to be issued,
          * @memo - the memo string that accompanies the token issue transaction.
          */
         [[eosio::action]]
         void issue( const name& to, const asset& quantity, const string& memo );

         /**
          * Retire action.
          *
          * @details The opposite for create action, if all validations succeed,
          * it debits the statstable.supply amount.
          *
          * @param quantity - the quantity of tokens to retire,
          * @param memo - the memo string to accompany the transaction.
          */
         [[eosio::action]]
         void retire( const asset& quantity, const string& memo );

         /**
          * Transfer action.
          *
          * @details Allows `from` account to transfer to `to` account the `quantity` tokens.
          * One account is debited and the other is credited with quantity tokens.
          *
          * @param from - the account to transfer from,
          * @param to - the account to be transferred to,
          * @param quantity - the quantity of tokens to be transferred,
          * @param memo - the memo string to accompany the transaction.
          */
         [[eosio::action]]
         void transfer( const name&    from,
                        const name&    to,
                        const asset&   quantity,
                        const string&  memo );
         /**
          * Open action.
          *
          * @details Allows `ram_payer` to create an account `owner` with zero balance for
          * token `symbol` at the expense of `ram_payer`.
          *
          * @param owner - the account to be created,
          * @param symbol - the token to be payed with by `ram_payer`,
          * @param ram_payer - the account that supports the cost of this action.
          *
          * More information can be read [here](https://github.com/EOSIO/eosio.contracts/issues/62)
          * and [here](https://github.com/EOSIO/eosio.contracts/issues/61).
          */
         [[eosio::action]]
         void open( const name& owner, const symbol& symbol, const name& ram_payer );

         /**
          * Close action.
          *
          * @details This action is the opposite for open, it closes the account `owner`
          * for token `symbol`.
          *
          * @param owner - the owner account to execute the close action for,
          * @param symbol - the symbol of the token to execute the close action for.
          *
          * @pre The pair of owner plus symbol has to exist otherwise no action is executed,
          * @pre If the pair of owner plus symbol exists, the balance has to be zero.
          */
         [[eosio::action]]
         void close( const name& owner, const symbol& symbol );

         /**
          * claim action.
          *
          * @details This action is run by the user to claim this iteration's allocation of freeos tokens.
          *
          * @param user - the user account to execute the claim action for.
          *
          * @pre Requires authorisation of the user account
          * @pre The user must pass claim eligibility requirements:
          * - must be registered as a freeos user (has a record in the users table)
          * - must not have already claimed for the current iteration
          * - must have staked the required amount of EOS tokens
          * - must hold the required amount of freeos tokens.
          */
         [[eosio::action]]
         void claim( const name& user);

         /**
          * unclaim action.
          *
          * @details This action is run by the contract account to reset the user's claim record and set their balances
          * @details of liquid and vested FREEOS to zero.
          *
          * @param user - the user account to execute the unclaim action for.
          *
          * @pre Requires authorisation of the contract account
          */
         [[eosio::action]]
         void unclaim( const name& user);

         /**
          * unvest action.
          *
          * @details This action is run by the user to release this iteration's allocation of vested freeos tokens.
          *
          * @param owner - the user account to execute the unvest action for.
          *
          * @pre Requires authorisation of the user account
          *
          */
         [[eosio::action]]
         void unvest(const name& user);

         /**
          * getcounts action.
          *
          * @details This action is run by anyone to print the counts of users registered and claim events.
          *
          */
         [[eosio::action]]
         void getcounts();

         /**
          * Get supply method.
          *
          * @details Gets the supply for token `sym_code`, created by `token_contract_account` account.
          *
          * @param token_contract_account - the account to get the supply for,
          * @param sym_code - the symbol to get the supply for.
          */
         static asset get_supply( const name& token_contract_account, const symbol_code& sym_code )
         {
            stats statstable( token_contract_account, sym_code.raw() );
            const auto& st = statstable.get( sym_code.raw() );
            return st.supply;
         }

         /**
          * Get balance method.
          *
          * @details Get the balance for a token `sym_code` created by `token_contract_account` account,
          * for account `owner`.
          *
          * @param token_contract_account - the token creator account,
          * @param owner - the account for which the token balance is returned,
          * @param currency_symbol - the token for which the balance is returned.
          */
         static asset get_balance( const name& token_contract_account, const name& owner, const symbol& currency_symbol )
         {
            asset return_balance = asset(0, currency_symbol); // default if accounts record does not exist

            accounts accountstable( token_contract_account, owner.value );
            const auto& ac = accountstable.find( currency_symbol.code().raw() );
            if (ac != accountstable.end()) {
              return_balance = ac->balance;
            }

            return return_balance;
         }

         /**
          * Clear deposit record action.
          *
          * @details Delete a record with key:iteration_number from the deposits table.
          *
          * @param iteration_number - identifies the record to delete,
          *
          */
          [[eosio::action]]
          void depositclear(uint64_t iteration_number);

          /**
           * Cancel unstake request action.
           *
           * @details Delete the unstake request from the unstakes table.
           *
           * @param user - identifies the user's unstake record to be deleted.
           *
           */
          [[eosio::action]]
          void unstakecncl(const name& user);


          /**
           * Reverify account_type for user action.
           *
           * @details Checks the verification table to see if user has been verified,
           * @details Changes account_type in the user record accordingly.
           *
           * @param user - identifies the user to be verified.
           *
           */
          [[eosio::action]]
          void reverify(name user);


         using create_action = eosio::action_wrapper<"create"_n, &freeos::create>;
         using issue_action = eosio::action_wrapper<"issue"_n, &freeos::issue>;
         using retire_action = eosio::action_wrapper<"retire"_n, &freeos::retire>;
         using transfer_action = eosio::action_wrapper<"transfer"_n, &freeos::transfer>;
         using open_action = eosio::action_wrapper<"open"_n, &freeos::open>;
         using close_action = eosio::action_wrapper<"close"_n, &freeos::close>;

      private:


         // asset ledger
         struct [[eosio::table]] account {
            asset    balance;

            uint64_t primary_key()const { return balance.symbol.code().raw(); }
         };

         typedef eosio::multi_index< "accounts"_n, account > accounts;
         typedef eosio::multi_index< "vestaccounts"_n, account > vestaccounts;


         struct [[eosio::table]] currency_stats {
            asset    supply;
            asset    max_supply;
            asset    conditional_supply;
            name     issuer;

            uint64_t primary_key()const { return supply.symbol.code().raw(); }
         };

         typedef eosio::multi_index< "stat"_n, currency_stats > stats;

         // ********************************
         //using namespace eosio;

         // the registered user table
         struct [[eosio::table]] user {
           asset          stake;            // how many EOS tokens staked
           char           account_type;     // user has EOS-login account, Dapp Account-login or other
           time_point_sec registered_time;  // when the user was registered
           time_point_sec staked_time;      // when the user staked their tokens (staked tokens have a time lock)
           uint16_t       votes;            // how many votes the user has made

           uint64_t primary_key() const {return stake.symbol.code().raw();}
         };

         using user_index = eosio::multi_index<"users"_n, user>;


         // new counters table - to replace the singleton
         struct [[eosio::table]] counter {
           uint32_t  usercount;
           uint32_t  claimevents;
           uint32_t  unvestpercent;
           uint32_t  unvestpercentiteration;
           uint32_t  iteration;
           uint32_t  failsafecounter;

           uint64_t primary_key() const { return 0; } // return a constant (0 in this case) to ensure a single-row table
         };

         using counter_index = eosio::multi_index<"counters"_n, counter>;


         // the user stake requirements
         struct [[eosio::table]] stakereq {
           uint64_t threshold;
           asset    requirement_a;
           asset    requirement_b;
           asset    requirement_c;
           asset    requirement_d;
           asset    requirement_e;
           asset    requirement_u;
           asset    requirement_v;
           asset    requirement_w;
           asset    requirement_x;
           asset    requirement_y;

           uint64_t primary_key() const { return 0; } // return a constant (0 in this case) to ensure a single-row table
         };

         using stakes_index = eosio::multi_index<"stakes"_n, stakereq>;


         // FREEOS USD-price - code: freeosconfig, scope: freeosconfig
         struct [[eosio::table]] price {
           double    currentprice;
           double    targetprice;

           uint64_t primary_key() const { return 0; } // return a constant (0 in this case) to ensure a single-row table
         };

         using exchange_index = eosio::multi_index<"exchangerate"_n, price>;

         // ********************************

         // parameter table

         struct [[eosio::table]] parameter {
           name virtualtable;
           name paramname;
           std::string value;

           uint64_t primary_key() const { return paramname.value;}
           uint64_t get_secondary_1() const {return virtualtable.value;}
         };

         using parameter_index = eosio::multi_index<"parameters"_n, parameter,
         indexed_by<"virtualtable"_n, const_mem_fun<parameter, uint64_t, &parameter::get_secondary_1>>
         >;


         // CONFIG stake requirements table

         struct [[eosio::table]] stakerequire {
           uint64_t    threshold;
           uint32_t    requirement_a;
           uint32_t    requirement_b;
           uint32_t    requirement_c;
           uint32_t    requirement_d;
           uint32_t    requirement_e;
           uint32_t    requirement_u;
           uint32_t    requirement_v;
           uint32_t    requirement_w;
           uint32_t    requirement_x;
           uint32_t    requirement_y;

           uint64_t primary_key() const { return threshold;}
         };

         using stakereq_index = eosio::multi_index<"stakereqs"_n, stakerequire>;


         // freeos airclaim iteration calendar

         struct [[eosio::table]] iteration {
           uint64_t    iteration_number;
           uint32_t    start;
           std::string start_date;
           uint32_t    end;
           std::string end_date;
           uint16_t    claim_amount;
           uint16_t    tokens_required;

           uint64_t primary_key() const { return iteration_number; }
         };

         using iteration_index = eosio::multi_index<"iterations"_n, iteration>;


         // claim history table - scoped on user account name
         struct [[eosio::table]] claimevent {
           uint64_t   iteration_number;
           uint32_t   claim_time;

           uint64_t primary_key() const { return iteration_number; }
         };

         using claim_index = eosio::multi_index<"claims"_n, claimevent>;


         // unvest history table - scoped on user account name
         struct [[eosio::table]] unvestevent {
           uint64_t   iteration_number;
           uint32_t   unvest_time;

           uint64_t primary_key() const { return iteration_number; }
         };

         using unvest_index = eosio::multi_index<"unvests"_n, unvestevent>;


         // record of last time timed processes were run - there is one record hence zero is returned as primary_key
         struct [[eosio::table]] ticker {
            uint32_t  tickly;
            uint32_t  hourly;
            uint32_t  daily;
            uint32_t  weekly;

            uint64_t primary_key()const { return 0; }
         };

         typedef eosio::multi_index< "tickers"_n, ticker > tickers;


         // schedulelog - recording scheduled task runs
         struct [[eosio::table]] schedulelog {
           std::string        task;
           std::string        trigger;
           uint64_t           time;

           uint64_t primary_key() const { return time;}
         };

         using schedulelog_index = eosio::multi_index<"schedulelog"_n, schedulelog>;


         // ***************** KYC from eosio.proton *************
         struct kyc_prov {
         	name kyc_provider;
         	string kyc_level;
         	uint64_t kyc_date;
         };

         struct [[eosio::table]] userinfo {
     			name                                     acc;
     			std::string                              name;
     			std::string                              avatar;
     			bool                                     verified;
     			uint64_t                                 date;
     			uint64_t                                 verifiedon;
     			eosio::name                              verifier;
     			std::vector<eosio::name>                 raccs;
     			std::vector<std::tuple<eosio::name, eosio::name>>  aacts;
     			std::vector<std::tuple<eosio::name, std::string>>  ac;
     			std::vector<kyc_prov>                              kyc;

     			uint64_t primary_key()const { return acc.value; }
     		};

         typedef eosio::multi_index< "usersinfo"_n, userinfo > usersinfo;


         // freedao deposits table
         struct [[eosio::table]] deposit {
           uint64_t   iteration;
           asset      accrued;

           uint64_t primary_key()const { return iteration; }

         };

         using deposit_index = eosio::multi_index<"deposits"_n, deposit>;


         // unstake queue
         struct [[eosio::table]] unstakereq {
           name             staker;
           time_point_sec   release_time;
           asset            amount;

           uint64_t primary_key() const { return release_time.sec_since_epoch(); }
           uint64_t get_secondary() const {return staker.value;}
         };

         using unstakereq_index = eosio::multi_index<"unstakes"_n, unstakereq,
         indexed_by<"staker"_n, const_mem_fun<unstakereq, uint64_t, &unstakereq::get_secondary>>
         >;


/*
// parameter table

struct [[eosio::table]] parameter {
  name virtualtable;
  name paramname;
  std::string value;

  uint64_t primary_key() const { return paramname.value;}
  uint64_t get_secondary_1() const {return virtualtable.value;}
};

using parameter_index = eosio::multi_index<"parameters"_n, parameter,
indexed_by<"virtualtable"_n, const_mem_fun<parameter, uint64_t, &parameter::get_secondary_1>>
>;
*/


         // ********************************

         void sub_balance( const name& owner, const asset& value );
         void add_balance( const name& owner, const asset& value, const name& ram_payer );

         //void sub_stake( const name& owner, const asset& value );
         //void add_stake( const name& owner, const asset& value, const name& ram_payer );

         registration_status register_user(const name& user);

         bool checkmasterswitch();
         bool checkschedulelogging();
         uint64_t getthreshold(uint32_t numusers);
         asset get_stake_requirement(char account_type);
         iteration getclaimiteration();
         bool eligible_to_claim(const name& claimant, iteration this_iteration);
         uint32_t updateclaimeventcount();
         uint16_t getfreedaomultiplier(uint32_t claimevents);
         void store_unregistered_stake(asset next_user_stake_requirement);
         float get_vested_proportion();
         void tick_process(std::string trigger);
         void hourly_process(std::string trigger);
         void daily_process(std::string trigger);
         void weekly_process(std::string trigger);
         void update_unvest_percentage();
         void update_stake_requirements(uint32_t numusers);
         void record_deposit(uint64_t iteration_number, asset amount);
         char get_account_type(name user);
         void request_stake_refund(name user, asset amount, time_point_sec staked_time);
         void refund_stakes();
         void refund_stake(name user, asset amount);
         void set_iteration_number();

   };
   /** @}*/ // end of @defgroup freeos freeos contract
