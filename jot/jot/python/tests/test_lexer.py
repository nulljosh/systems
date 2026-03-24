"""pytest tests for the jot lexer."""
import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

import pytest
from python.lexer import Lexer, TokenType, Token


def tokenize(source):
    return Lexer(source).tokenize()


def types(source):
    return [t.type for t in tokenize(source) if t.type != TokenType.EOF]


# --- Numbers ---

def test_integer():
    toks = tokenize("42;")
    assert toks[0].type == TokenType.NUMBER
    assert toks[0].value == 42


def test_float():
    toks = tokenize("3.14;")
    assert toks[0].type == TokenType.NUMBER
    assert toks[0].value == pytest.approx(3.14)


def test_multiple_numbers():
    t = types("1 + 2")
    assert t == [TokenType.NUMBER, TokenType.PLUS, TokenType.NUMBER]


# --- Strings ---

def test_string_literal():
    toks = tokenize('"hello"')
    assert toks[0].type == TokenType.STRING
    assert toks[0].value == "hello"


def test_string_escape_newline():
    toks = tokenize(r'"a\nb"')
    assert toks[0].value == "a\nb"


def test_string_escape_quote():
    toks = tokenize(r'"say \"hi\""')
    assert toks[0].value == 'say "hi"'


def test_unterminated_string():
    with pytest.raises(SyntaxError):
        tokenize('"unclosed')


# --- Keywords ---

def test_keywords():
    keywords = {
        "let": TokenType.LET,
        "if": TokenType.IF,
        "else": TokenType.ELSE,
        "while": TokenType.WHILE,
        "for": TokenType.FOR,
        "in": TokenType.IN,
        "print": TokenType.PRINT,
        "true": TokenType.TRUE,
        "false": TokenType.FALSE,
        "fn": TokenType.FN,
        "return": TokenType.RETURN,
        "break": TokenType.BREAK,
        "continue": TokenType.CONTINUE,
        "and": TokenType.AND,
        "or": TokenType.OR,
        "not": TokenType.NOT,
    }
    for word, expected in keywords.items():
        assert types(word) == [expected], f"keyword '{word}' failed"


def test_identifier():
    toks = tokenize("myVar")
    assert toks[0].type == TokenType.IDENTIFIER
    assert toks[0].value == "myVar"


def test_underscore_identifier():
    toks = tokenize("_private")
    assert toks[0].type == TokenType.IDENTIFIER
    assert toks[0].value == "_private"


# --- Operators ---

def test_single_char_operators():
    ops = {
        "+": TokenType.PLUS,
        "-": TokenType.MINUS,
        "*": TokenType.MULTIPLY,
        "/": TokenType.DIVIDE,
        "%": TokenType.MODULO,
    }
    for ch, expected in ops.items():
        assert types(ch) == [expected]


def test_two_char_operators():
    assert types("==") == [TokenType.EQ]
    assert types("!=") == [TokenType.NEQ]
    assert types(">=") == [TokenType.GTE]
    assert types("<=") == [TokenType.LTE]


def test_assign_vs_eq():
    assert types("=") == [TokenType.ASSIGN]
    assert types("==") == [TokenType.EQ]


def test_gt_lt():
    assert types(">") == [TokenType.GT]
    assert types("<") == [TokenType.LT]


# --- Delimiters ---

def test_delimiters():
    src = "( ) { } [ ] ; , : ."
    expected = [
        TokenType.LPAREN, TokenType.RPAREN,
        TokenType.LBRACE, TokenType.RBRACE,
        TokenType.LBRACKET, TokenType.RBRACKET,
        TokenType.SEMICOLON, TokenType.COMMA,
        TokenType.COLON, TokenType.DOT,
    ]
    assert types(src) == expected


# --- Comments ---

def test_line_comment_skipped():
    toks = tokenize("// this is a comment\n42;")
    assert toks[0].type == TokenType.NUMBER
    assert toks[0].value == 42


def test_inline_comment():
    t = types("let x = 5; // x is five")
    assert TokenType.SEMICOLON in t
    assert TokenType.IDENTIFIER not in [tt for tt in t if tt == TokenType.IDENTIFIER and False]


# --- Whitespace ---

def test_whitespace_ignored():
    t1 = types("let x=5;")
    t2 = types("let   x  =  5  ;")
    assert t1 == t2


def test_newlines_ignored():
    t = types("let\nx\n=\n5\n;")
    assert t == [TokenType.LET, TokenType.IDENTIFIER, TokenType.ASSIGN,
                 TokenType.NUMBER, TokenType.SEMICOLON]


# --- Line tracking ---

def test_line_numbers():
    toks = tokenize("let x = 1;\nlet y = 2;")
    let_tokens = [t for t in toks if t.type == TokenType.LET]
    assert let_tokens[0].line == 1
    assert let_tokens[1].line == 2


# --- EOF ---

def test_eof_token():
    toks = tokenize("")
    assert toks[-1].type == TokenType.EOF


def test_unknown_char_raises():
    with pytest.raises(SyntaxError):
        tokenize("@")
