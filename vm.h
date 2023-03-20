#ifndef blue_vm_h
#define blue_vm_h

#include "chunk.h"
#include "value.h"

// length of stack for now
#define STACK_MAX 256

typedef struct
{
    // chunk to run
    Chunk *chunk;

    // pointer of instruction to run
    uint8_t *ip;

    // stack
    Value stack[STACK_MAX];

    // points to element just past top of stack
    // points to where next new value should go
    Value *stackTop;

    // linked list of all objects
    Obj *objects;
} VM;

typedef enum
{
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

// expose vm
extern VM vm;

// config vm
void initVM();

// clear vm contents
void freeVM();

// interpret code
InterpretResult interpret(const char *source);

// append value
void push(Value value);

// remove
Value pop();

#endif