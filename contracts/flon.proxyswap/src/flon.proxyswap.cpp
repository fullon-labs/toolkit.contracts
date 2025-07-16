#include <cmath>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <eosio/eosio.hpp>
#include <string>

#include "flon.proxyswap.hpp"
#include "flon.token.hpp"
#include "utils.hpp"
using namespace wasm::db;

using namespace eosio;
using namespace std;
using namespace flon;


static constexpr eosio::name active_perm = "active"_n;





// clang-format off
#define CHECKC(exp, code, msg) \
   { if (!(exp)) eosio::check(false, string("[[") + to_string((int)code) + string("]] ") + msg); }
// clang-format on


using notifysettle_action   = action_wrapper<"notifysettle"_n,  &flonproxyswap::notifysettle>;

#define NOTIFY_SETTLE_ACTION( item, curr ) \
     { notifysettle_action act{ _self, { {_self, active_perm} } };\
          act.send( item, curr );}


enum class err: uint8_t {
    INVALID_FORMAT       = 0,
    TYPE_INVALID         = 1,
    FEE_NOT_FOUND        = 2,
    INSUFFICIENT_QUANTITY  = 3,
    NOT_POSITIVE         = 4,
    SYMBOL_MISMATCH      = 5,
    EXPIRED              = 6,
    PWHASH_INVALID       = 7,
    RECORD_NO_FOUND      = 8,
    NOT_REPEAT_RECEIVE   = 9,
    NOT_EXPIRED          = 10,
    ACCOUNT_INVALID      = 11,
    FEE_NOT_POSITIVE     = 12,
    VAILD_TIME_INVALID   = 13,
    MIN_UNIT_INVALID     = 14,
    REDPACK_EXIST       = 15,
    DID_NOT_AUTH         = 16,
    UNDER_MAINTENANCE    = 17,
    NONE_DELETED         = 19,
    IN_THE_WHITELIST     = 20,
    NON_RENEWAL          = 21,
    AMOUNT_TOO_SMALL     = 22,
    AMOUNT_TOO_LARGE     = 23,
    FEE_NOT_REQUIRED     = 24,
    DID_NOT_SUPPORTED    = 25,
    DID_PACK_SYMBOL_ERR  = 26,
    STATUS_MISMATCH      = 27,
    PAIR_EXISTED         = 28,  // 币对已存在
    PAIR_NOT_ENABLED     = 29,  // 币对未启用
    ORDER_NOT_OPEN       = 30,  // 订单不是open状态
    ORDER_ALREADY_HANDLED  = 31,   // 订单已处理
    PARAM_ERROR        = 32
 };


 
    // ========== 获取币种合约工具函数 ==========
    static name get_token_contract(const pair_t& pair, const asset& asset) {
        if (asset.symbol == pair.left_symbol.get_symbol())
            return pair.left_symbol.get_contract();
        if (asset.symbol == pair.right_symbol.get_symbol())
            return pair.right_symbol.get_contract();
        CHECKC(false, (int)err::ACCOUNT_INVALID, "token contract not found for symbol");
        return name();
    }

// ====== 初始化全局权限(admin) ======
void flonproxyswap::init(const name& admin, const name& oracle, const name& fee_receiver) {
    require_auth(get_self());
    _gstate.admin = admin;
    _gstate.oracle = oracle;
    _gstate.fee_receiver = fee_receiver;
    _global.set(_gstate, get_self());
}

void flonproxyswap::notifysettle(const order_t& order_item, const time_point_sec& curr_ts ) {
    require_auth(get_self());
    require_recipient(get_self());
}



// ====== 添加币对(admin) ======
void flonproxyswap::addtradpair(const name& tpcode,const extended_symbol& left_symbol,const extended_symbol& right_symbol,const asset& mini_left,const asset& mini_right,const uint8_t& max_slippage) {
    
    require_auth(_gstate.admin);

    pair_table pairs(get_self(), get_self().value);
    CHECKC(pairs.find(tpcode.value) == pairs.end(), err::PAIR_EXISTED, "pair already exists");
    CHECKC(mini_left.amount > 0, err::PARAM_ERROR, "mini_left must be positive");
    CHECKC(mini_right.amount > 0, err::PARAM_ERROR, "mini_right must be positive");
    CHECKC(left_symbol != right_symbol, err::PARAM_ERROR, "symbols must be different");

    pairs.emplace(_gstate.admin, [&](auto& row) {
    row.tpcode = tpcode;
    row.left_symbol = left_symbol;
    row.right_symbol = right_symbol;
    row.status = pair_status::ENABLED;
    row.mini_left = mini_left;
    row.mini_right = mini_right;
    row.max_slippage = max_slippage;
    });
}

void flonproxyswap::rmtradpair(const name& tpcode) {
    require_auth(_gstate.admin);  // 只允许管理员删除

    pair_table pairs(get_self(), get_self().value);
    auto pitr = pairs.find(tpcode.value);
    CHECKC(pitr != pairs.end(), err::RECORD_NO_FOUND, "pair not found");

    pairs.erase(pitr);
}

