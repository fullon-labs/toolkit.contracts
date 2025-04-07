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

[[eosio::action]]
void testcontract::addrm(std::vector<std::string> add_messages, name remove_acct) {
   require_auth(remove_acct);
   plan_t::idx_t plans(_self, remove_acct.value);
   auto itr = plans.begin();
   while (itr != plans.end()) {
      itr = plans.erase(itr);
   }
   for (auto msg : add_messages) {
      plans.emplace(remove_acct, [&](auto& item) {
         item.id = plans.available_primary_key();
         item.account = remove_acct;
         item.msg = msg;
      });
   }
}

[[eosio::action]]
void testcontract::rmadd(name remove_acct, std::vector<std::string> messages){
   require_auth(remove_acct);
   plan_t::idx_t plans(_self, remove_acct.value);
   auto itr = plans.begin();
   while (itr != plans.end()) {
      itr = plans.erase(itr);
   }
   for (auto msg : messages) {
      plans.emplace(remove_acct, [&](auto& item) {
         item.id = plans.available_primary_key();
         item.account = remove_acct;
         item.msg = msg;
      });
   }
}

[[eosio::action]]
void testcontract::rmaddrm(name remove_acct, std::vector<std::string> messages) {
   require_auth(remove_acct);
   plan_t::idx_t plans(_self, remove_acct.value);
   auto itr = plans.begin();
   while (itr != plans.end()) {
      itr = plans.erase(itr);
   }
   for (auto msg : messages) {
      plans.emplace(remove_acct, [&](auto& item) {
         item.id = plans.available_primary_key();
         item.account = remove_acct;
         item.msg = msg;
      });
   }
}

[[eosio::action]]
void testcontract::madd(std::vector<name> accts, std::vector<std::string> messages) {
   for (auto acct : accts) {
      require_auth(acct);
      plan_t::idx_t plans(_self, acct.value);
      for (auto msg : messages) {
         plans.emplace(acct, [&](auto& item) {
            item.id = plans.available_primary_key();
            item.account = acct;
            item.msg = msg;
         });
      }
   }
}

[[eosio::action]]
void testcontract::mremove(std::vector<name> accts) {
   for (auto acct : accts) {
      plan_t::idx_t plans(_self, acct.value);
      auto itr = plans.begin();
      while (itr != plans.end()) {
         itr = plans.erase(itr);
      }
   }
}

[[eosio::action]]
void testcontract::maddrm(std::vector<name> accts, std::vector<std::string> messages) {
   for (auto acct : accts) {
      require_auth(acct);
      plan_t::idx_t plans(_self, acct.value);
      auto itr = plans.begin();
      while (itr != plans.end()) {
         itr = plans.erase(itr);
      }
      for (auto msg : messages) {
         plans.emplace(acct, [&](auto& item) {
            item.id = plans.available_primary_key();
            item.account = acct;
            item.msg = msg;
         });
      }
   }
}

[[eosio::action]]
void testcontract::mrmadd(std::vector<name> accts, std::vector<std::string> messages) {
   for (auto acct : accts) {
      require_auth(acct);
      plan_t::idx_t plans(_self, acct.value);
      auto itr = plans.begin();
      while (itr != plans.end()) {
         itr = plans.erase(itr);
      }
      for (auto msg : messages) {
         plans.emplace(acct, [&](auto& item) {
            item.id = plans.available_primary_key();
            item.account = acct;
            item.msg = msg;
         });
      }
   }
}