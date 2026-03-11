#ifndef __CPEN212COMMON_H__
#define __CPEN212COMMON_H__

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/*
8-Byte Aligned Heap Block Format

The heap is divided into different blocks begining with an 8-byte header
of type size_t, followed by its contents.

Header:
    - Least significant bit determines whether the block is allocated (1),
      or freed (0)
    - Remaining bits are store the overall block size including the header.

Extracting Fields:
    - To allocate data, header & 1, 1 is defined as a constant ALLOC_BIT
    - To take the size, header & ~7, ~7 is defined as a constant MASK_SIZE

Allocated Block:
    [header (size = 1)][payload]

Free Block:
    [header (size = 0)][next free pointer][unutilized space]

The pointer being returned to the user is a variable, block_start + 8.
The minimum block size is 16 bytes with the 8-byte payload and header. 
*/
typedef struct {
    void *end;
} heap_state_t;

static inline size_t block_size(char *block) {
    return (*(size_t *)block) & ~(size_t)7u;
}

static inline int block_allocated(char *block) {
    return (*(size_t *)block) & (size_t)1u;
}

#endif // __CPEN212COMMON_H__