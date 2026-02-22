#!/usr/bin/env python3
"""
Test suite for the CPU profiler.
"""

import sys
import json
import os
import tempfile
from pathlib import Path

# Add current directory to path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from profiler_binding import CPUProfiler, FlameGraphConverter
from profiler_cli import ProfileReport


def test_basic_profiling():
    """Test basic profiling functionality."""
    print("\n✓ Test 1: Basic profiling")
    
    profiler = CPUProfiler()
    
    # Start profiling
    assert profiler.start(), "Failed to start profiler"
    
    # Do some work
    total = 0
    for i in range(100000):
        total += i * i
    
    # Stop profiling
    assert profiler.stop(), "Failed to stop profiler"
    
    print(f"  - Profiler completed: {total}")
    return True


def test_json_output():
    """Test JSON output generation."""
    print("\n✓ Test 2: JSON output")
    
    with tempfile.TemporaryDirectory() as tmpdir:
        json_file = os.path.join(tmpdir, "profile.json")
        
        profiler = CPUProfiler()
        profiler.start()
        
        # Do work
        for i in range(100000):
            _ = i ** 2
        
        profiler.stop()
        profiler.save_json(json_file)
        
        # Verify file exists and is valid JSON
        assert os.path.exists(json_file), "JSON file not created"
        
        with open(json_file) as f:
            data = json.load(f)
        
        assert "version" in data, "Missing version in JSON"
        assert "stacks" in data, "Missing stacks in JSON"
        assert data["total_samples"] > 0, "No samples captured"
        
        print(f"  - Generated valid JSON with {len(data['stacks'])} unique stacks")
        print(f"  - Total samples: {data['total_samples']}")
        return True


def test_summary_output():
    """Test summary text output."""
    print("\n✓ Test 3: Summary output")
    
    with tempfile.TemporaryDirectory() as tmpdir:
        summary_file = os.path.join(tmpdir, "summary.txt")
        
        profiler = CPUProfiler()
        profiler.start()
        
        # Do work
        for i in range(100000):
            _ = i + i
        
        profiler.stop()
        profiler.save_summary(summary_file)
        
        # Verify file exists
        assert os.path.exists(summary_file), "Summary file not created"
        
        with open(summary_file) as f:
            content = f.read()
        
        assert "SUMMARY" in content, "Missing SUMMARY header"
        assert "Samples" in content, "Missing Samples section"
        
        print(f"  - Generated summary ({len(content)} bytes)")
        return True


def test_flamegraph_conversion():
    """Test flame graph format conversion."""
    print("\n✓ Test 4: Flame graph conversion")
    
    with tempfile.TemporaryDirectory() as tmpdir:
        json_file = os.path.join(tmpdir, "profile.json")
        fg_file = os.path.join(tmpdir, "flamegraph.txt")
        
        profiler = CPUProfiler()
        profiler.start()
        
        for i in range(100000):
            _ = i % 7
        
        profiler.stop()
        profiler.save_json(json_file)
        
        # Convert to flamegraph format
        FlameGraphConverter.to_flamegraph_text(json_file, fg_file)
        
        assert os.path.exists(fg_file), "Flame graph file not created"
        
        with open(fg_file) as f:
            lines = f.readlines()
        
        assert len(lines) > 0, "Flame graph file is empty"
        
        # Verify format (should be "stack count")
        for line in lines[:3]:
            parts = line.strip().split()
            assert len(parts) >= 2, f"Invalid flame graph format: {line}"
            # Last part should be a number
            assert parts[-1].isdigit(), f"Invalid count in: {line}"
        
        print(f"  - Generated flame graph ({len(lines)} stacks)")
        return True


def test_profile_report():
    """Test profile report generation."""
    print("\n✓ Test 5: Profile report")
    
    with tempfile.TemporaryDirectory() as tmpdir:
        json_file = os.path.join(tmpdir, "profile.json")
        
        profiler = CPUProfiler()
        profiler.start()
        
        for i in range(100000):
            _ = i // 2
        
        profiler.stop()
        profiler.save_json(json_file)
        
        # Create report and verify it can read the data
        report = ProfileReport(json_file)
        
        assert report.data, "Failed to load profile data"
        assert "stacks" in report.data, "Missing stacks in report"
        
        print(f"  - Report loaded with {len(report.data['stacks'])} stacks")
        return True


def test_multiple_profiles():
    """Test running multiple profiles sequentially."""
    print("\n✓ Test 6: Multiple profiles")
    
    with tempfile.TemporaryDirectory() as tmpdir:
        for idx in range(3):
            profiler = CPUProfiler()
            profiler.start()
            
            for i in range(50000):
                _ = i * (idx + 1)
            
            profiler.stop()
            
            json_file = os.path.join(tmpdir, f"profile_{idx}.json")
            profiler.save_json(json_file)
            
            assert os.path.exists(json_file), f"Profile {idx} not created"
        
        # Verify all files exist
        files = list(Path(tmpdir).glob("profile_*.json"))
        assert len(files) == 3, f"Expected 3 profiles, got {len(files)}"
        
        print(f"  - Generated {len(files)} profiles successfully")
        return True


def run_all_tests():
    """Run all tests."""
    print("\n" + "="*60)
    print("CPU PROFILER TEST SUITE".center(60))
    print("="*60)
    
    tests = [
        test_basic_profiling,
        test_json_output,
        test_summary_output,
        test_flamegraph_conversion,
        test_profile_report,
        test_multiple_profiles,
    ]
    
    passed = 0
    failed = 0
    
    for test_func in tests:
        try:
            test_func()
            passed += 1
        except AssertionError as e:
            print(f"\n✗ {test_func.__name__} FAILED: {e}")
            failed += 1
        except Exception as e:
            print(f"\n✗ {test_func.__name__} ERROR: {e}")
            failed += 1
    
    print("\n" + "="*60)
    print(f"Results: {passed} passed, {failed} failed")
    print("="*60 + "\n")
    
    return failed == 0


if __name__ == "__main__":
    success = run_all_tests()
    sys.exit(0 if success else 1)
