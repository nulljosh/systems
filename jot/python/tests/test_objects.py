#!/usr/bin/env python3
"""
Test object/dictionary functionality in jot
"""

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from python.lexer import Lexer
from python.parser import Parser
from python.interpreter import Interpreter


def test_basic_objects():
    """Test basic object creation and access"""
    print("=== Test 1: Basic Objects ===")
    code = '''
    let obj = {name: "Alice", age: 30};
    print obj;
    print obj.name;
    print obj.age;
    '''
    
    interpreter = Interpreter()
    interpreter.run(code)
    print()


def test_bracket_notation():
    """Test bracket notation access"""
    print("=== Test 2: Bracket Notation ===")
    code = '''
    let person = {name: "Bob", city: "Vancouver"};
    print person["name"];
    
    let key = "city";
    print person[key];
    '''
    
    interpreter = Interpreter()
    interpreter.run(code)
    print()


def test_nested_objects():
    """Test nested objects"""
    print("=== Test 3: Nested Objects ===")
    code = '''
    let user = {
        name: "Charlie",
        address: {
            city: "Seattle",
            zip: 98101
        }
    };
    
    print user.address;
    print user.address.city;
    print user.address.zip;
    '''
    
    interpreter = Interpreter()
    interpreter.run(code)
    print()


def test_object_methods():
    """Test object methods (keys, values, has)"""
    print("=== Test 4: Object Methods ===")
    code = '''
    let data = {x: 10, y: 20, z: 30};
    
    print data.keys();
    print data.values();
    print data.has("x");
    print data.has("w");
    '''
    
    interpreter = Interpreter()
    interpreter.run(code)
    print()


def test_mixed_structures():
    """Test objects with arrays and vice versa"""
    print("=== Test 5: Mixed Structures ===")
    code = '''
    // Object with array
    let team = {
        name: "Engineers",
        members: ["Alice", "Bob", "Charlie"]
    };
    print team.members[1];
    
    // Array of objects
    let users = [
        {name: "Alice", role: "admin"},
        {name: "Bob", role: "user"}
    ];
    print users[0].name;
    print users[1].role;
    '''
    
    interpreter = Interpreter()
    interpreter.run(code)
    print()


def test_object_operations():
    """Test operations with object values"""
    print("=== Test 6: Object Operations ===")
    code = '''
    let point = {x: 5, y: 10};
    let sum = point.x + point.y;
    print sum;
    
    let mult = point.x * point.y;
    print mult;
    '''
    
    interpreter = Interpreter()
    interpreter.run(code)
    print()


def test_empty_object():
    """Test empty object"""
    print("=== Test 7: Empty Object ===")
    code = '''
    let empty = {};
    print empty;
    print empty.keys();
    print len(empty);
    '''
    
    interpreter = Interpreter()
    interpreter.run(code)
    print()


def test_object_update():
    """Test updating object properties"""
    print("=== Test 8: Object Updates ===")
    code = '''
    let config = {debug: true, port: 8080};
    print config.debug;
    
    config.debug = false;
    print config.debug;
    
    config.port = 3000;
    print config.port;
    '''
    
    interpreter = Interpreter()
    interpreter.run(code)
    print()


def test_bytecode_objects():
    """Test objects via interpreter (bytecode compiler removed)"""
    print("=== Test 9: Object Interpreter ===")
    interp = Interpreter()
    interp.run('''
    let obj = {name: "Test", value: 42};
    print obj.name;
    print obj.value;
    ''')
    print()


def test_complex_example():
    """Test a complex real-world example"""
    print("=== Test 10: Complex Example ===")
    code = '''
    let app = {
        name: "MyApp",
        version: "1.0",
        config: {
            debug: true,
            features: ["auth", "api", "ui"]
        },
        stats: {
            users: 1000,
            requests: 50000
        }
    };
    
    print app.name;
    print app.config.features[0];
    print app.stats.users;
    
    // Calculate average requests per user
    let avg = app.stats.requests / app.stats.users;
    print avg;
    '''
    
    interpreter = Interpreter()
    interpreter.run(code)
    print()


if __name__ == "__main__":
    print("Testing Object/Dictionary Support in jot\n")
    print("=" * 50)
    print()
    
    test_basic_objects()
    test_bracket_notation()
    test_nested_objects()
    test_object_methods()
    test_mixed_structures()
    test_object_operations()
    test_empty_object()
    test_object_update()
    test_bytecode_objects()
    test_complex_example()
    
    print("=" * 50)
    print("\nAll tests completed!")
