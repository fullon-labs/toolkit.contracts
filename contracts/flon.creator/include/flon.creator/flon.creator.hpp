#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <string>
#include <wasm_db.hpp>
#include "flon.creator/flon.token.hpp"
#include "flon.creator/flon.creator.db.hpp"

namespace flon {

using std::string;
using namespace eosio;
using namespace wasm::db;

/**
 * The `flon.creator` is Cross-chain (X -> flon -> Y) contract
 * 
 */

 #define hash(str) sha256(const_cast<char*>(str.c_str()), str.size())

#define TRANSFER(bank, from, to, quantity, memo) \
    {	token::transfer_action act{ bank, { {_self, active_perm} } };\
			act.send( from, to, quantity , memo );}


class [[eosio::contract("flon.creator")]] flon_creator : public contract {
private:
   dbc                      _db;
   global_singleton         _global;
   global_t                 _gstate;

public:
   using contract::contract;

   flon_creator(eosio::name receiver, eosio::name code, datastream<const char*> ds):
        _db(_self), contract(receiver, code, ds), 
        _global(_self, _self.value){
            
        if (_global.exists()) {
            _gstate = _global.get();

        } else { // first init
            _gstate = global_t{};
        }
    }

    ~flon_creator() { 
        _global.set( _gstate, get_self() );
    }

    /**
     * @usage: create a new account, signed & submitted by a proxy miner
     * @params:
     *   - sig: singature by user:  acct | pubkey
     **/
    ACTION newaccount(const name& admin, const eosio::public_key& pubkey, const name& acct); 

    ACTION init( const name& admin) {
        _check_admin( );
        _gstate.admin  = admin;
    }
    ACTION setgasquant( const asset& gas_quant) {
        _check_admin();
        _gstate.gas_quant  = gas_quant;
    }

    private:
    void _check_admin(){
        CHECKC( has_auth(_self) || has_auth(_gstate.admin), err::NO_AUTH, "no auth for operate" )
    }


    void _newaccount(const name& account, const public_key& pubkey);

};
} //namespace apollo
