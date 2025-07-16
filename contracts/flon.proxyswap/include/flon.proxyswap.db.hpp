#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>

#include "utils.hpp"
#include "wasm_db.hpp"

namespace flon {


using namespace eosio;
using std::string;
using std::set;
using std::vector;

#define TBL struct [[eosio::table, eosio::contract("flon.proxyswap")]]



// ========== 全局配置表 ==========
struct [[eosio::table("global"), eosio::contract("flon.proxyswap")]] global_t {
    name     admin;                  // 管理员，合约治理/升级/紧急操作
    name     oracle;                  // 预言机账户，喂价/外部数据接入
    name    fee_receiver;            // 费率接受账号
    EOSLIB_SERIALIZE(global_t, (admin)(oracle)(fee_receiver))
};
typedef eosio::singleton<"global"_n, global_t> global_singleton;



// ========== 币对表 ==========
struct [[eosio::table("pairs"), eosio::contract("flon.proxyswap")]] pair_t {
    name             tpcode;            // 币对ID
    extended_symbol  left_symbol;       // 左币种
    extended_symbol  right_symbol;      // 右币种,价格
    name             status;        // 0-停用 1-启用
    asset            mini_left;         // 最小左币种金额
    asset            mini_right;        // 最小右币种金额
    uint8_t           max_slippage;    // 最大滑点(如0.01)
    // uint16_t      fee_ratio = 30;    // (可选)单币对手续费 0.3%

    uint64_t primary_key() const { return tpcode.value; }
    EOSLIB_SERIALIZE(pair_t, (tpcode)(left_symbol)(right_symbol)
                             (status)(mini_left)(mini_right)(max_slippage))
};
typedef eosio::multi_index<"pairs"_n, pair_t> pair_table;


// ========== 订单表 ==========
struct [[eosio::table("orders"), eosio::contract("flon.proxyswap")]] order_t {
    uint64_t       order_id;
    name           owner;               // 下单人
    name           tpcode;              // 币对主键
    name           type;                // buy、sell
    asset          left_quant;          // 用户支付资产
    asset          price;               // 单时指定的价格
    uint8_t        slippage;            // 最大允许滑点（订单级）
    asset          right_quant;         // 实际成交（全额成交才写入，否则为0）
    asset          refund;              // 未成交退回
    name           status;              // created/finished/cancelled
    asset          fee;                 // 手续费
    string         memo;
    time_point_sec created_at;
    time_point_sec updated_at;

    uint64_t primary_key() const { return order_id; }
    uint64_t by_owner()    const { return owner.value; }
    uint64_t by_pair()     const { return tpcode.value; }

    EOSLIB_SERIALIZE(order_t, (order_id)(owner)(tpcode)(type)
                               (left_quant)(price)(slippage)
                               (right_quant)(refund)(status)
                               (fee)(memo)
                               (created_at)(updated_at))
};
typedef eosio::multi_index<"orders"_n, order_t,
    indexed_by<"byowner"_n, const_mem_fun<order_t, uint64_t, &order_t::by_owner>>,
    indexed_by<"bypair"_n,  const_mem_fun<order_t, uint64_t, &order_t::by_pair>>
> order_table;

} // namespace flon