# Build Your Own Shell

A Unix shell built from scratch in C99 -- command parsing, execution, piping, redirects, and job control.

## Build & Run

```bash
make
./shell
```

## Features
- REPL with prompt (`~/` home substitution)
- Command parsing with single/double quote support
- Environment variable expansion (`$VAR`, inside double quotes too)
- Pipes (`ls | grep .c | wc -l`)
- Redirects (`>`, `>>`, `<`)
- Builtins: `cd`, `exit`, `export`, `history`, `fg`, `bg`
- Background jobs (`sleep 10 &`)
- Signal handling (SIGINT/SIGTSTP ignored in parent, forwarded to children)

## Architecture

```
Input -> Tokenizer -> Pipeline Builder -> Executor -> Output
              |               |                |
         quotes, $VAR    redirects         fork/exec/pipe
         expansion       parsed here       dup2 for I/O
```

Single file: `src/shell.c` (~470 lines C99, zero dependencies beyond POSIX).

## Tests

```bash
make && bash tests/test_shell.sh
```

## Project Map

```svg
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 680 340" width="680" height="340" style="font-family:monospace;background:#f8fafc;border-radius:12px">
  <rect width="680" height="340" rx="12" fill="#f8fafc"/>
  <text x="340" y="28" text-anchor="middle" font-size="13" font-weight="bold" fill="#1e293b">shell -- Unix Shell from Scratch (C99)</text>
  <rect x="265" y="44" width="150" height="32" rx="6" fill="#0071e3" opacity="0.9"/>
  <text x="340" y="65" text-anchor="middle" font-size="11" fill="white" font-weight="bold">shell/ (root)</text>

  <rect x="60" y="118" width="80" height="28" rx="5" fill="#6366f1" opacity="0.85"/>
  <text x="100" y="136" text-anchor="middle" font-size="10" fill="white">src/</text>
  <rect x="160" y="118" width="80" height="28" rx="5" fill="#6366f1" opacity="0.85"/>
  <text x="200" y="136" text-anchor="middle" font-size="10" fill="white">tests/</text>
  <rect x="260" y="118" width="80" height="28" rx="5" fill="#94a3b8" opacity="0.5"/>
  <text x="300" y="136" text-anchor="middle" font-size="10" fill="#475569">python/</text>
  <rect x="360" y="118" width="90" height="28" rx="5" fill="#dcfce7" stroke="#86efac" stroke-width="1"/>
  <text x="405" y="136" text-anchor="middle" font-size="10" fill="#166534">Makefile</text>
  <rect x="470" y="118" width="90" height="28" rx="5" fill="#dcfce7" stroke="#86efac" stroke-width="1"/>
  <text x="515" y="136" text-anchor="middle" font-size="10" fill="#166534">README.md</text>

  <line x1="340" y1="76" x2="100" y2="118" stroke="#94a3b8" stroke-width="1" stroke-dasharray="4,2"/>
  <line x1="340" y1="76" x2="200" y2="118" stroke="#94a3b8" stroke-width="1" stroke-dasharray="4,2"/>
  <line x1="340" y1="76" x2="300" y2="118" stroke="#94a3b8" stroke-width="1" stroke-dasharray="4,2"/>
  <line x1="340" y1="76" x2="405" y2="118" stroke="#94a3b8" stroke-width="1" stroke-dasharray="4,2"/>
  <line x1="340" y1="76" x2="515" y2="118" stroke="#94a3b8" stroke-width="1" stroke-dasharray="4,2"/>

  <rect x="50" y="190" width="100" height="28" rx="5" fill="#e0e7ff" stroke="#818cf8" stroke-width="1"/>
  <text x="100" y="208" text-anchor="middle" font-size="10" fill="#3730a3">shell.c</text>
  <rect x="170" y="190" width="100" height="28" rx="5" fill="#e0e7ff" stroke="#818cf8" stroke-width="1"/>
  <text x="220" y="208" text-anchor="middle" font-size="10" fill="#3730a3">test_shell.sh</text>

  <line x1="100" y1="146" x2="100" y2="190" stroke="#818cf8" stroke-width="1.5"/>
  <line x1="200" y1="146" x2="220" y2="190" stroke="#818cf8" stroke-width="1.5"/>

  <rect x="80" y="270" width="520" height="28" rx="5" fill="#fef3c7" stroke="#fbbf24" stroke-width="1"/>
  <text x="340" y="288" text-anchor="middle" font-size="10" fill="#92400e">C99 -- tokenizer, pipeline builder, fork/exec, pipes, redirects, job control</text>
</svg>
```
