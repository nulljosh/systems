# nullC Benchmarks

## Complexity Tiers

### Tier 1: Basic Programs (Current)
- **Level 0**: Hello World (return 0)
- **Level 1**: Arithmetic (+-*/)
- **Level 2**: Variables and expressions
- **Status**: Parser working 

### Tier 2: Control Flow
- **Level 3**: If/else conditionals
- **Level 4**: While and for loops
- **Status**: TODO

### Tier 3: Functions & Recursion
- **Level 5**: Function calls with parameters
- **Level 6**: Recursive functions (factorial, fibonacci)
- **Status**: TODO

### Tier 4: Advanced Types
- **Level 7**: Pointers and arrays
- **Level 8**: Structs and nested structs
- **Level 9**: String manipulation
- **Status**: TODO

### Tier 5: Meta-Programming
- **Level 10**: Compiler within a compiler
  - Mini lexer/tokenizer
  - Expression evaluator
  - Self-hosting capability test
- **Status**: TODO

## Real-World Programs

Once nullC can compile all 10 levels, we test against real programs:

### Milestone A: Simple CLI Tools
- `wc` - word count
- `cat` - file concatenation
- `echo` - print text

### Milestone B: Data Structures
- Linked list implementation
- Binary tree
- Hash table

### Milestone C: Network Programs
- **TCP Echo Server** (accept connections, echo back)
- **HTTP Parser** (parse simple GET requests)
- **Mini Web Server** (serve static files)

### Milestone D: Self-Hosting
- **nullC compiles itself**
- Bootstrap: gcc compiles nullC v1 → nullC v1 compiles nullC v2
- If v1 == v2, compiler is self-hosting

## Performance Targets

nullC is educational, not optimized. But we track:

- **Compile time**: < 1s for <500 LOC
- **Binary size**: Similar to gcc -O0
- **Correctness**: 100% pass rate on all levels

## Stretch Goals

Beyond the 10 levels:

- Preprocessor (#include, #define)
- Standard library subset (stdio.h basics)
- Inline assembly
- Optimization passes
- LLVM IR output (instead of x86-64 asm)

## Comparison to Other Compilers

| Compiler | Lines of Code | Self-Hosting | Target |
|----------|---------------|--------------|--------|
| TCC      | ~30k          | Yes          | x86    |
| chibicc  | ~10k          | Yes          | x86-64 |
| 8cc      | ~7k           | Yes          | x86    |
| **nullC**| ~2k (goal)    | **Goal**     | x86-64 |

nullC aims to be the smallest self-hosting C compiler for educational purposes.
