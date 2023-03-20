#ifndef blue_value_h
#define blue_value_h

#include "common.h"

typedef struct Obj Obj;
typedef struct ObjString ObjString;

typedef enum
{
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ,
} ValueType;

// union to store data in same location, pg 526
typedef struct
{
    ValueType type;
    union
    {
        // stack
        bool boolean;
        double number;
        // heap
        Obj *obj;
    } as;
} Value;

// checking type before using AS_ macros
#define IS_BOOL(item) ((item).type == VAL_BOOL)
#define IS_NIL(item) ((item).type == VAL_NIL)
#define IS_NUMBER(item) ((item).type == VAL_NUMBER)
#define IS_OBJ(item) ((item).type == VAL_OBJ)

// unpack struct for C value; nil carries no extra data to rep nil
#define AS_BOOL(item) ((item).as.boolean)
#define AS_NUMBER(item) ((item).as.number)
#define AS_OBJ(item) ((item).as.obj)

// macros to mask C values into our struct
#define BOOL_VAL(item) ((Value){VAL_BOOL, {.boolean = item}})
#define NIL_VAL ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(item) ((Value){VAL_NUMBER, {.number = item}})
#define OBJ_VAL(item) ((Value){VAL_OBJ, {.obj = (Obj *)item}})

// array of literal values
typedef struct
{
    // size of value array
    int capacity;

    // size of items in values array
    int count;

    // array of values
    Value *values;
} ValueArray;

// returns a C bool for the users code to see
bool valuesEquate(Value a, Value b);

// create or clear array
void initValueArray(ValueArray *array);

// add item
void writeArrayValue(ValueArray *array, Value value);

// delete items
void freeValueArray(ValueArray *array);

// void print value
void printValue(Value value);

// print value with newline
void printlnValue(Value value);

#endif