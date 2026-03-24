from pathlib import Path
from . import register
from .base import BaseTool


@register
class WriteTool(BaseTool):
    name = "write"
    description = "Write content to a file (creates parent directories)"
    parameters = {
        "type": "object",
        "properties": {
            "file_path": {"type": "string", "description": "Absolute path to file"},
            "content": {"type": "string", "description": "Content to write"},
        },
        "required": ["file_path", "content"],
    }

    def execute(self, arguments: dict) -> str:
        path = Path(arguments["file_path"])
        try:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(arguments["content"])
            return f"[wrote {len(arguments['content'])} bytes to {path}]"
        except Exception as e:
            return f"[error writing file: {e}]"
