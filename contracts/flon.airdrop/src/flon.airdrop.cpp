#include "flon.airdrop.hpp"
#include "utils.hpp"
#include "flon.token.hpp"

namespace flon {

// ---------- 小工具 ----------
static inline time_point now() { return current_time_point(); }

static inline uint128_t bankcode_key(const eosio::name& bank, const eosio::symbol& sym) {
  return ( (uint128_t)bank.value << 64 ) | (uint128_t)sym.code().raw();
}

inline bool is_oracle(const std::set<name>& oracles, const name& n) {
  return oracles.find(n) != oracles.end();
}

inline void assert_token_allowed(name self, name token_bank, symbol sym) {
  tokens_t tokens(self, self.value);
  auto by = tokens.get_index<"bybankcode"_n>();
  auto it = by.find(bankcode_key(token_bank, sym));
  CHECKC(it != by.end(), err::STATUS_MISMATCH, "token not allowed");
}

inline plans_t::const_iterator get_plan(const plans_t& plans, uint64_t plan_id) {
  auto it = plans.find(plan_id);
  CHECKC(it != plans.end(), err::RECORD_NO_FOUND, "plan not found");
  return it;
}

// ---------- 全局配置 ----------
void airdrop::init(const name& admin, const std::set<name>& oracles) {
  require_auth(get_self());
  CHECKC(admin.value && is_account(admin), err::ACCOUNT_INVALID, "invalid admin");
  for (auto& o : oracles) CHECKC(o.value && is_account(o), err::ACCOUNT_INVALID, "invalid oracle");

  _gstate.admin         = admin;
  _gstate.oracles       = oracles;
  _gstate.last_plan_id  = 0;
  _gstate.last_token_id = 0;
  _global.set(_gstate, get_self());
}

void airdrop::addoracle(const name& oracle) {
  check(
        has_auth(_gstate.admin) || has_auth(get_self()),
        "[[16]] requires admin or self auth"
    );
  CHECKC(oracle.value && is_account(oracle), err::ACCOUNT_INVALID, "invalid oracle");
  CHECKC(!is_oracle(_gstate.oracles, oracle), err::ALREADY_EXISTS, "oracle already exists");
  _gstate.oracles.insert(oracle);
  _global.set(_gstate, get_self());
}

void airdrop::deloracle(const name& oracle) {
  check(
        has_auth(_gstate.admin) || has_auth(get_self()),
        "[[16]] requires admin or self auth"
    );
  auto it = _gstate.oracles.find(oracle);
  CHECKC(it != _gstate.oracles.end(), err::RECORD_NO_FOUND, "oracle not found");
  _gstate.oracles.erase(it);
  _global.set(_gstate, get_self());
}

// ---------- 可用币白名单 ----------
void airdrop::addtoken(const name& token_bank, const symbol& sym) {
  check(
        has_auth(_gstate.admin) || has_auth(get_self()),
        "[[16]] requires admin or self auth"
    );
  CHECKC(token_bank.value && is_account(token_bank), err::ACCOUNT_INVALID, "invalid token_bank");
  CHECKC(sym.is_valid(), err::INVALID_FORMAT, "invalid symbol");

  tokens_t tokens(get_self(), get_self().value);
  auto by = tokens.get_index<"bybankcode"_n>();
  CHECKC(by.find(bankcode_key(token_bank, sym)) == by.end(), err::ALREADY_EXISTS, "token already exists");

  tokens.emplace(get_self(), [&](auto& r){
    r.id         = ++_gstate.last_token_id;
    r.token_bank = token_bank;
    r.sym        = sym;
    r.created_at = now();
    r.updated_at = r.created_at;
  });

  _global.set(_gstate, get_self());
}

void airdrop::deltoken(const name& token_bank, const symbol& sym) {
  check(
        has_auth(_gstate.admin) || has_auth(get_self()),
        "[[16]] requires admin or self auth"
    );
  tokens_t tokens(get_self(), get_self().value);
  auto by = tokens.get_index<"bybankcode"_n>();
  auto it = by.find(bankcode_key(token_bank, sym));
  CHECKC(it != by.end(), err::RECORD_NO_FOUND, "token not found");
  by.erase(it);
}

// ---------- 新建计划 ----------
void airdrop::newplan(time_point started_at,
                      time_point ended_at,
                      const std::vector<token_rule_t>& claim_config_in) {
  check(
        has_auth(_gstate.admin) || has_auth(get_self()),
        "[[16]] requires admin or self auth"
    );
  CHECKC(started_at <= ended_at, err::VAILD_TIME_INVALID, "time window invalid");
  CHECKC(!claim_config_in.empty(), err::INVALID_FORMAT, "claim_config empty");

  // 校验 + 规范化（total.amount 强制 0；白名单校验）
  std::vector<token_rule_t> cfg;
  cfg.reserve(claim_config_in.size());

  for (const auto& rule : claim_config_in) {
    const auto& total_in = rule.total;
    const auto& per_in   = rule.per;

    CHECKC(total_in.contract == per_in.contract, err::ACCOUNT_INVALID, "contract mismatch in rule");
    CHECKC(total_in.quantity.symbol == per_in.quantity.symbol, err::SYMBOL_MISMATCH, "symbol mismatch in rule");
    CHECKC(per_in.quantity.amount > 0, err::NOT_POSITIVE, "per_user must be positive");

    assert_token_allowed(get_self(), per_in.contract, per_in.quantity.symbol);

    token_rule_t norm;
    norm.total = extended_asset{ asset{0, per_in.quantity.symbol}, per_in.contract };
    norm.per   = per_in;
    cfg.push_back(norm);
  }

  plans_t plans(get_self(), get_self().value);
  _gstate.last_plan_id += 1;

  plans.emplace(get_self(), [&](auto& p){
    p.id               = _gstate.last_plan_id;
    p.plan_started_at  = started_at;
    p.plan_ended_at    = ended_at;
    p.claim_config     = cfg;
    p.claimed_cnt      = 0;
    p.created_at       = now();
    p.updated_at       = p.created_at;
  });

}

// ---------- 修改计划 ----------
void airdrop::setplan(const uint64_t& plan_id,
                      std::optional<time_point> started_at,
                      std::optional<time_point> ended_at,
                      std::optional<std::vector<token_rule_t>> claim_config_in) {
  check(
        has_auth(_gstate.admin) || has_auth(get_self()),
        "[[16]] requires admin or self auth"
    );

  plans_t plans(get_self(), get_self().value);
  auto it = get_plan(plans, plan_id);

  plans.modify(it, same_payer, [&](auto& p){
    if (started_at) p.plan_started_at = *started_at;
    if (ended_at)   p.plan_ended_at   = *ended_at;
    CHECKC(p.plan_started_at <= p.plan_ended_at, err::VAILD_TIME_INVALID, "time window invalid");

    if (claim_config_in) {
      const auto& nc = *claim_config_in;
      CHECKC(nc.size() == p.claim_config.size(), err::INVALID_FORMAT, "claim_config size mismatch");

      for (size_t i = 0; i < nc.size(); ++i) {
        const auto& oldT = p.claim_config[i].total;
        const auto& oldP = p.claim_config[i].per;
        const auto& newT = nc[i].total;
        const auto& newP = nc[i].per;

        // total 完全不可变
        CHECKC(newT.contract == oldT.contract, err::ACCOUNT_INVALID, "total.contract changed");
        CHECKC(newT.quantity.symbol == oldT.quantity.symbol, err::SYMBOL_MISMATCH, "total.symbol changed");
        CHECKC(newT.quantity.amount == oldT.quantity.amount, err::INVALID_FORMAT, "total.amount changed");

        // per 只能改数量 & 必须与 total 对齐
        CHECKC(newP.contract == oldT.contract, err::ACCOUNT_INVALID, "per.contract mismatch with total");
        CHECKC(newP.quantity.symbol == oldT.quantity.symbol, err::SYMBOL_MISMATCH, "per.symbol mismatch with total");
        CHECKC(newP.quantity.amount > 0, err::NOT_POSITIVE, "per_user must be positive");

        p.claim_config[i].per = newP;
      }
    }

    p.updated_at = now();
  });
}

// ---------- 领取（仅 oracle） ----------
void airdrop::claimairdrop(const uint64_t& plan_id,
                           const name&     claimer,
                           const std::string& memo) {
  CHECKC(is_oracle(_gstate.oracles, claimer), err::DID_NOT_AUTH, "only oracle can claim");
  CHECKC(!memo.empty(), err::INVALID_FORMAT, "memo required");

  // memo: "user:<account>"
  auto parts = split(memo, ":");
  CHECKC(parts.size() == 2 && parts[0] == "user", err::INVALID_FORMAT, "memo must be 'user:<account>'");
  const name beneficiary(parts[1]);
  CHECKC(beneficiary.value && is_account(beneficiary), err::ACCOUNT_INVALID, "invalid beneficiary");

  plans_t plans(get_self(), get_self().value);
  auto it = get_plan(plans, plan_id);

  const auto t = now();
  CHECKC(t >= it->plan_started_at && t <= it->plan_ended_at, err::EXPIRED, "plan not in active window");

  plans.modify(it, same_payer, [&](auto& p){
    uint32_t paid_cnt = 0;

    for (auto& rule : p.claim_config) {
      const auto& per = rule.per;
      auto&       tot = rule.total;

      CHECKC(tot.contract == per.contract, err::ACCOUNT_INVALID, "contract mismatch in rule");
      CHECKC(tot.quantity.symbol == per.quantity.symbol, err::SYMBOL_MISMATCH, "symbol mismatch in rule");
      CHECKC(per.quantity.amount > 0, err::NOT_POSITIVE, "per must be positive");

      if ( (tot.quantity.amount - per.quantity.amount) > 0 ) {

        TRANSFER( tot.contract,beneficiary, per.quantity,
          std::string("airdrop:") + std::to_string(plan_id) + ":" + claimer.to_string()
        )
        // action(
        //   permission_level{ get_self(), "active"_n },
        //   tot.contract, "transfer"_n,
        //   std::make_tuple(
        //     get_self(),
        //     beneficiary,
        //     per.quantity,
        //     std::string("airdrop:") + std::to_string(plan_id) + ":" + claimer.to_string()
        //   )
        // ).send();

        tot.quantity -= per.quantity;
        ++paid_cnt;
      }
    }

    CHECKC(paid_cnt > 0, err::INSUFFICIENT_QUANTITY, "no token has sufficient balance");

    p.claimed_cnt += 1;
    p.updated_at   = now();
  });
}

// ---------- 入金路由 ----------
void airdrop::ontransfer(name from, name to, asset quantity, std::string memo) {
  if (to != get_self() || from == get_self()) return;

  CHECKC(quantity.amount > 0, err::NOT_POSITIVE, "quantity must be positive");
  CHECKC(quantity.symbol.is_valid(), err::INVALID_FORMAT, "invalid symbol");
  const name token_bank = get_first_receiver();

  // 白名单币
  assert_token_allowed(get_self(), token_bank, quantity.symbol);

  // memo: "add:<plan_id>"
  auto parts = split(memo, ":");
  CHECKC(parts.size() == 2 && parts[0] == "add", err::INVALID_FORMAT, "memo must be 'add:<plan_id>'");
  const uint64_t plan_id = to_uint64(parts[1], "plan_id");

  plans_t plans(get_self(), get_self().value);
  auto it = get_plan(plans, plan_id);

  plans.modify(it, same_payer, [&](auto& p){
    bool matched = false;
    for (auto& r : p.claim_config) {
      if (r.total.contract == token_bank && r.total.quantity.symbol == quantity.symbol) {
        r.total.quantity += quantity;
        matched = true;
      }
    }
    CHECKC(matched, err::STATUS_MISMATCH, "plan has no such token rule");
    p.updated_at = now();
  });
}

} // namespace flon