#!/usr/bin/env python3
"""
Examples of using the CPU profiler in different ways.
"""

from profiler_cli import profile_function, profiler_context, ProfilerConfig, ProfileReport


# ============================================================================
# Example 1: Using the decorator
# ============================================================================

@profile_function(output_dir="./profiler_output")
def fibonacci(n):
    """Fibonacci function to profile."""
    if n <= 1:
        return n
    return fibonacci(n - 1) + fibonacci(n - 2)


# ============================================================================
# Example 2: Using the context manager
# ============================================================================

def context_manager_example():
    """Example using context manager."""
    
    with profiler_context("sorting_example"):
        # Simulate some work
        data = list(range(100000, 0, -1))
        data.sort()
        result = sum(data)
    
    print(f"Sorting result: {result}")


# ============================================================================
# Example 3: Manual profiling
# ============================================================================

def manual_profiling_example():
    """Example with manual start/stop."""
    from profiler_binding import CPUProfiler, FlameGraphConverter
    
    config = ProfilerConfig("./profiler_output")
    profiler = CPUProfiler()
    
    print("\n📊 Manual profiling example")
    
    # Start profiling
    profiler.start()
    
    # Do work
    total = 0
    for i in range(1000000):
        total += i % 2
    
    # Stop profiling
    profiler.stop()
    
    # Save outputs
    json_file = str(config.get_filename("manual_example"))
    txt_file = str(config.get_filename("manual_example", ".txt"))
    fg_file = str(config.get_filename("manual_example", "_flamegraph.txt"))
    
    profiler.save_json(json_file)
    profiler.save_summary(txt_file)
    FlameGraphConverter.to_flamegraph_text(json_file, fg_file)
    
    # Analyze results
    report = ProfileReport(json_file)
    report.print_summary()
    report.print_hot_paths()
    
    print(f"\nResult: {total}")


# ============================================================================
# Example 4: Inline profiling with multiple segments
# ============================================================================

def multi_segment_profiling():
    """Profile multiple segments of code separately."""
    from profiler_binding import CPUProfiler, FlameGraphConverter
    
    config = ProfilerConfig("./profiler_output")
    
    # Segment 1: Fast operations
    print("\n📊 Segment 1: Fast operations")
    profiler1 = CPUProfiler()
    profiler1.start()
    
    for i in range(100000):
        x = i * 2
    
    profiler1.stop()
    
    json_file1 = str(config.get_filename("segment1"))
    profiler1.save_json(json_file1)
    report1 = ProfileReport(json_file1)
    report1.print_summary()
    
    # Segment 2: Slower operations
    print("\n📊 Segment 2: Slower operations")
    profiler2 = CPUProfiler()
    profiler2.start()
    
    for i in range(1000000):
        x = i ** 2
    
    profiler2.stop()
    
    json_file2 = str(config.get_filename("segment2"))
    profiler2.save_json(json_file2)
    report2 = ProfileReport(json_file2)
    report2.print_summary()


if __name__ == "__main__":
    import sys
    
    if len(sys.argv) > 1:
        example = sys.argv[1]
    else:
        print("""
CPU Profiler Examples

Run: python example_usage.py <example>

Examples:
  1 - Decorator-based profiling (fibonacci)
  2 - Context manager profiling (sorting)
  3 - Manual profiling (simple loop)
  4 - Multi-segment profiling
  all - Run all examples
""")
        sys.exit(1)
    
    if example == "1":
        fibonacci(20)
    elif example == "2":
        context_manager_example()
    elif example == "3":
        manual_profiling_example()
    elif example == "4":
        multi_segment_profiling()
    elif example == "all":
        print("Running all examples...")
        fibonacci(18)
        context_manager_example()
        manual_profiling_example()
        multi_segment_profiling()
    else:
        print(f"Unknown example: {example}")
        sys.exit(1)
