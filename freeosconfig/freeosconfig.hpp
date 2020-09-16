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
     * upsert action
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
    void upsert(
            name virtualtable,
            name paramname,
            std::string value
          );

    /**
     * erase action.
     *
     * @details This action deletes a parameter from the 'parameters' table.
     *
     * @param parameter - the name of the parameter.
     *
     * @pre requires permission of the contract account
     */
    [[eosio::action]]
    void erase ( name paramname );

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
     * configcheck action.
     *
     * @details This action is intended for testers. It displays the values associated with the threshold in the 'stakereqs' table and the parameter in the 'parameters' table.
     *
     * @param threshold - the lower bound of the range of user population (e.g. if the band is for 5,000-10,000 users, then enter 5000)
     * @param paramname - the name of the parameter
     *
     */
    [[eosio::action]]
    void configcheck(uint64_t threshold, name paramname);

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

};
/** @}*/ // end of @defgroup freeosconfig freeosconfig contract
