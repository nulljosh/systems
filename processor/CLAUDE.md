# processor

CPU simulator in C.

## Files

- `src/cpu.c` -- Fetch-decode-execute loop
- `src/cpu.h` -- CPU state, registers, opcodes
- `src/alu.c` -- Arithmetic logic unit
- `src/memory.c` -- Memory subsystem
- `src/main.c` -- CLI, loads binary

## Build

```bash
make && make test
```
