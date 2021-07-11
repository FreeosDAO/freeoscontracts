eosio-cpp -o freeosconfig.wasm freeosconfig.cpp -DTEST_BUILD -DFREEOS="\"freeos1\"" -DFREEOSCONFIG="\"freeoscfg1\"" -DFREEOSTOKENS="\"freeostoken1\"" -DDIVIDEND="\"optionsdiv1\"" --abigen

3d6c38bb8287374871781e7e0242638da284cdd5


cleos -u https://protontestnet.greymass.com push transaction '{
  "actions": [
    {
      "account": "cron",
      "name": "addcron",
      "data": {
        "account": "freeos",
        "contract": "freeos",
        "last_process": "2021-05-30T11:30:00.000",
        "seconds_interval": 300
      },
      "authorization": [
        {
          "actor": "freeos",
          "permission": "active"
        }
      ]
    }
  ]
}'