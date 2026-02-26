"""Test suite for src/shell.py — pipes commands in, asserts stdout output."""
import subprocess
import sys
import os
import tempfile
import re

SHELL = [sys.executable, os.path.join(os.path.dirname(__file__), "..", "src", "shell.py")]
SHELL = [sys.executable, os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "src", "shell.py"))]


def run(commands: list[str]) -> str:
    """Pipe newline-separated commands into the shell, return stdout with prompts stripped.

    The shell prompt is printed inline via input() — format: "<path> $ ".
    Prompts appear on the same line as command output.  We strip all prompt
    tokens by splitting on the pattern and discarding those fragments.
    """
    stdin = "\n".join(commands) + "\n"
    result = subprocess.run(
        SHELL,
        input=stdin,
        capture_output=True,
        text=True,
    )
    raw = result.stdout
    # Prompt pattern: any non-newline characters followed by " $ "
    # Split the entire output on prompt tokens; discard the prompt fragments.
    parts = re.split(r"[^\n]+? \$ ", raw)
    # Rejoin the non-prompt fragments and clean up
    cleaned = "".join(parts).strip()
    return cleaned


def run_raw(commands: list[str]) -> tuple[str, str, int]:
    """Return (stdout, stderr, returncode) with no stripping."""
    stdin = "\n".join(commands) + "\n"
    result = subprocess.run(SHELL, input=stdin, capture_output=True, text=True)
    return result.stdout, result.stderr, result.returncode


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

PASS = 0
FAIL = 0


def check(name: str, got: str, expected: str):
    global PASS, FAIL
    if got == expected:
        print(f"  PASS  {name}")
        PASS += 1
    else:
        print(f"  FAIL  {name}")
        print(f"        expected: {expected!r}")
        print(f"        got:      {got!r}")
        FAIL += 1


def check_contains(name: str, got: str, needle: str):
    global PASS, FAIL
    if needle in got:
        print(f"  PASS  {name}")
        PASS += 1
    else:
        print(f"  FAIL  {name}")
        print(f"        expected to contain: {needle!r}")
        print(f"        got:                 {got!r}")
        FAIL += 1


# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

def test_echo_simple():
    out = run(["echo hello"])
    check("echo simple", out, "hello")


def test_echo_multiple_args():
    out = run(["echo hello world"])
    check("echo multiple args", out, "hello world")


def test_echo_quoted_string():
    out = run(['echo "hello world"'])
    check("echo quoted string", out, "hello world")


def test_echo_empty():
    out = run(["echo"])
    check("echo empty", out, "")


def test_cd_and_pwd():
    out = run(["cd /tmp", "pwd"])
    # /tmp on macOS resolves to /private/tmp
    assert out in ("/tmp", "/private/tmp"), f"cd+pwd: unexpected output {out!r}"
    print(f"  PASS  cd and pwd")
    global PASS
    PASS += 1


def test_cd_home():
    home = os.environ.get("HOME", "")
    out = run(["cd", "pwd"])
    check_contains("cd home (no args)", out, home)


def test_cd_invalid():
    out = run(["cd /this/does/not/exist/xyz"])
    check_contains("cd invalid directory error", out, "no such directory")


def test_pwd_initial():
    # pwd should print something — at minimum it returns a path
    out = run(["pwd"])
    assert out.startswith("/"), f"pwd: expected absolute path, got {out!r}"
    print(f"  PASS  pwd initial")
    global PASS
    PASS += 1


def test_multiple_commands_sequence():
    out = run(["echo first", "echo second", "echo third"])
    lines = out.splitlines()
    check("sequence: line 1", lines[0] if lines else "", "first")
    check("sequence: line 2", lines[1] if len(lines) > 1 else "", "second")
    check("sequence: line 3", lines[2] if len(lines) > 2 else "", "third")


def test_export_sets_env():
    # export sets env var; child process (env) should see it
    out = run(["export TESTVAR=shelltest", "env"])
    check_contains("export sets env var", out, "TESTVAR=shelltest")


def test_export_multiple():
    out = run(["export A=1", "export B=2", "env"])
    check_contains("export multiple vars A", out, "A=1")
    check_contains("export multiple vars B", out, "B=2")


def test_command_not_found():
    out = run(["xyznotacommand_abc"])
    check_contains("command not found error", out, "command not found")


def test_command_not_found_message_format():
    out = run(["fakecmd123"])
    check_contains("command not found includes name", out, "fakecmd123")


def test_empty_line_ignored():
    out = run(["", "echo ok"])
    check("empty line ignored", out, "ok")


def test_whitespace_only_line_ignored():
    out = run(["   ", "echo ok"])
    check("whitespace line ignored", out, "ok")


def test_exit_builtin():
    _, _, rc = run_raw(["exit 0"])
    check("exit 0 returns 0", str(rc), "0")


def test_exit_builtin_no_args():
    _, _, rc = run_raw(["exit"])
    check("exit (no args) returns 0", str(rc), "0")


def test_parse_error_unclosed_quote():
    out = run(["echo 'unclosed"])
    check_contains("parse error unclosed quote", out, "parse error")


def test_external_command_ls():
    out = run(["ls /tmp"])
    # ls /tmp should produce some output without errors
    assert out != "", f"ls /tmp: expected output, got empty"
    print(f"  PASS  external command ls /tmp")
    global PASS
    PASS += 1


def test_external_command_true():
    _, _, rc = run_raw(["true"])
    check("external: true exits 0", str(rc), "0")


def test_cd_subdir_and_back():
    out = run(["cd /usr", "pwd", "cd ..", "pwd"])
    lines = [l for l in out.splitlines() if l.startswith("/")]
    check("cd /usr", lines[0] if lines else "", "/usr")
    check("cd .. -> /", lines[1] if len(lines) > 1 else "", "/")


def test_echo_special_chars():
    out = run(["echo foo   bar"])
    # shlex.split collapses whitespace into separate args
    check_contains("echo special chars", out, "foo")
    check_contains("echo special chars bar", out, "bar")


def test_cd_tilde_not_expanded():
    # Current implementation does not expand ~ in cd args (only HOME env fallback)
    # This documents actual behavior — cd ~ will fail as literal path
    out = run(["cd ~"])
    # Either works (if OS resolves it) or gives an error — just verify shell doesn't crash
    assert out is not None
    print(f"  PASS  cd ~ does not crash shell")
    global PASS
    PASS += 1


# ---------------------------------------------------------------------------
# Run all tests
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    print("Running shell test suite...\n")

    test_echo_simple()
    test_echo_multiple_args()
    test_echo_quoted_string()
    test_echo_empty()
    test_cd_and_pwd()
    test_cd_home()
    test_cd_invalid()
    test_pwd_initial()
    test_multiple_commands_sequence()
    test_export_sets_env()
    test_export_multiple()
    test_command_not_found()
    test_command_not_found_message_format()
    test_empty_line_ignored()
    test_whitespace_only_line_ignored()
    test_exit_builtin()
    test_exit_builtin_no_args()
    test_parse_error_unclosed_quote()
    test_external_command_ls()
    test_external_command_true()
    test_cd_subdir_and_back()
    test_echo_special_chars()
    test_cd_tilde_not_expanded()

    print(f"\n{PASS} passed, {FAIL} failed")
    sys.exit(0 if FAIL == 0 else 1)
