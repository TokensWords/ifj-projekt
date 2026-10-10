#include "parserExpressions.h"

static CompilerStatus parseIdentifierNode(SyntaxAnalyzer *syntaxAnalyzer,
                                          AbstractSyntaxNode **outNode)
{
    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    if (!isIdentifierToken(syntaxAnalyzer->currentToken)) {
        return COMPILER_SYNTAX_ERROR;
    }

    AbstractSyntaxNode *node = createNode(ABSTRACT_SYNTAX_IDENTIFIER,
                                          syntaxAnalyzer->currentToken->location);
    if (node == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    status = setNodeText(node, syntaxAnalyzer->currentToken->lexeme);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = advanceToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    *outNode = node;
    return COMPILER_OK;

cleanup:
    abstractSyntaxNodeDestroy(node);
    return status;
}

CompilerStatus parseTerm(SyntaxAnalyzer *syntaxAnalyzer,
                                AbstractSyntaxNode **outNode)
{
    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    if (outNode == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    *outNode = NULL;

    Token *token = syntaxAnalyzer->currentToken;
    if (token->type == TOKEN_IDENTIFIER) {
        return parseIdentifierNode(syntaxAnalyzer, outNode);
    }

    AbstractSyntaxNodeType nodeType;
    DataType inferredType;
    switch (token->type) {
    case TOKEN_INT_LITERAL:
        nodeType = ABSTRACT_SYNTAX_INT_LITERAL;
        inferredType = DATA_TYPE_INT;
        break;
    case TOKEN_DOUBLE_LITERAL:
        nodeType = ABSTRACT_SYNTAX_DOUBLE_LITERAL;
        inferredType = DATA_TYPE_DOUBLE;
        break;
    case TOKEN_STRING_LITERAL:
        nodeType = ABSTRACT_SYNTAX_STRING_LITERAL;
        inferredType = DATA_TYPE_STRING;
        break;
    case TOKEN_UNIT_LITERAL:
        nodeType = ABSTRACT_SYNTAX_UNIT_LITERAL;
        inferredType = DATA_TYPE_UNIT;
        break;
    default:
        return COMPILER_SYNTAX_ERROR;
    }

    AbstractSyntaxNode *node = createNode(nodeType, token->location);
    if (node == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    node->inferredType = inferredType;

    if (nodeType == ABSTRACT_SYNTAX_INT_LITERAL) {
        node->value.intValue = token->value.intValue;
    } else if (nodeType == ABSTRACT_SYNTAX_DOUBLE_LITERAL) {
        node->value.doubleValue = token->value.doubleValue;
    } else if (nodeType == ABSTRACT_SYNTAX_STRING_LITERAL) {
        status = setNodeText(node, token->lexeme);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
    }

    status = advanceToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    *outNode = node;
    return COMPILER_OK;

cleanup:
    abstractSyntaxNodeDestroy(node);
    return status;
}

CompilerStatus analyzerExpressionsParse(SyntaxAnalyzer *syntaxAnalyzer,
                                       AbstractSyntaxNode **outNode)
{
    if (syntaxAnalyzer == NULL || outNode == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    *outNode = NULL;
    return parseTerm(syntaxAnalyzer, outNode);
}

CompilerStatus parseFunctionCall(SyntaxAnalyzer *syntaxAnalyzer,
                                        AbstractSyntaxNode **outNode)
{
    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    if (!isIdentifierToken(syntaxAnalyzer->currentToken)) {
        return COMPILER_SYNTAX_ERROR;
    }

    AbstractSyntaxNode *call = createNode(ABSTRACT_SYNTAX_FUNCTION_CALL,
                                          syntaxAnalyzer->currentToken->location);
    if (call == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    AbstractSyntaxNode *argument = NULL;

    status = setNodeText(call, syntaxAnalyzer->currentToken->lexeme);
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

    if (!currentIsType(syntaxAnalyzer, TOKEN_RIGHT_PAREN)) {
        for (;;) {
            status = parseTerm(syntaxAnalyzer, &argument);
            if (status != COMPILER_OK) {
                goto cleanup;
            }
            status = appendOwnedChild(call, argument);
            argument = NULL;
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
    }

    status = expectType(syntaxAnalyzer, TOKEN_RIGHT_PAREN);
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    *outNode = call;
    return COMPILER_OK;

cleanup:
    abstractSyntaxNodeDestroy(argument);
    abstractSyntaxNodeDestroy(call);
    return status;
}

CompilerStatus analyzerExpressionsParseValueOrCall(SyntaxAnalyzer *syntaxAnalyzer,
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
    if (isIdentifierToken(syntaxAnalyzer->currentToken)) {
        status = ensureNextToken(syntaxAnalyzer);
        if (status != COMPILER_OK) {
            return status;
        }
        if (syntaxAnalyzer->nextToken != NULL &&
            syntaxAnalyzer->nextToken->type == TOKEN_LEFT_PAREN) {
            return parseFunctionCall(syntaxAnalyzer, outNode);
        }
    }
    return analyzerExpressionsParse(syntaxAnalyzer, outNode);
}
