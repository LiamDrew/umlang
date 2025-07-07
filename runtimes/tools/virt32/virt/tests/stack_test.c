#include "stack.h"
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

void general_use(void);
void pop_from_empty(void);
void repeated_operations(void);
void insert_many_elems(void);

int main(void)
{
    general_use();
    repeated_operations();
    insert_many_elems();

    /* These tests will throw errors */
    // pop_from_empty();
}

/* Stack Tests */

/* General Use */
void general_use(void)
{
    Stack_T *s = stack_init(10);

    stack_push(s, 1);
    stack_push(s, 2);
    stack_push(s, 3);
    stack_push(s, 4);
    stack_push(s, 5);
    stack_push(s, 6);
    stack_push(s, 7);
    stack_push(s, 8);
    stack_push(s, 9);
    stack_push(s, 10);
    stack_push(s, 11);
    printf("top: %d\n", stack_pop(s));
    printf("top: %d\n", stack_pop(s));
    printf("top: %d\n", stack_pop(s));
    printf("top: %d\n", stack_pop(s));
    printf("top: %d\n", stack_pop(s));
    printf("top: %d\n", stack_pop(s));
    printf("top: %d\n", stack_pop(s));
    printf("top: %d\n", stack_pop(s));
    printf("top: %d\n", stack_pop(s));
    printf("top: %d\n", stack_pop(s));
    printf("top: %d\n", stack_pop(s));

    assert(stack_is_empty(s));

    stack_free(s);
}

/* Try to pop from an empty stack */
void pop_from_empty(void)
{
    Stack_T *s = stack_init(1);
    stack_push(s, 1);
    printf("top: %d\n", stack_pop(s));
    printf("top: %d\n", stack_pop(s));

    /* This will error before the stack gets freed */
    stack_free(s);
}

/* Insert, remove all, insert more */
void repeated_operations(void)
{
    Stack_T *s = stack_init(1);
    stack_push(s, 1);
    stack_push(s, 2);
    printf("top: %d\n", stack_pop(s));
    printf("top: %d\n", stack_pop(s));
    stack_push(s, 3);
    stack_push(s, 4);
    stack_push(s, 5);
    printf("top: %d\n", stack_pop(s));
    printf("top: %d\n", stack_pop(s));
    printf("top: %d\n", stack_pop(s));

    assert(stack_is_empty(s));

    stack_free(s);
}

/* Insert 1000 elems */
void insert_many_elems(void)
{
    Stack_T *s = stack_init(1);

    for (uint32_t i = 0; i < 1000; i++) {
        stack_push(s, i);
    }

    assert(!stack_is_empty(s));

    for (uint32_t i = 0; i < 1000; i++) {
        stack_pop(s);
    }

    assert(stack_is_empty(s));

    stack_free(s);
}
