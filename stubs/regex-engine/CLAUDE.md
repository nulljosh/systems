# regex-engine

Regex compiler + NFA/DFA matching engine in C.

## Architecture

- `src/parser.c` -- Pattern parser, produces AST
- `src/nfa.c` -- Thompson NFA construction from AST
- `src/dfa.c` -- Subset construction NFA->DFA
- `src/match.c` -- Matching engine (NFA or DFA backend)
- `src/main.c` -- CLI entry point

## Build

```bash
make        # build
make test   # run tests
make clean  # cleanup
```

## Conventions

- C99, no external dependencies
- All memory manually managed (no GC)
- Tests in tests/ directory
