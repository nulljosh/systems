# Systems

v2.0.1

From-scratch implementations. Compiler, OS, networking, graphics, AI, and more.

## Rules

- C99 preferred for new C code. Rust and Python also used.
- Freestanding for OS code (NullOS) -- no libc
- Each project is standalone, no shared dependencies
- Makefiles for C projects, Cargo for Rust, requirements.txt for Python
- no emojis

## Projects

### Core Systems (C/C++/Rust)
- **nullC** -- C compiler targeting ARM64 macOS
- **NullOS** -- x86 operating system (QEMU)
- **shell** -- Unix shell with pipes, redirects, job control
- **debugger** -- ptrace debugger with breakpoints, single-step
- **text-editor** -- Terminal editor (Python)
- **container-runtime** -- Linux container runtime (Rust)
- **memory-allocator** -- Custom malloc implementation
- **profiler** -- CPU profiler with flame graphs
- **static-analyzer** -- AST analyzer for unused vars/imports
- **processor** -- CPU emulator
- **emulator** -- System emulator

### Networking & Infrastructure
- **bittorrent** -- BitTorrent client (Rust)
- **blockchain** -- Blockchain with proof-of-work (Rust)
- **git-clone** -- Git implementation (C)
- **regex-engine** -- NFA/DFA regex engine (C)
- **template-engine** -- HTML template engine (Python)

### Graphics & Simulation
- **physics-engine** -- 2D/3D rigid body dynamics (C)
- **voxel-engine** -- Minecraft-style voxel renderer (Rust)
- **game** -- 2D game engine with SDL2 (C)
- **augmented-reality** -- AR engine (Swift)
- **visual-recognition** -- CNN image classifier from scratch (Python)

### Other
- **bots/** -- Node.js bots: compass (navigation), dominos (ordering), weedbot (strain tracker)
- **jot** -- Jot language (see standalone jot repo for latest)
- **stubs** -- Build-your-own scaffolds

## Run

```bash
# nullC
cd nullC && make && ./nullc examples/hello.c

# NullOS
cd NullOS && make run  # requires QEMU

# shell
cd shell && make && ./shell

# debugger
cd debugger && make && ./debugger <program>

# text-editor
cd text-editor && python src/editor.py [file]

# Rust projects (bittorrent, blockchain, voxel-engine, container-runtime)
cd <project> && cargo build && cargo run

# C projects (physics-engine, game, regex-engine, git-clone)
cd <project> && make && ./<binary>

# Python projects (template-engine, visual-recognition)
cd <project> && pip install -r requirements.txt && python -m <module>
```
