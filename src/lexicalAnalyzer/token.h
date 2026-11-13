/*
 * Project: IFJ26 compiler
 * Authors: xkubanm00
 * AI-assisted scaffold.
 * Prompt: Define the token contract shared by lexicalAnalyzer and syntaxAnalyzer.
 */

#ifndef TOKEN_H
#define TOKEN_H

#include "compilerTypes.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    TOKEN_INVALID,
    TOKEN_EOF,
    TOKEN_NEWLINE,
    TOKEN_INDENT,
    TOKEN_DEDENT,
    TOKEN_IDENTIFIER,
    TOKEN_INT_LITERAL,
    TOKEN_DOUBLE_LITERAL,
    TOKEN_STRING_LITERAL,
    TOKEN_BOOL_LITERAL,
    TOKEN_UNIT_LITERAL,
    TOKEN_KEYWORD,
    TOKEN_OPERATOR,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_SHARP_PAREN,
    TOKEN_RIGHT_SHARP_PAREN,
    TOKEN_COMMA,
    TOKEN_COLON,
    TOKEN_SEMICOLON,
    TOKEN_DOT,
    TOKEN_AT,
    TOKEN_UNDERSCORE
} TokenType;

typedef union {
    int intValue;
    double doubleValue;
    bool boolValue;
} TokenValue;

typedef struct {
    TokenType type;
    char *lexeme;
    size_t lexemeLength;
    SourceLocation location;
    TokenValue value;
} Token;

Token *tokenCreate(void);
void tokenDestroy(Token *token);
CompilerStatus tokenSetLexeme(Token *token, const char *lexeme);

#endif
