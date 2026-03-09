#include "alloc.h"
#include <stdio.h>

int main(void) {
    allocator_t *allocator = create_allocator();

    void *ptr1 = mem_alloc(allocator, 128);
    void *ptr2 = mem_alloc(allocator, 256);
    void *ptr3 = mem_alloc(allocator, 128);

    printf("Memory allocation done.\n");
    mem_show(allocator);

    ptr1 = mem_realloc(allocator, ptr1, 128, 200);
    mem_free(allocator, ptr1, 200);
    mem_free(allocator, ptr2, 256);
    mem_free(allocator, ptr3, 128);

    printf("\nMemory deallocation done.\n");
    mem_show(allocator);

    destroy_allocator(allocator);

    return 0;
}