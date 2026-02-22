"""Static analyzer skeleton — finds unused imports and variables."""
import ast
import sys
from dataclasses import dataclass


@dataclass
class Issue:
    line: int
    col: int
    code: str
    message: str


class UnusedImportChecker(ast.NodeVisitor):
    """Detect imports that are never referenced."""
    def __init__(self):
        self.imports: dict[str, int] = {}
        self.used: set[str] = set()

    def visit_Import(self, node):
        for alias in node.names:
            name = alias.asname or alias.name
            self.imports[name] = node.lineno

    def visit_ImportFrom(self, node):
        for alias in node.names:
            name = alias.asname or alias.name
            self.imports[name] = node.lineno

    def visit_Name(self, node):
        self.used.add(node.id)

    def get_issues(self) -> list[Issue]:
        return [
            Issue(line, 0, "W001", f"Unused import: '{name}'")
            for name, line in self.imports.items()
            if name not in self.used
        ]


def analyze(source: str, filename: str = "<stdin>") -> list[Issue]:
    tree = ast.parse(source, filename)
    checker = UnusedImportChecker()
    checker.visit(tree)
    return checker.get_issues()


def main():
    if len(sys.argv) < 2:
        print("Usage: analyzer.py <file.py>")
        sys.exit(1)
    path = sys.argv[1]
    with open(path) as f:
        source = f.read()
    issues = analyze(source, path)
    for issue in issues:
        print(f"{path}:{issue.line}:{issue.col} [{issue.code}] {issue.message}")
    if not issues:
        print("No issues found.")


if __name__ == "__main__":
    main()
