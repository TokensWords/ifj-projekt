#ifndef PARSER_TYPES_H
#define PARSER_TYPES_H

#include "syntaxAnalyzerUtils.h"

/** @brief Parses a data type and stores it in outType. */
CompilerStatus analyzerTypesParseDataType(SyntaxAnalyzer *syntaxAnalyzer,
                                          DataType *outType);

#endif
