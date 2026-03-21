# Systems

v1.0.0

## Rules

- C99 preferred for new C code
- Freestanding for OS code (NullOS) -- no libc
- Each project is standalone, no shared dependencies
- Makefiles for C/C++ projects
- no emojis

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

# profiler
cd profiler && make && python profiler_cli.py

# static-analyzer
cd static-analyzer && python src/analyzer.py <file>
```

## Key Files

- nullC/src/main.c -- Compiler entry point for ARM64 macOS targets
- NullOS/src/kernel/main.c -- Kernel entry and core initialization for the custom x86 OS
- shell/src/shell.c -- Unix shell with pipes, redirects, env vars, builtins, background jobs, signals
- debugger/main.cpp -- ptrace debugger entry point with breakpoints, single-step, registers, memory r/w
- text-editor/src/editor.py -- Terminal editor with line numbers, search/replace, undo/redo
- container-runtime/src/main.rs -- Linux container runtime entry point
- profiler/profiler.c -- CPU profiler core with 1ms sampling and flame graph output
- static-analyzer/src/analyzer.py -- AST analyzer for unused vars/imports detection
