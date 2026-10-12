/**
 * @file  parserExpressions.h
 *
 * @project IFJ26 compiler
 * @author Matúš Čík
 */

#ifndef PARSER_EXPRESSIONS_H
#define PARSER_EXPRESSIONS_H

#include "syntaxAnalyzerUtils.h"

CompilerStatus parseIdentifierNode(SyntaxAnalyzer *syntaxAnalyzer,
                                   AbstractSyntaxNode **outNode);

CompilerStatus parseTerm(SyntaxAnalyzer *syntaxAnalyzer,
                         AbstractSyntaxNode **outNode);

CompilerStatus parseFunctionCall(SyntaxAnalyzer *syntaxAnalyzer,
                                 AbstractSyntaxNode **outNode);

CompilerStatus analyzerExpressionsParse(SyntaxAnalyzer *syntaxAnalyzer,
                                        AbstractSyntaxNode **outNode);

CompilerStatus analyzerExpressionsParseValueOrCall(SyntaxAnalyzer *syntaxAnalyzer,
                                                   AbstractSyntaxNode **outNode);

#endif
