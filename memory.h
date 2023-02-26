#ifndef blue_memory_h
#define blue_memory_h

#include "common.h"

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

#endif