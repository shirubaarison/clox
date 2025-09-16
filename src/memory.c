#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "memory.h"

block_meta *find_free_block(block_meta **last, size_t size) {
  block_meta *curr = global_base;
  while (curr && !(curr->free && curr->size >= size)) {
    *last = curr;
    curr = curr->next;
  }
  return curr;
}

block_meta *request_space(block_meta *last, size_t size) {
  block_meta *block;
  block = sbrk(0);
  void *request = sbrk(size + META_SIZE);
  assert((void *)block == request);

  if (request == (void *)-1) {
    return NULL; // failed
  }

  if (last)
    last->next = block;

  block->size = size;
  block->next = NULL;
  block->free = 0;
  block->magic = 0x12345678;

  return block;
}

void *malloc(size_t size) {
  block_meta *block;

  if (size <= 0)
    return NULL;

  // first call
  if (!global_base) {
    block = request_space(NULL, size);
    if (!block)
      return NULL;

    global_base = block;
  } else {
    block_meta *last = global_base;
    block = find_free_block(&last, size);
    if (!block) {
      // failed to find a free block
      block = request_space(last, size);
      if (!block)
        return NULL;
    } else {
      block->free = 0;
      block->magic = 0x77777777;
    }
  }

  return (block +
          1); // return region after block_meta, + 1 here means block_meta size
}

block_meta *get_block_ptr(void *ptr) { return (block_meta *)ptr - 1; }

void free(void *ptr) {
  if (!ptr)
    return;

  block_meta *block_ptr = get_block_ptr(ptr);
  assert(block_ptr->free == 0);
  assert(block_ptr->magic == 0x77777777 || block_ptr->magic == 0x12345678);
  block_ptr->free = 1;
  block_ptr->magic = 0x55555555;
}

void *realloc(void *ptr, size_t size) {
  // should work like malloc
  if (!ptr)
    return malloc(size);

  block_meta *block_ptr = get_block_ptr(ptr);
  if (block_ptr->size >= size)
    return ptr; // enough space

  void *new_ptr;
  new_ptr = malloc(size);
  if (!new_ptr)
    return NULL;

  memcpy(new_ptr, ptr, block_ptr->size);
  free(ptr);

  return new_ptr;
}

void *calloc(size_t nelem, size_t elsize) {
  size_t size = nelem * elsize; // TODO: check for overflow
  void *ptr = malloc(size);
  memset(ptr, 0, size);

  return ptr;
}

void *reallocate(void *pointer, size_t oldSize, size_t newSize) {
  if (newSize == 0) {
    free(pointer);
    return NULL;
  }

  void *result = realloc(pointer, newSize);
  if (result == NULL) {
    exit(1);
  }

  return result;
}
