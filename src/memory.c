#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "memory.h"

Header *find_free_block(Header **last, size_t size) {
  Header *curr = global_base;
  while (curr && !(curr->Header.free && curr->Header.size >= size)) {
    *last = curr;
    curr = curr->Header.next;
  }
  return curr;
}

Header *request_space(Header *last, size_t size) {
  Header *block;
  block = sbrk(0);
  void *request = sbrk(size + META_SIZE);
  assert((void *)block == request);

  if (request == (void *)-1) {
    return NULL; // failed
  }

  if (last)
    last->Header.next = block;

  block->Header.size = size;
  block->Header.next = NULL;
  block->Header.prev = NULL;
  block->Header.free = 0;
  block->Header.magic = 0x12345678;

  return block;
}

void *malloc(size_t size) {
  Header *block;

  if (size <= 0)
    return NULL;

  // first call
  if (!global_base) {
    block = request_space(NULL, size);
    if (!block)
      return NULL;

    global_base = block;
  } else {
    Header *last = global_base;
    block = find_free_block(&last, size);
    if (!block) {
      // failed to find a free block
      block = request_space(last, size);
      if (!block)
        return NULL;

    } else {
      block->Header.free = 0;
      block->Header.magic = 0x77777777;
    }
  }

  // return region after Header, + 1 here means Header size
  return (block + 1);
}

Header *get_header_ptr(void *ptr) { return (Header *)ptr - 1; }

void free(void *ptr) {
  if (!ptr)
    return;

  Header *header_ptr = get_header_ptr(ptr);
  assert(header_ptr->Header.free == 0);
  assert(header_ptr->Header.magic == 0x77777777 ||
         header_ptr->Header.magic == 0x12345678);
  header_ptr->Header.free = 1;
  header_ptr->Header.magic = 0x55555555;
}

void *realloc(void *ptr, size_t size) {
  // should work like malloc
  if (!ptr)
    return malloc(size);

  Header *header_ptr = get_header_ptr(ptr);
  if (header_ptr->Header.size >= size)
    return ptr; // enough space

  void *new_ptr;
  new_ptr = malloc(size);
  if (!new_ptr)
    return NULL;

  memcpy(new_ptr, ptr, header_ptr->Header.size);
  free(ptr);

  return new_ptr;
}

void *calloc(size_t nelem, size_t elsize) {
  if (nelem > SIZE_MAX / elsize) {
    return NULL; // overflow
  }
  size_t size = nelem * elsize;

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
