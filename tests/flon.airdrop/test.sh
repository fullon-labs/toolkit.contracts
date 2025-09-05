#!/bin/bash
shopt -s expand_aliases
source ~/.bashrc

airdrop_con=airdrop11111
mreg flon $airdrop_con flonian
mtran flon $airdrop_con "100.00000000 FLON"
mset $airdrop_con flon.airdrop
mcli set account permission $airdrop_con active --add-code





