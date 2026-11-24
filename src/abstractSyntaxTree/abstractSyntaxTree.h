/**
 * PROJECT: IFJ26 compiler
 * AUTHORS: Matúš Čík xcikmat00
 * BRIEF: Abstract syntax tree representation and ownership helpers
 *
 */

#ifndef ABSTRACT_SYNTAX_TREE_H
#define ABSTRACT_SYNTAX_TREE_H

#include "compilerTypes.h"
#include "typeDescriptor.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum {

    // --- structural ---

    /**
     * Represents the root of the abstract syntax tree
     * @children - top-level functions, type aliases, and case classes
     */
    ABSTRACT_SYNTAX_PROGRAM,

    /**
     * Represents a function declaration
     * @text - name of the function
     * @children - parameters and body of the function. Parameters are folowed by body. example:
     * [param1, param2, body]. It must have a body
     * @isEntryFunction - true if this function is the entry point of the program (taggeg by
     * `@main`), else false
     */
    ABSTRACT_SYNTAX_FUNCTION,

    /** A top-level type alias; @text is its name and @type is its definition. */
    ABSTRACT_SYNTAX_TYPE_ALIAS,

    /**
     * A top-level case class declaration.
     * @text - class name
     * @children - its public immutable fields, in declaration order
     */
    ABSTRACT_SYNTAX_CASE_CLASS,

    /**
     * A public immutable case-class field.
     * @text - field name
     * @type - declared field type
     */
    ABSTRACT_SYNTAX_CASE_CLASS_FIELD,

    /**
     * Represents a function parameter
     * @text - name of the parameter
     * @type - type of the parameter
     */
    ABSTRACT_SYNTAX_PARAMETER,

    /**
     * Represents a block of statements
     * @children - statements in the block
     */
    ABSTRACT_SYNTAX_BLOCK,

    /**
     * Represents an if statement
     * @children - condition, then block, else block (optional). example: [condition, thenBlock,
     * elseBlock]
     */
    ABSTRACT_SYNTAX_IF,

    /**
     * Represents an if expression
     * @children - condition, then branch, else branch. A branch is either an expression or an
     * AST_BLOCK when written across multiple lines. example: [condition, thenBranch, elseBranch]
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
     * @children - its source expression (a range or collection expression)
     */
    ABSTRACT_SYNTAX_FOR_GENERATOR,

    /**
     * Represents a generator guard
     * @children - guard condition
     */
    ABSTRACT_SYNTAX_FOR_GUARD,

    /**
     * Represents a range expression.
     * @text - "to" (inclusive) or "until" (exclusive)
     * @children - start, end, and optional step
     */
    ABSTRACT_SYNTAX_RANGE_EXPRESSION,

    /* TODO: compability alias, remove */
    ABSTRACT_SYNTAX_FOR_RANGE = ABSTRACT_SYNTAX_RANGE_EXPRESSION,

    /**
     * Represents a return statement
     * @children - expression being returned (optional)
     */
    ABSTRACT_SYNTAX_RETURN,

    // --- expressions ---

    /**
     * A match expression.
     * @children - scrutinee followed by ordered case branches
     */
    ABSTRACT_SYNTAX_MATCH_EXPRESSION,

    /**
     * One match branch.
     * @children - pattern, optional ABSTRACT_SYNTAX_MATCH_GUARD, result expression
     */
    ABSTRACT_SYNTAX_MATCH_CASE,

    /** A guard expression attached to a match case; @children contains its expression. */
    ABSTRACT_SYNTAX_MATCH_GUARD,

    /**
     * Represents calling any expression.
     * @children - callee expression followed by argument expressions
     */
    ABSTRACT_SYNTAX_CALL_EXPRESSION,

    /* TODO: compability alias, remove */
    ABSTRACT_SYNTAX_FUNCTION_CALL = ABSTRACT_SYNTAX_CALL_EXPRESSION,

    /**
     * Represents member selection on an expression.
     * @text - selected member name
     * @children - receiver expression
     */
    ABSTRACT_SYNTAX_MEMBER_ACCESS,

    /**
     * Represents a variable declaration
     * @text - name of the variable
     * @type - declared or inferred type of the variable
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
    /* Both spellings identify the same node; retain the intermediate-code API. */
    ABSTRACT_SYNTAX_BINARY_EXPRESSION = ABSTRACT_SYNTAX_EXPRESSION_BINARY,

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
} AbstractSyntaxNodeKind;

typedef union {
    int64_t intValue;
    double doubleValue;
    bool boolValue;
} AbstractSyntaxValue;

typedef struct AbstractSyntaxNode {
    AbstractSyntaxNodeKind kind;
    SourceLocation location;
    TypeDescriptor type;
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
AbstractSyntaxNode *abstractSyntaxNodeCreate(AbstractSyntaxNodeKind type, SourceLocation location);
void abstractSyntaxNodeDestroy(AbstractSyntaxNode *node);
CompilerStatus abstractSyntaxNodeSetText(AbstractSyntaxNode *node, const char *text);
CompilerStatus abstractSyntaxNodeAddChild(AbstractSyntaxNode *parent, AbstractSyntaxNode *child);
CompilerStatus abstractSyntaxNodeSetInferredType(AbstractSyntaxNode *node, DataType type);
CompilerStatus abstractSyntaxNodeCopyInferredType(AbstractSyntaxNode *node, const TypeDescriptor *type);
CompilerStatus abstractSyntaxNodeSetVariableType(AbstractSyntaxNode *node, DataType type);
#endif
