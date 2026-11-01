/*
 * Project: IFJ26 compiler
 * Authors: Václav Krytinář xkrytiv00
 * AI-assisted scaffold.
 * Prompt: Define semantic-analysis inputs, outputs, and status reporting.
 */

#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "abstractSyntaxTree.h"
#include "compilerTypes.h"
#include "symbolTable.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct SemAnalData SemAnalData;

/**
 * @brief Initializes semantic-analysis data and creates its function symbol table.
 * @param data The semantic-analysis data to initialize.
 * @return status of the program - error codes
 */
CompilerStatus semAnalDataInit(SemAnalData *data);

/**
 * @brief Expands the semantic-analysis symbol-table and recursion lists.
 * @param data The semantic-analysis data containing the lists.
 * @return status of the program - error codes
 */
CompilerStatus semAnalListsBigger(SemAnalData *data);

/**
 * @brief Removes the last entries from the semantic-analysis lists.
 * @param data The semantic-analysis data containing the lists.
 * @return status of the program - error codes
 */
CompilerStatus semAnalListsSmaller(SemAnalData *data);

/**
 * @brief Destroys resources owned by semantic-analysis data.
 * @param data The semantic-analysis data to destroy.
 */
void semAnalDataDestroy(SemAnalData *data);

/**
 * @brief goes through functions in AST and checks semantics
 * @param program sorce program AST got from syntax analyser
 * @param symbols symbol table to work with
 * @return status of the program - error codes
 */
CompilerStatus semanticAnalyzerAnalyze(SourceProgram *program, SymbolTable *symbols);

/**
 * @brief Allocates a new symbol and initializes with default values.
 * @return The created symbol.
 */
Symbol *semAnalCreateSymbolDefault();

/**
 * @brief Frees a symbol and its allocated function parameter types.
 * @param symbol The symbol to free.
 */
void semAnalFreeSymbol(Symbol *symbol);

/**
 * @brief gets parameter types from AST node
 * @param node pointer to AST function node to get parameter types from
 * @return array of data types
 */
DataType *semAnalAstGetParameterTypes(AbstractSyntaxNode *node);

/**
 * @brief Extracts information from current ast node to a new symbol.
 * This function does only extract information from the node + function parameter types from
 * function nodes. It does NOT set coresponding scope depth (default 0). It sets defined to false.
 * @param node node to be extracted from
 * @return status of the program - error codes
 */
Symbol *semAnalAstNodeToSymbol(AbstractSyntaxNode *node);

/**
 * @brief Adds builtin functions to the table of symbols.
 * @param symbols Table of symbols to insert builtin functions to.
 * @return status of the program - error codes
 */
CompilerStatus semAnalDefineBuiltinFunctions(SymbolTable *symbols);

/**
 * @brief Adds all user functions to the table of symbols.
 * It adds function name, explicit return type and parameters.
 * It does not check the function body or correct return semantics.
 * @param rootNode Root node of the AST.
 * @param symbols Table of symbols to insert the functions to.
 * @return status of the program - error codes
 */
CompilerStatus semAnalDeclareFunctions(AbstractSyntaxNode *rootNode, SymbolTable *symbols);

/**
 * @brief Checks that an entry function has no parameters and returns unit.
 * @param functionNode The function AST node to check.
 * @return status of the program - error codes
 */
CompilerStatus semAnalCheckMain(AbstractSyntaxNode *functionNode);

/**
 * @brief Checks whether an AST node ends with a return statement.
 * @param node The AST node to check.
 * @return true if the node ends with a return statement, otherwise false.
 */
bool semAnalNodeEndsWithReturn(AbstractSyntaxNode *node);

/**
 * @brief Checks whether both branches of an if statement return.
 * @param ifNode The if-statement AST node to check.
 * @return true if all branches return, otherwise false.
 */
bool semAnalCheckIfReturns(AbstractSyntaxNode *ifNode);

