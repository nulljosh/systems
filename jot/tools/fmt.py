#!/usr/bin/env python3
"""jot source code formatter.

Token-based formatter that normalizes whitespace and indentation.

Usage:
    python3 tools/fmt.py file.jot           # print formatted to stdout
    python3 tools/fmt.py --check file.jot   # exit 1 if not formatted
    python3 tools/fmt.py --write file.jot   # overwrite in place
"""
import sys
import os
import argparse

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from python.lexer import Lexer, TokenType


INDENT = "    "  # 4 spaces

# Tokens that increase indent after them
OPEN_BRACE = TokenType.LBRACE
CLOSE_BRACE = TokenType.RBRACE

# Tokens that should have space around them
BINARY_OPS = {
    TokenType.PLUS, TokenType.MINUS, TokenType.MULTIPLY, TokenType.DIVIDE,
    TokenType.MODULO, TokenType.ASSIGN, TokenType.EQ, TokenType.NEQ,
    TokenType.GT, TokenType.LT, TokenType.GTE, TokenType.LTE,
    TokenType.PLUS_ASSIGN, TokenType.MINUS_ASSIGN,
    TokenType.MULTIPLY_ASSIGN, TokenType.DIVIDE_ASSIGN,
    TokenType.AND, TokenType.OR, TokenType.QUESTION, TokenType.COLON,
}

# Keywords that expect a block
BLOCK_KEYWORDS = {
    TokenType.IF, TokenType.ELSE, TokenType.WHILE, TokenType.FOR,
    TokenType.FN, TokenType.CLASS, TokenType.TRY, TokenType.CATCH,
}

# Top-level declaration keywords (blank line before)
DECL_KEYWORDS = {TokenType.FN, TokenType.CLASS}


def format_source(source):
    """Format jot source code and return formatted string."""
    # Preserve comments by extracting them first
    lines = source.split("\n")
    comment_map = {}  # line_number -> comment text
    clean_lines = []
    for i, line in enumerate(lines):
        stripped = line.lstrip()
        if stripped.startswith("//") or stripped.startswith("#"):
            comment_map[i] = stripped
            clean_lines.append("")
        else:
            # Check for inline comments
            in_string = False
            comment_start = -1
            for j, ch in enumerate(line):
                if ch == '"' and (j == 0 or line[j-1] != '\\'):
                    in_string = not in_string
                if not in_string and ch == '/' and j + 1 < len(line) and line[j+1] == '/':
                    comment_start = j
                    break
                if not in_string and ch == '#':
                    comment_start = j
                    break
            if comment_start >= 0:
                comment_map[i] = line[comment_start:].strip()
                clean_lines.append(line[:comment_start])
            else:
                clean_lines.append(line)

    clean_source = "\n".join(clean_lines)

    try:
        lexer = Lexer(clean_source)
        tokens = lexer.tokenize()
    except SyntaxError:
        return source  # Can't parse, return as-is

    output = []
    indent = 0
    line_tokens = []
    prev_was_decl = False
    i = 0

    while i < len(tokens):
        tok = tokens[i]

        if tok.type == TokenType.EOF:
            break

        # Handle closing brace -- dedent first
        if tok.type == CLOSE_BRACE:
            if line_tokens:
                output.append(INDENT * indent + _join_tokens(line_tokens))
                line_tokens = []
            indent = max(0, indent - 1)
            output.append(INDENT * indent + "}")
            prev_was_decl = False
            i += 1
            continue

        # Handle open brace -- append to current line, then newline + indent
        if tok.type == OPEN_BRACE:
            line_tokens.append("{")
            output.append(INDENT * indent + _join_tokens(line_tokens))
            line_tokens = []
            indent += 1
            i += 1
            continue

        # Handle semicolons -- skip them (jot doesn't need them)
        if tok.type == TokenType.SEMICOLON:
            i += 1
            continue

        # Check if we need a blank line before fn/class at top level
        if tok.type in DECL_KEYWORDS and indent == 0 and output and output[-1].strip():
            output.append("")

        line_tokens.append(_token_str(tok))

        # Check if this token ends a statement
        next_tok = tokens[i + 1] if i + 1 < len(tokens) else None
        if _is_statement_end(tok, next_tok, indent):
            output.append(INDENT * indent + _join_tokens(line_tokens))
            line_tokens = []

        i += 1

    if line_tokens:
        output.append(INDENT * indent + _join_tokens(line_tokens))

    # Re-insert comments
    result_lines = output
    final_lines = []
    source_line_idx = 0
    for line in result_lines:
        # Insert any comments that were on lines before this
        while source_line_idx in comment_map:
            # Determine indent for comment
            final_lines.append(INDENT * indent + comment_map[source_line_idx])
            source_line_idx += 1
        final_lines.append(line)
        source_line_idx += 1

    # Append remaining comments
    while source_line_idx in comment_map:
        final_lines.append(comment_map[source_line_idx])
        source_line_idx += 1

    result = "\n".join(final_lines)
    # Normalize: single trailing newline, no trailing whitespace
    result = "\n".join(line.rstrip() for line in result.split("\n"))
    if not result.endswith("\n"):
        result += "\n"
    return result


