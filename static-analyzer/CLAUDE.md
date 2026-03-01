# Static Analyzer - Claude Notes

## Overview
Python source code static analyzer. AST-based. Detects unused variables/imports, unreachable code, basic type inference. Done/stable.

## Run
```bash
cd ~/Documents/Code/systems/static-analyzer
python src/analyzer.py target.py
python src/analyzer.py target.py --format json
python -m pytest tests/
```

## Features
- Python `ast` module parsing
- Pluggable rule engine (`Rule` ABC, auto-registration)
- 5 rules: W001 unused imports, W002 unused vars, W003 unreachable code, W004 variable shadowing, W005 unused args
- Text + JSON report output (`--format json`)
- 11 tests passing

## Status
Done/stable.
