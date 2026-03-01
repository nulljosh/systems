"""Static analyzer -- pluggable rule engine for Python source code."""
import ast
import json
import sys
from dataclasses import dataclass, asdict
from abc import ABC, abstractmethod


@dataclass
class Issue:
    line: int
    col: int
    code: str
    message: str


class Rule(ABC):
    """Base class for all analyzer rules."""
    @abstractmethod
    def check(self, tree: ast.AST, source: str) -> list[Issue]:
        ...


# Rule registry
_rules: list[Rule] = []


def register_rule(rule: Rule):
    """Register a rule instance with the analyzer."""
    _rules.append(rule)


def get_rules() -> list[Rule]:
    """Return all registered rules."""
    return list(_rules)


def analyze(source: str, filename: str = "<stdin>") -> list[Issue]:
    """Run all registered rules on the given source code."""
    tree = ast.parse(source, filename)
    issues = []
    for rule in _rules:
        issues.extend(rule.check(tree, source))
    issues.sort(key=lambda i: (i.line, i.col, i.code))
    return issues


def main():
    import argparse
    parser = argparse.ArgumentParser(description="Static analyzer for Python")
    parser.add_argument("file", help="Python file to analyze")
    parser.add_argument("--format", choices=["text", "json"], default="text",
                       help="Output format (default: text)")
    args = parser.parse_args()

    # Import rules to trigger registration
    from src.rules import unused_imports, unused_vars, unreachable, shadow, unused_args

    with open(args.file) as f:
        source = f.read()

    issues = analyze(source, args.file)

    if args.format == "json":
        print(json.dumps([asdict(i) for i in issues], indent=2))
    else:
        for issue in issues:
            print(f"{args.file}:{issue.line}:{issue.col} [{issue.code}] {issue.message}")
        if not issues:
            print("No issues found.")


if __name__ == "__main__":
    main()
