#ifndef PARSER_EXPRESSION_H
#define PARSER_EXPRESSION_H

#include "syntaxAnalyzer.h"

CompilerStatus parseExpression(SyntaxAnalyzer *syntaxAnalyzer,
                                        AbstractSyntaxNode **outNode);


#endif