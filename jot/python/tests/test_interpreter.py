#!/usr/bin/env python3
"""
Comprehensive test suite for JoshLang interpreter
Tests arithmetic, variables, expressions, and error handling
"""

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from python.interpreter import Interpreter


def test_arithmetic():
    """Test basic arithmetic operations"""
    print("Testing arithmetic...")

    interp = Interpreter()
    assert interp.run('let x = 5 + 3; x;') == 8, "Addition failed"
    assert interp.run('let x = 10 - 4; x;') == 6, "Subtraction failed"
    assert interp.run('let x = 6 * 7; x;') == 42, "Multiplication failed"
    assert interp.run('let x = 20 / 4; x;') == 5.0, "Division failed"
    assert interp.run('let x = 5 + 3 * 2; x;') == 11, "Order of operations failed"
    assert interp.run('let x = (5 + 3) * 2; x;') == 16, "Parentheses failed"

    print("  ✓ All arithmetic tests passed")


def test_variables():
    """Test variable assignment and retrieval"""
    print("Testing variables...")

    interp = Interpreter()
    assert interp.run('let x = 42; x;') == 42, "Simple assignment failed"

    interp.run('let x = 10;')
    assert interp.run('let x = 20; x;') == 20, "Reassignment failed"

    interp = Interpreter()
    interp.run('let a = 10;')
    interp.run('let b = 20;')
    assert interp.run('let sum = a + b; sum;') == 30, "Variable in expression failed"

    # Test undefined variable error
    try:
        interp = Interpreter()
        interp.run('print undefined_var;')
        assert False, "Should have raised error for undefined variable"
    except RuntimeError as e:
        assert "Undefined variable" in str(e)

    print("  ✓ All variable tests passed")


def test_expressions():
    """Test complex expressions"""
    print("Testing expressions...")

    interp = Interpreter()
    assert interp.run('let x = ((5 + 3) * 2) - 4; x;') == 12, "Nested expression failed"

    interp = Interpreter()
    interp.run('let x = 5;')
    interp.run('let y = 10;')
    interp.run('let z = 15;')
    assert interp.run('let sum = x + y + z; sum;') == 30, "Multiple variables failed"

    print("  ✓ All expression tests passed")


def test_strings():
    """Test string handling"""
    print("Testing strings...")

    interp = Interpreter()
    interp.run('let name = "Joshua";')
    assert interp.variables['name'] == "Joshua", "String variable failed"

    # Just check it doesn't error
    interp.run('print "Hello, World!";')

    print("  ✓ All string tests passed")


def test_booleans():
    """Test boolean literals"""
    print("Testing booleans...")

    interp = Interpreter()
    assert interp.run('let x = true; x;') == True, "true literal failed"
    assert interp.run('let y = false; y;') == False, "false literal failed"

    print("  ✓ All boolean tests passed")


def test_comparisons():
    """Test comparison operators"""
    print("Testing comparisons...")

    interp = Interpreter()
    assert interp.run('let x = 10 > 5; x;') == True, "Greater than failed"
    assert interp.run('let y = 3 > 8; y;') == False, "Greater than failed"
    assert interp.run('let x = 5 < 10; x;') == True, "Less than failed"
    assert interp.run('let y = 8 < 3; y;') == False, "Less than failed"
    assert interp.run('let x = 5 == 5; x;') == True, "Equal failed"
    assert interp.run('let y = 5 == 3; y;') == False, "Equal failed"

    # New operators
    assert interp.run('let x = 5 != 3; x;') == True, "Not equal failed"
    assert interp.run('let y = 5 != 5; y;') == False, "Not equal failed"
    assert interp.run('let x = 10 >= 10; x;') == True, "Greater or equal failed"
    assert interp.run('let y = 10 >= 5; y;') == True, "Greater or equal failed"
    assert interp.run('let z = 3 >= 5; z;') == False, "Greater or equal failed"
    assert interp.run('let x = 5 <= 5; x;') == True, "Less or equal failed"
    assert interp.run('let y = 3 <= 5; y;') == True, "Less or equal failed"
    assert interp.run('let z = 7 <= 5; z;') == False, "Less or equal failed"

    print("  ✓ All comparison tests passed")


def test_logical_operators():
    """Test logical operators"""
    print("Testing logical operators...")

    interp = Interpreter()
    assert interp.run('let x = true and true; x;') == True, "and failed"
    assert interp.run('let y = true and false; y;') == False, "and failed"
    assert interp.run('let z = false and false; z;') == False, "and failed"

    assert interp.run('let x = true or false; x;') == True, "or failed"
    assert interp.run('let y = false or false; y;') == False, "or failed"
    assert interp.run('let z = true or true; z;') == True, "or failed"

    assert interp.run('let x = not true; x;') == False, "not failed"
    assert interp.run('let y = not false; y;') == True, "not failed"

    # Complex expressions
    assert interp.run('let x = 5 > 3 and 10 < 20; x;') == True, "complex and failed"
    assert interp.run('let y = 5 > 3 or 10 > 20; y;') == True, "complex or failed"
    assert interp.run('let z = not (5 > 10); z;') == True, "complex not failed"

    print("  ✓ All logical operator tests passed")


