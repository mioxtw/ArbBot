## Setup cmake
```bash
mkdir bin
cd bin
cmake ..
```

## Buid
```bash
make
```

## Run
1. Add your API keys to the clients
2. Run the examples:
```bash
./src/example/arb
```

## Usage
```bash
--------------------------------------------------------
 [Master Mio] FTX Spot Futures Arbitrage Bot [C++] 
--------------------------------------------------------
Usage: ./arb [APIKey] [APISecret] [SubAccount] [open/close] [SpotMarketName] [Size] [Premium]
Example:
       ./arb ABCDEFGHIJ 123456789 SUBACCOUNT open  FTT/USD 10 0.001
       ./arb ABCDEFGHIJ 123456789 SUBACCOUNT close FTT/USDT 10 -0.001
```
