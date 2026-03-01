# Container Runtime - Claude Notes

## Overview
Minimal Linux container runtime. Namespaces, cgroups, filesystem isolation. Rust + C FFI.

## Stack
Rust (unsafe), C FFI, Linux syscalls (not macOS-native — needs Linux VM or Docker)

## Build
```bash
cd ~/Documents/Code/container-runtime
cargo build
```

## Modules
- `main.rs` -- clap CLI (`run` subcommand with `--memory`, `--cpu-weight`, `--hostname`)
- `container.rs` -- lifecycle orchestration (image -> cgroup -> namespace -> cleanup)
- `namespace.rs` -- `clone()` with NEWPID/NEWNS/NEWUTS/NEWNET, child exec
- `cgroup.rs` -- cgroups v2: memory.max, cpu.weight, cgroup.procs
- `filesystem.rs` -- pivot_root, mount proc/sysfs, umount oldroot
- `image.rs` -- tar/gzip unpacking into rootfs
- `network.rs` -- veth pair stub

## Status
Done/stable. Compiles on macOS (Linux-only code gated with `#[cfg(target_os = "linux")]`). 2 tests pass on macOS.
