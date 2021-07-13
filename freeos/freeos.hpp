#pragma once

#include "../common/freeoscommon.hpp"
#include <eosio/eosio.hpp>

namespace eosiosystem {
class system_contract;
}

namespace freedao {
using namespace eosio;
using std::string;

enum registration_status {
  registered_already,
  registered_success,
};

/**
 * @defgroup freeos freeos contract
 * @ingroup eosiocontracts
 *
 * freeos contract
 *
 * @details freeos contract defines the structures and actions that allow users
 * to manage the OPTION currency and control the AirClaim.
 * @{
 */
class[[eosio::contract("freeos")]] freeos : public contract {
public:
  using contract::contract;

  /**
   * version action.
   *
   * @details Prints the version of this contract.
   */
  [[eosio::action]] void version();

  /**
   * tick action.
   *
   * @details Triggers 'background' actions.
   */
  [[eosio::action]] void tick();

  /**
   * cron action.
   *
   * @details This action is run by the Proton CRON service.
   */
  [[eosio::action]] void cron();

  /**
   * reguser action.
   *
   * @details Creates a record for the user in the 'stakereqs' table.
   * @param user - the account to be registered,
   *
   * @pre Requires permission of the contract account
   *
   * If validation is successful a new entry in 'stakereqs' table is created for
   * the user account.
   */
  [[eosio::action]] void reguser(const name &user);

  /**
   * stake action.
   *
   * @details Adds a record of staked EOS tokens in the user account record in
   * the 'stakereqs' table.\n
   * @details This action is automatically invoked on the freeos account
   * receiving EOS from the user.\n
   *
   * @param user - the account sending the EOS tokens to freeos,
   * @param to - the 'freeos' account,
   * @param quantity - the asset, i.e. an amount of EOS which is transferred to
   * the freeos account,
   * @param memo - a memo describing the transaction
   *
   * @pre The user account must be previously registered
   * @pre The asset must be equal to the stake requirement for the user
   *
   * If validation is successful the record for the user account is updated to
   * record the amount staked and the date/time of the receipt.
   */
#ifdef TEST_BUILD
  [[eosio::on_notify("eosio.token::transfer")]] void stake(
      name user, name to, asset quantity, std::string memo);
#else
  [[eosio::on_notify("xtokens::transfer")]] void stake(
      name user, name to, asset quantity, std::string memo);
#endif

  /**
   * unstake action.
   *
   * @details Transfer staked EOS back to the user if the waiting time of
   * <holding period> has elapsed since staking.
   * @details Updates the record of staked EOS tokens in the user account record
   * in the 'stakereqs' table.\n
   *
   * @param user - the account to transfer the previously EOS tokens to,
   *
   * @pre The user account must be previously registered and has a non-zero
   * balance of staked EOS
   *
   * If transfer is successful the record for the user account is updated to set
   * the amount staked and stake time/date to zero.
   */
  [[eosio::action]] void unstake(const name &user);

  /**
   * Create action.
   *
   * @details Allows `issuer` account to create a token in supply of
   * `maximum_supply`.
   * @param issuer - the account that creates the token,
   * @param maximum_supply - the maximum supply set for the token created.
   *
   * @pre Token symbol has to be valid,
   * @pre Token symbol must not be already created,
   * @pre maximum_supply has to be smaller than the maximum supply allowed by
   * the system: 1^62 - 1.
   * @pre Maximum supply must be positive;
   *
   * If validation is successful a new entry in statstable for token symbol
   * scope gets created.
   */
  [[eosio::action]] void create(const name &issuer,
                                const asset &maximum_supply);

  /**
   * Transfer function.
   *
   * @details Allows `from` account to transfer to `to` account the `quantity`
   * tokens. One account is debited and the other is credited with quantity
   * tokens. Cannot be called by users because OPTIONs are not transferable.
   *
   * @param from - the account to transfer from,
   * @param to - the account to be transferred to,
   * @param quantity - the quantity of tokens to be transferred,
   * @param memo - the memo string to accompany the transaction.
   */
  void transfer(const name &owner, const name &to, const asset &quantity,
                const string &memo);

  /**
   * Convert action.
   *
   * @details Converts an amount of non-exchangeable currency to exchangeable
   * currency. The non-exchangeable currency is burned and an equivalent amount
   * of exchangeable currency issued and transferred.
   *
   * @param owner - the account to convert from,
   * @param quantity - the quantity of tokens to be converted.
   */
  [[eosio::action]] void convert(const name &owner, const asset &quantity);

