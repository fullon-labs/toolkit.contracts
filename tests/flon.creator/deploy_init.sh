#!/bin/bash
shopt -s expand_aliases
source ~/.bashrc

con="flon.creator"
FROM_ACCOUNT="flontest"
TRANSFER_AMOUNT="1000 FLON"

check_and_create_account() {
  local acc="$1"
  if tcli get account "$acc" &>/dev/null; then
    echo "✅ Account $acc already exists, skipping creation"
  else
    echo "➕ Creating account $acc"
    tnew "$acc"
    sleep 1
  fi
}


# 1. Check and create faucet account if needed
check_and_create_account "$con"

# 2. Set contract code
echo "📦 Deploying contract code"
tset "$con" "$con"
sleep 1

# 3. Transfer initial funds
echo "💸 Transferring $TRANSFER_AMOUNT from $FROM_ACCOUNT to $con"
ttran "$FROM_ACCOUNT" "$con" "$TRANSFER_AMOUNT"
sleep 1

# 4. Add permission to allow contract code execution
echo "🔐 Granting contract execution permission"
tcli set account permission "$con" active --add-code

echo "✅ FlonCreator contract initialization completed"
