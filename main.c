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

    // write number to constants
    int constant = addConstant(&chunk, 1.2);
    writeChunk(&chunk, OP_CONSTANT, 105);
    writeChunk(&chunk, constant, 105);

    // write another number
    constant = addConstant(&chunk, 3.5);
    writeChunk(&chunk, OP_CONSTANT, 105);
    writeChunk(&chunk, constant, 105);

    // write addition operation
    writeChunk(&chunk, OP_ADD, 105);

    // write another number
    constant = addConstant(&chunk, 5.7);
    writeChunk(&chunk, OP_CONSTANT, 105);
    writeChunk(&chunk, constant, 105);

    // write addition operation
    writeChunk(&chunk, OP_DIVIDE, 105);

    // run negation
    // writeChunk(&chunk, OP_NEGATE, 105);

    // write end of func or program
    writeChunk(&chunk, OP_RETURN, 105);

    // todo: remove
    disassembleChunk(&chunk, "test chunk");

    // read code
    interpret(&chunk);

    // free vm and code
    freeVM();
    freeChunk(&chunk);

    return 0;
}