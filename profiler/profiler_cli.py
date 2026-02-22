#!/usr/bin/env python3
"""
CPU Profiler CLI - Complete profiling tool for CPU, memory, and instructions.
Supports decorators, context managers, and standalone profiling.
"""

import os
import sys
import json
import time
import subprocess
from pathlib import Path
from contextlib import contextmanager
from typing import Optional, Callable, Any
from functools import wraps
from datetime import datetime

from profiler_binding import CPUProfiler, FlameGraphConverter


class ProfilerConfig:
    """Configuration for the profiler."""
    
    def __init__(self, output_dir: str = "./profiler_output"):
        self.output_dir = Path(output_dir).expanduser()
        self.output_dir.mkdir(parents=True, exist_ok=True)
        self.timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    
    def get_filename(self, suffix: str, ext: str = ".json") -> Path:
        """Get output filename with timestamp."""
        return self.output_dir / f"profile_{self.timestamp}_{suffix}{ext}"


class ProfileReport:
    """Generate and display profiling reports."""
    
    def __init__(self, json_file: str):
        self.json_file = json_file
        self.data = self._load_data()
    
    def _load_data(self) -> dict:
        """Load JSON profile data."""
        try:
            with open(self.json_file, 'r') as f:
                return json.load(f)
        except Exception as e:
            print(f"ERROR: Could not load {self.json_file}: {e}")
            return {}
    
    def print_summary(self) -> None:
        """Print execution summary."""
        data = self.data
        
        print("\n" + "="*60)
        print("PROFILER SUMMARY".center(60))
        print("="*60 + "\n")
        
        print(f"Elapsed Time:      {data.get('elapsed_seconds', 0):.2f}s")
        print(f"Total Samples:     {data.get('total_samples', 0):,}")
        print(f"Unique Stacks:     {len(data.get('stacks', []))}")
        print()
    
    def print_hot_paths(self, top_n: int = 15, max_depth: int = 6) -> None:
        """Print hottest execution paths."""
        stacks = sorted(
            self.data.get('stacks', []),
            key=lambda x: x.get('count', 0),
            reverse=True
        )
        
        print("="*60)
        print("HOT PATHS (Top %d)" % min(top_n, len(stacks)))
        print("="*60 + "\n")
        
        for idx, stack in enumerate(stacks[:top_n], 1):
            frames = stack.get('frames', [])
            count = stack.get('count', 0)
            instr = stack.get('instructions', 0)
            mem = stack.get('memory_delta', 0)
            
            pct = (100.0 * count) / max(1, self.data.get('total_samples', 1))
            
            print(f"{idx:2d}. [{pct:5.1f}%] {count:6d} samples | "
                  f"{instr:8d} instr | {mem/1024:8.1f}KB")
            
            # Print call stack
            for depth, frame in enumerate(reversed(frames[:max_depth])):
                symbol = frame.get('symbol', '?')
                addr = frame.get('addr', '')
                indent = "    " * depth
                print(f"{indent}↓ {symbol}")
            
            if len(frames) > max_depth:
                print("    " * max_depth + f"... +{len(frames) - max_depth} frames")
            print()
    
    def print_memory_stats(self, top_n: int = 10) -> None:
        """Print memory allocation hotspots."""
        stacks = sorted(
            self.data.get('stacks', []),
            key=lambda x: x.get('memory_delta', 0),
            reverse=True
        )
        
        print("\n" + "="*60)
        print("MEMORY HOTSPOTS (Top %d)" % min(top_n, len(stacks)))
        print("="*60 + "\n")
        
        for idx, stack in enumerate(stacks[:top_n], 1):
            frames = stack.get('frames', [])
            mem = stack.get('memory_delta', 0)
            
            if mem == 0:
                continue
            
            top_frame = frames[0] if frames else {}
            symbol = top_frame.get('symbol', '?')
            
            print(f"{idx:2d}. {mem/1024:10.1f}KB - {symbol}")
    
    def print_instruction_stats(self, top_n: int = 10) -> None:
        """Print instruction hotspots."""
        stacks = sorted(
            self.data.get('stacks', []),
            key=lambda x: x.get('instructions', 0),
            reverse=True
        )
        
        print("\n" + "="*60)
        print("INSTRUCTION HOTSPOTS (Top %d)" % min(top_n, len(stacks)))
        print("="*60 + "\n")
        
        for idx, stack in enumerate(stacks[:top_n], 1):
            frames = stack.get('frames', [])
            instr = stack.get('instructions', 0)
            
            if instr == 0:
                continue
            
            top_frame = frames[0] if frames else {}
            symbol = top_frame.get('symbol', '?')
            
            print(f"{idx:2d}. {instr:12,} instr - {symbol}")


