/**
 * @file  syntaxAnalyzer.h
 *
 * @brief Defines the syntax analyzer (parser) interface and lifecycle.
 *
 * @project IFJ26 compiler
 * @author Matúš Čík xcikmat00
 */

#ifndef SYNTAX_ANALYZER_H
#define SYNTAX_ANALYZER_H

#include "abstractSyntaxTree.h"
#include "compilerTypes.h"
#include "lexicalAnalyzer.h"

#include "syntaxAnalyzerUtils.h"

/**
 * @brief Creates a syntax analyzer with an empty token buffer.
 * @param lexicalAnalyzer The lexical analyzer that provides tokens. It must stay valid
 *     until the parser is destroyed.
 * @return An owned syntax analyzer, or NULL if the lexer is NULL or allocation fails.
 */
SyntaxAnalyzer *syntaxAnalyzerCreate(LexicalAnalyzer *lexicalAnalyzer);

/**
 * @brief Releases the analyzer and its buffered tokens.
 * @param syntaxAnalyzer The syntax analyzer to destroy. Can be NULL.
 * @note The caller still owns the lexical analyzer.
 */
void syntaxAnalyzerDestroy(SyntaxAnalyzer *syntaxAnalyzer);

/**
 * @brief Parses a complete source program into an abstract syntax tree.
 * @param syntaxAnalyzer The syntax analyzer at the start of the source program.
 * @param outProgram Receives an owned source program on success. With valid arguments,
 *     the parser sets it to NULL before parsing.
 * @return COMPILER_OK on success, or the corresponding compiler error code.
 * @note The program must declare exactly one `@main` function.
 */
CompilerStatus syntaxAnalyzerParseProgram(SyntaxAnalyzer *syntaxAnalyzer,
                                          SourceProgram **outProgram);

#endif
