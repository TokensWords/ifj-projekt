/**
 * @file  syntaxOperators.h
 *
 * @project IFJ26 compiler
 * @author Matúš Čík
 */

#ifndef SYNTAX_OPERATORS_H
#define SYNTAX_OPERATORS_H

#include "token.h"

#include <stdbool.h>

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
    EXPR_TERMINAL_UNARY_PLUS,
    EXPR_TERMINAL_UNARY_MINUS,
    EXPR_TERMINAL_LESS,
    EXPR_TERMINAL_LESS_EQUAL,
    EXPR_TERMINAL_GREATER,
    EXPR_TERMINAL_GREATER_EQUAL,
    EXPR_TERMINAL_EQUAL,
    EXPR_TERMINAL_NOT_EQUAL,
    EXPR_TERMINAL_LOGICAL_NOT,
    EXPR_TERMINAL_LOGICAL_AND,
    EXPR_TERMINAL_LOGICAL_OR,
    EXPR_TERMINAL_RANGE_TO,
    EXPR_TERMINAL_RANGE_UNTIL,
    EXPR_TERMINAL_RANGE_BY,
    EXPR_TERMINAL_COUNT,
} ExpressionTerminal;

/** @brief Maps a supported binary operator token to its expression terminal. */
bool syntaxOperatorFromToken(const Token *token, ExpressionTerminal *outTerminal);

/** @brief Returns the spelling stored in the corresponding AST node. */
const char *syntaxOperatorText(ExpressionTerminal terminal);

/**
 * @brief Looks up the relation between stacked and incoming terminals.
 * @return The table relation: shift (<), equal (=), reduce (>), or error.
 */
PrecedenceRelation syntaxPrecedenceRelation(ExpressionTerminal stacked,
                                            ExpressionTerminal incoming);
#endif
