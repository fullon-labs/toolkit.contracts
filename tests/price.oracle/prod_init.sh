
#!/bin/bash
shopt -s expand_aliases
source ~/.bashrc

con=price.oracle
bid=bidder
mreg flon $con flonian

mreg flon $bid flonian

mset $con price.oracle

mpush $con addoracle '[ "'$bid'" ]' -p $con
