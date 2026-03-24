"""Tests for jot language features: string interpolation, ternary, default params,
unary minus, math stdlib, type(), null, string methods, array methods,
compound assignment, error handling improvements."""
import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

import unittest
from python.interpreter import Interpreter


class TestStringInterpolation(unittest.TestCase):
    def setUp(self):
        self.interp = Interpreter()

    def test_basic_interpolation(self):
        self.interp.run('let name = "world"')
        result = self.interp.run('let x = "hello ${name}"; x;')
        self.assertEqual(result, "hello world")

    def test_expression_interpolation(self):
        result = self.interp.run('let x = "2 + 2 = ${2 + 2}"; x;')
        self.assertEqual(result, "2 + 2 = 4")

    def test_multiple_interpolations(self):
        self.interp.run('let a = "foo"')
        self.interp.run('let b = "bar"')
        result = self.interp.run('let x = "${a} and ${b}"; x;')
        self.assertEqual(result, "foo and bar")

    def test_interpolation_with_no_vars(self):
        result = self.interp.run('let x = "plain string"; x;')
        self.assertEqual(result, "plain string")

    def test_interpolation_number_format(self):
        self.interp.run('let n = 42')
        result = self.interp.run('let x = "value is ${n}"; x;')
        self.assertEqual(result, "value is 42")


class TestTernary(unittest.TestCase):
    def setUp(self):
        self.interp = Interpreter()

    def test_ternary_true(self):
        result = self.interp.run('let x = true ? 1 : 2; x;')
        self.assertEqual(result, 1)

    def test_ternary_false(self):
        result = self.interp.run('let x = false ? 1 : 2; x;')
        self.assertEqual(result, 2)

    def test_ternary_with_comparison(self):
        result = self.interp.run('let x = 10 > 5 ? "yes" : "no"; x;')
        self.assertEqual(result, "yes")

    def test_ternary_nested(self):
        result = self.interp.run('let x = true ? false ? 1 : 2 : 3; x;')
        self.assertEqual(result, 2)

    def test_ternary_in_expression(self):
        result = self.interp.run('let x = 1 + (true ? 10 : 20); x;')
        self.assertEqual(result, 11)


class TestDefaultParams(unittest.TestCase):
    def setUp(self):
        self.interp = Interpreter()

    def test_default_param_used(self):
        self.interp.run('fn greet(name, greeting = "hello") { return greeting + " " + name; }')
        result = self.interp.run('greet("Josh");')
        self.assertEqual(result, "hello Josh")

    def test_default_param_overridden(self):
        self.interp.run('fn greet(name, greeting = "hello") { return greeting + " " + name; }')
        result = self.interp.run('greet("Josh", "hey");')
        self.assertEqual(result, "hey Josh")

    def test_multiple_defaults(self):
        self.interp.run('fn add(a, b = 10, c = 20) { return a + b + c; }')
        result = self.interp.run('add(1);')
        self.assertEqual(result, 31)

    def test_partial_defaults(self):
        self.interp.run('fn add(a, b = 10, c = 20) { return a + b + c; }')
        result = self.interp.run('add(1, 2);')
        self.assertEqual(result, 23)

    def test_no_default_missing_arg_errors(self):
        self.interp.run('fn add(a, b) { return a + b; }')
        with self.assertRaises(RuntimeError):
            self.interp.run('add(1);')


class TestUnaryMinus(unittest.TestCase):
    def setUp(self):
        self.interp = Interpreter()

    def test_negative_literal(self):
        result = self.interp.run('let x = -5; x;')
        self.assertEqual(result, -5)

    def test_negative_in_expression(self):
        result = self.interp.run('let x = 10 + -3; x;')
        self.assertEqual(result, 7)

    def test_double_negative(self):
        result = self.interp.run('let x = --5; x;')
        self.assertEqual(result, 5)

    def test_negate_variable(self):
        self.interp.run('let x = 42')
        result = self.interp.run('let y = -x; y;')
        self.assertEqual(result, -42)

    def test_negate_non_number_errors(self):
        self.interp.run('let s = "hello"')
        with self.assertRaises(RuntimeError):
            self.interp.run('-s;')


