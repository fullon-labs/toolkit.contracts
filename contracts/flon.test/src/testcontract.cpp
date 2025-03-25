#include <testcontract.hpp>

[[eosio::action]]
void testcontract::hi( name nm ) {
   print_f("Name : %\n", nm);
}

[[eosio::action]]
void testcontract::check( name nm ) {
   print_f("Name : %\n", nm);
   eosio::check(nm == "testcontract"_n, "check name not equal to `testcontract`");
}

// Checks the input param `nm` and returns serialized std::pair<int, std::string> instance.
[[eosio::action]]
std::pair<int, std::string> testcontract::checkwithrv( name nm ) {
   print_f("Name : %\n", nm);

   std::pair<int, std::string> results = {0, "NOP"};
   if (nm == "testcontract"_n) {
      results = {0, "Validation has passed."};
   }
   else {
      results = {1, "Input param `name` not equal to `testcontract`."};
   }
   return results; // the `set_action_return_value` intrinsic is invoked automatically here
}

[[eosio::action]]
void testcontract::add(name acct, std::vector<std::string> messages) {
   require_auth(acct);
   for (auto msg : messages) {
      plan_t::idx_t plans(_self, acct.value);
      plans.emplace(acct, [&](auto& item) {
         item.id = plans.available_primary_key();
         item.account = acct;
         item.msg = msg;
      });
   }
}

[[eosio::action]]
void testcontract::remove(std::vector<name> accts) {
   for (auto acct : accts) {
      plan_t::idx_t plans(_self, acct.value);
      auto itr = plans.begin();
      while (itr != plans.end()) {
         itr = plans.erase(itr);
      }
   }
}

[[eosio::action]]
void testcontract::update(name acct, uint64_t id, std::string msg) {
   require_auth(acct);
   
   plan_t::idx_t plans(_self, acct.value);
   auto itr = plans.find(id);
   eosio::check(itr != plans.end(), "Record not found.");
   plans.modify(itr, acct, [&](auto& item) {
      item.msg = msg;
   });

}