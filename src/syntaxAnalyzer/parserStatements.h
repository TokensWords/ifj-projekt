/**
 * @file  parserStatements.h
 *
 * @project IFJ26 compiler
 * @author Matúš Čík
 */

#ifndef PARSER_STATEMENTS_H
#define PARSER_STATEMENTS_H

#include "syntaxAnalyzerUtils.h"

/** @brief Parses an if statement and stores its AST node in outNode. */
CompilerStatus parseIf(SyntaxAnalyzer *syntaxAnalyzer, AbstractSyntaxNode **outNode);

/** @brief Parses a while statement and stores its AST node in outNode. */
CompilerStatus parseWhile(SyntaxAnalyzer *syntaxAnalyzer, AbstractSyntaxNode **outNode);

/** @brief Parses one statement and stores its AST node in outNode. */
CompilerStatus parseStatement(SyntaxAnalyzer *syntaxAnalyzer, AbstractSyntaxNode **outNode);

/** @brief Parses an assignment. */
CompilerStatus parseAssignment(SyntaxAnalyzer *syntaxAnalyzer,
                               AbstractSyntaxNode **outNode);

/** @brief Parses block of statements. */
CompilerStatus parseBlock(SyntaxAnalyzer *syntaxAnalyzer,
                          AbstractSyntaxNode **outNode);

#endif
