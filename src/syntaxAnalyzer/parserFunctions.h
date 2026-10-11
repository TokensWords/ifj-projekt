#ifndef PARSER_FUNCTIONS_H
#define PARSER_FUNCTIONS_H

#include "syntaxAnalyzer.h"

CompilerStatus parseFunction(SyntaxAnalyzer *syntaxAnalyzer,
                             AbstractSyntaxNode **outNode);

#endif
