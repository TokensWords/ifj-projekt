/**
 * @file  syntaxOperators.c
 *
 * @project IFJ26 compiler
 * @author Matúš Čík
 */

#include "syntaxOperators.h"

#include "syntax.h"
#include <string.h>

typedef struct {
    ExpressionTerminal terminal;
    const char *text;
} OperatorDefinition;

static const OperatorDefinition operators[] = {
    {EXPR_TERMINAL_MULTIPLY, SYNTAX_OP_MULTIPLY},
    {EXPR_TERMINAL_DIVIDE, SYNTAX_OP_DIVIDE},
    {EXPR_TERMINAL_ADD, SYNTAX_OP_PLUS},
    {EXPR_TERMINAL_SUBTRACT, SYNTAX_OP_MINUS},
    {EXPR_TERMINAL_LESS, SYNTAX_OP_LESS},
    {EXPR_TERMINAL_LESS_EQUAL, SYNTAX_OP_LESS_EQUAL},
    {EXPR_TERMINAL_GREATER, SYNTAX_OP_GREATER},
    {EXPR_TERMINAL_GREATER_EQUAL, SYNTAX_OP_GREATER_EQUAL},
    {EXPR_TERMINAL_EQUAL, SYNTAX_OP_EQUAL},
    {EXPR_TERMINAL_NOT_EQUAL, SYNTAX_OP_NOT_EQUAL},
    {EXPR_TERMINAL_LOGICAL_NOT, SYNTAX_OP_LOGICAL_NOT},
    {EXPR_TERMINAL_LOGICAL_AND, SYNTAX_OP_LOGICAL_AND},
    {EXPR_TERMINAL_LOGICAL_OR, SYNTAX_OP_LOGICAL_OR}};

