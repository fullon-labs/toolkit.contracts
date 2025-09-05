#include "flon.airdrop.hpp"
#include <algorithm>
#include "utils.hpp"

using std::string;
using std::vector;
using std::optional;

namespace flon {

static void assert_token_allowed(const name self, const name token_bank, const symbol sym) {
    tokens_t tokens(self, self.value);
    auto idx = tokens.get_index<"bybankcode"_n>();
    auto it  = idx.find(((uint128_t)token_bank.value << 64) | sym.code().raw());
    check(it != idx.end() && it->enabled, "token not allowed");
}

static plan_t get_plan_or_fail(const name self, uint64_t plan_id) {
    plans_t plans(self, self.value);
    auto pit = plans.find(plan_id);
    check(pit != plans.end(), "plan not found");
    const auto now = current_time_point();
    check(now >= pit->started_at, "plan not started");
    check(now <= pit->ended_at,   "plan expired");
    if (pit->max_claims > 0) {
        check(pit->claimed_cnt < pit->max_claims, "plan reached max claims");
    }
    return *pit; // 注意：返回的是快照，修改需重新取迭代器
}

static bool str_stou64(const string& s, uint64_t& out) {
    if (s.empty()) return false;
    uint64_t v = 0;
    for (char c : s) {
        if (c < '0' || c > '9') return false;
        uint64_t d = static_cast<uint64_t>(c - '0');
        uint64_t nv = v * 10 + d;
        if (nv < v) return false; // 溢出
        v = nv;
    }
    out = v;
    return true;
}

static std::pair<uint64_t,uint64_t> parse_memo_plan_item(const string& memo) {
    // 允许格式（顺序不限，大小写严格）：
    // "plan:<id>|item:<id>" 或 "item:<id>|plan:<id>"
    uint64_t plan_id = 0, item_id = 0;
    size_t ppos = memo.find("plan:");
    size_t ipos = memo.find("item:");

    check(ppos != string::npos && ipos != string::npos, "invalid memo: need plan: and item:");
    auto parse_after = [&](size_t pos) -> uint64_t {
        size_t begin = pos;
        while (begin < memo.size() && memo[begin] != ':') ++begin;
        check(begin < memo.size() && memo[begin] == ':', "invalid memo: colon missing");
        ++begin;
        size_t end = begin;
        while (end < memo.size() && isdigit(memo[end])) ++end;
        uint64_t v = 0;
        check(str_stou64(memo.substr(begin, end - begin), v), "invalid memo: id parse failed");
        return v;
    };

    plan_id = parse_after(ppos);
    item_id = parse_after(ipos);
    return {plan_id, item_id};
}


void airdrop::init(const name& admin,
                   const name& rwid_contract,
                   const name& oracle_account)
{
    require_auth(get_self());

    CHECKC(is_account(admin),           err::ACCOUNT_INVALID, "admin account not exist");
    CHECKC(is_account(rwid_contract),   err::ACCOUNT_INVALID, "rwid_contract not exist");
    CHECKC(is_account(oracle_account),  err::ACCOUNT_INVALID, "oracle_account not exist");

    _gstate.admin          = admin;
    _gstate.rwid_contract  = rwid_contract;
    _gstate.oracle_account = oracle_account;
}

void airdrop::addtoken(const name& token_bank, const symbol& sym) {
    require_auth(_gstate.admin);
    CHECKC(token_bank.value != 0, err::ACCOUNT_INVALID, "token_bank empty");
    CHECKC(sym.is_valid(), err::INVALID_FORMAT, "invalid symbol");

    tokens_t tokens(get_self(), get_self().value);
    auto idx = tokens.get_index<"bybankcode"_n>();
    auto key = ((uint128_t)token_bank.value << 64) | sym.code().raw();
    auto it = idx.find(key);
    const auto now = current_time_point();

    if (it == idx.end()) {
        tokens.emplace(get_self(), [&](auto& r){
        r.id         = tokens.available_primary_key();
        r.token_bank = token_bank;
        r.sym        = sym;
        r.enabled    = true;
        r.created_at = now;
        r.updated_at = now;
        });
    } else {
        idx.modify(it, get_self(), [&](auto& r){
        r.enabled    = true;
        r.updated_at = now;
        });
    }
}

void airdrop::enabletoken(const name& token_bank, const symbol& sym, const bool& enabled) {
    require_auth(_gstate.admin);

    tokens_t tokens(get_self(), get_self().value);
    auto idx = tokens.get_index<"bybankcode"_n>();
    auto it  = idx.find(((uint128_t)token_bank.value << 64) | sym.code().raw());
    CHECKC(it != idx.end(), err::RECORD_NO_FOUND, "token not found");

    idx.modify(it, get_self(), [&](auto& r){
        r.enabled    = enabled;
        r.updated_at = current_time_point();
    });
}

void airdrop::deltoken(const name& token_bank, const symbol& sym) {
    require_auth(_gstate.admin);

    tokens_t tokens(get_self(), get_self().value);
    auto idx = tokens.get_index<"bybankcode"_n>();
    auto it  = idx.find(((uint128_t)token_bank.value << 64) | sym.code().raw());
    CHECKC(it != idx.end(), err::RECORD_NO_FOUND, "token not found");
    idx.erase(it);
}

void airdrop::newplan(time_point         started_at,
                      time_point         ended_at,
                      vector<name>       auth_types,
                      const uint64_t&    max_claims,
                      const uint32_t&    item_limit)
{
    require_auth(_gstate.admin);

    CHECKC(started_at <= ended_at, err::VAILD_TIME_INVALID, "time window invalid");
    for (auto n : auth_types) {
        CHECKC(n == AuthType::RWID || n == AuthType::ORACLE,
            err::TYPE_INVALID, "auth type invalid");
    }

    plans_t plans(get_self(), get_self().value);
    const auto now = current_time_point();

    // 用全局 last_plan_id 自增生成
    _gstate.last_plan_id += 1;
    uint64_t new_id = _gstate.last_plan_id;

    plans.emplace(get_self(), [&](auto& p){
        p.plan_id          = new_id;
        p.started_at       = started_at;
        p.ended_at         = ended_at;
        p.auth_types       = auth_types;
        p.max_claims       = max_claims;
        p.claimed_cnt      = 0;
        p.claim_item_limit = item_limit;
        p.created_at       = now;
        p.updated_at       = now;
    });
}

void airdrop::setplan(const uint64_t&          plan_id,
                      optional<time_point>     started_at,
                      optional<time_point>     ended_at,
                      optional<vector<name>>   auth_types,
                      optional<uint64_t>       max_claims,
                      optional<uint32_t>       item_limit)
{
    require_auth(_gstate.admin);

    plans_t plans(get_self(), get_self().value);
    auto pit = plans.find(plan_id);
    CHECKC(pit != plans.end(), err::RECORD_NO_FOUND, "plan not found");

    plans.modify(pit, same_payer, [&](auto& p){
        if (started_at) p.started_at = *started_at;
        if (ended_at)   p.ended_at   = *ended_at;
        if (auth_types) {
        for (auto n : *auth_types) {
            CHECKC(n == AuthType::RWID || n == AuthType::ORACLE, err::TYPE_INVALID, "auth type invalid");
        }
        p.auth_types = *auth_types;
        }
        if (max_claims)    p.max_claims       = *max_claims;
        if (item_limit)    p.claim_item_limit = *item_limit;
        CHECKC(p.started_at <= p.ended_at, err::VAILD_TIME_INVALID, "time window invalid");
        p.updated_at = current_time_point();
    });
}

void airdrop::additem(const uint64_t&   plan_id,
                      const name&       token_bank,
                      const symbol&     sym,
                      const asset&      quant,
                      optional<asset>   per_user_cap,
                      const uint64_t&   item_max_claims)
{
    require_auth(_gstate.admin);

    (void)get_plan_or_fail(get_self(), plan_id);

    CHECKC(is_account(token_bank),            err::ACCOUNT_INVALID, "token_bank not exist");
    CHECKC(sym.is_valid(),                    err::INVALID_FORMAT,  "invalid symbol");
    CHECKC(quant.symbol == sym,               err::SYMBOL_MISMATCH, "quant symbol mismatch");
    CHECKC(quant.amount > 0,                  err::NOT_POSITIVE,    "quant must be positive");

    // 白名单：(token_bank, sym) 必须启用
    assert_token_allowed(get_self(), token_bank, sym);

    items_t items(get_self(), plan_id);
    const auto now = current_time_point();

    // === 唯一性：同一 plan 内，同一 (token_bank, symbol+precision) 只能有一条 ===
    auto bybp = items.get_index<"bybankprec"_n>();
    const uint128_t key =
        ( (uint128_t)token_bank.value << 64 )
      | ( ((uint128_t)sym.code().raw() << 8) | sym.precision() );
    CHECKC(bybp.find(key) == bybp.end(),
           err::STATUS_MISMATCH,
           "item with same symbol already exists in this plan");

    _gstate.last_item_id += 1;
    const uint64_t new_item_id = _gstate.last_item_id;

    items.emplace(get_self(), [&](auto& r){
        r.item_id          = new_item_id;
        r.token_bank       = token_bank;
        r.sym              = sym;
        r.quant            = quant;
        r.deposited        = asset{0, sym};
        r.paid_total       = asset{0, sym};
        r.item_max_claims  = item_max_claims;
        r.item_claimed_cnt = 0;
        r.per_user_cap     = per_user_cap.value_or(asset{0, sym});
        CHECKC(r.per_user_cap.symbol == sym, err::SYMBOL_MISMATCH, "per_user_cap symbol mismatch");
        CHECKC(r.per_user_cap.amount >= 0,   err::INVALID_FORMAT,  "per_user_cap must be non-negative");
        r.created_at       = now;
        r.updated_at       = now;
    });
}

void airdrop::setitem(const uint64_t&   plan_id,
                      const uint64_t&   item_id,
                      optional<asset>   quant,
                      optional<asset>   per_user_cap,
                      optional<uint64_t> item_max_claims)
{
    require_auth(_gstate.admin);

    items_t items(get_self(), plan_id);
    auto it = items.find(item_id);
    CHECKC(it != items.end(), err::RECORD_NO_FOUND, "item not found");

    items.modify(it, same_payer, [&](auto& r){
        if (quant) {
        CHECKC(quant->symbol == r.sym, err::SYMBOL_MISMATCH, "quant symbol mismatch");
        CHECKC(quant->amount > 0,      err::NOT_POSITIVE,    "quant must be positive");
        r.quant = *quant;
        }
        if (per_user_cap) {
        CHECKC(per_user_cap->symbol == r.sym, err::SYMBOL_MISMATCH, "cap symbol mismatch");
        CHECKC(per_user_cap->amount >= 0,     err::INVALID_FORMAT,  "cap must be non-negative");
        r.per_user_cap = *per_user_cap;
        }
        if (item_max_claims) r.item_max_claims = *item_max_claims;

        r.updated_at = current_time_point();
    });
}

void airdrop::delitem(const uint64_t& plan_id, const uint64_t& item_id) {
    require_auth(_gstate.admin);

    items_t items(get_self(), plan_id);
    auto it = items.find(item_id);
    CHECKC(it != items.end(), err::RECORD_NO_FOUND, "item not found");
    CHECKC(it->deposited.amount == 0 && it->paid_total.amount == 0, err::STATUS_MISMATCH, "cannot delete funded/used item");

    items.erase(it);
}

void airdrop::claimairdrop(const uint64_t&    plan_id,
                          const name&        claimer,
                          optional<uint64_t> prefer_item_id,
                          const std::string& memo)
{
    require_auth(claimer);
    CHECKC(is_account(claimer), err::ACCOUNT_INVALID, "claimer not exist");

    auto plan = get_plan_or_fail(get_self(), plan_id);
    const bool need_oracle =
        std::find(plan.auth_types.begin(), plan.auth_types.end(), AuthType::ORACLE) != plan.auth_types.end();
    const bool need_rwid =
        std::find(plan.auth_types.begin(), plan.auth_types.end(), AuthType::RWID)   != plan.auth_types.end();
    const bool oracle_cfg      = (_gstate.oracle_account.value != 0);
    const bool is_oracle_claim = (oracle_cfg && claimer == _gstate.oracle_account);

    // 先基于 is_oracle_claim 确定受益人
    name beneficiary;
    if (is_oracle_claim) {
        // 代领：memo 必须是 "user:<account>"
        CHECKC(!memo.empty(), err::INVALID_FORMAT, "memo must contain beneficiary when oracle claims");
        CHECKC(memo.rfind("user:", 0) == 0, err::INVALID_FORMAT, "memo must be 'user:<account>'");

        std::string s = memo.substr(5); // 去掉 "user:"
        auto ltrim=[&](std::string& x){ while(!x.empty() && isspace((unsigned char)x.front())) x.erase(x.begin()); };
        auto rtrim=[&](std::string& x){ while(!x.empty() && isspace((unsigned char)x.back()))  x.pop_back(); };
        ltrim(s); rtrim(s);

        beneficiary = name(s);
        CHECKC(is_account(beneficiary), err::ACCOUNT_INVALID, "beneficiary not exist");
    } else {
        // 自领：受益人 = 本人
        beneficiary = claimer;
    }

    // ORACLE-only：只能 oracle 发起
    if (need_oracle && !need_rwid) {
        CHECKC(oracle_cfg, err::ACCOUNT_INVALID, "oracle not configured");
        CHECKC(is_oracle_claim, err::DID_NOT_AUTH, "only oracle can claim for this plan");
    }
    // RWID-only：必须自领，禁止 oracle 代领
    if (need_rwid && !need_oracle) {
        CHECKC(!is_oracle_claim, err::DID_NOT_AUTH, "self-claim only for RWID plan");
    }
    // Mixed（RWID+ORACLE）：两条路均可（自领需 RWID，代领不需 RWID）

    // 认证细则
    // 自领 + 需要 RWID → 校验 RWID
    if (need_rwid && !is_oracle_claim) {
        CHECKC(_gstate.rwid_contract.value != 0, err::DID_NOT_SUPPORTED, "rwid not configured");
        flon::account_rwid_t::idx_t accts(_gstate.rwid_contract, _gstate.rwid_contract.value);
        CHECKC(accts.find(beneficiary.value) != accts.end(), err::DID_NOT_AUTH, "user not rwid verified");
    }

    // 幂等：每计划每用户一次
    claims_t claims(get_self(), plan_id);
    auto byuser = claims.get_index<"byuser"_n>();
    CHECKC(byuser.find(beneficiary.value) == byuser.end(), err::NOT_REPEAT_RECEIVE, "already claimed");

    // 选条目（遵循 claim_item_limit；余额/条目人限/人均上限）
    items_t items(get_self(), plan_id);
    std::vector<item_t> selected;

    auto can_claim = [&](const item_t& it)->bool {
        // 余额足够
        if ((it.deposited.amount - it.paid_total.amount) < it.quant.amount) return false;
        // 条目人数上限
        if (it.item_max_claims > 0 && it.item_claimed_cnt >= it.item_max_claims) return false;
        // 每人上限（0 不限；>0 则 quant <= cap）
        if (it.per_user_cap.amount > 0 && it.quant.amount > it.per_user_cap.amount) return false;
        return true;
    };

    auto pick_one_by_id = [&](uint64_t item_id) {
        auto it = items.find(item_id);
        CHECKC(it != items.end(), err::RECORD_NO_FOUND, "item not found");
        CHECKC(can_claim(*it),     err::INSUFFICIENT_QUANTITY, "item not claimable");
        selected.push_back(*it);
    };

    if (plan.claim_item_limit == 1) {
        if (prefer_item_id.has_value()) {
        pick_one_by_id(*prefer_item_id);
        } else {
        bool found=false;
        for (auto it = items.begin(); it != items.end(); ++it) {
            if (can_claim(*it)) { selected.push_back(*it); found=true; break; }
        }
        CHECKC(found, err::INSUFFICIENT_QUANTITY, "no claimable item");
        }
    } else {
        for (auto it = items.begin(); it != items.end(); ++it)
        if (can_claim(*it)) selected.push_back(*it);
        CHECKC(!selected.empty(), err::INSUFFICIENT_QUANTITY, "no claimable items");
    }

    // 发放 & 更新条目
    vector<asset> rewards;
    for (const auto& it : selected) {
        action(
        permission_level{get_self(), "active"_n},
        it.token_bank, "transfer"_n,
        std::make_tuple(get_self(), beneficiary, it.quant, std::string("airdrop"))
        ).send();

        auto mit = items.find(it.item_id);
        items.modify(mit, same_payer, [&](auto& r){
        r.paid_total       += it.quant;
        r.item_claimed_cnt += 1;
        r.updated_at        = current_time_point();
        });

        rewards.push_back(it.quant);
        if (plan.claim_item_limit == 1) break;
    }

    // 更新计划统计
    {
        plans_t plans(get_self(), get_self().value);
        auto pit = plans.find(plan_id);
        plans.modify(pit, same_payer, [&](auto& p){
        p.claimed_cnt += 1;
        p.updated_at   = current_time_point();
        });
    }

    // 写领取记录
    claims.emplace(get_self(), [&](auto& r){
        r.id                = claims.available_primary_key(); if (r.id == 0) r.id = 1;
        r.claimer           = beneficiary;
        r.claimed_items     = rewards.size();
        r.rewards           = rewards;
        r.claimed_at_us     = current_time_point().time_since_epoch().count();
    });
}

void airdrop::ontransfer(name from, name to, asset quantity, std::string memo)
{
    if (to != get_self() || from == get_self()) return;

    CHECKC(quantity.amount > 0,        err::NOT_POSITIVE,   "quantity must be positive");
    CHECKC(quantity.symbol.is_valid(), err::INVALID_FORMAT, "invalid symbol");

    const name token_bank = get_first_receiver();

    // 仅接收 tokens 白名单（启用）的币
    tokens_t tokens(get_self(), get_self().value);
    const uint128_t bankcode =
        ( (uint128_t)token_bank.value << 64 )
        | ( ((uint128_t)quantity.symbol.code().raw() << 8) | quantity.symbol.precision() );
    auto bybankcode = tokens.get_index<"bybankcode"_n>();
    auto it_token = bybankcode.find(bankcode);
    CHECKC(it_token != bybankcode.end(), err::RECORD_NO_FOUND, "token not allowed");
    CHECKC(it_token->enabled,            err::STATUS_MISMATCH, "token disabled");

    // 解析 memo：add:<plan_id>:<item_id>
    auto parts = split(memo, ":");
    CHECKC(parts.size() == 3, err::INVALID_FORMAT, "memo must be 'add:<plan_id>:<item_id>'");
    CHECKC(parts[0] == "add", err::INVALID_FORMAT, "memo must start with 'add:'");

    const uint64_t plan_id = to_uint64(parts[1], "plan_id");
    const uint64_t item_id = to_uint64(parts[2], "item_id");

    // 计划存在（如需仅校验存在性，可换成你自己的 get_plan_exists）
    (void)get_plan_or_fail(get_self(), plan_id);

    // 定位条目（scope=plan_id）并校验 bank/symbol 一致
    items_t items(get_self(), plan_id);
    auto it_item = items.find(item_id);
    CHECKC(it_item != items.end(),                 err::RECORD_NO_FOUND, "item not found");
    CHECKC(it_item->token_bank == token_bank,      err::ACCOUNT_INVALID, "token bank mismatch with item");
    CHECKC(it_item->sym        == quantity.symbol, err::SYMBOL_MISMATCH, "symbol mismatch with item");

    // 累加入金
    items.modify(it_item, same_payer, [&](auto& r){
        r.deposited += quantity;
        r.updated_at = current_time_point();
    });
}

} // namespace flon