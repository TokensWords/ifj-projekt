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

CompilerStatus semanticAnalyzerAnalyze(SourceProgram *program, SymbolTable *symbols);

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
CompilerStatus semAnalResolveIntLiteral(AbstractSyntaxNode *intLiteralNode, SymbolTable *symbols);
CompilerStatus semAnalResolveDoubleLiteral(AbstractSyntaxNode *doubleLiteralNode, SymbolTable *symbols);
CompilerStatus semAnalResolveStringLiteral(AbstractSyntaxNode *stringLiteralNode, SymbolTable *symbols);
CompilerStatus semAnalResolveBooleanLiteral(AbstractSyntaxNode *booleanLiteralNode, SymbolTable *symbols);
CompilerStatus semAnalResolveUnitLiteral(AbstractSyntaxNode *unitLiteralNode, SymbolTable *symbols);

#endif

