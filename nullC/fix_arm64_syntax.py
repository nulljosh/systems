#!/usr/bin/env python3
import re, sys

with open("src/codegen.c") as f:
    c = f.read()

# Fix immediate syntax: $N -> #N
c = re.sub(r'\$(\d+)', r'#\1', c)

# Fix sub sp instruction
c = re.sub(r'sub sp, sp, #(\d+)', r'sub sp, sp, #\1', c)
c = re.sub(r'sub (#\d+), sp', r'sub sp, sp, \1', c)

# Fix mov immediate: mov #N, xN -> mov xN, #N
c = re.sub(r'mov (#\d+), (x\d+)', r'mov \2, \1', c)

# Fix memory ops: N(xN) -> [xN, #N]
c = re.sub(r'(\d+)\((x\d+)\)', r'[\2, #\1]', c)

# Fix leaq -> add/adr
c = re.sub(r'leaq ([^,]+), (x\d+)', r'add \2, x29, \1', c)
c = re.sub(r'add (x\d+), x29, \[x29, #(\d+)\]', r'add \1, x29, #\2', c)
c = re.sub(r'add (x\d+), x29, \.str(\d+)\(%rip\)', r'adr \1, .str\2', c)

# Fix cmp: cmp #0, xN -> cmp xN, #0
c = re.sub(r'cmp (#\d+), (x\d+)', r'cmp \2, \1', c)

# Fix add/sub with regs: add xN, xM -> add xM, xM, xN
c = re.sub(r'^\s*emit\(cg, "    add (x\d+), (x\d+)"\);', r'emit(cg, "    add \2, \2, \1");', c, flags=re.M)

# Fix str/ldr (incomplete - need sp addressing)
c = re.sub(r'str (x\d+)"', r'str \1, [sp, #-16]!"', c)
c = re.sub(r'ldr (x\d+)"', r'ldr \1, [sp], #16"', c)

# Fix mov for memory: mov (xN), xM -> ldr xM, [xN]
c = re.sub(r'mov \[(x\d+)\], (x\d+)', r'ldr \2, [\1]', c)

# Fix division (ARM doesn't have idivq - need sdiv)
c = re.sub(r'cqto', r'// cqto (ARM: use sdiv directly)', c)
c = re.sub(r'idivq (x\d+)', r'sdiv x0, x0, \1', c)

# Fix set instructions (ARM doesn't have - use cset)
c = re.sub(r'sete %al[\s\n]+.*movzbq %al, (x\d+)', r'cset \1, eq', c, flags=re.S)
c = re.sub(r'setne %al[\s\n]+.*movzbq %al, (x\d+)', r'cset \1, ne', c, flags=re.S)
c = re.sub(r'setl %al[\s\n]+.*movzbq %al, (x\d+)', r'cset \1, lt', c, flags=re.S)
c = re.sub(r'setg %al[\s\n]+.*movzbq %al, (x\d+)', r'cset \1, gt', c, flags=re.S)
c = re.sub(r'setle %al[\s\n]+.*movzbq %al, (x\d+)', r'cset \1, le', c, flags=re.S)
c = re.sub(r'setge %al[\s\n]+.*movzbq %al, (x\d+)', r'cset \1, ge', c, flags=re.S)

# Fix negq -> neg
c = re.sub(r'negq (x\d+)', r'neg \1, \1', c)

# Fix remaining %rip references
c = re.sub(r'%rip', r'', c)

with open("src/codegen.c", "w") as f:
    f.write(c)

print("ARM64 syntax fixed")
