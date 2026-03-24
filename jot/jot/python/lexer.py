#!/usr/bin/env python3
"""
JoshLang Lexer - Tokenizes source code into tokens

Tips for first language:
1. Start with just numbers and basic math
2. Add one feature at a time
3. Test everything as you go
4. Don't worry about optimization yet
"""

from enum import Enum, auto
from dataclasses import dataclass
from typing import List, Optional

class TokenType(Enum):
    # Literals
    NUMBER = auto()      # 123, 3.14
    STRING = auto()      # "hello"
    INTERP_STRING = auto()  # "hello ${name}"
    IDENTIFIER = auto()  # x, foo, bar
    TRUE = auto()        # true
    FALSE = auto()       # false
    NULL = auto()        # null

    # Keywords
    LET = auto()         # let x = 5
    IF = auto()          # if x > 5
    ELSE = auto()        # else
    WHILE = auto()       # while x < 10
    FOR = auto()         # for
    IN = auto()          # in
    PRINT = auto()       # print x
    AND = auto()         # and
    OR = auto()          # or
    NOT = auto()         # not
    FN = auto()          # fn
    RETURN = auto()      # return
    BREAK = auto()       # break
    CONTINUE = auto()    # continue
    IMPORT = auto()      # import
    TRY = auto()         # try
    CATCH = auto()       # catch
    THROW = auto()       # throw
    CLASS = auto()       # class
    NEW = auto()         # new
    THIS = auto()        # this
    
    # Operators
    PLUS = auto()        # +
    MINUS = auto()       # -
    MULTIPLY = auto()    # *
    DIVIDE = auto()      # /
    MODULO = auto()      # %
    ASSIGN = auto()      # =
    PLUS_ASSIGN = auto()    # +=
    MINUS_ASSIGN = auto()   # -=
    MULTIPLY_ASSIGN = auto() # *=
    DIVIDE_ASSIGN = auto()  # /=
    QUESTION = auto()    # ?
    GT = auto()          # >
    LT = auto()          # <
    EQ = auto()          # ==
    NEQ = auto()         # !=
    GTE = auto()         # >=
    LTE = auto()         # <=
    
    # Delimiters
    LPAREN = auto()      # (
    RPAREN = auto()      # )
    LBRACE = auto()      # {
    RBRACE = auto()      # }
    LBRACKET = auto()    # [
    RBRACKET = auto()    # ]
    SEMICOLON = auto()   # ;
    COMMA = auto()       # ,
    COLON = auto()       # :
    DOT = auto()         # .
    
    # Special
    EOF = auto()         # End of file

@dataclass
class Token:
    type: TokenType
    value: any
    line: int
    column: int

