# Systems

## Rules

- C99 preferred for new C code
- Freestanding for OS code (NullOS) -- no libc
- Each project is standalone, no shared dependencies
- Makefiles for C/C++ projects

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
