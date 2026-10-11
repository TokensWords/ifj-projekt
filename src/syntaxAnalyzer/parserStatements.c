/**
 * @file parserStatements.c
 * @brief Implements parsing of statements and blocks of statements.
 *
 * @project IFJ26 compiler
 * @author Matúš Čík
 */

#include "compilerTypes.h"
#include "syntaxAnalyzer.h"
#include "syntaxAnalyzerUtils.h"
#include "syntax.h"
#include "parserStatements.h"

#include <string.h>

static CompilerStatus expectBlockHeaderEnd(SyntaxAnalyzer *syntaxAnalyzer)
{
    if (!currentIsType(syntaxAnalyzer, TOKEN_NEWLINE)) {
        return COMPILER_SYNTAX_ERROR;
    }
    CompilerStatus status = advanceToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }

    return skipNewlines(syntaxAnalyzer);
}

CompilerStatus parseIf(SyntaxAnalyzer *syntaxAnalyzer,
                       AbstractSyntaxNode **outNode)
{
    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    SourceLocation location = syntaxAnalyzer->currentToken->location;
    AbstractSyntaxNode *ifNode = createNode(ABSTRACT_SYNTAX_IF, location);
    if (ifNode == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    AbstractSyntaxNode *condition = NULL;
    AbstractSyntaxNode *thenBlock = NULL;
    AbstractSyntaxNode *elseBlock = NULL;

    status = advanceToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    status = analyzerExpressionsParse(syntaxAnalyzer, &condition);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = appendOwnedChild(ifNode, condition);
    condition = NULL;
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    status = expectWord(syntaxAnalyzer, SYNTAX_KW_THEN);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = expectBlockHeaderEnd(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    status = parseBlock(syntaxAnalyzer, &thenBlock);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = appendOwnedChild(ifNode, thenBlock);
    thenBlock = NULL;
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    status = skipNewlines(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = expectWord(syntaxAnalyzer, SYNTAX_KW_ELSE);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = expectBlockHeaderEnd(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    status = parseBlock(syntaxAnalyzer, &elseBlock);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = appendOwnedChild(ifNode, elseBlock);
    elseBlock = NULL;
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    *outNode = ifNode;
    return COMPILER_OK;

cleanup:
    abstractSyntaxNodeDestroy(condition);
    abstractSyntaxNodeDestroy(thenBlock);
    abstractSyntaxNodeDestroy(elseBlock);
    abstractSyntaxNodeDestroy(ifNode);
    return status;
}

CompilerStatus parseWhile(SyntaxAnalyzer *syntaxAnalyzer,
                          AbstractSyntaxNode **outNode)
{
    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    SourceLocation location = syntaxAnalyzer->currentToken->location;
    AbstractSyntaxNode *whileNode = createNode(ABSTRACT_SYNTAX_WHILE, location);
    if (whileNode == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    AbstractSyntaxNode *condition = NULL;
    AbstractSyntaxNode *body = NULL;

    status = advanceToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    status = analyzerExpressionsParse(syntaxAnalyzer, &condition);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = appendOwnedChild(whileNode, condition);
    condition = NULL;
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    status = expectWord(syntaxAnalyzer, SYNTAX_KW_DO);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = expectBlockHeaderEnd(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    status = parseBlock(syntaxAnalyzer, &body);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = appendOwnedChild(whileNode, body);
    body = NULL;
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    *outNode = whileNode;
    return COMPILER_OK;

cleanup:
    abstractSyntaxNodeDestroy(condition);
    abstractSyntaxNodeDestroy(body);
    abstractSyntaxNodeDestroy(whileNode);
    return status;
}

/* Statement dispatch and indentation-delimited blocks. */

CompilerStatus parseStatement(SyntaxAnalyzer *syntaxAnalyzer,
                              AbstractSyntaxNode **outNode)
{
    if (syntaxAnalyzer == NULL || outNode == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    *outNode = NULL;

    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }

    if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_VAL) ||
        currentIsWord(syntaxAnalyzer, SYNTAX_KW_VAR)) {
        return parseVariableDeclaration(syntaxAnalyzer, outNode);
    }
    if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_RETURN)) {
        return parseReturn(syntaxAnalyzer, outNode);
    }
    if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_IF)) {
        return parseIf(syntaxAnalyzer, outNode);
    }
    if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_WHILE)) {
        return parseWhile(syntaxAnalyzer, outNode);
    }
    if (isIdentifierToken(syntaxAnalyzer->currentToken)) {
        status = ensureNextToken(syntaxAnalyzer);
        if (status != COMPILER_OK) {
            return status;
        }
        if (syntaxAnalyzer->nextToken != NULL &&
            syntaxAnalyzer->nextToken->lexeme != NULL &&
            strcmp(syntaxAnalyzer->nextToken->lexeme, SYNTAX_OP_ASSIGN) == 0) {
            return parseAssignment(syntaxAnalyzer, outNode);
        }
    }

    status = analyzerExpressionsParseValueOrCall(syntaxAnalyzer, outNode);
    if (status != COMPILER_OK) {
        return status;
    }
    if (*outNode == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    if ((*outNode)->type == ABSTRACT_SYNTAX_FUNCTION_CALL) {
        return COMPILER_OK;
    }

    AbstractSyntaxNode *expression = *outNode;
    AbstractSyntaxNode *statement = createNode(ABSTRACT_SYNTAX_EXPRESSION_STATEMENT,
                                               expression->location);
    if (statement == NULL) {
        abstractSyntaxNodeDestroy(expression);
        *outNode = NULL;
        return COMPILER_INTERNAL_ERROR;
    }
    status = appendOwnedChild(statement, expression);
    if (status != COMPILER_OK) {
        abstractSyntaxNodeDestroy(statement);
        *outNode = NULL;
        return status;
    }
    *outNode = statement;
    return COMPILER_OK;
}

