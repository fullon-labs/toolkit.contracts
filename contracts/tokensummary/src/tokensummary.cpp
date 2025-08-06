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

void tokensummary::view(const name& account) {
    // 1. 查询各币余额
    std::vector<asset> tokens = {
        { get_balance(SYS_BANK,     FLON,   account)  },
        { get_balance(CISUM_BANK,   CISUM,  account)  },
        { get_balance(MIRROR_BANK,  USDT,   account)  },
        { get_balance(MIRROR_BANK,  USDC,   account)  },
        { get_balance(MIRROR_BANK,  ETH,    account)  },
        { get_balance(MIRROR_BANK,  BTC,    account)  },
        { get_balance(MIRROR_BANK,  BNB,    account)  },
        { get_balance(MIRROR_BANK,  TRX,    account)  },
        { get_balance(MIRROR_BANK,  BUSD,   account)  },
        { get_balance(MIRROR_BANK,  DAI,    account)  },
        { get_balance(MIRROR_BANK,  DOGE,   account)  },
        { get_balance(MIRROR_BANK,  SHIB,   account)  },
        { get_balance(MIRROR_BANK,  SOL,    account)  },
        { get_balance(MIRROR_BANK,  STT,    account)  },
        { get_balance(MIRROR_BANK,  GAMO,   account)  }  
    };

    // return TokenSummary { tokens };

    // 2. 拼接非零资产
    string res = "Account currency view >>>\n[\n";
    bool first = true;
    for (const auto& token : tokens) {
        if (token.amount == 0) continue;

        if (!first) res += ",\n";
        res += "  \"" + token.to_string() + "\"";
        first = false;
    }
    res += "\n]";

    check(false, res);
}


