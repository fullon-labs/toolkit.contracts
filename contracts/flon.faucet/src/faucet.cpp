
#include <flon.token.hpp>
#include "faucet.hpp"
#include "utils.hpp"
#include "flon.system/flon.system.hpp"

#include <chrono>

using std::chrono::system_clock;
using namespace wasm;

static constexpr eosio::name active_permission{"active"_n};
#define CHECKC(exp, code, msg) \
   { if (!(exp)) eosio::check(false, string("$$$") + to_string((int)code) + string("$$$ ") + msg); }

// transfer out from contract self
#define TRANSFER_OUT(token_contract, to, quantity, memo) token::transfer_action(                                \
                token_contract, {{get_self(), active_permission}}) \
                .send(                                             \
                    get_self(), to, quantity, memo);

void faucet::active(const name& account, const public_key&  pubkey) {
    _newaccount(account, pubkey);
    _transfer(account);
    _update_account(account);
}

void faucet::_newaccount(const name& account, const public_key& pubkey) 
{
    flon::flon_system::newaccount_action  act(SYS_CONTRACT, { {_self, ACTIVE_PERM} });
    flon::authority owner_auth    = { 1, {{pubkey, 1}}, {}, {} };
    flon::authority active_auth   = { 1, {{pubkey, 1}}, {}, {} };
    act.send( _self, account, owner_auth, active_auth);

    flon::flon_system::buygas_action act2(SYS_CONTRACT, { {_self, ACTIVE_PERM} });
    act2.send( _self, account, asset(10000, SYS_SYMBOL));
}

void faucet::claim(const name& account) {
    _transfer(account);
    _update_account(account);
}
void faucet::_transfer(const name& account) 
{
    flon::flon_system::buygas_action act(SYS_CONTRACT, { {_self, ACTIVE_PERM} });
    act.send( _self, account, asset(100000000, SYS_SYMBOL));
}

void faucet::_update_account(const name& account) 
{
    account_t::idx_t account_tbl(get_self(), get_self().value);
    auto acct = account_tbl.find(account.value);

    if (acct == account_tbl.end()) {
        account_tbl.emplace(get_self(), [&](auto& row) {
            row.owner = account;
            row.updated_at = current_time_point();
        });
    } else {
        account_tbl.modify(acct, get_self(), [&](auto& row) {
            row.updated_at = current_time_point();
        });
    }
}