# Shell - Claude Notes

## Overview
Unix shell built from scratch in C99. Single file (~1,930 LOC). Tokenizer, glob expansion, command substitution, pipeline builder, fork/exec/waitpid, pipes (pipe/dup2), redirects, env var expansion, builtins, job control, signal handling, &&/|| logical operators, script mode. Zero deps beyond POSIX.

## Build & Run
```bash
cd ~/Documents/Code/systems/shell
make
./shell
./shell script.sh   # script mode
```

## Architecture
```
Input -> Tokenizer -> Glob Expander -> Command List -> Pipeline Builder -> Executor
```
- **Tokenizer**: handles single/double quotes, `$VAR`/`${VAR}`/`$?` expansion, `$(cmd)` command substitution, `~` tilde expansion, operators (`|`, `||`, `&&`, `>`, `>>`, `<`, `&`)
- **Glob expander**: POSIX `glob()` for `*`, `?`, `[...]` patterns
- **Command list**: splits on `&&`/`||`, executes conditionally based on `last_exit_status`
- **Pipeline builder**: splits tokens into command structs with redirect info
- **Executor**: fork/execvp for external commands, pipe() + dup2 for pipelines, open/dup2 for redirects
- **Builtins**: cd (with `~` expansion), exit, export, history (ring buffer), fg, bg, alias, source/.
- **Signals**: SIGINT/SIGTSTP ignored in parent, SIG_DFL in children. SIGCHLD reaps bg jobs.
- **Source/eval**: `eval_line()` runs tokenize->expand->execute. `source_file()` reads file line-by-line. `.shellrc` loaded on startup.
- **Script mode**: `./shell script.sh` sources the file and exits.

## Tests
```bash
make && bash tests/test_shell.sh
```
34 tests, all passing.

## Status
v1.1.0 -- Core scripting primitives. Next: v1.2 (control flow + functions).
