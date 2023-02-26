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

// binary ops: the only change is the operand; the do-while lets
// us define statements in the same scope without appending a
// semicolon for the actual macro call
#define BINARY_OP(op)     \
    do                    \
    {                     \
        double b = pop(); \
        double a = pop(); \
        push(a op b);     \
    } while (false)

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
        // addition
        case OP_ADD:
        {
            BINARY_OP(+);
            break;
        }
        // subtraction
        case OP_SUBTRACT:
        {
            BINARY_OP(-);
            break;
        }
        // multiplication
        case OP_MULTIPLY:
        {
            BINARY_OP(*);
            break;
        }
        // divison
        case OP_DIVIDE:
        {
            BINARY_OP(/);
            break;
        }
        case OP_NEGATE:
        {
            // just push a negative version of that value
            push(-pop());
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
#undef BINARY_OP
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