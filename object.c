#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

// init object macro
#define ALLOCATE_OBJ(type, objectType) (type *)allocateObject(sizeof(type), objectType)

// make space for any object type
static Obj *allocateObject(size_t size, ObjType type)
{
    // allocate
    Obj *object = (Obj *)reallocate(NULL, 0, size);
    object->type = type;

    // insert object at the head,
    // the linked list is essentially in reverse
    object->next = vm.objects;
    vm.objects = object;

    return object;
}

// creates new string on heap and initializes fields
static ObjString *allocateString(char *chars, int length)
{
    ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    return string;
}

ObjString *takeString(char *chars, int length)
{
    return allocateString(chars, length);
}

// copy string from source or other location into heap
ObjString *copyString(const char *chars, int length)
{
    char *heapString = ALLOCATE(char, length + 1);
    memcpy(heapString, chars, length);
    heapString[length] = '\0';
    return allocateString(heapString, length);
}

// handle different objects
void printObject(Value value)
{
    switch (OBJ_TYPE(value))
    {
    case OBJ_STRING:
        printf("%s", AS_CSTRING(value));
        break;
    }
}
