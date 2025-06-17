#!/bin/bash
shopt -s expand_aliases
source ~/.bashrc


con=flon.vest
mreg flon flon.vest flonian
mset $con $con

# 主要链上测试账户（需提前创建好）
owner=flonian      # 锁仓创建人
issuer=flonian     # 发起锁仓转账（和 owner 相同，演示用）
recv=flonvestrecv  # 实际领取人，可另设

# EOSIO 合约必须加 code 权限
mcli set account permission $con active --add-code

# 合约初始化: ["admin账户","方案费用 asset", "费用收款账户"]
mpush $con init '["flonian", "0.01000000 FLON", "flonadmin"]' -p $con@active

# 创建锁仓方案: [owner, title, asset_contract, asset_symbol, unlock_interval_days, unlock_times]
mpush $con addplan '["'"${owner}"'", "锁仓FLON一年", "flon.token", "8,FLON", 30, 12]' -p $owner

# 缴费激活（plan_id假设为1，实际可根据表查，通常是你刚创建的plan的id）
mpush flon.token transfer '["'"${issuer}"'", "'"${con}"'", "0.01000000 FLON", "plan:1"]' -p $issuer

# 实际锁仓资产给合约（比如锁仓 10 FLON，第一期30天后解锁）
mpush flon.token transfer '["'"${issuer}"'", "'"${con}"'", "10.00000000 FLON", "issue:'"${recv}"':1:30"]' -p $issuer