class Lexer:
    def __init__(self, source: str):
        self.source = source
        self.position = 0
        self.line = 1
        self.column = 1
        self.tokens = []
        
        # Keywords mapping
        self.keywords = {
            'let': TokenType.LET,
            'if': TokenType.IF,
            'else': TokenType.ELSE,
            'while': TokenType.WHILE,
            'for': TokenType.FOR,
            'in': TokenType.IN,
            'print': TokenType.PRINT,
            'true': TokenType.TRUE,
            'false': TokenType.FALSE,
            'null': TokenType.NULL,
            'and': TokenType.AND,
            'or': TokenType.OR,
            'not': TokenType.NOT,
            'fn': TokenType.FN,
            'return': TokenType.RETURN,
            'break': TokenType.BREAK,
            'continue': TokenType.CONTINUE,
            'import': TokenType.IMPORT,
            'try': TokenType.TRY,
            'catch': TokenType.CATCH,
            'throw': TokenType.THROW,
            'class': TokenType.CLASS,
            'new': TokenType.NEW,
            'this': TokenType.THIS,
        }
    
    def error(self, message: str):
        raise SyntaxError(f"Line {self.line}, Col {self.column}: {message}")
    
    def peek(self, offset: int = 0) -> Optional[str]:
        """Look at character without advancing"""
        pos = self.position + offset
        if pos < len(self.source):
            return self.source[pos]
        return None
    
    def advance(self) -> Optional[str]:
        """Get current char and move forward"""
        if self.position >= len(self.source):
            return None
            
        ch = self.source[self.position]
        self.position += 1
        
        if ch == '\n':
            self.line += 1
            self.column = 1
        else:
            self.column += 1
            
        return ch
    
    def skip_whitespace(self):
        """Skip spaces, tabs, newlines"""
        while self.peek() and self.peek() in ' \t\n\r':
            self.advance()
    
    def skip_comment(self):
        """Skip // and # comments"""
        while self.peek() and self.peek() != '\n':
            self.advance()
    
    def read_number(self) -> Token:
        """Read integer or float"""
        start_col = self.column
        num_str = ''
        
        while self.peek() and self.peek().isdigit():
            num_str += self.advance()
        
        # Handle decimals
        if self.peek() == '.' and self.peek(1) and self.peek(1).isdigit():
            num_str += self.advance()  # the '.'
            while self.peek() and self.peek().isdigit():
                num_str += self.advance()
            value = float(num_str)
        else:
            value = int(num_str)
        
        return Token(TokenType.NUMBER, value, self.line, start_col)
    
    def read_string(self) -> Token:
        """Read string literal, detecting ${...} interpolation"""
        start_col = self.column
        self.advance()  # Skip opening quote

        parts = []
        current = ''
        has_interpolation = False

        while self.peek() and self.peek() != '"':
            if self.peek() == '\\':
                self.advance()
                if self.peek() == 'n':
                    current += '\n'
                    self.advance()
                elif self.peek() == 't':
                    current += '\t'
                    self.advance()
                elif self.peek() == '"':
                    current += '"'
                    self.advance()
                elif self.peek() == '$':
                    current += '$'
                    self.advance()
                else:
                    current += self.advance()
            elif self.peek() == '$' and self.peek(1) == '{':
                has_interpolation = True
                if current:
                    parts.append(('lit', current))
                    current = ''
                self.advance()  # skip $
                self.advance()  # skip {
                expr = ''
                depth = 1
                while self.peek() and depth > 0:
                    if self.peek() == '{':
                        depth += 1
                    elif self.peek() == '}':
                        depth -= 1
                    if depth > 0:
                        expr += self.advance()
                    else:
                        self.advance()  # skip closing }
                parts.append(('expr', expr))
            else:
                current += self.advance()

        if not self.peek():
            self.error("Unterminated string")

        self.advance()  # Skip closing quote

        if has_interpolation:
            if current:
                parts.append(('lit', current))
            return Token(TokenType.INTERP_STRING, parts, self.line, start_col)
        else:
            return Token(TokenType.STRING, current, self.line, start_col)
    
    def read_identifier(self) -> Token:
        """Read identifier or keyword"""
        start_col = self.column
        value = ''
        
        # First char must be letter or underscore
        if self.peek().isalpha() or self.peek() == '_':
            value += self.advance()
        
        # Rest can be letters, digits, or underscore
        while self.peek() and (self.peek().isalnum() or self.peek() == '_'):
            value += self.advance()
        
        # Check if it's a keyword
        token_type = self.keywords.get(value, TokenType.IDENTIFIER)
        return Token(token_type, value, self.line, start_col)
    
    def next_token(self) -> Token:
        """Get next token"""
        # Skip all whitespace and comments
        while True:
            self.skip_whitespace()
            
            # Skip // and # comments
            if (self.peek() == '/' and self.peek(1) == '/') or self.peek() == '#':
                self.skip_comment()
                continue
            
            break
        
        # End of file
        if not self.peek():
            return Token(TokenType.EOF, None, self.line, self.column)
        
        ch = self.peek()
        
        # Numbers
        if ch.isdigit():
            return self.read_number()
        
        # Strings
        if ch == '"':
            return self.read_string()
        
        # Identifiers and keywords
        if ch.isalpha() or ch == '_':
            return self.read_identifier()
        
        # Two-char tokens (check these first)
        if ch == '=':
            col = self.column
            self.advance()
            if self.peek() == '=':
                self.advance()
                return Token(TokenType.EQ, '==', self.line, col)
            else:
                return Token(TokenType.ASSIGN, '=', self.line, col)

        if ch == '!':
            col = self.column
            self.advance()
            if self.peek() == '=':
                self.advance()
                return Token(TokenType.NEQ, '!=', self.line, col)
            else:
                self.error(f"Unexpected character: '!'")

        if ch == '>':
            col = self.column
            self.advance()
            if self.peek() == '=':
                self.advance()
                return Token(TokenType.GTE, '>=', self.line, col)
            else:
                return Token(TokenType.GT, '>', self.line, col)

        if ch == '<':
            col = self.column
            self.advance()
            if self.peek() == '=':
                self.advance()
                return Token(TokenType.LTE, '<=', self.line, col)
            else:
                return Token(TokenType.LT, '<', self.line, col)

        # Compound assignment and single-char operators
        if ch == '+':
            col = self.column
            self.advance()
            if self.peek() == '=':
                self.advance()
                return Token(TokenType.PLUS_ASSIGN, '+=', self.line, col)
            return Token(TokenType.PLUS, '+', self.line, col)

        if ch == '-':
            col = self.column
            self.advance()
            if self.peek() == '=':
                self.advance()
                return Token(TokenType.MINUS_ASSIGN, '-=', self.line, col)
            return Token(TokenType.MINUS, '-', self.line, col)

        if ch == '*':
            col = self.column
            self.advance()
            if self.peek() == '=':
                self.advance()
                return Token(TokenType.MULTIPLY_ASSIGN, '*=', self.line, col)
            return Token(TokenType.MULTIPLY, '*', self.line, col)

        if ch == '?':
            col = self.column
            self.advance()
            return Token(TokenType.QUESTION, '?', self.line, col)

        # Division or /=
        if ch == '/':
            col = self.column
            self.advance()
            if self.peek() == '=':
                self.advance()
                return Token(TokenType.DIVIDE_ASSIGN, '/=', self.line, col)
            return Token(TokenType.DIVIDE, '/', self.line, col)

        # Single-char tokens
        single_char = {
            '%': TokenType.MODULO,
            '(': TokenType.LPAREN,
            ')': TokenType.RPAREN,
            '{': TokenType.LBRACE,
            '}': TokenType.RBRACE,
            '[': TokenType.LBRACKET,
            ']': TokenType.RBRACKET,
            ';': TokenType.SEMICOLON,
            ',': TokenType.COMMA,
            ':': TokenType.COLON,
            '.': TokenType.DOT,
        }

        if ch in single_char:
            col = self.column
            self.advance()
            return Token(single_char[ch], ch, self.line, col)
        
        # Unknown character
        self.error(f"Unexpected character: '{ch}'")
    
    def tokenize(self) -> List[Token]:
        """Tokenize entire source"""
        tokens = []
        
        while True:
            token = self.next_token()
            tokens.append(token)
            if token.type == TokenType.EOF:
                break
                
        return tokens


def test_lexer():
    """Test the lexer with sample code"""
    code = '''
    // My first program
    let x = 10;
    let y = 20;
    let sum = x + y;
    
    if sum > 25 {
        print "Big sum!";
    } else {
        print sum;
    }
    
    while x < 100 {
        x = x * 2;
        print x;
    }
    '''
    
    lexer = Lexer(code)
    tokens = lexer.tokenize()
    
    print("Tokens:")
    for token in tokens:
        if token.type != TokenType.EOF:
            print(f"  {token.type.name:12} {repr(token.value):15} Line {token.line}:{token.column}")


if __name__ == "__main__":
    test_lexer()