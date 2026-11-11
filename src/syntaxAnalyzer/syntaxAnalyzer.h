/**
 * @file  syntaxAnalyzer.h
 *
 * @brief Defines the syntax analyzer (parser) interface and lifecycle.
 *
 * @project IFJ26 compiler
 * @author Matúš Čík
 */

#ifndef SYNTAX_ANALYZER_H
#define SYNTAX_ANALYZER_H

#include "abstractSyntaxTree.h"
#include "compilerTypes.h"
#include "lexicalAnalyzer.h"

#include "syntaxAnalyzerUtils.h"

/**
 * @brief Creates a new syntax analyzer instance.
 * @param lexicalAnalyzer lexical analyzer provides tokes. Borrowed.
 */
SyntaxAnalyzer *syntaxAnalyzerCreate(LexicalAnalyzer *lexicalAnalyzer);

/**
 * @brief Destroys a syntax analyzer instance and it's buffered tokens.
 * @param syntaxAnalyzer the syntax analyzer to destroy. Can be NULL.
 */
void syntaxAnalyzerDestroy(SyntaxAnalyzer *syntaxAnalyzer);

/**
 * @brief Parses a source program into an abstract syntax tree.
 * @param syntaxAnalyzer the syntax analyzer to use for parsing.
 * @param outProgram Receives an owned source program on success, set to NULL on failure.
 * @return COMPILER_OK on success else coresponding compiler error message.
 * @note Parser fails on invalid syntax programs.
 */
CompilerStatus syntaxAnalyzerParseProgram(SyntaxAnalyzer *syntaxAnalyzer,
                                          SourceProgram **outProgram);

#endif
