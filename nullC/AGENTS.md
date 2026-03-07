# AGENTS.md - AI Development Guide

## Token Efficiency

- Don't re-read files you just wrote/edited unless a write failed or something external changed.
- Don't re-run deterministic commands just to "double-check."
- Don't paste large code/file dumps unless asked; summarize diffs + outcomes.
- Batch related reads/commands; avoid redundant tool calls.
- Keep updates tight: what changed, why, what's left.

## Project Overview
nullC is a C compiler written in C. It's an educational project to understand compiler construction from first principles.

## For AI Agents Working on This Project

### Current Status (2026-02-09)
- Phase 1 (Lexer): COMPLETE
- Phase 2 (Parser): NOT STARTED
- Phase 3-5: NOT STARTED

### Key Files
- `src/lexer.{h,c}` - Tokenization (DONE)
- `src/parser.{h,c}` - AST construction (TODO)
- `src/ast.h` - AST node definitions (TODO)
- `src/codegen.c` - x86-64 assembly generation (TODO)
- `examples/level*.c` - 10 test programs of increasing complexity

### Development Workflow

1. **Read ROADMAP.md** first - understand the 10 complexity levels
2. **Read CLAUDE.md** - see milestones and commit format
3. **Test lexer** on any new examples:
   ```bash
   make
   ./nullc examples/level0_hello.c
   ```
4. **Commit format**: `type: description` (feat/fix/refactor/test/docs)
5. **Push often** - small commits are better

### Next Task: Parser (Milestone 1)

**Goal**: Build AST from tokens

**Files to create**:
- `src/ast.h` - AST node types (Program, Function, Statement, Expression)
- `src/parser.h` - Parser interface
- `src/parser.c` - Recursive descent parser

**Start simple**:
1. Parse `level0_hello.c` (empty main returning int)
2. Then `level1_arithmetic.c` (expressions)
3. Incrementally add complexity

**Test approach**:
```c
// In src/main.c, add after lexer output:
Parser parser;
parser_init(&parser, tokens);
ASTNode *ast = parse_program(&parser);
ast_print(ast);  // Debug output
ast_free(ast);
```

### Design Principles
- **Simplicity over cleverness** - clear code beats terse code
- **Incremental progress** - make it work, then make it better
- **Test each level** - verify parser handles all 10 complexity levels
- **Document surprises** - comment non-obvious design decisions

### Common Pitfalls
- Don't implement optimization early - get it working first
- Parser should build AST, not evaluate or generate code
- Operator precedence matters: handle it in the parser
- Left recursion in grammar will infinite loop - use iteration

### Resources
- ROADMAP.md - full project plan
- CLAUDE.md - milestones and conventions
- examples/ - 10 test programs to support

### Questions to Ask
- Does this parser handle all tokens from the lexer?
- Can it parse all 10 complexity levels?
- Is the AST structure clear and extensible?
- Are error messages helpful for debugging?

### Success Criteria for Next Milestone
- [ ] AST nodes defined for expressions, statements, declarations
- [ ] Parser can handle level0-2 (hello, arithmetic, variables)
- [ ] AST pretty-printer shows tree structure
- [ ] No segfaults or memory leaks (valgrind clean)
- [ ] Committed with message: "feat: implement parser foundation"

---

**Remember**: The goal isn't just a working compiler, it's understanding how compilers work. Code should be readable and educational.
