#pragma once

#include "wasm_db.hpp"

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
    RATE_OVERLOAD        = 19,
    DATA_MISMATCH        = 20,
    MISC                 = 255
 };
 
static constexpr symbol SYS_SYMBOL              = SYMBOL("FLON", 8);
static constexpr name SYS_BANK                  = "flon.token"_n;
static constexpr name FLON_CONTRACT             = "flon"_n;
static constexpr name OWNER_PERM                = "owner"_n;
static constexpr name ACTIVE_PERM               = "active"_n;

static constexpr uint64_t MAX_LOCK_DAYS         = 365 * 10;

#ifndef DAY_SECONDS_FOR_TEST
static constexpr uint64_t DAY_SECONDS           = 24 * 60 * 60;
#else
#warning "DAY_SECONDS_FOR_TEST should be used only for test!!!"
static constexpr uint64_t DAY_SECONDS           = DAY_SECONDS_FOR_TEST;
#endif//DAY_SECONDS_FOR_TEST

static constexpr uint32_t MAX_TITLE_SIZE        = 64;


namespace wasm { namespace db {

#define FAUCET_TBL [[eosio::table, eosio::contract("flon.faucet")]]
#define FAUCET_TBL_NAME(name) [[eosio::table(name), eosio::contract("flon.faucet")]]

struct FAUCET_TBL_NAME("global") global_t {
    name admin              = "flonianadmin"_n;


    EOSLIB_SERIALIZE( global_t, (admin) )
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;

struct FAUCET_TBL account_t {
    name            owner;                      //plan owner
    time_point      updated_at;                 //update time: last updated at

    uint64_t primary_key() const { return owner.value; }

    typedef multi_index< "accounts"_n, account_t > idx_t;

    EOSLIB_SERIALIZE( account_t, (owner)(updated_at) )

};

} }