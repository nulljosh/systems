# Build Your Own Container Runtime

A minimal Linux container runtime — namespaces, cgroups, and filesystem isolation.

## Scope
- Linux namespaces (PID, mount, network, UTS) via `clone()`
- Cgroup v2 resource limits (CPU weight, memory max)
- Filesystem isolation with `pivot_root` + proc/sysfs mounts
- OCI tarball unpacking (tar + gzip)
- CLI: `mycontainer run <image> <cmd> [--memory N] [--cpu-weight N] [--hostname S]`
- Network setup with veth pairs (stub, stretch goal)

Compiles on macOS, runs on Linux only. Tests skip gracefully on macOS.

## Learning Goals
- Linux namespaces and what "isolation" really means
- Cgroups v2 for resource control
- Mount namespaces and overlay filesystems
- How Docker/containerd actually work
- Systems programming with unsafe Rust + C FFI
- Security boundaries and capabilities

## Project Map

```svg
<svg viewBox="0 0 680 420" width="680" height="420" xmlns="http://www.w3.org/2000/svg" style="font-family:monospace;background:#f8fafc;border-radius:12px">
  <rect width="680" height="420" fill="#f8fafc" rx="12"/>
  <text x="340" y="28" text-anchor="middle" font-size="13" font-weight="bold" fill="#1e293b">container-runtime — minimal Linux container runtime in Rust</text>

  <!-- Root node -->
  <rect x="270" y="45" width="140" height="34" rx="8" fill="#0071e3"/>
  <text x="340" y="67" text-anchor="middle" font-size="11" fill="white">container-runtime/</text>

  <!-- Dashed lines from root -->
  <line x1="340" y1="79" x2="160" y2="140" stroke="#94a3b8" stroke-width="1.2" stroke-dasharray="4,3"/>
  <line x1="340" y1="79" x2="340" y2="140" stroke="#94a3b8" stroke-width="1.2" stroke-dasharray="4,3"/>
  <line x1="340" y1="79" x2="510" y2="140" stroke="#94a3b8" stroke-width="1.2" stroke-dasharray="4,3"/>

  <!-- src/ folder -->
  <rect x="110" y="140" width="100" height="34" rx="6" fill="#6366f1"/>
  <text x="160" y="162" text-anchor="middle" font-size="11" fill="white">src/</text>

  <!-- Cargo.toml -->
  <rect x="290" y="140" width="100" height="34" rx="6" fill="#e0f2fe"/>
  <text x="340" y="157" text-anchor="middle" font-size="11" fill="#0369a1">Cargo.toml</text>
  <text x="340" y="168" text-anchor="middle" font-size="9" fill="#64748b">workspace manifest</text>

  <!-- README / docs -->
  <rect x="460" y="140" width="100" height="34" rx="6" fill="#dcfce7"/>
  <text x="510" y="157" text-anchor="middle" font-size="11" fill="#166534">README.md</text>
  <text x="510" y="168" text-anchor="middle" font-size="9" fill="#64748b">scope + goals</text>

  <!-- src children: main.rs -->
  <line x1="160" y1="174" x2="160" y2="230" stroke="#94a3b8" stroke-width="1"/>
  <rect x="110" y="230" width="100" height="34" rx="6" fill="#e0e7ff"/>
  <text x="160" y="247" text-anchor="middle" font-size="11" fill="#3730a3">main.rs</text>
  <text x="160" y="258" text-anchor="middle" font-size="9" fill="#64748b">CLI + runtime core</text>

  <!-- Subsystem boxes -->
  <line x1="160" y1="264" x2="90" y2="320" stroke="#94a3b8" stroke-width="1"/>
  <line x1="160" y1="264" x2="160" y2="320" stroke="#94a3b8" stroke-width="1"/>
  <line x1="160" y1="264" x2="230" y2="320" stroke="#94a3b8" stroke-width="1"/>
  <line x1="160" y1="264" x2="340" y2="320" stroke="#94a3b8" stroke-width="1"/>
  <line x1="160" y1="264" x2="450" y2="320" stroke="#94a3b8" stroke-width="1"/>
  <line x1="160" y1="264" x2="560" y2="320" stroke="#94a3b8" stroke-width="1"/>

  <rect x="40" y="320" width="100" height="40" rx="6" fill="#e0e7ff"/>
  <text x="90" y="336" text-anchor="middle" font-size="10" fill="#3730a3">namespaces</text>
  <text x="90" y="348" text-anchor="middle" font-size="9" fill="#64748b">PID/mount/net/UTS</text>
  <text x="90" y="358" text-anchor="middle" font-size="9" fill="#64748b">isolation</text>

  <rect x="110" y="320" width="100" height="40" rx="6" fill="#e0e7ff"/>
  <text x="160" y="336" text-anchor="middle" font-size="10" fill="#3730a3">cgroups</text>
  <text x="160" y="348" text-anchor="middle" font-size="9" fill="#64748b">CPU + memory</text>
  <text x="160" y="358" text-anchor="middle" font-size="9" fill="#64748b">resource limits</text>

  <rect x="180" y="320" width="100" height="40" rx="6" fill="#e0e7ff"/>
  <text x="230" y="336" text-anchor="middle" font-size="10" fill="#3730a3">filesystem</text>
  <text x="230" y="348" text-anchor="middle" font-size="9" fill="#64748b">chroot / pivot_root</text>
  <text x="230" y="358" text-anchor="middle" font-size="9" fill="#64748b">overlay FS</text>

  <rect x="290" y="320" width="100" height="40" rx="6" fill="#e0e7ff"/>
  <text x="340" y="336" text-anchor="middle" font-size="10" fill="#3730a3">image</text>
  <text x="340" y="348" text-anchor="middle" font-size="9" fill="#64748b">OCI / tarball</text>
  <text x="340" y="358" text-anchor="middle" font-size="9" fill="#64748b">unpacking</text>

  <rect x="400" y="320" width="100" height="40" rx="6" fill="#e0e7ff"/>
  <text x="450" y="336" text-anchor="middle" font-size="10" fill="#3730a3">network</text>
  <text x="450" y="348" text-anchor="middle" font-size="9" fill="#64748b">veth pairs</text>
  <text x="450" y="358" text-anchor="middle" font-size="9" fill="#64748b">(stretch goal)</text>

  <rect x="510" y="320" width="100" height="40" rx="6" fill="#e0e7ff"/>
  <text x="560" y="336" text-anchor="middle" font-size="10" fill="#3730a3">CLI</text>
  <text x="560" y="348" text-anchor="middle" font-size="9" fill="#64748b">mycontainer run</text>
  <text x="560" y="358" text-anchor="middle" font-size="9" fill="#64748b">image cmd</text>

  <!-- Tech labels -->
  <text x="340" y="400" text-anchor="middle" font-size="9" fill="#64748b">Rust · Linux namespaces · cgroups v2 · OCI image spec</text>
</svg>
```
