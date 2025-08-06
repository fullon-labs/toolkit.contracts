#pragma once
#include <eosio/asset.hpp>
#include <eosio/name.hpp>

struct [[eosio::table, eosio::contract("tokensummary")]] accounts {
    eosio::asset balance;
    uint64_t primary_key() const { return balance.symbol.code().raw(); }
};

// 通用token资产表，不建议动，兼容多合约
typedef eosio::multi_index< "accounts"_n, accounts > tbl_accounts;