#include <stdlib.h>
#include <string.h>
#include "cpen212alloc.h"
#include "cpen212common.h"

static void coalesce_fwd(char *block, char *heap_end) {

    size_t sz = block_size(block); // get block size

    while (1) {
        char *next = block + sz; // find the next block 

        if (next >= heap_end) break;

        size_t next_sz = block_size(next);

        if (next_sz == 0) break;
        if (block_allocated(next)) break;

        sz += next_sz;
        *(size_t *)block = sz | (size_t)0u; // free
    }
}

static char *coalesce_bkd(char *heap_start, char *heap_end, char *block) {

    // Take heapstate as a struct
    char *curr = (char *)heap_start + sizeof(heap_state_t);

    while (curr < heap_end) {
        size_t sz = block_size(curr); // get the block size

        if (sz == 0) return NULL;

        char *next = curr + sz; // find next block

        if (next == block) return curr;
        if (next > block) return NULL;

        curr = next;
    }
    return NULL;
}

void *cpen212_init(void *heap_start, void *heap_end) {

    // Use the heapstate as a struct
    heap_state_t *st = (heap_state_t *)heap_start;
    st->end = heap_end;

    // Determine the address of the first block
    char *first = (char *)heap_start + sizeof(heap_state_t);

    // Compute remaining bytes
    size_t bytes = (size_t)((char*)heap_end - first);
    // Align the bytes to a mutiple of 8
    bytes &= ~(size_t)7u;

    // Write first block header with size and free flag
    *(size_t *)first = (bytes | (size_t)0u);

    return st;
}

// this alloc is broken: it blithely allocates past the end of the heap
void *cpen212_alloc(void *heap_handle, size_t nbytes) {
   
    // Return NULL if no bytes are allocated
    if (nbytes == 0) return NULL;

    // Use the heapstate as a struct
    heap_state_t *st = (heap_state_t *)heap_handle;
    char *heap_end = (char *)st->end;

    // Align the payload to a multiple of 8
    size_t payload_sz = (nbytes + 7u) & ~(size_t)7u;

    // Total required blocks
    size_t required = payload_sz + sizeof(size_t);

    // Take the first block
    char *first_curr = (char *)heap_handle + sizeof(heap_state_t);

    while (first_curr < heap_end) {
        size_t header = *(size_t *)first_curr; // Read the header
        size_t first_curr_sz = header & ~(size_t)7u; // Take the size
        size_t first_curr_alloc = header & (size_t)1u; // Extract alloc flag

        if (first_curr_sz == 0) return NULL;

        if ((first_curr_sz & (size_t)7u) != 0) return NULL;
        if (first_curr_sz < sizeof(size_t)) return NULL;
        if ((first_curr + first_curr_sz) > heap_end) return NULL;

        if ((first_curr_alloc == 0) && (first_curr_sz >= required)) {
            size_t remaining = first_curr_sz - required;

            // Form a block for the remaining bytes
            if (remaining >= (sizeof(size_t) + 8u)) {
                char *rest = first_curr + required;
                
                *(size_t *)rest = (remaining | (size_t)0u); // free remaining block
                *(size_t *)first_curr = (required | (size_t)1u); // allocated block
            } else {
                *(size_t *)first_curr = (first_curr_sz | (size_t)1u); // allocate whole block
            }

            return (first_curr + sizeof(size_t)); //return pointer to payload after header
        }

        first_curr += first_curr_sz; // jump pointer to next block
    }

    return NULL;
}

void cpen212_free(void *s, void *p) {

    // (void)s;

    if (p == NULL) return;

    heap_state_t *st = (heap_state_t *)s;
    char *heap_end = (char *)st->end;

    void *block = (char *)p - sizeof(size_t); // compute block start

    size_t sz = block_size(block);
    *(size_t *)block = sz | (size_t)0u;

    // coalesce fowards
    coalesce_fwd(block, heap_end);

    // coalesce backwords
    char *prev = coalesce_bkd(s, heap_end, block);

    if (prev && !block_allocated(prev)) {
        size_t new_sz = block_size(prev) + block_size(block);
        *(size_t *)prev = new_sz | (size_t)0u;

        coalesce_fwd(prev, heap_end); // coalesce fowards here
    }
}

