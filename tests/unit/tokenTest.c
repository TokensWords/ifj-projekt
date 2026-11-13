/*
 * Project: IFJ26 compiler
 * Author: AI testing agent.
 * Prompt: Test Token defaults and owned lexeme copies.
 */

#include "token.h"
#include "lexicalAnalyzer.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    Token *token = tokenCreate();
    assert(token != NULL);
    assert(token->type == TOKEN_INVALID);
    assert(token->lexeme == NULL);
    assert(token->lexemeLength == 0);
    assert(token->location.line == 1);
    assert(token->location.column == 1);

    char sourceLexeme[] = "identifier";
    assert(tokenSetLexeme(token, sourceLexeme) == COMPILER_OK);
    assert(token->lexeme != sourceLexeme);
    assert(strcmp(token->lexeme, "identifier") == 0);
    assert(token->lexemeLength == strlen("identifier"));

    sourceLexeme[0] = 'I';
    assert(strcmp(token->lexeme, "identifier") == 0);

    char *firstOwnedLexeme = token->lexeme;
    assert(tokenSetLexeme(token, "next") == COMPILER_OK);
    assert(strcmp(token->lexeme, "next") == 0);
    assert(token->lexemeLength == 4);
    assert(token->lexeme != firstOwnedLexeme);

    assert(tokenSetLexeme(NULL, "invalid") == COMPILER_INTERNAL_ERROR);
    assert(tokenSetLexeme(token, NULL) == COMPILER_INTERNAL_ERROR);

    tokenDestroy(token);
    tokenDestroy(NULL);

    FILE *input = tmpfile();
    assert(input != NULL);
    assert(fputs("true false\n", input) >= 0);
    rewind(input);

    LexicalAnalyzer *lexer = lexicalAnalyzerCreate(input);
    assert(lexer != NULL);
    Token *literal = NULL;
    assert(lexicalAnalyzerNextToken(lexer, &literal) == COMPILER_OK);
    assert(literal->type == TOKEN_BOOL_LITERAL);
    assert(literal->value.boolValue);
    tokenDestroy(literal);

    assert(lexicalAnalyzerNextToken(lexer, &literal) == COMPILER_OK);
    assert(literal->type == TOKEN_BOOL_LITERAL);
    assert(!literal->value.boolValue);
    tokenDestroy(literal);

    lexicalAnalyzerDestroy(lexer);
    fclose(input);
    return 0;
}
