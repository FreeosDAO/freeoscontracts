#include <eosio/eosio.hpp>
#include "../paramtable.hpp"

using namespace eosio;

class [[eosio::contract("configclient")]] configclient : public eosio::contract {
  public:
    configclient(name receiver, name code, datastream<const char*> ds):contract(receiver, code, ds) {}

    [[eosio::action]]
    void getvalue(name paramname) const;

    [[eosio::action]]
    void getall() const;

    [[eosio::action]]
    void gettable(name tablename);

  private:

    void print_parameter(parameter p) const;

};
