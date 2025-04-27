 #pragma once

#include <eosio/asset.hpp>
#include <eosio/privileged.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>
#include <utils.hpp>

#include <deque>
#include <optional>
#include <string>
#include <map>
#include <set>
#include <type_traits>

namespace flon {

using namespace std;
using namespace eosio;

#define SYMBOL(sym_code, precision) symbol(symbol_code(sym_code), precision)

static constexpr eosio::name active_perm{"active"_n};

static constexpr uint64_t percent_boost     = 10000;
static constexpr uint64_t max_memo_size     = 128;
static constexpr uint64_t max_addr_len      = 128;

static constexpr symbol FLON_SYMBOL         = SYMBOL("FLON", 8);

static constexpr name SYS_BANK                  = "flon.token"_n;
static constexpr name FLON_CONTRACT             = "flon"_n;
static constexpr name OWNER_PERM                = "owner"_n;
static constexpr name ACTIVE_PERM               = "active"_n;
enum class err: uint8_t {
    NONE                = 0,
    RECORD_NOT_FOUND    = 1,
    RECORD_EXISTING     = 2,
    ADDRESS_ILLEGAL     = 3,
    SYMBOL_MISMATCH     = 4,
    ADDRESS_MISMATCH    = 5,
    NOT_COMMON_XIN      = 6,
    STATUS_INCORRECT    = 7,
    PARAM_INCORRECT     = 8,
    NO_AUTH             = 9,
};

#define TBL struct [[eosio::table, eosio::contract("flon.creator")]]

struct [[eosio::table("global"), eosio::contract("flon.creator")]] global_t {
    name        admin       = "trustguardian"_n;                 
    asset       gas_quant   = asset(300000, FLON_SYMBOL);

    EOSLIB_SERIALIZE( global_t, (admin)(gas_quant) )
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;


} // flon
