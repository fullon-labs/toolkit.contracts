#!/bin/bash
shopt -s expand_aliases
source ~/.bashrc

con=flon.blind1
mreg flon $con flonian
mset $con flon.blindbox
admin=redpackadmin
mreg flonian $admin flonian
mcli set account permission $con active --add-code

mpush $con init '["6,MUST","flon.mtoken"]' -p $con



# sell:
user=user1

#push action amax.ntoken transfer '["user1","martf",[[$amout, [$tokenid, $prentid]]],"$price"]' -p user1

mpush flon.ntoken transfer '["'$user'","'$con'",[[2, [20000, 7]]],"10"]' -p $user



buy:

push action amax.mtoken transfer '["merchantx","martf","20.000000 MUSDT","$tokenid:$orderid:$price"]' -p merchantx

push action amax.mtoken transfer '["merchantx","martf","20.000000 MUSDT","20000:2:10"]' -p merchantx





