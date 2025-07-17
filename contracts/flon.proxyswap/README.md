# ProxySwap

## Introduction

This is to help swap tokens by proxing the swap orders over to 3rd-party liquidity providers like CEX (E.g. Binance) or DEX (E.g. uniswap)...etc

## Workflow

1. a user sends swap orders to `proxyswap` contract
1. the backend recieves the order and processes it from a selected 3rd-party LP and have the "mirrored" order fufilled
1. the backend admin submits an settlement transaction to the contract and the user recieves the counterparty token in his or her own wallet (proxy account)
1. the user might want to bridge the tokens from the proxy account to a 3rd chain
