#include <stdlib.h>

#include "memory.h"
#include "vm.h"

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

//
static void freeObject(Obj *object)
{
    switch (object->type)
    {
    case OBJ_FUNCTION:
    {
        ObjFunction *function = (ObjFunction *)object;
        freeChunk(&function->chunk);
        FREE(ObjFunction, object);
        break;
    }
    case OBJ_NATIVE:
        FREE(ObjNative, object);
        break;
    case OBJ_STRING:
    {
        ObjString *string = (ObjString *)object;
        FREE_ARRAY(char, string->chars, string->length + 1);
        FREE(ObjString, object);
        break;
    }
    }
}

// frees all objects in the vm
void freeObjects()
{
    Obj *curr = vm.objects;

    while (curr != NULL)
    {
        Obj *next = curr->next;
        freeObject(curr);
        curr = next;
    }
}