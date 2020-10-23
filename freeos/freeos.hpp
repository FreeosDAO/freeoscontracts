
     // *******************************
     // accounts_t table - vRam enabled
     // *******************************

     // account and currency_stats structs defined in dappservices/dappservices.hpp

     typedef dapp::multi_index<"accounts"_n, account> accounts_t;

     typedef eosio::multi_index<".accounts"_n, account> accounts_t_v_abi;
     TABLE shardbucket {
         std::vector<char> shard_uri;
         uint64_t shard;
         uint64_t primary_key() const { return shard; }
     };
     typedef eosio::multi_index<"accounts"_n, shardbucket> accounts_t_abi;


     // ***********
     // stats table
     // ***********

     typedef eosio::multi_index< "stat"_n, currency_stats > stats;


     // **************************
     // users table - vRam enabled
     // **************************

     struct [[eosio::table]] user {
       asset stake;                    // how many EOS tokens staked
       char account_type;              // user has EOS-login account, Dapp Account-login or other
       asset stake_requirement;        // the number of tokens the user is required to stake
       time_point_sec registered_time; // when the user was registered
       time_point_sec staked_time;     // when the user staked their tokens (staked tokens have a time lock)

       uint64_t primary_key() const {return stake.symbol.code().raw();}
     };

     // typedef eosio::multi_index<"users"_n, user> user_index;

     // new
     typedef dapp::multi_index<"users"_n, user> user_index_t;

     typedef eosio::multi_index<".users"_n, user> user_index_t_v_abi;

     typedef eosio::multi_index<"users"_n, shardbucket> user_index_t_abi;





     // **********************
     // user counter singleton
     // **********************

     struct [[eosio::table]] count {
       uint32_t  count;
     } ct;

     using user_singleton = eosio::singleton<"usercount"_n, count>;


     // ****************
     // parameters table
     // ****************

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


     // ***************
     // stakereqs table
     // ***************

     struct [[eosio::table]] stakerequire {
       uint64_t    threshold;
       uint32_t    requirement_e;
       uint32_t    requirement_d;
       uint32_t    requirement_v;
       uint32_t    requirement_x;

       uint64_t primary_key() const { return threshold;}
     };

     using stakereq_index = eosio::multi_index<"stakereqs"_n, stakerequire>;


     // ***********
     // weeks table
     // ***********

     struct [[eosio::table]] week {
       uint64_t    week_number;
       uint32_t    start;
       std::string start_date;
       uint32_t    end;
       std::string end_date;
       uint16_t    claim_amount;
       uint16_t    tokens_required;
       uint16_t    freedao_payment;

       uint64_t primary_key() const { return week_number; }
     };

     using week_index = eosio::multi_index<"weeks"_n, week>;


     // ******************************************
     // claims table - scoped on user account name
     // ******************************************

     struct [[eosio::table]] claimevent {
       uint64_t   week_number;
       uint32_t   claim_time;

       uint64_t primary_key() const { return week_number; }
     };

     using claim_index = eosio::multi_index<"claims"_n, claimevent>;




     // ********************************************
     // vaccount-compatible action parameter structs
     // ********************************************

     // equivalent to: reguser(const name& user, const std::string account_type)
     struct reguser_action {
         name vaccount;
         name user;
         std::string account_type;

         EOSLIB_SERIALIZE(reguser_action, (vaccount)(user)(account_type))
     };

     // equivalent to: unstake(const name& user)
     struct unstake_action {
         name vaccount;
         name user;

         EOSLIB_SERIALIZE(unstake_action, (vaccount)(user))
     };

     // equivalent to: transfer( const name& from, const name& to, const asset& quantity, const string&  memo )
     struct transfer_action {
         name vaccount;
         name from;
         name to;
         asset quantity;
         std::string memo;

         EOSLIB_SERIALIZE(transfer_action, (vaccount)(from)(to)(quantity)(memo))
     };

     // equivalent to: claim( const name& claimant )
     struct claim_action {
         name vaccount;
         name user;

         EOSLIB_SERIALIZE(claim_action, (vaccount)(user))
     };