class TestMathStdlib(unittest.TestCase):
    def setUp(self):
        self.interp = Interpreter()

    def test_abs_positive(self):
        self.assertEqual(self.interp.run('abs(5);'), 5)

    def test_abs_negative(self):
        self.assertEqual(self.interp.run('abs(-5);'), 5)

    def test_floor(self):
        self.assertEqual(self.interp.run('floor(3.7);'), 3)

    def test_ceil(self):
        self.assertEqual(self.interp.run('ceil(3.2);'), 4)

    def test_round(self):
        self.assertEqual(self.interp.run('round(3.5);'), 4)

    def test_round_down(self):
        self.assertEqual(self.interp.run('round(3.4);'), 3)

    def test_min_two_args(self):
        self.assertEqual(self.interp.run('min(3, 7);'), 3)

    def test_min_array(self):
        self.assertEqual(self.interp.run('min([5, 2, 8, 1]);'), 1)

    def test_max_two_args(self):
        self.assertEqual(self.interp.run('max(3, 7);'), 7)

    def test_max_array(self):
        self.assertEqual(self.interp.run('max([5, 2, 8, 1]);'), 8)

    def test_pow(self):
        self.assertEqual(self.interp.run('pow(2, 10);'), 1024)

    def test_sqrt(self):
        self.assertEqual(self.interp.run('sqrt(16);'), 4.0)


class TestTypeBuiltin(unittest.TestCase):
    def setUp(self):
        self.interp = Interpreter()

    def test_type_number(self):
        self.assertEqual(self.interp.run('type(42);'), "number")

    def test_type_float(self):
        self.assertEqual(self.interp.run('type(3.14);'), "number")

    def test_type_string(self):
        self.assertEqual(self.interp.run('type("hello");'), "string")

    def test_type_bool(self):
        self.assertEqual(self.interp.run('type(true);'), "bool")

    def test_type_array(self):
        self.assertEqual(self.interp.run('type([1,2,3]);'), "array")

    def test_type_object(self):
        self.assertEqual(self.interp.run('type({x: 1});'), "object")

    def test_type_null(self):
        self.assertEqual(self.interp.run('type(null);'), "null")


class TestNullLiteral(unittest.TestCase):
    def setUp(self):
        self.interp = Interpreter()

    def test_null_assignment(self):
        result = self.interp.run('let x = null; x;')
        self.assertIsNone(result)

    def test_null_comparison(self):
        result = self.interp.run('let x = null == null; x;')
        self.assertTrue(result)

    def test_null_inequality(self):
        result = self.interp.run('let x = null != 0; x;')
        self.assertTrue(result)

    def test_null_type(self):
        result = self.interp.run('type(null);')
        self.assertEqual(result, "null")


class TestStringMethods(unittest.TestCase):
    def setUp(self):
        self.interp = Interpreter()

    def test_upper(self):
        result = self.interp.run('let x = "hello".upper(); x;')
        self.assertEqual(result, "HELLO")

    def test_lower(self):
        result = self.interp.run('let x = "HELLO".lower(); x;')
        self.assertEqual(result, "hello")

    def test_trim(self):
        result = self.interp.run('let x = "  hello  ".trim(); x;')
        self.assertEqual(result, "hello")

    def test_contains_true(self):
        result = self.interp.run('let x = "hello world".contains("world"); x;')
        self.assertTrue(result)

    def test_contains_false(self):
        result = self.interp.run('let x = "hello world".contains("xyz"); x;')
        self.assertFalse(result)

    def test_replace(self):
        result = self.interp.run('let x = "hello world".replace("world", "jot"); x;')
        self.assertEqual(result, "hello jot")

    def test_indexOf_found(self):
        result = self.interp.run('let x = "hello".indexOf("ll"); x;')
        self.assertEqual(result, 2)

    def test_indexOf_not_found(self):
        result = self.interp.run('let x = "hello".indexOf("xyz"); x;')
        self.assertEqual(result, -1)

    def test_upper_on_variable(self):
        self.interp.run('let s = "test"')
        result = self.interp.run('s.upper();')
        self.assertEqual(result, "TEST")


