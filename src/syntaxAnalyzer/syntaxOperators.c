/**
 * @file  syntaxOperators.c
 *
 * @project IFJ26 compiler
 * @author Matúš Čík xcikmat00
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
    {EXPR_TERMINAL_LOGICAL_OR, SYNTAX_OP_LOGICAL_OR},
    {EXPR_TERMINAL_RANGE_TO, SYNTAX_KW_TO},
    {EXPR_TERMINAL_RANGE_UNTIL, SYNTAX_KW_UNTIL},
    {EXPR_TERMINAL_RANGE_BY, SYNTAX_KW_BY}};

#define S PRECEDENCE_SHIFT
#define E PRECEDENCE_EQUAL
#define R PRECEDENCE_REDUCE
#define X PRECEDENCE_ERROR

/* Rows are stacked terminals. Columns follow the order of ExpressionTerminal. */
// clang-format off
static const PrecedenceRelation precedenceTable[EXPR_TERMINAL_COUNT][EXPR_TERMINAL_COUNT] = {
                                     /*   end    val      (      )      *      /      +      -     u+     u-      <     <=      >     >=     ==     !=      !     &&     ||     to  until     by */
    [EXPR_TERMINAL_END] = {                 X,     S,     S,     X,     S,     S,     S,     S,     S,     S,     S,     S,     S,     S,     S,     S,     S,     S,     S,     S,     S,     S },
    [EXPR_TERMINAL_OPERAND] = {             R,     X,     X,     R,     R,     R,     R,     R,     X,     X,     R,     R,     R,     R,     R,     R,     X,     R,     R,     R,     R,     R },
    [EXPR_TERMINAL_LEFT_PAREN] = {          X,     S,     S,     E,     S,     S,     S,     S,     S,     S,     S,     S,     S,     S,     S,     S,     S,     S,     S,     S,     S,     X },
    [EXPR_TERMINAL_RIGHT_PAREN] = {         R,     X,     X,     R,     R,     R,     R,     R,     X,     X,     R,     R,     R,     R,     R,     R,     X,     R,     R,     R,     R,     R },
    [EXPR_TERMINAL_MULTIPLY] = {            R,     S,     S,     R,     R,     R,     R,     R,     S,     S,     R,     R,     R,     R,     R,     R,     S,     R,     R,     R,     R,     R },
    [EXPR_TERMINAL_DIVIDE] = {              R,     S,     S,     R,     R,     R,     R,     R,     S,     S,     R,     R,     R,     R,     R,     R,     S,     R,     R,     R,     R,     R },
    [EXPR_TERMINAL_ADD] = {                 R,     S,     S,     R,     S,     S,     R,     R,     S,     S,     R,     R,     R,     R,     R,     R,     S,     R,     R,     R,     R,     R },
    [EXPR_TERMINAL_SUBTRACT] = {            R,     S,     S,     R,     S,     S,     R,     R,     S,     S,     R,     R,     R,     R,     R,     R,     S,     R,     R,     R,     R,     R },
    [EXPR_TERMINAL_UNARY_PLUS] = {          R,     S,     S,     R,     R,     R,     R,     R,     S,     S,     R,     R,     R,     R,     R,     R,     S,     R,     R,     R,     R,     R },
    [EXPR_TERMINAL_UNARY_MINUS] = {         R,     S,     S,     R,     R,     R,     R,     R,     S,     S,     R,     R,     R,     R,     R,     R,     S,     R,     R,     R,     R,     R },
    [EXPR_TERMINAL_LESS] = {                R,     S,     S,     R,     S,     S,     S,     S,     S,     S,     X,     X,     X,     X,     R,     R,     S,     R,     R,     R,     R,     R },
    [EXPR_TERMINAL_LESS_EQUAL] = {          R,     S,     S,     R,     S,     S,     S,     S,     S,     S,     X,     X,     X,     X,     R,     R,     S,     R,     R,     R,     R,     R },
    [EXPR_TERMINAL_GREATER] = {             R,     S,     S,     R,     S,     S,     S,     S,     S,     S,     X,     X,     X,     X,     R,     R,     S,     R,     R,     R,     R,     R },
    [EXPR_TERMINAL_GREATER_EQUAL] = {       R,     S,     S,     R,     S,     S,     S,     S,     S,     S,     X,     X,     X,     X,     R,     R,     S,     R,     R,     R,     R,     R },
    [EXPR_TERMINAL_EQUAL] = {               R,     S,     S,     R,     S,     S,     S,     S,     S,     S,     S,     S,     S,     S,     X,     X,     S,     R,     R,     R,     R,     R },
    [EXPR_TERMINAL_NOT_EQUAL] = {           R,     S,     S,     R,     S,     S,     S,     S,     S,     S,     S,     S,     S,     S,     X,     X,     S,     R,     R,     R,     R,     R },
    [EXPR_TERMINAL_LOGICAL_NOT] = {         R,     S,     S,     R,     R,     R,     R,     R,     S,     S,     R,     R,     R,     R,     R,     R,     S,     R,     R,     R,     R,     R },
    [EXPR_TERMINAL_LOGICAL_AND] = {         R,     S,     S,     R,     S,     S,     S,     S,     S,     S,     S,     S,     S,     S,     S,     S,     S,     R,     R,     R,     R,     R },
    [EXPR_TERMINAL_LOGICAL_OR] = {          R,     S,     S,     R,     S,     S,     S,     S,     S,     S,     S,     S,     S,     S,     S,     S,     S,     S,     R,     R,     R,     R },
    [EXPR_TERMINAL_RANGE_TO] = {            R,     S,     S,     R,     S,     S,     S,     S,     S,     S,     S,     S,     S,     S,     R,     R,     S,     R,     R,     X,     X,     R },
    [EXPR_TERMINAL_RANGE_UNTIL] = {         R,     S,     S,     R,     S,     S,     S,     S,     S,     S,     S,     S,     S,     S,     R,     R,     S,     R,     R,     X,     X,     R },
    [EXPR_TERMINAL_RANGE_BY] = {            R,     S,     S,     R,     S,     S,     S,     S,     S,     S,     S,     S,     S,     S,     R,     R,     S,     R,     R,     R,     R,     X },
};
// clang-format on

#undef S
#undef E
#undef R
#undef X

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

PrecedenceRelation syntaxPrecedenceRelation(ExpressionTerminal stacked, ExpressionTerminal incoming) {
    if ((unsigned)stacked >= EXPR_TERMINAL_COUNT || (unsigned)incoming >= EXPR_TERMINAL_COUNT) {
        return PRECEDENCE_ERROR;
    }
    return precedenceTable[stacked][incoming];
}
