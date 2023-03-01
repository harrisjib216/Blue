#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

static void repl()
{
    // make repl length 1024
    char line[1024];

    // interpret each line until user quits
    for (;;)
    {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin))
        {
            printf("\n");
            break;
        }

        interpret(line);
    }
}

static char *readFile(const char *path)
{
    // open file
    FILE *file = fopen(path, "rb");

    // handle errors
    if (file == NULL)
    {
        fprintf(stderr, "Could not open this file: %s\n", path);
        exit(74);
    }

    // get size of file by counting how many bytes we've passed
    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    // copy contents of file into heap, append EOF character
    char *buffer = (char *)malloc(fileSize + 1);

    // not enough memory to read file
    if (buffer == NULL)
    {
        fprintf(stderr, "Not enough memory to read file: %s\nThis file is %lu bytes", path, fileSize + 1);
        exit(74);
    }

    // write file to buffer
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);

    // if read fails
    if (bytesRead < fileSize)
    {
        fprintf(stderr, "Could not read this file: %s\n", path);
        exit(74);
    }

    // append eof char
    buffer[bytesRead] = '\0';

    // close and pass file
    fclose(file);
    return buffer;
}

static void runFile(const char *path)
{
    // dynamically allocates and passes ownership
    char *file = readFile(path);

    // convert to byte code
    InterpretResult result = interpret(file);

    // clear file since we have our program
    free(file);

    // exit if errors
    if (result == INTERPRET_COMPILE_ERROR)
        exit(65);
    if (result == INTERPRET_RUNTIME_ERROR)
        exit(70);
}

int main(int argCount, const char *args[])
{
    // initialize vm
    initVM();

    // run repl, source file, or throw error
    if (argCount == 1)
    {
        repl();
    }
    else if (argCount == 2)
    {
        runFile(args[1]);
    }
    else
    {
        fprintf(stderr, "Usage: blue [file path]\n");
        exit(64);
    }

    // free vm and code
    freeVM();

    return 0;
}