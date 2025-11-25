#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>

#include <string>
#include "tokensummary.hpp"
#include "tokensummary.db.hpp"
#include <cstdint>

using std::string;
using namespace eosio;
using namespace flon;


void token_summary::_balance(const name& bank, const symbol& symb, const name& account, asset& balance) {
    tbl_accounts tmp(bank, account.value);
    auto itr = tmp.find(symb.code().raw());

    balance = (itr != tmp.end()) ? itr->balance : asset(0, symb);
}


std::string token_summary::format_amount(int64_t amount, uint8_t precision) {
    bool neg = amount < 0;
    if (neg) amount = -amount;
    std::string s = std::to_string(amount);
    if (precision > 0) {
        if (s.size() <= precision) s.insert(0, precision - s.size() + 1, '0');
        s.insert(s.size() - precision, ".");
    }
    if (neg) s.insert(s.begin(), '-');
    return s;
}

void token_summary::addtoken(const name& bank, const symbol& sym) {
    require_auth(_gstate.admin);

    check(is_account(bank), "bank account not exist");
    check(sym.is_valid(),   "invalid symbol");

    token_cfg_t cfg(get_self(), get_self().value);
    auto pk = make_tokencfg_pk(bank, sym);
    auto it = cfg.find(pk);
    check(it == cfg.end(), "token already in list");

    cfg.emplace(get_self(), [&](auto& r){
        r.bank = bank;
        r.sym  = sym;
    });
}

void token_summary::deltoken(const name& bank, const symbol& sym) {
    require_auth(_gstate.admin);

    token_cfg_t cfg(get_self(), get_self().value);
    auto pk = make_tokencfg_pk(bank, sym);
    auto it = cfg.find(pk);
    check(it != cfg.end(), "token not found");
    cfg.erase(it);
}

TokenSummary token_summary::view(const name& account) {
    TokenSummary summary;

    for (auto &row : token_cfg_t(get_self(), get_self().value)) {
        auto bal = asset(0, row.sym);
        _balance(row.bank, row.sym, account, bal);

        if (bal.amount > 0) {
            summary.tokens.push_back({
                row.bank,
                format_amount(bal.amount, bal.symbol.precision()),
                bal.symbol.code().to_string()
            });
        }
    }

    return summary;
}


