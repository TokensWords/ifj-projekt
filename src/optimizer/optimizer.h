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

#include "optimizerSymTable.h"

CompilerStatus optimizerRun(IntermediateProgram *program);

#endif
