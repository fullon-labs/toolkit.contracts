

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
