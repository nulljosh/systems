# Profiler - Claude Notes

## Overview
CPU profiler for Python + C. Sampling-based (1ms intervals). 730 LOC. Outputs JSON, text, flame graph format.

## Build
```bash
cd ~/Documents/Code/profiler
make   # builds libprofiler.dylib (macOS) or libprofiler.so (Linux)
```

## Usage
```bash
# CLI
python profiler_cli.py run myscript.py
python profiler_cli.py analyze profile_*.json
python profiler_cli.py flamegraph profile.json output.txt

# Decorator
from profiler_cli import profile_function

@profile_function(output_dir="./results")
def my_function():
    pass
```

## Key Files
- `profiler.c` / `profiler.h` — C extension (sampling, perf counters)
- `profiler_binding.py` — Python bindings
- `profiler_cli.py` — CLI and decorator API

## Status
Done/stable. 730 LOC.
