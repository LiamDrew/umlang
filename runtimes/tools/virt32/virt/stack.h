#ifndef STACK_H
#define STACK_H

#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>

typedef struct {
    uint32_t* stack;
    uint32_t size;
    uint32_t capacity;
} __attribute__((packed)) Stack_T;


inline Stack_T stack_init(uint32_t size)
{
    assert(size > 0);

    Stack_T s;
    s.stack = malloc(size * sizeof(uint32_t));
    s.capacity = size;
    s.size = 0;

    return s;
}

inline uint32_t stack_top(Stack_T s)
{
    assert(s.size > 0);
    return s.stack[s.size - 1];
}

inline Stack_T stack_push(Stack_T s, uint32_t elem)
{
    if (s.size == s.capacity)
    {
        /* Expand the stack */
        s.capacity *= 2;
        uint32_t *temp = malloc(s.capacity * sizeof(uint32_t));
        for (uint32_t i = 0; i < s.size; i++)
        {
            temp[i] = s.stack[i];
        }

        free(s.stack);
        s.stack = temp;
    }

    s.stack[s.size++] = elem;
    return s;
}

inline Stack_T stack_pop(Stack_T s)
{
    /* Omitted to improve performance:
     * assert(s.size > 0); */

    s.size--;
    return s;
}

inline bool stack_is_empty(Stack_T s)
{
    return s.size == 0;
}

inline void stack_free(Stack_T s)
{
    free(s.stack);
}

#endif 
