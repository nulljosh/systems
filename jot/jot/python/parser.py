#!/usr/bin/env python3
"""
jot Parser - Converts tokens into an Abstract Syntax Tree (AST)
"""

from dataclasses import dataclass
from typing import List, Optional, Union
from python.lexer import Lexer, Token, TokenType

# AST Node types
@dataclass
class ASTNode:
    pass

@dataclass
class Number(ASTNode):
    value: Union[int, float]

@dataclass
class String(ASTNode):
    value: str

@dataclass
class Boolean(ASTNode):
    value: bool

@dataclass
class NullLiteral(ASTNode):
    pass

@dataclass
class Variable(ASTNode):
    name: str

@dataclass
class BinaryOp(ASTNode):
    left: ASTNode
    operator: TokenType
    right: ASTNode

@dataclass
class UnaryOp(ASTNode):
    operator: TokenType
    operand: ASTNode

@dataclass
class Assignment(ASTNode):
    name: str
    value: ASTNode

@dataclass
class VariableDeclaration(ASTNode):
    name: str
    value: ASTNode

@dataclass
class CompoundAssignment(ASTNode):
    name: str
    operator: 'TokenType'
    value: ASTNode

@dataclass
class Ternary(ASTNode):
    condition: ASTNode
    then_expr: ASTNode
    else_expr: ASTNode

@dataclass
class StringInterpolation(ASTNode):
    parts: list  # List of ('lit', str) or ('expr', str) tuples

@dataclass
class Print(ASTNode):
    expression: ASTNode

@dataclass
class IfStatement(ASTNode):
    condition: ASTNode
    then_block: List[ASTNode]
    else_block: Optional[List[ASTNode]] = None

@dataclass
class WhileLoop(ASTNode):
    condition: ASTNode
    body: List[ASTNode]

@dataclass
class ForLoop(ASTNode):
    var: str
    iterable: ASTNode
    body: List[ASTNode]

@dataclass
class FunctionDef(ASTNode):
    name: str
    params: List[tuple]  # List of (name: str, default: Optional[ASTNode])
    body: List[ASTNode]

@dataclass
class FunctionCall(ASTNode):
    name: str
    args: List[ASTNode]

@dataclass
class Return(ASTNode):
    value: Optional[ASTNode]

@dataclass
class Break(ASTNode):
    pass

@dataclass
class Continue(ASTNode):
    pass

@dataclass
class Import(ASTNode):
    path: str

@dataclass
class TryCatch(ASTNode):
    try_block: List[ASTNode]
    catch_var: Optional[str]
    catch_block: List[ASTNode]

@dataclass
class Throw(ASTNode):
    value: ASTNode

@dataclass
class ClassDef(ASTNode):
    name: str
    methods: List['FunctionDef']

@dataclass
class NewInstance(ASTNode):
    class_name: str
    args: List[ASTNode]

@dataclass
class This(ASTNode):
    pass

@dataclass
class Array(ASTNode):
    elements: List[ASTNode]

@dataclass
class ArrayIndex(ASTNode):
    array: ASTNode
    index: ASTNode

@dataclass
class ObjectLiteral(ASTNode):
    pairs: List[tuple[str, ASTNode]]

@dataclass
class ObjectAccess(ASTNode):
    object: ASTNode
    key: Union[str, ASTNode]
    is_bracket: bool = False

@dataclass
class ObjectAssignment(ASTNode):
    object: ASTNode
    key: Union[str, ASTNode]
    value: ASTNode
    is_bracket: bool = False

@dataclass
class Program(ASTNode):
    statements: List[ASTNode]