void flonproxyswap::distradpair(const name& tpcode) {
    require_auth(_gstate.admin);

    pair_table pairs(get_self(), get_self().value);
    auto pitr = pairs.find(tpcode.value);
    CHECKC(pitr != pairs.end(), err::RECORD_NO_FOUND, "pair not found");
    CHECKC(pitr->status != pair_status::DISABLED, err::PAIR_NOT_ENABLED, "pair already disabled");

    pairs.modify(pitr, same_payer, [&](auto& row) {
        row.status = pair_status::DISABLED;
    });
}

void flonproxyswap::enabtradpair(const name& tpcode) {
    require_auth(_gstate.admin);

    pair_table pairs(get_self(), get_self().value);
    auto pitr = pairs.find(tpcode.value);
    CHECKC(pitr != pairs.end(), err::RECORD_NO_FOUND, "pair not found");
    CHECKC(pitr->status != pair_status::ENABLED, err::PAIR_EXISTED, "pair already enabled");

    pairs.modify(pitr, same_payer, [&](auto& row) {
        row.status = pair_status::ENABLED;
    });
}



/// ====== 用户下单（转账触发） ======
void flonproxyswap::on_transfer(const name& from, const name& to, const asset& quantity, const string& memo) {
    if (to != get_self() || from == get_self()) return;
    // 假设 memo = "tpcode:type:price:slippage"
    auto params = split(memo, ":");
    CHECKC(params.size() >= 4, err::PARAM_ERROR, "invalid memo format");

    auto tpcode_str = params[0];
    auto type_str = params[1];
    auto price_str = params[2];
    auto slippage_str = params[3];

    name tpcode(tpcode_str);
    eosio::name ordertype;
    if (type_str == "buy")       ordertype = order_type::BUY;
    else if (type_str == "sell") ordertype = order_type::SELL;
    else CHECKC(false, err::INVALID_FORMAT, "type must be buy or sell");

    CHECKC(!slippage_str.empty(), err::INVALID_FORMAT, "slippage missing");
    for (char c : slippage_str) CHECKC(isdigit(c), err::INVALID_FORMAT, "slippage must be integer");
    int8_t slippage = std::stoll(slippage_str);




    pair_table pairs(get_self(), get_self().value);
    auto pitr = pairs.find(tpcode.value);
    CHECKC(pitr != pairs.end(), err::RECORD_NO_FOUND, "pair not found");
    CHECKC(pitr->status == pair_status::ENABLED, err::PAIR_NOT_ENABLED, "pair not enabled");

    // 校验 price 格式（只允许数字和一个小数点）
    asset price = asset_from_string(price_str);
    CHECKC(price.amount > 0, err::INVALID_FORMAT, "price must be positive");
    // 检查 symbol、精度是否符合预期（比如和币对右币种/左币种一致）
    if(ordertype == order_type::BUY) {
        CHECKC(price.symbol == pitr->right_symbol.get_symbol(), err::SYMBOL_MISMATCH, "price symbol mismatch for buy");
    } else {
        CHECKC(price.symbol == pitr->left_symbol.get_symbol(), err::SYMBOL_MISMATCH, "price symbol mismatch for buy");
    }


    CHECKC(quantity.amount > 0, err::NOT_POSITIVE, "quantity must be positive");
    CHECKC(slippage >= 0 && slippage <= pitr->max_slippage, err::PARAM_ERROR, "slippage out of range");
    // 校验币种和精度
    if (ordertype == order_type::BUY) {
        CHECKC(quantity.symbol == pitr->left_symbol.get_symbol(), err::SYMBOL_MISMATCH, "symbol mismatch for buy");
        CHECKC(quantity >= pitr->mini_left, err::AMOUNT_TOO_SMALL, "below mini_left");
    } else {
        CHECKC(quantity.symbol == pitr->right_symbol.get_symbol(), err::SYMBOL_MISMATCH, "symbol mismatch for sell");
        CHECKC(quantity >= pitr->mini_right, err::AMOUNT_TOO_SMALL, "below mini_right");
    }

    // 目标币种 symbol，初始化 0 值，防止 symbol/精度出错
    asset zero_out = (ordertype == order_type::BUY) 
                                ? asset(0, pitr->right_symbol.get_symbol())
                                : asset(0, pitr->left_symbol.get_symbol());
    asset zero_fee = asset(0, quantity.symbol);
    
    order_table orders(get_self(), get_self().value);
    auto new_id = orders.available_primary_key();

    orders.emplace(get_self(), [&](auto& row) {
        row.order_id = new_id;
        row.owner = from;
        row.tpcode = tpcode;
        row.type = ordertype;
        row.left_quant = quantity;
        row.price = price;  
        row.slippage = slippage;
        row.right_quant = zero_out;
        row.refund = asset(0, quantity.symbol);
        row.status = order_status::CREATED;
        row.fee = asset(0, quantity.symbol);
        row.memo = memo;
        row.created_at = time_point_sec(current_time_point());
        row.updated_at = time_point_sec(current_time_point());
    });
}


