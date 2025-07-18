#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <string>
#include "flon.proxyswap.db.hpp"
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

    [[eosio::action]]
    void init(const name& admin, const name& oracle, const name& fee_receiver);

    [[eosio::action]]
    void addtradpair(const name& tpcode,
                const extended_symbol& left_symbol,
                const extended_symbol& right_symbol,
                const asset& mini_left,
                const asset& mini_right,
                const uint64_t& max_slippage);
                
    [[eosio::action]]
    void rmtradpair(const name& tpcode);

    [[eosio::action]]
    void enabtradpair(const name& tpcode);

    [[eosio::action]]
    void distradpair(const name& tpcode);

    [[eosio::action]]
    void finishorder(const uint64_t&  order_id,
                const asset& order_quant,
                const asset& deal_quant,
                const asset& deal_price,
                const asset& fee,
                const string& memo);

    [[eosio::action]]
    void cancelorder( const uint64_t&  order_id, const string&     reason );

    [[eosio::on_notify("*::transfer")]]
     void on_transfer(const name& from, const name& to, const asset& quantity, const string& memo);

    [[eosio::action]]
    void setadmin(const name& new_admin);

    [[eosio::action]]
    void setoracle(const name& new_oracle);
    
    [[eosio::action]]
    void notifysettle(const order_t& order_item, const time_point_sec& curr_ts);

};

} // namespace flon