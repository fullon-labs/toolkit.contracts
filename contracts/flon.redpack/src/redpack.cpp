
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

// void redpack::setfee(const extended_asset& fee) {
//     require_auth( _self );
//     CHECKC( fee.quantity.amount>0, err::FEE_NOT_POSITIVE, "fee not positive" );
    
//     _gstate.fee = fee;
// }

void redpack::delisttoken( const uint64_t& token_id ) {
    require_auth( _gstate.admin );

    auto token = tokenlist_t( token_id );
    CHECKC( _db.get( token ), err::RECORD_NO_FOUND, "no such token id: " + to_string( token_id ))
    _db.del( token );
}

void redpack::listtoken(const name& contract, const symbol& sym, const time_point_sec& expired_time ) {
    require_auth( _gstate.admin );
    // int64_t value = FLON::token::get_supply(contract, sym.code()).amount;
    // CHECKC( value > 0, err::SYMBOL_MISMATCH, "symbol mismatch" );
    
    tokenlist_t::idx_t tokenlist_tbl(_self, _self.value);
    auto tokenlist_index = tokenlist_tbl.get_index<"by.syscon"_n>();
    uint128_t sec_index = get_unionid(contract, sym.raw());
    auto tokenlist_iter = tokenlist_index.find(sec_index);
    auto found          = tokenlist_iter != tokenlist_index.end();
    auto tid            = found ? tokenlist_iter->id : tokenlist_tbl.available_primary_key();
    
    auto token          = tokenlist_t( tid );
    _db.get( token );

    token.expired_time  = expired_time;
    token.sym           = sym;
    token.contract      = contract;
    _db.set(token, _self);
}

// issue-in op: transfer tokens to the contract and lock them according to the given plan
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

void redpack::_token_transfer(const name& from, const name& to, const asset& quantity, const string& memo)
{
    if (from == _self || to != _self) return;
    CHECKC(quantity.amount > 0, err::NOT_POSITIVE, "quantity must be positive")

    // memo 解析
    auto parts = split(memo, ":");
    CHECKC(parts.size() == 4, err::INVALID_FORMAT, "invalid memo format, arg-size must be 4" )

    _handle_deposit(from, quantity, parts);
    
}

// -------------------------- 内部方法分离 ------------------------------

void redpack::_handle_deposit(const name& from, const asset& quantity, const vector<string>& parts) {
    name receiver_contract = get_first_receiver();

    // 校验 token 是否可用且未过期
    tokenlist_t::idx_t tokenlist_tbl(_self, _self.value);
    auto tokenlist_index = tokenlist_tbl.get_index<"by.syscon"_n>();
    uint128_t sec_index = get_unionid(receiver_contract, quantity.symbol.raw());
    auto iter = tokenlist_index.find(sec_index);
    CHECKC(iter != tokenlist_index.end(), err::NON_RENEWAL, "non-renewal");
    CHECKC(iter->expired_time > time_point_sec(current_time_point()), err::NON_RENEWAL, "non-renewal");


    // 业务参数 [ $count:$type:$code ]
    name code = name(parts[3]);
    redpack_t redpack(code);
    CHECKC(!_db.get(redpack), err::REDPACK_EXIST, "code already exists");

    int count = stoi(parts[1]);
    auto rp_type = name(stoi(parts[2]));
    CHECKC(rp_type == redpack_type::RANDOM || rp_type == redpack_type::MEAN || rp_type == redpack_type::RANDOM_DID || rp_type == redpack_type::MEAN_DID,
           err::TYPE_INVALID, "redpack type invalid");

    auto symb = quantity.symbol.code().to_string();
    bool is_did_type = (rp_type == redpack_type::RANDOM_DID || rp_type == redpack_type::MEAN_DID);

    if (is_did_type) {
        CHECKC(_gstate.did_supported, err::UNDER_MAINTENANCE, "did redpack not supported");
        CHECKC(symb == "FLON" || symb == "USDT" || symb == "USDC" || symb == "TYCHE",
               err::DID_PACK_SYMBOL_ERR, "DID redpack tokens can only be FLON|MUSDT|MUSDC|TYCHE");
    } else {
        CHECKC(quantity.amount / count >= 100, err::QUANTITY_NOT_ENOUGH,
               "Minimal unit 100 " + symb + " required");
    }

    // 保存红包信息
    redpack_t::idx_t redpacks(_self, _self.value);
    auto now = current_time_point();
    redpacks.emplace(_self, [&](auto& row) {
        row.code            = code;
        row.type            = rp_type;
        row.creator         = from;
        row.pw_hash         = parts[0] + ":" + get_first_receiver().to_string();
        row.total_quantity  = quantity;
        row.receiver_count  = count;
        row.remain_quantity = quantity;
        row.remain_count    = count;
        row.status          = redpack_status::CREATED;
        row.created_at      = now;
        row.updated_at      = now;
    });
}

