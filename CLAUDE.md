# systems

Low-level systems programming monorepo -- eight standalone projects.

## Projects

- **nullC** (C) -- C compiler targeting ARM64. Flagship project. `make && ./nullc examples/hello.c`
- **NullOS** (C, x86 ASM) -- Custom x86 OS. `make run` (requires QEMU)
- **shell** (C) -- Unix shell. `make && ./shell`
- **debugger** (C++) -- ptrace debugger. `make && ./debugger <program>`
- **text-editor** (Python) -- Terminal editor. `python src/editor.py [file]`
- **container-runtime** (Rust) -- Linux container runtime. Not yet implemented
- **profiler** (Python, C) -- CPU profiler. `make && python profiler_cli.py`
- **static-analyzer** (Python) -- AST analyzer. `python src/analyzer.py <file>`

## Conventions

- C99 preferred for new C code
- Freestanding for OS code (NullOS) -- no libc
- Each project is standalone with its own README, CLAUDE.md, and architecture.svg
- Makefiles for C/C++ projects
- No shared dependencies between projects
