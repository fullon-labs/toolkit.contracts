#!/bin/bash
shopt -s expand_aliases
source ~/.bashrc


check_and_create_account() {
  local acc="$1"
  if $TCLI get account "$acc" &>/dev/null; then
    echo "✅ Account $acc already exists, skipping creation"
  else
    echo "➕ Creating account $acc"
    tnew "$acc"
    sleep 1
  fi
}


con="pub1.token"
FROM_ACCOUNT=flon
TRANSFER_AMOUNT="100 FLON"

# 1. Check and create faucet account if needed
check_and_create_account "$con"

# 2. Set contract code
echo "📦 Deploying contract code"
tset "$con" pubkey.token
sleep 1

generate_key_pair

# 3. Transfer initial funds
echo "💸 Transferring $TRANSFER_AMOUNT from $FROM_ACCOUNT to $con"
ttran "$FROM_ACCOUNT" "$con" "$TRANSFER_AMOUNT" $pubKey
sleep 1

# 4. Add permission to allow contract code execution
echo "🔐 Granting contract execution permission"
$tcli set account permission "$con" active --add-code

echo "✅ Faucet contract initialization completed"
