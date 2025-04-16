#include <flon.creator/flon.creator.hpp>

#include <eosio/transaction.hpp>
#include<flon.creator/flon.system.hpp>
#include<utils.hpp>
#include<math.hpp>
#include<string>
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include<pubkey_utils.hpp>

namespace flon {

void flon_creator::newaccount(const name& admin, const eosio::public_key& pubkey, const name& acct) {
   require_auth(admin);
   check( admin== _gstate.admin, "no auth for operate" );
   check( !is_account(acct),     "Account already exists" );

   _newaccount(acct, pubkey);
}
void flon_creator::_newaccount(const name& account, const public_key& pubkey) 
{
    flon::flon_system::newaccount_action  act(FLON_CONTRACT, { {_self, ACTIVE_PERM} });
    flon::authority owner_auth    = { 1, {{pubkey, 1}}, {}, {} };
    flon::authority active_auth   = { 1, {{pubkey, 1}}, {}, {} };
    act.send( _self, account, owner_auth, active_auth);

    flon::flon_system::buygas_action act2(FLON_CONTRACT, { {_self, ACTIVE_PERM} });
    act2.send( _self, account, _gstate.gas_quant)
}



} // namespace flon

