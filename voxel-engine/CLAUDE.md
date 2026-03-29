# voxel-engine

Voxel renderer in Rust.

## Architecture

- `src/main.rs` -- Window creation, main loop
- `src/chunk.rs` -- 16x16x16 voxel chunks
- `src/mesh.rs` -- Greedy meshing algorithm
- `src/render.rs` -- OpenGL rendering pipeline
- `src/world.rs` -- Chunk manager, loading/unloading
- `src/terrain.rs` -- Procedural generation (Perlin noise)
- `src/camera.rs` -- FPS camera

## Dev

```bash
cargo build
cargo test
```

## Conventions

- Rust 2021 edition
- Raw OpenGL via gl crate (no engine)
- Greedy meshing for performance
