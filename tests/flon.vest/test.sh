con=vest.1
condir=flon.vest
# tset $con $condir 


owner=owner.1
# tnew $owner

issuer=issuer.1
# tnew $issuer

recv=recv.1
# tnew $recv


tcli set account permission $con active --add-code

tpush $con init '["0.01000000 FLON", "flonadmin"]' -p $con@active


tpush ${con} addplan '[ "'${owner}'", "锁仓FLON 一年 ", "flon.token", "8,FLON ",1,10] ' -p $owner


tpush flon.token transfer '[ "'$issuer'", '${con}', "1.00000000 FLON", "plan:2"] ' -p $issuer