
recv=flonvestrecv  # 实际领取人，可另设
owner=flonian      # 锁仓创建人
issuer=flonian     # 发起锁仓转账（和 owner 相同，演示用）
title="锁仓计划"
begDays=1
con=flon.vest  # 你的合约
plan_id=24
for times in {1200..1230}; do
  echo "# 创建锁仓方案, times=$times"
  mpush "$con" addplan \
    "[\"$owner\", \"$title$times\", \"flon.token\", \"8,FLON\", $begDays, $times]" -p "$owner"
  sleep 1
#   # 获取最新 plan_id（假设顺序创建，直接取最后一条）
# plan_id=$(mcli get table "$con" "$con" plans 2>/dev/null | awk '/^{/{flag=1}flag' | jq -r '.rows[-1].id')
# echo "# plan_id=$plan_id"
    plan_id=${plan_id}+1
  # 实际锁仓资产给合约，比如都锁10个 FLON
  mpush flon.token transfer \
    "[\"$issuer\", \"$con\", \"10.00000000 FLON\", \"issue:$recv:$plan_id:$begDays\"]" -p "$issuer"
done