class TestArrayMethods(unittest.TestCase):
    def setUp(self):
        self.interp = Interpreter()

    def test_indexOf_found(self):
        self.interp.run('let arr = [10, 20, 30]')
        result = self.interp.run('arr.indexOf(20);')
        self.assertEqual(result, 1)

    def test_indexOf_not_found(self):
        self.interp.run('let arr = [10, 20, 30]')
        result = self.interp.run('arr.indexOf(99);')
        self.assertEqual(result, -1)

    def test_includes_true(self):
        self.interp.run('let arr = [1, 2, 3]')
        result = self.interp.run('arr.includes(2);')
        self.assertTrue(result)

    def test_includes_false(self):
        self.interp.run('let arr = [1, 2, 3]')
        result = self.interp.run('arr.includes(99);')
        self.assertFalse(result)

    def test_flat(self):
        result = self.interp.run('let x = [[1, 2], [3, 4], 5].flat(); x;')
        self.assertEqual(result, [1, 2, 3, 4, 5])

    def test_concat(self):
        self.interp.run('let a = [1, 2]')
        self.interp.run('let b = [3, 4]')
        result = self.interp.run('a.concat(b);')
        self.assertEqual(result, [1, 2, 3, 4])


class TestCompoundAssignment(unittest.TestCase):
    def setUp(self):
        self.interp = Interpreter()

    def test_plus_assign(self):
        self.interp.run('let x = 10')
        result = self.interp.run('x += 5; x;')
        self.assertEqual(result, 15)

    def test_minus_assign(self):
        self.interp.run('let x = 10')
        result = self.interp.run('x -= 3; x;')
        self.assertEqual(result, 7)

    def test_multiply_assign(self):
        self.interp.run('let x = 10')
        result = self.interp.run('x *= 4; x;')
        self.assertEqual(result, 40)

    def test_divide_assign(self):
        self.interp.run('let x = 20')
        result = self.interp.run('x /= 4; x;')
        self.assertEqual(result, 5.0)

    def test_plus_assign_string(self):
        self.interp.run('let s = "hello"')
        result = self.interp.run('s += " world"; s;')
        self.assertEqual(result, "hello world")

    def test_compound_assign_undefined_errors(self):
        with self.assertRaises(RuntimeError):
            self.interp.run('y += 5;')


class TestErrorHandling(unittest.TestCase):
    def setUp(self):
        self.interp = Interpreter()

    def test_stack_overflow_detection(self):
        self.interp.run('fn recurse() { return recurse(); }')
        with self.assertRaises(RuntimeError) as ctx:
            self.interp.run('recurse();')
        self.assertIn("stack overflow", str(ctx.exception).lower())

    def test_arity_error_too_few(self):
        self.interp.run('fn add(a, b) { return a + b; }')
        with self.assertRaises(RuntimeError) as ctx:
            self.interp.run('add(1);')
        self.assertIn("expects", str(ctx.exception))

    def test_arity_error_too_many(self):
        self.interp.run('fn add(a, b) { return a + b; }')
        with self.assertRaises(RuntimeError) as ctx:
            self.interp.run('add(1, 2, 3);')
        self.assertIn("expects", str(ctx.exception))

    def test_undefined_variable_message(self):
        with self.assertRaises(RuntimeError) as ctx:
            self.interp.run('print xyz;')
        self.assertIn("Undefined variable", str(ctx.exception))
        self.assertIn("xyz", str(ctx.exception))

    def test_divide_by_zero_compound(self):
        self.interp.run('let x = 10')
        with self.assertRaises(RuntimeError):
            self.interp.run('x /= 0;')


class TestFormatValue(unittest.TestCase):
    """Test that print/str() formats values in jot-native style."""
    def setUp(self):
        self.interp = Interpreter()

    def test_str_bool(self):
        result = self.interp.run('str(true);')
        self.assertEqual(result, "true")

    def test_str_null(self):
        result = self.interp.run('str(null);')
        self.assertEqual(result, "null")

    def test_str_whole_float(self):
        result = self.interp.run('str(10 / 2);')
        self.assertEqual(result, "5")


if __name__ == "__main__":
    unittest.main()