  /**
   * claim action.
   *
   * @details This action is run by the user to claim this iteration's
   * allocation of freeos tokens.
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
  [[eosio::action]] void claim(const name &user);

  /**
   * unvest action.
   *
   * @details This action is run by the user to release this iteration's
   * allocation of vested freeos tokens.
   *
   * @param owner - the user account to execute the unvest action for.
   *
   * @pre Requires authorisation of the user account
   *
   */
  [[eosio::action]] void unvest(const name &user);

  /**
   * Get supply method.
   *
   * @details Gets the supply for token `sym_code`, created by
   * `token_contract_account` account.
   *
   * @param token_contract_account - the account to get the supply for,
   * @param sym_code - the symbol to get the supply for.
   */
  static asset get_supply(const name &token_contract_account,
                          const symbol_code &sym_code) {
    stats statstable(token_contract_account, sym_code.raw());
    const auto &st = statstable.get(sym_code.raw());
    return st.supply;
  }

  /**
   * Get balance method.
   *
   * @details Get the balance for a token `sym_code` created by
   * `token_contract_account` account, for account `owner`.
   *
   * @param token_contract_account - the token creator account,
   * @param owner - the account for which the token balance is returned,
   * @param currency_symbol - the token for which the balance is returned.
   */
  static asset get_balance(const name &token_contract_account,
                           const name &owner, const symbol &currency_symbol) {
    asset return_balance =
        asset(0, currency_symbol); // default if accounts record does not exist

    accounts accountstable(token_contract_account, owner.value);
    const auto &ac = accountstable.find(currency_symbol.code().raw());
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
  [[eosio::action]] void depositclear(uint64_t iteration_number);

  /**
   * Cancel unstake request action.
   *
   * @details Delete the unstake request from the unstakes table.
   *
   * @param user - identifies the user's unstake record to be deleted.
   *
   */
  [[eosio::action]] void unstakecncl(const name &user);

  /**
   * Reverify account_type for user action.
   *
   * @details Checks the verification table to see if user has been verified,
   * @details Changes account_type in the user record accordingly.
   *
   * @param user - identifies the user to be verified.
   *
   */
  [[eosio::action]] void reverify(name user);

  /**
   * allocate - an action wrapper for the transfer function
   *
   * @details Checks that the 'from' user belongs to the transferers whitelist
   * and then calls the transfer function.
   *
   * @param from      - the account to be deducted
   * @param to        - the account to be incremented
   * @param quantity  - the amount to be transferred
   * @param memo      - the memo accompanying the transaction
   *
   */
  [[eosio::action]] void allocate(const name &from, const name &to,
                                  const asset &quantity, const string &memo);

  /**
   * mint - an action wrapper for the issue function
   *
   * @details Checks that the 'minter' user belongs to a whitelist and then
   * calls the issue function.
   *
   * @param minter    - the account calling the mint action
   * @param to        - the account to be incremented
   * @param quantity  - the amount to be issued
   * @param memo      - the memo accompanying the transaction
   *
   */
  [[eosio::action]] void mint(const name &minter, const name &to,
                              const asset &quantity, const string &memo);

  /**
   * burn - an action wrapper for the retire function
   *
   * @details Checks that the 'burner' user belongs to a whitelist and then
   * calls the retire function.
   *
   * @param burner    - the account calling the burn action
   * @param to        - the account to be incremented
   * @param quantity  - the amount to be issued
   * @param memo      - the memo accompanying the transaction
   *
   */
  [[eosio::action]] void burn(const name &burner, const asset &quantity,
                              const string &memo);

private:
  void issue(const name &to, const asset &quantity, const string &memo);
  void retire(const asset &quantity, const string &memo);
  void sub_balance(const name &owner, const asset &value);
  void add_balance(const name &owner, const asset &value,
                   const name &ram_payer);

  registration_status register_user(const name &user);

  bool check_master_switch();
  uint32_t get_cached_iteration();
  bool checkschedulelogging();
  uint32_t get_stake_requirement(char account_type);
  iteration get_claim_iteration();
  bool eligible_to_claim(const name &claimant, iteration this_iteration);
  uint32_t update_claim_event_count();
  double get_freedao_multiplier(uint32_t claimevents);
  float get_vested_proportion();
  void update_unvest_percentage();
  void record_deposit(uint64_t iteration_number, asset amount);
  char get_account_type(name user);
  void request_stake_refund(name user, asset amount);
  void refund_stakes();
  void refund_stake(name user, asset amount);
};
/** @}*/ // end of @defgroup freeos freeos contract
} // namespace freedao