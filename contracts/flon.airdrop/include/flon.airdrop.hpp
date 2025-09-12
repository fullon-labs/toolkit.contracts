#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>
#include <string>
#include <vector>
#include <array>
#include <optional>

#include "flon.airdrop.db.hpp"
#include "utils.hpp"

namespace flon {

using namespace eosio;
using std::string;
using std::vector;
using std::array;
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
   STATUS_MISMATCH              = 27,
   ALREADY_EXISTS               = 28,
   EXCEED_LIMIT                 = 29
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
              const set<name>& oracles);

    /* ===== 预言机管理 ===== */
  ACTION addoracle(const name& oracle);
  ACTION deloracle(const name& oracle);

  /* ===== 可领取币白名单 ===== */
  ACTION addtoken(const name& token_bank, const symbol& sym);
  ACTION deltoken(const name& token_bank, const symbol& sym);

  /* ===== 计划管理 ===== */
  ACTION addplan(const string& title,
                      const time_point& started_at,
                      const time_point& ended_at,
                      const vector<extended_asset>& single_claim
                      );

  ACTION setplan(const uint64_t& plan_id,
                      std::optional<string> title,
                      std::optional<time_point> started_at,
                      std::optional<time_point> ended_at
                      );

  ACTION addclaims(const uint64_t& plan_id, const extended_asset& single_claim);
  ACTION delclaims(const uint64_t& plan_id);

  /* ===== oracle 代领 ===== */
  ACTION claim(const uint64_t& plan_id,
                      const name&     claimer,
                      const name&     beneficiary,
                      const string&   memo);

  /* ===== 入金路由（仅接收 tokens 白名单中的币） ===== */
  // memo: "add:<plan_id>"
  [[eosio::on_notify("*::transfer")]]
  void ontransfer(name from, name to, asset quantity, std::string memo);


  inline uint128_t bankcode_key(const name& bank, const symbol& sym) {
      return ((uint128_t)bank.value << 64) | (uint128_t)sym.code().raw();
  }

private:
  global_singleton _global;
  global_t         _gstate;
};

} // namespace flon