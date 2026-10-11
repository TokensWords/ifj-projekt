/**
 * @file syntaxAnalyzer.c
 * @project IFJ26 compiler
 * @author Matúš Čík
 */

#include "syntaxAnalyzerUtils.h"

#include "syntax.h"

#include "memory.h"

#include <string.h>

static CompilerStatus readToken(SyntaxAnalyzer *syntaxAnalyzer, Token **outToken)
{
    *outToken = NULL;
    CompilerStatus status = lexicalAnalyzerNextToken(syntaxAnalyzer->lexicalAnalyzer, outToken);
    if (status != COMPILER_OK) {
        tokenDestroy(*outToken);
        *outToken = NULL;
        return status;
    }
    if (*outToken == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    return COMPILER_OK;
}

/* --- Loading and Advancing Tokens Functions --- */

CompilerStatus ensureCurrentToken(SyntaxAnalyzer *syntaxAnalyzer)
{
    if (syntaxAnalyzer->currentToken != NULL) {
        return COMPILER_OK;
    }
    return readToken(syntaxAnalyzer, &syntaxAnalyzer->currentToken);
}

CompilerStatus ensureNextToken(SyntaxAnalyzer *syntaxAnalyzer)
{
    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK || syntaxAnalyzer->currentToken->type == TOKEN_EOF ||
        syntaxAnalyzer->nextToken != NULL) {
        return status;
    }
    return readToken(syntaxAnalyzer, &syntaxAnalyzer->nextToken);
}

CompilerStatus advanceToken(SyntaxAnalyzer *syntaxAnalyzer)
{
    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    if (syntaxAnalyzer->currentToken->type == TOKEN_EOF) {
        return COMPILER_SYNTAX_ERROR;
    }

    if (syntaxAnalyzer->nextToken != NULL) {
        tokenDestroy(syntaxAnalyzer->currentToken);
        syntaxAnalyzer->currentToken = syntaxAnalyzer->nextToken;
        syntaxAnalyzer->nextToken = NULL;
        return COMPILER_OK;
    }

    tokenDestroy(syntaxAnalyzer->currentToken);
    syntaxAnalyzer->currentToken = NULL;
    return ensureCurrentToken(syntaxAnalyzer);
}

CompilerStatus skipNewlines(SyntaxAnalyzer *syntaxAnalyzer)
{
    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    while (status == COMPILER_OK && currentIsType(syntaxAnalyzer, TOKEN_NEWLINE)) {
        status = advanceToken(syntaxAnalyzer);
    }
    return status;
}

/* --- Current Token Validation Functions --- */

bool tokenHasText(const Token *token, const char *text)
{
    return token != NULL && token->lexeme != NULL && strcmp(token->lexeme, text) == 0;
}

bool currentIsWord(const SyntaxAnalyzer *syntaxAnalyzer, const char *word)
{
    return tokenHasText(syntaxAnalyzer->currentToken, word);
}

bool currentIsOperator(const SyntaxAnalyzer *syntaxAnalyzer, const char *operatorText)
{
    return tokenHasText(syntaxAnalyzer->currentToken, operatorText);
}

bool currentIsType(const SyntaxAnalyzer *syntaxAnalyzer, TokenType type)
{
    return syntaxAnalyzer->currentToken != NULL && syntaxAnalyzer->currentToken->type == type;
}

CompilerStatus expectWord(SyntaxAnalyzer *syntaxAnalyzer, const char *word)
{
    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    if (!currentIsWord(syntaxAnalyzer, word)) {
        return COMPILER_SYNTAX_ERROR;
    }
    return advanceToken(syntaxAnalyzer);
}

CompilerStatus expectType(SyntaxAnalyzer *syntaxAnalyzer, TokenType type)
{
    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    if (!currentIsType(syntaxAnalyzer, type)) {
        return COMPILER_SYNTAX_ERROR;
    }
    return advanceToken(syntaxAnalyzer);
}

/* --- Token Validation Functions --- */

bool isIdentifierToken(const Token *token)
{
    return token != NULL && token->type == TOKEN_IDENTIFIER && token->lexeme != NULL;
}

bool isExpressionTerminator(const Token *token)
{
    return token == NULL || token->type == TOKEN_EOF || token->type == TOKEN_NEWLINE ||
           token->type == TOKEN_DEDENT || token->type == TOKEN_COMMA ||
           token->type == TOKEN_SEMICOLON || token->type == TOKEN_RIGHT_PAREN ||
           tokenHasText(token, SYNTAX_KW_THEN) || tokenHasText(token, SYNTAX_KW_DO);
}

/* --- AST Node Functions wrappers with guaranteed cleanup after failure --- */

CompilerStatus appendOwnedChild(AbstractSyntaxNode *parent, AbstractSyntaxNode *child)
{
    if (parent == NULL || child == NULL) {
        abstractSyntaxNodeDestroy(child);
        return COMPILER_INTERNAL_ERROR;
    }
    CompilerStatus status = abstractSyntaxNodeAddChild(parent, child);
    if (status != COMPILER_OK) {
        abstractSyntaxNodeDestroy(child);
    }
    return status;
}

AbstractSyntaxNode *createNode(AbstractSyntaxNodeType type, SourceLocation location)
{
    return abstractSyntaxNodeCreate(type, location);
}

CompilerStatus setNodeText(AbstractSyntaxNode *node, const char *text)
{
    return abstractSyntaxNodeSetText(node, text);
}
