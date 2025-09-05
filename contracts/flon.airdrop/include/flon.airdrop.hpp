#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>
#include <string>
#include <vector>
#include <optional>

#include "flon.airdrop.db.hpp"
#include "rwid.auth.db.hpp"

namespace flon {

using namespace eosio;
using std::string;
using std::vector;
using std::optional;

#define CHECKC(exp, code, msg) \
   { if (!(exp)) eosio::check(false, string("[[") + to_string((int)code) + string("]] ") + msg); }

enum class err: uint8_t {
   INVALID_FORMAT               = 0,
   TYPE_INVALID                 = 1,
   FEE_NOT_FOUND                = 2,
   INSUFFICIENT_QUANTITY        = 3,
   NOT_POSITIVE                 = 4,
   SYMBOL_MISMATCH              = 5,
   EXPIRED                      = 6,
   PWHASH_INVALID               = 7,
   RECORD_NO_FOUND              = 8,
   NOT_REPEAT_RECEIVE           = 9,
   NOT_EXPIRED                  = 10,
   ACCOUNT_INVALID              = 11,
   FEE_NOT_POSITIVE             = 12,
   VAILD_TIME_INVALID           = 13,
   MIN_UNIT_INVALID             = 14,
   REDPACK_EXIST                = 15,
   DID_NOT_AUTH                 = 16,
   UNDER_MAINTENANCE            = 17,
   NONE_DELETED                 = 19,
   IN_THE_WHITELIST             = 20,
   NON_RENEWAL                  = 21,
   AMOUNT_TOO_SMALL             = 22,
   AMOUNT_TOO_LARGE             = 23,
   FEE_NOT_REQUIRED             = 24,
   DID_NOT_SUPPORTED            = 25,
   DID_PACK_SYMBOL_ERR          = 26,
   STATUS_MISMATCH              = 27
};

class [[eosio::contract("flon.airdrop")]] airdrop : public eosio::contract {
public:
  using contract::contract;

  airdrop(eosio::name receiver, eosio::name code, datastream<const char*> ds)
  : contract(receiver, code, ds),
    _global(get_self(), get_self().value)
  {
    _gstate = _global.exists() ? _global.get() : global_t{};
  }

  ~airdrop() { _global.set(_gstate, get_self()); }

  /* ===== 全局配置 ===== */
  ACTION init(const name& admin,
              const name& rwid_contract,
              const name& oracle_account);

  /* ===== 可领取币白名单 ===== */
  ACTION addtoken(const name& token_bank, const symbol& sym);
  ACTION enabletoken(const name& token_bank, const symbol& sym, const bool& enabled);
  ACTION deltoken(const name& token_bank, const symbol& sym);

  /* ===== 计划管理 ===== */
  ACTION newplan(time_point         started_at,
                 time_point         ended_at,
                 vector<name>       auth_types,
                 const uint64_t&    max_claims   = 0,
                 const uint32_t&    item_limit   = 0);

  ACTION setplan(const uint64_t&            plan_id,
                 optional<time_point>       started_at,
                 optional<time_point>       ended_at,
                 optional<vector<name>>     auth_types,
                 optional<uint64_t>         max_claims,
                 optional<uint32_t>         item_limit);

  /* ===== 计划条目（scope: plan_id） ===== */
  ACTION additem(const uint64_t&      plan_id,
                 const name&          token_bank,
                 const symbol&        sym,
                 const asset&         quant,
                 optional<asset>      per_user_cap,
                 const uint64_t&      item_max_claims = 0);

  ACTION setitem(const uint64_t&      plan_id,
                 const uint64_t&      item_id,
                 optional<asset>      quant,
                 optional<asset>      per_user_cap,
                 optional<uint64_t>   item_max_claims);

  ACTION delitem(const uint64_t& plan_id, const uint64_t& item_id);

  /* ===== 用户领取 ===== */
  ACTION claimreward(const uint64_t&      plan_id,
                     const name&          claimer,
                     optional<uint64_t>   prefer_item_id,
                    const string& memo);

  /* ===== 入金路由 ===== */
  [[eosio::on_notify("*::transfer")]]
  void ontransfer(name from, name to, asset quantity, std::string memo);

private:
  global_singleton _global;
  global_t         _gstate;
};

} // namespace flon