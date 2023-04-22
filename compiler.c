#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

typedef void (*ParseFn)(bool canAssign);

typedef struct
{
    // typedef for function w/o args and return value
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

typedef struct
{
    Token variable;
    int depth;
} Local;

typedef struct
{
    Local locals[UINT8_COUNT];
    int localCount;
    int scopeDepth;
} Compiler;

Parser parser;
Compiler *current = NULL;
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
    // todo: remove this so all errors can be logged
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

// emitJump + patchJump = emitLoop -> while loop
static void emitLoop(int loopStart)
{
    emitByte(OP_LOOP);

    int offset = currentChunk()->count - loopStart + 2;

    if (offset > UINT16_MAX)
        error("This loop's body is too large.");

    emitByte((offset >> 8) & 0xff);
    emitByte(offset & 0xff);
}

// make space for the else clause code, use placeholders
static int emitJump(uint8_t instruction)
{
    emitByte(instruction);
    emitByte(0xff);
    emitByte(0xff);
    return currentChunk()->count - 2;
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

// after making the space, return to where we came from
// reset the jump offset
static void patchJump(int offset)
{
    int jump = currentChunk()->count - offset - 2;

    if (jump > UINT16_MAX)
    {
        error("This code body is too large. Try breaking this code into functions");
    }

    currentChunk()->code[offset] = (jump >> 8) & 0xff;
    currentChunk()->code[offset + 1] = jump & 0xff;
}

// todo: document
static void initCompiler(Compiler *compiler)
{
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    current = compiler;
}

// add return and debug
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

// one level deeper
static void beginScope()
{
    current->scopeDepth++;
}

// discard local variables from locals array
// todo: optimize with OP_POPN to remove multiple locals
static void endScope()
{
    current->scopeDepth--;

    while (current->localCount > 0 && current->locals[current->localCount - 1].depth > current->scopeDepth)
    {
        emitByte(OP_POP);
        current->localCount--;
    }
}

// function signatures for recursive functions
static void expression();
static void statement();
static void declaration();
static ParseRule *getRule(TokenType type);
static void parsePrecedence(Precedence precedence);
// todo: fix, removing this makes a bug
static uint8_t identifierConstant(Token *name);
static int resolveLocal(Compiler *compiler, Token *variable);

// handle value op value expressions
static void binary(bool canAssign)
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
    case TOKEN_CARET:
        emitByte(OP_EXPONENT);
        break;
    case TOKEN_SLASH:
        emitByte(OP_DIVIDE);
        break;
    default:
        return;
    }
}

// handle nil and boolean keywords in pratt parser table
static void literal(bool canAssign)
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
static void grouping(bool canAssign)
{
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expecting ')' after expression.");
}

