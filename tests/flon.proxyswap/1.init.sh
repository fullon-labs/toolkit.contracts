#!/bin/bash
shopt -s expand_aliases
source ~/.bashrc

con=flon.pt4
mreg flon $con flonian
mtran flon $con "500 FLON"

mset $con flon.proxyswap

mreg flon $admin flonian
mcli set account permission $con active --add-code

# 要创建的账号oracleuser

oracleadmin=oracleuser
mreg flon $oracleadmin flonian

feereceiver=feerecvuser
mreg flon $feereceiver flonian



mpush $con init '["'"${con}"'", "'"${oracleadmin}"'", "'"${feereceiver}"'",0]' -p $con


mpush $con addtradpair '["btc.usdt", {"sym":"8,BTC","contract":"flon.mtoken"},{"sym":"6,USDT","contract":"flon.mtoken"},"0.00013000 BTC","15.000000 USDT",500]' -p $con
mpush $con addtradpair '["eth.usdt", {"sym":"8,ETH","contract":"flon.mtoken"},{"sym":"6,USDT","contract":"flon.mtoken"},"0.00500000 ETH","15.000000 USDT",500]' -p $con



#USDT 买 FLON（买单）
# 参数说明：
# ["from", "to", "quantity", "memo"]
# from: flonian
# to: $con (合约账户)
# quantity: 100.000000 USDT
# memo: "btc.usdt:buy:130000.000000 USDT:10" （交易对:操作类型:价格:滑点）
mpush flon.mtoken transfer '["flonian","'"${con}"'", "100.000000 USDT", "btc.usdt:buy:130000.000000 USDT:10"]' -p flonian

mpush flon.mtoken transfer '["flonian", "'"${con}"'", "0.00020000 BTC", "btc.usdt:sell:130000.000000 USDT:10"]' -p flonian

#成交订单
mpush $con finishorder '[10, "100.000000 USDT","99.00000000 FLON","1.000000 USDT" ,"1.00000000 FLON", "order filled"]' -p $oracleadmin
#金额不一样是否报错
mpush $con finishorder '[2, "40.00000000 FLON","49.500000 USDT","1.000000 USDT" ,"0.500000 USDT", "order filled"]' -p $oracleadmin
#金额一样
mpush $con finishorder '[11, "50.00000000 FLON","49.500000 USDT","1.000000 USDT" ,"0.500000 USDT", "order filled"]' -p $oracleadmin
#取消订单

mpush $con cancelorder '[3, "order canceled by oracle"]' -p $oracleadmin




mpush $con setadmin '["flon.ps9"]' -p $con

mpush $con setoracle '["neworacle"]' -p $con


mpush $con rmtradpair '["btc.usdt"]' -p $con


mpush $con enabtradpair '["btc.usdt"]' -p $con


mpush $con distradpair '["flon.usdt"]' -p $con