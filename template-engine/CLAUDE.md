# template-engine

Jinja-style template compiler in Python.

## Architecture

- `template_engine/lexer.py` -- Tokenizer
- `template_engine/parser.py` -- AST parser
- `template_engine/compiler.py` -- Compiles AST to Python code
- `template_engine/environment.py` -- Template loading and rendering API
- `template_engine/__main__.py` -- CLI entry point

## Dev

```bash
cd ~/Documents/Code/template-engine
python -m pytest tests/
python -m template_engine render template.html data.json
```

## Conventions

- Python 3.9+, no external deps for core engine
- pytest for tests
