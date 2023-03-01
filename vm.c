#include "common.h"
#include "compiler.h"
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
        // literal values
        case OP_CONSTANT:
        {
            // print constant vlaue
            // todo: optimize
            Value constant = READ_CONSTANT();
            push(constant);
            break;
        }
        // binary ops, arithametic
        case OP_ADD:
        {
            BINARY_OP(+);
            break;
        }
        case OP_SUBTRACT:
        {
            BINARY_OP(-);
            break;
        }
        case OP_MULTIPLY:
        {
            BINARY_OP(*);
            break;
        }
        case OP_DIVIDE:
        {
            BINARY_OP(/);
            break;
        }
        // urnary ops
        case OP_NEGATE:
        {
            // just push a negative version of that value
            // todo: just convert the number to negative
            push(-pop());
            break;
        }
        // eof, program, function
        case OP_RETURN:
        {
            printlnValue(pop());
            return INTERPRET_OK;
        }
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

// compile source to byte code
InterpretResult interpret(const char *source)
{
    compile(source);
    return INTERPRET_OK;
}