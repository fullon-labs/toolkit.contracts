#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/privileged.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>

using namespace eosio;
using namespace std;
using std::string;

// using namespace wasm;
#define SYMBOL(sym_code, precision) symbol(symbol_code(sym_code), precision)

static constexpr eosio::name active_perm        {"active"_n};
static constexpr symbol SYS_SYMBOL              = SYMBOL("FLON", 8);
static constexpr name SYS_BANK                  { "flon.token"_n };
static constexpr uint32_t MIN_SINGLE_REDPACK    = 100;
static constexpr uint64_t seconds_per_month     = 24 * 3600 * 30;

#ifndef DAY_SECONDS_FOR_TEST
static constexpr uint64_t DAY_SECONDS           = 24 * 60 * 60;
#else
#warning "DAY_SECONDS_FOR_TEST should be used only for test!!!"
static constexpr uint64_t DAY_SECONDS           = DAY_SECONDS_FOR_TEST;
#endif//DAY_SECONDS_FOR_TEST

static constexpr uint32_t MAX_TITLE_SIZE        = 64;
static constexpr uint8_t    EXPIRY_HOURS        = 12;

namespace wasm { namespace db {

#define TG_TBL [[eosio::table, eosio::contract("flon.redpack")]]
#define TG_TBL_NAME(name) [[eosio::table(name), eosio::contract("flon.redpack")]]

struct TG_TBL_NAME("global") global_t {
    name            admin;
    uint16_t        expire_hours;   //discarded
    uint16_t        data_failure_hours;
    bool            did_supported;
    name            did_contract;
    uint64_t        did_id;         //DID id, 0 means not used

    EOSLIB_SERIALIZE( global_t, (admin)(expire_hours)(data_failure_hours)(did_supported)(did_contract)(did_id))
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;


inline uint128_t get_unionid( const name& rec, uint64_t packid ) {
     return ( (uint128_t) rec.value << 64 ) | packid;
}

struct TG_TBL redpack_t {
    name            code;   //PK
    name            type;   //RANDOM | MEAN
    name            creator;
    bool            did_required = false; //是否需要DID验证
    // if did_required is true, then pw_hash is the DID id, otherwise it is the password hash
    // pw_hash is a string in format: "password:contract_name"
    string          pw_hash;
    asset           total_quantity;
    uint64_t        receiver_count;
    asset           remain_quantity;
    uint64_t        remain_count      = 0;
    name            status;
    time_point      created_at;
    time_point      updated_at;

    uint64_t primary_key() const { return code.value; }

    redpack_t(){}
    redpack_t( const name& c ): code(c){}

    typedef eosio::multi_index<"redpacks"_n, redpack_t> idx_t;

    EOSLIB_SERIALIZE( redpack_t, (code)(type)(creator)(did_required)(pw_hash)(total_quantity)(receiver_count)(remain_quantity)(remain_count)
                                 (status)(created_at)(updated_at) )
};

struct TG_TBL claim_t {
    uint64_t        id;                         //PK
    name            redpack_code;
    name            creator;                    //redpack creator
    name            receiver;                   //claimer
    asset           quantity;                   //amount to receive
    time_point      claimed_at;                 //claim time: when the redpack is claimed
    
    claim_t() {}
    claim_t( const uint64_t& i ): id(i) {}

    uint64_t primary_key() const { return id; }
    uint128_t by_unionid() const { return get_unionid(receiver, redpack_code.value); }

    typedef eosio::multi_index<"claims"_n, claim_t,
        indexed_by<"by.unionid"_n,  const_mem_fun<claim_t, uint128_t, &claim_t::by_unionid> >
    > idx_t;

    EOSLIB_SERIALIZE( claim_t, (id)(redpack_code)(creator)(receiver)(quantity)(claimed_at) )
};

struct TG_TBL tokenlist_t {
    uint64_t        id;
    name            token_contract;   
    symbol          token_symbol;
    time_point_sec  expired_at;

    tokenlist_t(){}
    tokenlist_t( const uint64_t& i ): id(i){}

    uint64_t primary_key() const { return id; }
    uint128_t by_contract_symbol() const { return get_unionid(token_contract, token_symbol.raw()); }
    uint64_t  by_symbol() const { return token_symbol.raw(); }

    typedef eosio::multi_index<"tokenlist"_n, tokenlist_t,
        indexed_by<"by.consym"_n,  const_mem_fun<tokenlist_t, uint128_t, &tokenlist_t::by_contract_symbol> >,
        indexed_by<"by.sym"_n,  const_mem_fun<tokenlist_t, uint64_t, &tokenlist_t::by_symbol> >
    > idx_t;

    EOSLIB_SERIALIZE( tokenlist_t, (id)(token_contract)(token_symbol)(expired_at) )
};

} }