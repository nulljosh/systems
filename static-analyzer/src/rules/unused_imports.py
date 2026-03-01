"""W001: Unused imports."""
import ast
from src.analyzer import Rule, Issue, register_rule


class UnusedImportRule(Rule):
    """Detect imports that are never referenced."""

    def check(self, tree: ast.AST, source: str) -> list[Issue]:
        imports: dict[str, int] = {}
        used: set[str] = set()

        for node in ast.walk(tree):
            if isinstance(node, ast.Import):
                for alias in node.names:
                    name = alias.asname or alias.name
                    imports[name] = node.lineno
            elif isinstance(node, ast.ImportFrom):
                for alias in node.names:
                    name = alias.asname or alias.name
                    imports[name] = node.lineno
            elif isinstance(node, ast.Name):
                used.add(node.id)

        return [
            Issue(line, 0, "W001", f"Unused import: '{name}'")
            for name, line in imports.items()
            if name not in used
        ]


register_rule(UnusedImportRule())
