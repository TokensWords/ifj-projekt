/*
 * Project: IFJ26 compiler
 * Authors: TODO(team)
 * AI-assisted scaffold.
 * Prompt: Define the optional intermediate-code optimizer boundary.
 */

#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "compilerTypes.h"
#include "intermediateCode.h"

CompilerStatus optimizerRun(IntermediateProgram *program);

#endif
