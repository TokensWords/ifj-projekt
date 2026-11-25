/**
 * @file  parserFunctions.h
 *
 * @project IFJ26 compiler
 * @author Matúš Čík xcikmat00
 */

#ifndef PARSER_FUNCTIONS_H
#define PARSER_FUNCTIONS_H

#include "syntaxAnalyzer.h"

/**
 * @brief Parses a function definition, including its signature and body.
 * @param syntaxAnalyzer The syntax analyzer to use for parsing.
 * @param outNode Receives an owned AST node on success.
 * @return COMPILER_OK on success, or the corresponding compiler error code.
 * @note An optional `@main` annotation marks the node as an entry function.
 */
CompilerStatus parseFunction(SyntaxAnalyzer *syntaxAnalyzer, AbstractSyntaxNode **outNode);

#endif
