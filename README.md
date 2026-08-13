# Order Matching
## Introduction

Built an order matching system in C. 

The program simulates orders arriving with a random side, type, price, and size and get matched using price-time priority. 

Limit orders rest in the book at a specified price if they aren't filled immediately, and market orders take whatever liquidity that is currently available, and are discarded if they can't be filled. Canceling resting orders before they trade is also available. 

Both insertion and cancellation of orders are O(1).

## Results

100,000 orders were processed in 15 milliseconds. 

- 19,996 Market orders
- 80,004 Limit orders
- Best bid: $100.05
- Best ask: $100.32
- Spread: 0.27
- Total trades: 91,845
- Total volume: 1,134,320
- VWAP: $99.98
- 1,020 orders cancelled
- Remaining resting orders: 4,849

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