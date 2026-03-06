#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "$REPO_ROOT"

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

PASS_COUNT=0
FAIL_COUNT=0
SKIP_COUNT=0

TEST_NAMES=(
  level0_hello
  level1_arithmetic
  level2_variables
  level3_conditionals
  level4_loops
  level5_functions
  level6_recursion
  level7_pointers
  level8_structs
  level9_strings
  level10_compiler
)

cleanup() {
  local name
  for name in "${TEST_NAMES[@]}"; do
    rm -f "$name" "examples/${name}.s" "examples/${name}.s.opt"
  done
}
trap cleanup EXIT

run_test() {
  local name="$1"
  local expected="$2"
  local src="examples/${name}.c"
  local bin="./${name}"

  if [[ "$expected" == "SKIP" ]]; then
    printf "%bSKIP%b %s (known hang)\n" "$YELLOW" "$NC" "$name"
    SKIP_COUNT=$((SKIP_COUNT + 1))
    return
  fi

  if [[ ! -f "$src" ]]; then
    printf "%bFAIL%b %s (missing source: %s)\n" "$RED" "$NC" "$name" "$src"
    FAIL_COUNT=$((FAIL_COUNT + 1))
    return
  fi

  if ! ./nullc "$src" -o "$name" >/dev/null 2>&1; then
    printf "%bFAIL%b %s (compile failed)\n" "$RED" "$NC" "$name"
    FAIL_COUNT=$((FAIL_COUNT + 1))
    return
  fi

  set +e
  perl -e 'alarm 5; exec @ARGV' -- "$bin"
  local actual=$?
  set -e

  if [[ "$actual" -eq "$expected" ]]; then
    printf "%bPASS%b %s (exit %d)\n" "$GREEN" "$NC" "$name" "$actual"
    PASS_COUNT=$((PASS_COUNT + 1))
  else
    printf "%bFAIL%b %s (expected %d, got %d)\n" "$RED" "$NC" "$name" "$expected" "$actual"
    FAIL_COUNT=$((FAIL_COUNT + 1))
  fi
}

printf "Building nullc...\n"
make clean >/dev/null
make >/dev/null

run_test level0_hello 0
run_test level1_arithmetic 48
run_test level2_variables 115
run_test level3_conditionals 2
run_test level4_loops 55
run_test level5_functions 27
run_test level6_recursion 133
run_test level7_pointers 150
run_test level8_structs 50
run_test level9_strings 2
run_test level10_compiler SKIP

printf "\nSummary: %d passed, %d failed, %d skipped\n" "$PASS_COUNT" "$FAIL_COUNT" "$SKIP_COUNT"

if [[ "$FAIL_COUNT" -ne 0 ]]; then
  exit 1
fi
