#!/usr/bin/env python3
"""
jot Interpreter - Executes the AST
"""

import math
from dataclasses import dataclass
from typing import Dict, Any, Optional
from python.parser import *
from python.lexer import TokenType

class ReturnValue(Exception):
    """Exception to handle early returns from functions"""
    def __init__(self, value):
        self.value = value

class BreakException(Exception):
    """Exception to handle break statements in loops"""
    pass

class ContinueException(Exception):
    """Exception to handle continue statements in loops"""
    pass

class JotException(Exception):
    """User-thrown exception from jot code"""
    def __init__(self, value):
        self.value = value
        super().__init__(str(value))

MAX_CALL_DEPTH = 200


class Environment:
    def __init__(self, parent: Optional['Environment'] = None):
        self.parent = parent
        self.values: Dict[str, Any] = {}

    def define(self, name: str, value: Any):
        self.values[name] = value

    def get(self, name: str) -> Any:
        if name in self.values:
            return self.values[name]
        if self.parent is not None:
            return self.parent.get(name)
        raise KeyError(name)

    def assign(self, name: str, value: Any):
        if name in self.values:
            self.values[name] = value
            return
        if self.parent is not None:
            self.parent.assign(name, value)
            return
        self.values[name] = value


@dataclass
class UserFunction:
    declaration: FunctionDef
    closure: Environment

