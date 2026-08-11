# Order Matching
## Introduction

Built an order matching system in C. 

The program simulates orders arriving with a random side, type, price, and size and get matched using price-time priority. 

Limit orders rest in the book at a specified price if they aren't filled immediately, and market orders take whatever liquidity that is currently available, and are discarded if they can't be filled. Cancelling resting orders before they trade is also available. 

## Results

10,000 orders were processed in 0.002 seconds. 

- 1986 Market orders
- 8014 Limit orders
- Best bid: $99.65
- Best ask: $99.86
- Spread: 0.21
- Total trades: 9,254
- Total volume: 112,853
- VWAP: $99.96
- 107 orders cancelled
- Remaining resting orders: 421

## Assumptions

- Prices are generated in 1 cent ticks
- No transaction costs or fees
- Only simple limit and market orders were implemented
- side, type, price, and size are entirely randomly generated

## How to Run
```
git clone https://github.com/adi-h06/order-matching.git
gcc order_matching.c -o order_matching
./order_matching
```