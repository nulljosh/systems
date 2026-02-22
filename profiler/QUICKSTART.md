# Quick Start Guide - CPU Profiler

Get profiling in 5 minutes!

## Setup (30 seconds)

```bash
cd ~/Documents/Code/profiler
make              # Build the C extension
```

That's it! You're ready to profile.

## 1. Profile a Function (1 minute)

```python
from profiler_cli import profile_function

@profile_function()
def my_slow_function():
    total = 0
    for i in range(1000000):
        total += i * i
    return total

result = my_slow_function()
```

Run it:
```bash
python3 myscript.py
```

Output shows CPU hotspots, memory usage, and instructions automatically.

## 2. Profile a Code Block (1 minute)

```python
from profiler_cli import profiler_context

def main():
    with profiler_context("data_processing"):
        data = load_data()
        process(data)
    
    with profiler_context("sorting"):
        results = sorted(data)
    
    return results

main()
```

## 3. Profile from Command Line (1 minute)

```bash
# Profile a script
python3 profiler_cli.py run myscript.py

# Output appears in ./profiler_output/
ls profiler_output/
```

Files generated:
- `profile_*.json` - Full data (for tools)
- `summary.txt` - Human-readable summary
- `flamegraph.txt` - Input for flamegraph.pl

## 4. Analyze Results (1 minute)

```bash
# View summary
cat profiler_output/summary.txt

# View hot paths
python3 profiler_cli.py analyze profiler_output/profile_*.json

# Generate flame graph (requires flamegraph.pl)
cat profiler_output/flamegraph.txt | flamegraph.pl > graph.svg
open graph.svg
```

## Reading the Output

### Sample Run
```
📊 Profiling 'data_processing'...
✓ Profiler stopped

============================================================
PROFILER SUMMARY
============================================================

Elapsed Time:      2.34s
Total Samples:     2340
Unique Stacks:     145

============================================================
HOT PATHS (Top 15)
============================================================

 1. [42.3%]    990 samples | 50000 instr |    128.5KB
    ↓ process_item
    ↓ map_function
    ↓ main

 2. [15.2%]    355 samples | 18000 instr |     45.2KB
    ↓ malloc
    ↓ allocate_buffer
    ↓ main
    
[... more paths ...]
```

### What to look for:

**High-percentage paths** = CPU bottlenecks  
**High instruction count** = Heavy computation  
**High memory** = Allocation hotspots  

## Common Patterns

### Find the slowest function
```bash
python3 profiler_cli.py analyze profile.json | head -20
```

### Compare before/after optimization
```bash
# Before
python3 profiler_cli.py run slow_version.py
cp profiler_output/profile_*.json before.json

# After
python3 profiler_cli.py run fast_version.py
cp profiler_output/profile_*.json after.json

# Compare
python3 profiler_cli.py analyze before.json | head -10
python3 profiler_cli.py analyze after.json | head -10
```

### Profile with custom output dir
```python
@profile_function(output_dir="./results/experiment1")
def my_function():
    pass
```

### Multiple profiles
```python
from profiler_binding import CPUProfiler

for trial in range(3):
    profiler = CPUProfiler()
    profiler.start()
    # ... code ...
    profiler.stop()
    profiler.save_json(f"trial_{trial}.json")
```

## Troubleshooting

**"libprofiler.dylib not found"**
```bash
cd ~/Documents/Code/profiler
make clean && make
```

**"No samples captured"**
- Your function is too fast (< 1ms)
- Try running it in a loop or with more data

**"Symbol resolution fails"**
- Functions show as "??" instead of names
- Ensure debug symbols are available
- Use `nm` to check:
  ```bash
  nm libprofiler.dylib | grep -i function_name
  ```

## Next Steps

- Read `README.md` for detailed documentation
- Check `example_usage.py` for more patterns
- Try `test_profiler.py` to verify your setup

## Tips & Tricks

1. **Profile in Release Mode**: Build with optimizations (-O2)
2. **Increase Sample Rate**: Edit SAMPLE_INTERVAL_US in profiler.c
3. **Filter Output**: Use grep on flame graph text
4. **Track Memory Leaks**: Look for increasing memory_delta values
5. **Profile Different Data Sizes**: See how scalability impacts performance

---

**Questions?** Check README.md or look at the examples!
