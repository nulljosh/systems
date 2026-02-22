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

## Goals
- `mycontainer run <image> <cmd>` CLI
- PID, mount, network, UTS namespaces
- Cgroups v2 (CPU, memory limits)
- chroot/pivot_root filesystem isolation
- OCI tarball image unpacking
- veth pair networking (stretch)

## Status
Systems/learning project. Done/stable.
