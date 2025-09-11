con=flon.split

admin=flonian
mreg flon $con flonian
mset $con flon.split
mcli set account permission $con active --add-code

const name& owner, const string& title, const vector<split_unit_s>& conf, const bool& is_auto

mpush con addplans '[31,"测试",[[test1,5000],[test2,5000]],1]' -ptest2