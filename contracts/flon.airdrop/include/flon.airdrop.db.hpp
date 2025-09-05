#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/time.hpp>
#include <eosio/singleton.hpp>
#include <string>
#include <vector>

namespace flon {

using namespace eosio;
using std::string;
using std::vector;

#define TBL struct [[eosio::table, eosio::contract("flon.airdrop")]]
#define NTBL(name) struct [[eosio::table(name), eosio::contract("flon.airdrop")]]

namespace AuthType {
    static constexpr eosio::name RWID   { "rwid"_n   };
    static constexpr eosio::name ORACLE { "oracle"_n };
}

/* -----------------------------
 * Global 配置（singleton）
 * ----------------------------- */
struct [[eosio::table, eosio::contract("flon.airdrop")]] global_t {
    name                admin;             // 管理员账户
    name                rwid_contract;     // RWID 合约账户（为空则未启用）
    name                oracle_account;    // 预言机账号

    uint64_t            last_plan_id = 0;  // ★ 最近的计划 ID
    uint64_t            last_item_id = 0;  // ★ 最近的条目 ID

    EOSLIB_SERIALIZE( global_t, (admin)(rwid_contract)(oracle_account)(last_plan_id)(last_item_id) )
};
typedef eosio::singleton<"global"_n, global_t> global_singleton;

/* -----------------------------
 * 计划 Plan（计划级“一人一次”）
 * ----------------------------- */
NTBL("plans") plan_t {
    uint64_t            plan_id;
    time_point          started_at;               // 开始时间
    time_point          ended_at;                 // 结束时间

    vector<name>        auth_types;              // 本计划要求的认证类型集合
    uint64_t            max_claims       = 0;     // 最大可领人数（0=不限）
    uint64_t            claimed_cnt      = 0;     // 已领取人数
    uint32_t            claim_item_limit = 0;     // 一次领取最多可领的条目数（0=不限；1=只允一个）

    time_point          created_at;
    time_point          updated_at;

    uint64_t primary_key() const { return plan_id; }

    EOSLIB_SERIALIZE(plan_t,
        (plan_id)
        (started_at)(ended_at)(auth_types)
        (max_claims)(claimed_cnt)(claim_item_limit)
        (created_at)(updated_at)
    )
};
typedef eosio::multi_index<"plans"_n, plan_t> plans_t;

/* -----------------------------
 * 奖励条目 Reward Item（仅 FIXED）
 * ----------------------------- */
// scope: plan_id
NTBL("items") item_t {
    uint64_t            item_id;
    name                token_bank;
    symbol              sym;
    asset               quant;                   // 每人固定发放额
    asset               deposited;                // 累计入金
    asset               paid_total;               // 累计发放
    uint64_t            item_max_claims = 0;
    uint64_t            item_claimed_cnt = 0;
    asset               per_user_cap;
    time_point          created_at;
    time_point          updated_at;

    uint64_t  primary_key() const { return item_id; }
    uint64_t  bybank()      const { return token_bank.value; }
    uint64_t  bysym()       const { return sym.code().raw(); }
    uint128_t bybankprec() const {
    return ( (uint128_t)token_bank.value << 64 )
        | ( ((uint128_t)sym.code().raw() << 8) | sym.precision() );
    }
    EOSLIB_SERIALIZE( item_t,
        (item_id)(token_bank)(sym)(quant)
        (deposited)(paid_total)(item_max_claims)(item_claimed_cnt)
        (per_user_cap)(created_at)(updated_at)
    )
};
typedef eosio::multi_index<
  "items"_n, item_t,
  indexed_by<"bybank"_n,      const_mem_fun<item_t, uint64_t,  &item_t::bybank>>,
  indexed_by<"bysym"_n,       const_mem_fun<item_t, uint64_t,  &item_t::bysym>>,
  indexed_by<"bybankprec"_n,  const_mem_fun<item_t, uint128_t, &item_t::bybankprec>>
> items_t;

/* -----------------------------
 * 领取记录 Claims（计划级“一人一次”）
 * ----------------------------- */
// scope: plan_id
NTBL("claims") claim_t {
    uint64_t            id;                     // 自增主键（plan 级）
    name                claimer;                // 领用人
    uint32_t            claimed_items = 0;
    vector<asset>       rewards;
    uint64_t            claimed_at_us;

    uint64_t primary_key() const { return id; }
    uint64_t byuser()       const { return claimer.value; }
    uint64_t bytime()       const { return claimed_at_us; }

    EOSLIB_SERIALIZE( claim_t, (id)(claimer)(claimed_items)(rewards)(claimed_at_us) )
};

typedef eosio::multi_index<
  "claims"_n, claim_t,
  indexed_by<"byuser"_n, const_mem_fun<claim_t, uint64_t, &claim_t::byuser>>,
  indexed_by<"bytime"_n, const_mem_fun<claim_t, uint64_t, &claim_t::bytime>>
> claims_t;

/* -----------------------------
 * 全局可领取币种表（Tokens Allowlist）
 * ----------------------------- */
// scope: self
NTBL("tokens") token_t {
    uint64_t            id;
    name                token_bank;     // 代币合约账户
    symbol              sym;            // 币种（含精度）
    bool                enabled = true; // 是否启用
    time_point          created_at;
    time_point          updated_at;

    uint64_t  primary_key() const { return id; }
    uint128_t bybankcode()  const { return ( (uint128_t)token_bank.value << 64 ) | sym.code().raw(); }
    uint64_t  bybank()      const { return token_bank.value; }
    uint64_t  bycode()      const { return sym.code().raw(); }
    uint64_t  byenabled()   const { return enabled ? 1 : 0; }

    EOSLIB_SERIALIZE( token_t,
        (id)(token_bank)(sym)(enabled)(created_at)(updated_at)
    )
};
typedef eosio::multi_index<
  "tokens"_n, token_t,
  indexed_by<"bybankcode"_n, const_mem_fun<token_t, uint128_t, &token_t::bybankcode>>,
  indexed_by<"bybank"_n,     const_mem_fun<token_t, uint64_t,  &token_t::bybank>>,
  indexed_by<"bycode"_n,     const_mem_fun<token_t, uint64_t,  &token_t::bycode>>,
  indexed_by<"byenabled"_n,  const_mem_fun<token_t, uint64_t,  &token_t::byenabled>>
> tokens_t;

} // namespace flon