/*
 * Project: IFJ26 compiler
 * Authors: TODO(team)
 * AI-assisted scaffold.
 * Prompt: Create a semantic-analysis integration stub.
 */

#include "semanticAnalyzer.h"

CompilerStatus semanticAnalyzerAnalyze(SourceProgram *program, SymbolTable *symbols)
{
    if (program == NULL || symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    /* Team TODO: resolve symbols and annotate inferred tree types. */
    return COMPILER_INTERNAL_ERROR;
}
