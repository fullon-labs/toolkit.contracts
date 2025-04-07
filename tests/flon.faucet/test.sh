con=flon.faucet
tnew $con
tset $con flon.faucet
ttran flon $con "10000 FLON"
tcli set account permission $con active --add-code



tpush $con active '["aaaabbbbcccc", "FU6qouWrm3hKL9aCm9n4qkgmBPua14aXZVZM8TJjPJDWXPADHmX4"]' -p flon
tpush $con claim  '["test1"]' -p flon