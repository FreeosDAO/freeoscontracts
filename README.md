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
For detailed list, refer this official documentation from EOS.IO help document.
<link>

```
$ cleos -h
```

## Detailed Steps 

