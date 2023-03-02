#include <stdio.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

bool compile(const char *source, Chunk *chunk)
{
    // generate tokens from code
    initScanner(source);

    // prime the pump
    advance();

    // parse single expression
    expression();

    // make sure we reach eof
    consume(TOKEN_EOF, "Expected end of expression");
}