def _token_str(tok):
    """Convert token to its string representation."""
    if tok.value is not None:
        if tok.type == TokenType.STRING:
            return f'"{tok.value}"'
        if tok.type == TokenType.NUMBER:
            v = tok.value
            if isinstance(v, float) and v == int(v):
                return str(int(v))
            return str(v)
        return str(tok.value)
    return ""


def _join_tokens(tokens):
    """Join tokens with appropriate spacing."""
    if not tokens:
        return ""
    result = tokens[0]
    for i in range(1, len(tokens)):
        prev = tokens[i - 1]
        curr = tokens[i]
        # Always space around binary ops
        if curr in ("+", "-", "*", "/", "%", "=", "==", "!=", ">", "<",
                    ">=", "<=", "+=", "-=", "*=", "/=", "and", "or", "?", ":") or \
           prev in ("+", "-", "*", "/", "%", "=", "==", "!=", ">", "<",
                    ">=", "<=", "+=", "-=", "*=", "/=", "and", "or", "?"):
            result += " " + curr
        # Space after comma
        elif prev == ",":
            result += " " + curr
        # Space after keywords
        elif prev in ("if", "else", "while", "for", "in", "let", "print",
                      "fn", "return", "class", "new", "import", "try",
                      "catch", "throw", "not"):
            result += " " + curr
        # Space before brace
        elif curr == "{":
            result += " " + curr
        # No space after ( or [, or before ) or ] or ,
        elif prev in ("(", "[") or curr in (")", "]", ",", ".", "(", "["):
            result += curr
        # No space after .
        elif prev == ".":
            result += curr
        else:
            result += " " + curr
    return result


def _is_statement_end(tok, next_tok, indent):
    """Heuristic: does this token end a statement?"""
    if not next_tok or next_tok.type == TokenType.EOF:
        return True
    if next_tok.type == CLOSE_BRACE:
        return True
    # After a closing brace, start new line
    if tok.type == CLOSE_BRACE:
        return True

    # Keywords that start statements
    if next_tok.type in (TokenType.LET, TokenType.PRINT, TokenType.IF,
                         TokenType.WHILE, TokenType.FOR, TokenType.FN,
                         TokenType.CLASS, TokenType.RETURN, TokenType.BREAK,
                         TokenType.CONTINUE, TokenType.IMPORT, TokenType.TRY,
                         TokenType.THROW):
        return True
    return False


def main():
    parser = argparse.ArgumentParser(description="jot source code formatter")
    parser.add_argument("files", nargs="+", help="Files to format")
    parser.add_argument("--check", action="store_true",
                        help="Check if files are formatted (exit 1 if not)")
    parser.add_argument("--write", action="store_true",
                        help="Format files in place")
    args = parser.parse_args()

    exit_code = 0
    for path in args.files:
        if os.path.isdir(path):
            for root, _, files in os.walk(path):
                for f in files:
                    if f.endswith(".jot"):
                        exit_code |= _process_file(
                            os.path.join(root, f), args.check, args.write)
        else:
            exit_code |= _process_file(path, args.check, args.write)
    sys.exit(exit_code)


def _process_file(path, check, write):
    with open(path, "r") as f:
        source = f.read()

    formatted = format_source(source)

    if check:
        if source != formatted:
            print(f"UNFORMATTED: {path}")
            return 1
        return 0
    elif write:
        if source != formatted:
            with open(path, "w") as f:
                f.write(formatted)
            print(f"formatted: {path}")
        return 0
    else:
        sys.stdout.write(formatted)
        return 0


if __name__ == "__main__":
    main()
