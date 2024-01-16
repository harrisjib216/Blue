#ifndef blue_object_h
#define blue_object_h

#include "common.h"
#include "chunk.h"
#include "value.h"

#define OBJ_TYPE(item) (AS_OBJ(item)->type)

#define IS_FUNCTION(item) isObjType(item, OBJ_FUNCTION)
#define IS_NATIVE(item) isObjType(item, OBJ_NATIVE);
#define IS_STRING(item) isObjType(item, OBJ_STRING)

#define AS_FUNCTION(item) ((ObjFunction *)AS_OBJ(item))
#define AS_NATIVE(item) \
    (((ObjNative *)AS_OBJ(item))->function)
#define AS_STRING(item) ((ObjString *)AS_OBJ(item))
#define AS_CSTRING(item) (((ObjString *)AS_OBJ(item))->chars)

// types of objects for blue
typedef enum
{
    OBJ_FUNCTION,
    OBJ_NATIVE,
    OBJ_STRING,
} ObjType;

// each blue object will inherit this struct
// to define its type and possibly other fields
struct Obj
{
    ObjType type;

    // linked list node of all objects
    struct Obj *next;
};

// code chunk representation of a Blue function
typedef struct
{
    // share obj type
    Obj obj;
    // number of function parameters
    int arity;
    // code in the function
    Chunk chunk;
    // function name
    ObjString *name;
} ObjFunction;

// native C functions, callable in Blue
typedef Value (*NativeFunc)(int argCount, Value *args);

typedef struct
{
    Obj obj;
    NativeFunc function;
} ObjNative;

// extends obj and adds string properties
struct ObjString
{
    Obj obj;
    int length;
    char *chars;
    uint32_t hash;
};

// c function to declare byte code function
ObjFunction *newFunction();

// native C functions, callable in Blue
ObjNative *newNative(NativeFunc function);

// passes ownership of string by making a copy
ObjString *takeString(char *chars, int length);

// clone a string
ObjString *copyString(const char *chars, int length);

// handle object printing
void printObject(Value value);

// returns if the value/obj matches a certain ObjType
static inline bool isObjType(Value value, ObjType type)
{
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

// todo: support converting any object into a string

#endif
