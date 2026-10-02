/*
 * Project: IFJ26 compiler
 * Authors: TODO(team)
 * AI-assisted scaffold.
 * Prompt: Define syntax-analyzer boundaries without generating grammar tables.
 */

#ifndef SYNTAX_ANALYZER_H
#define SYNTAX_ANALYZER_H

#include "abstractSyntaxTree.h"
#include "compilerTypes.h"
#include "lexicalAnalyzer.h"

typedef struct SyntaxAnalyzer SyntaxAnalyzer;

SyntaxAnalyzer *syntaxAnalyzerCreate(LexicalAnalyzer *lexicalAnalyzer);
void syntaxAnalyzerDestroy(SyntaxAnalyzer *syntaxAnalyzer);
CompilerStatus syntaxAnalyzerParseProgram(SyntaxAnalyzer *syntaxAnalyzer, SourceProgram **outProgram);

#endif
