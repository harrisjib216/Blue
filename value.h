#ifndef blue_value_h
#define blue_value_h

#include "common.h"

// define value type
typedef double Value;

//
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

// print value
void printValue(Value value);

#endif