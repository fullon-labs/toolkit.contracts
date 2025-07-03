
#include <flon.token.hpp>
#include "redpack.hpp"
#include <did.ntoken/did.ntoken.db.hpp>
#include "utils.hpp"
#include <algorithm>
#include <chrono>
#include <eosio/transaction.hpp>
#include <eosio/crypto.hpp>


using std::chrono::system_clock;
using namespace wasm;
using namespace flon;

static constexpr eosio::name active_permission{"active"_n};

// transfer out from contract self
#define TRANSFER_OUT(bank, to, quantity, memo) \
    { action(permission_level{get_self(), "active"_n }, bank, "transfer"_n, std::make_tuple( _self, to, quantity, memo )).send(); }

inline int64_t get_precision(const symbol &s) {
    int64_t digit = s.precision();
    CHECKC(digit >= 0 && digit <= 18, err::SYMBOL_MISMATCH, "precision digit " + std::to_string(digit) + " should be in range[0,18]");
    return calc_precision(digit);
}

inline int64_t get_precision(const asset &a) {
    return get_precision(a.symbol);
}

void redpack::delisttoken( const uint64_t& token_id ) {
    require_auth( _gstate.admin );

    auto token = tokenlist_t( token_id );
    CHECKC( _db.get( token ), err::RECORD_NO_FOUND, "no such token id: " + to_string( token_id ))
    _db.del( token );
}

void redpack::listtoken(const name& contract, const symbol& sym, const time_point_sec& expired_time ) {
    require_auth( _gstate.admin );
    
    tokenlist_t::idx_t tokenlist_tbl(_self, _self.value);
    auto tokenlist_index = tokenlist_tbl.get_index<"by.consymb"_n>();
    uint128_t sec_index = get_unionid(contract, sym.raw());
    auto tokenlist_iter = tokenlist_index.find(sec_index);
    auto found          = tokenlist_iter != tokenlist_index.end();
    auto tid            = found ? tokenlist_iter->id : tokenlist_tbl.available_primary_key();
    
    auto token          = tokenlist_t( tid );
    _db.get( token );

    token.token_contract    = contract;
    token.token_symbol      = sym;
    
    _db.set(token, _self);
}

void redpack::on_token_transfer( const name& from, const name& to, const asset& quantity, const string& memo)
{
    _token_transfer( from, to, quantity, memo );
}

void redpack::on_mtoken_transfer( const name& from, const name& to, const asset& quantity, const string& memo)
{
    _token_transfer( from, to, quantity, memo );
}

void redpack::on_tychetoken_transfer( const name& from, const name& to, const asset& quantity, const string& memo) 
{ 
    _token_transfer( from, to, quantity, memo ); 
}

// deposit redpack memo format: [ wrap:$code:$type:$did_required:$count:$password_hash ]
// fee payment memo format: [ fee:$code:$fee_type ]
void redpack::_token_transfer(const name& from, const name& to, const asset& quantity, const string& memo)
{
    if (from == _self || to != _self) return;

    CHECKC(quantity.amount > 0, err::NOT_POSITIVE, "quantity must be positive");
    CHECKC(is_account(from), err::ACCOUNT_INVALID, "account invalid");

    auto parts = split(memo, ":");
    CHECKC(parts.size() >= 1, err::INVALID_FORMAT, "invalid memo format");

    const auto& action = parts[0];

    if (action == "awrap") { //admin wrap
        CHECKC(parts.size() >= 6, err::INVALID_FORMAT, "invalid wrap format");
        _handle_deposit(from, quantity, parts);
    } else if (action == "fee") {
        CHECKC(parts.size() >= 3, err::INVALID_FORMAT, "invalid fee format");
        _handle_fee_payment(quantity, parts);
    } else if (action == "uwrap") {//user wrap
        CHECKC(false, err::UNDER_MAINTENANCE, "userwrap is under maintenance");
    } else {
        CHECKC(false, err::INVALID_FORMAT, "unknown memo action: " + std::string(action));
    }
}

