#include <eosio/eosio.hpp>

using namespace eosio;

/**
 * @defgroup freeosconfig freeosconfig contract
 * @ingroup eosiocontracts
 *
 * freeosconfig contract
 *
 * @details Defines the structures and tables that allow users to maintain configuration tables.
 * @{
 */
class [[eosio::contract("freeosconfig")]] freeosconfig : public eosio::contract {
  public:
    /**
     * @details contract constructor
     */
    freeosconfig(name receiver, name code, datastream<const char*> ds):contract(receiver, code, ds) {}


    /**
     * paramupsert action
     *
     * @details This action creates a new parameter or modifies an existing parameter in the 'parameters' table.
     *
     * @param virtualtable - a way to group related parameters together, accessed as a secondary index,
     * @param parameter - the name of the parameter. Value must be unique as it forms the primary index,
     * @param value - the value of the parameter
     *
     * @pre requires permission of the contract account
     *
     */
    [[eosio::action]]
    void paramupsert(
            name virtualtable,
            name paramname,
            std::string value
          );

    /**
     * paramerase action.
     *
     * @details This action deletes a parameter from the 'parameters' table.
     *
     * @param parameter - the name of the parameter.
     *
     * @pre requires permission of the contract account
     */
    [[eosio::action]]
    void paramerase ( name paramname );

    /**
     * stakeupsert action.
     *
     * @details This action creates a new entry, or modifies an existing entry, in the 'stakereqs' table.
     *
     * @param threshold - the lower bound of the range of user population (e.g. if 5,000-10,000 users require a stake of 10 EOS, then enter 5000). This is the promary key of the table,
     * @param value_e - the amount of EOS stake required for an EOS wallet user,
     * @param value_d - the amount of EOS stake required for a Dapp Account user,
     * @param value_e - the amount of EOS stake required for a Voice verified user,
     * @param value_e - (reserved for future use, enter 0)
     *
     * @pre requires permission of the contract account
     */
    [[eosio::action]]
    void stakeupsert(uint64_t threshold,
    uint32_t  value_e,
    uint32_t  value_d,
    uint32_t  value_v,
    uint32_t  value_x);

    /**
     * stakeerase action.
     *
     * @details This action deletes an entry from the 'stakereqs' table.
     *
     * @param threshold - the lower bound of the range of user population (e.g. if the band is for 5,000-10,000 users, then enter 5000)
     *
     * @pre requires permission of the contract account
     */
    [[eosio::action]]
    void stakeerase(uint64_t threshold);


    /**
     * weekupsert action.
     *
     * @details This action creates a new entry, or modifies an existing entry, in the 'weeks' table.
     *
     * @param week_number - the airclaim week number
     * @param start - string representing the start of week in YYYY-MM-DD HH:MM:SS format,
     * @param end - string representing the end of week in YYYY-MM-DD HH:MM:SS format
     * @param claim_amount - the amount of FREEOS a user can claim in the week
     * @param claim_amount - the amount of FREEOS a user must hold before claiming
     * @param freedao_payment - the amount of FREEOS that is contributed to the freedao account
     *
     * @pre requires permission of the contract account
     */
    [[eosio::action]]
    void weekupsert(uint64_t week_number,
    std::string start,
    std::string end,
    uint16_t  claim_amount,
    uint16_t  tokens_required,
    uint16_t  freedao_payment);

    /**
     * getweek action.
     *
     * @details For testing purposes. This action prints a record from the 'weeks' table.
     *
     * @param week_number - the week number
     *
     * @pre requires permission of the contract account
     */
    [[eosio::action]]
    void getweek(uint64_t week_number);

    /**
     * weekerase action.
     *
     * @details This action deletes a record from the 'weeks' table.
     *
     * @param week_number - the week number
     *
     * @pre requires permission of the contract account
     */
    [[eosio::action]]
    void weekerase(uint64_t week_number);

    /**
     * getconfig action.
     *
     * @details This action is intended for testers. It displays the values associated with the threshold in the 'stakereqs' table and the parameter in the 'parameters' table.
     *
     * @param threshold - the lower bound of the range of user population (e.g. if the band is for 5,000-10,000 users, then enter 5000)
     * @param paramname - the name of the parameter
     *
     */
    [[eosio::action]]
    void getconfig(uint64_t threshold, name paramname);

    /**
     * getthreshold action.
     *
     * @details This action is intended for testing the 'stakereqs' table. It displays the stake requirement for the number of users and the account type.
     *
     * @param numusers - the lower bound of the range of user population (e.g. if the band is for 5,000-10,000 users, then enter 5000)
     * @param account_type - "e" for EOS wallet user, "d" for Dapp Account user, "v" for Voice verified user.
     *
     */
    [[eosio::action]]
    void getthreshold(uint64_t numusers, std::string account_type);


  private:
    // ************************************
    // parameter table

    struct [[eosio::table]] parameter {
      name virtualtable;
      name paramname;
      std::string value;

      uint64_t primary_key() const { return paramname.value;}
      uint64_t get_secondary() const {return virtualtable.value;}
    };

    using parameter_index = eosio::multi_index<"parameters"_n, parameter,
    indexed_by<"virtualtable"_n, const_mem_fun<parameter, uint64_t, &parameter::get_secondary>>
    >;


    // stake requirement table

    struct [[eosio::table]] stakerequire {
      uint64_t    threshold;
      uint32_t    requirement_e;
      uint32_t    requirement_d;
      uint32_t    requirement_v;
      uint32_t    requirement_x;

      uint64_t primary_key() const { return threshold;}
    };

    using stakereq_index = eosio::multi_index<"stakereqs"_n, stakerequire>;


    // freeos airclaim week calendar

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

    // ************************************

    // helper functions
    uint32_t parsetime(std::string datetime);
    uint32_t GetTimeStamp(  int year, int month, int day,
                            int hour, int minute, int second);

};
/** @}*/ // end of @defgroup freeosconfig freeosconfig contract
