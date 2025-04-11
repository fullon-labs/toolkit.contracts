#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>

#include <string>

namespace flon {

using std::string;
using namespace eosio;

static constexpr name SYS_BANK     = "flon.token"_n;
static constexpr name ELON_BANK     = "elon.token"_n;
static constexpr name MIRROR_BANK   = "flon.mtoken"_n;
static constexpr symbol FLON        = symbol(symbol_code("FLON"), 8);
static constexpr symbol ELON        = symbol(symbol_code("ELON"), 0);
static constexpr symbol USDT        = symbol(symbol_code("USDT"), 6);
static constexpr symbol USDC        = symbol(symbol_code("USDC"), 6);
static constexpr symbol ETH         = symbol(symbol_code("ETH"), 8);
static constexpr symbol BTC         = symbol(symbol_code("BTC"), 8);
static constexpr symbol BNB         = symbol(symbol_code("BNB"), 6);

class [[eosio::contract("tokensummary")]] tokensummary : public contract {
public:
   using contract::contract;

   ACTION view( const name& account ) {
      auto flon_bal    = get_balance(SYS_BANK,   FLON,   account);
      auto elon_bal    = get_balance(ELON_BANK,   ELON,   account);
      auto usdt_bal    = get_balance(MIRROR_BANK, USDT,   account);
      auto usdc_bal    = get_balance(MIRROR_BANK, USDC,   account);
      auto btc_bal     = get_balance(MIRROR_BANK, BTC,    account);
      auto eth_bal     = get_balance(MIRROR_BANK, ETH,    account);
      auto bnb_bal     = get_balance(MIRROR_BANK, BNB,    account);

      auto res          = "Asset currency view >>>    \n[\n  \""
                           + flon_bal.to_string()  + "\",\n  \"" 
                           + elon_bal.to_string()  + "\",\n  \"" 
                           + usdt_bal.to_string()  + "\",\n  \""
                           + usdc_bal.to_string()  + "\",\n  \""
                           + bnb_bal.to_string()   + "\",\n  \""
                           + btc_bal.to_string()   + "\",\n  \""
                           + eth_bal.to_string()   + "\n]";
                          

      check(false, res );
   }

   struct accounts {
      asset balance;
      uint64_t primary_key() const {return balance.symbol.code().raw();}
   };
   typedef eosio::ulti_index< name("accounts"), accounts > tbl_accounts;

private:
   asset get_balance(const name& bank, const symbol& symb, const name& account) {
      tbl_accounts tmp(bank, account.value);
      auto itr = tmp.find(symb.code().raw());

      if (itr != tmp.end())
         return itr->balance;
      else 
         return asset(0, symb);
   }
};
}
