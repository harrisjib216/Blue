#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

typedef struct
{
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

Parser parser;

Chunk *compilingChunk;

// get chunk we are compiling
static Chunk *currentChunk()
{
    return compilingChunk;
}

// print where the error occurred and its message
static void errorAt(Token *token, const char *message)
{
    // ignore other errors
    // todo: remove this so all errors can log
    if (parser.panicMode)
        return;

    // for exceptions
    parser.panicMode = true;

    // print error line
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF)
    {
        fprintf(stderr, " at end");
    }
    else if (token->type == TOKEN_ERROR)
    {
        // todo: figure out how to fix this
    }
    else
    {
        // print token if possible
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    // print our error message
    fprintf(stderr, ": %s\n", message);

    // inform we had an error
    parser.hadError = true;
}

// tell the user of error token from scanner
static void errorAtCurrent(const char *message)
{
    errorAt(&parser.current, message);
}

// "just found an error"
static void error(const char *message)
{
    errorAt(&parser.previous, message);
}

// advance to get next token
static void advance()
{
    parser.previous = parser.current;

    for (;;)
    {
        parser.current = scanToken();

        if (parser.current.type != TOKEN_ERROR)
            break;

        errorAtCurrent(parser.current.start);
    }
}

// read token and validate expected type
static void consume(TokenType type, const char *message)
{
    if (parser.current.type == type)
    {
        advance();
        return;
    }

    errorAtCurrent(message);
}

// translate parsed code to bytecode
static void emitByte(uint8_t byte)
{
    writeChunk(currentChunk(), byte, parser.previous.line);
}

// write opcode with one byte operand
static void emitBytes(uint8_t byte1, uint8_t byte2)
{
    emitByte(byte1);
    emitByte(byte2);
}

// temporary: print expression value
static void emitReturn()
{
    emitByte(OP_RETURN);
}

// finish compiler instructions?
static void endCompiler()
{
    emitReturn();
}

// convert string token to number
// todo: fix whacky stuff i did with numbers
static void number()
{
    double value = strtod(parser.previous.start, NULL);
    emitConstant(value);
}

//
static void expression()
{
}

// compilation was successful if no error appeared
bool compile(const char *source, Chunk *chunk)
{
    // make a scanner to generate tokens from code
    initScanner(source);

    // init chunk to compile
    compilingChunk = chunk;

    // initialize parser errors to false
    parser.hadError = false;
    parser.panicMode = false;

    // prime the pump
    advance();

    // parse single expression
    expression();

    // make sure we reach eof
    consume(TOKEN_EOF, "Expected end of expression");

    // finished compiling chunk
    endCompiler();

    return !parser.hadError;
}