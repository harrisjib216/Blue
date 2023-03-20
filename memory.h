#ifndef blue_memory_h
#define blue_memory_h

#include "common.h"
#include "object.h"

// create new heap array
#define ALLOCATE(type, length) \
    (type *)reallocate(NULL, 0, sizeof(type) * (length))

// resize an allocation to zero bytes
#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0);

// default to eight or double heap size
#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity)*2)

// reallocate space and return it
#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type *)reallocate(pointer, sizeof(type) * (oldCount), sizeof(type) * (newCount))

// clear the array
#define FREE_ARRAY(type, pointer, oldCount) \
    reallocate(pointer, sizeof(type) * (oldCount), 0)

// free, exit, or make space
void *reallocate(void *pointer, size_t oldSize, size_t newSize);

// frees all objects
void freeObjects();

#endif