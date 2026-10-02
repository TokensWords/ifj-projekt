/*
 * Project: IFJ26 compiler
 * Authors: TODO(team)
 * AI-assisted scaffold.
 * Prompt: Define an opaque lexical-analyzer interface without generating its automaton.
 */

#ifndef LEXICAL_ANALYZER_H
#define LEXICAL_ANALYZER_H

#include "compilerTypes.h"
#include "token.h"

#include <stdio.h>

typedef struct LexicalAnalyzer LexicalAnalyzer;

LexicalAnalyzer *lexicalAnalyzerCreate(FILE *input);
void lexicalAnalyzerDestroy(LexicalAnalyzer *lexicalAnalyzer);
CompilerStatus lexicalAnalyzerNextToken(LexicalAnalyzer *lexicalAnalyzer, Token **outToken);

#endif
