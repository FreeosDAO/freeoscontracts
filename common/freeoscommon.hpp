// common include definitions
#include <string>

// account names
const std::string freeos_acct = "freeos333333";
const std::string freeosconfig_acct = "freeosconfig";
const std::string freedao_acct = "freeosdivide";
const std::string verification_contract = "eosio.proton";   // contains the usersinfo table

// currency symbol for network
const std::string CURRENCY_SYMBOL_CODE = "XPR";

// amount of time to hold user stake for before unstaking is permitted
const uint32_t STAKE_HOLD_TIME_SECONDS = 18000;  // 18000 = 5 hours / one week = 604800

// hard floor for the target exchange rate - it can never go below this
const double HARD_EXCHANGE_RATE_FLOOR = 0.0167;

// number of 'unstakes' a user can perform
const uint64_t MAX_UNSTAKES_TO_PERFORM = 2;

// common error/notification messages
const std::string msg_freeos_system_not_available = "Freeos system is not currently operating. Please try later";
const std::string msg_account_not_registered = "Account is not registered with freeos";
