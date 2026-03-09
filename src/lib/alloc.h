#ifndef ALLOC_H
#define ALLOC_H

#include <stdint.h>
#include <pthread.h>
#include <stddef.h>

#define PAGE_SIZE 4096
#define ARENA_SIZE (PAGE_SIZE * 10)
#define SLAB_SIZE  (PAGE_SIZE * 2)

typedef struct slab_header {
    struct slab_header *next_slab;
    uint32_t object_size;
    uint32_t free_count;
    void *free_list;
    void *memory;
} slab_header_t;

typedef struct extent_header {
    struct extent_header *next_extent;
    void *memory;
    size_t size;
} extent_header_t;

typedef struct arena {
    extent_header_t *extent_list;
    slab_header_t *slab_list;
} arena_t;

typedef struct allocator {
    arena_t *arena;
    pthread_mutex_t lock;
} allocator_t;

// Основные функции аллокатора
allocator_t *create_allocator(void);
void destroy_allocator(allocator_t *allocator);

void *mem_alloc(allocator_t *allocator, size_t size);
void mem_free(allocator_t *allocator, void *ptr, size_t size);
void *mem_realloc(allocator_t *allocator, void *ptr, size_t old_size, size_t new_size);

// Для отладки
void mem_show(allocator_t *allocator);

#endif // ALLOC_H