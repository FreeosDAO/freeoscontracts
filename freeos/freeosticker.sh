cleos -u https://kylin-dsp-1.liquidapps.io push action freeos333333 tick '[]' -p freeosticker -x 600 -s -d -j >transaction.json
cleos -u https://kylin-dsp-1.liquidapps.io sign transaction.json -k $FREEOS_TICKER -p

