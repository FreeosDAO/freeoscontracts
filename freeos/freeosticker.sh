# DEV2 environment
# cleos -u https://protontestnet.greymass.com push action freeosd tick '[]' -p freeosticker -x 600 -s -d -j >transaction0.json
# cleos -u https://protontestnet.greymass.com sign transaction0.json -k $FREEOS_TICKER -p

# QA2 environment
cleos -u https://protontestnet.greymass.com push action freeos tick '[]' -p freeosticker -x 600 -s -d -j >transaction1.json
cleos -u https://protontestnet.greymass.com sign transaction1.json -k $FREEOS_TICKER -p

# Automation environment
# cleos -u https://protontestnet.greymass.com push action freeosa tick '[]' -p freeosticker -x 600 -s -d -j >transaction2.json
# cleos -u https://protontestnet.greymass.com sign transaction2.json -k $FREEOS_TICKER -p

# QA4 (frontend testing) environment
cleos -u https://protontestnet.greymass.com push action freeos4 tick '[]' -p freeosticker -x 600 -s -d -j >transaction4.json
cleos -u https://protontestnet.greymass.com sign transaction4.json -k $FREEOS_TICKER -p

# AlphaUK environment
#cleos -u https://protontestnet.greymass.com push action freeosu tick '[]' -p freeosticker -x 600 -s -d -j >transaction5.json
#cleos -u https://protontestnet.greymass.com sign transaction5.json -k $FREEOS_TICKER -p

