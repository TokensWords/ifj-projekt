/**
 * @file  parserExpressions.c
 *
 * @project IFJ26 compiler
 * @author Matúš Čík
 */

#include "parserExpressions.h"
#include "parserTypes.h"

#include "syntaxAnalyzerUtils.h"

#include "memory.h"
#include "syntax.h"
#include "syntaxOperators.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static CompilerStatus parseIfExpressionBranch(SyntaxAnalyzer *syntaxAnalyzer,
                                              AbstractSyntaxNode **outNode, bool *outIsMultiline,
                                              bool isElseBranch) {
    if (syntaxAnalyzer == NULL || outNode == NULL || outIsMultiline == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    *outNode = NULL;
    *outIsMultiline = false;

    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    if (currentIsType(syntaxAnalyzer, TOKEN_NEWLINE)) {
        status = skipNewlines(syntaxAnalyzer);
        if (status != COMPILER_OK) {
            return status;
        }
        if (!currentIsType(syntaxAnalyzer, TOKEN_INDENT)) {
            return COMPILER_SYNTAX_ERROR;
        }
        status = advanceToken(syntaxAnalyzer);
        if (status != COMPILER_OK) {
            return status;
        }
        *outIsMultiline = true;
    }

    status = analyzerExpressionsParse(syntaxAnalyzer, outNode);
    if (status != COMPILER_OK) {
        return status;
    }

    if (*outIsMultiline) {
        if (!currentIsType(syntaxAnalyzer, TOKEN_NEWLINE)) {
            abstractSyntaxNodeDestroy(*outNode);
            *outNode = NULL;
            return COMPILER_SYNTAX_ERROR;
        }
        status = skipNewlines(syntaxAnalyzer);
        if (status != COMPILER_OK) {
            abstractSyntaxNodeDestroy(*outNode);
            *outNode = NULL;
            return status;
        }
        if (!currentIsType(syntaxAnalyzer, TOKEN_DEDENT)) {
            abstractSyntaxNodeDestroy(*outNode);
            *outNode = NULL;
            return COMPILER_SYNTAX_ERROR;
        }
        status = advanceToken(syntaxAnalyzer);
        if (status != COMPILER_OK) {
            abstractSyntaxNodeDestroy(*outNode);
            *outNode = NULL;
            return status;
        }
        if (isElseBranch) {
            syntaxAnalyzer->newlineConsumed = true;
        }
    }

    return COMPILER_OK;
}

static CompilerStatus parseIfExpression(SyntaxAnalyzer *syntaxAnalyzer,
                                        AbstractSyntaxNode **outNode) {
    if (syntaxAnalyzer == NULL || outNode == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    *outNode = NULL;

    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    SourceLocation location = syntaxAnalyzer->currentToken->location;
    AbstractSyntaxNode *ifExpression = createNode(ABSTRACT_SYNTAX_IF_EXPRESSION, location);
    AbstractSyntaxNode *condition = NULL;
    AbstractSyntaxNode *thenExpression = NULL;
    AbstractSyntaxNode *elseExpression = NULL;
    bool thenIsMultiline = false;
    bool elseIsMultiline = false;
    bool previousConsumedNewline = syntaxAnalyzer->newlineConsumed;
    syntaxAnalyzer->newlineConsumed = false;
    if (ifExpression == NULL) {
        syntaxAnalyzer->newlineConsumed = previousConsumedNewline;
        return COMPILER_INTERNAL_ERROR;
    }

    status = advanceToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = analyzerExpressionsParse(syntaxAnalyzer, &condition);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    syntaxAnalyzer->newlineConsumed = false;

    status = expectWord(syntaxAnalyzer, SYNTAX_KW_THEN);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = appendOwnedChild(ifExpression, condition);
    condition = NULL;
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    status = parseIfExpressionBranch(syntaxAnalyzer, &thenExpression, &thenIsMultiline, false);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = appendOwnedChild(ifExpression, thenExpression);
    thenExpression = NULL;
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    syntaxAnalyzer->newlineConsumed = false;

    if (currentIsType(syntaxAnalyzer, TOKEN_NEWLINE)) {
        status = skipNewlines(syntaxAnalyzer);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
    }
    status = expectWord(syntaxAnalyzer, SYNTAX_KW_ELSE);
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    status = parseIfExpressionBranch(syntaxAnalyzer, &elseExpression, &elseIsMultiline, true);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = appendOwnedChild(ifExpression, elseExpression);
    elseExpression = NULL;
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    syntaxAnalyzer->newlineConsumed =
        previousConsumedNewline || elseIsMultiline || syntaxAnalyzer->newlineConsumed;
    *outNode = ifExpression;
    return COMPILER_OK;

cleanup:
    abstractSyntaxNodeDestroy(condition);
    abstractSyntaxNodeDestroy(thenExpression);
    abstractSyntaxNodeDestroy(elseExpression);
    abstractSyntaxNodeDestroy(ifExpression);
    syntaxAnalyzer->newlineConsumed = previousConsumedNewline;
    return status;
}

CompilerStatus parseIdentifierNode(SyntaxAnalyzer *syntaxAnalyzer, AbstractSyntaxNode **outNode) {
    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    if (!isIdentifierToken(syntaxAnalyzer->currentToken)) {
        return COMPILER_SYNTAX_ERROR;
    }

    AbstractSyntaxNode *node =
        createNode(ABSTRACT_SYNTAX_IDENTIFIER, syntaxAnalyzer->currentToken->location);
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

CompilerStatus parseTerm(SyntaxAnalyzer *syntaxAnalyzer, AbstractSyntaxNode **outNode) {
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

    /* Unit literal is represented by two lexer tokens: `TOKEN_LEFT_PAREN` and `TOKEN_RIGHT_PAREN`
     */
    if (token->type == TOKEN_LEFT_PAREN) {
        status = ensureNextToken(syntaxAnalyzer);
        if (status != COMPILER_OK) {
            return status;
        }
        if (syntaxAnalyzer->nextToken == NULL ||
            syntaxAnalyzer->nextToken->type != TOKEN_RIGHT_PAREN) {
            return COMPILER_SYNTAX_ERROR;
        }

        AbstractSyntaxNode *unit = createNode(ABSTRACT_SYNTAX_UNIT_LITERAL, token->location);
        if (unit == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }
        status = abstractSyntaxNodeSetInferredType(unit, DATA_TYPE_UNIT);
        if (status != COMPILER_OK) {
            abstractSyntaxNodeDestroy(unit);
            return status;
        }

        status = advanceToken(syntaxAnalyzer);
        if (status == COMPILER_OK) {
            status = advanceToken(syntaxAnalyzer);
        }
        if (status != COMPILER_OK) {
            abstractSyntaxNodeDestroy(unit);
            return status;
        }
        *outNode = unit;
        return COMPILER_OK;
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
    case TOKEN_BOOL_LITERAL:
        nodeType = ABSTRACT_SYNTAX_BOOLEAN_LITERAL;
        inferredType = DATA_TYPE_BOOLEAN;
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
    status = abstractSyntaxNodeSetInferredType(node, inferredType);
    if (status != COMPILER_OK) {
        abstractSyntaxNodeDestroy(node);
        return status;
    }

    if (nodeType == ABSTRACT_SYNTAX_INT_LITERAL) {
        node->value.intValue = token->value.intValue;
    } else if (nodeType == ABSTRACT_SYNTAX_DOUBLE_LITERAL) {
        node->value.doubleValue = token->value.doubleValue;
    } else if (nodeType == ABSTRACT_SYNTAX_STRING_LITERAL) {
        status = setNodeText(node, token->lexeme);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
    } else if (nodeType == ABSTRACT_SYNTAX_BOOLEAN_LITERAL) {
        node->value.boolValue = token->value.boolValue;
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

static CompilerStatus parseCallWithCallee(SyntaxAnalyzer *syntaxAnalyzer,
                                          AbstractSyntaxNode **callee,
                                          AbstractSyntaxNode **outCall) {
    if (syntaxAnalyzer == NULL || callee == NULL || *callee == NULL || outCall == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    *outCall = NULL;
    CompilerStatus status = COMPILER_OK;
    AbstractSyntaxNode *call = NULL;
    AbstractSyntaxNode *argument = NULL;
    call = createNode(ABSTRACT_SYNTAX_CALL_EXPRESSION, (*callee)->location);
    if (call == NULL) {
        status = COMPILER_INTERNAL_ERROR;
        goto cleanup;
    }
    status = appendOwnedChild(call, *callee);
    *callee = NULL;
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = expectType(syntaxAnalyzer, TOKEN_LEFT_PAREN);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    while (!currentIsType(syntaxAnalyzer, TOKEN_RIGHT_PAREN)) {
        status = analyzerExpressionsParse(syntaxAnalyzer, &argument);
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
    status = expectType(syntaxAnalyzer, TOKEN_RIGHT_PAREN);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    *outCall = call;
    call = NULL;
    return COMPILER_OK;

cleanup:
    abstractSyntaxNodeDestroy(argument);
    abstractSyntaxNodeDestroy(call);
    return status;
}

static CompilerStatus parseExpressionPostfix(SyntaxAnalyzer *syntaxAnalyzer,
                                             AbstractSyntaxNode **expression, bool grouped) {
    CompilerStatus status = COMPILER_OK;
    AbstractSyntaxNode *member = NULL;
    AbstractSyntaxNode *call = NULL;
    bool canCall = grouped || (*expression)->kind == ABSTRACT_SYNTAX_IDENTIFIER ||
                   (*expression)->kind == ABSTRACT_SYNTAX_MEMBER_ACCESS;
    for (;;) {
        if (currentIsType(syntaxAnalyzer, TOKEN_DOT)) {
            SourceLocation location = syntaxAnalyzer->currentToken->location;
            status = advanceToken(syntaxAnalyzer);
            if (status != COMPILER_OK) {
                goto cleanup;
            }
            if (!isIdentifierToken(syntaxAnalyzer->currentToken)) {
                status = COMPILER_SYNTAX_ERROR;
                goto cleanup;
            }
            member = createNode(ABSTRACT_SYNTAX_MEMBER_ACCESS, location);
            if (member == NULL) {
                status = COMPILER_INTERNAL_ERROR;
                goto cleanup;
            }
            status = setNodeText(member, syntaxAnalyzer->currentToken->lexeme);
            if (status == COMPILER_OK) {
                status = advanceToken(syntaxAnalyzer);
            }
            if (status == COMPILER_OK) {
                status = appendOwnedChild(member, *expression);
                *expression = NULL;
            }
            if (status != COMPILER_OK) {
                goto cleanup;
            }
            *expression = member;
            member = NULL;
            canCall = true;
            continue;
        }
        if (currentIsType(syntaxAnalyzer, TOKEN_LEFT_PAREN)) {
            if (!canCall) {
                status = COMPILER_SYNTAX_ERROR;
                goto cleanup;
            }
            status = parseCallWithCallee(syntaxAnalyzer, expression, &call);
            if (status != COMPILER_OK) {
                goto cleanup;
            }
            *expression = call;
            call = NULL;
            canCall = false;
            continue;
        }
        break;
    }

cleanup:
    abstractSyntaxNodeDestroy(member);
    abstractSyntaxNodeDestroy(call);
    return status;
}

static CompilerStatus parseExpressionOperand(SyntaxAnalyzer *syntaxAnalyzer,
                                             AbstractSyntaxNode **outNode) {
    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    if (outNode == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    *outNode = NULL;
    bool grouped = false;
    if (currentIsType(syntaxAnalyzer, TOKEN_LEFT_PAREN)) {
        SourceLocation location = syntaxAnalyzer->currentToken->location;
        status = ensureNextToken(syntaxAnalyzer);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        if (syntaxAnalyzer->nextToken != NULL &&
            syntaxAnalyzer->nextToken->type == TOKEN_RIGHT_PAREN) {
            AbstractSyntaxNode *unit = createNode(ABSTRACT_SYNTAX_UNIT_LITERAL, location);
            if (unit == NULL) {
                status = COMPILER_INTERNAL_ERROR;
                goto cleanup;
            }
            status = abstractSyntaxNodeSetInferredType(unit, DATA_TYPE_UNIT);
            if (status != COMPILER_OK) {
                abstractSyntaxNodeDestroy(unit);
                goto cleanup;
            }
            *outNode = unit;
            status = advanceToken(syntaxAnalyzer);
            if (status == COMPILER_OK) {
                status = advanceToken(syntaxAnalyzer);
            }
            if (status != COMPILER_OK) {
                goto cleanup;
            }
            status = parseExpressionPostfix(syntaxAnalyzer, outNode, true);
            if (status != COMPILER_OK) {
                goto cleanup;
            }
            return COMPILER_OK;
        }
        status = advanceToken(syntaxAnalyzer);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        status = analyzerExpressionsParse(syntaxAnalyzer, outNode);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        status = expectType(syntaxAnalyzer, TOKEN_RIGHT_PAREN);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        grouped = true;
    } else if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_LIST)) {
        AbstractSyntaxNode *identifier =
            createNode(ABSTRACT_SYNTAX_IDENTIFIER, syntaxAnalyzer->currentToken->location);
        if (identifier == NULL) {
            status = COMPILER_INTERNAL_ERROR;
            goto cleanup;
        }
        *outNode = identifier;
        status = setNodeText(identifier, syntaxAnalyzer->currentToken->lexeme);
        if (status == COMPILER_OK) {
            status = advanceToken(syntaxAnalyzer);
        }
        if (status != COMPILER_OK) {
            goto cleanup;
        }
    } else {
        status = parseTerm(syntaxAnalyzer, outNode);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
    }
    status = parseExpressionPostfix(syntaxAnalyzer, outNode, grouped);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    return COMPILER_OK;

cleanup:
    abstractSyntaxNodeDestroy(*outNode);
    *outNode = NULL;
    return status;
}

CompilerStatus parseFunctionCall(SyntaxAnalyzer *syntaxAnalyzer, AbstractSyntaxNode **outNode) {
    if (syntaxAnalyzer == NULL || outNode == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    *outNode = NULL;
    AbstractSyntaxNode *callee = NULL;
    CompilerStatus status = parseIdentifierNode(syntaxAnalyzer, &callee);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    AbstractSyntaxNode *call = NULL;
    status = parseCallWithCallee(syntaxAnalyzer, &callee, &call);
    if (status != COMPILER_OK) {
        abstractSyntaxNodeDestroy(call);
        goto cleanup;
    }
    *outNode = call;
    return COMPILER_OK;

cleanup:
    abstractSyntaxNodeDestroy(callee);
    return status;
}

CompilerStatus analyzerExpressionsParseValueOrCall(SyntaxAnalyzer *syntaxAnalyzer,
                                                   AbstractSyntaxNode **outNode) {
    if (syntaxAnalyzer == NULL || outNode == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    *outNode = NULL;

    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    return analyzerExpressionsParse(syntaxAnalyzer, outNode);
}

static CompilerStatus parseMatchPattern(SyntaxAnalyzer *syntaxAnalyzer,
                                        AbstractSyntaxNode **outPattern, bool allowUntypedBinding) {
    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    *outPattern = NULL;

    Token *token = syntaxAnalyzer->currentToken;
    if (token->type == TOKEN_UNDERSCORE) {
        AbstractSyntaxNode *pattern = createNode(ABSTRACT_SYNTAX_IDENTIFIER, token->location);
        if (pattern == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }
        status = setNodeText(pattern, token->lexeme);
        if (status == COMPILER_OK) {
            status = advanceToken(syntaxAnalyzer);
        }
        if (status != COMPILER_OK) {
            abstractSyntaxNodeDestroy(pattern);
            return status;
        }
        *outPattern = pattern;
        return COMPILER_OK;
    }

    bool isLiteral = token->type == TOKEN_INT_LITERAL || token->type == TOKEN_DOUBLE_LITERAL ||
                     token->type == TOKEN_STRING_LITERAL || token->type == TOKEN_BOOL_LITERAL ||
                     token->type == TOKEN_UNIT_LITERAL || token->type == TOKEN_LEFT_PAREN;
    bool isSignedLiteral = token->type == TOKEN_OPERATOR && (tokenHasText(token, SYNTAX_OP_PLUS) ||
                                                             tokenHasText(token, SYNTAX_OP_MINUS));
    if (isLiteral || isSignedLiteral) {
        AbstractSyntaxNode *literal = NULL;
        if (isSignedLiteral) {
            status = analyzerExpressionsParse(syntaxAnalyzer, &literal);
            if (status == COMPILER_OK &&
                (literal->kind != ABSTRACT_SYNTAX_UNARY_EXPRESSION || literal->childCount != 1 ||
                 (literal->children[0]->kind != ABSTRACT_SYNTAX_INT_LITERAL &&
                  literal->children[0]->kind != ABSTRACT_SYNTAX_DOUBLE_LITERAL))) {
                status = COMPILER_SYNTAX_ERROR;
            }
        } else {
            status = parseTerm(syntaxAnalyzer, &literal);
        }
        if (status != COMPILER_OK) {
            abstractSyntaxNodeDestroy(literal);
            return status;
        }
        *outPattern = literal;
        return COMPILER_OK;
    }

    if (!isIdentifierToken(token)) {
        return COMPILER_SYNTAX_ERROR;
    }

    SourceLocation location = token->location;
    AbstractSyntaxNode *identifier = createNode(ABSTRACT_SYNTAX_IDENTIFIER, location);
    AbstractSyntaxNode *constructor = NULL;
    if (identifier == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    status = setNodeText(identifier, token->lexeme);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = advanceToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    if (currentIsType(syntaxAnalyzer, TOKEN_COLON)) {
        status = advanceToken(syntaxAnalyzer);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        status = analyzerTypesParseType(syntaxAnalyzer, &identifier->type);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
    } else if (currentIsType(syntaxAnalyzer, TOKEN_LEFT_PAREN)) {
        constructor = createNode(ABSTRACT_SYNTAX_CALL_EXPRESSION, location);
        if (constructor == NULL) {
            status = COMPILER_INTERNAL_ERROR;
            goto cleanup;
        }
        status = appendOwnedChild(constructor, identifier);
        identifier = NULL;
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        status = advanceToken(syntaxAnalyzer);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        while (!currentIsType(syntaxAnalyzer, TOKEN_RIGHT_PAREN)) {
            AbstractSyntaxNode *childPattern = NULL;
            status = parseMatchPattern(syntaxAnalyzer, &childPattern, true);
            if (status != COMPILER_OK) {
                abstractSyntaxNodeDestroy(childPattern);
                goto cleanup;
            }
            status = appendOwnedChild(constructor, childPattern);
            if (status != COMPILER_OK) {
                goto cleanup;
            }
            if (currentIsType(syntaxAnalyzer, TOKEN_RIGHT_PAREN)) {
                break;
            }
            status = expectType(syntaxAnalyzer, TOKEN_COMMA);
            if (status != COMPILER_OK) {
                goto cleanup;
            }
            if (currentIsType(syntaxAnalyzer, TOKEN_RIGHT_PAREN)) {
                break;
            }
        }
        status = expectType(syntaxAnalyzer, TOKEN_RIGHT_PAREN);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
    } else if (!allowUntypedBinding) {
        status = COMPILER_SYNTAX_ERROR;
        goto cleanup;
    }

    *outPattern = constructor != NULL ? constructor : identifier;
    constructor = NULL;
    identifier = NULL;
    return COMPILER_OK;

cleanup:
    abstractSyntaxNodeDestroy(identifier);
    abstractSyntaxNodeDestroy(constructor);
    return status;
}

static bool isWildcardPattern(const AbstractSyntaxNode *pattern) {
    return pattern != NULL && pattern->kind == ABSTRACT_SYNTAX_IDENTIFIER &&
           pattern->text != NULL && strcmp(pattern->text, SYNTAX_PATTERN_WILDCARD) == 0;
}

static CompilerStatus parseMatchCase(SyntaxAnalyzer *syntaxAnalyzer,
                                     AbstractSyntaxNode **outMatchCase) {
    *outMatchCase = NULL;
    CompilerStatus status = expectWord(syntaxAnalyzer, SYNTAX_KW_CASE);
    if (status != COMPILER_OK) {
        return status;
    }

    AbstractSyntaxNode *matchCase =
        createNode(ABSTRACT_SYNTAX_MATCH_CASE, syntaxAnalyzer->currentToken->location);
    AbstractSyntaxNode *pattern = NULL;
    AbstractSyntaxNode *guard = NULL;
    AbstractSyntaxNode *result = NULL;
    if (matchCase == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    status = parseMatchPattern(syntaxAnalyzer, &pattern, false);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    bool isWildcard = isWildcardPattern(pattern);
    status = appendOwnedChild(matchCase, pattern);
    pattern = NULL;
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_IF)) {
        if (isWildcard) {
            status = COMPILER_SYNTAX_ERROR;
            goto cleanup;
        }
        SourceLocation guardLocation = syntaxAnalyzer->currentToken->location;
        guard = createNode(ABSTRACT_SYNTAX_MATCH_GUARD, guardLocation);
        if (guard == NULL) {
            status = COMPILER_INTERNAL_ERROR;
            goto cleanup;
        }
        status = advanceToken(syntaxAnalyzer);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        AbstractSyntaxNode *guardExpression = NULL;
        status = analyzerExpressionsParse(syntaxAnalyzer, &guardExpression);
        if (status != COMPILER_OK) {
            abstractSyntaxNodeDestroy(guardExpression);
            goto cleanup;
        }
        syntaxAnalyzer->newlineConsumed = false;
        status = appendOwnedChild(guard, guardExpression);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        status = appendOwnedChild(matchCase, guard);
        guard = NULL;
        if (status != COMPILER_OK) {
            goto cleanup;
        }
    }

    status = expectWord(syntaxAnalyzer, SYNTAX_OP_MATCH_ARROW);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = analyzerExpressionsParse(syntaxAnalyzer, &result);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = appendOwnedChild(matchCase, result);
    result = NULL;
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    *outMatchCase = matchCase;
    return COMPILER_OK;

cleanup:
    abstractSyntaxNodeDestroy(pattern);
    abstractSyntaxNodeDestroy(guard);
    abstractSyntaxNodeDestroy(result);
    abstractSyntaxNodeDestroy(matchCase);
    return status;
}

static CompilerStatus parseMatchExpression(SyntaxAnalyzer *syntaxAnalyzer,
                                           AbstractSyntaxNode *scrutinee,
                                           AbstractSyntaxNode **outNode) {
    CompilerStatus status = COMPILER_OK;
    AbstractSyntaxNode *matchExpression =
        createNode(ABSTRACT_SYNTAX_MATCH_EXPRESSION, scrutinee->location);
    AbstractSyntaxNode *matchCase = NULL;
    bool hasDefaultCase = false;
    bool previousConsumedNewline = syntaxAnalyzer->newlineConsumed;
    syntaxAnalyzer->newlineConsumed = false;
    if (matchExpression == NULL) {
        abstractSyntaxNodeDestroy(scrutinee);
        syntaxAnalyzer->newlineConsumed = previousConsumedNewline;
        return COMPILER_INTERNAL_ERROR;
    }

    status = appendOwnedChild(matchExpression, scrutinee);
    scrutinee = NULL;
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = expectWord(syntaxAnalyzer, SYNTAX_KW_MATCH);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = expectType(syntaxAnalyzer, TOKEN_NEWLINE);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = skipNewlines(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = expectType(syntaxAnalyzer, TOKEN_INDENT);
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    while (!currentIsType(syntaxAnalyzer, TOKEN_DEDENT) &&
           !currentIsType(syntaxAnalyzer, TOKEN_EOF)) {
        status = parseMatchCase(syntaxAnalyzer, &matchCase);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        bool resultConsumedNewline = syntaxAnalyzer->newlineConsumed;
        syntaxAnalyzer->newlineConsumed = false;
        hasDefaultCase = isWildcardPattern(matchCase->children[0]);
        status = appendOwnedChild(matchExpression, matchCase);
        matchCase = NULL;
        if (status != COMPILER_OK) {
            goto cleanup;
        }

        if (currentIsType(syntaxAnalyzer, TOKEN_NEWLINE)) {
            status = skipNewlines(syntaxAnalyzer);
            if (status != COMPILER_OK) {
                goto cleanup;
            }
        } else if (!currentIsType(syntaxAnalyzer, TOKEN_DEDENT) && !resultConsumedNewline) {
            status = COMPILER_SYNTAX_ERROR;
            goto cleanup;
        }

        if (hasDefaultCase && !currentIsType(syntaxAnalyzer, TOKEN_DEDENT)) {
            status = COMPILER_SYNTAX_ERROR;
            goto cleanup;
        }
    }

    if (!hasDefaultCase) {
        status = COMPILER_SYNTAX_ERROR;
        goto cleanup;
    }
    status = expectType(syntaxAnalyzer, TOKEN_DEDENT);
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    syntaxAnalyzer->newlineConsumed = previousConsumedNewline;
    *outNode = matchExpression;
    return COMPILER_OK;

cleanup:
    abstractSyntaxNodeDestroy(scrutinee);
    abstractSyntaxNodeDestroy(matchCase);
    abstractSyntaxNodeDestroy(matchExpression);
    syntaxAnalyzer->newlineConsumed = previousConsumedNewline;
    return status;
}

typedef enum {
    EXPRESSION_STACK_MARKER,
    EXPRESSION_STACK_TERMINAL,
    EXPRESSION_STACK_NONTERMINAL
} ExpressionStackEntryKind;

typedef struct {
    ExpressionStackEntryKind kind;
    ExpressionTerminal terminal;
    SourceLocation location;
    AbstractSyntaxNode *node;
} ExpressionStackEntry;

static CompilerStatus reserveExpressionStack(ExpressionStackEntry **stack, size_t *capacity,
                                             size_t required) {
    if (*capacity >= required) {
        return COMPILER_OK;
    }
    size_t newCapacity = *capacity == 0 ? 8 : *capacity;
    while (newCapacity < required) {
        newCapacity *= 2;
    }
    ExpressionStackEntry *newStack =
        memoryResizeArray(*stack, newCapacity, sizeof(ExpressionStackEntry));
    if (newStack == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    *stack = newStack;
    *capacity = newCapacity;
    return COMPILER_OK;
}

static CompilerStatus pushExpressionStack(ExpressionStackEntry **stack, size_t *count,
                                          size_t *capacity, ExpressionStackEntry entry) {
    CompilerStatus status = reserveExpressionStack(stack, capacity, *count + 1);
    if (status != COMPILER_OK) {
        return status;
    }
    (*stack)[(*count)++] = entry;
    return COMPILER_OK;
}

static size_t topTerminalIndex(const ExpressionStackEntry *stack, size_t count) {
    while (count > 0) {
        size_t index = --count;
        if (stack[index].kind == EXPRESSION_STACK_TERMINAL) {
            return index;
        }
    }
    return SIZE_MAX;
}

static bool hasOpenParenthesis(const ExpressionStackEntry *stack, size_t count) {
    for (size_t index = 0; index < count; index++) {
        if (stack[index].kind == EXPRESSION_STACK_TERMINAL &&
            stack[index].terminal == EXPR_TERMINAL_LEFT_PAREN) {
            return true;
        }
    }
    return false;
}

static bool expressionExpectsOperand(const ExpressionStackEntry *stack, size_t count) {
    /* A sign is unary at expression start or after a token that still needs an operand. */
    while (count > 0) {
        const ExpressionStackEntry *entry = &stack[--count];
        if (entry->kind == EXPRESSION_STACK_MARKER) {
            continue;
        }
        if (entry->kind != EXPRESSION_STACK_TERMINAL) {
            return false;
        }

        ExpressionTerminal terminal = entry->terminal;
        return terminal == EXPR_TERMINAL_END || terminal == EXPR_TERMINAL_LEFT_PAREN ||
               terminal == EXPR_TERMINAL_UNARY_PLUS || terminal == EXPR_TERMINAL_UNARY_MINUS ||
               terminal == EXPR_TERMINAL_LOGICAL_NOT ||
               (terminal >= EXPR_TERMINAL_MULTIPLY && terminal <= EXPR_TERMINAL_SUBTRACT) ||
               (terminal >= EXPR_TERMINAL_LESS && terminal <= EXPR_TERMINAL_NOT_EQUAL) ||
               terminal == EXPR_TERMINAL_LOGICAL_AND || terminal == EXPR_TERMINAL_LOGICAL_OR ||
               terminal == EXPR_TERMINAL_RANGE_TO || terminal == EXPR_TERMINAL_RANGE_UNTIL ||
               terminal == EXPR_TERMINAL_RANGE_BY;
    }
    return true;
}

static bool isBinaryExpressionOperator(ExpressionTerminal terminal) {
    return (terminal >= EXPR_TERMINAL_MULTIPLY && terminal <= EXPR_TERMINAL_SUBTRACT) ||
           (terminal >= EXPR_TERMINAL_LESS && terminal <= EXPR_TERMINAL_NOT_EQUAL) ||
           terminal == EXPR_TERMINAL_LOGICAL_AND || terminal == EXPR_TERMINAL_LOGICAL_OR ||
           terminal == EXPR_TERMINAL_RANGE_TO || terminal == EXPR_TERMINAL_RANGE_UNTIL;
}

static ExpressionTerminal terminalForToken(const SyntaxAnalyzer *syntaxAnalyzer, const Token *token,
                                           const ExpressionStackEntry *stack, size_t stackCount) {
    if (token == NULL) {
        return EXPR_TERMINAL_END;
    }
    if (syntaxAnalyzer->newlineConsumed && token->type != TOKEN_EOF &&
        token->type != TOKEN_NEWLINE && token->type != TOKEN_DEDENT && token->type != TOKEN_COMMA &&
        token->type != TOKEN_RIGHT_PAREN) {
        return EXPR_TERMINAL_END;
    }
    if (token->type == TOKEN_KEYWORD && tokenHasText(token, SYNTAX_KW_IF)) {
        return EXPR_TERMINAL_OPERAND;
    }
    if (token->type == TOKEN_IDENTIFIER || token->type == TOKEN_INT_LITERAL ||
        token->type == TOKEN_DOUBLE_LITERAL || token->type == TOKEN_STRING_LITERAL ||
        token->type == TOKEN_BOOL_LITERAL || token->type == TOKEN_UNIT_LITERAL ||
        (token->type == TOKEN_KEYWORD && tokenHasText(token, SYNTAX_KW_LIST))) {
        return EXPR_TERMINAL_OPERAND;
    }
    if (token->type == TOKEN_LEFT_PAREN) {
        return EXPR_TERMINAL_OPERAND;
    }
    if (token->type == TOKEN_RIGHT_PAREN && hasOpenParenthesis(stack, stackCount)) {
        return EXPR_TERMINAL_RIGHT_PAREN;
    }
    if (token->type == TOKEN_OPERATOR) {
        ExpressionTerminal terminal;
        if (syntaxOperatorFromToken(token, &terminal)) {
            if (expressionExpectsOperand(stack, stackCount)) {
                if (terminal == EXPR_TERMINAL_ADD) {
                    return EXPR_TERMINAL_UNARY_PLUS;
                }
                if (terminal == EXPR_TERMINAL_SUBTRACT) {
                    return EXPR_TERMINAL_UNARY_MINUS;
                }
            }
            return terminal;
        }
    }
    if (token->type == TOKEN_KEYWORD && tokenHasText(token, SYNTAX_KW_TO)) {
        return EXPR_TERMINAL_RANGE_TO;
    }
    if (token->type == TOKEN_KEYWORD && tokenHasText(token, SYNTAX_KW_UNTIL)) {
        return EXPR_TERMINAL_RANGE_UNTIL;
    }
    if (token->type == TOKEN_KEYWORD && tokenHasText(token, SYNTAX_KW_BY)) {
        return EXPR_TERMINAL_RANGE_BY;
    }
    return EXPR_TERMINAL_END;
}

static void destroyExpressionStack(ExpressionStackEntry *stack, size_t count) {
    for (size_t index = 0; index < count; index++) {
        abstractSyntaxNodeDestroy(stack[index].node);
    }
    free(stack);
}

static CompilerStatus insertHandleMarker(ExpressionStackEntry **stack, size_t *count,
                                         size_t *capacity, size_t terminalIndex) {
    CompilerStatus status = reserveExpressionStack(stack, capacity, *count + 1);
    if (status != COMPILER_OK) {
        return status;
    }
    size_t insertionIndex = terminalIndex + 1;
    for (size_t index = *count; index > insertionIndex; index--) {
        (*stack)[index] = (*stack)[index - 1];
    }
    (*stack)[insertionIndex] = (ExpressionStackEntry){.kind = EXPRESSION_STACK_MARKER};
    (*count)++;
    return COMPILER_OK;
}

static CompilerStatus reduceExpressionHandle(ExpressionStackEntry *stack, size_t *stackCount) {
    size_t markerIndex = *stackCount;
    while (markerIndex > 0) {
        markerIndex--;
        if (stack[markerIndex].kind == EXPRESSION_STACK_MARKER) {
            break;
        }
    }
    if (markerIndex >= *stackCount || stack[markerIndex].kind != EXPRESSION_STACK_MARKER) {
        return COMPILER_SYNTAX_ERROR;
    }

    size_t handleStart = markerIndex + 1;
    size_t handleLength = *stackCount - handleStart;
    if (handleLength == 1 && stack[handleStart].kind == EXPRESSION_STACK_TERMINAL &&
        stack[handleStart].terminal == EXPR_TERMINAL_OPERAND && stack[handleStart].node != NULL) {
        stack[handleStart].kind = EXPRESSION_STACK_NONTERMINAL;
        stack[markerIndex] = stack[handleStart];
        *stackCount = markerIndex + 1;
        return COMPILER_OK;
    }

    if (handleLength == 2 && stack[handleStart].kind == EXPRESSION_STACK_TERMINAL &&
        (stack[handleStart].terminal == EXPR_TERMINAL_UNARY_PLUS ||
         stack[handleStart].terminal == EXPR_TERMINAL_UNARY_MINUS ||
         stack[handleStart].terminal == EXPR_TERMINAL_LOGICAL_NOT) &&
        stack[handleStart + 1].kind == EXPRESSION_STACK_NONTERMINAL &&
        stack[handleStart + 1].node != NULL) {
        /* Keep prefix signs as unary AST nodes so semantic analysis sees the operator. */
        AbstractSyntaxNode *operand = stack[handleStart + 1].node;
        AbstractSyntaxNode *node =
            createNode(ABSTRACT_SYNTAX_UNARY_EXPRESSION, stack[handleStart].location);
        if (node == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }
        CompilerStatus status = setNodeText(node, syntaxOperatorText(stack[handleStart].terminal));
        if (status != COMPILER_OK) {
            abstractSyntaxNodeDestroy(node);
            return status;
        }

        stack[handleStart + 1].node = NULL;
        status = appendOwnedChild(node, operand);
        if (status != COMPILER_OK) {
            abstractSyntaxNodeDestroy(node);
            return status;
        }
        stack[markerIndex] =
            (ExpressionStackEntry){.kind = EXPRESSION_STACK_NONTERMINAL, .node = node};
        *stackCount = markerIndex + 1;
        return COMPILER_OK;
    }

    if (handleLength == 3 && stack[handleStart].kind == EXPRESSION_STACK_NONTERMINAL &&
        stack[handleStart + 1].kind == EXPRESSION_STACK_TERMINAL &&
        stack[handleStart + 1].terminal == EXPR_TERMINAL_RANGE_BY &&
        stack[handleStart + 2].kind == EXPRESSION_STACK_NONTERMINAL) {
        AbstractSyntaxNode *range = stack[handleStart].node;
        AbstractSyntaxNode *step = stack[handleStart + 2].node;
        if (range == NULL || range->kind != ABSTRACT_SYNTAX_RANGE_EXPRESSION ||
            range->childCount != 2 || step == NULL) {
            return COMPILER_SYNTAX_ERROR;
        }
        CompilerStatus status = appendOwnedChild(range, step);
        stack[handleStart].node = NULL;
        stack[handleStart + 2].node = NULL;
        if (status != COMPILER_OK) {
            abstractSyntaxNodeDestroy(range);
            return status;
        }
        stack[markerIndex] =
            (ExpressionStackEntry){.kind = EXPRESSION_STACK_NONTERMINAL, .node = range};
        *stackCount = markerIndex + 1;
        return COMPILER_OK;
    }

    if (handleLength == 3 && stack[handleStart].kind == EXPRESSION_STACK_NONTERMINAL &&
        stack[handleStart + 1].kind == EXPRESSION_STACK_TERMINAL &&
        isBinaryExpressionOperator(stack[handleStart + 1].terminal) &&
        stack[handleStart + 2].kind == EXPRESSION_STACK_NONTERMINAL) {
        AbstractSyntaxNode *left = stack[handleStart].node;
        AbstractSyntaxNode *right = stack[handleStart + 2].node;
        ExpressionTerminal operator = stack[handleStart + 1].terminal;
        bool isRange = operator == EXPR_TERMINAL_RANGE_TO || operator == EXPR_TERMINAL_RANGE_UNTIL;
        AbstractSyntaxNode *node = createNode(isRange ? ABSTRACT_SYNTAX_RANGE_EXPRESSION
                                                      : ABSTRACT_SYNTAX_EXPRESSION_BINARY,
                                              stack[handleStart + 1].location);
        if (node == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }
        CompilerStatus status = setNodeText(node, syntaxOperatorText(operator));
        if (status != COMPILER_OK) {
            abstractSyntaxNodeDestroy(node);
            return status;
        }

        stack[handleStart].node = NULL;
        status = appendOwnedChild(node, left);
        if (status != COMPILER_OK) {
            stack[handleStart + 2].node = NULL;
            abstractSyntaxNodeDestroy(right);
            abstractSyntaxNodeDestroy(node);
            return status;
        }
        stack[handleStart + 2].node = NULL;
        status = appendOwnedChild(node, right);
        if (status != COMPILER_OK) {
            abstractSyntaxNodeDestroy(node);
            return status;
        }
        stack[markerIndex] =
            (ExpressionStackEntry){.kind = EXPRESSION_STACK_NONTERMINAL, .node = node};
        *stackCount = markerIndex + 1;
        return COMPILER_OK;
    }

    if (handleLength == 3 && stack[handleStart].kind == EXPRESSION_STACK_TERMINAL &&
        stack[handleStart].terminal == EXPR_TERMINAL_LEFT_PAREN &&
        stack[handleStart + 1].kind == EXPRESSION_STACK_NONTERMINAL &&
        stack[handleStart + 2].kind == EXPRESSION_STACK_TERMINAL &&
        stack[handleStart + 2].terminal == EXPR_TERMINAL_RIGHT_PAREN) {
        stack[markerIndex] = stack[handleStart + 1];
        *stackCount = markerIndex + 1;
        return COMPILER_OK;
    }
    return COMPILER_SYNTAX_ERROR;
}

static CompilerStatus shiftExpressionToken(SyntaxAnalyzer *syntaxAnalyzer,
                                           ExpressionStackEntry **stack, size_t *stackCount,
                                           size_t *stackCapacity, ExpressionTerminal terminal) {
    Token *token = syntaxAnalyzer->currentToken;
    ExpressionStackEntry entry = {
        .kind = EXPRESSION_STACK_TERMINAL, .terminal = terminal, .location = token->location};
    if (terminal == EXPR_TERMINAL_OPERAND) {
        if (token->type == TOKEN_KEYWORD && tokenHasText(token, SYNTAX_KW_IF)) {
            CompilerStatus status = parseIfExpression(syntaxAnalyzer, &entry.node);
            if (status != COMPILER_OK) {
                return status;
            }
            status = pushExpressionStack(stack, stackCount, stackCapacity, entry);
            if (status != COMPILER_OK) {
                abstractSyntaxNodeDestroy(entry.node);
            }
            return status;
        }
        CompilerStatus status = parseExpressionOperand(syntaxAnalyzer, &entry.node);
        if (status != COMPILER_OK) {
            return status;
        }
        status = pushExpressionStack(stack, stackCount, stackCapacity, entry);
        if (status != COMPILER_OK) {
            abstractSyntaxNodeDestroy(entry.node);
        }
        return status;
    }

    CompilerStatus status = pushExpressionStack(stack, stackCount, stackCapacity, entry);
    if (status != COMPILER_OK) {
        return status;
    }
    if (terminal == EXPR_TERMINAL_RIGHT_PAREN) {
        syntaxAnalyzer->newlineConsumed = false;
    }
    return advanceToken(syntaxAnalyzer);
}

CompilerStatus analyzerExpressionsParse(SyntaxAnalyzer *syntaxAnalyzer,
                                        AbstractSyntaxNode **outNode) {
    if (syntaxAnalyzer == NULL || outNode == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    *outNode = NULL;

    ExpressionStackEntry *stack = NULL;
    size_t stackCount = 0;
    size_t stackCapacity = 0;
    CompilerStatus status = pushExpressionStack(
        &stack, &stackCount, &stackCapacity,
        (ExpressionStackEntry){.kind = EXPRESSION_STACK_TERMINAL, .terminal = EXPR_TERMINAL_END});
    if (status != COMPILER_OK) {
        destroyExpressionStack(stack, stackCount);
        return status;
    }
    status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    for (;;) {
        Token *token = syntaxAnalyzer->currentToken;
        bool emptyParentheses = false;
        if (token != NULL && token->type == TOKEN_LEFT_PAREN) {
            status = ensureNextToken(syntaxAnalyzer);
            if (status != COMPILER_OK) {
                goto cleanup;
            }
            emptyParentheses = syntaxAnalyzer->nextToken != NULL &&
                               syntaxAnalyzer->nextToken->type == TOKEN_RIGHT_PAREN;
        }

        ExpressionTerminal incoming = terminalForToken(syntaxAnalyzer, token, stack, stackCount);
        if (emptyParentheses) {
            incoming = EXPR_TERMINAL_OPERAND;
        }
        size_t stackedTerminalIndex = topTerminalIndex(stack, stackCount);
        if (stackedTerminalIndex == SIZE_MAX) {
            status = COMPILER_INTERNAL_ERROR;
            goto cleanup;
        }
        ExpressionTerminal stacked = stack[stackedTerminalIndex].terminal;

        if (incoming == EXPR_TERMINAL_END && stackCount == 2 &&
            stack[1].kind == EXPRESSION_STACK_NONTERMINAL) {
            if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_MATCH)) {
                AbstractSyntaxNode *scrutinee = stack[1].node;
                stack[1].node = NULL;
                status = parseMatchExpression(syntaxAnalyzer, scrutinee, outNode);
                destroyExpressionStack(stack, stackCount);
                return status;
            }
            *outNode = stack[1].node;
            stack[1].node = NULL;
            if (token != NULL && (token->type == TOKEN_COMMA || token->type == TOKEN_RIGHT_PAREN)) {
                syntaxAnalyzer->newlineConsumed = false;
            }
            destroyExpressionStack(stack, stackCount);
            return COMPILER_OK;
        }

        PrecedenceRelation relation = syntaxPrecedenceRelation(stacked, incoming);
        if (relation == PRECEDENCE_ERROR) {
            status = COMPILER_SYNTAX_ERROR;
        } else if (relation == PRECEDENCE_REDUCE) {
            status = reduceExpressionHandle(stack, &stackCount);
        } else if (relation == PRECEDENCE_SHIFT) {
            status = insertHandleMarker(&stack, &stackCount, &stackCapacity, stackedTerminalIndex);
            if (status != COMPILER_OK) {
                goto cleanup;
            }
            status =
                shiftExpressionToken(syntaxAnalyzer, &stack, &stackCount, &stackCapacity, incoming);
        } else if (relation == PRECEDENCE_EQUAL) {
            status =
                shiftExpressionToken(syntaxAnalyzer, &stack, &stackCount, &stackCapacity, incoming);
        } else {
            status = COMPILER_INTERNAL_ERROR;
        }
        if (status != COMPILER_OK) {
            goto cleanup;
        }
    }

cleanup:
    destroyExpressionStack(stack, stackCount);
    return status;
}
