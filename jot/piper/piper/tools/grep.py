import subprocess
import shutil
from . import register
from .base import BaseTool


@register
class GrepTool(BaseTool):
    name = "grep"
    description = "Search file contents with regex pattern"
    parameters = {
        "type": "object",
        "properties": {
            "pattern": {"type": "string", "description": "Regex pattern to search for"},
            "path": {"type": "string", "description": "File or directory to search in", "default": "."},
            "glob": {"type": "string", "description": "File glob filter (e.g. '*.py')"},
            "case_insensitive": {"type": "boolean", "default": False},
        },
        "required": ["pattern"],
    }

    def execute(self, arguments: dict) -> str:
        pattern = arguments["pattern"]
        path = arguments.get("path", ".")
        rg = shutil.which("rg")
        if rg:
            cmd = [rg, "--no-heading", "-n", pattern, path]
            if arguments.get("glob"):
                cmd.extend(["--glob", arguments["glob"]])
            if arguments.get("case_insensitive"):
                cmd.append("-i")
        else:
            cmd = ["grep", "-rn", pattern, path]
            if arguments.get("case_insensitive"):
                cmd.append("-i")
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=15)
            output = result.stdout.strip()
            return output[:10000] if output else "[no matches]"
        except subprocess.TimeoutExpired:
            return "[search timed out]"
        except Exception as e:
            return f"[error: {e}]"
