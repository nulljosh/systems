![Shell](icon.svg)

# Shell

![version](https://img.shields.io/badge/version-v1.1.0-blue)

A Unix shell built from scratch in C99 -- command parsing, execution, piping, redirects, job control, and scripting primitives.

## Build & Run

```bash
make
./shell
```

Script mode:

```bash
./shell script.sh
```

## Features

### Interactive
- REPL with raw-mode line editor (termios)
- Prompt with `~/` home substitution + git branch display
- Up/down history navigation + Ctrl+R reverse search
- Tab completion (PATH commands + cwd files)
- Aliases with recursive expansion

### Scripting (v1.1)
- Glob expansion (`*.c`, `?`, `[a-z]`)
- `&&` and `||` logical operators
- Exit status tracking (`$?`)
- Command substitution (`$(cmd)`)
- Tilde expansion everywhere (`~/...`)
- `source` / `.` builtin
- `.shellrc` loading on startup
- Script file execution (`shell script.sh`)
- Comment stripping (`# ...`)

### Core
- Command parsing with single/double quote support
- Environment variable expansion (`$VAR`, `${VAR}`, inside double quotes)
- Pipes (`ls | grep .c | wc -l`)
- Redirects (`>`, `>>`, `<`)
- Builtins: `cd`, `exit`, `export`, `history`, `fg`, `bg`, `alias`, `source`
- Background jobs (`sleep 10 &`)
- Signal handling (SIGINT/SIGTSTP ignored in parent, forwarded to children)

## Architecture

```
Input -> Tokenizer -> Glob Expander -> Command List -> Pipeline Builder -> Executor
              |                              |                |                |
         quotes, $VAR,              &&/|| splitting     redirects         fork/exec/pipe
         $(), tilde                                    parsed here       dup2 for I/O
```

Single file: `src/shell.c` (~1,930 lines C99, zero dependencies beyond POSIX).

## Tests

```bash
make && bash tests/test_shell.sh
```

34 tests covering: echo, quoting, cd, pwd, env vars, pipes, redirects, history, exit codes, `$?`, `&&`/`||`, tilde, `$(...)`, source, script mode, globs.

## Roadmap

### v1.2 -- Control Flow & Functions
- `if / elif / else / fi`
- `for x in ... ; do ... ; done`
- `while / until` loops
- `case / esac`
- Functions `name() { ... }`
- `local` variables + `return`
- Here documents `<<EOF`

### v2.0 -- Advanced Scripting
- Arrays, parameter expansion (`${var:-default}`)
- Brace expansion (`{a,b}`, `{1..10}`)
- Arithmetic `$(( x + 1 ))`
- `trap`, `set -e/-x/-u`, subshells, process substitution

### v2.5 -- Fish/Zsh Interactive
- Syntax highlighting (real-time token colorization)
- Autosuggestions (ghost text from history)
- Programmable completion
- Right prompt (RPROMPT)
- `**` recursive glob

### v3.0 -- Polish & Compliance
- POSIX sh compliance mode
- Man page, comprehensive test suite, benchmarks
- Spelling correction, plugin/theme system

## Architecture

See [architecture.svg](architecture.svg) for a visual overview.