/* Unspecified entries are invalid precedence relations. */
static const PrecedenceRelation precedenceTable[EXPR_TERMINAL_COUNT][EXPR_TERMINAL_COUNT] = {
    [EXPR_TERMINAL_END] = {[EXPR_TERMINAL_OPERAND] = PRECEDENCE_SHIFT,
                           [EXPR_TERMINAL_LEFT_PAREN] = PRECEDENCE_SHIFT,
                           [EXPR_TERMINAL_MULTIPLY] = PRECEDENCE_SHIFT,
                           [EXPR_TERMINAL_DIVIDE] = PRECEDENCE_SHIFT,
                           [EXPR_TERMINAL_ADD] = PRECEDENCE_SHIFT,
                           [EXPR_TERMINAL_SUBTRACT] = PRECEDENCE_SHIFT,
                           [EXPR_TERMINAL_UNARY_PLUS] = PRECEDENCE_SHIFT,
                           [EXPR_TERMINAL_UNARY_MINUS] = PRECEDENCE_SHIFT,
                           [EXPR_TERMINAL_LESS] = PRECEDENCE_SHIFT,
                           [EXPR_TERMINAL_LESS_EQUAL] = PRECEDENCE_SHIFT,
                           [EXPR_TERMINAL_GREATER] = PRECEDENCE_SHIFT,
                           [EXPR_TERMINAL_GREATER_EQUAL] = PRECEDENCE_SHIFT,
                           [EXPR_TERMINAL_EQUAL] = PRECEDENCE_SHIFT,
                           [EXPR_TERMINAL_NOT_EQUAL] = PRECEDENCE_SHIFT},
    [EXPR_TERMINAL_OPERAND] = {[EXPR_TERMINAL_END] = PRECEDENCE_REDUCE,
                               [EXPR_TERMINAL_RIGHT_PAREN] = PRECEDENCE_REDUCE,
                               [EXPR_TERMINAL_MULTIPLY] = PRECEDENCE_REDUCE,
                               [EXPR_TERMINAL_DIVIDE] = PRECEDENCE_REDUCE,
                               [EXPR_TERMINAL_ADD] = PRECEDENCE_REDUCE,
                               [EXPR_TERMINAL_SUBTRACT] = PRECEDENCE_REDUCE,
                               [EXPR_TERMINAL_LESS] = PRECEDENCE_REDUCE,
                               [EXPR_TERMINAL_LESS_EQUAL] = PRECEDENCE_REDUCE,
                               [EXPR_TERMINAL_GREATER] = PRECEDENCE_REDUCE,
                               [EXPR_TERMINAL_GREATER_EQUAL] = PRECEDENCE_REDUCE,
                               [EXPR_TERMINAL_EQUAL] = PRECEDENCE_REDUCE,
                               [EXPR_TERMINAL_NOT_EQUAL] = PRECEDENCE_REDUCE},
    [EXPR_TERMINAL_LEFT_PAREN] = {[EXPR_TERMINAL_OPERAND] = PRECEDENCE_SHIFT,
                                  [EXPR_TERMINAL_LEFT_PAREN] = PRECEDENCE_SHIFT,
                                  [EXPR_TERMINAL_RIGHT_PAREN] = PRECEDENCE_EQUAL,
                                  [EXPR_TERMINAL_MULTIPLY] = PRECEDENCE_SHIFT,
                                  [EXPR_TERMINAL_DIVIDE] = PRECEDENCE_SHIFT,
                                  [EXPR_TERMINAL_ADD] = PRECEDENCE_SHIFT,
                                  [EXPR_TERMINAL_SUBTRACT] = PRECEDENCE_SHIFT,
                                  [EXPR_TERMINAL_UNARY_PLUS] = PRECEDENCE_SHIFT,
                                  [EXPR_TERMINAL_UNARY_MINUS] = PRECEDENCE_SHIFT,
                                  [EXPR_TERMINAL_LESS] = PRECEDENCE_SHIFT,
                                  [EXPR_TERMINAL_LESS_EQUAL] = PRECEDENCE_SHIFT,
                                  [EXPR_TERMINAL_GREATER] = PRECEDENCE_SHIFT,
                                  [EXPR_TERMINAL_GREATER_EQUAL] = PRECEDENCE_SHIFT,
                                  [EXPR_TERMINAL_EQUAL] = PRECEDENCE_SHIFT,
                                  [EXPR_TERMINAL_NOT_EQUAL] = PRECEDENCE_SHIFT},
    [EXPR_TERMINAL_RIGHT_PAREN] = {[EXPR_TERMINAL_END] = PRECEDENCE_REDUCE,
                                   [EXPR_TERMINAL_RIGHT_PAREN] = PRECEDENCE_REDUCE,
                                   [EXPR_TERMINAL_MULTIPLY] = PRECEDENCE_REDUCE,
                                   [EXPR_TERMINAL_DIVIDE] = PRECEDENCE_REDUCE,
                                   [EXPR_TERMINAL_ADD] = PRECEDENCE_REDUCE,
                                   [EXPR_TERMINAL_SUBTRACT] = PRECEDENCE_REDUCE,
                                   [EXPR_TERMINAL_LESS] = PRECEDENCE_REDUCE,
                                   [EXPR_TERMINAL_LESS_EQUAL] = PRECEDENCE_REDUCE,
                                   [EXPR_TERMINAL_GREATER] = PRECEDENCE_REDUCE,
                                   [EXPR_TERMINAL_GREATER_EQUAL] = PRECEDENCE_REDUCE,
                                   [EXPR_TERMINAL_EQUAL] = PRECEDENCE_REDUCE,
                                   [EXPR_TERMINAL_NOT_EQUAL] = PRECEDENCE_REDUCE},
    [EXPR_TERMINAL_MULTIPLY] = {[EXPR_TERMINAL_OPERAND] = PRECEDENCE_SHIFT,
                                [EXPR_TERMINAL_LEFT_PAREN] = PRECEDENCE_SHIFT,
                                [EXPR_TERMINAL_UNARY_PLUS] = PRECEDENCE_SHIFT,
                                [EXPR_TERMINAL_UNARY_MINUS] = PRECEDENCE_SHIFT,
                                [EXPR_TERMINAL_END] = PRECEDENCE_REDUCE,
                                [EXPR_TERMINAL_RIGHT_PAREN] = PRECEDENCE_REDUCE,
                                [EXPR_TERMINAL_MULTIPLY] = PRECEDENCE_REDUCE,
                                [EXPR_TERMINAL_DIVIDE] = PRECEDENCE_REDUCE,
                                [EXPR_TERMINAL_ADD] = PRECEDENCE_REDUCE,
                                [EXPR_TERMINAL_SUBTRACT] = PRECEDENCE_REDUCE,
                                [EXPR_TERMINAL_LESS] = PRECEDENCE_REDUCE,
                                [EXPR_TERMINAL_LESS_EQUAL] = PRECEDENCE_REDUCE,
                                [EXPR_TERMINAL_GREATER] = PRECEDENCE_REDUCE,
                                [EXPR_TERMINAL_GREATER_EQUAL] = PRECEDENCE_REDUCE,
                                [EXPR_TERMINAL_EQUAL] = PRECEDENCE_REDUCE,
                                [EXPR_TERMINAL_NOT_EQUAL] = PRECEDENCE_REDUCE},
    [EXPR_TERMINAL_DIVIDE] = {[EXPR_TERMINAL_OPERAND] = PRECEDENCE_SHIFT,
                              [EXPR_TERMINAL_LEFT_PAREN] = PRECEDENCE_SHIFT,
                              [EXPR_TERMINAL_UNARY_PLUS] = PRECEDENCE_SHIFT,
                              [EXPR_TERMINAL_UNARY_MINUS] = PRECEDENCE_SHIFT,
                              [EXPR_TERMINAL_END] = PRECEDENCE_REDUCE,
                              [EXPR_TERMINAL_RIGHT_PAREN] = PRECEDENCE_REDUCE,
                              [EXPR_TERMINAL_MULTIPLY] = PRECEDENCE_REDUCE,
                              [EXPR_TERMINAL_DIVIDE] = PRECEDENCE_REDUCE,
                              [EXPR_TERMINAL_ADD] = PRECEDENCE_REDUCE,
                              [EXPR_TERMINAL_SUBTRACT] = PRECEDENCE_REDUCE,
                              [EXPR_TERMINAL_LESS] = PRECEDENCE_REDUCE,
                              [EXPR_TERMINAL_LESS_EQUAL] = PRECEDENCE_REDUCE,
                              [EXPR_TERMINAL_GREATER] = PRECEDENCE_REDUCE,
                              [EXPR_TERMINAL_GREATER_EQUAL] = PRECEDENCE_REDUCE,
                              [EXPR_TERMINAL_EQUAL] = PRECEDENCE_REDUCE,
                              [EXPR_TERMINAL_NOT_EQUAL] = PRECEDENCE_REDUCE},
    [EXPR_TERMINAL_ADD] = {[EXPR_TERMINAL_OPERAND] = PRECEDENCE_SHIFT,
                           [EXPR_TERMINAL_LEFT_PAREN] = PRECEDENCE_SHIFT,
                           [EXPR_TERMINAL_UNARY_PLUS] = PRECEDENCE_SHIFT,
                           [EXPR_TERMINAL_UNARY_MINUS] = PRECEDENCE_SHIFT,
                           [EXPR_TERMINAL_MULTIPLY] = PRECEDENCE_SHIFT,
                           [EXPR_TERMINAL_DIVIDE] = PRECEDENCE_SHIFT,
                           [EXPR_TERMINAL_END] = PRECEDENCE_REDUCE,
                           [EXPR_TERMINAL_RIGHT_PAREN] = PRECEDENCE_REDUCE,
                           [EXPR_TERMINAL_ADD] = PRECEDENCE_REDUCE,
                           [EXPR_TERMINAL_SUBTRACT] = PRECEDENCE_REDUCE,
                           [EXPR_TERMINAL_LESS] = PRECEDENCE_REDUCE,
                           [EXPR_TERMINAL_LESS_EQUAL] = PRECEDENCE_REDUCE,
                           [EXPR_TERMINAL_GREATER] = PRECEDENCE_REDUCE,
                           [EXPR_TERMINAL_GREATER_EQUAL] = PRECEDENCE_REDUCE,
                           [EXPR_TERMINAL_EQUAL] = PRECEDENCE_REDUCE,
                           [EXPR_TERMINAL_NOT_EQUAL] = PRECEDENCE_REDUCE},
    [EXPR_TERMINAL_SUBTRACT] = {[EXPR_TERMINAL_OPERAND] = PRECEDENCE_SHIFT,
                                [EXPR_TERMINAL_LEFT_PAREN] = PRECEDENCE_SHIFT,
                                [EXPR_TERMINAL_UNARY_PLUS] = PRECEDENCE_SHIFT,
                                [EXPR_TERMINAL_UNARY_MINUS] = PRECEDENCE_SHIFT,
                                [EXPR_TERMINAL_MULTIPLY] = PRECEDENCE_SHIFT,
                                [EXPR_TERMINAL_DIVIDE] = PRECEDENCE_SHIFT,
                                [EXPR_TERMINAL_END] = PRECEDENCE_REDUCE,
                                [EXPR_TERMINAL_RIGHT_PAREN] = PRECEDENCE_REDUCE,
                                [EXPR_TERMINAL_ADD] = PRECEDENCE_REDUCE,
                                [EXPR_TERMINAL_SUBTRACT] = PRECEDENCE_REDUCE,
                                [EXPR_TERMINAL_LESS] = PRECEDENCE_REDUCE,
                                [EXPR_TERMINAL_LESS_EQUAL] = PRECEDENCE_REDUCE,
                                [EXPR_TERMINAL_GREATER] = PRECEDENCE_REDUCE,
                                [EXPR_TERMINAL_GREATER_EQUAL] = PRECEDENCE_REDUCE,
                                [EXPR_TERMINAL_EQUAL] = PRECEDENCE_REDUCE,
                                [EXPR_TERMINAL_NOT_EQUAL] = PRECEDENCE_REDUCE},
    [EXPR_TERMINAL_UNARY_PLUS] = {[EXPR_TERMINAL_OPERAND] = PRECEDENCE_SHIFT,
                                  [EXPR_TERMINAL_LEFT_PAREN] = PRECEDENCE_SHIFT,
                                  [EXPR_TERMINAL_UNARY_PLUS] = PRECEDENCE_SHIFT,
                                  [EXPR_TERMINAL_UNARY_MINUS] = PRECEDENCE_SHIFT,
                                  [EXPR_TERMINAL_END] = PRECEDENCE_REDUCE,
                                  [EXPR_TERMINAL_RIGHT_PAREN] = PRECEDENCE_REDUCE,
                                  [EXPR_TERMINAL_MULTIPLY] = PRECEDENCE_REDUCE,
                                  [EXPR_TERMINAL_DIVIDE] = PRECEDENCE_REDUCE,
                                  [EXPR_TERMINAL_ADD] = PRECEDENCE_REDUCE,
                                  [EXPR_TERMINAL_SUBTRACT] = PRECEDENCE_REDUCE,
                                  [EXPR_TERMINAL_LESS] = PRECEDENCE_REDUCE,
                                  [EXPR_TERMINAL_LESS_EQUAL] = PRECEDENCE_REDUCE,
                                  [EXPR_TERMINAL_GREATER] = PRECEDENCE_REDUCE,
                                  [EXPR_TERMINAL_GREATER_EQUAL] = PRECEDENCE_REDUCE,
                                  [EXPR_TERMINAL_EQUAL] = PRECEDENCE_REDUCE,
                                  [EXPR_TERMINAL_NOT_EQUAL] = PRECEDENCE_REDUCE},
    [EXPR_TERMINAL_UNARY_MINUS] = {[EXPR_TERMINAL_OPERAND] = PRECEDENCE_SHIFT,
                                   [EXPR_TERMINAL_LEFT_PAREN] = PRECEDENCE_SHIFT,
                                   [EXPR_TERMINAL_UNARY_PLUS] = PRECEDENCE_SHIFT,
                                   [EXPR_TERMINAL_UNARY_MINUS] = PRECEDENCE_SHIFT,
                                   [EXPR_TERMINAL_END] = PRECEDENCE_REDUCE,
                                   [EXPR_TERMINAL_RIGHT_PAREN] = PRECEDENCE_REDUCE,
                                   [EXPR_TERMINAL_MULTIPLY] = PRECEDENCE_REDUCE,
                                   [EXPR_TERMINAL_DIVIDE] = PRECEDENCE_REDUCE,
                                   [EXPR_TERMINAL_ADD] = PRECEDENCE_REDUCE,
                                   [EXPR_TERMINAL_SUBTRACT] = PRECEDENCE_REDUCE,
                                   [EXPR_TERMINAL_LESS] = PRECEDENCE_REDUCE,
                                   [EXPR_TERMINAL_LESS_EQUAL] = PRECEDENCE_REDUCE,
                                   [EXPR_TERMINAL_GREATER] = PRECEDENCE_REDUCE,
                                   [EXPR_TERMINAL_GREATER_EQUAL] = PRECEDENCE_REDUCE,
                                   [EXPR_TERMINAL_EQUAL] = PRECEDENCE_REDUCE,
                                   [EXPR_TERMINAL_NOT_EQUAL] = PRECEDENCE_REDUCE},
    [EXPR_TERMINAL_LESS] = {[EXPR_TERMINAL_OPERAND] = PRECEDENCE_SHIFT,
                            [EXPR_TERMINAL_LEFT_PAREN] = PRECEDENCE_SHIFT,
                            [EXPR_TERMINAL_UNARY_PLUS] = PRECEDENCE_SHIFT,
                            [EXPR_TERMINAL_UNARY_MINUS] = PRECEDENCE_SHIFT,
                            [EXPR_TERMINAL_MULTIPLY] = PRECEDENCE_SHIFT,
                            [EXPR_TERMINAL_DIVIDE] = PRECEDENCE_SHIFT,
                            [EXPR_TERMINAL_ADD] = PRECEDENCE_SHIFT,
                            [EXPR_TERMINAL_SUBTRACT] = PRECEDENCE_SHIFT,
                            [EXPR_TERMINAL_END] = PRECEDENCE_REDUCE,
                            [EXPR_TERMINAL_RIGHT_PAREN] = PRECEDENCE_REDUCE,
                            [EXPR_TERMINAL_LESS] = PRECEDENCE_ERROR,
                            [EXPR_TERMINAL_LESS_EQUAL] = PRECEDENCE_ERROR,
                            [EXPR_TERMINAL_GREATER] = PRECEDENCE_ERROR,
                            [EXPR_TERMINAL_GREATER_EQUAL] = PRECEDENCE_ERROR,
                            [EXPR_TERMINAL_EQUAL] = PRECEDENCE_REDUCE,
                            [EXPR_TERMINAL_NOT_EQUAL] = PRECEDENCE_REDUCE},
    [EXPR_TERMINAL_LESS_EQUAL] = {[EXPR_TERMINAL_OPERAND] = PRECEDENCE_SHIFT,
                                  [EXPR_TERMINAL_LEFT_PAREN] = PRECEDENCE_SHIFT,
                                  [EXPR_TERMINAL_UNARY_PLUS] = PRECEDENCE_SHIFT,
                                  [EXPR_TERMINAL_UNARY_MINUS] = PRECEDENCE_SHIFT,
                                  [EXPR_TERMINAL_MULTIPLY] = PRECEDENCE_SHIFT,
                                  [EXPR_TERMINAL_DIVIDE] = PRECEDENCE_SHIFT,
                                  [EXPR_TERMINAL_ADD] = PRECEDENCE_SHIFT,
                                  [EXPR_TERMINAL_SUBTRACT] = PRECEDENCE_SHIFT,
                                  [EXPR_TERMINAL_END] = PRECEDENCE_REDUCE,
                                  [EXPR_TERMINAL_RIGHT_PAREN] = PRECEDENCE_REDUCE,
                                  [EXPR_TERMINAL_LESS] = PRECEDENCE_ERROR,
                                  [EXPR_TERMINAL_LESS_EQUAL] = PRECEDENCE_ERROR,
                                  [EXPR_TERMINAL_GREATER] = PRECEDENCE_ERROR,
                                  [EXPR_TERMINAL_GREATER_EQUAL] = PRECEDENCE_ERROR,
                                  [EXPR_TERMINAL_EQUAL] = PRECEDENCE_REDUCE,
                                  [EXPR_TERMINAL_NOT_EQUAL] = PRECEDENCE_REDUCE},
    [EXPR_TERMINAL_GREATER] = {[EXPR_TERMINAL_OPERAND] = PRECEDENCE_SHIFT,
                               [EXPR_TERMINAL_LEFT_PAREN] = PRECEDENCE_SHIFT,
                               [EXPR_TERMINAL_UNARY_PLUS] = PRECEDENCE_SHIFT,
                               [EXPR_TERMINAL_UNARY_MINUS] = PRECEDENCE_SHIFT,
                               [EXPR_TERMINAL_MULTIPLY] = PRECEDENCE_SHIFT,
                               [EXPR_TERMINAL_DIVIDE] = PRECEDENCE_SHIFT,
                               [EXPR_TERMINAL_ADD] = PRECEDENCE_SHIFT,
                               [EXPR_TERMINAL_SUBTRACT] = PRECEDENCE_SHIFT,
                               [EXPR_TERMINAL_END] = PRECEDENCE_REDUCE,
                               [EXPR_TERMINAL_RIGHT_PAREN] = PRECEDENCE_REDUCE,
                               [EXPR_TERMINAL_LESS] = PRECEDENCE_ERROR,
                               [EXPR_TERMINAL_LESS_EQUAL] = PRECEDENCE_ERROR,
                               [EXPR_TERMINAL_GREATER] = PRECEDENCE_ERROR,
                               [EXPR_TERMINAL_GREATER_EQUAL] = PRECEDENCE_ERROR,
                               [EXPR_TERMINAL_EQUAL] = PRECEDENCE_REDUCE,
                               [EXPR_TERMINAL_NOT_EQUAL] = PRECEDENCE_REDUCE},
    [EXPR_TERMINAL_GREATER_EQUAL] = {[EXPR_TERMINAL_OPERAND] = PRECEDENCE_SHIFT,
                                     [EXPR_TERMINAL_LEFT_PAREN] = PRECEDENCE_SHIFT,
                                     [EXPR_TERMINAL_UNARY_PLUS] = PRECEDENCE_SHIFT,
                                     [EXPR_TERMINAL_UNARY_MINUS] = PRECEDENCE_SHIFT,
                                     [EXPR_TERMINAL_MULTIPLY] = PRECEDENCE_SHIFT,
                                     [EXPR_TERMINAL_DIVIDE] = PRECEDENCE_SHIFT,
                                     [EXPR_TERMINAL_ADD] = PRECEDENCE_SHIFT,
                                     [EXPR_TERMINAL_SUBTRACT] = PRECEDENCE_SHIFT,
                                     [EXPR_TERMINAL_END] = PRECEDENCE_REDUCE,
                                     [EXPR_TERMINAL_RIGHT_PAREN] = PRECEDENCE_REDUCE,
                                     [EXPR_TERMINAL_LESS] = PRECEDENCE_ERROR,
                                     [EXPR_TERMINAL_LESS_EQUAL] = PRECEDENCE_ERROR,
                                     [EXPR_TERMINAL_GREATER] = PRECEDENCE_ERROR,
                                     [EXPR_TERMINAL_GREATER_EQUAL] = PRECEDENCE_ERROR,
                                     [EXPR_TERMINAL_EQUAL] = PRECEDENCE_REDUCE,
                                     [EXPR_TERMINAL_NOT_EQUAL] = PRECEDENCE_REDUCE},
    [EXPR_TERMINAL_EQUAL] = {[EXPR_TERMINAL_OPERAND] = PRECEDENCE_SHIFT,
                             [EXPR_TERMINAL_LEFT_PAREN] = PRECEDENCE_SHIFT,
                             [EXPR_TERMINAL_UNARY_PLUS] = PRECEDENCE_SHIFT,
                             [EXPR_TERMINAL_UNARY_MINUS] = PRECEDENCE_SHIFT,
                             [EXPR_TERMINAL_MULTIPLY] = PRECEDENCE_SHIFT,
                             [EXPR_TERMINAL_DIVIDE] = PRECEDENCE_SHIFT,
                             [EXPR_TERMINAL_ADD] = PRECEDENCE_SHIFT,
                             [EXPR_TERMINAL_SUBTRACT] = PRECEDENCE_SHIFT,
                             [EXPR_TERMINAL_LESS] = PRECEDENCE_SHIFT,
                             [EXPR_TERMINAL_LESS_EQUAL] = PRECEDENCE_SHIFT,
                             [EXPR_TERMINAL_GREATER] = PRECEDENCE_SHIFT,
                             [EXPR_TERMINAL_GREATER_EQUAL] = PRECEDENCE_SHIFT,
                             [EXPR_TERMINAL_END] = PRECEDENCE_REDUCE,
                             [EXPR_TERMINAL_RIGHT_PAREN] = PRECEDENCE_REDUCE,
                             [EXPR_TERMINAL_EQUAL] = PRECEDENCE_ERROR,
                             [EXPR_TERMINAL_NOT_EQUAL] = PRECEDENCE_ERROR},
    [EXPR_TERMINAL_NOT_EQUAL] = {[EXPR_TERMINAL_OPERAND] = PRECEDENCE_SHIFT,
                                 [EXPR_TERMINAL_LEFT_PAREN] = PRECEDENCE_SHIFT,
                                 [EXPR_TERMINAL_UNARY_PLUS] = PRECEDENCE_SHIFT,
                                 [EXPR_TERMINAL_UNARY_MINUS] = PRECEDENCE_SHIFT,
                                 [EXPR_TERMINAL_MULTIPLY] = PRECEDENCE_SHIFT,
                                 [EXPR_TERMINAL_DIVIDE] = PRECEDENCE_SHIFT,
                                 [EXPR_TERMINAL_ADD] = PRECEDENCE_SHIFT,
                                 [EXPR_TERMINAL_SUBTRACT] = PRECEDENCE_SHIFT,
                                 [EXPR_TERMINAL_LESS] = PRECEDENCE_SHIFT,
                                 [EXPR_TERMINAL_LESS_EQUAL] = PRECEDENCE_SHIFT,
                                 [EXPR_TERMINAL_GREATER] = PRECEDENCE_SHIFT,
                                 [EXPR_TERMINAL_GREATER_EQUAL] = PRECEDENCE_SHIFT,
                                 [EXPR_TERMINAL_END] = PRECEDENCE_REDUCE,
                                 [EXPR_TERMINAL_RIGHT_PAREN] = PRECEDENCE_REDUCE,
                                 [EXPR_TERMINAL_EQUAL] = PRECEDENCE_ERROR,
                                 [EXPR_TERMINAL_NOT_EQUAL] = PRECEDENCE_ERROR}};

