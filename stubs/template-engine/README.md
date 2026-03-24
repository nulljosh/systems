<img src="icon.svg" width="80">

# template-engine

![version](https://img.shields.io/badge/version-v0.1.0-blue)

Jinja-style template compiler built from scratch in Python. Supports variables, conditionals, loops, filters, and template inheritance.

## Features

- `{{ variable }}` interpolation
- `{% if %}` / `{% for %}` control flow
- `{{ value | filter }}` filter pipeline
- Template inheritance with `{% extends %}` / `{% block %}`

## Usage

```bash
pip install -r requirements.txt
python -m template_engine render template.html data.json
```

## Test

```bash
python -m pytest tests/
```

## License

MIT 2026 Joshua Trommel
