#pragma once

#include <eosio/asset.hpp>
#include <eosio/privileged.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>


#include <optional>
#include <string>
#include <map>
#include <set>
#include <type_traits>


namespace flon {

using namespace std;
using namespace eosio;

//Scope: _self
struct [[eosio::table, eosio::contract("rwid.auth")]] account_rwid_t {
    name                        account;        //PK
    string                      rwid_info;    //value: md5(md5(RM + salt)
    time_point                  created_at;

    account_rwid_t() {}
    account_rwid_t(const name& i): account(i) {}

    uint64_t primary_key()const { return account.value ; }

    typedef eosio::multi_index< "acctrwid"_n,  account_rwid_t> idx_t;

    EOSLIB_SERIALIZE( account_rwid_t, (account)(rwid_info)(created_at) )
};


} //namespace flon