void *cpen212_realloc(void *s, void *prev, size_t nbytes) {
    if (prev == NULL) {
        return cpen212_alloc(s, nbytes); // allocate memory
    }
    if (nbytes == 0) {
        cpen212_free(s, prev); // free memory
        return NULL;
    }

    heap_state_t *st = (heap_state_t *)s;
    char *heap_end = (char *)st->end;

    // forwards
    char *prev_block = (char *)prev - sizeof(size_t); // compute start of old header
    size_t prev_header = *(size_t *)prev_block; // read previous header
    size_t prev_sz = prev_header & ~(size_t)7u; // take total size of previous block
    size_t prev_payload_sz = prev_sz - sizeof(size_t); // take old payload (total size - header)

    size_t new_payload_sz = (nbytes + 7u) & ~(size_t)7u;
    size_t new_required = new_payload_sz + sizeof(size_t);

    if (new_required <= prev_sz) return prev;

    size_t merged = prev_sz;
    char *next = prev_block + prev_sz;

    while (next < heap_end) {
        size_t next_header = *(size_t *)next;
        size_t next_sz = next_header & ~(size_t)7u;
        size_t next_alloc = next_header & (size_t)1u;

        if (next_sz == 0) break;
        if (next_alloc != 0) break;

        merged += next_sz;
        if (merged >= new_required) break;

        next += next_sz;
    }

    if (merged >= new_required) {
        size_t remainder = merged - new_required;

        if (remainder >= (sizeof(size_t) + 8u)) {
            char *rest = prev_block + new_required;
            *(size_t *)rest = (remainder | (size_t)0u);

            *(size_t *)prev_block = (new_required | (size_t)1u);
        } else {
            *(size_t *)prev_block = (merged | (size_t)1u);
        }

        return prev;
    }

    // backwards
    char *prev_blk = coalesce_bkd(s, heap_end, prev_block);
    if (prev_blk != NULL) {
        size_t prev_head = *(size_t *)prev_blk; // find previous header
        size_t p_sz = prev_head & ~(size_t)7u; // get previous size
        size_t prev_alloc = prev_head & (size_t)1u; // allocate data

        if (prev_alloc == 0) {
            size_t total = p_sz + prev_sz;
            char *next2 = prev_block + prev_sz;
            while (next2 < heap_end) {
                size_t next_head = *(size_t *)next2;
                size_t n_sz = next_head & ~(size_t)7u;
                size_t next_alloc = next_head & (size_t)1u;
                if (n_sz == 0 || next_alloc) break;
                total += n_sz;
                if (total >= new_required) break;
                next2 += n_sz;
            }

            if (total >= new_required) {
                char *new_block = prev_blk;
                void *new_payload = new_block + sizeof(size_t);

                size_t move_sz = (nbytes < new_payload_sz) ? nbytes : new_payload_sz;
                memmove(new_payload, prev, move_sz); // copies bytes from one block to another

                size_t remainder = total - new_required;

                if (remainder >= (sizeof(size_t) + 8u)) {
                    char *rest = new_block + new_required;
                    *(size_t *)rest = (remainder | (size_t)0u);
                    *(size_t *)new_block = (new_required | (size_t)1u);
                } else {
                    *(size_t *)new_block = (total | (size_t)1u);
                }

                return new_payload;
            }
        }
    }

    void *newp = cpen212_alloc(s, nbytes); // allocate data if forwards/backwards implementations fail
    if (newp == NULL) return NULL;

    size_t copy_sz = (nbytes < prev_payload_sz) ? nbytes : prev_payload_sz; // take small value
    memcpy(newp, prev, copy_sz); // copy old payload to new payload

    cpen212_free(s, prev); // free the old block
    return newp;
}