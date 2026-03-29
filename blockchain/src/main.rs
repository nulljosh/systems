use std::env;

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        eprintln!("Usage: blockchain <node|mine|send>");
        std::process::exit(1);
    }
    println!("Blockchain node starting...");
    // TODO: initialize chain, start P2P, begin mining
}
