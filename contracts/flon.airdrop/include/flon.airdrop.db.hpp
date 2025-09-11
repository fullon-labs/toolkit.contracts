#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/time.hpp>
#include <eosio/singleton.hpp>
#include <string>
#include <vector>
#include <array>
#include <set>

namespace flon {

using namespace eosio;
using std::string;
using std::vector;
using std::array;
using std::set;

#define TBL struct [[eosio::table, eosio::contract("flon.airdrop")]]
#define NTBL(name) struct [[eosio::table(name), eosio::contract("flon.airdrop")]]

/* -----------------------------
 * Global 配置（singleton）
 * ----------------------------- */
struct [[eosio::table, eosio::contract("flon.airdrop")]] global_t {
    name            admin;              // 管理员账户
    set<name>       oracles;            // 预言机账号列表
    uint64_t        last_plan_id = 0;   // 最近的计划 ID
    uint64_t        last_token_id = 0;  // 最近的token ID
    EOSLIB_SERIALIZE(global_t, (admin)(oracles)(last_plan_id) (last_token_id))
};
typedef eosio::singleton<"global"_n, global_t> global_singleton;

struct token_rule_t {
  extended_asset total;   // 计划总额度（余额池）
  extended_asset per;     // 单人领取额度（FIXED）

  // ★ 必须：否则 newplan/claim 序列化会失败
  EOSLIB_SERIALIZE(token_rule_t, (total)(per))
};

/* -----------------------------
 * 计划 Plan（计划级“一人一次”）
 * - claim_config: 每个元素为 [total_quota, per_user]（均为 extended_asset）
 * ----------------------------- */
NTBL("plans") plan_t {
    uint64_t     id;
    time_point   plan_started_at;                 // 开始时间
    time_point   plan_ended_at;                   // 结束时间
    std::vector<token_rule_t> claim_config; // 每币配置：[总额度, 单人额度]
    uint64_t     claimed_cnt = 0;                 // 已领取人数
    string       title;
    time_point   created_at;
    time_point   updated_at;

    uint64_t primary_key() const { return id; }

    EOSLIB_SERIALIZE(plan_t,
        (id)(plan_started_at)(plan_ended_at)(claim_config)
        (claimed_cnt)(title)(created_at)(updated_at)
    )
};
typedef eosio::multi_index<"plans"_n, plan_t> plans_t;

/* -----------------------------
 * 全局可领取币种白名单（Tokens Allowlist）
 * ontransfer 只接收此表内且 enabled=true 的币
 * ----------------------------- */
NTBL("tokens") token_t {
    uint64_t   id;
    name       token_bank;     // 代币合约账户
    symbol     sym;            // 币种（含精度）
    time_point created_at;
    time_point updated_at;

    uint64_t  primary_key() const { return id; }
    uint128_t bybankcode()  const { return ( (uint128_t)token_bank.value << 64 )
                                          | (uint128_t)sym.code().raw(); }
    uint64_t  bybank()      const { return token_bank.value; }
    uint64_t  bycode()      const { return sym.code().raw(); }

    EOSLIB_SERIALIZE(token_t, (id)(token_bank)(sym)(created_at)(updated_at))
};
typedef eosio::multi_index<
  "tokens"_n, token_t,
  indexed_by<"bybankcode"_n, const_mem_fun<token_t, uint128_t, &token_t::bybankcode>>,
  indexed_by<"bybank"_n,     const_mem_fun<token_t, uint64_t,  &token_t::bybank>>,
  indexed_by<"bycode"_n,     const_mem_fun<token_t, uint64_t,  &token_t::bycode>>
> tokens_t;

} // namespace flon