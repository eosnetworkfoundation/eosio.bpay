# EOS Block Pay (`eosio.bpay`) [![Bun Test](https://github.com/eosnetworkfoundation/eosio.bpay/actions/workflows/test.yml/badge.svg)](https://github.com/eosnetworkfoundation/eosio.bpay/actions/workflows/test.yml)

## Overview

The `eosio.bpay` contract handles system block pay distribution earned by fees.

```mermaid
graph TD
    eosio --> |System fees in EOS| eosio.fees
    eosio.fees --> eosio.bpay
    eosio.bpay --> top{Top 21 Producers}
    top --> bp1
    top --> bp2
    top --> bp...
    top --> bp20
    top --> bp21
```

## Development and Testing

### Build Instructions

To compile the contract, developers can use the following command:

```sh
$ cdt-cpp eosio.bpay.cpp -I ./include
```

### Testing Framework

The contract includes a comprehensive testing suite designed to validate its functionality. The tests are executed using the following commands:

```sh
$ npm test

> test
> bun test
```
