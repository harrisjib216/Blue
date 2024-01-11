#ifndef blue_vm_h
#define blue_vm_h

#include "chunk.h"
#include "object.h"
#include "table.h"
#include "value.h"

// length of function calls
#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

// single ongoing function call, the current would be at the top
typedef struct
{
    // current function being called
    ObjFunction *function;

    // jump back to before the function call started
    uint8_t *ip;

    // vm value stack
    Value *slots;
} CallFrame;

typedef struct
{
    // visualize a function-call stack
    CallFrame frames[FRAMES_MAX];
    int frameCount;

    // chunk to run
    Chunk *chunk;

    // pointer of instruction to run
    uint8_t *ip;

    // stack
    Value stack[STACK_MAX];

    // points to element just past top of stack
    // points to where next new value should go
    Value *stackTop;

    // global variables
    Table globals;

    // hash of all strings
    Table strings;

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