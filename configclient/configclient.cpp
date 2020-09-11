#include <eosio/eosio.hpp>
#include "configclient.hpp"
#include "../paramtable.hpp"

using namespace eosio;


[[eosio::action]]
void configclient::getvalue(name paramname) const {

    parameter_index parameters("freeosconfig"_n, "freeosconfig"_n.value);
    auto iterator = parameters.find(paramname.value);

    // check if the parameter is in the table or not
    if (iterator == parameters.end() ) {
        // the parameter is not in the table, so insert
        print("Parameter ", paramname, " is not in the config table");
    } else {
        // the parameter is in the table
        const auto& p = *iterator;
        print_parameter(p);
    }
}

[[eosio::action]]
void configclient::getall() const {

    parameter_index parameters("freeosconfig"_n, "freeosconfig"_n.value);
    auto iterator = parameters.begin();

    while (iterator != parameters.end()) {
      const auto& p = *iterator;
      print(p.paramname, ": '", p.value, "' ");
      iterator++;
    }
}

[[eosio::action]]
void configclient::gettable(name tablename) {
    parameter_index parameters("freeosconfig"_n, "freeosconfig"_n.value);
    auto table_index = parameters.get_index<name("virtualtable")>();
    auto iterator = table_index.find(tablename.value);

    while (iterator->virtualtable == tablename) {
      print(iterator->paramname, ": ", iterator->value, " ");
      iterator++;
    }
}


void configclient::print_parameter(parameter p) const {
  print("Parameter: '", p.paramname, "' in virtual table: '", p.virtualtable, "' has value: '", p.value, "'");
}
