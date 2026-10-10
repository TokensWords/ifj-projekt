#ifndef PARSER_EXPRESSIONS_H
#define PARSER_EXPRESSIONS_H

#include "syntaxAnalyzerUtils.h"

CompilerStatus analyzerExpressionsParse(SyntaxAnalyzer *syntaxAnalyzer,
                                        AbstractSyntaxNode **outNode);

CompilerStatus analyzerExpressionsParseValueOrCall(SyntaxAnalyzer *syntaxAnalyzer,
                                                   AbstractSyntaxNode **outNode);

#endif
