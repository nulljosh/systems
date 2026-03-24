#!/usr/bin/env python3
"""
jot - Scripting language interpreter

Usage:
    python3 main.py                    # Start REPL
    python3 main.py script.jot         # Run a script
    python3 main.py --version          # Show version
"""

import sys
from python.interpreter import Interpreter

VERSION = "5.0.0"

def main():
    if len(sys.argv) > 1:
        if sys.argv[1] == '--version':
            print(f"jot v{VERSION}")
            return

        filename = sys.argv[1]
        interpreter = Interpreter()
        try:
            with open(filename, 'r') as f:
                code = f.read()
            interpreter.run(code)
        except FileNotFoundError:
            print(f"Error: File '{filename}' not found")
            sys.exit(1)
        except Exception as e:
            print(f"Error: {e}")
            sys.exit(1)
    else:
        interpreter = Interpreter()
        print(f"jot v{VERSION}")
        print("Type 'exit' to quit\n")

        while True:
            try:
                code = input("jot> ")

                if code.strip() == 'exit':
                    break

                interpreter.run(code)

            except (SyntaxError, RuntimeError) as e:
                print(f"Error: {e}")
            except KeyboardInterrupt:
                print("\nGoodbye!")
                break
            except EOFError:
                break

if __name__ == "__main__":
    main()
