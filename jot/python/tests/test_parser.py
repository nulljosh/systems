"""pytest tests for the jot parser (AST construction)."""
import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

import pytest
from python.lexer import Lexer, TokenType
from python.parser import (
    Parser, Program, Assignment, Number, String, Boolean, Variable,
    BinaryOp, UnaryOp, Print, IfStatement, WhileLoop, ForLoop,
    FunctionDef, FunctionCall, Return, Break, Continue, Array,
    ArrayIndex, ObjectLiteral, TryCatch, Throw,
)


def parse(source):
    tokens = Lexer(source).tokenize()
    return Parser(tokens).parse()


def first(source):
    return parse(source).statements[0]


# --- Literals ---

def test_number_literal():
    node = first("5;")
    assert isinstance(node, Number)
    assert node.value == 5


def test_float_literal():
    node = first("3.14;")
    assert isinstance(node, Number)
    assert node.value == pytest.approx(3.14)


def test_string_literal():
    node = first('"hi";')
    assert isinstance(node, String)
    assert node.value == "hi"


def test_true_literal():
    node = first("true;")
    assert isinstance(node, Boolean)
    assert node.value is True


def test_false_literal():
    node = first("false;")
    assert isinstance(node, Boolean)
    assert node.value is False


# --- Assignment ---

def test_let_assignment():
    node = first("let x = 42;")
    assert isinstance(node, Assignment)
    assert node.name == "x"
    assert isinstance(node.value, Number)
    assert node.value.value == 42


def test_reassignment():
    node = first("x = 10;")
    assert isinstance(node, Assignment)
    assert node.name == "x"


# --- Binary operations ---

def test_addition():
    node = first("1 + 2;")
    assert isinstance(node, BinaryOp)
    assert node.operator == TokenType.PLUS
    assert isinstance(node.left, Number)
    assert isinstance(node.right, Number)


def test_precedence_mul_over_add():
    node = first("1 + 2 * 3;")
    # Should be 1 + (2 * 3), so top-level is PLUS
    assert isinstance(node, BinaryOp)
    assert node.operator == TokenType.PLUS
    assert isinstance(node.right, BinaryOp)
    assert node.right.operator == TokenType.MULTIPLY


def test_parentheses_override_precedence():
    node = first("(1 + 2) * 3;")
    assert isinstance(node, BinaryOp)
    assert node.operator == TokenType.MULTIPLY
    inner = node.left
    assert isinstance(inner, BinaryOp)
    assert inner.operator == TokenType.PLUS


def test_comparison_operators():
    for op_src, expected_type in [
        ("a > b", TokenType.GT), ("a < b", TokenType.LT),
        ("a == b", TokenType.EQ), ("a != b", TokenType.NEQ),
        ("a >= b", TokenType.GTE), ("a <= b", TokenType.LTE),
    ]:
        node = first(op_src + ";")
        assert isinstance(node, BinaryOp)
        assert node.operator == expected_type


def test_logical_and_or():
    node = first("a and b;")
    assert node.operator == TokenType.AND
    node2 = first("a or b;")
    assert node2.operator == TokenType.OR


# --- Unary ---

def test_unary_not():
    node = first("not true;")
    assert isinstance(node, UnaryOp)
    assert node.operator == TokenType.NOT


# --- Print ---

def test_print_statement():
    node = first('print "hello";')
    assert isinstance(node, Print)
    assert isinstance(node.expression, String)


# --- If statement ---

def test_if_no_else():
    node = first("if x > 0 { print x; }")
    assert isinstance(node, IfStatement)
    assert node.else_block is None
    assert len(node.then_block) == 1


def test_if_else():
    node = first("if x { print 1; } else { print 2; }")
    assert isinstance(node, IfStatement)
    assert node.else_block is not None
    assert len(node.else_block) == 1


# --- While ---

def test_while_loop():
    node = first("while x < 10 { x = x + 1; }")
    assert isinstance(node, WhileLoop)
    assert len(node.body) == 1


# --- For ---

def test_for_loop():
    node = first("for i in arr { print i; }")
    assert isinstance(node, ForLoop)
    assert node.var == "i"
    assert len(node.body) == 1


# --- Functions ---

def test_function_def_no_params():
    node = first("fn greet() { print \"hi\"; }")
    assert isinstance(node, FunctionDef)
    assert node.name == "greet"
    assert node.params == []


def test_function_def_with_params():
    node = first("fn add(a, b) { return a + b; }")
    assert isinstance(node, FunctionDef)
    assert node.params == [("a", None), ("b", None)]


def test_function_call_no_args():
    node = first("foo();")
    assert isinstance(node, FunctionCall)
    assert node.name == "foo"
    assert node.args == []


def test_function_call_with_args():
    node = first("add(1, 2);")
    assert isinstance(node, FunctionCall)
    assert len(node.args) == 2


# --- Return / Break / Continue ---

def test_return_value():
    stmts = parse("fn f() { return 42; }").statements
    fn = stmts[0]
    ret = fn.body[0]
    assert isinstance(ret, Return)
    assert isinstance(ret.value, Number)


def test_break():
    node = first("while true { break; }").body[0]
    assert isinstance(node, Break)


def test_continue():
    node = first("while true { continue; }").body[0]
    assert isinstance(node, Continue)


# --- Arrays ---

def test_array_literal():
    node = first("[1, 2, 3];")
    assert isinstance(node, Array)
    assert len(node.elements) == 3


def test_array_index():
    node = first("arr[0];")
    assert isinstance(node, ArrayIndex)


# --- Objects ---

def test_object_literal():
    node = first('let o = {x: 1, y: 2};')
    assert isinstance(node, Assignment)
    assert isinstance(node.value, ObjectLiteral)
    assert len(node.value.pairs) == 2


# --- Try/Catch ---

def test_try_catch():
    src = 'try { print 1; } catch (e) { print e; }'
    node = first(src)
    assert isinstance(node, TryCatch)
    assert node.catch_var == "e"


# --- Error cases ---

def test_optional_semicolons():
    """Semicolons are optional -- this should parse fine"""
    program = parse("let x = 5 let y = 6")
    assert len(program.statements) == 2


def test_unclosed_paren_raises():
    with pytest.raises(SyntaxError):
        parse("let x = (1 + 2;")


def test_empty_program():
    prog = parse("")
    assert isinstance(prog, Program)
    assert prog.statements == []
