#ifndef ANALYZER_FUNCTION_H
#define ANALYZER_FUNCTION_H

#include "syntaxAnalyzer.h"

CompilerStatus parseFunction(SyntaxAnalyzer *syntaxAnalyzer,
                                              AbstractSyntaxNode **outNode);

#endif
