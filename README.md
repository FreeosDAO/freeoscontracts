# Freeos AirClaim Contracts

This document explains the commands required by the users to perform the FreeOs Smart Contract (the contract) actions.
It includes these smart contracts with set of associated actions, which are briefly described below-

## The freeos contract
> Stored in the freeostokens account
- Create and issue the FREEOS currency. (create and issue actions)
- Registering the user in the 'users' table (reguser action)
- Listening for when the user transfers the required EOS stake to the freeosoffice account. (stake action, which is an observer of the eosio.token contract transfer action)
- Allowing the user to 'unstake' the EOS tokens, subject to business rules. (unstake action, which calls the transfer action)
- Processing a weekly claim request from the user (applying the business rules for eligibility) and then transferring the weekly amount of FREEOS to the user. (claim action, which calls the transfer action)

## The freeosconfig contract
> Stored in the freeosconfig account.

For use by freedao members to maintain various configuration data tables:
- 'parameters' table, for example there is a 'global masterswitch' parameter which can be used to temporarily halt the system in the event of a problem. (actions are paramupsert, paramerase)
- 'stakereqs' table, describing how many EOS tokens are required depending on user type and the number of users already registered. (actions are stakeupsert and stakeerase)
- 'weeks' table. This is the freeos calendar and describes the start/end dates for each week, the amount of FREEOS to be claimed, the amount of FREEOS to be held by the user. (actions are weekupsert and weekerase)
- Various table 'read' actions for use in testing and administration (actions are getweek, getconfig, getthreshold)


### Here are the actions associated with the FreeOS Smart Contract (FSC)-
```
Register
Stake
Unstake
Claim
```

## Steps 
1. A user holds a valid EOS Wallet account (it can be other accounts such as Kylin, Proton, or LiquidApp etc.)
2. The user purchases the necessary computation resources such as CPU, RAM, and Network Bandwidth
3. The user registers for the contract via invoking the reguser action of the FSC to become an AirClaim Participant.
4. Once registered, user calls for the Stake Action of the contract, as per the staking requirement communicated to him/her.
5. The user can also invoke the Unstake action.
6. When required, the Claim action of the contract is invoked