void redpack::claimredpack(const name& claimer, const name& code, const string& pwhash)
{
    // require_auth( _gstate.admin );  // 1. 只有 admin 有权操作
    CHECKC( has_auth(claimer) || has_auth(_gstate.admin), err::ACCOUNT_INVALID, "auth failed: not claimer nor admin" )

    // 2. 读取红包主表，查不到直接抛错
    redpack_t redpack(code);
    CHECKC(_db.get(redpack), err::RECORD_NO_FOUND, "redpack not found");

    // 3. 拆分 pw_hash，提取 token 合约账户名（如无则从 tokenlist 补齐）
    auto pw_hash = split(redpack.pw_hash, ":");
    auto contract_name = name(pw_hash[1]);
    if (contract_name.length() == 0) {
        tokenlist_t::idx_t tokenlist_tbl(_self, _self.value);
        auto tokenlist_index = tokenlist_tbl.get_index<"by.sym"_n>();
        auto tokenlist_iter = tokenlist_index.find(redpack.total_quantity.symbol.raw());
        CHECKC(tokenlist_iter != tokenlist_index.end(), err::RECORD_NO_FOUND, "token list not found");
        contract_name = tokenlist_iter->contract;
    }

    // 4. 校验密码是否一致
    CHECKC(pw_hash[0] == pwhash, err::PWHASH_INVALID, "incorrect password");

    // 5. 校验红包状态为 CREATED（未过期、未结束）
    CHECKC(redpack.status == redpack_status::CREATED, err::EXPIRED, "redpack has expired");

    // 6. 若是 DID 类型红包，需要做 DID 认证校验
    bool is_auth = false;
    if ( redpack.type == redpack_type::RANDOM_DID || redpack.type == redpack_type::MEAN_DID) {
        auto claimer_acnts = flon::account_t::idx_t(_gstate.did_contract, claimer.value);
        for (auto claimer_acnts_iter = claimer_acnts.begin(); claimer_acnts_iter != claimer_acnts.end(); ++claimer_acnts_iter) {
            if (claimer_acnts_iter->balance.amount > 0) {
                is_auth = true;
                break;
            }
        }
        CHECKC(is_auth, err::DID_NOT_AUTH, "did is not authenticated");
    }

    // 7. 检查该 claimer 是否已领取过该红包（防止重复领取）
    claim_t::idx_t claims(_self, _self.value);
    auto claims_index = claims.get_index<"by.unionid"_n>();
    uint128_t sec_index = get_unionid(claimer, code.value);
    auto claims_iter = claims_index.find(sec_index);
    CHECKC(claims_iter == claims_index.end(), err::NOT_REPEAT_RECEIVE, "Can't repeat to receive");

    // 8. 计算本次可领取红包数量（支持随机、均分模式）
    asset redpack_quantity;
    switch( redpack.type.value ) {
        case redpack_type::RANDOM.value:
        case redpack_type::RANDOM_DID.value:
            _assign_redpack(redpack, redpack_quantity);
            break;

        case redpack_type::MEAN.value:
        case redpack_type::MEAN_DID.value:
            redpack_quantity = (redpack.remain_count == 1) ? redpack.remain_quantity : redpack.total_quantity / redpack.receiver_count;
            break;
    }

    // 9. 红包资产实际转出
    TRANSFER_OUT(contract_name, claimer, redpack_quantity, string("red pack transfer"));

    // 10. 红包主表更新剩余数量与状态
    redpack.remain_count--;
    redpack.remain_quantity -= redpack_quantity;
    redpack.updated_at      = time_point_sec(current_time_point());
    if (redpack.remain_count == 0) {
        redpack.status = redpack_status::FINISHED;
    }
    _db.set(redpack, _self);

    // 11. 插入领取记录
    claims.emplace(_self, [&](auto& row) {
        row.id              = claims.available_primary_key();
        row.red_pack_code   = code;
        row.creator         = redpack.creator;
        row.receiver        = claimer;
        row.quantity        = redpack_quantity;
        row.claimed_at      = current_time_point();
    });
}

