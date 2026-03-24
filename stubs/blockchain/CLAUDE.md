# blockchain

Proof-of-work blockchain in Rust.

## Architecture

- `src/main.rs` -- Node CLI
- `src/block.rs` -- Block structure, hashing (SHA-256)
- `src/chain.rs` -- Chain validation, longest chain rule
- `src/transaction.rs` -- Transaction creation and verification
- `src/miner.rs` -- Proof-of-work mining (difficulty target)
- `src/mempool.rs` -- Unconfirmed transaction pool
- `src/network.rs` -- P2P gossip protocol

## Dev

```bash
cargo build
cargo test
```

## Conventions

- Rust 2021 edition
- SHA-256 for block hashing
- Adjustable difficulty target
