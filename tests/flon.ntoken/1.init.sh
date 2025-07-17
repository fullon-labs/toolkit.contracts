#!/bin/bash
shopt -s expand_aliases
source ~/.bashrc



con=flon.ntoken
mreg flon $con flonian
mset $con flon.ntoken

mreg flon usera flonian
sleep 1

mpush flon.ntoken create '["flonian",300,[100,0],"https://bafybeih4ncgscmow23uz5s7uup5ljh4jmzk5qicuo747juyx4gmcj3d4vu.ipfs.nftstorage.link","usera"]' -p flonian

mpush flon.ntoken issue '["flonian",[1,[100,0]],"1st issue"]' -p flonian

mpush flon.ntoken issue '["flonian",[1,[101,0]],"1st issue"]' -p flonian

mpush flon.ntoken issue '["flonian",[1,[102,0]],"1st issue"]' -p flonian

mpush flon.ntoken issue '["flonian",[1,[103,0]],"1st issue"]' -p flonian


mpush flon.ntoken transfer '["flonian","merchantx",[[1,[100,0]]],""]' -p flonian

mpush flon.ntoken transfer '["flonian","merchantx",[[1,[101,0]]],""]' -p flonian

mpush flon.ntoken transfer '["flonian","merchantx",[[1,[102,0]]],""]' -p flonian

mpush flon.ntoken transfer '["flonian","merchantx",[[1,[103,0]]],""]' -p flonian

