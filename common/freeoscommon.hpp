#include <eosio/eosio.hpp>
#include <string>

#define _STRINGIZE(x) #x
#define STRINGIZE(x) _STRINGIZE(x)

std::string freeos_acct = STRINGIZE(FREEOS);
std::string freeosconfig_acct = STRINGIZE(FREEOSCONFIG);
std::string freeostokens_acct = STRINGIZE(FREEOSTOKENS);
std::string freedao_acct = STRINGIZE(DIVIDEND);

const std::string verification_contract = "eosio.proton";   // contains the usersinfo table

// currency symbol for network
const std::string SYSTEM_CURRENCY_CODE = "XPR";
const std::string NON_EXCHANGEABLE_CURRENCY_CODE = "OPTION";
const std::string EXCHANGEABLE_CURRENCY_CODE = "FREEOS";

// hard floor for the target exchange rate - it can never go below this
const double HARD_EXCHANGE_RATE_FLOOR = 0.0167;

// common error/notification messages
const std::string msg_freeos_system_not_available = "Freeos system is not currently operating. Please try later";
const std::string msg_account_not_registered = "Account is not registered with freeos";

// define for test/debug build
#define TEST_BUILD

 struct price {
   double    currentprice;
   double    targetprice;

   uint64_t primary_key() const { return 0; } // return a constant (0 in this case) to ensure a single-row table
 };

 using exchange_index = eosio::multi_index<"exchangerate"_n, price>;
