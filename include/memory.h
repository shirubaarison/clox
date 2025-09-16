#ifndef CLOX_MEMORY_H
#define CLOX_MEMORY_H

#include "common.h"

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, pointer, oldCount, newCount)                          \
  (type *)reallocate(pointer, sizeof(type) * (oldCount),                       \
                     sizeof(type) * (newCount))

#define FREE_ARRAY(type, pointer, oldCount)                                    \
  reallocate(pointer, sizeof(type) * (oldCount), 0)

// for simplicity, it will be a single linked list.
typedef struct block_meta {
  size_t size;
  struct block_meta *next;
  int free;
  int magic; // For debbuging only
} block_meta;

#define META_SIZE sizeof(block_meta)

void *global_base = NULL; // linked list head

block_meta *find_free_block(block_meta **last, size_t size);
block_meta *request_space(block_meta *last, size_t size);
block_meta *get_block_ptr(void *ptr);

void *malloc(size_t size);
void free(void *ptr);
void *realloc(void *ptr, size_t size);
void *calloc(size_t nelem, size_t elsize);

void *reallocate(void *pointer, size_t oldSize, size_t newSize);

#endif // !CLOX_MEMORY_H
