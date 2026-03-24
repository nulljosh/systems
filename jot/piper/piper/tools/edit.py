from pathlib import Path
from . import register
from .base import BaseTool


@register
class EditTool(BaseTool):
    name = "edit"
    description = "Find and replace text in a file"
    parameters = {
        "type": "object",
        "properties": {
            "file_path": {"type": "string", "description": "Absolute path to file"},
            "old_string": {"type": "string", "description": "Text to find"},
            "new_string": {"type": "string", "description": "Replacement text"},
            "replace_all": {"type": "boolean", "description": "Replace all occurrences", "default": False},
        },
        "required": ["file_path", "old_string", "new_string"],
    }

    def execute(self, arguments: dict) -> str:
        path = Path(arguments["file_path"])
        try:
            content = path.read_text()
            old = arguments["old_string"]
            new = arguments["new_string"]
            count = content.count(old)
            if count == 0:
                return "[old_string not found in file]"
            if arguments.get("replace_all", False):
                result = content.replace(old, new)
            else:
                if count > 1:
                    return f"[old_string appears {count} times -- not unique, use replace_all or provide more context]"
                result = content.replace(old, new, 1)
            path.write_text(result)
            return f"[replaced {count} occurrence(s)]"
        except FileNotFoundError:
            return f"[file not found: {path}]"
        except Exception as e:
            return f"[error editing file: {e}]"