// ========== 订单完成，oracle 权限 ==========
void flonproxyswap::finishorder(const uint64_t& order_id,const asset& right_quant,const asset& fee,const string& memo) {
    require_auth(_gstate.oracle);

    order_table orders(get_self(), get_self().value);
    auto oitr = orders.find(order_id);
    CHECKC(oitr != orders.end(),  err::RECORD_NO_FOUND, "order not found");
    CHECKC(oitr->status == order_status::CREATED, err::ORDER_NOT_OPEN, "order not in pending state");

    // 取得币对信息
    pair_table pairs(get_self(), get_self().value);
    auto pitr = pairs.find(oitr->tpcode.value);
    CHECKC(pitr != pairs.end(), err::RECORD_NO_FOUND, "pair not found");
    CHECKC(pitr->status == pair_status::ENABLED, err::PAIR_NOT_ENABLED, "pair not enabled");

    // 1. 划转目标币
    if (right_quant.amount > 0) {
        // 校验符号和精度
        if (oitr->type == order_type::BUY) { // buy
            CHECKC(right_quant.symbol == pitr->right_symbol.get_symbol(), err::SYMBOL_MISMATCH, "symbol mismatch for right_quant");
        } else { // sell
            CHECKC(right_quant.symbol == pitr->left_symbol.get_symbol(), err::SYMBOL_MISMATCH, "symbol mismatch for right_quant");
        }
        name token_contract = get_token_contract(*pitr, right_quant);
        TRANSFER(token_contract, oitr->owner, right_quant, "swap filled");
    }

    // 2. 平台收手续费（如有）
    if (fee.amount > 0) {
        // fee 符号检查
        CHECKC(fee.symbol == right_quant.symbol, err::SYMBOL_MISMATCH, "fee symbol mismatch");
        name token_contract = get_token_contract(*pitr, fee);
        TRANSFER(token_contract, _gstate.fee_receiver, fee, "swap fee");
    }
    order_t order_item = *oitr;
    // 3. 更新订单状态
    orders.modify(oitr, same_payer, [&](auto& row) {
        row.right_quant = right_quant;
        row.fee = fee;
        row.status = order_status::FINISHED;
        row.memo = memo;
        row.updated_at = time_point_sec(current_time_point());
    });
    auto ts =  time_point_sec(current_time_point()) ;
    NOTIFY_SETTLE_ACTION( order_item, ts);
    orders.erase(oitr);

}

// ========== 订单取消，oracle 权限 ==========
void flonproxyswap::cancelorder(const uint64_t& order_id,const string& memo) {
    require_auth(_gstate.oracle);

    order_table orders(get_self(), get_self().value);
    auto oitr = orders.find(order_id);
    CHECKC(oitr != orders.end(), err::RECORD_NO_FOUND, "order not found");
    CHECKC(oitr->status == order_status::CREATED, err::ORDER_ALREADY_HANDLED, "order already processed");

    // 获取币对
    pair_table pairs(get_self(), get_self().value);
    auto pitr = pairs.find(oitr->tpcode.value);
    CHECKC(pitr != pairs.end(), err::RECORD_NO_FOUND, "pair not found");

    // 全额退回
    if (oitr->left_quant.amount > 0) {
        // 校验币种和精度
        if (oitr->type == order_type::BUY) {
            CHECKC(oitr->left_quant.symbol == pitr->left_symbol.get_symbol(), err::SYMBOL_MISMATCH, "refund symbol mismatch for buy");
        } else {
            CHECKC(oitr->left_quant.symbol == pitr->right_symbol.get_symbol(), err::SYMBOL_MISMATCH, "refund symbol mismatch for sell");
        }
        name token_contract = get_token_contract(*pitr, oitr->left_quant);
        TRANSFER(token_contract, oitr->owner, oitr->left_quant, "swap refund");
    }
    order_t order_item = *oitr;
    // 更新订单状态
    orders.modify(oitr, same_payer, [&](auto& row) {
        row.refund = row.left_quant;
        row.status = order_status::CANCELLED;
        row.memo = memo;
        row.updated_at = time_point_sec(current_time_point());
    });

    auto ts =  time_point_sec(current_time_point()) ;
    NOTIFY_SETTLE_ACTION( order_item, ts);
    orders.erase(oitr);
}

// ========== 修改管理员权限 ==========
void flonproxyswap::setadmin(const name& new_admin) {
    require_auth(_gstate.admin);

    _gstate.admin = new_admin;
    _global.set(_gstate, get_self());
}

// ========== 修改预言机权限 ==========
void flonproxyswap::setoracle(const name& new_oracle) {
    require_auth(_gstate.admin);

    _gstate.oracle = new_oracle;
    _global.set(_gstate, get_self());
}
