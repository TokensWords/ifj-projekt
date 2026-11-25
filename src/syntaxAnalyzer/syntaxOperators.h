/**
 * @file  syntaxOperators.h
 *
 * @project IFJ26 compiler
 * @author Matúš Čík xcikmat00
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

/**
 * @brief Maps a supported operator token to its expression terminal.
 * @param token The token to check. Can be NULL.
 * @param outTerminal Receives the terminal on success.
 * @return True if the terminal was set, or false otherwise.
 * @note The parser uses the expression context to tell unary and binary plus or minus apart.
 */
bool syntaxOperatorFromToken(const Token *token, ExpressionTerminal *outTerminal);

/**
 * @brief Gets the spelling used for an operator in the AST.
 * @param terminal Expression terminal to look up.
 * @return The operator text, or an empty string if the terminal has no spelling. The string
 *     has static storage.
 */
const char *syntaxOperatorText(ExpressionTerminal terminal);

/**
 * @brief Determines how the parser should handle the incoming terminal.
 * @param stacked The top terminal on the expression stack.
 * @param incoming Terminal for the current input token.
 * @return The shift, equal, or reduce relation, or PRECEDENCE_ERROR for an invalid pair.
 */
PrecedenceRelation syntaxPrecedenceRelation(ExpressionTerminal stacked,
                                            ExpressionTerminal incoming);
#endif