// convert string token to number
// todo: fix whacky stuff i did with numbers
static void number(bool canAssign)
{
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

// short circuit or
static void or_(bool canAssign)
{
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump = emitJump(OP_JUMP);

    patchJump(elseJump);
    emitByte(OP_POP);

    parsePrecedence(PREC_OR);
    patchJump(endJump);
}

// define constant of string from source
static void string(bool canAssign)
{
    // the +1 and -2 trim the quotation marks
    emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

static void namedVariable(Token variable, bool canAssign)
{
    uint8_t getOp, setOp;
    int arg = resolveLocal(current, &variable);

    if (arg != -1)
    {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    }
    else
    {
        arg = identifierConstant(&variable);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    if (canAssign && match(TOKEN_EQUAL))
    {
        expression();
        emitBytes(setOp, (uint8_t)arg);
    }
    else
    {
        emitBytes(getOp, (uint8_t)arg);
    }
}

// identify variables
static void variable(bool canAssign)
{
    namedVariable(parser.previous, canAssign);
}

// prefix expression
static void unary(bool canAssign)
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
    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);

    while (precedence <= getRule(parser.current.type)->precedence)
    {
        advance();

        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }

    // report an error if equal token is not consumed
    if (canAssign && match(TOKEN_EQUAL))
    {
        error("Invalid assignment target");
    }
}

// add to chunk constant table
static uint8_t identifierConstant(Token *name)
{
    return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

// check if two identifier token names equate
static bool identifiersEqual(Token *a, Token *b)
{
    if (a->length != b->length)
        return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

// look for local variable
static int resolveLocal(Compiler *compiler, Token *variable)
{
    for (int i = compiler->localCount - 1; i >= 0; i--)
    {
        Local *local = &compiler->locals[i];

        if (identifiersEqual(variable, &local->variable))
        {
            if (local->depth == -1)
            {
                error("Can't read local variable in initializer");
            }

            return i;
        }
    }

    return -1;
}

// define a local variable to point to a token
static void addLocal(Token name)
{
    // todo: change this? its not a bad
    if (current->localCount == UINT8_COUNT)
    {
        error("Too many local variables.");
        return;
    }

    Local *local = &current->locals[current->localCount++];
    local->variable = name;
    local->depth = -1;
}

// add a variable to the scope, define its depth, bail if at
// ground level 0
static void declareVariable()
{
    // variable is a global at level 0
    if (current->scopeDepth == 0)
        return;

    // create local variable
    Token *currVar = &parser.previous;
    addLocal(*currVar);

    // todo: refactor to not need -1
    // todo: faster approach than looping?
    for (int i = current->localCount - 1; i >= 0; i--)
    {
        Local *local = &current->locals[i];

        if (local->depth != -1 && local->depth < current->scopeDepth)
        {
            break;
        }

        if (identifiersEqual(currVar, &local->variable))
        {
            error("Variable already defined");
        }
    }
}

// requires next token to be an identifier
static uint8_t parseVariable(const char *errorMessage)
{
    consume(TOKEN_IDENTIFIER, errorMessage);

    declareVariable();

    // return dummy index for current scope
    if (current->scopeDepth > 0)
        return 0;

    return identifierConstant(&parser.previous);
}

//
static void markInitialized()
{
    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

// op instruction to store initial value for the snew variable
// make/mark variable available for use
static void defineVariable(uint8_t global)
{
    if (current->scopeDepth > 0)
    {
        markInitialized();
        return;
    }

    emitBytes(OP_DEFINE_GLOBAL, global);
}

// short circuit and
static void and_(bool canAssign)
{
    int endJump = emitJump(OP_JUMP_IF_FALSE);

    emitByte(OP_POP);
    parsePrecedence(PREC_AND);

    patchJump(endJump);
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
    [TOKEN_CARET] = {NULL, binary, PREC_FACTOR},
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, and_, PREC_AND},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FUNC] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL] = {literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, or_, PREC_OR},
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

// consume delcarations and statements in current block
static void block()
{
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
    {
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Expected closing brace: }");
}

// get variable name and value, default to nil if value isn't present
static void variableDeclaration()
{
    uint8_t global = parseVariable("Expected variable name");

    if (match(TOKEN_EQUAL))
    {
        expression();
    }
    else
    {
        emitByte(OP_NIL);
    }

    // todo: remove
    // expect var declaration to have semi colon
    consume(TOKEN_SEMICOLON, "Expected ;");

    defineVariable(global);
}

// expression followed by semi color
static void expressionStatement()
{
    expression();
    // todo: remove
    consume(TOKEN_SEMICOLON, "Expected ';'");
    emitByte(OP_POP);
}

// c style for loops
static void forStatement()
{
    // define a scope for this for loop to have local variables
    beginScope();

    // consume first statement, that inits a variable
    consume(TOKEN_LEFT_PAREN, "Expected ( after for");
    if (match(TOKEN_SEMICOLON))
    {
        // no initializer
    }
    else if (match(TOKEN_VAR))
    {
        // create variable
        variableDeclaration();
    }
    else
    {
        // parse function
        expressionStatement();
    }

    // condition statement
    int loopStart = currentChunk()->count;
    int exitJump = -1;
    if (!match(TOKEN_SEMICOLON))
    {
        expression();
        consume(TOKEN_SEMICOLON, "Expected ; in loop condition");

        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP);
    }

    consume(TOKEN_SEMICOLON, "Expected ;");
    consume(TOKEN_RIGHT_PAREN, "Expected ) to close the for clause");

    // increment statement
    if (!match(TOKEN_RIGHT_PAREN))
    {
        int codeJump = emitJump(OP_JUMP);
        int incrementStart = currentChunk()->count;
        expression();
        emitByte(OP_POP);
        consume(TOKEN_RIGHT_PAREN, "Expected ) to close the for clause");

        emitLoop(loopStart);
        loopStart = incrementStart;
        patchJump(codeJump);
    }

    // code in the for loop
    statement();
    emitLoop(loopStart);

    // patch the jump, remove conditional clause
    if (exitJump != -1)
    {
        patchJump(exitJump);
        emitByte(OP_POP);
    }

    // exit loop
    endScope();
}

// compile condition, backpatch for else/then
static void ifStatement()
{
    // todo: do not parenthesis in code?
    consume(TOKEN_LEFT_PAREN, "Expected ( after if.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expected ) after condition.");

    // if ... then move instruction pointer here
    // but also allocate space in the byte array for that code
    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);

    // parse the statement
    statement();

    // find place for vm.ip
    int elseJump = emitJump(OP_JUMP);

    // return
    patchJump(thenJump);
    emitByte(OP_POP);

    // there's a statement in the else clause
    // expressions are statements in blue
    if (match(TOKEN_ELSE))
    {
        statement();
    }

    // return
    patchJump(elseJump);
}

