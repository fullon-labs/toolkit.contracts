#include <price.oracle/price.oracle.hpp>
#include <price.oracle/utils.hpp>

using namespace eosio;
using namespace std;
using namespace orcale;

using std::string;

/**
 * 添加预言人
 */
void price_oracle::addoracle(const name& oracle) {
    require_auth(get_self());

    auto oracle_item = oracle_t(oracle);
    CHECKC( !_dbc.get( oracle_item ), err::NO_AUTH, "oracle account is existing");
    _dbc.set( _self.value, oracle_item, false);
}

void price_oracle::removeoracle(const name& oracle) {
    require_auth(get_self());
    auto oracle_item = oracle_t(oracle);
    CHECKC( _dbc.get( oracle_item), err::RECORD_NOT_FOUND,  "oracle account is invalid" );
    _dbc.del_scope(_self.value, oracle_item);
}

void price_oracle::addcoin(const name& coin) {
    require_auth(get_self());

    CHECKC( _gstate.prices.count(coin) == 0, err::RECORD_FOUND, "coin is existing" );
    _gstate.prices[coin] = 0;
}

void price_oracle::removecoin(const name& coin) {
    require_auth(get_self());
    CHECKC( _gstate.prices.count(coin) , err::RECORD_NOT_FOUND, "coin is not found" );
    _gstate.prices.erase(coin);
}

void price_oracle::updateprice(const name& oracle, const std::vector<coin_price_info>& infos ) {
    require_auth(oracle);
    auto oracle_item = oracle_t(oracle);
    CHECKC( _dbc.get( oracle_item ), err::NO_AUTH,  "oracle account is invalid" );
    CHECKC( !infos.empty(), err::PARAM_ERROR, "infos length must bigger than 0")
    for(auto& info : infos) {
        _updateprice( info.tpcode, info.price );
    }
}
//tpcode: btc.usdt, eth.usdt
//price: 100.000000 USDT
void price_oracle::_updateprice(const name& tpcode, const asset& price) {
    // 拆分 tpcode，检查格式
    std::string tpcode_str = tpcode.to_string();
    auto coins = split(tpcode_str, ".");
    CHECKC(coins.size() == 2, err::PARAM_ERROR, "tpcode format error, expect XXX.QUOTE");

    name base_coin = name(coins[0]);
    name quote_coin = name(coins[1]);

    // 校验 quote_code 一致性
    CHECKC(quote_coin == _gstate.quote_code, err::RECORD_NOT_FOUND,
           "quote coin is not found, expect " + _gstate.quote_code.to_string());

    // 校验 symbol 一致性
    CHECKC(price.symbol == _gstate.quote_symbol, err::RECORD_NOT_FOUND,
           "price symbol is not found, expect " + _gstate.quote_symbol.code().to_string());

    // 校验该 base_coin 是否已注册
    auto it = _gstate.prices.find(base_coin);
    CHECKC(it != _gstate.prices.end(), err::RECORD_NOT_FOUND,
           "base coin " + base_coin.to_string() + " is not registered");

    // 检查价格区间（防异常波动）
    auto old_price = it->second;
    if (old_price != 0) {
        auto upper_limit = multiply_decimal64(old_price, 15, 10);
        auto lower_limit = multiply_decimal64(old_price, 5, 10);
        CHECKC(price.amount > lower_limit && price.amount < upper_limit, err::PARAM_ERROR,
               "price not valid: out of 0.5x~1.5x last value");
    }

    // 更新全局价格
    _gstate.prices[base_coin] = price.amount;

    // 更新（或替换）该币对应的 price 表，始终只保留一条最新数据
    coin_price_t::idx_t prices(_self, base_coin.value);
    auto itr = prices.begin();
    uint64_t id = 1;
    if (itr != prices.end()) {
        id = itr->id + 1;
        prices.erase(itr);
    }

    prices.emplace(_self, [&](auto& p) {
        p.id         = id;
        p.price      = price;
        p.tpcode     = tpcode;
        p.updated_at = current_block_time();
    });
}

