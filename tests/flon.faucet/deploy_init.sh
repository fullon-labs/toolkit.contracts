#!/bin/bash
shopt -s expand_aliases
source ~/.bashrc

FAUCET_CONTRACT="flon.faucet"
FROM_ACCOUNT="flon"
TRANSFER_AMOUNT="1000000 FLON"

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


# 1. Check and create faucet account if needed
check_and_create_account "$FAUCET_CONTRACT"

# 2. Set contract code
echo "📦 Deploying contract code"
tset "$FAUCET_CONTRACT" "$FAUCET_CONTRACT"
sleep 1

# 3. Transfer initial funds
echo "💸 Transferring $TRANSFER_AMOUNT from $FROM_ACCOUNT to $FAUCET_CONTRACT"
ttran "$FROM_ACCOUNT" "$FAUCET_CONTRACT" "$TRANSFER_AMOUNT"
sleep 1

# 4. Add permission to allow contract code execution
echo "🔐 Granting contract execution permission"
$TCLI set account permission "$FAUCET_CONTRACT" active --add-code

echo "✅ Faucet contract initialization completed"
