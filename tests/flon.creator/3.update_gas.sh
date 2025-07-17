con="flon.creator"
mset "$con" "$con"
sleep 1
mpush "$con" setgasquant '{"gas_quant": "1.00000000 FLON"}' -p ${con}@owner