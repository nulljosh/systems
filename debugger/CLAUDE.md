# Debugger - Claude Notes

## Overview
ptrace-based C debugger. 380 LOC. macOS + Linux x86-64.

## Build
```bash
cd ~/Documents/Code/debugger
make clean && make
./debugger ./test_prog
```

## Commands
```
run           start program
break <addr>  set INT3 breakpoint (hex address)
continue / c  run to breakpoint
step / s      single instruction
print $rax    read register
print *0x1000 read memory
quit / q      exit
```

## Getting Addresses
```bash
nm ./myprog | grep main   # find function address
gcc -g -O0 -o myprog myprog.c  # compile with debug symbols
```

## Files
- `debugger.h` (45 LOC), `debugger.cpp` (320 LOC), `main.cpp` (15 LOC)

## Status
Done/stable. No symbol resolution, disassembly, or multi-thread support.
