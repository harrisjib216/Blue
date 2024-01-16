#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "math.h"
#include "memory.h"
#include "vm.h"

VM vm;

// native functions
static Value clockNative(int argCount, Value *args)
{
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

// prints the contents of file
static Value printFileNative(int argCount, Value *args)
{
    char buf[1024];
    FILE *file;
    size_t nread;

    file = fopen(AS_CSTRING(args[0]), "r");
    if (file)
    {
        // print contnets 'sizeof buffer' bytes at a time
        while ((nread = fread(buf, 1, sizeof buf, file)) > 0)
        {
            fwrite(buf, 1, nread, stdout);
        }

        // deal with error
        if (ferror(file))
        {
            printf("Error printing: %s", AS_CSTRING(args[0]));
        }

        fclose(file);
    }
    else
    {
        // file does not exist
        fclose(file);
        printf("Error file does not exist: %s", AS_CSTRING(args[0]));
        exit(1);
    }

    return NUMBER_VAL(100);
}

// config: point stackTop to the beginning
static void resetStack()
{
    vm.stackTop = vm.stack;
    vm.frameCount = 0;
}

// todo: document
static void runtimeError(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    // print stack trace
    for (int i = vm.frameCount - 1; i > -1; i--)
    {
        CallFrame *frame = &vm.frames[i];
        ObjFunction *function = frame->function;

        size_t instruction = frame->ip - function->chunk.code - 1;

        fprintf(stderr, "[Line %d] in ", function->chunk.lines[instruction]);

        if (function->name == NULL)
        {
            fprintf(stderr, "script\n");
        }
        else
        {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }

    CallFrame *frame = &vm.frames[vm.frameCount - 1];
    size_t instruction = frame->ip - frame->function->chunk.code - 1;
    int line = frame->function->chunk.lines[instruction];
    fprintf(stderr, "[line %d] in script.\n", line);

    resetStack();
}

// push a native function onto the stack
static void defineNative(const char *name, NativeFunc function)
{
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function)));
    tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

// set up vm
void initVM()
{
    resetStack();
    vm.objects = NULL;
    initTable(&vm.globals);
    initTable(&vm.strings);

    // define more native funcs
    defineNative("clock", clockNative);
    defineNative("printFile", printFileNative);
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

// just "called" a function in the interpreter, so grow the stack
static bool call(ObjFunction *function, int argCount)
{
    if (argCount != function->arity)
    {
        runtimeError("Expected %d arguments but go %d.", function->arity, argCount);
        return false;
    }

    if (vm.frameCount == FRAMES_MAX)
    {
        runtimeError("Call stack is too large (Stack overflow..)");
        return false;
    }

    CallFrame *frame = &vm.frames[vm.frameCount++];
    frame->function = function;
    frame->ip = function->chunk.code;
    frame->slots = vm.stackTop - argCount - 1;
    return true;
}

// errors if not a function?
static bool callValue(Value callee, int argCount)
{
    if (IS_OBJ(callee))
    {
        switch (OBJ_TYPE(callee))
        {
        case OBJ_FUNCTION:
            return call(AS_FUNCTION(callee), argCount);
        case OBJ_NATIVE:
        {
            NativeFunc native = AS_NATIVE(callee);
            Value result = native(argCount, vm.stackTop - argCount);
            vm.stackTop -= argCount + 1;
            push(result);
            return true;
        }
        default:
            // non-callable object type
            break;
        }
    }

    runtimeError("You can only call functions and classes.");
    return false;
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
    CallFrame *frame = &vm.frames[vm.frameCount - 1];

// return pointer
#define READ_BYTE() (*frame->ip++)

// read from the padded space in the chunk/bytecode
#define READ_SHORT() \
    (frame->ip += 2, \
     (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))

// get literal value
#define READ_CONSTANT() \
    (frame->function->chunk.constants.values[READ_BYTE()])

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
        disassembleInstruction(
            &frame->function->chunk,
            (int)(frame->ip - frame->function->chunk.code));
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
            push(frame->slots[slot]);
            break;
        }
        case OP_SET_LOCAL:
        {
            uint8_t slot = READ_BYTE();
            frame->slots[slot] = peek(0);
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
            frame = &vm.frames[vm.frameCount - 1];
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
            frame->ip += offset;
            break;
        }
        case OP_JUMP_IF_FALSE:
        {
            uint16_t offset = READ_SHORT();
            if (isFalsey(peek(0)))
                frame->ip += offset;
            break;
        }
        case OP_LOOP:
        {
            uint16_t offset = READ_SHORT();
            frame->ip -= offset;
            break;
        }
        case OP_CALL:
        {
            int argCount = READ_BYTE();
            if (!callValue(peek(argCount), argCount))
            {
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        // eof, program, function
        case OP_RETURN:
        {
            Value value = pop();
            vm.frameCount--;

            if (vm.frameCount == 0)
            {
                pop();
                return INTERPRET_OK;
            }

            vm.stackTop = frame->slots;
            push(value);
            frame = &vm.frames[vm.frameCount - 1];
            break;
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
    // compile source code, get top level code/function
    ObjFunction *function = compile(source);

    // if null, there was a compile time error and we
    // dont have a starting place for the code
    if (function == NULL)
        return INTERPRET_COMPILE_ERROR;

    // begin executing
    call(function, 0);

    // run code
    return run();
}