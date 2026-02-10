

con=flon.rp3
user=usertest3
tnew $user
mcli transfer $user $con "1.00000000 FLON" "awrap:code1123:random:0:5:abcd1234"


mcli get table $con $con redpacks
admin=redpackadmin
con=flon.rp3

mpush $con claimredpack '["usertest3", "code1123", "abcd1234"]' -p $admin

tnew usertes1
tnew usertes2
tnew usertes3
mpush $con claimredpack '["usertes1", "code1123", "abcd1234"]' -p $admin
mpush $con claimredpack '["usertes2", "code1123", "abcd1234"]' -p $admin
mpush $con claimredpack '["usertes3", "code1123", "abcd1234"]' -p $admin


tnew usertes4
mpush $con claimredpack '["usertes4", "code1123", "abcd1234"]' -p $admin






mpush flon.token transfer '["gahbnbehaskk", "flon.redpack", "1.00000000 FLON", "awrap:jqembth2gttd:random:0:2:30f9d7a87aa168fa38fe921288864cd542e2118fe8a2417f1ec5b86a8807f2a1"]' -p gahbnbehaskk

mpush cisum.token transfer '["flonian", "flon.redpack", "1.0000 CISUM", "awrap:jqembth2gtte:random:0:2:30f9d7a87aa158fa38fe921288864cd542e2118fe8a2417f1ec5b86a8807f2a1:dddd"]' -p flonian



mpush flon.redpack claimredpack '["usertest3", "code1325", "code1325"]' -p redpackadmin@owner
mpush flon.redpack claimredpack '["gahbnbehaskk", "code1325", "code1325"]' -p redpackadmin@owner
mpush flon.redpack claimredpack '["flontest", "code1325", "code1325"]' -p redpackadmin@owner
mpush flon.redpack claimredpack '["flonian", "code1325", "code1325"]' -p redpackadmin@owner
mpush flon.redpack claimredpack '["bscn1rn5ttv2", "code1325", "code1325"]' -p redpackadmin@owner
mpush flon.redpack claimredpack '["stt4mqz2ag2t", "code1325", "code1325"]' -p redpackadmin@owner




mpush flon.redpack   delclaims '[100]'  -p flon.redpack


mpush flon.redpack listtoken '["sing.token", "8,SING"]' -p flonian
mpush flon.redpack listtoken '["cisum.token", "4,CISUM"]' -p flonian