static const OperatorDefinition *operatorDefinition(ExpressionTerminal terminal) {
    for (size_t index = 0; index < sizeof(operators) / sizeof(operators[0]); index++) {
        if (operators[index].terminal == terminal) {
            return &operators[index];
        }
    }
    return NULL;
}

bool syntaxOperatorFromToken(const Token *token, ExpressionTerminal *outTerminal) {
    if (token == NULL || token->type != TOKEN_OPERATOR || token->lexeme == NULL ||
        outTerminal == NULL) {
        return false;
    }

    for (size_t index = 0; index < sizeof(operators) / sizeof(operators[0]); index++) {
        if (strcmp(token->lexeme, operators[index].text) == 0) {
            *outTerminal = operators[index].terminal;
            return true;
        }
    }
    return false;
}

const char *syntaxOperatorText(ExpressionTerminal terminal) {
    if (terminal == EXPR_TERMINAL_UNARY_PLUS) {
        return SYNTAX_OP_PLUS;
    }
    if (terminal == EXPR_TERMINAL_UNARY_MINUS) {
        return SYNTAX_OP_MINUS;
    }
    const OperatorDefinition *definition = operatorDefinition(terminal);
    return definition == NULL ? "" : definition->text;
}

static bool isPrefixOperator(ExpressionTerminal terminal) {
    return terminal == EXPR_TERMINAL_UNARY_PLUS || terminal == EXPR_TERMINAL_UNARY_MINUS ||
           terminal == EXPR_TERMINAL_LOGICAL_NOT;
}

