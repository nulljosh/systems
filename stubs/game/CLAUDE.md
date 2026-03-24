# game

Terminal game engine + roguelike in C.

## Architecture

- `src/main.c` -- Entry point, game loop
- `src/render.c` -- ncurses rendering
- `src/input.c` -- Keyboard input handling
- `src/entity.c` -- Entity-component system
- `src/map.c` -- Procedural map generation
- `src/physics.c` -- Collision detection

## Build

```bash
make
./game
```

## Conventions

- C99, depends on ncurses
- 60 FPS game loop with fixed timestep
