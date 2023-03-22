#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct
{
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

typedef enum
{
    PREC_NONE,
    PREC_ASSIGNMENT,
    PREC_OR,
    PREC_AND,
    PREC_EQUALITY,
    PREC_COMPARISON,
    PREC_TERM,
    PREC_FACTOR,
    PREC_UNARY,
    PREC_CALL,
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)();

typedef struct
{
    // typedef for function w/o args and return value
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

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

// return if current token matches type param
static bool check(TokenType type)
{
    return parser.current.type == type;
}

// if current token matches type param, consume
static bool match(TokenType type)
{
    if (!check(type))
        return false;

    advance();

    return true;
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

// add literal and type cast index
static uint8_t makeConstant(Value value)
{
    int constant = addConstant(currentChunk(), value);

    // ensure we don't exceed more than 256 constants
    if (constant > UINT8_MAX)
    {
        error("Too many literals in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

// append byte instruction of literal value
static void emitConstant(Value value)
{
    emitBytes(OP_CONSTANT, makeConstant(value));
}

// finish compiler instructions?
static void endCompiler()
{
    emitReturn();

#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError)
    {
        disassembleChunk(currentChunk(), "code");
    }
#endif
}

// function signatures for recursive functions
static void expression();
static void statement();
static void declaration();
static ParseRule *getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

// handle value op value expressions
static void binary()
{
    TokenType operatorType = parser.previous.type;
    ParseRule *rule = getRule(operatorType);

    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (operatorType)
    {
    case TOKEN_BANG_EQUAL:
        emitBytes(OP_EQUAL, OP_NOT);
        break;
    case TOKEN_EQUAL_EQUAL:
        emitByte(OP_EQUAL);
        break;
    case TOKEN_GREATER:
        emitByte(OP_GREATER);
        break;
    case TOKEN_GREATER_EQUAL:
        emitBytes(OP_LESS, OP_NOT);
        break;
    case TOKEN_LESS:
        emitByte(OP_LESS);
        break;
    case TOKEN_LESS_EQUAL:
        emitBytes(OP_GREATER, OP_NOT);
        break;
    case TOKEN_PLUS:
        emitByte(OP_ADD);
        break;
    case TOKEN_MINUS:
        emitByte(OP_SUBTRACT);
        break;
    case TOKEN_STAR:
        emitByte(OP_MULTIPLY);
        break;
    case TOKEN_SLASH:
        emitByte(OP_DIVIDE);
        break;
    default:
        return;
    }
}

// handle nil and boolean keywords in pratt parser table
static void literal()
{
    switch (parser.previous.type)
    {
    case TOKEN_NIL:
    {
        emitByte(OP_NIL);
        break;
    }
    case TOKEN_TRUE:
    {
        emitByte(OP_TRUE);
        break;
    }
    case TOKEN_FALSE:
    {
        emitByte(OP_FALSE);
        break;
    }
    default:
        return;
    }
}

// handle expressions in parenthesis
static void grouping()
{
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expecting ')' after expression.");
}

// convert string token to number
// todo: fix whacky stuff i did with numbers
static void number()
{
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

// define constant of string from source
static void string()
{
    // the +1 and -2 trim the quotation marks
    emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

// prefix expression
static void unary()
{
    TokenType operatorType = parser.previous.type;

    // compile operand
    parsePrecedence(PREC_UNARY);

    // write op instruction
    switch (operatorType)
    {
    case TOKEN_BANG:
        emitByte(OP_NOT);
        break;
    case TOKEN_MINUS:
        emitByte(OP_NEGATE);
        break;
    default:
        // unreachable
        return;
    }
}

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER] = {NULL, NULL, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, NULL, PREC_NONE},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FUNC] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL] = {literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, NULL, PREC_NONE},
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
    [TOKEN_THIS] = {NULL, NULL, PREC_NONE},
    [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
    [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

// compiles code in correct order rather than how things appear
static void parsePrecedence(Precedence precedence)
{
    // read next token
    advance();

    // look up parse rule
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;

    // user syntax error
    if (prefixRule == NULL)
    {
        error("Expected an expression.");
        return;
    }

    // compiles rest of prefix expression
    prefixRule();

    while (precedence <= getRule(parser.current.type)->precedence)
    {
        advance();

        ParseFn infixRule = getRule(parser.previous.type)->infix;

        infixRule();
    }
}

// return rule for function at index
static ParseRule *getRule(TokenType type)
{
    return &rules[type];
}

// compile expressions based on precedence
static void expression()
{
    parsePrecedence(PREC_ASSIGNMENT);
}

// make op code to print user's values
static void printStatement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expected ';'");
    emitByte(OP_PRINT);
}

// supports variables or statements
static void declaration()
{
    statement();
}

// handles: funcs, prints, classes etc
static void statement()
{
    // programmer has a print statement
    if (match(TOKEN_PRINT))
    {
        printStatement();
    }
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

    // scan all tokens for the program
    advance();

    // loop to gather all statements or expressions
    while (!match(TOKEN_EOF))
    {
        declaration();
    }

    // finished compiling chunk
    endCompiler();

    return !parser.hadError;
}