// -------------------------- 内部方法分离 ------------------------------
// memo format: [ wrap:$code:$type:$did_required:$count:$password_hash:$contract ]
void redpack::_handle_deposit(const name& from, const asset& quantity, const vector<string>& parts) {
    CHECKC(parts.size() >= 6, err::INVALID_FORMAT, "invalid wrap memo format");

    auto token_contract = get_first_receiver();

    // 校验 token 是否在白名单中
    tokenlist_t::idx_t tokenlist_tbl(_self, _self.value);
    auto tokenlist_index = tokenlist_tbl.get_index<"by.consymb"_n>();
    uint128_t consymb_id = get_unionid(token_contract, quantity.symbol.raw());
    auto iter = tokenlist_index.find(consymb_id);
    CHECKC(iter != tokenlist_index.end(), err::RECORD_NO_FOUND, "redpack token not listed");

    // parts[1]: code
    name code = name(parts[1]);
    redpack_t redpack(code);
    CHECKC(!_db.get(redpack), err::REDPACK_EXIST, "code already exists");

    // parts[2]: type
    auto rp_type = name(stoi(parts[2]));
    CHECKC(rp_type == redpack_type::RANDOM || rp_type == redpack_type::MEAN, err::TYPE_INVALID, "redpack type invalid");

    // parts[3]: did_required
    bool did_required = (stoi(parts[3]) == 1);

    // parts[4]: count
    int count = stoi(parts[4]);
    CHECKC(count > 0, err::NOT_POSITIVE, "redpack count must be positive");
    CHECKC(count <= 10000, err::AMOUNT_TOO_LARGE, "redpack count must be no greater than 10000");

    // parts[5]: password_hash
    string passwd_hash = parts[5];

    auto symb = quantity.symbol.code().to_string();
    CHECKC(quantity.amount / count >= 100, err::INSUFFICIENT_QUANTITY, "Minimal unit 100 " + symb + " required");

    if (did_required) {
        CHECKC(symb == "FLON" || symb == "USDT" || symb == "USDC" || symb == "TYCHE",
               err::DID_PACK_SYMBOL_ERR, "DID redpack tokens can only be FLON|MUSDT|MUSDC|TYCHE");
    }

    // 保存红包信息
    redpack_t::idx_t redpacks(_self, _self.value);
    auto now = current_time_point();
    redpacks.emplace(_self, [&](auto& row) {
        row.code            = code;
        row.type            = rp_type;
        row.wrapper         = from;
        row.passwd_hash     = passwd_hash;
        row.token_contract  = token_contract;
        row.total_quant     = quantity;
        row.total_count     = count;
        row.remaining_quant = quantity;
        row.remaining_count = count;
        row.status          = _gstate.unwrap_fee_required ? redpack_status::CREATED
                                                         : redpack_status::SERVICING;
        row.did_required    = did_required;
        row.created_at      = now;
        row.updated_at      = now;
    });
}

void redpack::_handle_fee_payment(const asset& fee_quant, const vector<string>& parts) {
    // 处理手续费支付逻辑
    CHECKC( _gstate.unwrap_unit_fee.amount > 0, err::FEE_NOT_REQUIRED, "zero fee charged" )
    CHECKC( _gstate.unwrap_fee_required, err::FEE_NOT_REQUIRED, "fee not required" )
    CHECKC( fee_quant.symbol == SYS_SYMBOL, err::SYMBOL_MISMATCH, "Only FLON fees are accepted" )
    
    name code = name(parts[0]);
    redpack_t redpack(code);
    CHECKC( redpack.unwrapped_by_admin, err::ACCOUNT_INVALID, "" )

    CHECKC( _db.get(redpack), err::REDPACK_EXIST, "redpack not yet created" )
    
    
    auto fee_required = redpack.total_count * _gstate.unwrap_unit_fee;
    CHECKC( fee_quant.amount >= fee_required.amount, err::FEE_NOT_POSITIVE, "insufficient fee: " + fee_required.to_string() )

    TRANSFER_OUT( SYS_BANK, _gstate.admin, fee_quant, "fee to redpack: " + code.to_string() )

    // 更新红包状态为 SERVICING
    CHECKC( redpack.status == redpack_status::CREATED, err::EXPIRED, "redpack status <> CREATED" )
    redpack.status = redpack_status::SERVICING; // 更新红包状态为 SERVICING
    redpack.updated_at = current_time_point();
    _db.set(redpack, _self);
}

