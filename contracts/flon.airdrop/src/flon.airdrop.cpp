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
void airdrop::addplan(const string&             title,
                      const time_point&         started_at,
                      const time_point&         ended_at )
{
    // 允许 admin 或合约自身操作
    check(has_auth(_gstate.admin) || has_auth(get_self()),
          "[[16]] requires admin or self auth");

    // 基本时间校验
    CHECKC(started_at <= ended_at, err::VAILD_TIME_INVALID, "time window invalid");

    // 写入计划
    plans_t plans(get_self(), get_self().value);
    _gstate.last_plan_id += 1;

    const auto nowtp = current_time_point();
    plans.emplace(get_self(), [&](auto& p){
        p.id               = _gstate.last_plan_id;
        p.title            = title;
        p.plan_started_at  = started_at;
        p.plan_ended_at    = ended_at;
        p.created_at       = nowtp;
        p.updated_at       = nowtp;
    });
}

 void airdrop::setclaim(const uint64_t& plan_id, const extended_symbol& symb, const asset& single_claim, const asset& allocated) {
   check(
        has_auth(_gstate.admin) || has_auth(get_self()),
        "[[16]] requires admin or self auth"
    );

    CHECKC( single_claim.symbol == symb.get_symbol(), err::TYPE_INVALID, "single vs symb type mismatch" )
    CHECKC( single_claim.symbol == allocated.symbol, err::TYPE_INVALID, "single vs allocated type mismatch" )
    // 基本校验
    CHECKC(is_account(symb.get_contract()), err::ACCOUNT_INVALID, "token contract not exist: " + symb.get_contract().to_string());
    CHECKC(single_claim.amount > 0,  err::NOT_POSITIVE,    "single_claim must be positive");
    CHECKC(allocated >= single_claim,  err::NOT_POSITIVE,    "allocated - single must be positive");

    // 白名单/允许币种校验
    assert_token_allowed(get_self(), symb.get_contract(), symb.get_symbol());

    plans_t plans(get_self(), get_self().value);
    auto it = get_plan(plans, plan_id);

    plans.modify(it, same_payer, [&](auto& p){
      p.claims[symb] = claim_conf_s { single_claim, allocated };
    });
}

void airdrop::delclaim(const uint64_t& plan_id, const extended_symbol& symb) {
   check(
        has_auth(_gstate.admin) || has_auth(get_self()),
        "[[16]] requires admin or self auth"
    );

    plans_t plans(get_self(), get_self().value);
    auto it = get_plan(plans, plan_id);

    plans.modify(it, same_payer, [&](auto& p){
      p.claims.erase( symb );
    });
}

// ---------- 修改计划 ----------
void airdrop::setplan(const uint64_t& plan_id,
                      std::optional<string> title,
                      std::optional<time_point> started_at,
                      std::optional<time_point> ended_at) {
  check(
        has_auth(_gstate.admin) || has_auth(get_self()),
        "[[16]] requires admin or self auth"
    );

  plans_t plans(get_self(), get_self().value);
  auto it = get_plan(plans, plan_id);

  plans.modify(it, same_payer, [&](auto& p){
    if (title)     p.title           = *title;
    if (started_at) p.plan_started_at = *started_at;
    if (ended_at)   p.plan_ended_at   = *ended_at;

    CHECKC(p.plan_started_at <= p.plan_ended_at,
           err::VAILD_TIME_INVALID, "time window invalid");

    p.updated_at = now();
  });
}

void airdrop::delplan( const uint64_t& plan_id) {
  require_auth( _self );

  plans_t plans(get_self(), get_self().value);
  auto it = get_plan(plans, plan_id);

  plans.erase( it );
}

void airdrop::claim(const uint64_t& plan_id,
                           const name&     claimer,
                           const name&     beneficiary,
                           const std::string& memo) {
  // 必须由 oracle 签名发起
  require_auth(claimer);
  CHECKC(is_oracle(_gstate.oracles, claimer), err::DID_NOT_AUTH,
         "only oracle can claim");

  CHECKC(beneficiary.value && is_account(beneficiary), err::ACCOUNT_INVALID,
         "invalid beneficiary");

  plans_t plans(get_self(), get_self().value);
  auto it = get_plan(plans, plan_id);

  const auto t = now();
  CHECKC(t >= it->plan_started_at && t <= it->plan_ended_at, err::EXPIRED,
         "plan not in active time window");

  plans.modify(it, same_payer, [&](auto& p){
    uint32_t paid_cnt = 0;

    for (auto& claim : p.claims) {
      const auto& claim_symb    = claim.first;
      const auto& single_claim  = claim.second.single_claim;
      auto&    available_claim  = claim.second.available;

      // 足额才能发放（允许刚好等于）
      auto claim_quant = available_claim >= single_claim ? single_claim : available_claim;
      
        TRANSFER(claim_symb.get_contract(),
                 beneficiary,
                 claim_quant,
                 std::string("airdrop:") + std::to_string(plan_id) );

        available_claim -= claim_quant;
        ++paid_cnt;
    }

    CHECKC(paid_cnt > 0, err::INSUFFICIENT_QUANTITY,
           "no token rule has sufficient balance in plan");

    p.claimed_cnt += 1;
    p.updated_at   = now();
  });
}


// // ---------- 入金路由 ----------
// void airdrop::ontransfer(name from, name to, asset quantity, std::string memo) {
//   if (to != get_self() || from == get_self()) return;

//   CHECKC(quantity.amount > 0, err::NOT_POSITIVE, "quantity must be positive");
//   CHECKC(quantity.symbol.is_valid(), err::INVALID_FORMAT, "invalid symbol");
//   const name token_bank = get_first_receiver();

//   // 白名单币
//   assert_token_allowed(get_self(), token_bank, quantity.symbol);

//   // memo: "plan:<plan_id>"
//   auto parts = split(memo, ":");
//   CHECKC(parts.size() == 2 && parts[0] == "plan", err::INVALID_FORMAT, "memo must be 'plan:<plan_id>'");
//   const uint64_t plan_id = to_uint64(parts[1], "plan_id");

//   plans_t plans(get_self(), get_self().value);
//   auto it = get_plan(plans, plan_id);

//   plans.modify(it, same_payer, [&](auto& p){
//     bool matched = false;
//     for (auto& r : p.claims) {
//       if (r.available.contract == token_bank && r.available.quantity.symbol == quantity.symbol) {
//         r.available.quantity += quantity;
//         matched = true;
//       }
//     }
//     CHECKC(matched, err::STATUS_MISMATCH, "plan has no such token rule");
//     p.updated_at = now();
//   });
// }

} // namespace flon