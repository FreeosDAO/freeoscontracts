#include "freeosconfig.hpp"
#include "../common/freeoscommon.hpp"

// using namespace eosio;
namespace freedao {

const std::string VERSION = "0.112";

// ACTION
void freeosconfig::version() {
  std::string version_message = freeos_acct + "/" + freeosconfig_acct + "/" +
                                freeostokens_acct + "/" + freedao_acct +
                                " version = " + VERSION;

  check(false, version_message);
}

// ACTION
void freeosconfig::paramupsert(name virtualtable, name paramname,
                               std::string value) {

  require_auth(_self);
  parameters_index parameters_table(get_self(), get_self().value);
  auto parameter_iterator = parameters_table.find(paramname.value);

  // check if the parameter is in the table or not
  if (parameter_iterator == parameters_table.end()) {
    // the parameter is not in the table, so insert
    parameters_table.emplace(_self, [&](auto &parameter) {
      parameter.virtualtable = virtualtable;
      parameter.paramname = paramname;
      parameter.value = value;
    });

  } else {
    // the parameter is in the table, so update
    parameters_table.modify(parameter_iterator, _self, [&](auto &parameter) {
      parameter.virtualtable = virtualtable;
      parameter.value = value;
    });
  }
}

// erase parameter from the table
// ACTION
void freeosconfig::paramerase(name paramname) {
  require_auth(_self);

  parameters_index parameters_table(get_self(), get_self().value);
  auto parameter_iterator = parameters_table.find(paramname.value);

  // check if the parameter is in the table or not
  check(parameter_iterator != parameters_table.end(),
        "config parameter does not exist");

  // the parameter is in the table, so delete
  parameters_table.erase(parameter_iterator);
}

// ACTION
void freeosconfig::currentrate(double price) {

  // check if the exchange account is calling this action, or the contract itself
  parameters_index parameters_table(get_self(), get_self().value);
  auto parameter_iterator = parameters_table.find(name("exchangeacc").value);
  if (parameter_iterator != parameters_table.end()) {
    require_auth(name(parameter_iterator->value));
  } else {
    require_auth(_self);
  }

  check(price > 0.0, "current rate must be positive");

  exchange_index rates_table(get_self(), get_self().value);
  auto rate_iterator = rates_table.begin();

  // check if the rate exists in the table
  if (rate_iterator == rates_table.end()) {
    // the rate is not in the table, so insert
    rates_table.emplace(_self, [&](auto &rate) { rate.currentprice = price; });

  } else {
    // the rate is in the table, so update
    rates_table.modify(rate_iterator, _self,
                       [&](auto &rate) { rate.currentprice = price; });
  }
}

// ACTION
void freeosconfig::targetrate(double exchangerate) {

  // require_auth(_self); // v0.111
  // v0.112 change - check if the locking contract account is calling this action, or the contract itself
  parameters_index parameters_table(get_self(), get_self().value);
  auto parameter_iterator = parameters_table.find(name("lockingacc").value);
  if (parameter_iterator != parameters_table.end()) {
    require_auth(name(parameter_iterator->value));
  } else {
    require_auth(_self);
  }
  // end of v0.112 change

  check(exchangerate > 0.0, "target rate must be positive");

  double new_exchangerate = exchangerate;

  // ensure it is not set below the hardcoded floor
  if (new_exchangerate < HARD_EXCHANGE_RATE_FLOOR) {
    new_exchangerate = HARD_EXCHANGE_RATE_FLOOR;
  }

  exchange_index rates_table(get_self(), get_self().value);
  auto rate_iterator = rates_table.begin();

  // check if the rate exists in the table
  if (rate_iterator == rates_table.end()) {
    // the rate is not in the table, so insert
    rates_table.emplace(
        _self, [&](auto &rate) { rate.targetprice = new_exchangerate; });

  } else {
    // the rate is in the table, so update
    rates_table.modify(rate_iterator, _self, [&](auto &rate) {
      rate.targetprice = new_exchangerate;
    });
  }
}

// erase rate from the table
// ACTION
void freeosconfig::rateerase() {
  require_auth(_self);

  exchange_index rates_table(get_self(), get_self().value);
  auto rate_iterator = rates_table.begin();

  // check if the rate is in the table
  check(rate_iterator != rates_table.end(), "rate record does not exist");

  // the rate record is in the table, so delete
  rates_table.erase(rate_iterator);
}

// stake requirements table actions

// ACTION
void freeosconfig::stakeupsert(uint64_t threshold, uint32_t value_a,
                               uint32_t value_b, uint32_t value_c,
                               uint32_t value_d, uint32_t value_e,
                               uint32_t value_u, uint32_t value_v,
                               uint32_t value_w, uint32_t value_x,
                               uint32_t value_y) {

  require_auth(_self);
  stakereq_index stakereqs_table(get_self(), get_self().value);
  auto stakereq_iterator = stakereqs_table.find(threshold);

  // check if the threshold is in the table or not
  if (stakereq_iterator == stakereqs_table.end()) {
    // the threshold is not in the table, so insert
    stakereqs_table.emplace(_self, [&](auto &stakereq) {
      stakereq.threshold = threshold;
      stakereq.requirement_a = value_a;
      stakereq.requirement_b = value_b;
      stakereq.requirement_c = value_c;
      stakereq.requirement_d = value_d;
      stakereq.requirement_e = value_e;
      stakereq.requirement_u = value_u;
      stakereq.requirement_v = value_v;
      stakereq.requirement_w = value_w;
      stakereq.requirement_x = value_x;
      stakereq.requirement_y = value_y;
    });

  } else {
    // the threshold is in the table, so update
    stakereqs_table.modify(
        stakereq_iterator, _self,
        [&](auto &stakereq) { // second argument was "freeosconfig"_n
          stakereq.threshold = threshold;
          stakereq.requirement_a = value_a;
          stakereq.requirement_b = value_b;
          stakereq.requirement_c = value_c;
          stakereq.requirement_d = value_d;
          stakereq.requirement_e = value_e;
          stakereq.requirement_u = value_u;
          stakereq.requirement_v = value_v;
          stakereq.requirement_w = value_w;
          stakereq.requirement_x = value_x;
          stakereq.requirement_y = value_y;
        });
  }
}

// erase stake requirement from the table
// ACTION
void freeosconfig::stakeerase(uint64_t threshold) {
  require_auth(_self);

  stakereq_index stakereqs_table(get_self(), get_self().value);
  auto stakereq_iterator = stakereqs_table.find(threshold);

  // check if the parameter is in the table
  check(stakereq_iterator != stakereqs_table.end(),
        "stake requirement record does not exist");

  // the parameter is in the table, so delete
  stakereqs_table.erase(stakereq_iterator);
}

// add an account to the transferers whitelist
// ACTION
void freeosconfig::transfadd(name account) {
  require_auth(_self);

  transferers_index transferers_table(get_self(), get_self().value);
  transferers_table.emplace(
      _self, [&](auto &transferer) { transferer.account = account; });
}

// erase an account from the transferers whitelist
// ACTION
void freeosconfig::transferase(name account) {
  require_auth(_self);

  transferers_index transferers_table(get_self(), get_self().value);
  auto transferer_iterator = transferers_table.find(account.value);

  // check if the account is in the table
  check(transferer_iterator != transferers_table.end(),
        "account is not in the transferers table");

  // the account is in the table, so delete
  transferers_table.erase(transferer_iterator);
}

// add an account to the issuers whitelist
// ACTION
void freeosconfig::minteradd(name account) {
  require_auth(_self);

  minters_index minters_table(get_self(), get_self().value);
  minters_table.emplace(_self, [&](auto &issuer) { issuer.account = account; });
}

// erase an account from the issuers whitelist
// ACTION
void freeosconfig::mintererase(name account) {
  require_auth(_self);

  minters_index minters_table(get_self(), get_self().value);
  auto minter_iterator = minters_table.find(account.value);

  // check if the account is in the table
  check(minter_iterator != minters_table.end(),
        "account is not in the minters table");

  // the account is in the table, so delete
  minters_table.erase(minter_iterator);
}

// add an account to the burners whitelist
// ACTION
void freeosconfig::burneradd(name account) {
  require_auth(_self);

  burners_index burners_table(get_self(), get_self().value);
  burners_table.emplace(_self, [&](auto &burner) { burner.account = account; });
}

// erase an account from the burners whitelist
// ACTION
void freeosconfig::burnererase(name account) {
  require_auth(_self);

  burners_index burners_table(get_self(), get_self().value);
  auto burner_iterator = burners_table.find(account.value);

  // check if the account is in the table
  check(burner_iterator != burners_table.end(),
        "account is not in the burners table");

  // the account is in the table, so delete
  burners_table.erase(burner_iterator);
}

// upsert an iteration into the iterations table
// ACTION
void freeosconfig::iterupsert(uint32_t iteration_number, time_point start,
                              time_point end, uint16_t claim_amount,
                              uint16_t tokens_required) {

  require_auth(_self);

  check(iteration_number != 0, "iteration number must not be 0");

  iterations_index iterations_table(get_self(), get_self().value);
  auto iteration_iterator = iterations_table.find(iteration_number);

  // check if the iteration is in the table or not
  if (iteration_iterator == iterations_table.end()) {
    // the iteration is not in the table, so insert
    iterations_table.emplace(_self, [&](auto &iteration) {
      iteration.iteration_number = iteration_number;
      iteration.start = start;
      iteration.end = end;
      iteration.claim_amount = claim_amount;
      iteration.tokens_required = tokens_required;
    });
  } else {
    // the iteration is in the table, so update
    iterations_table.modify(iteration_iterator, _self, [&](auto &iteration) {
      iteration.iteration_number = iteration_number;
      iteration.start = start;
      iteration.end = end;
      iteration.claim_amount = claim_amount;
      iteration.tokens_required = tokens_required;
    });
  }
}

// erase an iteration record from the iterations table - contract action
// ACTION
void freeosconfig::itererase(uint32_t iteration_number) {
  require_auth(_self);

  iter_delete(iteration_number);
}

// erase an iteration record from the iterations table - called by freeos
// contract
// ACTION
void freeosconfig::iterclear(uint32_t iteration_number) {
  require_auth(name(freeos_acct));

  iter_delete(iteration_number);
}

// function to delete an iteration record
void freeosconfig::iter_delete(uint32_t iteration_number) {
  iterations_index iterations_table(get_self(), get_self().value);
  auto iteration_iterator = iterations_table.find(iteration_number);

  if (iteration_iterator != iterations_table.end()) {
    iterations_table.erase(iteration_iterator);
  }
}

