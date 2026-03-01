"""W003: Unreachable code."""
import ast
from src.analyzer import Rule, Issue, register_rule


class UnreachableCodeRule(Rule):
    """Detect code after return, break, continue, or raise."""

    def check(self, tree: ast.AST, source: str) -> list[Issue]:
        issues = []
        self._check_body(tree, issues)
        return issues

    def _check_body(self, node: ast.AST, issues: list[Issue]):
        """Walk all nodes that contain statement bodies."""
        for child in ast.walk(node):
            # Check nodes with a body list
            for attr in ("body", "orelse", "finalbody", "handlers"):
                stmts = getattr(child, attr, None)
                if not isinstance(stmts, list):
                    continue
                self._check_stmts(stmts, issues)

    def _check_stmts(self, stmts: list, issues: list[Issue]):
        """Check a list of statements for unreachable code."""
        terminal_types = (ast.Return, ast.Break, ast.Continue, ast.Raise)
        for i, stmt in enumerate(stmts):
            if isinstance(stmt, terminal_types) and i < len(stmts) - 1:
                next_stmt = stmts[i + 1]
                issues.append(
                    Issue(next_stmt.lineno, 0, "W003",
                          f"Unreachable code after {type(stmt).__name__.lower()}")
                )
                break  # only report first unreachable statement


register_rule(UnreachableCodeRule())
