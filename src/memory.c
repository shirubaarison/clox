#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "memory.h"

Header *find_free_block(Header **last, size_t size) {
  Header *curr = list_head;
  while (curr && !(curr->s.free && curr->s.size >= size)) {
    *last = curr;
    curr = curr->s.next;
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
    last->s.next = block;

  block->s.size = size;
  block->s.next = NULL;

  if (last)
    block->s.prev = last;
  else
    block->s.prev = NULL;

  block->s.free = 0;
  block->s.magic = 0x12345678;

  return block;
}

void *malloc(size_t size) {
  Header *block;

  if (size <= 0)
    return NULL;

  if (!list_head) { // first call
    base.s.next = list_head = &base;
    base.s.size = 0;

    block = request_space(list_head, size);
    if (!block)
      return NULL;

    list_head = block;
  } else {
    Header *last = list_head;
    block = find_free_block(&last, size);

    if (!block) {
      // failed to find a free block
      block = request_space(last, size);
      if (!block)
        return NULL;

    } else { // found free block
      block->s.free = 0;
      block->s.magic = 0x77777777;

      if (block->s.size != size) {
        if (block->s.size >= size + META_SIZE + 1) { // larger than what I need
          // split the blocks
          Header *new_block = (Header *)((char *)(block + 1) + size);
          new_block->s.size = block->s.size - size - META_SIZE;
          new_block->s.next = block->s.next;
          new_block->s.prev = block;
          new_block->s.free = 1;

          block->s.size = size;
          block->s.next = new_block;

          if (new_block->s.next)
            new_block->s.next->s.prev = new_block;
        }
      }
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
  assert(header_ptr->s.free == 0);
  assert(header_ptr->s.magic == 0x77777777 ||
         header_ptr->s.magic == 0x12345678);

  header_ptr->s.free = 1;
  header_ptr->s.magic = 0x55555555;

  // merge adjacent free blocks
  if (header_ptr->s.prev != &base && header_ptr->s.prev->s.free == 1) {
    header_ptr->s.prev->s.size += META_SIZE + header_ptr->s.size;
    header_ptr->s.prev->s.next = header_ptr->s.next;
    if (header_ptr->s.next) {
      header_ptr->s.next->s.prev = header_ptr->s.prev;
    }
    header_ptr = header_ptr->s.prev; // merged block becomes current
  }

  if (header_ptr->s.next && header_ptr->s.next->s.free == 1) {
    header_ptr->s.size += META_SIZE + header_ptr->s.next->s.size;
    header_ptr->s.next = header_ptr->s.next->s.next;
    if (header_ptr->s.next) {
      header_ptr->s.next->s.prev = header_ptr;
    }
  }
}

void *realloc(void *ptr, size_t size) {
  // should work like malloc
  if (!ptr)
    return malloc(size);

  Header *header_ptr = get_header_ptr(ptr);
  if (header_ptr->s.size >= size)
    return ptr; // enough space

  void *new_ptr;
  new_ptr = malloc(size);
  if (!new_ptr)
    return NULL;

  memcpy(new_ptr, ptr, header_ptr->s.size);
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
