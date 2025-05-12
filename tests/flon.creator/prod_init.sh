# 启用 alias
shopt -s expand_aliases

# 加载 alias，比如 tcli、tset、ttran、tnew
source ~/.bashrc

# 设置变量
con="flon.creator"
FROM_ACCOUNT="flon"
TRANSFER_AMOUNT="1000 FLON"

# 检查账号是否存在
mcli get account "$con"

admin_pk=FUxxxx
con_pk=FUxxxx2
mreg flon securewarden $admin_pk
mreg flon "$con" $con_pk

#mnew securewarden
#mnew "$con"
sleep 1

# 部署合约代码
mset "$con" "$con"
sleep 1

# 转账初始化资金
mtran "$FROM_ACCOUNT" "$con" "$TRANSFER_AMOUNT"
sleep 1

# 添加执行权限
mcli set account permission "$con" active --add-code
