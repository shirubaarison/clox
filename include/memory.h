#ifndef CLOX_MEMORY_H
#define CLOX_MEMORY_H

#include "common.h" // IWYU pragma: keep

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, pointer, oldCount, newCount)                          \
  (type *)reallocate(pointer, sizeof(type) * (oldCount),                       \
                     sizeof(type) * (newCount))

#define FREE_ARRAY(type, pointer, oldCount)                                    \
  reallocate(pointer, sizeof(type) * (oldCount), 0)

typedef long double Align;

typedef union header {
  struct {
    size_t size;
    union header *next;
    union header *prev;
    int free;
    int magic; // For debbuging only
  } s;

  Align x; // is double long heavier than the previous struct?
} Header;

#define META_SIZE sizeof(Header)

static Header base;
static Header *list_head = NULL; // linked list head

Header *find_free_block(Header **last, size_t size);
Header *request_space(Header *last, size_t size);
Header *get_header_ptr(void *ptr);

void *malloc(size_t size);
void free(void *ptr);
void *realloc(void *ptr, size_t size);
void *calloc(size_t nelem, size_t elsize);

void *reallocate(void *pointer, size_t oldSize, size_t newSize);

#endif // !CLOX_MEMORY_H
