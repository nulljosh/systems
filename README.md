![Systems](icon.svg)

# Systems

![version](https://img.shields.io/badge/version-v1.0.0-blue)

Low-level systems programming monorepo. Compilers, operating systems, shells, debuggers, profilers.

## Projects

- **nullC** (C) -- C compiler targeting ARM64 macOS. All 10 levels passing. Peephole optimization
- **NullOS** (C, x86 ASM) -- Custom x86 OS. Two-stage bootloader, protected mode, VGA, shell
- **shell** (C) -- Unix shell. Pipes, redirects, env vars, builtins, background jobs, signals
- **debugger** (C++) -- ptrace debugger. Breakpoints, single-step, registers, memory r/w
- **text-editor** (Python) -- Terminal editor. Line numbers, search/replace, undo/redo
- **container-runtime** (Rust) -- Linux container runtime (skeleton)
- **profiler** (Python, C) -- CPU profiler. 1ms sampling, flame graph output
- **static-analyzer** (Python) -- AST analyzer. Unused vars/imports detection
- **memory-allocator** -- Custom memory allocator (scaffold)
- **processor** -- CPU simulator (scaffold)
- **emulator** -- System emulator (scaffold)

## Run

Each project is standalone with its own Makefile or entry point. See individual READMEs.

## Roadmap

- nullC: optimizer passes, more codegen targets
- NullOS: filesystem, userspace, ELF loader
- shell: control flow (if/for/while/functions)
- container-runtime: namespaces, cgroups v2, overlay FS
- static-analyzer: additional lint rules, config file

## License

MIT 2026 Joshua Trommel