void redpack::cancel( const name& code )
{
    redpack_t redpack(code);
    CHECKC( _db.get(redpack), err::RECORD_NO_FOUND, "redpack not found" );
    CHECKC( current_time_point() > redpack.created_at + eosio::hours(_gstate.expire_hours), err::NOT_EXPIRED, "expiration date is not reached" );
    if(redpack.status == redpack_status::CREATED){
        auto pw_hash = split(redpack.pw_hash, ":");
        auto contract = pw_hash[1];
        if (contract.size() == 0) {
            tokenlist_t::idx_t tokenlist_tbl(_self, _self.value);
            auto tokenlist_index = tokenlist_tbl.get_index<"by.sym"_n>();
            auto tokenlist_iter = tokenlist_index.find(redpack.total_quantity.symbol.raw());
            CHECKC( tokenlist_iter != tokenlist_index.end(), err::RECORD_NO_FOUND, "token list not found" );
            TRANSFER_OUT(tokenlist_iter->contract, redpack.creator, redpack.remain_quantity, string("red pack cancel transfer"));
        } else {
            auto contract_name = name(pw_hash[1]);
            TRANSFER_OUT(contract_name, redpack.creator, redpack.remain_quantity, string("red pack cancel transfer"));
        }
    }
    _db.del(redpack);
}

void redpack::delclaims( const uint64_t& max_rows )
{
    set<name> none_exist_list;

    claim_t::idx_t claim_idx(_self, _self.value);
    auto claim_itr = claim_idx.begin();

    size_t count = 0;
    for (; count < max_rows && claim_itr != claim_idx.end(); ) {

        bool redpack_none_exist = none_exist_list.count(claim_itr->red_pack_code) > 0 ? true : false;
        if (!redpack_none_exist){
            redpack_t redpack(claim_itr->red_pack_code);
            redpack_none_exist = !_db.get(redpack);
            if (redpack_none_exist){
                claim_itr = claim_idx.erase(claim_itr);
                none_exist_list.insert(claim_itr->red_pack_code);
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

// asset redpack::_calc_fee(const asset& fee, const uint64_t count) {
//     // calc order quantity value by price
//     auto value = multiply<uint64_t>(fee.amount, count);

//     return asset(value, fee.symbol);
// }

void redpack::_assign_redpack(const redpack_t& redpack, asset& assigned) {
    // calc order quantity value by price
    if ( redpack.remain_count == 1 ) {
        assigned = redpack.remain_quantity;
        return;
    }

    uint64_t quantity = redpack.remain_quantity.amount / redpack.remain_count * 2;
    uint8_t precision = 0;
    if (redpack.remain_quantity.symbol.precision() <= 2)
        precision = 0;
    else
        precision = redpack.remain_quantity.symbol.precision() - 2;

    assigned = asset( _rand(asset(quantity, redpack.remain_quantity.symbol), precision), redpack.remain_quantity.symbol );
}

uint64_t redpack::_rand(asset max_quantity,  uint16_t min_unit) {
    auto mixedBlock = tapos_block_prefix() * tapos_block_num();
    const char *mixedChar = reinterpret_cast<const char *>(&mixedBlock);
    auto hash = sha256( (char *)mixedChar, sizeof(mixedChar));
    int64_t min_unit_throot = power10(min_unit);

    auto r1 = (uint64_t)hash.data()[0];
    float rand = 1/min_unit_throot+r1 % 100 / 100.00;
    int64_t round_throot = power10(max_quantity.symbol.precision() - min_unit);
    uint64_t rand_value = (uint64_t)(max_quantity.amount * rand) / round_throot * round_throot;
    uint64_t min_value = get_precision(max_quantity) / min_unit_throot;
    
    return( rand_value < min_value ? min_value : rand_value );
    
}

