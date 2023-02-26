#include "common.h"

#include "chunk.h"
#include "debug.h"
#include "vm.h"

int main(int argc, const char *argv[])
{
    // initialize vm
    initVM();

    // make chunk of code
    Chunk chunk;
    initChunk(&chunk);

    // write constant value to chunk
    int constant = addConstant(&chunk, 1.2);
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, constant, 123);

    // write end of func or program
    writeChunk(&chunk, OP_RETURN, 123);

    // todo: remove
    disassembleChunk(&chunk, "test chunk");

    // read code
    interpret(&chunk);

    // free vm and code
    freeVM();
    freeChunk(&chunk);

    return 0;
}