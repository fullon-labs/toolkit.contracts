#!/bin/bash
shopt -s expand_aliases
source ~/.bashrc

tokensummary_con=tokensummary
mreg flon $tokensummary_con flonian
mtran flon $tokensummary_con "100.00000000 FLON"

mset $tokensummary_con tokensummary

mcli set account permission $tokensummary_con active --add-code







mpush tokensummary view '["flonian"]' -p flonian


mpush tokensummary view '["flonian"]' --read -j



mpush $tokensummary_con addtoken '["flon.token","8,FLON"]' -p $tokensummary_con
mpush $tokensummary_con addtoken '["flon.mtoken","6,USDT"]' -p $tokensummary_con
mpush $tokensummary_con addtoken '["flon.mtoken","6,USDC"]' -p $tokensummary_con
mpush $tokensummary_con addtoken '["flon.mtoken","8,ETH"]'  -p $tokensummary_con
mpush $tokensummary_con addtoken '["flon.mtoken","8,BTC"]'  -p $tokensummary_con
mpush $tokensummary_con addtoken '["flon.mtoken","8,BNB"]'  -p $tokensummary_con
mpush $tokensummary_con addtoken '["flon.mtoken","6,TRX"]'  -p $tokensummary_con
mpush $tokensummary_con addtoken '["flon.mtoken","8,BUSD"]' -p $tokensummary_con
mpush $tokensummary_con addtoken '["flon.mtoken","6,DAI"]'  -p $tokensummary_con
mpush $tokensummary_con addtoken '["flon.mtoken","8,DOGE"]' -p $tokensummary_con
mpush $tokensummary_con addtoken '["flon.mtoken","8,SHIB"]' -p $tokensummary_con
mpush $tokensummary_con addtoken '["flon.mtoken","8,SOL"]'  -p $tokensummary_con
mpush $tokensummary_con addtoken '["flon.mtoken","8,STT"]'  -p $tokensummary_con
mpush $tokensummary_con addtoken '["flon.mtoken","8,GAMO"]' -p $tokensummary_con


mpush $tokensummary_con deltoken '["flon.mtoken","8,GAMO"]' -p $tokensummary_con

#  tx=$(mcli push action -djs ts.ps1 view '["flonian"]' --read --return-packed)
#  curl -X POST --url https://t.flonscan.io/v1/chain/send_read_only_transaction -d "{\"transaction\": $tx}" | jq .


#  curl -X POST https://t.flonscan.io/v1/chain/send_read_only_transaction \
#    -H "Content-Type: application/json" \
#    -d '{
#          "transaction": {
#            "signatures": [],
#            "compression": "none",
#            "packed_context_free_data": "",
#            "packed_trx": "e46f946869db1886dcb2000000000100000000045c01ce0000000000c095db0008000000601a37695c00"
#          }
#        }'