def test_error_handling():
    """Test error handling"""
    print("Testing error handling...")

    # Division by zero
    try:
        interp = Interpreter()
        interp.run('let x = 10 / 0;')
        assert False, "Should have raised division by zero error"
    except RuntimeError as e:
        assert "Division by zero" in str(e)

    # Syntax error
    try:
        interp = Interpreter()
        interp.run('let x = ;')
        assert False, "Should have raised syntax error"
    except Exception:
        pass  # Expected

    print("  ✓ All error handling tests passed")


def test_multiple_statements():
    """Test programs with multiple statements"""
    print("Testing multiple statements...")

    interp = Interpreter()
    code = '''
        let x = 5;
        let y = 10;
        let sum = x + y;
        sum;
    '''
    assert interp.run(code) == 15, "Sequential statements failed"

    interp = Interpreter()
    code = '''
        let x = 10;
        let x = 20;
        let x = 30;
        x;
    '''
    assert interp.run(code) == 30, "Variable shadowing failed"

    print("  ✓ All multiple statement tests passed")


def test_string_concatenation():
    """Test string concatenation"""
    print("Testing string concatenation...")

    interp = Interpreter()
    assert interp.run('let x = "hello" + " world"; x;') == "hello world", "String concatenation failed"
    assert interp.run('let a = "foo"; let b = "bar"; let c = a + b; c;') == "foobar", "String variable concatenation failed"

    print("  ✓ All string concatenation tests passed")


def test_string_methods():
    """Test string methods"""
    print("Testing string methods...")

    interp = Interpreter()
    assert interp.run('let x = len("hello"); x;') == 5, "String len failed"
    assert interp.run('let x = slice("hello", 0, 3); x;') == "hel", "String slice failed"
    assert interp.run('let x = slice("hello world", 6, 11); x;') == "world", "String slice failed"

    result = interp.run('let x = split("a,b,c", ","); x;')
    assert result == ["a", "b", "c"], "String split failed"

    print("  ✓ All string method tests passed")


def test_break_continue():
    """Test break and continue statements"""
    print("Testing break and continue...")

    # Test break in while loop
    interp = Interpreter()
    code = '''
        let i = 0;
        let sum = 0;
        while i < 10 {
            if i == 5 {
                break;
            }
            sum = sum + i;
            i = i + 1;
        }
        sum;
    '''
    assert interp.run(code) == 10, "Break in while loop failed"  # 0+1+2+3+4 = 10

    # Test continue in while loop
    interp = Interpreter()
    code = '''
        let i = 0;
        let sum = 0;
        while i < 10 {
            i = i + 1;
            if i == 3 or i == 5 {
                continue;
            }
            sum = sum + i;
        }
        sum;
    '''
    assert interp.run(code) == 47, "Continue in while loop failed"  # 1+2+4+6+7+8+9+10 = 47

    # Test break in for loop
    interp = Interpreter()
    code = '''
        let sum = 0;
        for i in range(0, 10) {
            if i == 5 {
                break;
            }
            sum = sum + i;
        }
        sum;
    '''
    assert interp.run(code) == 10, "Break in for loop failed"  # 0+1+2+3+4 = 10

    # Test continue in for loop
    interp = Interpreter()
    code = '''
        let sum = 0;
        for i in range(1, 11) {
            if i == 3 or i == 5 {
                continue;
            }
            sum = sum + i;
        }
        sum;
    '''
    assert interp.run(code) == 47, "Continue in for loop failed"  # 1+2+4+6+7+8+9+10 = 47

    print("  ✓ All break/continue tests passed")


def main():
    """Run all tests"""
    print("=" * 60)
    print("JoshLang Interpreter Test Suite")
    print("=" * 60)
    print()

    try:
        test_arithmetic()
        test_variables()
        test_expressions()
        test_strings()
        test_booleans()
        test_comparisons()
        test_logical_operators()
        test_error_handling()
        test_multiple_statements()
        test_string_concatenation()
        test_string_methods()
        test_break_continue()

        print()
        print("=" * 60)
        print("✓ ALL TESTS PASSED")
        print("=" * 60)
        return 0

    except AssertionError as e:
        print()
        print("=" * 60)
        print(f"✗ TEST FAILED: {e}")
        print("=" * 60)
        return 1
    except Exception as e:
        print()
        print("=" * 60)
        print(f"✗ UNEXPECTED ERROR: {e}")
        print("=" * 60)
        import traceback
        traceback.print_exc()
        return 1


if __name__ == "__main__":
    sys.exit(main())
