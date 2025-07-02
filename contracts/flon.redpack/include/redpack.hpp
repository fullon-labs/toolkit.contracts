#include "redpackdb.hpp"
#include "wasm_db.hpp"

using namespace std;
using namespace wasm::db;

#define CHECKC(exp, code, msg) \
   { if (!(exp)) eosio::check(false, string("[[") + to_string((int)code) + string("]] ") + msg); }

enum class err: uint8_t {
   INVALID_FORMAT       = 0,
   TYPE_INVALID         = 1,
   FEE_NOT_FOUND        = 2,
   QUANTITY_NOT_ENOUGH  = 3,
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
   DID_PACK_SYMBOL_ERR  = 31
};

namespace redpack_type {
   static constexpr eosio::name RANDOM          = "random"_n;
   static constexpr eosio::name MEAN            = "mean"_n;
};

namespace redpack_status {
    static constexpr eosio::name CREATED        = "created"_n;
    static constexpr eosio::name FINISHED       = "finished"_n;
    static constexpr eosio::name CANCELLED      = "cancelled"_n;
};


class [[eosio::contract("flon.redpack")]] redpack: public eosio::contract {
private:
    dbc                 _db;
    global_singleton    _global;
    global_t            _gstate;

public:
    using contract::contract;

    redpack(eosio::name receiver, eosio::name code, datastream<const char*> ds):
        _db(_self),
        contract(receiver, code, ds),
        _global(_self, _self.value)
    {
        _gstate = _global.exists() ? _global.get() : global_t{};
    }

    ~redpack() {
        _global.set(_gstate, get_self());
    }

    ACTION listtoken(const name& contract, const symbol& sym, const time_point_sec& expired_time);
    ACTION delisttoken( const uint64_t& token_id );

    [[eosio::on_notify("flon.token::transfer")]]
    void on_token_transfer(const name& from, const name& to, const asset& quantity, const string& memo);

    [[eosio::on_notify("flon.mtoken::transfer")]]
    void on_mtoken_transfer(const name& from, const name& to, const asset& quantity, const string& memo );

    [[eosio::on_notify("tyche.token::transfer")]] 
    void on_tychetoken_transfer( const name& from, const name& to, const asset& quantity, const string& memo );


    ACTION claimredpack( const name& claimer, const name& code, const string& pwhash );
    ACTION cancel( const name& code );
    ACTION delclaims( const uint64_t& max_rows );

    ACTION init(const name& admin, const uint16_t& hours, const bool& did_supported, const uint64_t& did_id, const name& did_contract) {
        require_auth( _self );
        CHECKC( is_account(admin), err::ACCOUNT_INVALID, "account invalid" );
        CHECKC( hours > 0, err::VAILD_TIME_INVALID, "valid time must be positive" );

        _gstate.admin               = admin;
        _gstate.expire_hours        = hours;
        _gstate.did_supported       = did_supported;
        _gstate.did_id              = did_id;
        _gstate.did_contract        = did_contract;
    }

private:
    void _token_transfer( const name& from, const name& to, const asset& quantity, const string& memo );

    // asset _calc_fee(const asset& fee, const uint64_t count);
    void _assign_redpack(const redpack_t& redpack, asset& assigned);
    uint64_t _rand(asset max_quantity,  uint16_t min_unit);

    void _handle_deposit(const name& from, const asset& quantity, const vector<string>& parts);
    // void _handle_fee_payment(const asset& quantity, const vector<string>& parts);
}; //contract redpack