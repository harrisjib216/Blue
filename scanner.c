#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

typedef struct
{
    const char *start;
    const char *current;
    int line;
} Scanner;

Scanner scanner;

// default scanner
void initScanner(const char *source)
{
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

// could be a number
static bool isDigit(char c)
{
    return c == '_' || (c >= '0' && c <= '9');
}

// could be a character
static bool isAlpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

// is at end
static bool isAtEnd()
{
    return *scanner.current == '\0';
}

// move up point to token ahead
// move back to return the previous char
static char advance()
{
    scanner.current++;
    return scanner.current[-1];
}

// get current char
static char peek()
{
    return *scanner.current;
}

// get next char
static char peekNext()
{
    if (isAtEnd())
        return '\0';
    return scanner.current[1];
}

// make a token based on type and current position
static Token makeToken(TokenType type)
{
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;

    return token;
}

// make a token with an error message
// todo: optimize
static Token errorToken(const char *message)
{
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner.line;

    return token;
}

// continuosly skip white-spaces in the source code
// we dont return in case there is more white space
static void skipWhitespace()
{
    for (;;)
    {
        switch (peek())
        {
        case ' ':
        case '\r':
        case '\t':
            // any white space
            advance();
            break;
        case '\n':
            // new lines
            scanner.line++;
            advance();
            break;
        case '/':
            if (peekNext() == '/')
            {
                // loop to end of comment
                while (peek() != '\n' && !isAtEnd())
                    advance();
            }
            else
            {
                // exit so scanToken can assess it
                return;
            }
            break;
        default:
            return;
        }
    }
}

// conclude if this keyword is ours
// otherwise return default identifier token
static TokenType checkKeyword(int start, int length, const char *rest, TokenType type)
{
    if (scanner.current - scanner.start == start + length && memcmp(scanner.start + start, rest, length) == 0)
    {
        return type;
    }

    return TOKEN_IDENTIFIER;
}

// return if token is ours or not
static TokenType identifierType()
{
    // todo: here?
    bool isNotOneChar = scanner.current - scanner.start > 1;

    switch (scanner.start[0])
    {
    case 'a':
        return checkKeyword(1, 2, "nd", TOKEN_AND);
    case 'c':
        return checkKeyword(1, 4, "lass", TOKEN_CLASS);
    case 'e':
        return checkKeyword(1, 3, "lse", TOKEN_ELSE);
    case 'f':
        if (isNotOneChar)
        {
            switch (scanner.start[1])
            {
            case 'a':
                return checkKeyword(2, 3, "lse", TOKEN_FALSE);
            case 'o':
                return checkKeyword(2, 1, "r", TOKEN_FOR);
            case 'u':
                return checkKeyword(2, 2, "nc", TOKEN_FUNC);
            }
        }
        break;
    case 'i':
        return checkKeyword(1, 1, "f", TOKEN_IF);
    case 'n':
        return checkKeyword(1, 2, "il", TOKEN_NIL);
    case 'o':
        return checkKeyword(1, 1, "r", TOKEN_OR);
    case 'p':
        return checkKeyword(1, 4, "rint", TOKEN_PRINT);
    case 'r':
        return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
    case 's':
        return checkKeyword(1, 4, "uper", TOKEN_SUPER);
    case 't':
        if (isNotOneChar)
        {
            switch (scanner.start[1])
            {
            case 'h':
                return checkKeyword(2, 2, "is", TOKEN_THIS);
            case 'r':
                return checkKeyword(2, 2, "ue", TOKEN_TRUE);
            }
        }
        break;
    case 'v':
        return checkKeyword(1, 2, "ar", TOKEN_VAR);
    case 'w':
        return checkKeyword(1, 4, "hile", TOKEN_WHILE);
    }

    return TOKEN_IDENTIFIER;
}

// user or blue reserved keywords
static Token identifier()
{
    while (isAlpha(peek()) || isDigit(peek()))
        advance();

    return makeToken(identifierType());
}

// string literals
static Token string()
{
    while (peek() != '"' && !isAtEnd())
    {
        if (peek() == '\n')
        {
            scanner.line++;
        }

        advance();
    }

    if (isAtEnd())
        return errorToken("Unterminated string.");

    // get to closing quote
    advance();
    return makeToken(TOKEN_STRING);
}

// number literals
// todo: make sure numbers are properly converted
static Token number()
{
    // consume all possible digits
    while (isDigit(peek()))
    {
        advance();
    }

    // consume if there are two decimals points
    if (peek() == '.' && isDigit(peekNext()))
    {
        advance();

        while (isDigit(peek()))
            advance();
    }

    // return number token
    return makeToken(TOKEN_NUMBER);
}

// check if current symbol equals symbol param
static bool match(char expected)
{
    // exit if at end or doesn't match
    if (isAtEnd() || *scanner.current != expected)
        return false;

    // move up if it does
    scanner.current++;

    // symbols equate
    return true;
}

// todo: sort functions
Token scanToken()
{
    skipWhitespace();

    // begin
    scanner.start = scanner.current;

    // exit if finished
    if (isAtEnd())
        return makeToken(TOKEN_EOF);

    char c = advance();

    // reserved and defined keywords
    if (isAlpha(c))
        return identifier();

    // handle numbers
    if (isDigit(c))
        return number();

    switch (c)
    {
    // one symbol token
    case '(':
        return makeToken(TOKEN_LEFT_PAREN);
    case ')':
        return makeToken(TOKEN_RIGHT_PAREN);
    case '{':
        return makeToken(TOKEN_LEFT_BRACE);
    case '}':
        return makeToken(TOKEN_RIGHT_BRACE);
    case ';':
        // todo: remove
        return makeToken(TOKEN_SEMICOLON);
    case ',':
        return makeToken(TOKEN_COMMA);
    case '.':
        return makeToken(TOKEN_DOT);
    case '-':
        return makeToken(TOKEN_MINUS);
    case '+':
        return makeToken(TOKEN_PLUS);
    case '/':
        return makeToken(TOKEN_SLASH);
    case '*':
        return makeToken(TOKEN_STAR);
    case '^':
        return makeToken(TOKEN_CARET);
    // two symbol token
    case '!':
        return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '=':
        return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '<':
        return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case '>':
        return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    // literal values
    case '"':
        return string();
    }

    return errorToken("Unexpected character.");
}