void redpack::claimredpack(const name& claimer, const name& code, const string& pwhash)
{
    // 1. 权限校验：只有 admin 可代领（防止暴露口令被脚本爆破）
    require_auth(_gstate.admin);
    // 如果未来需要开放给 claimer 自主领取，可取消注释以下行
    // CHECKC(has_auth(claimer) || has_auth(_gstate.admin), err::ACCOUNT_INVALID, "auth failed: not claimer nor admin");

    // 2. 查询红包主记录
    redpack_t redpack(code);
    CHECKC(_db.get(redpack), err::RECORD_NO_FOUND, "redpack not found");

    // 3. 校验口令哈希
    CHECKC(redpack.passwd_hash == pwhash, err::PWHASH_INVALID, "incorrect password");

    // 4. 校验红包状态
    CHECKC(redpack.status == redpack_status::SERVICING, err::EXPIRED, "redpack not available for claiming");

    // 5. DID 红包校验
    if (redpack.did_required) {
        const auto& required_dids = _gstate.dids; // set<uint64_t>
        CHECKC(!required_dids.empty(), err::DID_NOT_SUPPORTED, "DID not supported");

        auto claimer_acnts = flon::account_t::idx_t(_gstate.did_contract, claimer.value);

        // 直接遍历 claimer 的 DID，逐个判断是否在 required_dids 中，匹配即退出
        bool matched = false;
        for (const auto& acc : claimer_acnts) {
            if (acc.balance.amount > 0) {
                uint64_t did = acc.balance.symbol.raw();
                if (required_dids.count(did)) {
                    matched = true;
                    break;
                }
            }
        }

        CHECKC(matched, err::DID_NOT_AUTH, "DID not authorized");
    }

    // 6. 防止重复领取（索引: claimer + code）
    claim_t::idx_t claims(_self, _self.value);
    auto claims_index = claims.get_index<"by.unionid"_n>();
    uint128_t union_id = get_unionid(claimer, code.value);
    CHECKC(claims_index.find(union_id) == claims_index.end(), err::NOT_REPEAT_RECEIVE, "already claimed");

    // 7. 计算应领取数量
    asset assigned_quant;
    if (redpack.type == redpack_type::RANDOM) {
        _assign_redpack(redpack, assigned_quant);
    } else if (redpack.type == redpack_type::MEAN) {
        assigned_quant = (redpack.remaining_count == 1)
                         ? redpack.remaining_quant
                         : redpack.total_quant / redpack.total_count;
    } else {
        CHECKC(false, err::TYPE_INVALID, "invalid redpack type");
    }

    // 8. 安全校验
    CHECKC(redpack.remaining_quant >= assigned_quant, err::INSUFFICIENT_QUANTITY, "not enough quantity left");
    CHECKC(redpack.remaining_count > 0, err::INSUFFICIENT_QUANTITY, "no redpack left");

    // 9. 发放红包（合约转账）
    TRANSFER_OUT(redpack.token_contract, claimer, assigned_quant, "redpack: " + code.to_string());

    // 10. 更新红包主表
    redpack.remaining_quant -= assigned_quant;
    redpack.remaining_count--;
    if (redpack.remaining_count == 0) {
        redpack.status = redpack_status::FINISHED;
    }
    redpack.updated_at = current_time_point();
    _db.set(redpack, _self);

    // 11. 保存领取记录
    claims.emplace(_self, [&](auto& row) {
        row.id              = claims.available_primary_key();
        row.redpack_code    = code;
        row.redpack_wrapper = redpack.wrapper;
        row.claimer         = claimer;
        row.quantity        = assigned_quant;
        row.claimed_at      = current_time_point();
    });
}

