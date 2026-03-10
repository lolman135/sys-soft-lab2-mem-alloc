#include "alloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGE_SIZE 4096
#define SLAB_SIZE (PAGE_SIZE * 2)

static void *system_malloc(size_t size) {
    void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (ptr == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

static void system_free(void *ptr, size_t size) {
    if (ptr != NULL) {
        munmap(ptr, size);
    }
}

static arena_t *create_arena(void) {
    arena_t *arena = (arena_t *)system_malloc(sizeof(arena_t));
    arena->extent_list = NULL;
    arena->slab_list = NULL;
    return arena;
}

static slab_header_t *create_slab(size_t object_size) {
    slab_header_t *slab = (slab_header_t *)system_malloc(sizeof(slab_header_t));
    slab->object_size = object_size;
    slab->free_count = SLAB_SIZE / object_size;
    slab->memory = system_malloc(SLAB_SIZE);
    slab->free_list = slab->memory;
    slab->next_slab = NULL;
    void *current = slab->memory;

    for (uint32_t i = 0; i < slab->free_count - 1; ++i) {
        *((void **)current) = (void *)((char *)current + object_size);
        current = *((void **)current);
    }

    *((void **)current) = NULL;
    return slab;
}

allocator_t *create_allocator(void) {
    allocator_t *allocator = (allocator_t *)system_malloc(sizeof(allocator_t));
    allocator->arena = create_arena();
    pthread_mutex_init(&allocator->lock, NULL);
    return allocator;
}

void *mem_alloc(allocator_t *allocator, size_t size) {
    if (size == 0) size = 1;
    pthread_mutex_lock(&allocator->lock);
    slab_header_t *slab = allocator->arena->slab_list;

    while (slab) {
        if (slab->object_size == size && slab->free_count > 0) {
            void *object = slab->free_list;
            slab->free_list = *((void **)slab->free_list);
            slab->free_count--;
            pthread_mutex_unlock(&allocator->lock);
            return object;
        }
        slab = slab->next_slab;
    }

    slab = create_slab(size);
    slab->next_slab = allocator->arena->slab_list;
    allocator->arena->slab_list = slab;
    void *object = slab->free_list;
    slab->free_list = *((void **)slab->free_list);
    slab->free_count--;

    pthread_mutex_unlock(&allocator->lock);
    return object;
}

void mem_free(allocator_t *allocator, void *ptr, size_t size) {
    if (!ptr) return;
    pthread_mutex_lock(&allocator->lock);
    slab_header_t *slab = allocator->arena->slab_list;

    while (slab) 
    {
        if (slab->object_size == size) 
        {
            *((void **)ptr) = slab->free_list;
            slab->free_list = ptr;
            slab->free_count++;
            pthread_mutex_unlock(&allocator->lock);
            return;
        }
        slab = slab->next_slab;
    }
    system_free(ptr, size);
    pthread_mutex_unlock(&allocator->lock);
}

void *mem_realloc(allocator_t *allocator, void *ptr, size_t old_size, size_t new_size) {
    if (new_size == old_size) {
        return ptr;
    }
    if (!ptr) {
        return mem_alloc(allocator, new_size);
    }
    void *new_ptr = mem_alloc(allocator, new_size);
    memcpy(new_ptr, ptr, old_size < new_size ? old_size : new_size);
    mem_free(allocator, ptr, old_size);
    return new_ptr;
}

void mem_show(allocator_t *allocator) {
    pthread_mutex_lock(&allocator->lock);
    slab_header_t *slab = allocator->arena->slab_list;

    if (!slab) {
        printf("No slabs allocated yet.\n");
        pthread_mutex_unlock(&allocator->lock);
        return;
    }

    while (slab) {
        uint32_t total_slots = SLAB_SIZE / slab->object_size;

        printf("Slab for %u-byte objects:\n", slab->object_size);
        printf(" • Total slots : %u\n", total_slots);
        printf(" • Free : %u\n", slab->free_count);
        printf(" • Occupied : %u\n", total_slots - slab->free_count);
        printf(" • Memory block : %p .. %p\n",
               slab->memory,
               (char*)slab->memory + SLAB_SIZE - 1);
        printf(" • Free list head : %p\n", slab->free_list);

        if (slab->free_list) {
            printf(" • Free objects:\n");
            void *cur = slab->free_list;
            while (cur) {
                printf("   %p\n", cur);
                cur = *(void**)cur;
            }
        } else {
            printf(" • No free objects in this slab.\n");
        }

        printf("\n");
        slab = slab->next_slab;
    }
    pthread_mutex_unlock(&allocator->lock);
}

void allocator_destroy(allocator_t *allocator) {
    if (!allocator) return;
    pthread_mutex_lock(&allocator->lock);

    slab_header_t *slab = allocator->arena->slab_list;
    while (slab) {
        slab_header_t *next = slab->next_slab;
        system_free(slab->memory, SLAB_SIZE);
        system_free(slab, sizeof(slab_header_t));
        slab = next;
    }

    extent_header_t *extent = allocator->arena->extent_list;
    while (extent) {
        extent_header_t *next = extent->next_extent;
        system_free(extent->memory, extent->size);
        system_free(extent, sizeof(extent_header_t));
        extent = next;
    }

    pthread_mutex_unlock(&allocator->lock);
    pthread_mutex_destroy(&allocator->lock);
    system_free(allocator->arena, sizeof(arena_t));
    system_free(allocator, sizeof(allocator_t));
}