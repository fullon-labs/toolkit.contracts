#!/bin/bash
shopt -s expand_aliases
source ~/.bashrc

con_ts=ts.ps1
mreg flon $con_ts flonian
mtran flon $con_ts "100.00000000 FLON"

mset $con_ts tokensummary

mcli set account permission $con_ts active --add-code







mpush $con_ts view '["flonian"]' -p $con_ts


mpush $con_ts view '["flonian"]' --read





tx=$(mcli push action -djs ts.ps1 view '["flonian"]' --read --return-packed)
curl -X POST --url https://t.flonscan.io/v1/chain/send_read_only_transaction -d "{\"transaction\": $tx}" | jq .


curl -X POST https://t.flonscan.io/v1/chain/send_read_only_transaction \
  -H "Content-Type: application/json" \
  -d '{
        "transaction": {
          "signatures": [],
          "compression": "none",
          "packed_context_free_data": "",
          "packed_trx": "e46f946869db1886dcb2000000000100000000045c01ce0000000000c095db0008000000601a37695c00"
        }
      }'