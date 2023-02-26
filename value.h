#ifndef blue_value_h
#define blue_value_h

#include "common.h"

typedef double Value;

typedef struct
{
    int capacity;
    int count;
    Value *values;
} ValueArray;

void initValueArray(ValueArray *array);

void writeArrayValue(ValueArray *array, Value value);

void freeValueArray(ValueArray *array);

void printValue(Value value);

#endif