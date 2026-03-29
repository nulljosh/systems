# bittorrent
v0.1.0
## Rules
- Project is a BitTorrent client in Rust
- Rust 2021 edition
- tokio for async networking
- No unsafe blocks unless absolutely necessary
- no emojis
## Run
```bash
cargo build
cargo build --release
./target/release/bittorrent download file.torrent
cargo test
cargo clippy
```
## Key Files
- `src/main.rs` -- CLI entry point
- `src/bencode.rs` -- Bencode encoder/decoder
- `src/torrent.rs` -- .torrent file parser
- `src/tracker.rs` -- HTTP/UDP tracker protocol
- `src/peer.rs` -- Peer wire protocol (TCP)
- `src/piece.rs` -- Piece selection and verification (SHA-1)
- `src/disk.rs` -- File I/O and piece assembly
