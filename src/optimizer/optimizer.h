/*
 * Project: IFJ26 compiler
 * Authors: Václav Krytinář xkrytiv00
 * AI-assisted scaffold.
 * Prompt: Define the optional intermediate-code optimizer boundary.
 */

#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "compilerTypes.h"
#include "intermediateCode.h"

typedef struct BbtElem BbtElem;

CompilerStatus optimizerRun(IntermediateProgram *program);

bool optimizerGetNextBlock(size_t *start, size_t *end, IntermediateProgram *program);

CompilerStatus optimizerOptimizeBlock(size_t start, size_t end, IntermediateProgram *program);

CompilerStatus optimizerBackwardPass(BbtElem bbtTable[], size_t start, size_t end, IntermediateProgram *program);

CompilerStatus optimizerForwardPass(BbtElem bbtTable[], size_t start, size_t end, IntermediateProgram *program);

#endif // OPTIMIZER_H
