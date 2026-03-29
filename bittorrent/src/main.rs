use std::env;

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() < 3 {
        eprintln!("Usage: bittorrent download <file.torrent>");
        std::process::exit(1);
    }
    println!("Downloading: {}", args[2]);
    // TODO: parse torrent, connect to tracker, download pieces
}