## Prerequisite
The cleos command is installed on the user machine. 
Please note that cleos supports Ubuntu Linux (LTS 16 amd 18) as well as MacOs Catalina.
For detailed list, refer this official documentation from the official EOS.IO 
[help document](https://developers.eos.io/manuals/eos/v2.1/cleos/command-reference/index)

```bash
12:14:56 08/12/2020 ~/Documents/Projects/EOS_DAO$ cleos -h
Command Line Interface to EOSIO Client
Usage: cleos [OPTIONS] SUBCOMMAND

Options:
  -h,--help                   Print this help message and exit
  -u,--url TEXT=http://127.0.0.1:8888/
                              the http/https URL where nodeos is running
  --wallet-url TEXT=unix:///Users/shahid/eosio-wallet/keosd.sock
                              the http/https URL where keosd is running

```
<details>
  <summary> Click here to expand for full output</summary> 
 <p>
   
```bash
12:14:56 08/12/2020 ~/Documents/Projects/EOS_DAO$ cleos -h
Command Line Interface to EOSIO Client
Usage: cleos [OPTIONS] SUBCOMMAND

Options:
  -h,--help                   Print this help message and exit
  -u,--url TEXT=http://127.0.0.1:8888/
                              the http/https URL where nodeos is running
  --wallet-url TEXT=unix:///Users/shahid/eosio-wallet/keosd.sock
                              the http/https URL where keosd is running
  -r,--header                 pass specific HTTP header; repeat this option to pass multiple headers
  -n,--no-verify              don't verify peer certificate when using HTTPS
  --no-auto-keosd             don't automatically launch a keosd if one is not currently running
  -v,--verbose                output verbose errors and action console output
  --print-request             print HTTP request to STDERR
  --print-response            print HTTP response to STDERR


Subcommands:
  version                     Retrieve version information
  create                      Create various items, on and off the blockchain
  convert                     Pack and unpack transactions
  get                         Retrieve various items and information from the blockchain
  set                         Set or update blockchain state
  transfer                    Transfer tokens from account to account
  net                         Interact with local p2p network connections
  wallet                      Interact with local wallet
  sign                        Sign a transaction
  push                        Push arbitrary transactions to the blockchain
  multisig                    Multisig contract commands
  wrap                        Wrap contract commands
  system                      Send eosio.system contract action to the blockchain.

```

</p>
</details> 


## Detailed Steps 
> You need a unique account name with wallet that will be used while executing the actions of the SC. It must not be in use. The name should be less than 13 characters and only contains the following symbol .12345abcdefghijklmnopqrstuvwxyz

> Creating following alias for ease. Run these commands in the terminal (shell)
```bash
alias kylin='cleos -u https://kylin-dsp-1.liquidapps.io'
```

```bash
alias proton='cleos -u https://api-testnet-proton.eosarabia.net:443'
```

### For demonstration I am using my account name `mshahid25nov`.

On the Terminal, execute these commands:
```bash
acc=mshahid25nov
```
## Create above account
```bash
curl http://faucet-kylin.blockzone.net/create/${acc}
```
> Note down the private keys for Owner and Active, when above command is executed.

## Create a Wallet whose password is saved locally in the $acc.pass file
```bash
cleos wallet create --file $acc.pass -n $acc
cat $acc.pass
```

### Now import the private keys to the wallet. These keys are important and if lost, you won't be able to retrieve the account. 
```bash
cleos wallet import -n $acc --private-key <KEY-1>
cleos wallet import -n $acc --private-key <KEY-2>
```
### To see the wallet and password files for the user:
```bash
ls -ltr ${acc}*
```

## Loof for the wallets and which are unlocked (marked with '*'
```
cleos wallet list
```

## If your wallet shows as locked (no '*' against the wallet)
```bash
cleos wallet unlock --password `cat $acc.pass` -n $acc
```

## Get the tokens to buy the computation resources : RAM, CPU, Network Bandwidth
```bash
curl http://faucet-kylin.blockzone.net/get_token/${acc}
curl http://faucet-kylin.blockzone.net/get_token/${acc}
```
> Now buy the resources. You can run the URL in the browser too.
```bash
kylin system buyram $acc $acc "100.0000 EOS"
```
<details>
<summary>Sample Output</summary>
<p>

```
executed transaction: ba038d01103f5f63867e8ebbe1835daa3ba69f5006964c5ad3fc74d1e97e6177  128 bytes  939 us
#         eosio <= eosio::buyram                {"payer":"mshahid25nov","receiver":"mshahid25nov","quant":"100.0000 EOS"}
#   eosio.token <= eosio.token::transfer        {"from":"mshahid25nov","to":"eosio.ram","quantity":"99.5000 EOS","memo":"buy ram"}
#   eosio.token <= eosio.token::transfer        {"from":"mshahid25nov","to":"eosio.ramfee","quantity":"0.5000 EOS","memo":"ram fee"}
#   eosio.token <= eosio.token::transfer        {"from":"eosio.ramfee","to":"eosio.rex","quantity":"0.5000 EOS","memo":"transfer from eosio.ramfee t...
#  mshahid25nov <= eosio.token::transfer        {"from":"mshahid25nov","to":"eosio.ram","quantity":"99.5000 EOS","memo":"buy ram"}
#     eosio.ram <= eosio.token::transfer        {"from":"mshahid25nov","to":"eosio.ram","quantity":"99.5000 EOS","memo":"buy ram"}
#  mshahid25nov <= eosio.token::transfer        {"from":"mshahid25nov","to":"eosio.ramfee","quantity":"0.5000 EOS","memo":"ram fee"}
#  eosio.ramfee <= eosio.token::transfer        {"from":"mshahid25nov","to":"eosio.ramfee","quantity":"0.5000 EOS","memo":"ram fee"}
#  eosio.ramfee <= eosio.token::transfer        {"from":"eosio.ramfee","to":"eosio.rex","quantity":"0.5000 EOS","memo":"transfer from eosio.ramfee t...
#     eosio.rex <= eosio.token::transfer        {"from":"eosio.ramfee","to":"eosio.rex","quantity":"0.5000 EOS","memo":"transfer from eosio.ramfee t...
warning: transaction executed locally, but may not be confirmed by the network yet         ] 

```

</p>
</details>  

```bash
kylin system delegatebw $acc $acc "20.0000 EOS" "80.0000 EOS" -p $acc@active
```
<details>
<summary>Sample Output</summary>
<p>

```
executed transaction: a7e2a544d9715f6dff9601cd104fa93731ee41d5da2f0d1a0f50f74c10ca1896  144 bytes  645 us
#         eosio <= eosio::delegatebw            {"from":"mshahid25nov","receiver":"mshahid25nov","stake_net_quantity":"20.0000 EOS","stake_cpu_quant...
#   eosio.token <= eosio.token::transfer        {"from":"mshahid25nov","to":"eosio.stake","quantity":"100.0000 EOS","memo":"stake bandwidth"}
#  mshahid25nov <= eosio.token::transfer        {"from":"mshahid25nov","to":"eosio.stake","quantity":"100.0000 EOS","memo":"stake bandwidth"}
#   eosio.stake <= eosio.token::transfer        {"from":"mshahid25nov","to":"eosio.stake","quantity":"100.0000 EOS","memo":"stake bandwidth"}
warning: transaction executed locally, but may not be confirmed by the network yet         ] 

```

</p>
</details>  


## Now the resources are acquired, execute the Smart Contract action `reguser` to Register for FreeDAO 
```
kylin push action freeos333333 reguser "[\"${acc}\",\"e\"]" -p $acc@active
```
## Check the state
```bash
kylin push action freeos333333 getuser "[\"${acc}\",\"e\"]" -p $acc@active
```

## Look into the `users` and `claims` tables
```bash
kylin get table freeos333333 $acc users
```

```
kylin get table freeos333333 $acc claims
```

<details>
<summary>Sample Output</summary>
<p>

```bash
06:30:38 03/12/2020 ~/Documents/Projects/EOS_DAO$ kylin get table freeos333333 $acc users
{
  "rows": [{
      "stake": "0.0000 EOS",
      "account_type": 101,
      "stake_requirement": "10.0000 EOS",
      "registered_time": "2020-12-03T05:24:07",
      "staked_time": "1970-01-01T00:00:00"
    }
  ],
  "more": false,
  "next_key": ""
}
06:31:27 03/12/2020 ~/Documents/Projects/EOS_DAO$ kylin get table freeos333333 $acc claims
{
  "rows": [],
  "more": false,
  "next_key": ""
}

```

</p>
</details>  



## Look into the `weeks` table
```
kylin get table freeosconfig freeosconfig weeks
```
## Check the token and currency balances before `transfer` action to Stake it
```bash
kylin get currency balance eosio.token $acc EOS
kylin get currency balance freeos333333  $acc FREEOS
kylin get currency balance eosio.token freeos333333 EOS
kylin get currency balance freeos333333 freeos333333 FREEOS
kylin get currency balance freeos333333 freedao33333 FREEOS
```

## Execute the Stake 
```bash
kylin push action eosio.token transfer "[\"${acc}\",\"freeos333333\", \"10.0000 EOS\", \"${acc} stake to freeos\"]" -p $acc@active
```
<details>
<summary>Sample `transfer` Output</summary>
<p>

```
executed transaction: e831138bfe06931b900289d5ff2aa54d0e4be26d374011d76ef59b3253b2ddc9  160 bytes  364 us
#   eosio.token <= eosio.token::transfer        {"from":"mshahid25nov","to":"freeos333333","quantity":"10.0000 EOS","memo":"mshahid25nov stake to fr...
#  mshahid25nov <= eosio.token::transfer        {"from":"mshahid25nov","to":"freeos333333","quantity":"10.0000 EOS","memo":"mshahid25nov stake to fr...
#  freeos333333 <= eosio.token::transfer        {"from":"mshahid25nov","to":"freeos333333","quantity":"10.0000 EOS","memo":"mshahid25nov stake to fr...
>> 10.0000 EOS stake received for account mshahid25nov
warning: transaction executed locally, but may not be confirmed by the network yet         ] 

```

</p>
</details>  

## Check the token and currency balances after `transfer` action to Stake
```bash
kylin get currency balance eosio.token $acc EOS
kylin get currency balance freeos333333 $acc FREEOS
kylin get currency balance eosio.token freeos333333 EOS
kylin get currency balance freeos333333 freeos333333 FREEOS
kylin get currency balance freeos333333 freedao33333 FREEOS
```


## Get the details
```bash
kylin push action freeos333333 getuser "[\"${acc}\"]" -p $acc@active
```

## CLAIM
```bash
kylin push action freeos333333 claim "[\"${acc}\"]" -p $acc@active
```

## UNSTAKE
```bash
kylin push action freeos333333 unstake "[\"${acc}\"]" -p $acc@active
```

## If you have access to the wallets of freeosconfig and freeos333333 accounts, you can get the week table entries
```bash
kylin push action freeosconfig getweek '[1]' -p freeosconfig@active
kylin push action freeosconfig getweek '[2]' -p freeosconfig@active
kylin push action freeosconfig getweek '[3]' -p freeosconfig@active
kylin push action freeos333333 getcounts '[]' -p freeos333333@active
```
