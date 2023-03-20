#ifndef blue_object_h
#define blue_object_h

#include "common.h"
#include "value.h"

#define OBJ_TYPE(item) (AS_OBJ(item)->type)

#define IS_STRING(item) isObjType(item, OBJ_STRING)

#define AS_STRING(item) ((ObjString *)AS_OBJ(item))
#define AS_CSTRING(item) (((ObjString *)AS_OBJ(value))->chars)

// types of objects for blue
typedef enum
{
    OBJ_STRING,
} ObjType;

// each blue object will inherit this struct
// to define its type and possibly other fields
struct Obj
{
    ObjType type;
};

// extends obj and adds string properties
struct ObjString
{
    Obj obj;
    int length;
    char *chars;
};

// clone a string
ObjString *copyString(const char *chars, int length);

// returns if the value/obj matches a certain ObjType
static inline bool isObjType(Value value, ObjType type)
{
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif
