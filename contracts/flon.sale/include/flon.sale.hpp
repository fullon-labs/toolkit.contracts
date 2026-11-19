#pragma once

#include <eosio/asset.hpp>
#include <eosio/contract.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>

#include <string>

class [[eosio::contract("flon.sale")]] flonsale final : public eosio::contract {
public:
   flonsale(eosio::name receiver, eosio::name code, eosio::datastream<const char*> ds);

   /**
    * Initialize the sale controller.  The contract account must authorize this
    * action, so deployment cannot accidentally assign an external controller.
    */
   [[eosio::action]] void init(eosio::name admin);

   /** Create one immutable-price payment/payout offer. */
   [[eosio::action]] void createoffer(
      eosio::name pair,
      eosio::extended_asset payment_unit,
      eosio::extended_asset payout_unit,
      eosio::asset inventory_cap,
      eosio::name revenue_account,
      eosio::time_point_sec starts_at,
      eosio::time_point_sec ends_at,
      std::string note);

   /** Pause or resume a pair without changing its price or accounting. */
   [[eosio::action]] void setpaused(eosio::name pair, bool paused);

   /** Move unsold payout inventory out only after pausing the pair. */
   [[eosio::action]] void withdrawinv(eosio::name pair, eosio::name to, eosio::asset quantity, std::string memo);

   /** Transfer administration after an explicit two-step acceptance. */
   [[eosio::action]] void proposeadmin(eosio::name new_admin);
   [[eosio::action]] void acceptadmin();
   [[eosio::action]] void cancelorder(uint64_t order_id);
   [[eosio::action]] void fillorder(uint64_t order_id, eosio::asset payment);

   [[eosio::on_notify("*::transfer")]] void ontransfer(
      eosio::name from,
      eosio::name to,
      eosio::asset quantity,
      std::string memo);

private:
   struct [[eosio::table("config")]] config_row {
      eosio::name admin;
      eosio::name proposed_admin;
   };
   using config_singleton = eosio::singleton<"config"_n, config_row>;

   struct [[eosio::table]] offer {
      eosio::name             pair;
      eosio::extended_asset   payment_unit;
      eosio::extended_asset   payout_unit;
      eosio::asset            inventory_cap;
      eosio::asset            reserved;        // preserves existing offer row layout
      eosio::asset            funded;
      eosio::asset            sold;
      eosio::asset            received;
      eosio::name             revenue_account;
      eosio::time_point_sec   starts_at;
      eosio::time_point_sec   ends_at;          // epoch zero means no end date
      bool                    paused = true;
      std::string             note;

      uint64_t primary_key() const { return pair.value; }
   };
   using offers_table = eosio::multi_index<"offers"_n, offer>;

   struct [[eosio::table]] order {
      uint64_t id;
      eosio::name buyer;
      eosio::name pair;
      eosio::asset payment;
      eosio::asset payout;
      eosio::asset settled_payment;
      uint64_t primary_key() const { return id; }
   };
   using orders_table = eosio::multi_index<"orders"_n, order>;
   struct [[eosio::table("orderseq")]] order_sequence {
      uint64_t last_id = 0;
   };
   using order_sequence_table = eosio::singleton<"orderseq"_n, order_sequence>;


   // Standard token balance row, queried against the payout token contract.
   struct [[eosio::table]] token_account {
      eosio::asset balance;
      uint64_t primary_key() const { return balance.symbol.code().raw(); }
   };
   using token_accounts = eosio::multi_index<"accounts"_n, token_account>;

   config_singleton _config;
   offers_table     _offers;

   config_row config();
   void require_admin();
   void require_initialized();
   eosio::asset balance_of(const eosio::extended_symbol& token) const;
   void transfer_token(eosio::name token_contract, eosio::name to, eosio::asset quantity, const std::string& memo) const;
   void settle(offers_table::const_iterator offer_itr, eosio::name buyer,
               eosio::asset payment, eosio::asset payout);
};
