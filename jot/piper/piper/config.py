import json
from dataclasses import dataclass, field
from pathlib import Path


CONFIG_PATH = Path.home() / ".piper" / "config.json"


@dataclass
class Config:
    model_path: str = ""
    arthur_endpoint: str = "http://localhost:5001"
    max_tokens: int = 2048
    temperature: float = 0.7
    tools_enabled: list = field(default_factory=lambda: ["bash", "read", "write", "edit", "grep", "glob"])

    @classmethod
    def load(cls):
        if CONFIG_PATH.exists():
            with open(CONFIG_PATH) as f:
                data = json.load(f)
            return cls(**{k: v for k, v in data.items() if k in cls.__dataclass_fields__})
        return cls()