CompilerStatus parseBlock(SyntaxAnalyzer *syntaxAnalyzer,
                          AbstractSyntaxNode **outNode)
{
    if (syntaxAnalyzer == NULL || outNode == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    *outNode = NULL;

    CompilerStatus status = expectType(syntaxAnalyzer, TOKEN_INDENT);
    if (status != COMPILER_OK) {
        return status;
    }

    SourceLocation location = syntaxAnalyzer->currentToken->location;
    AbstractSyntaxNode *block = createNode(ABSTRACT_SYNTAX_BLOCK, location);
    if (block == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    AbstractSyntaxNode *statement = NULL;
    status = skipNewlines(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    while (!currentIsType(syntaxAnalyzer, TOKEN_DEDENT) &&
           !currentIsType(syntaxAnalyzer, TOKEN_EOF)) {
        status = parseStatement(syntaxAnalyzer, &statement);
        if (status != COMPILER_OK) {
            goto cleanup;
        }

        status = appendOwnedChild(block, statement);
        statement = NULL;
        if (status != COMPILER_OK) {
            goto cleanup;
        }

        if (currentIsType(syntaxAnalyzer, TOKEN_NEWLINE)) {
            status = skipNewlines(syntaxAnalyzer);
            if (status != COMPILER_OK) {
                goto cleanup;
            }
        } else if (!currentIsType(syntaxAnalyzer, TOKEN_DEDENT)) {
            status = COMPILER_SYNTAX_ERROR;
            goto cleanup;
        }
    }

    if (block->childCount == 0) {
        status = COMPILER_SYNTAX_ERROR;
        goto cleanup;
    }

    status = expectType(syntaxAnalyzer, TOKEN_DEDENT);
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    *outNode = block;
    return COMPILER_OK;

cleanup:
    abstractSyntaxNodeDestroy(statement);
    abstractSyntaxNodeDestroy(block);
    return status;
}
