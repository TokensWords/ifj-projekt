/**
 * PROJECT: IFJ26 compiler
 * AUTHORS: Matúš Čík
 * BRIEF: Abstract syntax tree representation and ownership helpers
 * 
 * IDEA: Remove struct ASTNODE field isMutable and isEntryFunction by adding more ASTNODE types, example: ABSTRACT_SYNTAX_FUNCTION_ENTRY, ABSTRACT_SYNTAX_VARIABLE_DECLARATION_MUTABLE, ABSTRACT_SYNTAX_VARIABLE_DECLARATION_IMMUTABLE.
 * IDEA: Make ASTNODE easier to work with by introducing funtions of getters/setter for node types.
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
     * Represents an if expression in assigment statement
     * @children - condition, then expression, else expression. example: [condition, thenExpression, elseExpression]
     */
    ABSTRACT_SYNTAX_IF_EXPRESSION,

    /**
     * Represents a while loop
     * @children - condition then block of the loop. example: [condition, block]
     */
    ABSTRACT_SYNTAX_WHILE,

    /**
     * Represents a for loop
     * @children - clauses and body, in that order
     */
    ABSTRACT_SYNTAX_FOR,

    /**
     * Ordered generator and guard clauses of a for loop
     */
    ABSTRACT_SYNTAX_FOR_CLAUSES,

    /**
     * Represents a generator binding
     * @text - binding name
     * @children - its source (currently a range)
     */
    ABSTRACT_SYNTAX_FOR_GENERATOR,

    /**
     * Represents a generator guard
     * @children - guard condition
     */
    ABSTRACT_SYNTAX_FOR_GUARD,

    /**
     * Represents a numeric range source
     * @text - "to" (inclusive) or "until" (exclusive)
     * @children - start, end, optional step
     */
    ABSTRACT_SYNTAX_FOR_RANGE,

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

    /**
     * Represents a unary expression
     * @text - unary operator
     * @children - operand
     */
    ABSTRACT_SYNTAX_UNARY_EXPRESSION,

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
    union {
        AbstractSyntaxNodeType type;
        AbstractSyntaxNodeType kind;
    };
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
    bool isMutable;
} AbstractSyntaxNode;

typedef struct {
    AbstractSyntaxNode *root;
} SourceProgram;

SourceProgram *sourceProgramCreate(void);
void sourceProgramDestroy(SourceProgram *program);
AbstractSyntaxNode *abstractSyntaxNodeCreate(AbstractSyntaxNodeType  type, SourceLocation location);
void abstractSyntaxNodeDestroy(AbstractSyntaxNode *node);
CompilerStatus abstractSyntaxNodeSetText(AbstractSyntaxNode *node, const char *text);
CompilerStatus abstractSyntaxNodeAddChild(AbstractSyntaxNode *parent, AbstractSyntaxNode *child);

#endif
