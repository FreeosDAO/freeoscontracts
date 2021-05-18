<<<<<<< HEAD
cleos -u https://protontestnet.greymass.com push action freeos333333 tick '[]' -p freeosticker -x 600 -s -d -j >transaction.json
cleos -u https://protontestnet.greymass.com sign transaction.json -k $FREEOS_TICKER -p
=======
# DEV2 environment
cleos -u https://protontestnet.greymass.com push action freeosd tick '[]' -p freeosticker -x 600 -s -d -j >transaction0.json
cleos -u https://protontestnet.greymass.com sign transaction0.json -k $FREEOS_TICKER -p

# QA2 environment
cleos -u https://protontestnet.greymass.com push action freeos tick '[]' -p freeosticker -x 600 -s -d -j >transaction1.json
cleos -u https://protontestnet.greymass.com sign transaction1.json -k $FREEOS_TICKER -p

# Automation environment
cleos -u https://protontestnet.greymass.com push action freeosa tick '[]' -p freeosticker -x 600 -s -d -j >transaction2.json
cleos -u https://protontestnet.greymass.com sign transaction2.json -k $FREEOS_TICKER -p
>>>>>>> options

