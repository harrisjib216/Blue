#include <stdlib.h>

#include "memory.h"

// return reallocated heap space
void *reallocate(void *pointer, size_t oldSize, size_t newSize)
{
    // delete item, return null, end of program
    if (newSize == 0)
    {
        free(pointer);
        return NULL;
    }

    // make heap space
    void *result = realloc(pointer, newSize);

    // exit program if there's not enough memory
    if (result == NULL)
        exit(1);

    return result;
}