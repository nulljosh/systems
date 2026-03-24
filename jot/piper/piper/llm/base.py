from __future__ import annotations
from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from typing import Optional


@dataclass
class ToolCall:
    name: str
    arguments: dict


@dataclass
class Response:
    text: str = ""
    tool_calls: list = field(default_factory=list)


class BaseLLM(ABC):
    @abstractmethod
    def generate(self, messages: list[dict], tools: Optional[list[dict]] = None) -> Response:
        ...
