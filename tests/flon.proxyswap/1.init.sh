#!/bin/bash
shopt -s expand_aliases
source ~/.bashrc

con=flon.pt4
mreg flon $con flonian
mtran flon $con "500 FLON"


mtran flon $con "500 FLON"

mset $con flon.proxyswap

mreg flon $admin flonian
mcli set account permission $con active --add-code

# 要创建的账号oracleuser

oracleadmin=oracleuser
mreg flon $oracleadmin flonian

feereceiver=feerecvuser
mreg flon $feereceiver flonian



mpush $con init '["'"${con}"'", "'"${oracleadmin}"'", "'"${feereceiver}"'"]' -p $con


mpush $con addtradpair '["usdt.flon",{"sym":"8,FLON","contract":"flon.token"},{"sym":"6,USDT","contract":"flon.mtoken"},"1.00000000 FLON","10.000000 USDT",500]' -p $con

#USDT 买 FLON（买单）
mpush flon.mtoken transfer '["flonian","'"${con}"'", "100.000000 USDT", "usdt.flon:buy:1.000000 USDT:10"]' -p flonian
mpush flon.token transfer '["flonian", "'"${con}"'", "50.00000000 FLON", "usdt.flon:sell:1.000000 USDT:10"]' -p flonian

#成交订单
mpush $con finishorder '[1, "100.000000 USDT","99.00000000 FLON","1.000000 USDT" ,"1.00000000 FLON", "order filled"]' -p $oracleadmin
#金额不一样是否报错
mpush $con finishorder '[2, "40.00000000 FLON","49.500000 USDT","1.000000 USDT" ,"0.500000 USDT", "order filled"]' -p $oracleadmin
#金额一样
mpush $con finishorder '[2, "50.00000000 FLON","49.500000 USDT","1.000000 USDT" ,"0.500000 USDT", "order filled"]' -p $oracleadmin
#取消订单

mpush $con cancelorder '[3, "order canceled by oracle"]' -p $oracleadmin




mpush $con setadmin '["flon.ps9"]' -p $con

mpush $con setoracle '["neworacle"]' -p $con


mpush $con rmtradpair '["usdt.flon"]' -p $con


mpush $con enabtradpair '["usdt.flon"]' -p $con


mpush $con distradpair '["usdt.flon"]' -p $con