/**
 * @file  parserLiterals.h
 *
 * @project IFJ26 compiler
 * @author Matúš Čík
 */

#ifndef PARSER_LITERALS_H
#define PARSER_LITERALS_H

#include "syntaxAnalyzerUtils.h"

/** @brief Parses a scalar type or recursive List[T] type into outType. */
CompilerStatus analyzerTypesParseType(SyntaxAnalyzer *syntaxAnalyzer, TypeDescriptor **outType);

#endif