/**
 * @brief Resolves and validates a function's return type from its body.
 * @param functionNode The function AST node.
 * @param functionSymbol The symbol associated with the function.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveFunctionReturnType(AbstractSyntaxNode *functionNode, Symbol *functionSymbol);

/**
 * @brief Defines a function by resolving its parameters and body.
 * @param functionNode The function AST node to define.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalDefineFunction(AbstractSyntaxNode *functionNode, SemAnalData *data);

/**
 * @brief Declares a function parameter in the symbol table.
 * @param parameterNode The parameter AST node to declare.
 * @param symbols The symbol table in which to declare the parameter.
 * @return status of the program - error codes
 */
CompilerStatus semAnalDeclareParameter(AbstractSyntaxNode *parameterNode, SymbolTable *symbols);

/**
 * @brief Resolves the statements in a block.
 * @param blockNode The block AST node to resolve.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveBlock(AbstractSyntaxNode *blockNode, SemAnalData *data);

/**
 * @brief Resolves an AST node and performs semantic checks.
 * Basically a switch to call correct resolve function.
 * @param node The AST node to resolve.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveNode(AbstractSyntaxNode *node, SemAnalData *data);

/**
 * @brief Resolves an if statement and its branches.
 * @param ifNode The if-statement AST node to resolve.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveIf(AbstractSyntaxNode *ifNode, SemAnalData *data);

/**
 * @brief Resolves a while statement and its body.
 * @param whileNode The while-statement AST node to resolve.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveWhile(AbstractSyntaxNode *whileNode, SemAnalData *data);

/**
 * @brief Resolves a return statement and checks its return type.
 * @param returnNode The return-statement AST node to resolve.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveReturn(AbstractSyntaxNode *returnNode, SemAnalData *data);

/**
 * @brief Finds a function in the current recursion list.
 * @param funName The function name to find.
 * @param data The semantic-analysis data containing the recursion list.
 * @return The function's index if found, -1 if it is not recursive, or -2 on invalid input.
 */
int semAnalInRecursion(const char *funName, SemAnalData *data);

/**
 * @brief Checks that functions in the recursion list have declared return types.
 * @param recursionI The index in the recursion list to start checking from.
 * @param data The semantic-analysis data containing the recursion list and function symbols.
 * @return status of the program - error codes
 */
CompilerStatus semAnalCheckRecursion(size_t recursionI, SemAnalData *data);

/**
 * @brief Resolves a function call and checks its arguments.
 * @param functionCallNode The function-call AST node to resolve.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveFunctionCall(AbstractSyntaxNode *functionCallNode, SemAnalData *data);

/**
 * @brief Resolves a variable declaration and checks its type.
 * @param variableDeclarationNode The variable-declaration AST node to resolve.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveVariableDeclaration(AbstractSyntaxNode *variableDeclarationNode, SemAnalData *data);

/**
 * @brief Resolves an assignment and checks its type.
 * @param assignmentNode The assignment AST node to resolve.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveAssignment(AbstractSyntaxNode *assignmentNode, SemAnalData *data);

/**
 * @brief Resolves an expression statement.
 * @param expressionStatementNode The expression-statement AST node to resolve.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveExpressionStatement(AbstractSyntaxNode *expressionStatementNode, SemAnalData *data);

/**
 * @brief Resolves a binary expression and checks operand types.
 * @param expressionBinaryNode The binary-expression AST node to resolve.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveExpressionBinary(AbstractSyntaxNode *expressionBinaryNode, SemAnalData *data);

/**
 * @brief Resolves a unary expression and checks its operand type.
 * @param unaryExpressionNode The unary-expression AST node to resolve.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveUnaryExpression(AbstractSyntaxNode *unaryExpressionNode, SemAnalData *data);

/**
 * @brief Resolves an identifier and looks up its symbol.
 * @param identifierNode The identifier AST node to resolve.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveIdentifier(AbstractSyntaxNode *identifierNode, SemAnalData *data);

/**
 * @brief Gets the data type to which a "line of code" should be evaluated to.
 * @param node The AST node to evaluate.
 * @return The data type produced by the node.
 */
DataType semAnalEvaluatesToType(AbstractSyntaxNode *node);

#endif
