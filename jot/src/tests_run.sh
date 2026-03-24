#!/bin/bash
# jot C test runner -- compares stdout of .jot files against .expected files

set -euo pipefail

DIR="$(cd "$(dirname "$0")" && pwd)"
JOT="$DIR/jot"

if [ ! -x "$JOT" ]; then
    echo "Error: jot binary not found. Run 'make' first."
    exit 1
fi

pass=0
fail=0
errors=""

for test_file in "$DIR"/tests/*.jot; do
    name="$(basename "$test_file" .jot)"
    expected="$DIR/tests/$name.expected"

    if [ ! -f "$expected" ]; then
        echo -e "\033[33mSKIP\033[0m $name (no .expected file)"
        continue
    fi

    actual=$("$JOT" "$test_file" 2>&1) || true

    if [ "$actual" = "$(cat "$expected")" ]; then
        echo -e "\033[32mPASS\033[0m $name"
        pass=$((pass + 1))
    else
        echo -e "\033[31mFAIL\033[0m $name"
        diff --color=always <(echo "$actual") "$expected" || true
        fail=$((fail + 1))
        errors="$errors $name"
    fi
done

echo ""
echo "Results: $pass passed, $fail failed"

if [ $fail -gt 0 ]; then
    echo "Failed:$errors"
    exit 1
fi
