/*
 * Project: IFJ26 compiler
 * Authors: TODO(team)
 * AI-assisted scaffold.
 * Prompt: Create an optional optimizer integration stub.
 */

#include "optimizer.h"

CompilerStatus optimizerRun(IntermediateProgram *program)
{
    if (program == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    /* Optional team TODO: add verified semantics-preserving passes. */
    return COMPILER_OK;
}

