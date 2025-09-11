con=flon.split

admin=flonian
mreg flon $con flonian
mset $con flon.split
mcli set account permission $con active --add-code
