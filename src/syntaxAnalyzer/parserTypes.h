/**
 * @file  parserTypes.h
 *
 * @project IFJ26 compiler
 * @author Matúš Čík xcikmat00
 */

#ifndef PARSER_TYPES_H
#define PARSER_TYPES_H

#include "syntaxAnalyzerUtils.h"

/**
 * @brief Parses a type expression.
 * @param syntaxAnalyzer The syntax analyzer to use for parsing.
 * @param outType An initialized type descriptor. The parser frees its old contents.
 *     Receives the parsed type on success. The parser resets it if parsing fails.
 * @return COMPILER_OK on success, or the corresponding compiler error code.
 */
CompilerStatus analyzerTypesParseType(SyntaxAnalyzer *syntaxAnalyzer, TypeDescriptor *outType);

/**
 * @brief Parses a top-level type alias declaration.
 * @param syntaxAnalyzer The syntax analyzer to use for parsing.
 * @param outNode Receives an owned AST node on success.
 * @return COMPILER_OK on success, or the corresponding compiler error code.
 */
CompilerStatus analyzerTypesParseAlias(SyntaxAnalyzer *syntaxAnalyzer,
                                       AbstractSyntaxNode **outNode);

/**
 * @brief Parses a top-level case class and its typed fields.
 * @param syntaxAnalyzer The syntax analyzer to use for parsing.
 * @param outNode Receives an owned AST node on success.
 * @return COMPILER_OK on success, or the corresponding compiler error code.
 */
CompilerStatus analyzerTypesParseCaseClass(SyntaxAnalyzer *syntaxAnalyzer,
                                           AbstractSyntaxNode **outNode);

#endif
