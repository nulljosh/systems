#!/usr/bin/env python3
import re

with open("src/codegen.c") as f:
    lines = f.readlines()

out = []
for line in lines:
    # Fix memory loads: mov xN, OFFSET(x29) -> ldr xN, [x29, #OFFSET]
    line = re.sub(r'mov (x\d+), (-?\d+)\(x29\)', r'ldr \1, [x29, #\2]', line)

    # Fix memory stores: mov xN, OFFSET(x29) -> str xN, [x29, #OFFSET]
    line = re.sub(r'mov (-?\d+)\(x29\), (x\d+)', r'str \2, [x29, #\1]', line)

    # Fix memory deref: mov (xN), xM -> ldr xM, [xN]
    line = re.sub(r'mov \((x\d+)\), (x\d+)', r'ldr \2, [\1]', line)

    # Fix memory store: mov xN, (xM) -> str xN, [xM]
    line = re.sub(r'mov (x\d+), \((x\d+)\)', r'str \1, [\2]', line)

    # Fix sub with 2 operands: sub xN, xM -> sub xN, xM, xN
    line = re.sub(r'sub (x\d+), (x\d+)"', r'sub \1, \2, \1"', line)

    # Fix mul with 2 operands: mul xN, xM -> mul xN, xN, xM
    line = re.sub(r'mul (x\d+), (x\d+)"', r'mul \1, \1, \2"', line)

    # Fix add xN, xM -> add xM, xM, xN
    line = re.sub(r'add (x\d+), (x\d+)"', r'add \2, \2, \1"', line)

    #Fix neg: neg xN -> neg xN, xN
    line = re.sub(r'neg (x\d+)"', r'neg \1, \1"', line)

    out.append(line)

with open("src/codegen.c", "w") as f:
    f.writelines(out)

print("Fixed ARM64 syntax")
