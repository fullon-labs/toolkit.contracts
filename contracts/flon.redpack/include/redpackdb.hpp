#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/privileged.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>
#include "flon.ntoken/flon.ntoken.db.hpp"

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

static constexpr uint32_t   MAX_TITLE_SIZE        = 64;
static constexpr uint8_t    EXPIRY_HOURS        = 12;

namespace wasm { namespace db {

#define TG_TBL [[eosio::table, eosio::contract("flon.redpack")]]
#define TG_TBL_NAME(name) [[eosio::table(name), eosio::contract("flon.redpack")]]

struct TG_TBL_NAME("global") global_t {
    name            admin;
    name            claim_admin; // 领取红包的账户，通常是管理员
    name            did_contract;
    set<uint64_t>   dids;
    uint16_t        redpack_expiry_hours; //红包过期时间，单位小时
    asset           unwrap_unit_fee = asset(1000, SYS_SYMBOL); //unit fee for admin unwrap, 0.001 FLON
    bool            unwrap_fee_required = false; //是否对红包创建人征收手续费

    EOSLIB_SERIALIZE( global_t, (admin)(claim_admin)(did_contract)(dids)(redpack_expiry_hours)(unwrap_unit_fee)(unwrap_fee_required) )
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;


struct TG_TBL_NAME("globalidx") globalidx_t {
    uint64_t last_redpack_id =0;
    uint64_t last_claim_id   =0;

    EOSLIB_SERIALIZE( globalidx_t, (last_redpack_id)(last_claim_id) )
};
typedef eosio::singleton< "globalidx"_n, globalidx_t > globalidx_singleton;



inline uint128_t get_unionid( const name& rec, uint64_t packid ) {
     return ( (uint128_t) rec.value << 64 ) | packid;
}

struct TG_TBL redpack_t {
    uint64_t        redpack_id;
    name            code;   //PK
    name            assign_type;   //RANDOM | MEAN
    name            creator; //redpack creator, the one who deposit the token
    bool            did_required = false; //是否需要DID验证
    bool            unwrapped_by_admin = true; //如果是管理员帮助领取，则管理员支付Gas费, 否则用户拆红包并支付Gas费
    name            cover_code;           //红包的图片编号

    // if did_required is true, then passwd_hash is the DID id, otherwise it is the password hash
    string          passwd_hash;    //redpack password hash
    name            token_contract;
    asset           total_quant;
    uint64_t        total_count;
    asset           remaining_quant;
    uint64_t        remaining_count      = 0;
    asset           fee                  = asset(0, SYS_SYMBOL); //unit_fee * total_count
    name            status;
    time_point      created_at;
    time_point      updated_at;

    uint64_t primary_key()  const { return code.value; }
    uint64_t by_id()        const { return redpack_id; }

    redpack_t(){}
    redpack_t( const name& c ): code(c){}

    typedef eosio::multi_index<
        "redpacks"_n, redpack_t,
        indexed_by<"byid"_n, const_mem_fun<redpack_t, uint64_t, &redpack_t::by_id>>
    > idx_t;

    EOSLIB_SERIALIZE( redpack_t, (redpack_id)(code)(assign_type)(creator)(did_required)(unwrapped_by_admin)(cover_code)
                                 (passwd_hash)(token_contract)(total_quant)(total_count)(remaining_quant)(remaining_count)
                                (fee)(status)(created_at)(updated_at) )
};

struct TG_TBL claim_t {
    uint64_t        id;                         //PK
    uint64_t        redpack_id;
    name            redpack_code;
    name            redpack_creator;            //redpack creator
    name            claimer;                    // who receives the redpack
    asset           quantity;                   //amount to receive
    time_point      claimed_at;                 //claim time: when the redpack is claimed

    claim_t() {}
    claim_t( const uint64_t& i ): id(i) {}

    uint64_t primary_key() const { return id; }
    uint128_t by_unionid() const { return get_unionid(claimer, redpack_code.value); }

    typedef eosio::multi_index<"claims"_n, claim_t,
        indexed_by<"by.unionid"_n,  const_mem_fun<claim_t, uint128_t, &claim_t::by_unionid> >
    > idx_t;

    EOSLIB_SERIALIZE( claim_t, (id)(redpack_id)(redpack_code)(redpack_creator)(claimer)(quantity)(claimed_at) )
};

struct TG_TBL tokenlist_t {
    uint64_t        id;
    name            token_contract;
    symbol          token_symbol;

    tokenlist_t(){}
    tokenlist_t( const uint64_t& i ): id(i){}

    uint64_t primary_key() const { return id; }
    uint128_t by_contract_symbol() const { return get_unionid(token_contract, token_symbol.raw()); }
    uint64_t  by_symbol() const { return token_symbol.raw(); }

    typedef eosio::multi_index<"tokenlist"_n, tokenlist_t,
        indexed_by<"by.consymb"_n,  const_mem_fun<tokenlist_t, uint128_t, &tokenlist_t::by_contract_symbol> >,
        indexed_by<"by.symb"_n,  const_mem_fun<tokenlist_t, uint64_t, &tokenlist_t::by_symbol> >
    > idx_t;

    EOSLIB_SERIALIZE( tokenlist_t, (id)(token_contract)(token_symbol) )
};

} }