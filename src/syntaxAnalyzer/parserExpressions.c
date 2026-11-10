/**
 * @file  parserExpressions.c
 *
 * @project IFJ26 compiler
 * @author Matúš Čík
 */

#include "parserExpressions.h"

#include "syntaxAnalyzerUtils.h"

#include "memory.h"
#include "syntaxOperators.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

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
        unit->inferredType = DATA_TYPE_UNIT;

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

CompilerStatus parseFunctionCall(SyntaxAnalyzer *syntaxAnalyzer, AbstractSyntaxNode **outNode) {
    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    if (!isIdentifierToken(syntaxAnalyzer->currentToken)) {
        return COMPILER_SYNTAX_ERROR;
    }

    AbstractSyntaxNode *call =
        createNode(ABSTRACT_SYNTAX_FUNCTION_CALL, syntaxAnalyzer->currentToken->location);
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
                                                   AbstractSyntaxNode **outNode) {
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
               (terminal >= EXPR_TERMINAL_MULTIPLY && terminal <= EXPR_TERMINAL_SUBTRACT) ||
               (terminal >= EXPR_TERMINAL_LESS && terminal <= EXPR_TERMINAL_NOT_EQUAL);
    }
    return true;
}

static bool isBinaryExpressionOperator(ExpressionTerminal terminal) {
    return (terminal >= EXPR_TERMINAL_MULTIPLY && terminal <= EXPR_TERMINAL_SUBTRACT) ||
           (terminal >= EXPR_TERMINAL_LESS && terminal <= EXPR_TERMINAL_NOT_EQUAL);
}

static ExpressionTerminal terminalForToken(const Token *token, const ExpressionStackEntry *stack,
                                           size_t stackCount) {
    if (token == NULL) {
        return EXPR_TERMINAL_END;
    }
    if (token->type == TOKEN_IDENTIFIER || token->type == TOKEN_INT_LITERAL ||
        token->type == TOKEN_DOUBLE_LITERAL || token->type == TOKEN_STRING_LITERAL ||
        token->type == TOKEN_UNIT_LITERAL) {
        return EXPR_TERMINAL_OPERAND;
    }
    if (token->type == TOKEN_LEFT_PAREN) {
        return EXPR_TERMINAL_LEFT_PAREN;
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
         stack[handleStart].terminal == EXPR_TERMINAL_UNARY_MINUS) &&
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
        isBinaryExpressionOperator(stack[handleStart + 1].terminal) &&
        stack[handleStart + 2].kind == EXPRESSION_STACK_NONTERMINAL) {
        AbstractSyntaxNode *left = stack[handleStart].node;
        AbstractSyntaxNode *right = stack[handleStart + 2].node;
        AbstractSyntaxNode *node =
            createNode(ABSTRACT_SYNTAX_EXPRESSION_BINARY, stack[handleStart + 1].location);
        if (node == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }
        CompilerStatus status =
            setNodeText(node, syntaxOperatorText(stack[handleStart + 1].terminal));
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
        if (token->type == TOKEN_LEFT_PAREN) {
            AbstractSyntaxNode *unit = createNode(ABSTRACT_SYNTAX_UNIT_LITERAL, token->location);
            if (unit == NULL) {
                return COMPILER_INTERNAL_ERROR;
            }
            entry.node = unit;
            CompilerStatus status = pushExpressionStack(stack, stackCount, stackCapacity, entry);
            if (status != COMPILER_OK) {
                abstractSyntaxNodeDestroy(unit);
                return status;
            }
            status = advanceToken(syntaxAnalyzer);
            if (status != COMPILER_OK) {
                return status;
            }
            return advanceToken(syntaxAnalyzer);
        }
        if (token->type == TOKEN_IDENTIFIER && syntaxAnalyzer->nextToken != NULL &&
            syntaxAnalyzer->nextToken->type == TOKEN_LEFT_PAREN) {
            return COMPILER_SYNTAX_ERROR;
        }
        CompilerStatus status = parseTerm(syntaxAnalyzer, &entry.node);
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

        ExpressionTerminal incoming = terminalForToken(token, stack, stackCount);
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
            *outNode = stack[1].node;
            stack[1].node = NULL;
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
