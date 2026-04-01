# jot Language Specification

Version 5.0.0

## 1. Lexical Structure

**Comments**: `//` or `#` to end of line.
**Semicolons**: Optional. Ignored by the parser.
**Whitespace**: Insignificant outside strings.
**Identifiers**: `[a-zA-Z_][a-zA-Z0-9_]*`

## 2. Types

| Type     | Literal examples         | Notes                          |
|----------|--------------------------|--------------------------------|
| number   | `42`, `3.14`, `-1`       | IEEE 754 double                |
| string   | `"hello"`, `"a\nb"`     | Double-quoted, escape sequences|
| boolean  | `true`, `false`          |                                |
| null     | `null`                   | Absence of value               |
| array    | `[1, 2, 3]`             | Heterogeneous, dynamic         |
| object   | `{x: 1, y: 2}`          | String-keyed hash map          |
| function | `fn name(x) { ... }`    | First-class, closures          |

## 3. Variables

```jot
let x = 10          // declaration + definition
x = 20              // reassignment
x += 5              // compound: +=, -=, *=, /=

// Destructuring (v5.0.0)
let [a, b, c] = [1, 2, 3]           // array
let [first, ...rest] = [1, 2, 3, 4] // array with rest
let {x, y} = {x: 10, y: 20}         // object
```

Scoping: lexical. `let` defines in the current scope. Assignment walks up the scope chain to find an existing binding.

## 4. Operators

**Arithmetic**: `+` `-` `*` `/` `%`
**Comparison**: `>` `<` `>=` `<=` `==` `!=`
**Logical**: `and` `or` `not`
**Ternary**: `condition ? then_expr : else_expr`
**Compound assignment**: `+=` `-=` `*=` `/=`
**Unary**: `-` (negate), `not`

String `+` concatenates. If either operand is a string, the other is coerced.

Precedence (low to high): `or`, `and`, comparison, addition, multiplication, unary, postfix.

## 5. Strings

Double-quoted. Escape sequences: `\n` `\t` `\"` `\\` `\$`.

**Interpolation**: `"hello ${name}, ${2 + 3}"` -- expressions inside `${}` are evaluated.

**Methods**: `.upper()` `.lower()` `.trim()` `.contains(sub)` `.replace(old, new)` `.indexOf(sub)`

## 6. Control Flow

```jot
if condition {
    ...
} else if other {
    ...
} else {
    ...
}

while condition {
    ...
}

for item in iterable {
    ...
}
```

`break` and `continue` work inside loops.

`for` iterates over arrays, strings (by character), and objects (by key).

## 7. Functions

```jot
fn name(a, b, c = default_value) {
    return result
}
```

- Parameters with `= expr` have default values
- Functions are first-class values: storable, passable, returnable
- Implicit `null` return if no `return` statement
- Maximum call depth: 200

### Closures

Functions capture their lexical environment at definition time:

```jot
fn make_counter() {
    let count = 0
    fn increment() {
        count += 1
        return count
    }
    return increment
}
let c = make_counter()
print c()  // 1
print c()  // 2
```

Each closure instance has independent captured state.

## 8. Classes

```jot
class Name {
    fn init(params) {
        this.field = value
    }
    fn method(params) {
        return this.field
    }
}

let obj = new Name(args)
obj.method()
```

- `init` is the constructor (called automatically by `new`)
- `this` refers to the current instance
- Methods are dispatched via `__class__` lookup
- No inheritance

## 9. Arrays

```jot
let arr = [1, "two", true, null]
arr[0]              // indexing (0-based)
arr.length          // element count
```

## 10. Objects

```jot
let obj = {name: "jot", version: 4}
obj.name            // dot access
obj["name"]         // bracket access
obj.name = "new"    // mutation
obj.length          // key count
```

**Methods**: `.keys()` `.values()` `.has(key)`

## 11. Builtins

### Core
`len(x)` `push(arr, item)` `pop(arr)` `range(start, end)` `str(x)` `int(x)` `type(x)`

### String
`slice(s, start, end)` `split(s, delim)` `join(arr, delim)`

### Array
`sort(arr)` `reverse(arr)`

### Math
`abs(x)` `floor(x)` `ceil(x)` `round(x)` `min(a, b)` `max(a, b)` `pow(base, exp)` `sqrt(x)`

### Functional
`map(fn, arr)` `filter(fn, arr)` `reduce(fn, arr, init)`

### I/O
`read(path)` `write(path, content)` `append(path, content)`

### JSON
`parse(str)` `stringify(obj)`

### HTTP
`http_get(url)` `http_post(url, data)`

## 12. Imports

```jot
import "path/to/file.jot"
```

Circular imports are prevented (each file imported at most once per program).

## 13. Error Handling

```jot
try {
    throw "error message"
} catch (e) {
    print e
}
```

Runtime errors inside `try` blocks are caught. `throw` accepts any value (converted to string).

### Error Message Format

```
SyntaxError line <line>:<col>: <message>
ParseError line <line>:<col>: <message>
RuntimeError line <line>: <message>
```

## 14. Limits

| Limit          | Value |
|----------------|-------|
| Call depth     | 200   |
| Scope depth    | 256   |
| Try depth      | 32    |
| Import depth   | 64    |