// revert redpack to the creator
// if the redpack is not expired, the creator can get back the remain quantity
void redpack::cancel( const name& code )
{
    redpack_t redpack(code);
    CHECKC( _db.get(redpack), err::RECORD_NO_FOUND, "redpack not found" );
    CHECKC( current_time_point() > redpack.created_at + eosio::hours(_gstate.redpack_expiry_hours), err::NOT_EXPIRED, 
            "expiration is not reached" )

    CHECKC( redpack.status == redpack_status::CREATED, err::EXPIRED, "redpack has expired" )

    TRANSFER_OUT(redpack.token_contract, redpack.wrapper, redpack.remaining_quant, string("redpack remaining returned") )
    
    _db.del(redpack);
}

void redpack::delclaims( const uint64_t& max_rows )
{
    set<name> none_exist_list;

    claim_t::idx_t claim_idx(_self, _self.value);
    auto claim_itr = claim_idx.begin();

    size_t count = 0;
    for (; count < max_rows && claim_itr != claim_idx.end(); ) {

        bool redpack_none_exist = none_exist_list.count(claim_itr->redpack_code) > 0 ? true : false;
        if (!redpack_none_exist){
            redpack_t redpack(claim_itr->redpack_code);
            redpack_none_exist = !_db.get(redpack);
            if (redpack_none_exist){
                claim_itr = claim_idx.erase(claim_itr);
                none_exist_list.insert(claim_itr->redpack_code);
                count++;
            } else {
                break;
            }
        } else {
            claim_itr = claim_idx.erase(claim_itr);
            count++;
        }
    }
    CHECKC(count > 0, err::NONE_DELETED, "delete invalid");
}

void redpack::_assign_redpack(const redpack_t& redpack, asset& assigned) {
    // calc order quantity value by price
    if ( redpack.remaining_count == 1 ) {
        assigned = redpack.remaining_quant;
        return;
    }

    uint64_t quantity = redpack.remaining_quant.amount / redpack.remaining_count * 2;
    uint8_t precision = 0;
    if (redpack.remaining_quant.symbol.precision() <= 2)
        precision = 0;
    else
        precision = redpack.remaining_quant.symbol.precision() - 2;

    assigned = asset( _rand(asset(quantity, redpack.remaining_quant.symbol), precision), redpack.remaining_quant.symbol );
}

uint64_t redpack::_rand(asset max_quantity, uint16_t min_unit) {
    // 1. 获取区块级伪随机种子（不是真正随机，仅限奖励分配等非强安全场景）
    uint64_t mixed = tapos_block_prefix() * tapos_block_num();
    auto hash = sha256(reinterpret_cast<const char*>(&mixed), sizeof(mixed));

    // 2. 使用 hash 前 8 字节作为伪随机整数种子
    uint64_t seed = 0;
    memcpy(&seed, hash.data(), sizeof(seed));  // 提高分布质量

    // 3. 构造精度相关参数
    int precision = max_quantity.symbol.precision();              // 例如 6（支持 0 ~ 18）
    int64_t unit_base = power10(precision);                       // 10^precision
    int64_t min_unit_value = unit_base / power10(min_unit);      // 最小单位整数值，如 10^6 / 10^2 = 10^4

    // 4. 可分配最大单位数（红包最大数量）
    int64_t max_units = max_quantity.amount / min_unit_value;

    // 5. 生成随机整数单位数，范围 [1, max_units]
    uint64_t rand_units = (seed % max_units) + 1;  // 保证至少 1

    // 6. 返回最终随机金额
    return rand_units * min_unit_value;
}