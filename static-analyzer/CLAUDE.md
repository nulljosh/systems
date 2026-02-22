# Static Analyzer - Claude Notes

## Overview
Python source code static analyzer. AST-based. Detects unused variables/imports, unreachable code, basic type inference. Done/stable.

## Run
```bash
cd ~/Documents/Code/static-analyzer
python analyzer.py target.py
python analyzer.py target.py --format json
```

## Features
- Python `ast` module parsing
- Unused variable/import detection
- Unreachable code detection
- Custom rule engine
- Text + JSON report output

## Status
Done/stable. ~780 LOC Python.
