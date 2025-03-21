con=vest.1
condir=flon.vest
tcli set contract $con ./build/contracts/$condir -p ${con}@active


owner=owner.1
tnew $owner

issuer=issuer.1
tnew $issuer

recv=$recv.1
tnew $recv


tcli set account permission amax.custody active --add-code
