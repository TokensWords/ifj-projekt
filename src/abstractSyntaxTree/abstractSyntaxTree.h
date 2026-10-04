/*
 * Project: IFJ26 compiler
 * Authors: TODO(team)
 * AI-assisted scaffold.
 * Prompt: Define a flexible abstract-syntax-tree ownership contract.
 */

#ifndef ABSTRACT_SYNTAX_TREE_H
#define ABSTRACT_SYNTAX_TREE_H

#include "compilerTypes.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    ABSTRACT_SYNTAX_PROGRAM,
    ABSTRACT_SYNTAX_FUNCTION,
    ABSTRACT_SYNTAX_PARAMETER,
    ABSTRACT_SYNTAX_BLOCK,
    ABSTRACT_SYNTAX_VARIABLE_DECLARATION,
    ABSTRACT_SYNTAX_ASSIGNMENT,
    ABSTRACT_SYNTAX_FUNCTION_CALL,
    ABSTRACT_SYNTAX_RETURN,
    ABSTRACT_SYNTAX_IF,
    ABSTRACT_SYNTAX_WHILE,
    ABSTRACT_SYNTAX_EXPRESSION_STATEMENT,
    ABSTRACT_SYNTAX_BINARY_EXPRESSION,
    ABSTRACT_SYNTAX_UNARY_EXPRESSION,
    ABSTRACT_SYNTAX_IDENTIFIER,
    ABSTRACT_SYNTAX_INT_LITERAL,
    ABSTRACT_SYNTAX_DOUBLE_LITERAL,
    ABSTRACT_SYNTAX_STRING_LITERAL,
    ABSTRACT_SYNTAX_UNIT_LITERAL,
    ABSTRACT_SYNTAX_BOOLEAN_LITERAL
} AbstractSyntaxNodeKind;

typedef union {
    int64_t intValue;
    double doubleValue;
    bool boolValue;
} AbstractSyntaxValue;

typedef struct AbstractSyntaxNode {
    AbstractSyntaxNodeKind kind;
    SourceLocation location;
    DataType inferredType;
    char *text;
    AbstractSyntaxValue value;
    struct AbstractSyntaxNode **children;
    size_t childCount;
    size_t childCapacity;
    bool isEntryFunction;
} AbstractSyntaxNode;

typedef struct {
    AbstractSyntaxNode *root;
} SourceProgram;

SourceProgram *sourceProgramCreate(void);
void sourceProgramDestroy(SourceProgram *program);
AbstractSyntaxNode *abstractSyntaxNodeCreate(AbstractSyntaxNodeKind kind, SourceLocation location);
void abstractSyntaxNodeDestroy(AbstractSyntaxNode *node);
CompilerStatus abstractSyntaxNodeSetText(AbstractSyntaxNode *node, const char *text);
CompilerStatus abstractSyntaxNodeAddChild(AbstractSyntaxNode *parent, AbstractSyntaxNode *child);

#endif
