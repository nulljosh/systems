# Build Your Own Text Editor

A terminal-based text editor built with Python and curses, inspired by nano/vim.

## Scope
- Terminal UI with curses for rendering
- File open, edit, save workflow
- Line numbers, status bar, command bar
- Search and replace
- Syntax highlighting (stretch goal)
- Undo/redo stack

## Learning Goals
- Terminal I/O and escape sequences
- Buffer/gap buffer data structures
- Event loop and keyboard input handling
- File I/O and encoding
- MVC architecture in a real application

## Project Map

```svg
<svg viewBox="0 0 680 420" width="680" height="420" xmlns="http://www.w3.org/2000/svg" style="font-family:monospace;background:#f8fafc;border-radius:12px">
  <text x="340" y="28" text-anchor="middle" font-size="13" font-weight="bold" fill="#1e293b">text-editor — Terminal Text Editor (Python + curses)</text>

  <!-- Root node -->
  <rect x="255" y="48" width="170" height="36" rx="8" fill="#0071e3"/>
  <text x="340" y="70" text-anchor="middle" font-size="11" fill="white" font-weight="bold">text-editor/</text>

  <!-- Dashed lines from root -->
  <line x1="300" y1="84" x2="160" y2="150" stroke="#94a3b8" stroke-width="1.5" stroke-dasharray="5,3"/>
  <line x1="340" y1="84" x2="340" y2="150" stroke="#94a3b8" stroke-width="1.5" stroke-dasharray="5,3"/>
  <line x1="380" y1="84" x2="520" y2="150" stroke="#94a3b8" stroke-width="1.5" stroke-dasharray="5,3"/>

  <!-- src/ folder -->
  <rect x="80" y="150" width="160" height="36" rx="8" fill="#6366f1"/>
  <text x="160" y="168" text-anchor="middle" font-size="11" fill="white" font-weight="bold">src/</text>
  <text x="160" y="180" text-anchor="middle" font-size="9" fill="#e0e7ff">core modules</text>

  <!-- Docs -->
  <rect x="270" y="150" width="140" height="36" rx="8" fill="#86efac"/>
  <text x="340" y="168" text-anchor="middle" font-size="11" fill="#14532d">README.md</text>
  <text x="340" y="180" text-anchor="middle" font-size="9" fill="#64748b">setup + usage docs</text>

  <!-- License/setup -->
  <rect x="440" y="150" width="140" height="36" rx="8" fill="#86efac"/>
  <text x="510" y="168" text-anchor="middle" font-size="11" fill="#14532d">LICENSE / setup.md</text>
  <text x="510" y="180" text-anchor="middle" font-size="9" fill="#64748b">MIT 2026</text>

  <!-- src children -->
  <line x1="110" y1="186" x2="110" y2="250" stroke="#6366f1" stroke-width="1.5"/>
  <line x1="210" y1="186" x2="210" y2="250" stroke="#6366f1" stroke-width="1.5"/>

  <!-- editor.py -->
  <rect x="30" y="250" width="155" height="40" rx="8" fill="#818cf8"/>
  <text x="108" y="268" text-anchor="middle" font-size="11" fill="white">editor.py</text>
  <text x="108" y="281" text-anchor="middle" font-size="9" fill="#e0e7ff">main editor loop</text>

  <!-- analyzer.py (helper) -->
  <rect x="135" y="250" width="155" height="40" rx="8" fill="#818cf8"/>
  <text x="213" y="268" text-anchor="middle" font-size="11" fill="white">analyzer.py</text>
  <text x="213" y="281" text-anchor="middle" font-size="9" fill="#e0e7ff">syntax helper</text>

  <!-- Features section -->
  <text x="160" y="325" text-anchor="middle" font-size="11" font-weight="bold" fill="#1e293b">Editor Features</text>
  <line x1="108" y1="290" x2="108" y2="340" stroke="#818cf8" stroke-width="1.5"/>

  <rect x="20" y="340" width="180" height="28" rx="6" fill="#e0e7ff"/>
  <text x="110" y="358" text-anchor="middle" font-size="10" fill="#3730a3">curses terminal rendering</text>

  <line x1="108" y1="368" x2="108" y2="383" stroke="#818cf8" stroke-width="1.5"/>
  <rect x="20" y="383" width="180" height="28" rx="6" fill="#e0e7ff"/>
  <text x="110" y="401" text-anchor="middle" font-size="10" fill="#3730a3">open / edit / save files</text>

  <!-- Right column features -->
  <rect x="380" y="250" width="270" height="155" rx="8" fill="#f1f5f9"/>
  <text x="515" y="272" text-anchor="middle" font-size="11" font-weight="bold" fill="#1e293b">Feature Set</text>
  <text x="515" y="292" text-anchor="middle" font-size="10" fill="#475569">Line numbers + status bar</text>
  <text x="515" y="310" text-anchor="middle" font-size="10" fill="#475569">Search and replace</text>
  <text x="515" y="328" text-anchor="middle" font-size="10" fill="#475569">Undo / redo stack</text>
  <text x="515" y="346" text-anchor="middle" font-size="10" fill="#475569">Syntax highlighting (stretch)</text>
  <text x="515" y="364" text-anchor="middle" font-size="10" fill="#475569">Gap buffer data structure</text>
  <text x="515" y="382" text-anchor="middle" font-size="10" fill="#475569">Keyboard event loop</text>
  <text x="515" y="398" text-anchor="middle" font-size="10" fill="#475569">MVC architecture</text>
</svg>
```
