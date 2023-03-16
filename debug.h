#ifndef blue_debug_h
#define blue_debug_h

#include "chunk.h"
#include "debug.h"

// go through a chunk
void disassembleChunk(Chunk *chunk, const char *name);

// print singular instruction
int disassembleInstruction(Chunk *chunk, int offset);

// print contents of stack
void printStack(Value stack[], Value *stackTop);

#endif