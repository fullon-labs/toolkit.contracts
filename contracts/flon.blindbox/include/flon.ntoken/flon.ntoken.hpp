#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/permission.hpp>

#include <string>

#include <flon.ntoken/flon.ntoken.db.hpp>
#define TRANSFER_N(bank, to, quants, memo) \
    {	ntoken::transfer_action act{ bank, { {_self, active_permission} } };\
			act.send( _self, to, quants , memo );}

         
namespace flon {

using std::string;
using std::vector;

using namespace eosio;


/**
 * The `flon.ntoken` sample system contract defines the structures and actions that allow users to create, issue, and manage tokens for AMAX based blockchains. It demonstrates one way to implement a smart contract which allows for creation and management of tokens. It is possible for one to create a similar contract which suits different needs. However, it is recommended that if one only needs a token with the below listed actions, that one uses the `flon.ntoken` contract instead of developing their own.
 *
 * The `flon.ntoken` contract class also implements two useful public static methods: `get_supply` and `get_balance`. The first allows one to check the total supply of a specified token, created by an account and the second allows one to check the balance of a token for a specified account (the token creator account has to be specified as well).
 *
 * The `flon.ntoken` contract manages the set of tokens, accounts and their corresponding balances, by using two internal multi-index structures: the `accounts` and `stats`. The `accounts` multi-index table holds, for each row, instances of `account` object and the `account` object holds information about the balance of one token. The `accounts` table is scoped to an eosio account, and it keeps the rows indexed based on the token's symbol.  This means that when one queries the `accounts` multi-index table for an account name the result is all the tokens that account holds at the moment.
 *
 * Similarly, the `stats` multi-index table, holds instances of `currency_stats` objects for each row, which contains information about current supply, maximum supply, and the creator account for a symbol token. The `stats` table is scoped to the token symbol.  Therefore, when one queries the `stats` table for a token symbol the result is one single entry/row corresponding to the queried symbol token if it was previously created, or nothing, otherwise.
 */
class ntoken {

public:  
   ACTION transfer( const name& from, const name& to, const vector<nasset>& assets, const string& memo );
   using transfer_action = action_wrapper< "transfer"_n, &ntoken::transfer >;

   ACTION transferfrom( const name& owner, const name& from, const name& to, const vector<nasset>& assets, const string& memo );
   using transfer_from_action = action_wrapper< "transferfrom"_n, &ntoken::transferfrom >;
   /**
    * @brief fragment a NFT into multiple common or unique NFT pieces
    *
    * @return ACTION
    */
   // ACTION fragment();

   ACTION setnotary(const name& notary, const bool& to_add);

   ACTION setipowner(const uint64_t& symbid, const name& ip_owner);

   ACTION settokenuri(const uint64_t& symbid, const string& url);
   /**
    * @brief notary to notarize a NFT asset by its token ID
    *
    * @param notary
    * @param token_id
    * @return ACTION
    */
   ACTION notarize(const name& notary, const uint32_t& token_id);
   ACTION approve( const name& spender, const name& sender, const uint32_t& token_pid, const uint64_t& amount );
   ACTION setcreator( const name& creator, const bool& to_add);

   ACTION setcheck( const bool& check_creator);
   
   
};
} //namespace flon
