# jot - Claude Notes

## Overview
Single-project `jot` repository with Python reference interpreter and C99 production interpreter. v4.0.0.

## Build/Test
```bash
# Python interpreter
PYTHONPATH=. python3 python/main.py examples/hello.jot
PYTHONPATH=. python3 -m pytest python/tests/

# C interpreter
make -C src/
src/jot examples/hello.jot
make -C src/ test
```

## Structure
- `python/` - Python reference interpreter (lexer, parser, interpreter, tests)
- `src/` - C99 production interpreter (lexer, parser, interpreter, value, table, builtins)
- `examples/` - Example .jot programs
- `docs/` - Language documentation

No nested legacy project copies remain in the repo root.

## Status
Stable. Used as training data for Arthur LLM.

## Checkpoints

- `jot` has crossed the line from experiment to real language project
- Current shape: Python reference interpreter + C99 runtime + tests + examples + docs
- Main unfinished engineering work is semantic rigor, not feature novelty

### Current Roadblocks

- No formal spec
- Runtime parity can drift
- Closure semantics still need to be treated as first-class language behavior
- Tooling and benchmarks are not mature yet

### Next Priorities

- Lock down closures and scope behavior
- Add stronger Python/C parity coverage
- Improve diagnostics and error consistency
- Add benchmark discipline before major runtime optimization claims
- Add formatter/editor tooling once semantics are stable

## Quick Commands
- `./scripts/simplify.sh`
