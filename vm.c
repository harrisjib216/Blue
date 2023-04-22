#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "math.h"
#include "memory.h"
#include "vm.h"

VM vm;

// config: point stackTop to the beginning
static void resetStack()
{
    vm.stackTop = vm.stack;
}

// todo: document
static void runtimeError(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm.ip - vm.chunk->code - 1;

    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script.\n", line);

    resetStack();
}

// set up vm
void initVM()
{
    resetStack();
    vm.objects = NULL;
    initTable(&vm.globals);
    initTable(&vm.strings);
}

// clear vm
// todo: finish function
void freeVM()
{
    freeTable(&vm.globals);
    freeTable(&vm.strings);
    freeObjects();
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

// return element in stack
static Value peek(int distance)
{
    return vm.stackTop[-1 - distance];
}

// todo: define what our language considers falsey
static bool isFalsey(Value value)
{
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

// join strings
// todo: move this
static void concatenate()
{
    ObjString *b = AS_STRING(pop());
    ObjString *a = AS_STRING(pop());

    int length = a->length + b->length;

    char *newString = ALLOCATE(char, length + 1);

    // first copy a
    memcpy(newString, a->chars, a->length);

    // then copy b, a.length away from first space
    memcpy(newString + a->length, b->chars, b->length);
    newString[length] = '\0';

    ObjString *result = takeString(newString, length);
    push(OBJ_VAL(result));
}

// START OF THE RUN PROGRAM
static InterpretResult run()
{
// return pointer
#define READ_BYTE() (*vm.ip++)

// get literal value
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

// read from the padded space in the chunk/bytecode
#define READ_SHORT() \
    (vm.ip += 2, (uint16_t)((vm.ip[-2] << 8 | vm.ip[-1])))

// get string value
#define READ_STRING() AS_STRING(READ_CONSTANT())

// binary ops: the only change is the operand; the do-while lets
// us define statements in the same scope without appending a
// semicolon for the actual macro call (refactor to find the error yourself).
// task: both values in the stack are numbers and can produce a binary op
// otherwise, eject with runtime error. the wrapper or macro to use is
// the valueType prop
#define BINARY_OP(valueType, op)                        \
    do                                                  \
    {                                                   \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) \
        {                                               \
            runtimeError("Values must be numbers.");    \
            return INTERPRET_RUNTIME_ERROR;             \
        }                                               \
        double b = AS_NUMBER(pop());                    \
        double a = AS_NUMBER(pop());                    \
        push(valueType(a op b));                        \
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
        case OP_NIL:
        {
            push(NIL_VAL);
            break;
        }
        case OP_TRUE:
        {
            push(BOOL_VAL(true));
            break;
        }
        case OP_FALSE:
        {
            push(BOOL_VAL(false));
            break;
        }
        // instruction, forgets a value from the stack
        case OP_POP:
        {
            pop();
            break;
        }
        case OP_GET_LOCAL:
        {
            // push local value to give O(1) read time
            uint8_t slot = READ_BYTE();
            push(vm.stack[slot]);
            break;
        }
        case OP_SET_LOCAL:
        {
            uint8_t slot = READ_BYTE();
            vm.stack[slot] = peek(0);
            break;
        }
        case OP_GET_GLOBAL:
        {
            ObjString *name = READ_STRING();

            Value value;
            if (!tableGet(&vm.globals, name, &value))
            {
                runtimeError("Undefied variable: %s", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }

            push(value);
            break;
        }
        case OP_DEFINE_GLOBAL:
        {
            // places variable from constants into global table
            ObjString *varName = READ_STRING();
            tableSet(&vm.globals, varName, peek(0));
            pop();
            break;
        }
        case OP_SET_GLOBAL:
        {
            ObjString *name = READ_STRING();

            if (tableSet(&vm.globals, name, peek(0)))
            {
                // remove variable that we set if
                tableDelete(&vm.globals, name);
                runtimeError("Undefined variable: %s", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }

            break;
        }
        // logical, comparison
        case OP_EQUAL:
        {
            Value b = pop();
            Value a = pop();
            push(BOOL_VAL(valuesEquate(a, b)));
            break;
        }
        case OP_GREATER:
        {
            BINARY_OP(BOOL_VAL, >);
            break;
        }
        case OP_LESS:
        {
            BINARY_OP(BOOL_VAL, <);
            break;
        }
        // binary ops, arithametic
        case OP_ADD:
        {
            if (IS_STRING(peek(0)) && IS_STRING(peek(1)))
            {
                concatenate();
            }
            else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1)))
            {
                // todo: retry this BINARY_OP(NUMBER_VAL, +);
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(NUMBER_VAL(a + b));
            }
            else
            {
                runtimeError("Values must be two strings or numbers.");
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OP_SUBTRACT:
        {
            BINARY_OP(NUMBER_VAL, -);
            break;
        }
        case OP_MULTIPLY:
        {
            BINARY_OP(NUMBER_VAL, *);
            break;
        }
        case OP_DIVIDE:
        {
            BINARY_OP(NUMBER_VAL, /);
            break;
        }
        case OP_EXPONENT:
        {
            if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1)))
            {
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(NUMBER_VAL(pow(a, b)));
            }
            else
            {
                runtimeError("Values must be numbers.");
                return INTERPRET_RUNTIME_ERROR;
            }

            break;
        }
        // urnary ops
        case OP_NOT:
        {
            push(BOOL_VAL(isFalsey(pop())));
            break;
        }
        case OP_NEGATE:
        {
            // fail if not a number
            if (!IS_NUMBER(peek(0)))
            {
                runtimeError("The operand or value must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }

            // just push a negative version of that value
            // todo: just convert the number to negative
            push(NUMBER_VAL(-AS_NUMBER(pop())));
            break;
        }
        // statements
        case OP_PRINT:
        {
            printlnValue(pop());
            break;
        }
        case OP_JUMP:
        {
            uint16_t offset = READ_SHORT();
            vm.ip += offset;
            break;
        }
        case OP_JUMP_IF_FALSE:
        {
            uint16_t offset = READ_SHORT();
            if (isFalsey(peek(0)))
                vm.ip += offset;
            break;
        }
        case OP_LOOP:
        {
            uint16_t offset = READ_SHORT();
            vm.ip -= offset;
            break;
        }
        // eof, program, function
        case OP_RETURN:
        {
            return INTERPRET_OK;
        }
        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

// compile source to byte code
InterpretResult interpret(const char *source)
{
    // make code chunk
    Chunk chunk;
    initChunk(&chunk);

    // handle compilation errors
    if (!compile(source, &chunk))
    {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    // pass chunk to vm
    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    // run code
    InterpretResult result = run();
    freeChunk(&chunk);
    return result;
}