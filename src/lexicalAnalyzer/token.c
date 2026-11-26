/*
 * Project: IFJ26 compiler
 * Authors: Martin Kubáň xkubanm00
 * AI-assisted scaffold.
 * Prompt: Implement token allocation and ownership helpers.
 */

#include "token.h"

#include "memory.h"

#include <stdlib.h>
#include <string.h>

/**
 * Creates new empty token structure with type TOKEN_INVALID
 * @return NULL if there was a problem when creating token,
 * otherwise return pointer to the created token
 */
Token *tokenCreate(void)
{
    Token *token = memoryAllocate(sizeof(Token));
    if (token == NULL) {
        return NULL;
    }

    token->type = TOKEN_INVALID;
    token->lexeme = NULL;
    token->lexemeLength = 0;
    token->location.line = 1;
    token->location.column = 1;
    token->value.intValue = 0;
    return token;
}

/**
 * Deallocates the token lexeme and destroys the token
 * @param token pointer to the token which will be destroyed
 */
void tokenDestroy(Token *token)
{
    if (token == NULL) {
        return;
    }

    free(token->lexeme);
    free(token);
}

/**
 * Copies the lexeme, replaces the previous token lexeme,
 * and updates lexemeLength.
 * @param token pointer to the token whose lexeme will be changed
 * @param lexeme new token lexeme
 * @return COMPILER_INTERNAL_ERROR if token or lexeme are NULL, or when memory allocation failed,
 * otherwise return COMPILER_OK
 */
CompilerStatus tokenSetLexeme(Token *token, const char *lexeme)
{
    if (token == NULL || lexeme == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    char *copy = memoryDuplicateString(lexeme);
    if (copy == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    free(token->lexeme);
    token->lexeme = copy;
    token->lexemeLength = strlen(copy);
    return COMPILER_OK;
}

