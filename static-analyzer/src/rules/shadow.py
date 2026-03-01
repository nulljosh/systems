"""W004: Variable shadowing in nested scopes."""
import ast
from src.analyzer import Rule, Issue, register_rule


class ShadowRule(Rule):
    """Detect variables that shadow names from enclosing scopes."""

    def check(self, tree: ast.AST, source: str) -> list[Issue]:
        issues = []
        self._check_scope(tree, set(), issues)
        return issues

    def _check_scope(self, node: ast.AST, outer_names: set[str], issues: list[Issue]):
        """Check scope for shadowed names."""
        local_names: set[str] = set()

        for child in ast.iter_child_nodes(node):
            if isinstance(child, (ast.FunctionDef, ast.AsyncFunctionDef)):
                # Collect arg names
                func_names = set()
                for arg in child.args.args:
                    if arg.arg in outer_names or arg.arg in local_names:
                        issues.append(
                            Issue(child.lineno, 0, "W004",
                                  f"Parameter '{arg.arg}' shadows outer scope variable")
                        )
                    func_names.add(arg.arg)
                # Recurse with combined scope
                combined = outer_names | local_names | func_names
                self._check_scope(child, combined, issues)
                local_names.add(child.name)
            elif isinstance(child, ast.ClassDef):
                self._check_scope(child, outer_names | local_names, issues)
                local_names.add(child.name)
            else:
                for subnode in ast.walk(child):
                    if isinstance(subnode, ast.Name) and isinstance(subnode.ctx, ast.Store):
                        name = subnode.id
                        if name in outer_names and name not in local_names:
                            issues.append(
                                Issue(subnode.lineno, 0, "W004",
                                      f"Variable '{name}' shadows outer scope variable")
                            )
                        local_names.add(name)


register_rule(ShadowRule())
