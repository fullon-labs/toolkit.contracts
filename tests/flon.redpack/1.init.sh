#!/bin/bash
shopt -s expand_aliases
source ~/.bashrc

con=flon.rp3
mreg flon $con flonian
mset $con flon.redpack
admin=redpackadmin
treg flonian $admin flonian
mcli set account permission $con active --add-code


mpush $con init '{"admin": "'$admin'", "hours": 24, "did_supported": true, "did_id": 1, "did_contract": "did.ntoken"}' -p $con
# mpush $con setfee '{"fee": ["1.00000000 FLON", "flon.token"]}' -p $con

mpush $con listtoken  '{"contract": "flon.mtoken", "sym": "6,USDT", "expired_time": "2099-01-01T00:00:00"}' -p $admin
mpush $con listtoken  '{"contract": "flon.token", "sym": "8,FLON", "expired_time": "2099-01-01T00:00:00"}' -p $admin
mpush $con listtoken  '{"contract": "flon.mtoken", "sym": "8,ETH", "expired_time": "2099-01-01T00:00:00"}' -p $admin
mpush $con listtoken  '{"contract": "flon.mtoken", "sym": "8,BTC", "expired_time": "2099-01-01T00:00:00"}' -p $admin
mpush $con listtoken  '{"contract": "flon.mtoken", "sym": "8,USDT", "expired_time": "2099-01-01T00:00:00"}' -p $admin