class Parser:
    def __init__(self, tokens: List[Token]):
        self.tokens = tokens
        self.position = 0

    def error(self, message: str):
        if self.position < len(self.tokens):
            token = self.tokens[self.position]
            raise SyntaxError(f"Line {token.line}:{token.column} - {message}")
        else:
            raise SyntaxError(f"Unexpected end of file - {message}")

    def peek(self, offset: int = 0) -> Optional[Token]:
        pos = self.position + offset
        if pos < len(self.tokens):
            return self.tokens[pos]
        return None

    def advance(self) -> Token:
        if self.position >= len(self.tokens):
            self.error("Unexpected end of file")
        token = self.tokens[self.position]
        self.position += 1
        return token

    def match(self, *types: TokenType) -> bool:
        current = self.peek()
        if current and current.type in types:
            return True
        return False

    def consume(self, token_type: TokenType, message: str) -> Token:
        if not self.match(token_type):
            self.error(message)
        return self.advance()

    def optional_semicolon(self):
        """Consume semicolon if present (makes ; optional)"""
        if self.match(TokenType.SEMICOLON):
            self.advance()

    def parse_primary(self) -> ASTNode:
        # Unary not
        if self.match(TokenType.NOT):
            op = self.advance()
            operand = self.parse_primary()
            return UnaryOp(op.type, operand)

        # Unary minus
        if self.match(TokenType.MINUS):
            op = self.advance()
            operand = self.parse_primary()
            return UnaryOp(op.type, operand)

        # Numbers
        if self.match(TokenType.NUMBER):
            return Number(self.advance().value)

        # Strings
        if self.match(TokenType.STRING):
            return String(self.advance().value)

        # Interpolated strings
        if self.match(TokenType.INTERP_STRING):
            return StringInterpolation(self.advance().value)

        # Booleans
        if self.match(TokenType.TRUE):
            self.advance()
            return Boolean(True)

        if self.match(TokenType.FALSE):
            self.advance()
            return Boolean(False)

        # Null literal
        if self.match(TokenType.NULL):
            self.advance()
            return NullLiteral()

        # this keyword
        if self.match(TokenType.THIS):
            self.advance()
            return This()

        # new ClassName(args)
        if self.match(TokenType.NEW):
            self.advance()
            class_name = self.consume(TokenType.IDENTIFIER, "Expected class name after 'new'").value
            self.consume(TokenType.LPAREN, "Expected '(' after class name")

            args = []
            if not self.match(TokenType.RPAREN):
                args.append(self.parse_expression())
                while self.match(TokenType.COMMA):
                    self.advance()
                    args.append(self.parse_expression())

            self.consume(TokenType.RPAREN, "Expected ')' after arguments")
            return NewInstance(class_name, args)

        # Variables or function calls
        if self.match(TokenType.IDENTIFIER):
            name = self.advance().value
            if self.match(TokenType.LPAREN):
                self.advance()
                args = []
                if not self.match(TokenType.RPAREN):
                    args.append(self.parse_expression())
                    while self.match(TokenType.COMMA):
                        self.advance()
                        args.append(self.parse_expression())
                self.consume(TokenType.RPAREN, "Expected ')' after arguments")
                return FunctionCall(name, args)
            else:
                return Variable(name)

        # Array literal
        if self.match(TokenType.LBRACKET):
            self.advance()
            elements = []
            if not self.match(TokenType.RBRACKET):
                elements.append(self.parse_expression())
                while self.match(TokenType.COMMA):
                    self.advance()
                    elements.append(self.parse_expression())
            self.consume(TokenType.RBRACKET, "Expected ']'")
            return Array(elements)

        # Object literal
        if self.match(TokenType.LBRACE):
            self.advance()
            pairs = []
            if not self.match(TokenType.RBRACE):
                key = self.consume(TokenType.IDENTIFIER, "Expected property name").value
                self.consume(TokenType.COLON, "Expected ':' after property name")
                value = self.parse_expression()
                pairs.append((key, value))

                while self.match(TokenType.COMMA):
                    self.advance()
                    if self.match(TokenType.RBRACE):
                        break
                    key = self.consume(TokenType.IDENTIFIER, "Expected property name").value
                    self.consume(TokenType.COLON, "Expected ':' after property name")
                    value = self.parse_expression()
                    pairs.append((key, value))

            self.consume(TokenType.RBRACE, "Expected '}'")
            return ObjectLiteral(pairs)

        # Parentheses
        if self.match(TokenType.LPAREN):
            self.advance()
            expr = self.parse_expression()
            self.consume(TokenType.RPAREN, "Expected ')' after expression")
            return expr

        self.error("Expected expression")

    def parse_postfix(self) -> ASTNode:
        left = self.parse_primary()

        while True:
            if self.match(TokenType.LBRACKET):
                self.advance()
                index = self.parse_expression()
                self.consume(TokenType.RBRACKET, "Expected ']'")
                left = ArrayIndex(left, index)

            elif self.match(TokenType.DOT):
                self.advance()
                if self.match(TokenType.IDENTIFIER):
                    name = self.advance().value

                    if self.match(TokenType.LPAREN):
                        self.advance()
                        args = []
                        if not self.match(TokenType.RPAREN):
                            args.append(self.parse_expression())
                            while self.match(TokenType.COMMA):
                                self.advance()
                                args.append(self.parse_expression())
                        self.consume(TokenType.RPAREN, "Expected ')' after arguments")
                        left = FunctionCall(f"__method_{name}", [left] + args)
                    else:
                        left = ObjectAccess(left, name, is_bracket=False)
                else:
                    self.error("Expected property name after '.'")
            else:
                break

        return left

    def parse_multiplication(self) -> ASTNode:
        left = self.parse_postfix()
        while self.match(TokenType.MULTIPLY, TokenType.DIVIDE, TokenType.MODULO):
            op = self.advance()
            right = self.parse_postfix()
            left = BinaryOp(left, op.type, right)
        return left

    def parse_addition(self) -> ASTNode:
        left = self.parse_multiplication()
        while self.match(TokenType.PLUS, TokenType.MINUS):
            op = self.advance()
            right = self.parse_multiplication()
            left = BinaryOp(left, op.type, right)
        return left

    def parse_comparison(self) -> ASTNode:
        left = self.parse_addition()
        while self.match(TokenType.GT, TokenType.LT, TokenType.EQ, TokenType.NEQ, TokenType.GTE, TokenType.LTE):
            op = self.advance()
            right = self.parse_addition()
            left = BinaryOp(left, op.type, right)
        return left

    def parse_and(self) -> ASTNode:
        left = self.parse_comparison()
        while self.match(TokenType.AND):
            op = self.advance()
            right = self.parse_comparison()
            left = BinaryOp(left, op.type, right)
        return left

    def parse_or(self) -> ASTNode:
        left = self.parse_and()
        while self.match(TokenType.OR):
            op = self.advance()
            right = self.parse_and()
            left = BinaryOp(left, op.type, right)
        return left

    def parse_ternary(self) -> ASTNode:
        expr = self.parse_or()
        if self.match(TokenType.QUESTION):
            self.advance()
            then_expr = self.parse_ternary()
            self.consume(TokenType.COLON, "Expected ':' in ternary expression")
            else_expr = self.parse_ternary()
            return Ternary(expr, then_expr, else_expr)
        return expr

    def parse_expression(self) -> ASTNode:
        return self.parse_ternary()

    def _parse_params(self) -> List[tuple]:
        """Parse function parameters with optional defaults"""
        params = []
        if not self.match(TokenType.RPAREN):
            pname = self.consume(TokenType.IDENTIFIER, "Expected parameter name").value
            default = None
            if self.match(TokenType.ASSIGN):
                self.advance()
                default = self.parse_expression()
            params.append((pname, default))
            while self.match(TokenType.COMMA):
                self.advance()
                pname = self.consume(TokenType.IDENTIFIER, "Expected parameter name").value
                default = None
                if self.match(TokenType.ASSIGN):
                    self.advance()
                    default = self.parse_expression()
                params.append((pname, default))
        return params

    def parse_block(self) -> List[ASTNode]:
        self.consume(TokenType.LBRACE, "Expected '{'")
        statements = []
        while not self.match(TokenType.RBRACE) and not self.match(TokenType.EOF):
            stmt = self.parse_statement()
            if stmt:
                statements.append(stmt)
        self.consume(TokenType.RBRACE, "Expected '}'")
        return statements

    def parse_statement(self) -> Optional[ASTNode]:
        # Class definition
        if self.match(TokenType.CLASS):
            self.advance()
            name = self.consume(TokenType.IDENTIFIER, "Expected class name").value
            self.consume(TokenType.LBRACE, "Expected '{' after class name")

            methods = []
            while not self.match(TokenType.RBRACE):
                if not self.match(TokenType.FN):
                    self.error("Expected method definition in class")
                self.advance()
                method_name = self.consume(TokenType.IDENTIFIER, "Expected method name").value
                self.consume(TokenType.LPAREN, "Expected '(' after method name")
                params = self._parse_params()
                self.consume(TokenType.RPAREN, "Expected ')' after parameters")
                body = self.parse_block()
                methods.append(FunctionDef(method_name, params, body))

            self.consume(TokenType.RBRACE, "Expected '}' after class body")
            return ClassDef(name, methods)

        # Function definition
        if self.match(TokenType.FN):
            self.advance()
            name = self.consume(TokenType.IDENTIFIER, "Expected function name").value
            self.consume(TokenType.LPAREN, "Expected '(' after function name")
            params = self._parse_params()
            self.consume(TokenType.RPAREN, "Expected ')' after parameters")
            body = self.parse_block()
            return FunctionDef(name, params, body)

        # Return statement
        if self.match(TokenType.RETURN):
            self.advance()
            value = None
            if not self.match(TokenType.SEMICOLON) and not self.match(TokenType.RBRACE) and not self.match(TokenType.EOF):
                value = self.parse_expression()
            self.optional_semicolon()
            return Return(value)

        # Break statement
        if self.match(TokenType.BREAK):
            self.advance()
            self.optional_semicolon()
            return Break()

        # Continue statement
        if self.match(TokenType.CONTINUE):
            self.advance()
            self.optional_semicolon()
            return Continue()

        # Import statement
        if self.match(TokenType.IMPORT):
            self.advance()
            path = self.consume(TokenType.STRING, "Expected string path after import").value
            self.optional_semicolon()
            return Import(path)

        # Try-catch statement
        if self.match(TokenType.TRY):
            self.advance()
            try_block = self.parse_block()

            self.consume(TokenType.CATCH, "Expected 'catch' after try block")

            catch_var = None
            if self.match(TokenType.LPAREN):
                self.advance()
                catch_var = self.consume(TokenType.IDENTIFIER, "Expected variable name in catch").value
                self.consume(TokenType.RPAREN, "Expected ')' after catch variable")

            catch_block = self.parse_block()
            return TryCatch(try_block, catch_var, catch_block)

        # Throw statement
        if self.match(TokenType.THROW):
            self.advance()
            value = self.parse_expression()
            self.optional_semicolon()
            return Throw(value)

        # If statement
        if self.match(TokenType.IF):
            self.advance()
            condition = self.parse_expression()
            then_block = self.parse_block()

            else_block = None
            if self.match(TokenType.ELSE):
                self.advance()
                if self.match(TokenType.IF):
                    # else if chain
                    else_block = [self.parse_statement()]
                else:
                    else_block = self.parse_block()

            return IfStatement(condition, then_block, else_block)

        # While loop
        if self.match(TokenType.WHILE):
            self.advance()
            condition = self.parse_expression()
            body = self.parse_block()
            return WhileLoop(condition, body)

        # For loop
        if self.match(TokenType.FOR):
            self.advance()
            var = self.consume(TokenType.IDENTIFIER, "Expected variable name").value
            self.consume(TokenType.IN, "Expected 'in'")
            iterable = self.parse_expression()
            body = self.parse_block()
            return ForLoop(var, iterable, body)

        # Variable assignment
        if self.match(TokenType.LET):
            self.advance()
            name = self.consume(TokenType.IDENTIFIER, "Expected variable name").value
            self.consume(TokenType.ASSIGN, "Expected '=' in assignment")
            value = self.parse_expression()
            self.optional_semicolon()
            return VariableDeclaration(name, value)

        # Print statement
        if self.match(TokenType.PRINT):
            self.advance()
            expr = self.parse_expression()
            self.optional_semicolon()
            return Print(expr)

        # Re-assignment or compound assignment
        if self.match(TokenType.IDENTIFIER):
            if self.peek(1) and self.peek(1).type == TokenType.ASSIGN:
                name = self.advance().value
                self.advance()  # consume '='
                value = self.parse_expression()
                self.optional_semicolon()
                return Assignment(name, value)
            if self.peek(1) and self.peek(1).type in (TokenType.PLUS_ASSIGN, TokenType.MINUS_ASSIGN, TokenType.MULTIPLY_ASSIGN, TokenType.DIVIDE_ASSIGN):
                name = self.advance().value
                op = self.advance()
                value = self.parse_expression()
                self.optional_semicolon()
                return CompoundAssignment(name, op.type, value)

        # Expression statement (may be object property assignment)
        if not self.match(TokenType.EOF):
            expr = self.parse_expression()

            if isinstance(expr, (ObjectAccess, ArrayIndex)) and self.match(TokenType.ASSIGN):
                self.advance()
                value = self.parse_expression()
                self.optional_semicolon()

                if isinstance(expr, ObjectAccess):
                    return ObjectAssignment(expr.object, expr.key, value, expr.is_bracket)
                else:
                    return ObjectAssignment(expr.array, expr.index, value, is_bracket=True)

            self.optional_semicolon()
            return expr

        return None

    def parse(self) -> Program:
        statements = []
        while not self.match(TokenType.EOF):
            stmt = self.parse_statement()
            if stmt:
                statements.append(stmt)
        return Program(statements)
