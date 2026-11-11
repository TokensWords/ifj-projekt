/**
 * @file  parserTypes.h
 *
 * @project IFJ26 compiler
 * @author Matúš Čík
 */

#ifndef PARSER_TYPES_H
#define PARSER_TYPES_H

#include "syntaxAnalyzerUtils.h"

/** @brief Parses a type expression and stores its owned descriptor in outType. */
CompilerStatus analyzerTypesParseType(SyntaxAnalyzer *syntaxAnalyzer, TypeDescriptor **outType);

/** @brief Parses a top-level type alias declaration. */
CompilerStatus analyzerTypesParseAlias(SyntaxAnalyzer *syntaxAnalyzer,
                                       AbstractSyntaxNode **outNode);

#endif
