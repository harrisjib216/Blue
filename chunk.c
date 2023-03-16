#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

// make chunk of code
void initChunk(Chunk *chunk)
{
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    initValueArray(&chunk->constants);
}

// empty chunk of code
void freeChunk(Chunk *chunk)
{
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}

// add code to chunk
void writeChunk(Chunk *chunk, uint8_t byte, int line)
{
    // grow array if not enough space
    if (chunk->capacity < chunk->count + 1)
    {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
        chunk->lines = GROW_ARRAY(int, chunk->lines, oldCapacity, chunk->capacity);
    }

    // append bytecode, lines, and increment count
    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

// add literal value to constants array, returns its index
int addConstant(Chunk *chunk, Value value)
{
    writeArrayValue(&chunk->constants, value);
    return chunk->constants.count - 1;
}