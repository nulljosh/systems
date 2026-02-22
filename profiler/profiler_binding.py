"""
Python ctypes binding for the C profiler.
"""

import ctypes
import json
import os
import sys
from pathlib import Path
from typing import Optional, Dict, List
import time

# Load the compiled C library
def load_profiler_lib(lib_path: str):
    """Load the profiler C library."""
    try:
        lib = ctypes.CDLL(lib_path)
    except OSError as e:
        raise RuntimeError(f"Failed to load profiler library from {lib_path}: {e}")
    
    # Define function signatures
    lib.profiler_start.argtypes = []
    lib.profiler_start.restype = ctypes.c_int
    
    lib.profiler_stop.argtypes = []
    lib.profiler_stop.restype = ctypes.c_int
    
    lib.profiler_write_json.argtypes = [ctypes.c_char_p]
    lib.profiler_write_json.restype = ctypes.c_int
    
    lib.profiler_write_summary.argtypes = [ctypes.c_char_p]
    lib.profiler_write_summary.restype = ctypes.c_int
    
    return lib


class CPUProfiler:
    """High-level CPU profiler interface."""
    
    def __init__(self, lib_path: Optional[str] = None):
        """Initialize the profiler with the C library."""
        if lib_path is None:
            # Default to libprofiler.dylib in the same directory
            lib_path = str(Path(__file__).parent / "libprofiler.dylib")
        
        self.lib = load_profiler_lib(lib_path)
        self.is_profiling = False
        self.start_time = None
        self.end_time = None
    
    def start(self) -> bool:
        """Start profiling."""
        if self.is_profiling:
            print("ERROR: Profiler is already running")
            return False
        
        result = self.lib.profiler_start()
        if result == 0:
            self.is_profiling = True
            self.start_time = time.time()
            print("✓ Profiler started")
            return True
        else:
            print("ERROR: Failed to start profiler")
            return False
    
    def stop(self) -> bool:
        """Stop profiling."""
        if not self.is_profiling:
            print("ERROR: Profiler is not running")
            return False
        
        result = self.lib.profiler_stop()
        if result == 0:
            self.is_profiling = False
            self.end_time = time.time()
            print("✓ Profiler stopped")
            return True
        else:
            print("ERROR: Failed to stop profiler")
            return False
    
    def save_json(self, output_path: str) -> bool:
        """Save profiling data as JSON (flame graph format)."""
        output_path = os.path.expanduser(output_path)
        result = self.lib.profiler_write_json(output_path.encode())
        if result == 0:
            print(f"✓ Flame graph JSON saved to {output_path}")
            return True
        else:
            print(f"ERROR: Failed to save JSON to {output_path}")
            return False
    
    def save_summary(self, output_path: str) -> bool:
        """Save human-readable profiling summary."""
        output_path = os.path.expanduser(output_path)
        result = self.lib.profiler_write_summary(output_path.encode())
        if result == 0:
            print(f"✓ Summary saved to {output_path}")
            return True
        else:
            print(f"ERROR: Failed to save summary to {output_path}")
            return False
    
    def profile(self, func, *args, **kwargs):
        """Profile a single function call."""
        self.start()
        try:
            result = func(*args, **kwargs)
            return result
        finally:
            self.stop()


class FlameGraphConverter:
    """Convert profiler JSON to flame graph compatible format."""
    
    @staticmethod
    def load_profile_data(json_path: str) -> Dict:
        """Load profiler JSON output."""
        with open(json_path, 'r') as f:
            return json.load(f)
    
    @staticmethod
    def to_flamegraph_text(json_path: str, output_path: str) -> None:
        """
        Convert profiler JSON to flamegraph-compatible text format.
        Format: function1;function2;function3 count
        """
        data = FlameGraphConverter.load_profile_data(json_path)
        
        with open(output_path, 'w') as f:
            for stack in data.get('stacks', []):
                frames = stack.get('frames', [])
                count = stack.get('count', 0)
                
                # Build stack string (top to bottom)
                stack_str = ';'.join(
                    frame.get('symbol', 'unknown')
                    for frame in reversed(frames)
                )
                
                f.write(f"{stack_str} {count}\n")
        
        print(f"✓ Flame graph text format saved to {output_path}")
    
    @staticmethod
    def print_call_graph(json_path: str, max_depth: int = 5) -> None:
        """Print call graph from profiler data."""
        data = FlameGraphConverter.load_profile_data(json_path)
        
        print("\n=== CALL GRAPH (Top Paths) ===\n")
        
        stacks = sorted(
            data.get('stacks', []),
            key=lambda x: x.get('count', 0),
            reverse=True
        )
        
        for stack in stacks[:10]:
            frames = stack.get('frames', [])
            count = stack.get('count', 0)
            instr = stack.get('instructions', 0)
            
            print(f"Samples: {count:6d} | Instructions: {instr:8d}")
            
            for i, frame in enumerate(reversed(frames[:max_depth])):
                indent = "  " * i
                symbol = frame.get('symbol', 'unknown')
                addr = frame.get('addr', '')
                print(f"{indent}↓ {symbol} ({addr})")
            
            if len(frames) > max_depth:
                print(f"  ... {len(frames) - max_depth} more frames")
            print()


def main():
    """CLI interface for the profiler."""
    import sys
    
    if len(sys.argv) < 2:
        print("""
CPU Profiler - Usage:

  python profiler_binding.py profile <output_dir>
      Run with profiler attached (test program)
  
  python profiler_binding.py convert <json_file> <flamegraph_text>
      Convert JSON to flamegraph text format
  
  python profiler_binding.py analyze <json_file>
      Print call graph and hot paths
""")
        sys.exit(1)
    
    cmd = sys.argv[1]
    
    if cmd == "profile":
        output_dir = sys.argv[2] if len(sys.argv) > 2 else "."
        os.makedirs(output_dir, exist_ok=True)
        
        profiler = CPUProfiler()
        
        # Test function that does some work
        def test_work():
            total = 0
            for i in range(1000000):
                total += i * i
            return total
        
        profiler.profile(test_work)
        
        json_file = os.path.join(output_dir, "profile.json")
        summary_file = os.path.join(output_dir, "summary.txt")
        flamegraph_file = os.path.join(output_dir, "flamegraph.txt")
        
        profiler.save_json(json_file)
        profiler.save_summary(summary_file)
        
        FlameGraphConverter.to_flamegraph_text(json_file, flamegraph_file)
        FlameGraphConverter.print_call_graph(json_file)
        
        print(f"\nFiles created in {output_dir}:")
        print(f"  - profile.json (for analysis)")
        print(f"  - flamegraph.txt (for flamegraph.pl)")
        print(f"  - summary.txt (human-readable)")
    
    elif cmd == "convert":
        json_file = sys.argv[2]
        flamegraph_file = sys.argv[3]
        FlameGraphConverter.to_flamegraph_text(json_file, flamegraph_file)
    
    elif cmd == "analyze":
        json_file = sys.argv[2]
        FlameGraphConverter.print_call_graph(json_file)
    
    else:
        print(f"Unknown command: {cmd}")
        sys.exit(1)


if __name__ == "__main__":
    main()
