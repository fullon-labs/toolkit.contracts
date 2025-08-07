#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>

#include <string>
#include "tokensummary.hpp"
#include "tokensummary.db.hpp"

using std::string;
using namespace eosio;
using namespace flon;


asset tokensummary::get_balance(const name& bank, const symbol& symb, const name& account) {
   tbl_accounts tmp(bank, account.value);
   auto itr = tmp.find(symb.code().raw());

   if (itr != tmp.end())
      return itr->balance;
   else 
      return asset(0, symb);
}

TokenSummary tokensummary::view(const name& account) {
    std::vector<asset> result;
    std::vector<std::pair<name, symbol>> tokenlist = {
        {SYS_BANK, FLON},
        {MIRROR_BANK, USDT},
        {MIRROR_BANK, USDC},
        {MIRROR_BANK, ETH},
        {MIRROR_BANK, BTC},
        {MIRROR_BANK, BNB},
        {MIRROR_BANK, TRX},
        {MIRROR_BANK, BUSD},
        {MIRROR_BANK, DAI},
        {MIRROR_BANK, DOGE},
        {MIRROR_BANK, SHIB},
        {MIRROR_BANK, SOL},
        {MIRROR_BANK, STT},
        {MIRROR_BANK, GAMO}
    };

    for (const auto& item : tokenlist) {
        asset bal = get_balance(item.first, item.second, account);
        if (bal.amount > 0) result.push_back(bal);
    }

    TokenSummary summary;
    summary.tokens = std::move(result);
    return summary;
}


