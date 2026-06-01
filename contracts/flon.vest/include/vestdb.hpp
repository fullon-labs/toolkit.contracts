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
 
static constexpr eosio::name active_perm        {"active"_n};
static constexpr symbol SYS_SYMBOL              = SYMBOL("FLON", 8);
static constexpr name SYS_BANK                  { "flon.token"_n };

static constexpr uint64_t MAX_LOCK_DAYS         = 365 * 10;
static constexpr uint32_t MAX_IMPORT_ROWS       = 50;

#ifndef DAY_SECONDS_FOR_TEST
static constexpr uint64_t DAY_SECONDS           = 24 * 60 * 60;
#else
#warning "DAY_SECONDS_FOR_TEST should be used only for test!!!"
static constexpr uint64_t DAY_SECONDS           = DAY_SECONDS_FOR_TEST;
#endif//DAY_SECONDS_FOR_TEST

static constexpr uint32_t MAX_TITLE_SIZE        = 64;


namespace wasm { namespace db {

#define VEST_TBL [[eosio::table, eosio::contract("flon.vest")]]
#define VEST_TBL_NAME(name) [[eosio::table(name), eosio::contract("flon.vest")]]

struct VEST_TBL_NAME("global") global_t {
    name admin              = "flonianadmin"_n;
    name fee_receiver       = "flon.fee"_n;
    asset plan_fee          = asset(0, SYS_SYMBOL);
    

    EOSLIB_SERIALIZE( global_t, (admin)(fee_receiver)(plan_fee) )
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;


namespace plan_status {
    static constexpr eosio::name PENDING        = "pending"_n;
    static constexpr eosio::name ENABLED        = "enabled"_n;
    static constexpr eosio::name DISABLED       = "disabled"_n;
};

struct VEST_TBL_NAME("vestplans") vest_plan_t { //scope: _self
    uint64_t        id;                         //PK, auto-increment
    name            owner;
    string          title;
    name            asset_contract;
    symbol          asset_symbol;
    uint64_t        unlock_interval_days;
    uint64_t        unlock_times;
    asset           total_issued;
    asset           total_unlocked;
    asset           total_refunded;
    name            status = plan_status::ENABLED;
    time_point      created_at;
    time_point      updated_at;

    uint64_t primary_key() const { return id; }

    uint128_t by_owner() const { return (uint128_t)owner.value << 64 | (uint128_t)id; }

    typedef eosio::multi_index<"vestplans"_n, vest_plan_t,
        indexed_by<"by.owneridx"_n,  const_mem_fun<vest_plan_t, uint128_t, &vest_plan_t::by_owner> >
    > tbl_t;

    EOSLIB_SERIALIZE( vest_plan_t, (id)(owner)(title)(asset_contract)(asset_symbol)(unlock_interval_days)(unlock_times)
                                   (total_issued)(total_unlocked)(total_refunded)(status)(created_at)(updated_at) )
};

namespace issue_status {
    static constexpr eosio::name ONGOING        = "ongoing"_n;
    static constexpr eosio::name TERMINATED     = "terminated"_n;
    static constexpr eosio::name FINISHED       = "finished"_n;
};


struct VEST_TBL_NAME("vestissues") vest_issue_t {   //scope: _self
    uint64_t      issue_id = 0;                     // PK, auto-increment
    uint64_t      plan_id = 0;
    name          issuer;
    name          receiver;
    asset         issued;
    asset         locked;
    asset         unlocked;
    uint64_t      first_unlock_days = 0;
    uint64_t      unlock_interval_days;
    uint64_t      unlock_times;
    name          status = issue_status::ONGOING;
    time_point    issued_at;
    time_point    updated_at;
    string        memo;

    uint64_t primary_key() const { return issue_id; }

    uint128_t by_plan() const { return (uint128_t)plan_id << 64 | (uint128_t)issue_id; }
    uint128_t by_receiver_issue() const { return (uint128_t)receiver.value << 64 | (uint128_t)issue_id; }
    uint128_t by_planreceiver() const { return (uint128_t)plan_id << 64 | (uint128_t)receiver.value; }

    typedef eosio::multi_index<"vestissues"_n, vest_issue_t,
        indexed_by<"by.planidx"_n,      const_mem_fun<vest_issue_t, uint128_t, &vest_issue_t::by_plan>>,
        indexed_by<"by.recveridx"_n,    const_mem_fun<vest_issue_t, uint128_t, &vest_issue_t::by_receiver_issue>>,
        indexed_by<"by.planrcver"_n,    const_mem_fun<vest_issue_t, uint128_t, &vest_issue_t::by_planreceiver>>
    > tbl_t;

    EOSLIB_SERIALIZE( vest_issue_t,  (issue_id)(plan_id)(issuer)(receiver)(issued)(locked)(unlocked)
                                     (first_unlock_days)(unlock_interval_days)(unlock_times)
                                     (status)(issued_at)(updated_at)(memo) )
};
//scope: _self
struct VEST_TBL account {
    name    owner;
    uint64_t last_plan_id;

    uint64_t primary_key()const { return owner.value; }

    typedef multi_index_ex< "accounts"_n, account > tbl_t;

    EOSLIB_SERIALIZE( account,  (owner)(last_plan_id) )
};

} }
