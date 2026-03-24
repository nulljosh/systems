#!/bin/bash
# jot benchmark runner -- compares C and Python interpreter performance
set -e

DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$DIR/.." && pwd)"
JOT_C="$ROOT/src/jot"
RUNS=3

printf "%-20s %10s %10s %8s\n" "Benchmark" "C (s)" "Python (s)" "Ratio"
printf "%-20s %10s %10s %8s\n" "--------------------" "----------" "----------" "--------"

for bench in "$DIR"/*.jot; do
    name=$(basename "$bench" .jot)

    # Time C interpreter (median of 3 runs)
    c_times=()
    for ((r=1; r<=RUNS; r++)); do
        t=$( { time "$JOT_C" "$bench" > /dev/null 2>&1; } 2>&1 | grep real | awk '{print $2}' )
        # Convert to seconds
        min=$(echo "$t" | sed 's/m.*//')
        sec=$(echo "$t" | sed 's/.*m//' | sed 's/s//')
        total=$(echo "$min * 60 + $sec" | bc)
        c_times+=("$total")
    done
    c_median=$(printf '%s\n' "${c_times[@]}" | sort -n | sed -n '2p')

    # Time Python interpreter (median of 3 runs)
    py_times=()
    for ((r=1; r<=RUNS; r++)); do
        t=$( { time (cd "$ROOT" && PYTHONPATH=. python3 python/main.py "$bench") > /dev/null 2>&1; } 2>&1 | grep real | awk '{print $2}' )
        min=$(echo "$t" | sed 's/m.*//')
        sec=$(echo "$t" | sed 's/.*m//' | sed 's/s//')
        total=$(echo "$min * 60 + $sec" | bc)
        py_times+=("$total")
    done
    py_median=$(printf '%s\n' "${py_times[@]}" | sort -n | sed -n '2p')

    # Calculate ratio
    if [ "$(echo "$c_median > 0" | bc)" -eq 1 ]; then
        ratio=$(echo "scale=1; $py_median / $c_median" | bc 2>/dev/null || echo "N/A")
    else
        ratio="N/A"
    fi

    printf "%-20s %10.3f %10.3f %7sx\n" "$name" "$c_median" "$py_median" "$ratio"
done
