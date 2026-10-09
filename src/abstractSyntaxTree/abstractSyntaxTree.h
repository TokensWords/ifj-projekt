/**
 * @author Matúš Čík
 */

#ifndef ABSTRACT_SYNTAX_TREE_H
#define ABSTRACT_SYNTAX_TREE_H

#include "compilerTypes.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum {

    // --- structural ---

    /**
     * Represents the root of the abstract syntax tree
     * @children - top-level functions in the program
     */
    ABSTRACT_SYNTAX_PROGRAM,
    
    /**
     * Represents a function declaration
     * @text - name of the function
     * @children - parameters and body of the function. Parameters are folowed by body. example: [param1, param2, body]. It must have a body
     * @isEntryFunction - true if this function is the entry point of the program (taggeg by `@main`), else false
     */
    ABSTRACT_SYNTAX_FUNCTION,

    /**
     * Represents a function parameter
     * @text - name of the parameter
     * @variableType - type of the parameter
     */
    ABSTRACT_SYNTAX_PARAMETER,

    /**
     * Represents a block of statements
     * @children - statements in the block
     */
    ABSTRACT_SYNTAX_BLOCK,

    /**
     * Represents an if statement
     * @children - condition, then block, else block (optional). example: [condition, thenBlock, elseBlock]
     */
    ABSTRACT_SYNTAX_IF,

    /**
     * Represents a while loop
     * @children - condition then block of the loop. example: [condition, block]
     */
    ABSTRACT_SYNTAX_WHILE,

    /**
     * Represents a return statement
     * @children - expression being returned (optional)
     */
    ABSTRACT_SYNTAX_RETURN,

    // --- expressions ---

    /**
     * Represents a function call
     * @text - name of the function
     * @children - arguments to the function (optional)
     */
    ABSTRACT_SYNTAX_FUNCTION_CALL,

    /**
     * Represents a variable declaration
     * @text - name of the variable
     * @variableType - type of the variable
     */
    ABSTRACT_SYNTAX_VARIABLE_DECLARATION,

    /**
     * Represents an assignment
     * @text - name of the variable being assigned to
     * @children - assigned expression
     */
    ABSTRACT_SYNTAX_ASSIGNMENT,

    /**
     * Represents an expression statement
     * @children - the expression (1 child)
     */
    ABSTRACT_SYNTAX_EXPRESSION_STATEMENT,

    /**
     * Represents a binary expression
     * @children - left and right operands. example: [left, right]
     */
    ABSTRACT_SYNTAX_EXPRESSION_BINARY,

    // --- values ---

    /**
     * Represents a variable in an expression
     * @text - variable name
     */
    ABSTRACT_SYNTAX_IDENTIFIER,

    /**
     * Represents an integer literal
     * @value.intValue
     */
    ABSTRACT_SYNTAX_INT_LITERAL,

    /**
     * Represents a double literal
     * @value.doubleValue
     */
    ABSTRACT_SYNTAX_DOUBLE_LITERAL,

    /**
     * Represents a string literal
     * @text - the string value
     */
    ABSTRACT_SYNTAX_STRING_LITERAL,

    /**
     * Represents a boolean literal
     * @AbstractSyntaxValue.boolValue
     */
    ABSTRACT_SYNTAX_BOOLEAN_LITERAL,

    /**
     * Represents a unit literal. Unit is like void
     */
    ABSTRACT_SYNTAX_UNIT_LITERAL
} AbstractSyntaxNodeType;

typedef union {
    int64_t intValue;
    double doubleValue;
    bool boolValue;
} AbstractSyntaxValue;

typedef struct AbstractSyntaxNode {
    AbstractSyntaxNodeType type;
    SourceLocation location;
    DataType inferredType;
    DataType variableType;
    char *text;
    AbstractSyntaxValue value;
    struct AbstractSyntaxNode **children;
    size_t childCount;
    size_t childCapacity;
    size_t variableId;
    bool isEntryFunction;
} AbstractSyntaxNode;

typedef struct {
    AbstractSyntaxNode *root;
} SourceProgram;

SourceProgram *sourceProgramCreate(void);
void sourceProgramDestroy(SourceProgram *program);
AbstractSyntaxNode *abstractSyntaxNodeCreate(AbstractSyntaxNodeType  type, SourceLocation location);
void abstractSyntaxNodeDestroy(AbstractSyntaxNode *node);

#endif
