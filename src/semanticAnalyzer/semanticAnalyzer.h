/*
 * Project: IFJ26 compiler
 * Authors: TODO(team)
 * AI-assisted scaffold.
 * Prompt: Define semantic-analysis inputs, outputs, and status reporting.
 */

#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "abstractSyntaxTree.h"
#include "compilerTypes.h"
#include "symbolTable.h"

CompilerStatus semanticAnalyzerAnalyze(SourceProgram *program, SymbolTable *symbols);

#endif

