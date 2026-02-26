#!/bin/bash
# Test suite for shell -- C implementation
# Run: cd systems/shell && make && bash tests/test_shell.sh

SHELL_BIN="./shell"
PASS=0
FAIL=0
TMPF="/tmp/shell_test_$$"

check() {
    local name="$1" got="$2" expected="$3"
    if [ "$got" = "$expected" ]; then
        echo "  PASS  $name"
        ((PASS++))
    else
        echo "  FAIL  $name"
        echo "        expected: '$expected'"
        echo "        got:      '$got'"
        ((FAIL++))
    fi
}

check_contains() {
    local name="$1" got="$2" needle="$3"
    if echo "$got" | grep -q "$needle"; then
        echo "  PASS  $name"
        ((PASS++))
    else
        echo "  FAIL  $name"
        echo "        expected to contain: '$needle'"
        echo "        got:                 '$got'"
        ((FAIL++))
    fi
}

# strip prompt lines from output
run() {
    printf '%s\n' "$@" | $SHELL_BIN 2>&1 | sed 's/^[^$]*\$ //' | grep -v '^\s*$'
}

run_raw() {
    printf '%s\n' "$@" | $SHELL_BIN 2>&1
}

echo "Running shell test suite..."
echo ""

# echo
out=$(run "echo hello")
check "echo simple" "$out" "hello"

out=$(run "echo hello world")
check "echo multiple args" "$out" "hello world"

out=$(run 'echo "hello world"')
check "echo quoted string" "$out" "hello world"

# cd + pwd
out=$(run "cd /tmp" "pwd")
check_contains "cd and pwd" "$out" "/tmp"

out=$(run "cd /this/does/not/exist/xyz" 2>&1)
check_contains "cd invalid directory" "$out" "no such directory"

# pwd
out=$(run "pwd")
check_contains "pwd returns path" "$out" "/"

# sequence
out=$(run "echo first" "echo second" "echo third")
check_contains "sequence first" "$out" "first"
check_contains "sequence second" "$out" "second"
check_contains "sequence third" "$out" "third"

# export + env
out=$(run "export TESTVAR=shelltest" "env")
check_contains "export sets env" "$out" "TESTVAR=shelltest"

# $VAR expansion
out=$(run "export FOO=bar" 'echo $FOO')
check_contains "env var expansion" "$out" "bar"

# command not found
out=$(run "xyznotacommand_abc" 2>&1)
check_contains "command not found" "$out" "command not found"

# empty line
out=$(run "" "echo ok")
check_contains "empty line ignored" "$out" "ok"

# exit
printf 'exit 0\n' | $SHELL_BIN > /dev/null 2>&1
check "exit 0" "$?" "0"

# parse error
out=$(run "echo 'unclosed" 2>&1)
check_contains "unclosed quote error" "$out" "parse error"

# pipes
out=$(run "echo hello | cat")
check "pipe echo to cat" "$out" "hello"

out=$(printf 'echo -e "aaa\nbbb\nccc" | grep bbb\nexit\n' | $SHELL_BIN 2>&1 | sed 's/^[^$]*\$ //' | grep -v '^\s*$')
check_contains "pipe with grep" "$out" "bbb"

# redirects
printf 'echo redirect_test > %s\nexit\n' "$TMPF" | $SHELL_BIN > /dev/null 2>&1
got=$(cat "$TMPF" 2>/dev/null)
check "redirect out" "$got" "redirect_test"
rm -f "$TMPF"

printf 'echo line1 > %s\necho line2 >> %s\nexit\n' "$TMPF" "$TMPF" | $SHELL_BIN > /dev/null 2>&1
got=$(cat "$TMPF" 2>/dev/null)
check_contains "redirect append" "$got" "line1"
check_contains "redirect append line2" "$got" "line2"
rm -f "$TMPF"

# redirect in
echo "from_file" > "$TMPF"
out=$(printf 'cat < %s\nexit\n' "$TMPF" | $SHELL_BIN 2>&1 | sed 's/^[^$]*\$ //' | grep -v '^\s*$')
check "redirect in" "$out" "from_file"
rm -f "$TMPF"

# history
out=$(run "echo one" "echo two" "history")
check_contains "history shows entries" "$out" "echo one"

# external command
out=$(run "true")
check "external true" "$?" "0"

echo ""
echo "$PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ] && exit 0 || exit 1
