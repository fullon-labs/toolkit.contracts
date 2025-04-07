#include <eosio/eosio.hpp>
using namespace eosio;

struct [[eosio::table, eosio::contract("flon.test")]] plan_t {
   
   uint64_t id = 0;
   name    account;
   std::string  msg;
   uint64_t primary_key() const { return id; }
   uint64_t scope() const { return account.value; }

   typedef eosio::multi_index<"plan"_n, plan_t,
      indexed_by<"by.account"_n, const_mem_fun<plan_t, uint64_t, &plan_t::scope >>
      >idx_t;
   EOSLIB_SERIALIZE(plan_t, (id)(account)(msg))
};

class [[eosio::contract("flon.test")]] testcontract: public eosio::contract {
   public:
      using contract::contract;

      [[eosio::action]] 
      void hi( name nm );

      [[eosio::action]] 
      void check( name nm );

      [[eosio::action]] 
      void add(name acct, std::vector<std::string> messages);

      [[eosio::action]]
      void update(name nm, uint64_t id, std::string msg);

      [[eosio::action]]
      void remove(std::vector<name> accts);

      [[eosio::action]] 
      void addrm(std::vector<std::string> add_messages, name remove_acct);


      [[eosio::action]] 
      void rmadd(name remove_acct, std::vector<std::string> messages);

      [[eosio::action]] 
      void rmaddrm(name remove_acct, std::vector<std::string> messages);

      [[eosio::action]] 
      void madd(std::vector<name> accts, std::vector<std::string> messages);

      [[eosio::action]] 
      void mremove(std::vector<name> accts);

      [[eosio::action]] 
      void maddrm(std::vector<name> accts, std::vector<std::string> messages);
      
      [[eosio::action]] 
      void mrmadd(std::vector<name> accts, std::vector<std::string> messages);

      [[eosio::action]]
      std::pair<int, std::string> checkwithrv( name nm );

      using hi_action = action_wrapper<"hi"_n, &testcontract::hi>;
      using check_action = action_wrapper<"check"_n, &testcontract::check>;
      using checkwithrv_action = action_wrapper<"checkwithrv"_n, &testcontract::checkwithrv>;
};