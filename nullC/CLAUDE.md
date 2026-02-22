# nullC Development Notes

## Current Status (2026-02-13)
- Lexer: Complete
- Parser: Complete
- Codegen: Complete (10/10 levels)
- Target: ARM64 (Darwin/macOS)

## Test Results
- Level 0-10: All passing (structs fixed)

## Development Commands

Build and test:
```bash
make
./nullc examples/level1_arithmetic.c
./level1_arithmetic && echo "Exit: $?"
```

Test all levels:
```bash
for f in examples/level*.c; do
  ./nullc "$f" && ./${f%.c} && echo "$f: $?"
done
```

## Commit Format
- feat: New feature
- fix: Bug fix
- refactor: Code restructure
- docs: Documentation

## Recent Fixes (2026-02-13)
1. Fixed nested struct size calculation (recursive field sizing)
2. Fixed struct field offset calculation (sum field sizes, not assume 8 bytes)
3. Fixed struct parameter passing (preserve x0 register using x9)

## Next Steps
1. Optimize codegen (register allocation)
2. Self-hosting test (compile nullC with nullC)
3. Add more test cases

## Code Style
- C99 standard
- Clear over clever
- Comment non-obvious logic
- Test each level incrementally
