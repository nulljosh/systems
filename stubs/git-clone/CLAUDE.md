# git-clone

Git internals from scratch in C.

## Architecture

- `src/object.c` -- Blob, tree, commit objects (SHA-1 hashed, zlib compressed)
- `src/index.c` -- Staging area
- `src/refs.c` -- Branch and tag references
- `src/pack.c` -- Packfile format
- `src/diff.c` -- Myers diff algorithm
- `src/main.c` -- CLI dispatcher

## Build

```bash
make        # build
make test   # run tests
```

## Conventions

- C99, depends on zlib and openssl (SHA-1)
- .mygit/ directory structure mirrors .git/
