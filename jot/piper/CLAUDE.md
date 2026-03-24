# Piper -- CLI Coding Assistant

## Overview
CLI agent powered by Arthur LLM. Tool-use loop architecture: user input -> agent -> LLM -> tool calls -> execute -> loop -> return text.

## Structure
```
piper/
  cli.py          REPL (prompt_toolkit + rich)
  agent.py        Tool-use agent loop (max 10 iterations)
  config.py       Config from ~/.piper/config.json
  llm/
    base.py       BaseLLM, Response, ToolCall abstractions
    arthur.py     Arthur LLM stub (wire in when inference works)
  tools/
    __init__.py   ToolRegistry with @register decorator
    base.py       BaseTool abstract class
    bash.py       Shell execution (subprocess)
    read.py       File reader
    write.py      File writer
    edit.py       Find/replace
    grep.py       Ripgrep wrapper
    glob_tool.py  Pathlib glob
```

## Commands
```bash
pip install -e .        # Install
piper                   # Run REPL
python -c "from piper.tools import get_all_tools; print(get_all_tools())"  # Verify tools
```

## Dependencies
- prompt_toolkit (REPL)
- rich (output formatting)
- Python 3.9+

## Roadmap
1. Get Arthur inference producing coherent text
2. Implement tool-use output format (structured JSON generation)
3. System prompt with tool definitions
4. Conversation context management (token budgeting)
5. Streaming output
6. Permission system for tool execution
