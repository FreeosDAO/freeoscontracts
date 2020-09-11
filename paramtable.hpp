#ifndef __PARAMTABLE_H_INCLUDED
#define __PARAMTABLE_H_INCLUDED

using namespace eosio;

// parameter table

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

#endif
