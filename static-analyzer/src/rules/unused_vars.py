"""W002: Unused variables."""
import ast
from src.analyzer import Rule, Issue, register_rule


class UnusedVarRule(Rule):
    """Detect variables assigned but never referenced."""

    def check(self, tree: ast.AST, source: str) -> list[Issue]:
        issues = []
        self._check_scope(tree, issues)
        return issues

    def _check_scope(self, node: ast.AST, issues: list[Issue]):
        """Check a scope (module, function, class) for unused variables."""
        assigned: dict[str, int] = {}
        referenced: set[str] = set()

        for child in ast.iter_child_nodes(node):
            if isinstance(child, (ast.FunctionDef, ast.AsyncFunctionDef, ast.ClassDef)):
                # Recurse into nested scopes
                self._check_scope(child, issues)
                referenced.add(child.name)  # function/class name is "used" in outer scope
                continue

            for subnode in ast.walk(child):
                if isinstance(subnode, ast.Name):
                    if isinstance(subnode.ctx, ast.Store):
                        if subnode.id not in assigned and not subnode.id.startswith("_"):
                            assigned[subnode.id] = subnode.lineno
                    elif isinstance(subnode.ctx, (ast.Load, ast.Del)):
                        referenced.add(subnode.id)

        for name, line in assigned.items():
            if name not in referenced:
                issues.append(Issue(line, 0, "W002", f"Unused variable: '{name}'"))


register_rule(UnusedVarRule())