class Interpreter:
    def __init__(self):
        self.globals = Environment()
        self.environment = self.globals
        self.variables = self.globals.values
        self.functions: Dict[str, UserFunction] = {}
        self.classes: Dict[str, 'ClassDef'] = {}
        self.imported_files: set = set()
        self.current_instance: Optional[Dict] = None
        self._call_depth = 0
        self._setup_stdlib()

    def error(self, message: str):
        raise RuntimeError(f"Runtime Error: {message}")

    def _format_value(self, value):
        """Format a value for display (print, str(), interpolation)"""
        if value is None:
            return "null"
        elif isinstance(value, bool):
            return "true" if value else "false"
        elif isinstance(value, float) and value == int(value):
            return str(int(value))
        else:
            return str(value)

    def _type_of(self, value):
        """Return jot type name for a value"""
        if value is None:
            return "null"
        elif isinstance(value, bool):
            return "bool"
        elif isinstance(value, (int, float)):
            return "number"
        elif isinstance(value, str):
            return "string"
        elif isinstance(value, list):
            return "array"
        elif isinstance(value, dict):
            return "object"
        else:
            return "unknown"

    def _flatten(self, arr):
        """Flatten array one level deep"""
        result = []
        for item in arr:
            if isinstance(item, list):
                result.extend(item)
            else:
                result.append(item)
        return result

    def _min_impl(self, *args):
        if len(args) == 1 and isinstance(args[0], list):
            if len(args[0]) == 0:
                self.error("min() requires non-empty array")
            return min(args[0])
        if len(args) < 2:
            self.error("min() requires at least 2 arguments or an array")
        return min(args)

    def _max_impl(self, *args):
        if len(args) == 1 and isinstance(args[0], list):
            if len(args[0]) == 0:
                self.error("max() requires non-empty array")
            return max(args[0])
        if len(args) < 2:
            self.error("max() requires at least 2 arguments or an array")
        return max(args)

    def _http_post_impl(self, url: str, data: str) -> str:
        if not isinstance(url, str) or not isinstance(data, str):
            self.error("http_post() requires string arguments")
        import urllib.request
        req = urllib.request.Request(url, data=data.encode('utf-8'), method='POST')
        req.add_header('Content-Type', 'application/json')
        return urllib.request.urlopen(req).read().decode('utf-8')

    def _resolve_function_by_name(self, name: str) -> UserFunction:
        try:
            value = self.environment.get(name)
            if isinstance(value, UserFunction):
                return value
        except KeyError:
            pass

        func = self.functions.get(name)
        if func is not None:
            return func

        self.error(f"Function '{name}' not defined")

    def _call_function(self, function: UserFunction, args, bound_instance=None):
        """Call a function with evaluated arguments, handling default params"""
        if self._call_depth >= MAX_CALL_DEPTH:
            self.error("Maximum call depth exceeded (stack overflow)")

        self._call_depth += 1
        previous_env = self.environment
        previous_instance = self.current_instance
        call_env = Environment(function.closure)
        self.environment = call_env
        self.current_instance = bound_instance

        try:
            params = function.declaration.params
            for i, param_info in enumerate(params):
                if isinstance(param_info, tuple):
                    name, default = param_info
                else:
                    name, default = param_info, None

                if i < len(args):
                    call_env.define(name, args[i])
                elif default is not None:
                    call_env.define(name, self.evaluate(default))
                else:
                    self.error(f"Missing argument for parameter '{name}'")

            result = None
            try:
                for stmt in function.declaration.body:
                    result = self.evaluate(stmt)
            except ReturnValue as ret:
                result = ret.value
            except RecursionError:
                self.error("Maximum call depth exceeded (stack overflow)")

            return result
        finally:
            self.environment = previous_env
            self.current_instance = previous_instance
            self._call_depth -= 1

    def _map_impl(self, func_name, arr):
        if not isinstance(arr, list):
            self.error("map() requires array")
        if not isinstance(func_name, str):
            self.error("map() requires function name as string")
        func = self._resolve_function_by_name(func_name)
        return [self._call_function(func, [item]) for item in arr]

    def _filter_impl(self, func_name, arr):
        if not isinstance(arr, list):
            self.error("filter() requires array")
        if not isinstance(func_name, str):
            self.error("filter() requires function name as string")
        func = self._resolve_function_by_name(func_name)
        return [item for item in arr if self._call_function(func, [item])]

    def _reduce_impl(self, func_name, arr, init=None):
        if not isinstance(arr, list):
            self.error("reduce() requires array")
        if not isinstance(func_name, str):
            self.error("reduce() requires function name as string")
        if len(arr) == 0:
            return init
        func = self._resolve_function_by_name(func_name)
        accumulator = init if init is not None else arr[0]
        start_idx = 0 if init is not None else 1
        for i in range(start_idx, len(arr)):
            accumulator = self._call_function(func, [accumulator, arr[i]])
        return accumulator

    def _setup_stdlib(self):
        """Built-in functions"""
        self.builtins = {
            # Core
            'len': lambda x: len(x) if isinstance(x, (list, str, dict)) else self.error("len() requires array, string, or object"),
            'push': lambda arr, item: arr.append(item) or arr if isinstance(arr, list) else self.error("push() requires array"),
            'pop': lambda arr: arr.pop() if isinstance(arr, list) and len(arr) > 0 else self.error("pop() requires non-empty array"),
            'range': lambda start, end: list(range(int(start), int(end))),
            'str': lambda x: self._format_value(x),
            'int': lambda x: int(x),
            'slice': lambda s, start, end: s[int(start):int(end)] if isinstance(s, str) else self.error("slice() requires string"),
            'split': lambda s, delim: s.split(delim) if isinstance(s, str) else self.error("split() requires string"),
            'sort': lambda arr: sorted(arr) if isinstance(arr, list) else self.error("sort() requires array"),
            'reverse': lambda arr: list(reversed(arr)) if isinstance(arr, list) else self.error("reverse() requires array"),
            'join': lambda arr, delim="": delim.join(str(x) for x in arr) if isinstance(arr, list) else self.error("join() requires array"),

            # Type checking
            'type': lambda x: self._type_of(x),

            # Math stdlib
            'abs': lambda x: abs(x),
            'floor': lambda x: math.floor(x),
            'ceil': lambda x: math.ceil(x),
            'round': lambda x: round(x),
            'min': lambda *args: self._min_impl(*args),
            'max': lambda *args: self._max_impl(*args),
            'pow': lambda base, exp: base ** exp,
            'sqrt': lambda x: math.sqrt(x),

            # Object methods (called via obj.method())
            '__method_keys': lambda obj: list(obj.keys()) if isinstance(obj, dict) else self.error("keys() requires object"),
            '__method_values': lambda obj: list(obj.values()) if isinstance(obj, dict) else self.error("values() requires object"),
            '__method_has': lambda obj, key: key in obj if isinstance(obj, dict) else self.error("has() requires object"),

            # String methods (called via str.method())
            '__method_upper': lambda s: s.upper() if isinstance(s, str) else self.error("upper() requires string"),
            '__method_lower': lambda s: s.lower() if isinstance(s, str) else self.error("lower() requires string"),
            '__method_trim': lambda s: s.strip() if isinstance(s, str) else self.error("trim() requires string"),
            '__method_contains': lambda s, sub: sub in s if isinstance(s, str) else self.error("contains() requires string"),
            '__method_replace': lambda s, old, new: s.replace(old, new) if isinstance(s, str) else self.error("replace() requires string"),
            '__method_indexOf': lambda container, item: (
                container.find(item) if isinstance(container, str)
                else (container.index(item) if item in container else -1) if isinstance(container, list)
                else self.error("indexOf() requires string or array")
            ),

            # Array methods (called via arr.method())
            '__method_includes': lambda arr, item: item in arr if isinstance(arr, list) else self.error("includes() requires array"),
            '__method_flat': lambda arr: self._flatten(arr) if isinstance(arr, list) else self.error("flat() requires array"),
            '__method_concat': lambda arr, other: arr + other if isinstance(arr, list) and isinstance(other, list) else self.error("concat() requires two arrays"),

            # File I/O
            'read': lambda path: open(path, 'r').read() if isinstance(path, str) else self.error("read() requires string"),
            'write': lambda path, content: (open(path, 'w').write(content), content)[1] if isinstance(path, str) else self.error("write() requires string"),
            'append': lambda path, content: (open(path, 'a').write(content), content)[1] if isinstance(path, str) else self.error("append() requires string"),
            # JSON
            'parse': lambda s: __import__('json').loads(s) if isinstance(s, str) else self.error("parse() requires string"),
            'stringify': lambda obj: __import__('json').dumps(obj),
            # HTTP
            'http_get': lambda url: __import__('urllib.request').request.urlopen(url).read().decode('utf-8') if isinstance(url, str) else self.error("http_get() requires string"),
            'http_post': lambda url, data: self._http_post_impl(url, data),
            # Functional programming
            'map': lambda func_name, arr: self._map_impl(func_name, arr),
            'filter': lambda func_name, arr: self._filter_impl(func_name, arr),
            'reduce': lambda func_name, arr, init=None: self._reduce_impl(func_name, arr, init),
        }

    def evaluate(self, node: ASTNode) -> Any:
        """Evaluate an AST node"""
        if isinstance(node, Number):
            return node.value

        elif isinstance(node, String):
            return node.value

        elif isinstance(node, Boolean):
            return node.value

        elif isinstance(node, NullLiteral):
            return None

        elif isinstance(node, Variable):
            try:
                return self.environment.get(node.name)
            except KeyError:
                if node.name in self.functions:
                    return self.functions[node.name]
                self.error(f"Undefined variable: '{node.name}'")

        elif isinstance(node, UnaryOp):
            operand = self.evaluate(node.operand)
            if node.operator == TokenType.NOT:
                return not operand
            elif node.operator == TokenType.MINUS:
                if not isinstance(operand, (int, float)):
                    self.error(f"Cannot negate {self._type_of(operand)}")
                return -operand
            else:
                self.error(f"Unknown unary operator: {node.operator}")

        elif isinstance(node, BinaryOp):
            left = self.evaluate(node.left)
            right = self.evaluate(node.right)

            if node.operator == TokenType.PLUS:
                return left + right
            elif node.operator == TokenType.MINUS:
                return left - right
            elif node.operator == TokenType.MULTIPLY:
                return left * right
            elif node.operator == TokenType.DIVIDE:
                if right == 0:
                    self.error("Division by zero")
                return left / right
            elif node.operator == TokenType.MODULO:
                if right == 0:
                    self.error("Modulo by zero")
                return left % right
            elif node.operator == TokenType.GT:
                return left > right
            elif node.operator == TokenType.LT:
                return left < right
            elif node.operator == TokenType.EQ:
                return left == right
            elif node.operator == TokenType.NEQ:
                return left != right
            elif node.operator == TokenType.GTE:
                return left >= right
            elif node.operator == TokenType.LTE:
                return left <= right
            elif node.operator == TokenType.AND:
                return left and right
            elif node.operator == TokenType.OR:
                return left or right
            else:
                self.error(f"Unknown operator: {node.operator}")

        elif isinstance(node, Ternary):
            condition = self.evaluate(node.condition)
            if condition:
                return self.evaluate(node.then_expr)
            else:
                return self.evaluate(node.else_expr)

        elif isinstance(node, Assignment):
            value = self.evaluate(node.value)
            self.environment.assign(node.name, value)
            return value

        elif isinstance(node, VariableDeclaration):
            value = self.evaluate(node.value)
            self.environment.define(node.name, value)
            return value

        elif isinstance(node, CompoundAssignment):
            try:
                current = self.environment.get(node.name)
            except KeyError:
                self.error(f"Undefined variable: '{node.name}'")
            value = self.evaluate(node.value)
            if node.operator == TokenType.PLUS_ASSIGN:
                result = current + value
            elif node.operator == TokenType.MINUS_ASSIGN:
                result = current - value
            elif node.operator == TokenType.MULTIPLY_ASSIGN:
                result = current * value
            elif node.operator == TokenType.DIVIDE_ASSIGN:
                if value == 0:
                    self.error("Division by zero")
                result = current / value
            else:
                self.error(f"Unknown compound operator: {node.operator}")
            self.environment.assign(node.name, result)
            return result

        elif isinstance(node, StringInterpolation):
            result = ""
            for kind, content in node.parts:
                if kind == 'lit':
                    result += content
                elif kind == 'expr':
                    from python.lexer import Lexer as SubLexer
                    from python.parser import Parser as SubParser
                    lexer = SubLexer(content)
                    tokens = lexer.tokenize()
                    parser = SubParser(tokens)
                    expr = parser.parse_expression()
                    value = self.evaluate(expr)
                    result += self._format_value(value)
            return result

        elif isinstance(node, Print):
            value = self.evaluate(node.expression)
            print(self._format_value(value))
            return None

        elif isinstance(node, IfStatement):
            condition = self.evaluate(node.condition)
            if condition:
                result = None
                for stmt in node.then_block:
                    result = self.evaluate(stmt)
                return result
            elif node.else_block:
                result = None
                for stmt in node.else_block:
                    result = self.evaluate(stmt)
                return result
            return None

        elif isinstance(node, WhileLoop):
            result = None
            while self.evaluate(node.condition):
                try:
                    for stmt in node.body:
                        result = self.evaluate(stmt)
                except BreakException:
                    break
                except ContinueException:
                    continue
            return result

        elif isinstance(node, ForLoop):
            iterable = self.evaluate(node.iterable)
            if not isinstance(iterable, list):
                self.error("For loop requires an array")
            result = None
            for value in iterable:
                self.environment.assign(node.var, value)
                try:
                    for stmt in node.body:
                        result = self.evaluate(stmt)
                except BreakException:
                    break
                except ContinueException:
                    continue
            return result

        elif isinstance(node, FunctionDef):
            function = UserFunction(node, self.environment)
            self.functions[node.name] = function
            self.environment.define(node.name, function)
            return None

        elif isinstance(node, ClassDef):
            self.classes[node.name] = node
            return None

        elif isinstance(node, NewInstance):
            if node.class_name not in self.classes:
                self.error(f"Undefined class: '{node.class_name}'")

            class_def = self.classes[node.class_name]
            instance = {'__class__': node.class_name}

            for method in class_def.methods:
                instance[method.name] = UserFunction(method, self.environment)

            if 'init' in instance:
                init_method = instance['init']
                arg_values = [self.evaluate(arg) for arg in node.args]
                self._call_function(init_method, arg_values, bound_instance=instance)

            return instance

        elif isinstance(node, This):
            if self.current_instance is None:
                self.error("'this' used outside of class method")
            return self.current_instance

        elif isinstance(node, FunctionCall):
            # Method calls (obj.method())
            if node.name.startswith('__method_'):
                method_name = node.name[9:]

                if len(node.args) == 0:
                    self.error("Method call requires object")

                obj = self.evaluate(node.args[0])
                other_args = [self.evaluate(arg) for arg in node.args[1:]]

                # Check builtin methods first (string, array, object builtins)
                if node.name in self.builtins:
                    return self.builtins[node.name](obj, *other_args)

                # Then check object/class instance methods
                if isinstance(obj, dict):
                    if method_name in obj:
                        method = obj[method_name]
                        if isinstance(method, UserFunction):
                            return self._call_function(method, other_args, bound_instance=obj)
                    self.error(f"Object has no method '{method_name}'")

                self.error(f"Cannot call method '{method_name}' on {self._type_of(obj)}")

            # Built-in functions
            if node.name in self.builtins:
                args = [self.evaluate(arg) for arg in node.args]
                return self.builtins[node.name](*args)

            # User-defined functions
            func_def = self._resolve_function_by_name(node.name)

            # Validate arity with default params
            required = sum(1 for p in func_def.declaration.params for name, default in [p if isinstance(p, tuple) else (p, None)] if default is None)
            total = len(func_def.declaration.params)
            if len(node.args) < required or len(node.args) > total:
                if required == total:
                    self.error(f"Function '{node.name}' expects {total} args, got {len(node.args)}")
                else:
                    self.error(f"Function '{node.name}' expects {required}-{total} args, got {len(node.args)}")

            arg_values = [self.evaluate(arg) for arg in node.args]
            return self._call_function(func_def, arg_values)

        elif isinstance(node, Return):
            value = self.evaluate(node.value) if node.value else None
            raise ReturnValue(value)

        elif isinstance(node, Break):
            raise BreakException()

        elif isinstance(node, Continue):
            raise ContinueException()

        elif isinstance(node, TryCatch):
            try:
                result = None
                for stmt in node.try_block:
                    result = self.evaluate(stmt)
                return result
            except JotException as e:
                if node.catch_var:
                    self.environment.assign(node.catch_var, e.value)
                result = None
                for stmt in node.catch_block:
                    result = self.evaluate(stmt)
                return result
            except Exception as e:
                if node.catch_var:
                    self.environment.assign(node.catch_var, str(e))
                result = None
                for stmt in node.catch_block:
                    result = self.evaluate(stmt)
                return result

        elif isinstance(node, Throw):
            value = self.evaluate(node.value)
            raise JotException(value)

        elif isinstance(node, Import):
            import os
            from python.lexer import Lexer
            from python.parser import Parser

            path = node.path
            abs_path = os.path.abspath(path)

            if abs_path in self.imported_files:
                return None

            self.imported_files.add(abs_path)

            try:
                with open(abs_path, 'r') as f:
                    source = f.read()

                lexer = Lexer(source)
                tokens = lexer.tokenize()
                parser = Parser(tokens)
                program = parser.parse()

                for stmt in program.statements:
                    self.evaluate(stmt)

                return None
            except FileNotFoundError:
                self.error(f"Cannot import '{path}': file not found")
            except Exception as e:
                self.error(f"Error importing '{path}': {e}")

        elif isinstance(node, Array):
            return [self.evaluate(elem) for elem in node.elements]

        elif isinstance(node, ArrayIndex):
            container = self.evaluate(node.array)
            index = self.evaluate(node.index)

            if isinstance(container, list):
                if not isinstance(index, (int, float)):
                    self.error("Array index must be a number")
                index = int(index)
                if index < 0 or index >= len(container):
                    self.error(f"Array index {index} out of bounds (length {len(container)})")
                return container[index]
            elif isinstance(container, dict):
                if not isinstance(index, str):
                    self.error("Object key must be a string")
                if index not in container:
                    self.error(f"Object has no property '{index}'")
                return container[index]
            elif isinstance(container, str):
                if not isinstance(index, (int, float)):
                    self.error("String index must be a number")
                index = int(index)
                if index < 0 or index >= len(container):
                    self.error(f"String index {index} out of bounds (length {len(container)})")
                return container[index]
            else:
                self.error(f"Cannot index {self._type_of(container)}")

        elif isinstance(node, ObjectLiteral):
            obj = {}
            for key, value_node in node.pairs:
                value = self.evaluate(value_node)
                obj[key] = value
            return obj

        elif isinstance(node, ObjectAccess):
            obj = self.evaluate(node.object)
            if not isinstance(obj, dict):
                self.error(f"Cannot access property on {self._type_of(obj)}")

            if node.is_bracket:
                key = self.evaluate(node.key)
                if not isinstance(key, str):
                    self.error("Object key must be a string")
            else:
                key = node.key

            if key not in obj:
                self.error(f"Object has no property '{key}'")
            return obj[key]

        elif isinstance(node, ObjectAssignment):
            obj = self.evaluate(node.object)
            if not isinstance(obj, dict):
                self.error(f"Cannot assign property on {self._type_of(obj)}")

            if node.is_bracket:
                key = self.evaluate(node.key)
                if not isinstance(key, str):
                    self.error("Object key must be a string")
            else:
                key = node.key

            value = self.evaluate(node.value)
            obj[key] = value
            return value

        elif isinstance(node, Program):
            result = None
            for statement in node.statements:
                result = self.evaluate(statement)
            return result

        else:
            self.error(f"Unknown node type: {type(node).__name__}")

    def run(self, code: str):
        """Run jot code"""
        from python.lexer import Lexer
        from python.parser import Parser

        lexer = Lexer(code)
        tokens = lexer.tokenize()
        parser = Parser(tokens)
        ast = parser.parse()
        return self.evaluate(ast)
