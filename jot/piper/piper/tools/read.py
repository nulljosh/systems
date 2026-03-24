from pathlib import Path
from . import register
from .base import BaseTool


@register
class ReadTool(BaseTool):
    name = "read"
    description = "Read contents of a file"
    parameters = {
        "type": "object",
        "properties": {
            "file_path": {"type": "string", "description": "Absolute path to file"},
            "offset": {"type": "integer", "description": "Line number to start from (1-based)"},
            "limit": {"type": "integer", "description": "Max lines to read"},
        },
        "required": ["file_path"],
    }

    def execute(self, arguments: dict) -> str:
        path = Path(arguments["file_path"])
        try:
            lines = path.read_text().splitlines(keepends=True)
            offset = arguments.get("offset", 1) - 1
            limit = arguments.get("limit", len(lines))
            selected = lines[offset : offset + limit]
            numbered = [f"{i + offset + 1}\t{line}" for i, line in enumerate(selected)]
            return "".join(numbered) or "[empty file]"
        except FileNotFoundError:
            return f"[file not found: {path}]"
        except Exception as e:
            return f"[error reading file: {e}]"