  // ************************************************************************************
  // ************* eosio.proton actions for populating usersinfo table
  // ******************
  // ************************************************************************************

#ifdef TEST_BUILD
// ACTION
// Required for development purposes as eosio.proton kyc verification is not available on the Proton Testnet.
// Will be removed from the production build.
void freeosconfig::userverify(name acc, name verifier, bool verified) {

  require_auth(get_self());

  check(is_account(acc), "Account does not exist.");

  usersinfo usrinf(get_self(), get_self().value);
  auto existing = usrinf.find(acc.value);

  if (existing != usrinf.end()) {
    check(existing->verified != verified, "This status alredy set");
    usrinf.modify(existing, get_self(), [&](auto &p) {
      p.verified = verified;
      if (verified) {
        p.verifiedon = eosio::current_time_point().sec_since_epoch();
        p.verifier = verifier;
      } else {
        p.verifiedon = 0;
        p.verifier = ""_n;
      }
      p.date = eosio::current_time_point().sec_since_epoch();
    });
  } else {
    usrinf.emplace(get_self(), [&](auto &p) {
      p.acc = acc;
      p.name = "";
      p.avatar = "";
      p.verified = verified;

      if (verified) {
        p.verifiedon = eosio::current_time_point().sec_since_epoch();
        p.verifier = verifier;
      } else {
        p.verifiedon = 0;
        p.verifier = ""_n;
      }
      p.date = eosio::current_time_point().sec_since_epoch();
    });
  }

} // end of userverify
#endif

#ifdef TEST_BUILD
// ACTION
// Required for development purposes as eosio.proton kyc verification is not available on the Proton Testnet.
// Will be removed from the production build.
void freeosconfig::addkyc(name acc, name kyc_provider, std::string kyc_level,
                          uint64_t kyc_date) {

  require_auth(get_self());

  kyc_prov kyc;
  kyc.kyc_provider = kyc_provider;
  kyc.kyc_level = kyc_level;
  kyc.kyc_date = kyc_date;

  usersinfo usrinf(get_self(), get_self().value);
  auto itr_usrs = usrinf.require_find(
      acc.value, string("User " + acc.to_string() + " not found").c_str());

  for (auto i = 0; i < itr_usrs->kyc.size(); i++) {
    if (itr_usrs->kyc[i].kyc_provider == kyc.kyc_provider) {
      check(false, string("There is already approval from " +
                          kyc.kyc_provider.to_string())
                       .c_str());
      break;
    }
  }

  usrinf.modify(itr_usrs, get_self(), [&](auto &p) { p.kyc.push_back(kyc); });
}
#endif

// ************************************************************************************
// ************************************************************************************
// ************************************************************************************
} // namespace freedao