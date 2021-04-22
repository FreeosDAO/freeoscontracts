cleos -u https://protontestnet.greymass.com push action freeos333333 tick '[]' -p freeosticker -x 600 -s -d -j >transaction.json
cleos -u https://protontestnet.greymass.com sign transaction.json -k $FREEOS_TICKER -p

