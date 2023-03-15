#ifndef blue_value_h
#define blue_value_h

#include "common.h"

typedef enum
{
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
} ValueType;

// union to store data in same location, pg 526
typedef struct
{
    ValueType type;
    union
    {
        bool boolean;
        double number;
    } as;
} Value;

// checking type before using AS_ macros
#define IS_BOOL(data) ((data).type == VAL_BOOL)
#define IS_NIL(data) ((data).type == VAL_NIL)
#define IS_NUMBER(data) ((data).type == VAL_NUMBER)

// unpack struct for C value; nil carries no extra data to rep nil
#define AS_BOOL(data) ((data).as.boolean)
#define AS_NUMBER(data) ((data).as.number)

// macros to mask C values into our struct
#define BOOL_VAL(data) ((Value){VAL_BOOL, {.boolean = data}})
#define NIL_VAL ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(data) ((Value){VAL_NUMBER, {.number = data}})

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