![Piper](icon.svg)

# Piper

![version](https://img.shields.io/badge/version-v0.1.0-blue)

CLI coding assistant powered by Arthur LLM. A from-scratch alternative to Claude Code, running on a custom transformer model.

## Architecture

![Architecture](architecture.svg)

## Install

```bash
cd ~/Documents/Code/piper
pip install -e .
```

## Usage

```bash
piper
```

Type at the `piper>` prompt. `/quit` to exit.

## Tools

| Tool | Description |
|------|-------------|
| bash | Shell command execution |
| read | File reader with offset/limit |
| write | File writer |
| edit | Find/replace in files |
| grep | Regex search (ripgrep) |
| glob | File pattern matching |

## Status

Scaffolding complete. Arthur LLM integration pending -- model needs coherent text generation before tool-use can be wired in.

## License

MIT 2026 Joshua Trommel
