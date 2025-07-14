


owner=anyonetokenx      # 锁仓创建人
title="锁仓计划"
begDays=1
times=3
con=flon.vest  # 你的合约
token=anyonetokenx  # 代币合约

mpush "$con" addplan \
    "[\"$owner\", \"$title$times\", \"$token\", \"8,MUSK\", $begDays, $times]" -p "$owner"



owner=huf3mdivopgz      # 锁仓创建人
title="锁仓计划"
begDays=1
times=1200
con=flon.vest  # 你的合约
token=anyonetokenx  # 代币合约
issuer=huf3mdivopgz     # 发起锁仓转账（和 owner 相同，演示用）
plan_id=2
recv=flonflonflon  # 实际领取人，可另设

mpush $token transfer \
    "[\"$issuer\", \"$con\", \"10.00000000 MUSK\", \"issue:$recv:$plan_id:$begDays\"]" -p "$issuer"

