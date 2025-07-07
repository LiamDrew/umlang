#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "recycler.h"
#include "driver.h"
#include "stack.h"
#include "mem_state.h"

// void general_test(void);

int main(void)
{
    printf("Not testing the recycler for now\n");
    // general_test();
}

// void general_test(void)
// {
//     /* Recycler Tests */

//     // TODO: this needs to get all fixed up. Many things about the interface
//     // have changed since these tests were written.

//     uint32_t kernel_size = 100000;
//     uint8_t *umem = init_memory_system(kernel_size);

//     Stack_T *s = &((Stack_T *)mem->recycler)[0];
//     assert(stack_is_empty(s));
//     free_segment(mem, 32);
//     assert(!stack_is_empty(s));
//     assert(stack_top(s) == 24);

//     // Finding freed segment at the initial index
//     uint32_t segment = find_freed_segment(mem, 32);
//     printf("segment: %d\n", segment);
//     assert(segment == 24);

//     // Finding freed segment at the next index
//     assert(stack_is_empty(s));
//     free_segment(mem, 64);
//     s = &((Stack_T *)mem->recycler)[1];

//     assert(!stack_is_empty(s));
//     assert(stack_top(s) == 56);

//     segment = find_freed_segment(mem, 32);
//     printf("segment: %d\n", segment);
//     assert(segment == 56);

//     // Finding freed segment on empty freelists
//     segment = find_freed_segment(mem, 32);
//     assert((int)segment == -1);

//     stack_free(s);
// }

// void general_test(void)
// {
//     /* Recycler Tests */

//     // TODO: this needs to get all fixed up. Many things about the interface
//     // have changed since these tests were written.

//     uint32_t kernel_size = 100000;
//     Mem_T *mem = init_memory_system(kernel_size);

//     Stack_T *s = &((Stack_T *)mem->recycler)[0];
//     assert(stack_is_empty(s));
//     free_segment(mem, 32);
//     assert(!stack_is_empty(s));
//     assert(stack_top(s) == 24);

//     // Finding freed segment at the initial index
//     uint32_t segment = find_freed_segment(mem, 32);
//     printf("segment: %d\n", segment);
//     assert(segment == 24);

//     // Finding freed segment at the next index
//     assert(stack_is_empty(s));
//     free_segment(mem, 64);
//     s = &((Stack_T *)mem->recycler)[1];

//     assert(!stack_is_empty(s));
//     assert(stack_top(s) == 56);

//     segment = find_freed_segment(mem, 32);
//     printf("segment: %d\n", segment);
//     assert(segment == 56);

//     // Finding freed segment on empty freelists
//     segment = find_freed_segment(mem, 32);
//     assert((int)segment == -1);

//     stack_free(s);
// }
