
#!/bin/bash
shopt -s expand_aliases
source ~/.bashrc

con=price.oracle
bid=bidder
mreg flon $con flonian

mreg flon $bid flonian

mset $con price.oracle

mpush $con addoracle '[ "'$bid'" ]' -p $con
mpush $con addcoin '[ "btc" ]' -p $con
mpush $con addcoin '[ "eth" ]' -p $con
mpush $con addcoin '[ "bnb" ]' -p $con


mpush $con updateprice '[ "'$bid'", [{"tpcode":"btc.usdt","price":"117951.220000 USDT"},{"tpcode":"eth.usdt","price":"2968.110000 USDT"}] ]' -p $bid


