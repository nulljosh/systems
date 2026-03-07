"""Minimal shell skeleton — REPL + fork/exec."""
import os
import sys
import shlex


BUILTINS = {}


def builtin(name):
    def decorator(fn):
        BUILTINS[name] = fn
        return fn
    return decorator


@builtin("cd")
def cmd_cd(args):
    path = args[0] if args else os.environ.get("HOME", "/")
    try:
        os.chdir(path)
    except FileNotFoundError:
        print(f"cd: no such directory: {path}")


@builtin("exit")
def cmd_exit(args):
    sys.exit(int(args[0]) if args else 0)


@builtin("export")
def cmd_export(args):
    for arg in args:
        if "=" in arg:
            k, v = arg.split("=", 1)
            os.environ[k] = v


def execute(args: list[str]):
    if not args:
        return
    cmd, *rest = args
    if cmd in BUILTINS:
        BUILTINS[cmd](rest)
        return
    pid = os.fork()
    if pid == 0:
        try:
            os.execvp(cmd, args)
        except FileNotFoundError:
            print(f"{cmd}: command not found")
            sys.exit(127)
    else:
        os.waitpid(pid, 0)


def main():
    while True:
        try:
            line = input(f"{os.getcwd().replace(os.environ.get('HOME','~'), '~')} $ ")
        except (EOFError, KeyboardInterrupt):
            print()
            break
        if not line.strip():
            continue
        try:
            args = shlex.split(line)
        except ValueError as e:
            print(f"parse error: {e}")
            continue
        execute(args)


if __name__ == "__main__":
    main()
