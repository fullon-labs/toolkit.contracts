#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>

#include <string>
#include "tokensummary.hpp"
#include "tokensummary.db.hpp"
#include <cstdint>

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


std::string tokensummary::format_amount(int64_t amount, uint8_t precision) {
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

Tokensummaryresult tokensummary::view(const name& account) {
    std::vector<token_info> result;
    result.reserve(16);

    std::vector<std::pair<name, symbol>> tokenlist = {
        { SYS_BANK,     FLON },
        { MIRROR_BANK,  USDT },
        { MIRROR_BANK,  USDC },
        { MIRROR_BANK,  ETH  },
        { MIRROR_BANK,  BTC  },
        { MIRROR_BANK,  BNB  },
        { MIRROR_BANK,  TRX  },
        { MIRROR_BANK,  BUSD },
        { MIRROR_BANK,  DAI  },
        { MIRROR_BANK,  DOGE },
        { MIRROR_BANK,  SHIB },
        { MIRROR_BANK,  SOL  },
        { MIRROR_BANK,  STT  },
        { MIRROR_BANK,  GAMO }
    };

    for (const auto& item : tokenlist) {

        asset bal = get_balance(item.first, item.second, account);
        if (bal.amount <= 0) continue;

        std::string bal_str = format_amount(bal.amount, bal.symbol.precision());

        result.emplace_back(token_info{ item.first, bal_str, bal.symbol.code().to_string() });
    }

    Tokensummaryresult summary;
    summary.tokens = std::move(result);
    return summary;
}


