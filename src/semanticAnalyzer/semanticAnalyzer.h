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

typedef struct SemAnalData SemAnalData;

/**
 * @brief Initializes semantic-analysis data and creates its function symbol table.
 * @param data The semantic-analysis data to initialize.
 * @return status of the program - error codes
 */
CompilerStatus semAnalDataInit(SemAnalData *data);

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

CompilerStatus semAnalResolveFunction(AbstractSyntaxNode *functionNode, SymbolTable *symbols);
CompilerStatus semAnalResolveParameter(AbstractSyntaxNode *parameterNode, SymbolTable *symbols);
CompilerStatus semAnalResolveBlock(AbstractSyntaxNode *blockNode, SymbolTable *symbols);
CompilerStatus semAnalResolveIf(AbstractSyntaxNode *ifNode, SymbolTable *symbols);
CompilerStatus semAnalResolveWhile(AbstractSyntaxNode *whileNode, SymbolTable *symbols);
CompilerStatus semAnalResolveReturn(AbstractSyntaxNode *returnNode, SymbolTable *symbols);
CompilerStatus semAnalResolveFunctionCall(AbstractSyntaxNode *functionCallNode, SymbolTable *symbols);
CompilerStatus semAnalResolveVariableDeclaration(AbstractSyntaxNode *variableDeclarationNode, SymbolTable *symbols);
CompilerStatus semAnalResolveAssignment(AbstractSyntaxNode *assignmentNode, SymbolTable *symbols);
CompilerStatus semAnalResolveExpressionStatement(AbstractSyntaxNode *expressionStatementNode, SymbolTable *symbols);
CompilerStatus semAnalResolveExpressionBinary(AbstractSyntaxNode *expressionBinaryNode, SymbolTable *symbols);
CompilerStatus semAnalResolveUnaryExpression(AbstractSyntaxNode *unaryExpressionNode, SymbolTable *symbols);
CompilerStatus semAnalResolveIdentifier(AbstractSyntaxNode *identifierNode, SymbolTable *symbols);

#endif
