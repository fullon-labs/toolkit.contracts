#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>

#include <string>
#include "tokensummary.hpp"


using std::string;
using namespace eosio;
using namespace flon;


asset  tokensummary::get_balance(const name& bank, const symbol& symb, const name& account) {
   tbl_accounts tmp(bank, account.value);
   auto itr = tmp.find(symb.code().raw());

   if (itr != tmp.end())
      return itr->balance;
   else 
      return asset(0, symb);
}



void tokensummary::view(const name& account) {
    // 1. 查询各币余额
    struct CoinItem {
        asset bal;
        string label;
    };
    std::vector<CoinItem> coins = {
        { get_balance(SYS_BANK,     FLON,   account), "FLON"   },
        { get_balance(CISUM_BANK,   CISUM,  account), "CISUM"  },
        { get_balance(MIRROR_BANK,  USDT,   account), "USDT"   },
        { get_balance(MIRROR_BANK,  USDC,   account), "USDC"   },
        { get_balance(MIRROR_BANK,  ETH,    account), "ETH"    },
        { get_balance(MIRROR_BANK,  BTC,    account), "BTC"    },
        { get_balance(MIRROR_BANK,  BNB,    account), "BNB"    },
        { get_balance(MIRROR_BANK,  TRX,    account), "TRX"    },
        { get_balance(MIRROR_BANK,  BUSD,   account), "BUSD"   },
        { get_balance(MIRROR_BANK,  DAI,    account), "DAI"    },
        { get_balance(MIRROR_BANK,  DOGE,   account), "DOGE"   },
        { get_balance(MIRROR_BANK,  SHIB,   account), "SHIB"   },
        { get_balance(MIRROR_BANK,  SOL,    account), "SOL"    },
        { get_balance(MIRROR_BANK,  STT,    account), "STT"    },
        { get_balance(MIRROR_BANK,  GAMO,   account), "GAMO"   }  
    };

    // 2. 拼接非零资产
    string res = "Asset currency view >>>\n[\n";
    bool first = true;
    for (const auto& c : coins) {
        if (c.bal.amount == 0) continue;
        if (!first) res += ",\n";
        res += "  \"" + c.label + ": " + c.bal.to_string() + "\"";
        first = false;
    }
    res += "\n]";

    check(false, res);
}


