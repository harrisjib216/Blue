#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
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

// request heap space for this function
ObjFunction *newFunction()
{
    ObjFunction *function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->name = NULL;
    initChunk(&function->chunk);
    return function;
}

// native C functions, callable in Blue
ObjNative *newNative(NativeFunc function)
{
    ObjNative *native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function = function;
    return native;
}

// uses FNV-1a hash function
static uint32_t hashString(const char *key, int length)
{
    uint32_t hash = 2166136261u;

    for (int i = 0; i < length; i++)
    {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }

    return hash;
}

// creates new string on heap and initializes fields
// todo: perform hashing here
static ObjString *allocateString(char *chars, int length, uint32_t hash)
{
    ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;

    // todo: stop compiler from continuosly setting variable
    tableSet(&vm.strings, string, NIL_VAL);
    return string;
}

// returns location of string
ObjString *takeString(char *chars, int length)
{
    uint32_t hash = hashString(chars, length);

    // return reference if string already exists
    ObjString *interned = tableFindString(&vm.strings, chars, length, hash);
    if (interned != NULL)
    {
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }

    return allocateString(chars, length, hash);
}

// copy string from source or other location into heap
ObjString *copyString(const char *chars, int length)
{
    uint32_t hash = hashString(chars, length);

    ObjString *interned = tableFindString(&vm.strings, chars, length, hash);

    if (interned != NULL)
        return interned;

    char *heapString = ALLOCATE(char, length + 1);
    memcpy(heapString, chars, length);
    heapString[length] = '\0';

    return allocateString(heapString, length, hash);
}

// allow blue lang to print functions
// todo: print arguments it expects?
static void printFunction(ObjFunction *function)
{
    if (function->name == NULL)
    {
        printf("<script>");
        return;
    }

    printf("<func %s>", function->name->chars);
}

// handle different objects
void printObject(Value value)
{
    switch (OBJ_TYPE(value))
    {
    case OBJ_FUNCTION:
        printFunction(AS_FUNCTION(value));
        break;
    case OBJ_NATIVE:
        printf("<native fn>");
        break;
    case OBJ_STRING:
        printf("%s", AS_CSTRING(value));
        break;
    }
}
