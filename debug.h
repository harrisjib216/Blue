#ifndef blue_debug_h
#define blue_debug_h

#include "chunk.h"
#include "debug.h"

void disassembleChunk(Chunk *chunk, const char *name);

int disassembleInstruction(Chunk *chunk, int offset);

#endif