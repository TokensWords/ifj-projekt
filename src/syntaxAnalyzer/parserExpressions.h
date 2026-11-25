/**
 * @file  parserExpressions.h
 *
 * @project IFJ26 compiler
 * @author Matúš Čík xcikmat00
 */

#ifndef PARSER_EXPRESSIONS_H
#define PARSER_EXPRESSIONS_H

#include "syntaxAnalyzerUtils.h"

/**
 * @brief Parses an identifier and copies its name into an AST node.
 * @param syntaxAnalyzer The syntax analyzer to use for parsing.
 * @param outNode Receives an owned AST node on success. The output stays unchanged on failure.
 * @return COMPILER_OK on success, or the corresponding compiler error code.
 */
CompilerStatus parseIdentifierNode(SyntaxAnalyzer *syntaxAnalyzer, AbstractSyntaxNode **outNode);

/**
 * @brief Parses a single identifier or literal.
 * @param syntaxAnalyzer The syntax analyzer to use for parsing.
 * @param outNode Receives an owned AST node on success.
 * @return COMPILER_OK on success, or the corresponding compiler error code.
 * @note The parser accepts `()` as a Unit literal.
 */
CompilerStatus parseTerm(SyntaxAnalyzer *syntaxAnalyzer, AbstractSyntaxNode **outNode);

/**
 * @brief Parses a named function call and its argument list.
 * @param syntaxAnalyzer The syntax analyzer to use for parsing.
 * @param outNode Receives an owned AST node on success.
 * @return COMPILER_OK on success, or the corresponding compiler error code.
 * @note The function name must be a single identifier.
 */
CompilerStatus parseFunctionCall(SyntaxAnalyzer *syntaxAnalyzer, AbstractSyntaxNode **outNode);

/**
 * @brief Parses an expression using operator precedence.
 * @param syntaxAnalyzer The syntax analyzer to use for parsing.
 * @param outNode Receives an owned AST node on success.
 * @return COMPILER_OK on success, or the corresponding compiler error code.
 * @note The parser leaves the token that ends the expression for the caller.
 */
CompilerStatus analyzerExpressionsParse(SyntaxAnalyzer *syntaxAnalyzer,
                                        AbstractSyntaxNode **outNode);

/**
 * @brief Parses an expression in a for-loop clause.
 * @param syntaxAnalyzer The syntax analyzer to use for parsing.
 * @param outNode Receives an owned AST node on success.
 * @return COMPILER_OK on success, or the corresponding compiler error code.
 * @note The parser stops before an `if` after a complete expression
 *     so the caller can parse the guard.
 */
CompilerStatus analyzerExpressionsParseForClause(SyntaxAnalyzer *syntaxAnalyzer,
                                                 AbstractSyntaxNode **outNode);

/**
 * @brief Parses a value expression, including function calls.
 * @param syntaxAnalyzer The syntax analyzer to use for parsing.
 * @param outNode Receives an owned AST node on success.
 * @return COMPILER_OK on success, or the corresponding compiler error code.
 */
CompilerStatus analyzerExpressionsParseValueOrCall(SyntaxAnalyzer *syntaxAnalyzer,
                                                   AbstractSyntaxNode **outNode);

#endif
