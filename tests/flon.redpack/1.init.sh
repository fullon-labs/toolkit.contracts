#!/bin/bash
shopt -s expand_aliases
source ~/.bashrc

con=flon.redpack
admin=flonian
claim_admin=redpackclaim

mreg flon $con flonian
mset $con flon.redpack
mreg flonian $claim_admin flonian
mcli set account permission $con active --add-code


mpush $con init '{"admin": "'$admin'", "claim_admin": "'$claim_admin'", "hours": 24, "did_supported": true, "did_id": 1, "did_contract": "did.ntoken"}' -p $con
# mpush $con setfee '{"fee": ["1.00000000 FLON", "flon.token"]}' -p $con

mpush $con listtoken  '{"contract": "flon.mtoken", "sym": "6,USDT"}' -p $admin
mpush $con listtoken  '{"contract": "flon.token", "sym": "8,FLON"}' -p $admin
mpush $con listtoken  '{"contract": "flon.mtoken", "sym": "8,ETH"}' -p $admin
mpush $con listtoken  '{"contract": "flon.mtoken", "sym": "8,BTC"}' -p $admin