
#ifndef SYNTAX_OPERATORS_H
#define SYNTAX_OPERATORS_H

#include "token.h"

typedef enum {
    PRECEDENCE_ERROR,
    PRECEDENCE_SHIFT,
    PRECEDENCE_EQUAL,
    PRECEDENCE_REDUCE
} PrecedenceRelation;

typedef enum {
    EXPR_TERMINAL_END,
    EXPR_TERMINAL_OPERAND,
    EXPR_TERMINAL_LEFT_PAREN,
    EXPR_TERMINAL_RIGHT_PAREN,
    EXPR_TERMINAL_MULTIPLY,
    EXPR_TERMINAL_DIVIDE,
    EXPR_TERMINAL_ADD,
    EXPR_TERMINAL_SUBTRACT,
    EXPR_TERMINAL_LESS,
    EXPR_TERMINAL_LESS_EQUAL,
    EXPR_TERMINAL_GREATER,
    EXPR_TERMINAL_GREATER_EQUAL,
    EXPR_TERMINAL_EQUAL,
    EXPR_TERMINAL_NOT_EQUAL,
    EXPR_TERMINAL_COUNT,
} ExpressionTerminal;

bool syntaxOperatorFromToken(const Token *token, ExpressionTerminal *outTerminal);

const char *syntaxOperatorText(ExpressionTerminal terminal);

PrecedenceRelation syntaxPrecedenceCompare(ExpressionTerminal stacked,
                                            ExpressionTerminal incoming);

#endif