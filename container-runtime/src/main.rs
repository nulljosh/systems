mod container;
mod namespace;
mod cgroup;
mod filesystem;
mod image;
mod network;

use clap::{Parser, Subcommand};

#[derive(Parser)]
#[command(name = "mycontainer", about = "Minimal Linux container runtime")]
struct Cli {
    #[command(subcommand)]
    command: Commands,
}

#[derive(Subcommand)]
enum Commands {
    /// Run a command inside a new container
    Run {
        /// Path to OCI image tarball or rootfs directory
        image: String,
        /// Command to execute inside the container
        #[arg(trailing_var_arg = true)]
        cmd: Vec<String>,
        /// Memory limit in bytes (e.g., 67108864 for 64MB)
        #[arg(long, default_value = "67108864")]
        memory: u64,
        /// CPU weight (1-10000, default 100)
        #[arg(long, default_value = "100")]
        cpu_weight: u64,
        /// Hostname for the container
        #[arg(long, default_value = "container")]
        hostname: String,
    },
}

fn main() {
    let cli = Cli::parse();

    match cli.command {
        Commands::Run {
            image,
            cmd,
            memory,
            cpu_weight,
            hostname,
        } => {
            if cmd.is_empty() {
                eprintln!("Error: must specify a command to run");
                std::process::exit(1);
            }
            let config = container::ContainerConfig {
                image_path: image,
                command: cmd,
                memory_limit: memory,
                cpu_weight,
                hostname,
            };
            if let Err(e) = container::run(config) {
                eprintln!("Container error: {}", e);
                std::process::exit(1);
            }
        }
    }
}
