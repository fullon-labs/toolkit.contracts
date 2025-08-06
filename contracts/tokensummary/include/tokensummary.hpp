#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <string>
#include "tokensummary.db.hpp"

namespace flon {

using std::string;
using namespace eosio;

static constexpr name SYS_BANK     = "flon.token"_n;
static constexpr name ELON_BANK    = "elon.token"_n;
static constexpr name MIRROR_BANK  = "flon.mtoken"_n;
static constexpr symbol FLON       = symbol(symbol_code("FLON"), 8);
static constexpr symbol USDT       = symbol(symbol_code("USDT"), 6);
static constexpr symbol USDC       = symbol(symbol_code("USDC"), 6);
static constexpr symbol ETH        = symbol(symbol_code("ETH"), 8);
static constexpr symbol BTC        = symbol(symbol_code("BTC"), 8);
static constexpr symbol BNB        = symbol(symbol_code("BNB"), 6);
static constexpr symbol EOS        = symbol(symbol_code("EOS"), 4);
static constexpr symbol TRX        = symbol(symbol_code("TRX"), 6);
static constexpr symbol BUSD       = symbol(symbol_code("BUSD"), 6);
static constexpr symbol DAI        = symbol(symbol_code("DAI"), 18);
static constexpr symbol TUSD       = symbol(symbol_code("TUSD"), 18);
static constexpr symbol MATIC      = symbol(symbol_code("MATIC"), 18);
static constexpr symbol DOGE       = symbol(symbol_code("DOGE"), 8);
static constexpr symbol SHIB       = symbol(symbol_code("SHIB"), 18);
static constexpr symbol SOL        = symbol(symbol_code("SOL"), 9);   
static constexpr symbol STC        = symbol(symbol_code("STC"), 8);   
static constexpr symbol STE        = symbol(symbol_code("STE"), 8);   
static constexpr symbol STT        = symbol(symbol_code("STT"), 8);   
static constexpr symbol GAMO       = symbol(symbol_code("GAMO"), 8);   


class [[eosio::contract("tokensummary")]] tokensummary : public contract {
public:
    using contract::contract;

    ACTION view(const name& account);

private:
    asset get_balance(const name& bank, const symbol& symb, const name& account) ;
};

}