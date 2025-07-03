
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

void redpack::_token_transfer(const name& from, const name& to, const asset& quantity, const string& memo)
{
    if (from == _self || to != _self) return;
    CHECKC(quantity.amount > 0, err::NOT_POSITIVE, "quantity must be positive")
    CHECKC( is_account(from), err::ACCOUNT_INVALID, "account invalid" );
    
    auto parts = split(memo, ":");
    switch( parts.size() ) {
        case 6: // deposit redpack memo format: [ $code:$type:$did_required:$count:$password_hash:$contract ]
            _handle_deposit(from, quantity, parts);
            break;

        case 2: // fee payment memo format: [ $code:$fee_type ]
            _handle_fee_payment(quantity, parts);
            break;

        default:
            CHECKC(false, err::INVALID_FORMAT, "invalid memo format");
    }
}

// -------------------------- 内部方法分离 ------------------------------
// memo format: [ $code:$type:$did_required:$count:$password_hash:$contract ]
void redpack::_handle_deposit(const name& from, const asset& quantity, const vector<string>& parts) {
    auto token_contract = get_first_receiver();

    // 校验token是否在 tokenlist 中, 否则拒绝创建红包
    tokenlist_t::idx_t tokenlist_tbl(_self, _self.value);
    auto tokenlist_index = tokenlist_tbl.get_index<"by.consymb"_n>();
    uint128_t consymb_id = get_unionid( token_contract, quantity.symbol.raw() );
    auto iter = tokenlist_index.find( consymb_id );
    CHECKC( iter != tokenlist_index.end(), err::RECORD_NO_FOUND, "redpack token not listed");
    
    // -- memo parts[0]: code
    name code = name(parts[0]);
    redpack_t redpack(code);
    CHECKC( !_db.get(redpack), err::REDPACK_EXIST, "code already exists" );
    
    // -- memo parts[1]: type
    auto rp_type = name(stoi(parts[1]));
    CHECKC( (rp_type == redpack_type::RANDOM || rp_type == redpack_type::MEAN), err::TYPE_INVALID, "redpack type invalid");

    // -- memo parts[2]: did_required
    bool did_required = ( stoi(parts[2]) == 1 );  // 是否需要 DID 认证: 0: no, 1: yes

    // -- memo parts[3]: count
    int count = stoi(parts[3]);
    CHECKC( count > 0, err::NOT_POSITIVE, "redpack count must be positive" );
    CHECKC( count <= 10000, err::AMOUNT_TOO_LARGE, "redpack count must be no greater than 10000" );
    
    // -- memo parts[4]: password_hash
    string passwd_hash = parts[4];

    auto symb = quantity.symbol.code().to_string();
    CHECKC( quantity.amount / count >= 100, err::INSUFFICIENT_QUANTITY, "Minimal unit 100 " + symb + " required")

    if( redpack.did_required ) {
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
        row.status          = redpack_status::CREATED;
        row.created_at      = now;
        row.updated_at      = now;
    });
}

void redpack::_handle_fee_payment(const asset& fee_quant, const vector<string>& parts) {
    // 处理手续费支付逻辑
    CHECKC( _gstate.unwrap_unit_fee.amount > 0, err::FEE_NOT_REQUIRED, "zero fee charged" )
    CHECKC( _gstate.unrwap_fee_required, err::FEE_NOT_REQUIRED, "fee not required" )
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
    require_auth( _gstate.admin );  // 1. 只有 admin 有权操作 - 否则pwhash有人领了，口令就暴露，容易被脚本攻击
    // CHECKC( has_auth(claimer) || has_auth(_gstate.admin), err::ACCOUNT_INVALID, "auth failed: not claimer nor admin" )

    // 2. 读取红包主表，查不到直接抛错
    redpack_t redpack(code);
    CHECKC( _db.get(redpack), err::RECORD_NO_FOUND, "redpack not found" )

    // 4. 校验密码是否一致
    CHECKC( redpack.passwd_hash == pwhash, err::PWHASH_INVALID, "incorrect password");

    // 5. 校验红包状态为 SERVICING (已经完成支付)
    CHECKC( redpack.status == redpack_status::SERVICING, err::EXPIRED, "redpack has expired");

    // 6. 若是 DID 类型红包，需要做 DID 认证校验
    if ( redpack.did_required ) {
        auto is_auth = false;
        auto claimer_acnts = flon::account_t::idx_t( _gstate.did_contract, claimer.value );
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
    asset assigned_quant;
    switch( redpack.type.value ) {
        case redpack_type::RANDOM.value:
            _assign_redpack(redpack, assigned_quant);
            break;

        case redpack_type::MEAN.value:
            assigned_quant = (redpack.remaining_count == 1) ? redpack.remaining_quant : redpack.total_quant / redpack.total_count;
            break;
    }

    // 9. 红包资产实际转出
    TRANSFER_OUT( redpack.token_contract, claimer, assigned_quant, string("redpack: " + code.to_string()) )

    // 10. 红包主表更新剩余数量与状态
    CHECKC( redpack.remaining_quant >= assigned_quant, err::INSUFFICIENT_QUANTITY, "insufficient redpack quantity" )
    CHECKC( redpack.remaining_count > 0, err::INSUFFICIENT_QUANTITY, "insufficient redpack count" )
    
    redpack.remaining_quant -= assigned_quant;
    redpack.remaining_count--;
    if (redpack.remaining_count == 0) {
        redpack.status = redpack_status::FINISHED;
    }

    auto now                = current_time_point();
    redpack.updated_at      = now;
    _db.set(redpack, _self);

    // 11. 插入领取记录
    claims.emplace(_self, [&](auto& row) {
        row.id              = claims.available_primary_key();
        row.redpack_code    = code;
        row.redpack_wrapper = redpack.wrapper;
        row.claimer         = claimer;
        row.quantity        = assigned_quant;
        row.claimed_at      = now;
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