static bool isBinaryOperator(ExpressionTerminal terminal) {
    return (terminal >= EXPR_TERMINAL_MULTIPLY && terminal <= EXPR_TERMINAL_SUBTRACT) ||
           (terminal >= EXPR_TERMINAL_LESS && terminal <= EXPR_TERMINAL_NOT_EQUAL) ||
           terminal == EXPR_TERMINAL_LOGICAL_AND || terminal == EXPR_TERMINAL_LOGICAL_OR;
}

static int binaryOperatorPrecedence(ExpressionTerminal terminal) {
    switch (terminal) {
    case EXPR_TERMINAL_MULTIPLY:
    case EXPR_TERMINAL_DIVIDE:
        return 5;
    case EXPR_TERMINAL_ADD:
    case EXPR_TERMINAL_SUBTRACT:
        return 4;
    case EXPR_TERMINAL_LESS:
    case EXPR_TERMINAL_LESS_EQUAL:
    case EXPR_TERMINAL_GREATER:
    case EXPR_TERMINAL_GREATER_EQUAL:
        return 3;
    case EXPR_TERMINAL_EQUAL:
    case EXPR_TERMINAL_NOT_EQUAL:
        return 2;
    case EXPR_TERMINAL_LOGICAL_AND:
        return 1;
    case EXPR_TERMINAL_LOGICAL_OR:
        return 0;
    default:
        return -1;
    }
}

