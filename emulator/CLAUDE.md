# emulator

CHIP-8 emulator in C.

## Files

- `src/chip8.c` -- CPU: fetch/decode/execute, timers
- `src/chip8.h` -- State: 4K RAM, 16 regs, stack, display
- `src/display.c` -- 64x32 monochrome display (SDL or terminal)
- `src/input.c` -- 16-key hex keypad mapping
- `src/main.c` -- ROM loader, main loop

## Build

```bash
make && make test
```
