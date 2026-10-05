/*
 * Project: IFJ26 compiler
 * Authors: xhajeka00
 * AI-assisted scaffold.
 * Prompt: Define IFJcode26 emission inputs and outputs.
 */

#ifndef GENERATOR_H
#define GENERATOR_H

#include "compilerTypes.h"
#include "intermediateCode.h"

#include <stdio.h>

CompilerStatus generatorWrite(const IntermediateProgram *program, FILE *output);

#endif

