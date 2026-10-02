/*
 * Project: IFJ26 compiler
 * Author: AI testing agent.
 * Prompt: Test Token defaults and owned lexeme copies.
 */

#include "token.h"

#include <assert.h>
#include <string.h>

int main(void)
{
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
    return 0;
}
