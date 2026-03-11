# Heap Allocator

A custom dynamic memory allocator implemented in C, providing `malloc`/`free`/`realloc`-equivalent functionality over a fixed heap region.

## Overview

This allocator manages a contiguous region of memory bounded by `heap_start` and `heap_end`. It uses an **implicit free list** with **8-byte aligned blocks**, a **first-fit** placement policy, and **immediate bidirectional coalescing** on free.

## Block Format

Each block begins with an 8-byte header (`size_t`) followed by its payload:

```
Allocated block:  [ header | payload ... ]
Free block:       [ header | next_free_ptr | unused ... ]
```

**Header encoding:**
- Bits `[N:3]` — total block size (including header), always a multiple of 8
- Bit `[0]` — allocation status: `1` = allocated, `0` = free

Size and status are extracted with:
```c
size_t sz    = header & ~(size_t)7u;  // mask off lower 3 bits
size_t alloc = header &  (size_t)1u;  // check LSB
```

The minimum block size is **16 bytes** (8-byte header + 8-byte minimum payload).

## API

| Function | Description |
|---|---|
| `cpen212_init(heap_start, heap_end)` | Initializes the allocator over the given heap region. Returns a `heap_handle`. |
| `cpen212_alloc(heap_handle, nbytes)` | Allocates at least `nbytes` bytes. Returns an 8-byte aligned pointer, or `NULL`. |
| `cpen212_free(heap_handle, p)` | Frees a previously allocated block. Coalesces adjacent free blocks. |
| `cpen212_realloc(heap_handle, prev, nbytes)` | Resizes an allocation, preferring in-place expansion. Falls back to alloc+copy+free. |
| `cpen212_debug(heap_handle, op)` | Heap consistency checker (`op=0`) or custom debug operations (`op!=0`). |

All functions operate only within the heap region — no system memory allocation calls are used.

## Implementation Details

### Initialization
A `heap_state_t` struct is written at `heap_start` to store `heap_end`. The remainder of the heap is initialized as a single large free block.

### Allocation (`cpen212_alloc`)
Uses **first-fit**: scans from the first block and returns the first free block large enough. If the block has excess space (≥ 16 bytes remaining), it is split.

### Free (`cpen212_free`)
Marks the block as free, then coalesces:
1. **Forward coalescing** (`coalesce_fwd`) — merges with consecutive free blocks.
2. **Backward coalescing** (`coalesce_bkd`) — walks the heap from the start to find the predecessor block and merges if it is also free.

### Reallocation (`cpen212_realloc`)
Attempts in-place resize in three stages before falling back:
1. Shrink: if the block is already large enough, return as-is.
2. Expand forward: absorb adjacent free blocks.
3. Expand backward: absorb the preceding free block and shift the payload with `memmove`.
4. Fallback: `cpen212_alloc` + `memcpy` + `cpen212_free`.

## Build

```bash
make          # builds the cpen212alloc binary
make clean    # removes object files and binary
```

Requires `cpen212alloc.o` and `cpen212debug.o`.