// make op code to print user's values
static void printStatement()
{
    expression();
    // todo: remove
    consume(TOKEN_SEMICOLON, "Expected ';'");
    emitByte(OP_PRINT);
}

// while loop
static void whileStatement()
{
    int loopStart = currentChunk()->count;

    // todo: remove?
    // evaluate condition
    consume(TOKEN_LEFT_PAREN, "Expected ( after while.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expected ) after while condition");

    // skip the body if condition is false
    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);

    // eval inside code
    statement();
    emitLoop(loopStart);

    // return
    patchJump(exitJump);
    emitByte(OP_POP);
}

static void synchronize()
{
    // todo: remove
    parser.panicMode = false;
    while (parser.current.type != TOKEN_EOF)
    {
        if (parser.previous.type == TOKEN_SEMICOLON)
            return;

        switch (parser.current.type)
        {
        case TOKEN_CLASS:
        case TOKEN_FUNC:
        case TOKEN_VAR:
        case TOKEN_FOR:
        case TOKEN_IF:
        case TOKEN_WHILE:
        case TOKEN_PRINT:
        case TOKEN_RETURN:
            // these items begin new statements
            return;
        default:; // nothing for now
        }

        advance();
    }
}

// supports variables or statements
static void declaration()
{
    if (match(TOKEN_VAR))
    {
        variableDeclaration();
    }
    else
    {
        statement();
    }

    if (parser.panicMode)
        synchronize();
}

// handles: funcs, prints, classes etc
static void statement()
{
    // programmer has a print statement
    if (match(TOKEN_PRINT))
    {
        printStatement();
    }
    else if (match(TOKEN_IF))
    {
        ifStatement();
    }
    else if (match(TOKEN_FOR))
    {
        forStatement();
    }
    else if (match(TOKEN_WHILE))
    {
        whileStatement();
    }
    else if (match(TOKEN_LEFT_BRACE))
    {
        beginScope();
        block();
        endScope();
    }
    else
    {
        expressionStatement();
    }
}

// compilation was successful if no error appeared
bool compile(const char *source, Chunk *chunk)
{
    // make a scanner to generate tokens from code
    initScanner(source);

    // todo: document
    Compiler compiler;
    initCompiler(&compiler);

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