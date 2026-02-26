# Shell - Claude Notes

## Overview
Unix shell built from scratch in C99. Single file (~470 LOC). Tokenizer, pipeline builder, fork/exec/waitpid, pipes (pipe/dup2), redirects, env var expansion, builtins, job control, signal handling. Zero deps beyond POSIX.

## Build & Run
```bash
cd ~/Documents/Code/systems/shell
make
./shell
```

## Architecture
```
Input -> Tokenizer -> Pipeline Builder -> Executor
```
- **Tokenizer**: handles single/double quotes, `$VAR` expansion, operators (`|`, `>`, `>>`, `<`, `&`)
- **Pipeline builder**: splits tokens into command structs with redirect info
- **Executor**: fork/execvp for external commands, pipe() + dup2 for pipelines, open/dup2 for redirects
- **Builtins**: cd (with `~` expansion), exit, export, history (ring buffer), fg, bg
- **Signals**: SIGINT/SIGTSTP ignored in parent, SIG_DFL in children. SIGCHLD reaps bg jobs.

## Tests
```bash
make && bash tests/test_shell.sh
```

## Status
v2.0.0 -- C99 rewrite. Python version archived in `python/`.
