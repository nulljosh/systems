"""W005: Unused function arguments."""
import ast
from src.analyzer import Rule, Issue, register_rule


class UnusedArgRule(Rule):
    """Detect function arguments that are never referenced in the body."""

    def check(self, tree: ast.AST, source: str) -> list[Issue]:
        issues = []
        for node in ast.walk(tree):
            if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
                self._check_function(node, issues)
        return issues

    def _check_function(self, node: ast.FunctionDef, issues: list[Issue]):
        """Check a single function for unused arguments."""
        # Skip methods with 'self' or 'cls' as first arg
        args = [a.arg for a in node.args.args]
        skip = set()
        if args and args[0] in ("self", "cls"):
            skip.add(args[0])

        # Also skip args starting with _
        for a in args:
            if a.startswith("_"):
                skip.add(a)

        # Collect all referenced names in the function body
        referenced: set[str] = set()
        for child in ast.walk(node):
            if isinstance(child, ast.Name) and isinstance(child.ctx, ast.Load):
                referenced.add(child.id)

        for arg in args:
            if arg not in skip and arg not in referenced:
                issues.append(
                    Issue(node.lineno, 0, "W005",
                          f"Unused argument: '{arg}' in function '{node.name}'")
                )


register_rule(UnusedArgRule())
