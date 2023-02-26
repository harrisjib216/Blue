#include "common.h"
#include "debug.h"
#include "vm.h"

VM vm;

// set up vm
void initVM()
{
}

// clear vm
void freeVM()
{
}

// START OF THE RUN PROGRAM
static InterpretResult run()
{
// return pointer
#define READ_BYTE() (*vm.ip++)

// get literal value
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

    // check which instruction to execute
    // if there are bytecode instructions to run
    for (;;)
    {

// print instruction if in debug
#ifdef DEBUG_TRACE_EXECUTION
        disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif

        uint8_t instruction;

        switch (instruction = READ_BYTE())
        {
        case OP_CONSTANT:
        {
            // print constant vlaue
            Value constant = READ_CONSTANT();
            printValue(constant);
            break;
        }
        case OP_RETURN:
        {
            // end of func or program
            return INTERPRET_OK;
        }
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
}

// run bytecode interpreter
InterpretResult interpret(Chunk *chunk)
{
    // assign code chunk
    vm.chunk = chunk;

    // set pointer
    vm.ip = vm.chunk->code;

    // begin executing
    return run();
}