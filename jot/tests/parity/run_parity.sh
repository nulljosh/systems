#!/bin/bash
# Cross-runtime parity test runner
# Runs each .jot through both Python and C interpreters, compares output
set -e

DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$DIR/../.." && pwd)"
JOT_C="$ROOT/src/jot"
PASS=0
FAIL=0
ERRORS=""

for test in "$DIR"/*.jot; do
    name=$(basename "$test" .jot)
    expected="$DIR/${name}.expected"

    if [ ! -f "$expected" ]; then
        echo "SKIP $name (no .expected file)"
        continue
    fi

    exp=$(cat "$expected")

    # C interpreter
    c_out=$("$JOT_C" "$test" 2>&1) || true
    # Python interpreter
    py_out=$(cd "$ROOT" && PYTHONPATH=. python3 python/main.py "$test" 2>&1) || true

    c_pass=0; py_pass=0; match=0
    [ "$c_out" = "$exp" ] && c_pass=1
    [ "$py_out" = "$exp" ] && py_pass=1
    [ "$c_out" = "$py_out" ] && match=1

    if [ $c_pass -eq 1 ] && [ $py_pass -eq 1 ]; then
        printf "\033[32mPASS\033[0m %s\n" "$name"
        PASS=$((PASS+1))
    else
        printf "\033[31mFAIL\033[0m %s" "$name"
        [ $c_pass -eq 0 ] && printf " [C mismatch]"
        [ $py_pass -eq 0 ] && printf " [Python mismatch]"
        [ $match -eq 0 ] && printf " [C!=Python]"
        printf "\n"
        FAIL=$((FAIL+1))
        ERRORS="$ERRORS $name"
    fi
done

echo ""
echo "Results: $PASS passed, $FAIL failed"
if [ $FAIL -gt 0 ]; then
    echo "Failed:$ERRORS"
    exit 1
fi
