# Incognito L1

Copyright (c) 2017-2026, The Incognito Project. P2P FOREVER.
Portions Copyright (c) 2014-2024, The Monero Project.
Portions Copyright (c) 2012-2013, The Cryptonote developers.

## Introduction

Incognito L1 is a private, secure, untraceable, decentralised Proof-of-Work digital currency forked from the Monero codebase. It uses the same privacy technologies (Ring Signatures, RingCT, Bulletproofs+) as Monero but with custom economic parameters and an independent network.

**Privacy:** Incognito L1 uses a cryptographically sound system to allow you to send and receive funds without your transactions being easily revealed on the blockchain. This ensures that your purchases, receipts, and all transfers remain private by default.

**Security:** Using the power of a distributed peer-to-peer consensus network, every transaction on the network is cryptographically secured. Individual wallets have a 25-word mnemonic seed that is only displayed once and can be written down to backup the wallet.

**Untraceability:** By taking advantage of ring signatures, a special property of a certain type of cryptography, Incognito L1 is able to ensure that transactions are not only untraceable but have an optional measure of ambiguity that ensures that transactions cannot easily be tied back to an individual user or computer.

**Decentralization:** The utility of Incognito L1 depends on its decentralised peer-to-peer consensus network - anyone should be able to run the incognito software, validate the integrity of the blockchain, and participate in all aspects of the incognito network using consumer-grade commodity hardware.

## Key Specifications

| Parameter | Value |
|-----------|-------|
| Total Supply | 888,888,888 L1 |
| Decimal Places | 10 |
| Block Time | 120 seconds (v2) |
| Consensus | Proof of Work (CryptoNight v7-v10, WhirlpoolX v11+) |
| P2P Port (mainnet) | 39001 |
| RPC Port (mainnet) | 39002 |
| ZMQ Port (mainnet) | 39003 |
| P2P Port (testnet) | 39011 |
| RPC Port (testnet) | 39012 |
| Address Prefix | 0x69F9 |

## Compiling Incognito L1 from source

### Dependencies

See the Monero README for the full dependency list. The same dependencies apply.

### Build instructions

#### On Linux and macOS

```bash
cd incognito-l1
make
```

The resulting executables can be found in `build/release/bin`

* Run Incognito L1 with `./bin/incognitod --detach`

#### On Windows

Follow the Monero build instructions for Windows (MSYS2/MinGW), using this repository instead.

## Running incognitod

The build places the binary in `bin/` sub-directory within the build directory from which cmake was invoked. To run in the foreground:

```bash
./bin/incognitod
```

To run in background:

```bash
./bin/incognitod --log-file incognitod.log --detach
```

To run as a systemd service, copy [incognitod.service](utils/systemd/incognitod.service) to `/etc/systemd/system/` and [incognitod.conf](utils/conf/incognitod.conf) to `/etc/`.

## License

See [LICENSE](LICENSE).

## Contributing

If you want to help out, see [CONTRIBUTING](CONTRIBUTING.md) for a set of guidelines.

## Acknowledgements

Incognito L1 is based on the Monero project (https://github.com/monero-project/monero). We gratefully acknowledge the work of the Monero Project and all its contributors.
