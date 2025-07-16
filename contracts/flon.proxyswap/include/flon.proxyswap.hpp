#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <string>
#include "flon.proxyswap.db.hpp"   // 数据表结构
using namespace eosio;
using namespace wasm::db;
using namespace std;


namespace order_status {
    static constexpr eosio::name CREATED        = "created"_n;
    static constexpr eosio::name FINISHED      = "finished"_n;
    static constexpr eosio::name CANCELLED       = "cancelled"_n;
};

namespace pair_status {
    static constexpr eosio::name ENABLED        = "able"_n;
    static constexpr eosio::name DISABLED      = "disable"_n;
};

namespace order_type {
    static constexpr eosio::name BUY        = "buy"_n;
    static constexpr eosio::name SELL      = "sell"_n;
};





namespace flon {

class [[eosio::contract("flon.proxyswap")]] flonproxyswap : public contract {
private:
    global_singleton _global;
    global_t         _gstate;
    dbc              _db;

public:
    using contract::contract;

    flonproxyswap(eosio::name receiver, eosio::name code, datastream<const char*> ds):
        _db(_self),
        contract(receiver, code, ds),
        _global(_self, _self.value)
    {
        _gstate = _global.exists() ? _global.get() : global_t{};
    }

    ~flonproxyswap() {
        _global.set(_gstate, get_self());
    }

    // ---- 初始化全局权限(admin) ----
    [[eosio::action]]
    void init(const name& admin, const name& oracle, const name& fee_receiver);

    // ---- 添加币对(admin) ----
    [[eosio::action]]
    void addtradpair(const name& tpcode,const extended_symbol& left_symbol,const extended_symbol& right_symbol,const asset& mini_left,const asset& mini_right,const uint8_t& max_slippage);
    // ---- 删除币对(admin) ----
    [[eosio::action]]
    void rmtradpair(const name& tpcode);

    // 启用币对（admin权限）
    [[eosio::action]]
    void enabtradpair(const name& tpcode);

    // 禁用币对（admin权限）
    [[eosio::action]]
    void distradpair(const name& tpcode);

    // ---- 订单完成(oracle) ----
    [[eosio::action]]
    void finishorder(const uint64_t&  order_id,const asset& right_quant,const asset& fee, const string& memo);

    // ---- 订单取消(oracle) ----
    [[eosio::action]]
    void cancelorder( const uint64_t&  order_id, const string&     reason );

    // ---- 用户下单（转账触发）----
    [[eosio::on_notify("*::transfer")]]
     void on_transfer(const name& from, const name& to, const asset& quantity, const string& memo);

    // ---- 可选: 修改管理员/预言机 ----
    [[eosio::action]]
    void setadmin(const name& new_admin);

    [[eosio::action]]
    void setoracle(const name& new_oracle);
    
    [[eosio::action]]
    void notifysettle(const order_t& order_item, const time_point_sec& curr_ts);



};

} // namespace flon