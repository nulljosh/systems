# nullC Development Notes

## Current Status (2026-03-06)
- Lexer: Complete
- Parser: Complete
- Codegen: Complete (10/10 levels)
- Peephole Optimizer: Complete
- Target: ARM64 (Darwin/macOS)

## Test Results
- Level 0-10: All passing

## Development Commands

```bash
make            # Build compiler
make test       # Run all level tests
make clean      # Remove generated files
```

## Commit Format
- feat: New feature
- fix: Bug fix
- refactor: Code restructure
- docs: Documentation

## Key Fixes
1. Fixed nested struct size calculation (recursive field sizing)
2. Fixed struct field offset calculation (sum field sizes, not assume 8 bytes)
3. Fixed struct parameter passing (preserve x0 register using x9)

## Code Style
- C99 standard
- Clear over clever
- Comment non-obvious logic
- Test each level incrementally
