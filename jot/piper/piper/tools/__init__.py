from __future__ import annotations
from typing import Optional
from .base import BaseTool

_registry: dict[str, type[BaseTool]] = {}
_instances: dict[str, BaseTool] = {}


def register(cls):
    _registry[cls.name] = cls
    return cls


def _get_instance(name: str) -> BaseTool:
    if name not in _instances:
        _instances[name] = _registry[name]()
    return _instances[name]


def get_all_tools() -> list[BaseTool]:
    return [_get_instance(name) for name in _registry]


def get_tool(name: str) -> Optional[BaseTool]:
    if name not in _registry:
        return None
    return _get_instance(name)


# Import all tools to trigger registration
from . import bash, read, write, edit, grep, glob_tool  # noqa: F401, E402
