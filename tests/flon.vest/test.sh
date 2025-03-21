con=vest.1
condir=flon.vest
tcli set contract $con ./build/contracts/$condir -p ${con}@active


owner=owner.1
tcli system newaccount flon $owner FO8ixPk3x4wZQu1bwBtw67JznFr5LVcA9bfDpkS7grnms3JNm7Qq  --transfer-quant "5.000000 FLON" -p flon


