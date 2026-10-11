#include "parserFunctions.h"

#include "syntax.h"
#include "syntaxAnalyzerUtils.h"

#include <stdbool.h>

static CompilerStatus parseParameter(SyntaxAnalyzer *syntaxAnalyzer,
                                     AbstractSyntaxNode **outNode)
{
    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    if (!isIdentifierToken(syntaxAnalyzer->currentToken)) {
        return COMPILER_SYNTAX_ERROR;
    }

    AbstractSyntaxNode *parameter = createNode(ABSTRACT_SYNTAX_PARAMETER,
                                               syntaxAnalyzer->currentToken->location);
    if (parameter == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    status = setNodeText(parameter, syntaxAnalyzer->currentToken->lexeme);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = advanceToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = expectType(syntaxAnalyzer, TOKEN_COLON);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = analyzerTypesParseDataType(syntaxAnalyzer, &parameter->variableType);
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    *outNode = parameter;
    return COMPILER_OK;

cleanup:
    abstractSyntaxNodeDestroy(parameter);
    return status;
}

/* Validates and consumes an adjacent @main annotation. */
static CompilerStatus parseMainAnnotation(SyntaxAnalyzer *syntaxAnalyzer)
{
    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    if (!currentIsType(syntaxAnalyzer, TOKEN_AT)) {
        return COMPILER_SYNTAX_ERROR;
    }

    SourceLocation atLocation = syntaxAnalyzer->currentToken->location;
    status = ensureNextToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    if (!tokenHasText(syntaxAnalyzer->nextToken, SYNTAX_ANNOTATION_MAIN) ||
        syntaxAnalyzer->nextToken->location.line != atLocation.line ||
        syntaxAnalyzer->nextToken->location.column != atLocation.column + 1) {
        return COMPILER_SYNTAX_ERROR;
    }

    status = advanceToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }

    return expectWord(syntaxAnalyzer, SYNTAX_ANNOTATION_MAIN);
}

CompilerStatus parseFunction(SyntaxAnalyzer *syntaxAnalyzer,
                             AbstractSyntaxNode **outNode)
{
    if (syntaxAnalyzer == NULL || outNode == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    *outNode = NULL;

    bool isEntryFunction = false;
    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }

    if (currentIsType(syntaxAnalyzer, TOKEN_AT)) {
        status = parseMainAnnotation(syntaxAnalyzer);
        if (status != COMPILER_OK) {
            return status;
        }
        isEntryFunction = true;

        if (currentIsType(syntaxAnalyzer, TOKEN_NEWLINE)) {
            status = advanceToken(syntaxAnalyzer);
            if (status != COMPILER_OK) {
                return status;
            }
        }
    }

    status = expectWord(syntaxAnalyzer, SYNTAX_KW_DEF);
    if (status != COMPILER_OK) {
        return status;
    }
    if (!isIdentifierToken(syntaxAnalyzer->currentToken)) {
        return COMPILER_SYNTAX_ERROR;
    }

    AbstractSyntaxNode *function = createNode(ABSTRACT_SYNTAX_FUNCTION,
                                              syntaxAnalyzer->currentToken->location);
    AbstractSyntaxNode *body = NULL;
    AbstractSyntaxNode *bodyValue = NULL;
    AbstractSyntaxNode *statement = NULL;
    if (function == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    function->isEntryFunction = isEntryFunction;
    status = setNodeText(function, syntaxAnalyzer->currentToken->lexeme);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = advanceToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = expectType(syntaxAnalyzer, TOKEN_LEFT_PAREN);
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    while (!currentIsType(syntaxAnalyzer, TOKEN_RIGHT_PAREN)) {
        AbstractSyntaxNode *parameter = NULL;
        status = parseParameter(syntaxAnalyzer, &parameter);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        status = appendOwnedChild(function, parameter);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        if (!currentIsType(syntaxAnalyzer, TOKEN_COMMA)) {
            break;
        }

        size_t commaLine = syntaxAnalyzer->currentToken->location.line;
        status = advanceToken(syntaxAnalyzer);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        if (currentIsType(syntaxAnalyzer, TOKEN_RIGHT_PAREN)) {
            if (syntaxAnalyzer->currentToken->location.line <= commaLine) {
                status = COMPILER_SYNTAX_ERROR;
                goto cleanup;
            }
            break;
        }
    }

    status = expectType(syntaxAnalyzer, TOKEN_RIGHT_PAREN);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    if (currentIsType(syntaxAnalyzer, TOKEN_COLON)) {
        status = advanceToken(syntaxAnalyzer);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        status = analyzerTypesParseDataType(syntaxAnalyzer, &function->variableType);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
    }

    status = expectWord(syntaxAnalyzer, SYNTAX_OP_ASSIGN);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    if (currentIsType(syntaxAnalyzer, TOKEN_NEWLINE)) {
        status = advanceToken(syntaxAnalyzer);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        status = skipNewlines(syntaxAnalyzer);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        status = analyzerStatementsParseBlock(syntaxAnalyzer, &body);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
    } else {
        status = analyzerExpressionsParseValueOrCall(syntaxAnalyzer, &bodyValue);
        if (status != COMPILER_OK) {
            goto cleanup;
        }

        body = createNode(ABSTRACT_SYNTAX_BLOCK, bodyValue->location);
        if (body == NULL) {
            status = COMPILER_INTERNAL_ERROR;
            goto cleanup;
        }
        if (bodyValue->kind == ABSTRACT_SYNTAX_FUNCTION_CALL) {
            status = appendOwnedChild(body, bodyValue);
            bodyValue = NULL;
            if (status != COMPILER_OK) {
                goto cleanup;
            }
        } else {
            statement = createNode(ABSTRACT_SYNTAX_EXPRESSION_STATEMENT,
                                   bodyValue->location);
            if (statement == NULL) {
                status = COMPILER_INTERNAL_ERROR;
                goto cleanup;
            }
            status = appendOwnedChild(statement, bodyValue);
            bodyValue = NULL;
            if (status != COMPILER_OK) {
                goto cleanup;
            }
            status = appendOwnedChild(body, statement);
            statement = NULL;
            if (status != COMPILER_OK) {
                goto cleanup;
            }
        }
    }

    status = appendOwnedChild(function, body);
    body = NULL;
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    *outNode = function;
    function = NULL;

cleanup:
    abstractSyntaxNodeDestroy(statement);
    abstractSyntaxNodeDestroy(bodyValue);
    abstractSyntaxNodeDestroy(body);
    abstractSyntaxNodeDestroy(function);
    return status;
}
