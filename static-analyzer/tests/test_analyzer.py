"""Tests for the static analyzer -- all 5 rules."""
import pytest
import sys
import os

# Add project root to path
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from src.analyzer import analyze, _rules, register_rule

# Import rules to trigger registration (only once)
from src.rules import unused_imports, unused_vars, unreachable, shadow, unused_args

FIXTURES = os.path.join(os.path.dirname(__file__), "fixtures")


def _analyze_fixture(name: str):
    path = os.path.join(FIXTURES, name)
    with open(path) as f:
        source = f.read()
    return analyze(source, path)


class TestW001UnusedImports:
    def test_detects_unused_import(self):
        issues = _analyze_fixture("unused_imports.py")
        w001 = [i for i in issues if i.code == "W001"]
        assert len(w001) == 1
        assert "os" in w001[0].message

    def test_no_false_positive_on_used_import(self):
        issues = _analyze_fixture("unused_imports.py")
        w001 = [i for i in issues if i.code == "W001"]
        assert not any("sys" in i.message for i in w001)

    def test_clean_file_no_w001(self):
        issues = _analyze_fixture("clean.py")
        w001 = [i for i in issues if i.code == "W001"]
        assert len(w001) == 0


class TestW002UnusedVars:
    def test_detects_unused_var(self):
        issues = _analyze_fixture("unused_vars.py")
        w002 = [i for i in issues if i.code == "W002"]
        assert len(w002) == 1
        assert "y" in w002[0].message

    def test_no_false_positive_on_used_var(self):
        issues = _analyze_fixture("unused_vars.py")
        w002 = [i for i in issues if i.code == "W002"]
        assert not any("x" in i.message for i in w002)


class TestW003Unreachable:
    def test_detects_after_return(self):
        issues = _analyze_fixture("unreachable.py")
        w003 = [i for i in issues if i.code == "W003"]
        assert any("return" in i.message for i in w003)

    def test_detects_after_break(self):
        issues = _analyze_fixture("unreachable.py")
        w003 = [i for i in issues if i.code == "W003"]
        assert any("break" in i.message for i in w003)


class TestW004Shadow:
    def test_detects_shadow(self):
        issues = _analyze_fixture("shadow.py")
        w004 = [i for i in issues if i.code == "W004"]
        assert len(w004) >= 1
        assert "x" in w004[0].message


class TestW005UnusedArgs:
    def test_detects_unused_arg(self):
        issues = _analyze_fixture("unused_args.py")
        w005 = [i for i in issues if i.code == "W005"]
        assert len(w005) == 1
        assert "greeting" in w005[0].message

    def test_no_false_positive_on_used_arg(self):
        issues = _analyze_fixture("unused_args.py")
        w005 = [i for i in issues if i.code == "W005"]
        assert not any("name" in i.message for i in w005)


class TestCleanFile:
    def test_no_issues(self):
        issues = _analyze_fixture("clean.py")
        assert len(issues) == 0
