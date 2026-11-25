/**
 * @file  parserStatements.h
 *
 * @project IFJ26 compiler
 * @author Matúš Čík xcikmat00
 */

#ifndef PARSER_STATEMENTS_H
#define PARSER_STATEMENTS_H

#include "syntaxAnalyzerUtils.h"

/**
 * @brief Parses an if statement with an optional else branch.
 * @param syntaxAnalyzer The syntax analyzer positioned at the `if` keyword.
 * @param outNode Receives an owned AST node on success.
 * @return COMPILER_OK on success, or the corresponding compiler error code.
 * @note The AST contains an empty else block when there is no else branch.
 */
CompilerStatus parseIf(SyntaxAnalyzer *syntaxAnalyzer, AbstractSyntaxNode **outNode);

/**
 * @brief Parses a while statement with a condition and a body.
 * @param syntaxAnalyzer The syntax analyzer positioned at the `while` keyword.
 * @param outNode Receives an owned AST node on success.
 * @return COMPILER_OK on success, or the corresponding compiler error code.
 * @note The body can be an inline statement or an indented block.
 */
CompilerStatus parseWhile(SyntaxAnalyzer *syntaxAnalyzer, AbstractSyntaxNode **outNode);

/**
 * @brief Parses a for loop and stores its clauses in source order.
 * @param syntaxAnalyzer The syntax analyzer positioned at the `for` keyword.
 * @param outNode Receives an owned AST node on success.
 * @return COMPILER_OK on success, or the corresponding compiler error code.
 * @note The header must contain exactly one generator and may include guards.
 */
CompilerStatus parseFor(SyntaxAnalyzer *syntaxAnalyzer, AbstractSyntaxNode **outNode);

/**
 * @brief Parses a single statement.
 * @param syntaxAnalyzer The syntax analyzer to use for parsing.
 * @param outNode Receives an owned AST node on success.
 * @return COMPILER_OK on success, or the corresponding compiler error code.
 * @note A function call is stored directly. Other expressions get an expression statement node.
 */
CompilerStatus parseStatement(SyntaxAnalyzer *syntaxAnalyzer, AbstractSyntaxNode **outNode);

/**
 * @brief Parses an assignment to a bare identifier.
 * @param syntaxAnalyzer The syntax analyzer to use for parsing.
 * @param outNode Receives an owned AST node on success.
 * @return COMPILER_OK on success, or the corresponding compiler error code.
 */
CompilerStatus parseAssignment(SyntaxAnalyzer *syntaxAnalyzer, AbstractSyntaxNode **outNode);

/**
 * @brief Parses an indented block with at least one statement.
 * @param syntaxAnalyzer The syntax analyzer to use for parsing.
 * @param outNode Receives an owned AST node on success.
 * @return COMPILER_OK on success, or the corresponding compiler error code.
 * @note Consumes the opening INDENT and its matching DEDENT.
 */
CompilerStatus parseBlock(SyntaxAnalyzer *syntaxAnalyzer, AbstractSyntaxNode **outNode);

#endif
