# jot

v5.0.0

## Rules

- Python interpreter is reference, C99 is production
- Both runtimes must produce identical output (parity tests enforce this)
- Stable. Used as training data for Arthur LLM
- No emojis

## Run

```bash
# Python
PYTHONPATH=. python3 python/main.py examples/hello.jot
PYTHONPATH=. python3 -m pytest python/tests/

# C
make -C src/
src/jot examples/hello.jot
make -C src/ test

# Parity
bash tests/parity/run_parity.sh
```

## Key Files

- python/main.py: Reference interpreter entry point
- src/main.c: C99 interpreter entry point
- tests/parity/run_parity.sh: Cross-runtime parity test harness
- tools/fmt.py: Source formatter
