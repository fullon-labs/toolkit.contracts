#pragma once

#include <eosio/asset.hpp>
#include <eosio/action.hpp>

#include <string>

#include <flon.applybp/flon.applybp.db.hpp>
#include <wasm_db.hpp>

namespace flon {

using std::string;
using std::vector;

#define TRANSFER(bank, to, quantity, memo) \
    {	mtoken::transfer_action act{ bank, { {_self, active_perm} } };\
			act.send( _self, to, quantity , memo );}
         
using namespace wasm::db;
using namespace eosio;

static constexpr name      NFT_BANK    = "did.ntoken"_n;
static constexpr eosio::name active_perm{"active"_n};


enum class err: uint8_t {
   NONE                 = 0,
   RECORD_NOT_FOUND     = 1,
   RECORD_EXISTING      = 2,
   SYMBOL_MISMATCH      = 4,
   PARAM_ERROR          = 5,
   MEMO_FORMAT_ERROR    = 6,
   PAUSED               = 7,
   NO_AUTH              = 8,
   NOT_POSITIVE         = 9,
   NOT_STARTED          = 10,
   OVERSIZED            = 11,
   TIME_EXPIRED         = 12,
   NOTIFY_UNRELATED     = 13,
   ACTION_REDUNDANT     = 14,
   ACCOUNT_INVALID      = 15,
   FEE_INSUFFICIENT     = 16,
   FIRST_CREATOR        = 17,
   STATUS_ERROR         = 18,
   SCORE_NOT_ENOUGH     = 19,
   NEED_REQUIRED_CHECK  = 20

};

/**
 * The `flon.applybp` contract manages block producer application profiles.
 */
class [[eosio::contract("flon.applybp")]] flon_applybp : public contract {
   
   private:
      dbc                 _dbc;
   public:
      using contract::contract;
  
   flon_applybp(eosio::name receiver, eosio::name code, datastream<const char*> ds): contract(receiver, code, ds),
         _dbc(get_self()),
         _global(get_self(), get_self().value),
         _producer_tbl(get_self(), get_self().value)
    {
        _gstate = _global.exists() ? _global.get() : global_t{};
        
    }

    ~flon_applybp() { _global.set( _gstate, get_self() ); }


   ACTION init( const name& admin);


   ACTION applybp( const name& owner,
                  const string& logo_uri,
                  const string& org_name,
                  const string& org_info,
                  const name& dao_code,
                  const string& reward_shared_plan,
                  const string& manifesto,
                  const string& issuance_plan);


   ACTION updatebp(const name& owner,
                  const string& logo_uri,
                  const string& org_name,
                  const string& org_info,
                  const name& dao_code,
                  const string& reward_shared_plan,
                  const string& manifesto,
                  const string& issuance_plan);

   ACTION addproducer(const name& submiter,
                  const name& owner,
                  const string& logo_uri,
                  const string& org_name,
                  const string& org_info,
                  const name& dao_code,
                  const string& reward_shared_plan,
                  const string& manifesto,
                  const string& issuance_plan);

   ACTION setstatus( const name& submiter, const name& owner, const name& status);

   private:
      global_singleton    _global;
      global_t            _gstate;
      producer_t::table   _producer_tbl;

      void _set_producer(const name& owner,
                  const string& logo_uri,
                  const string& org_name,
                  const string& org_info,
                  const name& dao_code,
                  const string& reward_shared_plan,
                  const string& manifesto,
                  const string& issuance_plan);
};
} //namespace flon
