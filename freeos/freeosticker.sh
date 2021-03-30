cleos -u https://api-testnet-proton.eosarabia.net push action freeos333333 tick '[]' -p freeosticker -x 600 -s -d -j >transaction.json
cleos -u https://api-testnet-proton.eosarabia.net sign transaction.json -k $FREEOS_TICKER -p

