/**
 * @file driver.h
 * @date February 2025
 * @brief
 * The interface for initializing and using the virtual memory system.
 */

#ifndef DRIVER_H
#define DRIVER_H

#include "recycler.h"
#include <assert.h>
#include <string.h>

extern uint8_t *usable;
extern Stack_T *rec;
extern Mem_T *mem;
extern uint32_t start_unused;

uint8_t *init_memory_system(uint32_t kernel_size);

void terminate_memory_system(void);

/* Kernel (Re)allocate (kern_realloc):
 * Overwrite the zero segment and initialize all memory to zero */
uint32_t kern_realloc(uint32_t size);


/* Kernel Memory Copy (kern_memcpy):
 * Copies data from "userspace" to "kernel space" */
void kern_memcpy(uint32_t src_addr, uint32_t copy_size);

/* Virtual Segment Calloc (vs_calloc): 
 * Carve out a segment of virtual memory and serve it to the program as
 * zeroed-out v^2 memory */
inline uint32_t vs_calloc(uint8_t *umem, uint32_t size)
{
    /* Users may only ask vs_malloc for (2^24 - 8) bytes of contiguous space
     * Omitted for performance reasons. The user must use our module correctly:
     * assert(size < MAX_ALLOC); */

    /* Look for segments to be recycled. If there are freed segments that are
     * ready to be recycled, recycled them */
    uint32_t freed_seg = find_freed_segment(size, rec);

    /* check that a free segment is available */
    if (freed_seg != SEG_NOT_FOUND)
    {
        uint32_t *freed_seg_addr = convert_address(umem, freed_seg);

        freed_seg_addr[-1] = size;

        memset(freed_seg_addr, 0, size);

        return freed_seg;
    }

    /* If no segments can be recycled, carve a fresh one from the heap */
    uint32_t user_start = start_unused + BOOK_SIZE;

    /* Find the number of 32 byte blocks need to fill the allocation */
    uint32_t num_blocks = get_idx_from_alloc_size(size) + 1;
    uint32_t user_cap = (num_blocks * BLOCK_SIZE) - BOOK_SIZE;

    /* Check that we still have enough 'carvable' memory in the heap.
     * Add BOOK_SIZE to user start to account for kernel bookkeeping
     * Omitting this for performance reasons:
     * assert(GB4 - (user_start + BOOK_SIZE) >= user_cap); */

    /* Update the beginning of the unused heap */
    start_unused = user_start + user_cap;

    uint32_t *user_addr = convert_address(umem, user_start);

    user_addr[-2] = user_cap;
    user_addr[-1] = size;

    return user_start;
}

/* Virtual Segment Free (vs_free):
 * Free a virtual segment for future use. */
inline void vs_free(uint32_t addr)
{
    free_segment(usable, addr, rec);
}

/* Set At (set_at):
 * Store a uint32_t at a virtual address */
inline void set_at(uint8_t *umem, uint32_t addr, uint32_t value)
{
    uint32_t *dest = convert_address(umem, addr);
    *dest = value;
}

/* Get At (get_at):
 * Get the uint32_t stored at a virtual address */
inline uint32_t get_at(uint8_t *umem, uint32_t addr)
{
    uint32_t *src = convert_address(umem, addr);
    return *src;
}

#endif
