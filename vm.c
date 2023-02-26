#include "common.h"
#include "debug.h"
#include "vm.h"

VM vm;

// config: point stackTop to the beginning
static void resetStack()
{
    vm.stackTop = vm.stack;
}

// set up vm
void initVM()
{
    resetStack();
}

// clear vm
void freeVM()
{
}

// append value
void push(Value value)
{
    *vm.stackTop = value;
    vm.stackTop++;
}

// remove element
Value pop()
{
    // since stackTop points to the next available item
    // we don't need to remove this value
    // we label it as available by pointing to it
    vm.stackTop--;
    return *vm.stackTop;
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
        // print stack values
        printStack(vm.stack, vm.stackTop);

        // print instruction with data
        disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif

        uint8_t instruction;

        switch (instruction = READ_BYTE())
        {
        case OP_CONSTANT:
        {
            // print constant vlaue
            // todo: optimize
            Value constant = READ_CONSTANT();
            push(constant);
            break;
        }
        case OP_RETURN:
        {
            // end of func or program
            printlnValue(pop());
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