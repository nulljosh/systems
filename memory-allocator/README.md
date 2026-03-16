# memory-allocator

Custom malloc/free implementation in C. First-fit allocation with block splitting and coalescing.

## Build

```bash
make
make test
```

## Design

- `sbrk()`-based heap extension
- Free list with first-fit strategy
- Block splitting on alloc, coalescing on free
- Thread-safe with mutex locks
