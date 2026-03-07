#!/usr/bin/env python3
"""
Convert x86-64 assembly generation to ARM64 AArch64
"""

import re
import sys

def convert_to_arm64(content):
    """Convert x86-64 assembly codegen to ARM64"""

    # Register mappings
    reg_map = {
        r'%%rax': 'x0',    # Return value / temp
        r'%%rdi': 'x9',    # Temp (was first arg, but args now in x0-x7)
        r'%%rsi': 'x10',   # Temp
        r'%%rdx': 'x11',   # Temp
        r'%%rcx': 'x12',   # Temp
        r'%%r8': 'x13',    # Temp
        r'%%r9': 'x14',    # Temp
        r'%%rbp': 'x29',   # Frame pointer
        r'%%rsp': 'sp',    # Stack pointer
    }

    # Apply register mappings
    for old_reg, new_reg in reg_map.items():
        content = re.sub(old_reg, new_reg, content)

    # Instruction conversions - specific patterns first

    # Function prologue pattern
    content = re.sub(
        r'emit\(cg, "    pushq x29"\);[\s\n]+emit\(cg, "    movq sp, x29"\);',
        'emit(cg, "    stp x29, x30, [sp, #-16]!");\n    emit(cg, "    mov x29, sp");',
        content
    )

    # Function epilogue pattern
    content = re.sub(
        r'emit\(cg, "    movq x29, sp"\);[\s\n]+emit\(cg, "    popq x29"\);[\s\n]+emit\(cg, "    retq"\);',
        'emit(cg, "    mov sp, x29");\n    emit(cg, "    ldp x29, x30, [sp], #16");\n    emit(cg, "    ret");',
        content
    )

    # Individual instruction mappings
    inst_map = {
        r'\bpushq\b': 'str',   # Note: will need manual fixup for actual push operations
        r'\bpopq\b': 'ldr',     # Note: will need manual fixup for actual pop operations
        r'\bmovq\b': 'mov',
        r'\baddq\b': 'add',
        r'\bsubq\b': 'sub',
        r'\bimulq\b': 'mul',
        r'\bcmpq\b': 'cmp',
        r'\bretq\b': 'ret',
        r'\bcallq\b': 'bl',
        r'\bjmp\b': 'b',
        # Conditional jumps
        r'\bje\b': 'b.eq',
        r'\bjne\b': 'b.ne',
        r'\bjg\b': 'b.gt',
        r'\bjge\b': 'b.ge',
        r'\bjl\b': 'b.lt',
        r'\bjle\b': 'b.le',
        r'\bja\b': 'b.hi',   # unsigned >
        r'\bjae\b': 'b.hs',  # unsigned >=
        r'\bjb\b': 'b.lo',   # unsigned <
        r'\bjbe\b': 'b.ls',  # unsigned <=
    }

    for old_inst, new_inst in inst_map.items():
        content = re.sub(old_inst, new_inst, content)

    # Fix stack operations - ARM64 uses different syntax
    # pushq %rax -> str x0, [sp, #-16]!
    # popq %rdi -> ldr x9, [sp], #16
    # These need manual attention since the pattern varies

    # Fix alignment - ARM64 uses #16 not $16 for stack
    content = re.sub(r'subq \$(\d+), sp', r'sub sp, sp, #\1', content)

    return content

def main():
    input_file = "src/codegen.c"
    output_file = "src/codegen.c.arm64"

    with open(input_file, 'r') as f:
        content = f.read()

    converted = convert_to_arm64(content)

    with open(output_file, 'w') as f:
        f.write(converted)

    print(f"Converted {input_file} -> {output_file}")
    print("Note: Manual review required for push/pop operations and addressing modes")

if __name__ == "__main__":
    main()
