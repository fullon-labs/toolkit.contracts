#include "faucetdb.hpp"

using namespace std;
using namespace wasm::db;

class [[eosio::contract("flon.faucet")]] faucet: public eosio::contract {
private:
    global_singleton    _global;
    global_t            _gstate;

public:
    using contract::contract;

    faucet(eosio::name receiver, eosio::name code, datastream<const char*> ds):
        contract(receiver, code, ds),
        _global(get_self(), get_self().value)
    {
        _gstate = _global.exists() ? _global.get() : global_t{};
    }

    ACTION active(const name& account, const public_key&  pubkey);
    ACTION claim(const name& account);

private:
    void _newaccount(const name& account, const public_key& pubkey);
    void _transfer(const name& account);
    void _update_account(const name& account);

}; //contract faucet