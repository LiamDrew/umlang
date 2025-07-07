#ifndef RECYCLER_H
#define RECYCLER_H

#include "mem_state.h"
#include "stack.h"

// Do these functions need to be inlined? I think not
inline Stack_T *recycler_init(void)
{
    Stack_T *recycler = malloc(sizeof(Stack_T) * REC_BUCKETS);
    assert(recycler != NULL);

    for (uint32_t i = 0; i < REC_BUCKETS; i++)
    {
        Stack_T s = stack_init(INIT_STACK_SIZE);
        recycler[i] = s;
    }

    return recycler;
}

inline void free_recycler(Stack_T *rec)
{
    for (uint32_t i = 0; i < REC_BUCKETS; i++) {
        stack_free(rec[i]);
    }

    free(rec);
}

inline uint32_t find_freed_segment(uint32_t size, Stack_T *rec)
{
    uint32_t index = get_idx_from_alloc_size(size);

    /* Omitted to improve performance:
     * assert(index < REC_BUCKETS); */

    Stack_T s = rec[index];

    /* if no segment of size 'size', check next bucket */
    if (stack_is_empty(s))
        return SEG_NOT_FOUND;

    uint32_t freed_segment = stack_top(s);
    rec[index] = stack_pop(s);
    return freed_segment;
}

inline void free_segment(uint8_t *umem, uint32_t seg_addr, Stack_T *rec)
{
    uint32_t sys_addr = seg_addr - BOOK_SIZE;

    uint32_t *virt = convert_address(umem, sys_addr);
    uint32_t cap = *virt;

    uint32_t index = ((cap + 8) / 32) - 1;

    /* NOTE: intentionally storing the user-facing v^2 address in the stack
     * for easy reuse in the future */
    rec[index] = stack_push(rec[index], seg_addr);
}

#endif