def profile_function(output_dir: Optional[str] = None) -> Callable:
    """Decorator to profile a function."""
    
    def decorator(func: Callable) -> Callable:
        @wraps(func)
        def wrapper(*args, **kwargs) -> Any:
            config = ProfilerConfig(output_dir or "./profiler_output")
            profiler = CPUProfiler()
            
            print(f"\n📊 Profiling {func.__name__}...")
            
            profiler.start()
            try:
                result = func(*args, **kwargs)
            finally:
                profiler.stop()
            
            # Save outputs
            json_file = str(config.get_filename(func.__name__))
            txt_file = str(config.get_filename(func.__name__, ".txt"))
            fg_file = str(config.get_filename(func.__name__, "_flamegraph.txt"))
            
            profiler.save_json(json_file)
            profiler.save_summary(txt_file)
            FlameGraphConverter.to_flamegraph_text(json_file, fg_file)
            
            # Show report
            report = ProfileReport(json_file)
            report.print_summary()
            report.print_hot_paths(top_n=10)
            report.print_memory_stats()
            report.print_instruction_stats()
            
            print(f"\n✓ Profile saved to {config.output_dir}")
            return result
        
        return wrapper
    
    return decorator


@contextmanager
def profiler_context(name: str = "profile", output_dir: Optional[str] = None):
    """Context manager for profiling a block of code."""
    config = ProfilerConfig(output_dir or "./profiler_output")
    profiler = CPUProfiler()
    
    print(f"\n📊 Profiling '{name}'...")
    profiler.start()
    
    try:
        yield profiler
    finally:
        profiler.stop()
        
        # Save outputs
        json_file = str(config.get_filename(name))
        txt_file = str(config.get_filename(name, ".txt"))
        fg_file = str(config.get_filename(name, "_flamegraph.txt"))
        
        profiler.save_json(json_file)
        profiler.save_summary(txt_file)
        FlameGraphConverter.to_flamegraph_text(json_file, fg_file)
        
        # Show report
        report = ProfileReport(json_file)
        report.print_summary()
        report.print_hot_paths()
        report.print_memory_stats()
        report.print_instruction_stats()
        
        print(f"\n✓ Profile saved to {config.output_dir}")


def main():
    """CLI interface."""
    
    if len(sys.argv) < 2:
        print("""
CPU Profiler v1.0
Usage: python profiler_cli.py <command> [options]

Commands:
  run <script>              Profile a Python script
  interactive              Interactive profiling session
  analyze <json_file>       Analyze a profile JSON file
  flamegraph <json_file>    Generate flame graph text format
  help                     Show this help message

Examples:
  python profiler_cli.py run myprogram.py
  python profiler_cli.py analyze profile_20240101_120000_myprogram.json
  python profiler_cli.py flamegraph profile.json > flame.txt
""")
        return
    
    cmd = sys.argv[1]
    
    if cmd == "run" and len(sys.argv) > 2:
        script_file = sys.argv[2]
        output_dir = sys.argv[3] if len(sys.argv) > 3 else "./profiler_output"
        
        config = ProfilerConfig(output_dir)
        profiler = CPUProfiler()
        
        print(f"\n📊 Profiling {script_file}...")
        profiler.start()
        
        try:
            # Run the script
            with open(script_file) as f:
                code = compile(f.read(), script_file, 'exec')
                exec(code)
        finally:
            profiler.stop()
        
        # Save and analyze
        json_file = str(config.get_filename(Path(script_file).stem))
        txt_file = str(config.get_filename(Path(script_file).stem, ".txt"))
        fg_file = str(config.get_filename(Path(script_file).stem, "_flamegraph.txt"))
        
        profiler.save_json(json_file)
        profiler.save_summary(txt_file)
        FlameGraphConverter.to_flamegraph_text(json_file, fg_file)
        
        report = ProfileReport(json_file)
        report.print_summary()
        report.print_hot_paths()
        
        print(f"\n✓ Profiles saved to {config.output_dir}")
    
    elif cmd == "analyze" and len(sys.argv) > 2:
        json_file = sys.argv[2]
        report = ProfileReport(json_file)
        report.print_summary()
        report.print_hot_paths()
        report.print_memory_stats()
        report.print_instruction_stats()
    
    elif cmd == "flamegraph" and len(sys.argv) > 2:
        json_file = sys.argv[2]
        output_file = sys.argv[3] if len(sys.argv) > 3 else "flamegraph.txt"
        FlameGraphConverter.to_flamegraph_text(json_file, output_file)
    
    elif cmd == "interactive":
        print("\n🎯 Interactive Profiler\n")
        profiler = CPUProfiler()
        
        while True:
            cmd = input("profile> ").strip()
            
            if cmd in ["exit", "quit"]:
                break
            elif cmd == "start":
                profiler.start()
            elif cmd == "stop":
                profiler.stop()
            elif cmd.startswith("save "):
                output_dir = cmd.split(" ", 1)[1]
                config = ProfilerConfig(output_dir)
                json_file = str(config.get_filename("interactive"))
                txt_file = str(config.get_filename("interactive", ".txt"))
                profiler.save_json(json_file)
                profiler.save_summary(txt_file)
                print(f"✓ Saved to {config.output_dir}")
            elif cmd == "help":
                print("""
Commands:
  start           - Start profiling
  stop            - Stop profiling
  save <dir>      - Save profiling data
  exit            - Exit
""")
            else:
                print("Unknown command. Type 'help' for usage.")
    
    else:
        print(f"Unknown command: {cmd}")
        sys.exit(1)


if __name__ == "__main__":
    main()
