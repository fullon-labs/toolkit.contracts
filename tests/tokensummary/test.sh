#!/bin/bash
shopt -s expand_aliases
source ~/.bashrc

con_ts=ts.ps1
mreg flon $con_ts flonian
mtran flon $con_ts "100.00000000 FLON"

mset $con_ts tokensummary

mcli set account permission $con_ts active --add-code







mpush $con_ts view '["flonian"]' -p $con_ts

