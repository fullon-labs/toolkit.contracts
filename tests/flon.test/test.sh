con=test.1
condir=flon.test
tset $con $condir

u1=a
tnew $u1
tpush $con add '["'$u1'", ["aaaaaa","bbbbbbb"]]' -p $u1

# b,c

tcli get account a


tpush $con remove '[["a","b"]]' -p $u1

