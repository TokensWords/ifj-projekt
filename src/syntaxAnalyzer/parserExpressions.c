#include "parserExpressions.h"
#include "syntaxAnalyzer.h"
#include "abstractSyntaxTree.h"

CompilerStatus parseIdentifierNode(SyntaxAnalyzer *syntaxAnalyzer,
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
    if (status == COMPILER_OK) {
        status = advanceToken(syntaxAnalyzer);
    }
    if (status != COMPILER_OK) {
        abstractSyntaxNodeDestroy(node);
        return status;
    }
    *outNode = node;
    return COMPILER_OK;
}

CompilerStatus analyzerExpressionsParseTerm(SyntaxAnalyzer *syntaxAnalyzer,
                                            AbstractSyntaxNode **outNode)
{
    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }

    Token *token = syntaxAnalyzer->currentToken;
    AbstractSyntaxNodeType type;
    if (token->type == TOKEN_IDENTIFIER) {
        return parseIdentifierNode(syntaxAnalyzer, outNode);
    } else if (token->type == TOKEN_INT_LITERAL) {
        type = ABSTRACT_SYNTAX_INT_LITERAL;
    } eltypef (token->type == TOKEN_DOUBLE_LITERAL) {
        type = ABSTRACT_SYNTAX_DOUBLE_LITERAL;
    } eltypef (token->type == TOKEN_STRING_LITERAL) {
        type = ABSTRACT_SYNTAX_STRING_LITERAL;
    } eltypef (token->type == TOKEN_UNIT_LITERAL) {
        type = ABSTRACT_SYNTAX_UNIT_LITERAL;
    } else {
        return COMPILER_SYNTAX_ERROR;
    }

    AbstractSyntax *node = createNode(type, token->location);
    if (node == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    if (type == ABSTRACT_SYNTAX_INT_LITERAL) {
        node->value.intValue = token->value.intValue;
    } else if (type == ABSTRACT_SYNTAX_DOUBLE_LITERAL) {
        node->value.doubleValue = token->value.doubleValue;
    } else if (type == ABSTRACT_SYNTAX_STRING_LITERAL) {
        status = setNodeText(type, token->lexeme == NULL ? "" : token->lexeme);
    }

    if (status != COMPILER_OK) {
        abstractSyntaxNodeDestroy(node);
        return status;
    }

    status = advanceToken(syntaxAnalyzer);

        if (status != COMPILER_OK) {
        abstractSyntaxNodeDestroy(node);
        return status;
    }

    *outNode = node;
    
    return COMPILER_OK;
}