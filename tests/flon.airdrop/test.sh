#!/bin/bash
shopt -s expand_aliases
source ~/.bashrc

airdrop_con=airdrop11125
mreg flon $airdrop_con flonian
mtran flon $airdrop_con "100.00000000 FLON"
mset $airdrop_con flon.airdrop
mcli set account permission $airdrop_con active --add-code

admin=flonian
issuer_owner=airdp.issuer
mreg flon $issuer_owner flonian
mtran flon $issuer_owner "100.00000000 FLON"

mpush $nestar_token  addwhitelist '[ "'$airdrop_con'"]'   -p $nestar_token

mpush $airdrop_con init '["flonian", ["myadmin","airdp.issuer"]]' -p $airdrop_con


mpush $airdrop_con addtoken '["flon.token", "8,FLON"]' -p $admin
mpush $airdrop_con addtoken '["cisum.token", "8,CISUM"]' -p $admin
mpush $airdrop_con addtoken '["nestar.token", "4,NESTAR"]' -p $admin

mpush $airdrop_con deltoken '["nest21.token", "4,NESTAR"]' -p $admin
mpush $airdrop_con deltoken '["flon.token", "8,FLON"]' -p $admin
mpush $airdrop_con deltoken '["cisum.token", "8,CISUM"]' -p $admin


mpush $airdrop_con addplan '[
  "test",
  "2025-09-10T00:00:00",
  "2025-09-20T23:59:59",
  [
    { "quantity":"10.00000000 CISUM", "contract":"cisum.token" }
  ]
]' -p $admin


mpush cisum.token transfer '["flonian", "'$airdrop_con'", "5000.00000000 CISUM", "plan:1"]' -p flonian

mpush nest21.token transfer '["nes11.issuer", "'$airdrop_con'", "5000.0000 NESTAR", "plan:1"]' -p nes11.issuer

mpush $airdrop_con claimairdrop '[1,"myadmin","user:gahbnbehaskk"]' -p myadmin



mpush $airdrop_con addplan '[
  "yesagasgs测试测试",
  "2025-09-01T00:00:00",
  "2025-09-20T23:59:59",
  [
     {"quantity":"10.00000000 CISUM","contract":"cisum.token"},
     {"quantity":"5.0000 NESTAR","contract":"nest21.token"},
     {"quantity":"5.00000000 FLON","contract":"flon.token"}
  ]
]' -p $admin


mpush cisum.token transfer '["flonian", "'$airdrop_con'", "20.00000000 CISUM", "plan:2"]' -p flonian

mpush nest21.token transfer '["nes11.issuer", "'$airdrop_con'", "10.0000 NESTAR", "plan:2"]' -p nes11.issuer


mpush $airdrop_con claimairdrop '[2,"myadmin","user:gahbnbehaskk"]' -p myadmin

mpush flon.token transfer '["flon", "'$airdrop_con'", "20.00000000 FLON", "plan:2"]' -p flon

mpush $airdrop_con claimairdrop '[2,"myadmin","user:gahbnbehaskk"]' -p myadmin