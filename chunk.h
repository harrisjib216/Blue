#ifndef blue_chunk_h
#define blue_chunk_h

#include "common.h"
#include "value.h"

// types of code to execute
typedef enum
{
    OP_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_GLOBAL,
    OP_DEFINE_GLOBAL,
    OP_SET_GLOBAL,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_EXPONENT,
    OP_NOT,
    OP_NEGATE,
    OP_PRINT,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,
    OP_CALL,
    OP_RETURN,
} OpCode;

// chunk of code
typedef struct
{
    // number of things in chunk
    int count;

    // limit of chunk
    int capacity;

    // pointer to code
    uint8_t *code;

    // lines of code, places where errors can occur
    int *lines;

    // array of literal values
    ValueArray constants;
} Chunk;

// create chunk
void initChunk(Chunk *chunk);

// clear chunk
void freeChunk(Chunk *chunk);

// write code to chunk
void writeChunk(Chunk *chunk, uint8_t byte, int line);

// add literal value
int addConstant(Chunk *chunk, Value value);

#endif