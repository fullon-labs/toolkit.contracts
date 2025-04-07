con=faucet.1
tnew $con
tset $con flon.faucet
ttran flon $con "100 FLON"
tcli set account permission $con active --add-code



tpush $con active '["aaaab", "FU6qouWrm3hKL9aCm9n4qkgmBPua14aXZVZM8TJjPJDWXPADHmX4"]' -p flon