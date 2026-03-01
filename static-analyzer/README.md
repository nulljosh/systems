# Build Your Own Static Analyzer

A Python source code static analyzer — lint-like tool that catches bugs without running code.

## Scope
- Parse Python source into AST
- Pluggable rule engine with 5 rules:
  - W001: Unused imports
  - W002: Unused variables
  - W003: Unreachable code (after return/break/continue/raise)
  - W004: Variable shadowing in nested scopes
  - W005: Unused function arguments
- Report generation (text + JSON via `--format json`)

## Learning Goals
- Abstract syntax trees and tree walking
- Python's `ast` module internals
- Control flow and data flow analysis
- Pattern matching on code structures
- Compiler frontend concepts without building a compiler

## Project Map

```svg
<svg viewBox="0 0 680 420" width="680" height="420" xmlns="http://www.w3.org/2000/svg" style="font-family:monospace;background:#f8fafc;border-radius:12px">
  <text x="340" y="28" text-anchor="middle" font-size="13" font-weight="bold" fill="#1e293b">static-analyzer — Python Source Code Analyzer</text>

  <!-- Root node -->
  <rect x="250" y="48" width="180" height="36" rx="8" fill="#0071e3"/>
  <text x="340" y="70" text-anchor="middle" font-size="11" fill="white" font-weight="bold">static-analyzer/</text>

  <!-- Dashed lines from root -->
  <line x1="300" y1="84" x2="160" y2="150" stroke="#94a3b8" stroke-width="1.5" stroke-dasharray="5,3"/>
  <line x1="340" y1="84" x2="340" y2="150" stroke="#94a3b8" stroke-width="1.5" stroke-dasharray="5,3"/>
  <line x1="380" y1="84" x2="520" y2="150" stroke="#94a3b8" stroke-width="1.5" stroke-dasharray="5,3"/>

  <!-- src/ folder -->
  <rect x="80" y="150" width="160" height="36" rx="8" fill="#6366f1"/>
  <text x="160" y="168" text-anchor="middle" font-size="11" fill="white" font-weight="bold">src/</text>
  <text x="160" y="180" text-anchor="middle" font-size="9" fill="#e0e7ff">core modules</text>

  <!-- README / setup -->
  <rect x="270" y="150" width="140" height="36" rx="8" fill="#86efac"/>
  <text x="340" y="168" text-anchor="middle" font-size="11" fill="#14532d">README.md</text>
  <text x="340" y="180" text-anchor="middle" font-size="9" fill="#64748b">documentation</text>

  <!-- Config/license -->
  <rect x="440" y="150" width="140" height="36" rx="8" fill="#86efac"/>
  <text x="510" y="168" text-anchor="middle" font-size="11" fill="#14532d">LICENSE / setup.md</text>
  <text x="510" y="180" text-anchor="middle" font-size="9" fill="#64748b">MIT 2026</text>

  <!-- src children - solid lines -->
  <line x1="100" y1="186" x2="100" y2="250" stroke="#6366f1" stroke-width="1.5"/>
  <line x1="160" y1="186" x2="160" y2="250" stroke="#6366f1" stroke-width="1.5"/>
  <line x1="220" y1="186" x2="220" y2="250" stroke="#6366f1" stroke-width="1.5"/>

  <!-- analyzer.py -->
  <rect x="40" y="250" width="130" height="40" rx="8" fill="#818cf8"/>
  <text x="105" y="268" text-anchor="middle" font-size="11" fill="white">analyzer.py</text>
  <text x="105" y="281" text-anchor="middle" font-size="9" fill="#e0e7ff">AST walker + rules</text>

  <!-- editor.py (unused in analyzer context, but exists) -->
  <rect x="185" y="250" width="130" height="40" rx="8" fill="#818cf8"/>
  <text x="250" y="268" text-anchor="middle" font-size="11" fill="white">editor.py</text>
  <text x="250" y="281" text-anchor="middle" font-size="9" fill="#e0e7ff">source editor util</text>

  <!-- Analysis Stages -->
  <text x="130" y="330" text-anchor="middle" font-size="11" font-weight="bold" fill="#1e293b">Analysis Pipeline</text>
  <line x1="105" y1="290" x2="105" y2="345" stroke="#818cf8" stroke-width="1.5"/>

  <rect x="30" y="345" width="150" height="28" rx="6" fill="#e0e7ff"/>
  <text x="105" y="363" text-anchor="middle" font-size="10" fill="#3730a3">Parse AST (ast module)</text>

  <line x1="105" y1="373" x2="105" y2="388" stroke="#818cf8" stroke-width="1.5"/>
  <rect x="30" y="388" width="150" height="28" rx="6" fill="#e0e7ff"/>
  <text x="105" y="406" text-anchor="middle" font-size="10" fill="#3730a3">Detect unused vars/imports</text>

  <!-- Report output -->
  <rect x="400" y="280" width="210" height="50" rx="8" fill="#fef3c7"/>
  <text x="505" y="303" text-anchor="middle" font-size="11" fill="#92400e" font-weight="bold">Report Output</text>
  <text x="505" y="318" text-anchor="middle" font-size="9" fill="#64748b">text + JSON formats</text>

  <line x1="315" y1="305" x2="400" y2="305" stroke="#94a3b8" stroke-width="1.5" stroke-dasharray="4,3"/>

  <!-- Checks list -->
  <rect x="400" y="345" width="210" height="60" rx="8" fill="#e0e7ff"/>
  <text x="505" y="363" text-anchor="middle" font-size="10" fill="#3730a3" font-weight="bold">Checks</text>
  <text x="505" y="378" text-anchor="middle" font-size="9" fill="#64748b">Unused vars / imports</text>
  <text x="505" y="391" text-anchor="middle" font-size="9" fill="#64748b">Unreachable code</text>
  <text x="505" y="404" text-anchor="middle" font-size="9" fill="#64748b">Type inference (basic)</text>
</svg>
```
