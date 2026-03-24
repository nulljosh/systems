# physics-engine

2D rigid body physics engine in C.

## Architecture

- `src/main.c` -- Demo entry point
- `src/world.c` -- Physics world, timestep management
- `src/body.c` -- Rigid body (position, velocity, mass, shape)
- `src/collision.c` -- SAT collision detection
- `src/solver.c` -- Impulse-based constraint solver
- `src/broadphase.c` -- Spatial partitioning (grid)

## Build

```bash
make
make test
```

## Conventions

- C99, no external deps
- Fixed timestep (1/60s), semi-implicit Euler integration