PrecedenceRelation syntaxPrecedenceRelation(ExpressionTerminal stacked,
                                            ExpressionTerminal incoming) {
    if (stacked >= EXPR_TERMINAL_COUNT || incoming >= EXPR_TERMINAL_COUNT) {
        return PRECEDENCE_ERROR;
    }

    if (incoming == EXPR_TERMINAL_LOGICAL_NOT) {
        return stacked == EXPR_TERMINAL_END || stacked == EXPR_TERMINAL_LEFT_PAREN ||
                       isPrefixOperator(stacked) || isBinaryOperator(stacked)
                   ? PRECEDENCE_SHIFT
                   : PRECEDENCE_ERROR;
    }
    if (isPrefixOperator(stacked)) {
        return incoming == EXPR_TERMINAL_OPERAND || incoming == EXPR_TERMINAL_LEFT_PAREN ||
                       isPrefixOperator(incoming)
                   ? PRECEDENCE_SHIFT
               : incoming == EXPR_TERMINAL_END || incoming == EXPR_TERMINAL_RIGHT_PAREN ||
                       isBinaryOperator(incoming)
                   ? PRECEDENCE_REDUCE
                   : PRECEDENCE_ERROR;
    }

    if (stacked == EXPR_TERMINAL_LOGICAL_AND || stacked == EXPR_TERMINAL_LOGICAL_OR) {
        if (incoming == EXPR_TERMINAL_END || incoming == EXPR_TERMINAL_RIGHT_PAREN) {
            return PRECEDENCE_REDUCE;
        }
        if (incoming == EXPR_TERMINAL_OPERAND || incoming == EXPR_TERMINAL_LEFT_PAREN ||
            isPrefixOperator(incoming)) {
            return PRECEDENCE_SHIFT;
        }
        if (isBinaryOperator(incoming)) {
            return binaryOperatorPrecedence(stacked) < binaryOperatorPrecedence(incoming)
                       ? PRECEDENCE_SHIFT
                       : PRECEDENCE_REDUCE;
        }
        return PRECEDENCE_ERROR;
    }

    if (incoming == EXPR_TERMINAL_LOGICAL_AND || incoming == EXPR_TERMINAL_LOGICAL_OR) {
        if (stacked == EXPR_TERMINAL_END || stacked == EXPR_TERMINAL_LEFT_PAREN) {
            return PRECEDENCE_SHIFT;
        }
        if (stacked == EXPR_TERMINAL_OPERAND || stacked == EXPR_TERMINAL_RIGHT_PAREN ||
            isBinaryOperator(stacked)) {
            return PRECEDENCE_REDUCE;
        }
        return PRECEDENCE_ERROR;
    }

    return precedenceTable[stacked][incoming];
}
