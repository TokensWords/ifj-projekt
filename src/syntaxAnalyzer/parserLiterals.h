/**
 * @file  parserLiterals.h
 *
 * @project IFJ26 compiler
 * @author Matúš Čík
 */

#ifndef PARSER_LITERALS_H
#define PARSER_LITERALS_H

#include "syntaxAnalyzerUtils.h"

/** @brief Parses a data type and stores it in outType. */
CompilerStatus analyzerTypesParseDataType(SyntaxAnalyzer *syntaxAnalyzer, DataType *outType);

#endif
