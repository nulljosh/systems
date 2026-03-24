from pathlib import Path
from . import register
from .base import BaseTool


@register
class GlobTool(BaseTool):
    name = "glob"
    description = "Find files matching a glob pattern"
    parameters = {
        "type": "object",
        "properties": {
            "pattern": {"type": "string", "description": "Glob pattern (e.g. '**/*.py')"},
            "path": {"type": "string", "description": "Root directory to search from", "default": "."},
        },
        "required": ["pattern"],
    }

    def execute(self, arguments: dict) -> str:
        root = Path(arguments.get("path", "."))
        pattern = arguments["pattern"]
        try:
            matches = sorted(root.glob(pattern))
            if not matches:
                return "[no matches]"
            result = "\n".join(str(m) for m in matches[:200])
            if len(matches) > 200:
                result += f"\n[... and {len(matches) - 200} more]"
            return result
        except Exception as e:
            return f"[error: {e}]"
