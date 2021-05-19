#pragma once

#include <eosio/eosio.hpp>

namespace freedao {

using namespace eosio;

/**
 * @defgroup freeosconfig freeosconfig contract
 * @ingroup eosiocontracts
 *
 * freeosconfig contract
 *
 * @details Defines the structures and tables that allow users to maintain
 * configuration tables.
 * @{
 */
class[[eosio::contract("freeosconfig")]] freeosconfig : public eosio::contract {
public:
  /**
   * @details contract constructor
   */
  freeosconfig(name receiver, name code, datastream<const char *> ds)
      : contract(receiver, code, ds){}

  /**
   * version action.
   *
   * @details Prints the version of this contract.
   */
  [[eosio::action]] void
  version();

  /**
   * paramupsert action
   *
   * @details This action creates a new parameter or modifies an existing
   * parameter in the 'parameters' table.
   *
   * @param virtualtable - a way to group related parameters together, accessed
   * as a secondary index,
   * @param parameter - the name of the parameter. Value must be unique as it
   * forms the primary index,
   * @param value - the value of the parameter
   *
   * @pre requires permission of the contract account
   *
   */
  [[eosio::action]] void paramupsert(name virtualtable, name paramname,
                                     std::string value);

  /**
   * paramerase action.
   *
   * @details This action deletes a parameter from the 'parameters' table.
   *
   * @param parameter - the name of the parameter.
   *
   * @pre requires permission of the contract account
   */
  [[eosio::action]] void paramerase(name paramname);

  /**
   * stakeupsert action.
   *
   * @details This action creates a new entry, or modifies an existing entry, in
   * the 'stakereqs' table.
   *
   * @param threshold - the lower bound of the range of user population (e.g. if
   * 5,000-10,000 users require a stake of 10 EOS, then enter 5000). This is the
   * promary key of the table,
   * @param value_a - the amount of EOS stake required for a 'type a' user,
   * @param value_b - the amount of EOS stake required for a 'type b' user,
   * @param value_c - the amount of EOS stake required for a 'type c' user,
   * @param value_d - the amount of EOS stake required for a 'type d' user,
   * @param value_e - the amount of EOS stake required for a 'type e' user,
   * @param value_u - the amount of EOS stake required for a 'type u' user,
   * @param value_v - the amount of EOS stake required for a 'type v' user,
   * @param value_w - the amount of EOS stake required for a 'type w' user,
   * @param value_x - the amount of EOS stake required for a 'type x' user,
   * @param value_y - the amount of EOS stake required for a 'type y' user,
   *
   * @pre requires permission of the contract account
   */
  [[eosio::action]] void stakeupsert(
      uint64_t threshold, uint32_t value_a, uint32_t value_b, uint32_t value_c,
      uint32_t value_d, uint32_t value_e, uint32_t value_u, uint32_t value_v,
      uint32_t value_w, uint32_t value_x, uint32_t value_y);

  /**
   * stakeerase action.
   *
   * @details This action deletes an entry from the 'stakereqs' table.
   *
   * @param threshold - the lower bound of the range of user population (e.g. if
   * the band is for 5,000-10,000 users, then enter 5000)
   *
   * @pre requires permission of the contract account
   */
  [[eosio::action]] void stakeerase(uint64_t threshold);

  /**
   * iterupsert action.
   *
   * @details This action creates a new entry, or modifies an existing entry, in
   * the 'iterations' table.
   *
   * @param iteration_number - the airclaim iteration number
   * @param start - string representing the start of iteration in
   * YYYY-MM-DDTHH:MM:SS format,
   * @param end - string representing the end of iteration in
   * YYYY-MM-DDTHH:MM:SS format
   * @param claim_amount - the amount of FREEOS a user can claim in the
   * iteration
   * @param tokens_required - the amount of FREEOS a user must hold before
   * claiming
   *
   * @pre requires permission of the contract account
   */
  [[eosio::action]] void iterupsert(uint32_t iteration_number, time_point start,
                                    time_point end, uint16_t claim_amount,
                                    uint16_t tokens_required);

  /**
   * itererase action.
   *
   * @details This action deletes a record from the 'iterations' table.
   *
   * @param iteration_number - the iteration number
   *
   * @pre requires permission of the freeosconfig contract account
   */
  [[eosio::action]] void itererase(uint32_t iteration_number);

  /**
   * iterclear action.
   *
   * @details This action deletes a record from the 'iterations' table.
   *
   * @param iteration_number - the iteration number
   *
   * @pre requires permission of the freeos contract account
   */
  [[eosio::action]] void iterclear(uint32_t iteration_number);

  /**
   * currentrate action.
   *
   * @details This action creates a new record, or modifies the existing
   * (single) record, in the 'exchangerate' table. Sets the usdprice field.
   *
   * @param price - the current US Dollar price for 1 FREEOS
   *
   * @pre requires permission of the contract account
   */
  [[eosio::action]] void currentrate(double price);

  /**
   * targetrate action.
   *
   * @details This action creates a new record, or modifies the existing
   * (single) record, in the 'exchangerate' table. Sets the targetrate field.
   *
   * @param price - the target US Dollar price for 1 FREEOS
   *
   * @pre requires permission of the contract account
   */
  [[eosio::action]] void targetrate(double price);

  /**
   * rateerase action.
   *
   * @details This action deletes the (single) record from the 'exchangerate'
   * table.
   *
   * @pre requires permission of the contract account
   */
  [[eosio::action]] void rateerase();

  /**
   * transfadd action.
   *
   * @details This action adds an account to the transferers table, a whitelist
   * of who can call the OPTION transfer function.
   *
   * @param account - the name of the account to be added
   *
   * @pre requires permission of the contract account
   */
  [[eosio::action]] void transfadd(name account);

  /**
   * transferase action.
   *
   * @details This action deletes an account to the transferers table, a
   * whitelist of who can call the OPTION transfer function.
   *
   * @param account - the name of the account to be deleted
   *
   * @pre requires permission of the contract account
   */
  [[eosio::action]] void transferase(name account);

  /**
   * minteradd action.
   *
   * @details This action adds an account to the minters table, a whitelist
   * of who can call the OPTION issue function.
   *
   * @param account - the name of the account to be added
   *
   * @pre requires permission of the contract account
   */
  [[eosio::action]] void minteradd(name account);

  /**
   * mintererase action.
   *
   * @details This action deletes an account to the minters table, a
   * whitelist of who can call the OPTION issue function.
   *
   * @param account - the name of the account to be deleted
   *
   * @pre requires permission of the contract account
   */
  [[eosio::action]] void mintererase(name account);

  /**
   * burneradd action.
   *
   * @details This action adds an account to the burners table, a whitelist
   * of who can call the OPTION retire function.
   *
   * @param account - the name of the account to be added
   *
   * @pre requires permission of the contract account
   */
  [[eosio::action]] void burneradd(name account);

  /**
   * burnererase action.
   *
   * @details This action deletes an account to the issuerers table, a
   * whitelist of who can call the OPTION retire function.
   *
   * @param account - the name of the account to be deleted
   *
   * @pre requires permission of the contract account
   */
  [[eosio::action]] void burnererase(name account);

  // ********* simulated eosio.proton actions for populating usersinfo table
  // ***********
  [[eosio::action]] void userverify(name acc, name verifier, bool verified);

  [[eosio::action]] void addkyc(name acc, name kyc_provider,
                                std::string kyc_level, uint64_t kyc_date);

  // ***********************************************************************************

private:
  // helper functions
  void iter_delete(uint32_t iteration_number);
};
/** @}*/ // end of @defgroup freeosconfig freeosconfig contract
} // namespace freedao