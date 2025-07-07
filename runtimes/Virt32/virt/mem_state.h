#ifndef MEM_STATE_H
#define MEM_STATE_H

#define BOOK_SIZE 8
#define MIN_SEG_SIZE 32

/* The user can allocate at most 2^24 - 8 bytes. This means there are 2^19
 * different segment lengths a user can allocate, and we are going to use a
 * unique bucket to recycle each one. */
#define MAX_ALLOC (((uint32_t)1 << 24) - BOOK_SIZE)
#define KERN_SIZE MAX_ALLOC
#define REC_BUCKETS ((uint32_t)1 << 19) /* 2^19 recyclable segment lengths */

#define GB4 ((uint64_t)1 << 32) /* 4 GB = 2^32 */
#define BOOK_SIZE 8
#define BLOCK_SIZE 32

#define INIT_STACK_SIZE 2
#define SEG_NOT_FOUND 1

#include <stdint.h>

typedef struct {
    void *mem;              /* Pointer to the full 4GB of memory */
    void *usable_mem;       /* Pointer to the beginning of usable memory */
    void *recycler;         /* Array of stacks for recycling segments */
    uint32_t kernel_virtual_size;
    uint32_t begin_unused;
} Mem_T;

inline void *convert_address(uint8_t *umem, uint32_t addr)
{
    return umem + addr;
}

inline uint32_t get_idx_from_alloc_size(uint32_t size)
{
    /* Will allocate 1 block when it gets an exact fit */
    uint32_t num_blocks = ((size + BOOK_SIZE - 1) / 32);
    return num_blocks;
}

#endif
