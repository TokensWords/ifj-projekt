/*
 * Project: IFJ26 compiler
 * Authors: TODO(team)
 * AI-assisted scaffold.
 * Prompt: Create lexical-analyzer lifecycle scaffolding without automaton logic.
 */

#include "lexicalAnalyzer.h"

#include "memory.h"

#include <stdbool.h>
#include <stdlib.h>

struct LexicalAnalyzer {
    FILE *input;
    size_t line;
    size_t column;
    size_t currentIndent;
    bool reachedEof;
};

LexicalAnalyzer *lexicalAnalyzerCreate(FILE *input)
{
    if (input == NULL) {
        return NULL;
    }

    LexicalAnalyzer *lexicalAnalyzer = memoryAllocate(sizeof(LexicalAnalyzer));
    if (lexicalAnalyzer == NULL) {
        return NULL;
    }

    lexicalAnalyzer->input = input;
    lexicalAnalyzer->line = 1;
    lexicalAnalyzer->column = 1;
    lexicalAnalyzer->currentIndent = 0;
    lexicalAnalyzer->reachedEof = false;
    return lexicalAnalyzer;
}

void lexicalAnalyzerDestroy(LexicalAnalyzer *lexicalAnalyzer)
{
    free(lexicalAnalyzer);
}

CompilerStatus lexicalAnalyzerNextToken(LexicalAnalyzer *lexicalAnalyzer, Token **outToken)
{
    if (lexicalAnalyzer == NULL || outToken == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    *outToken = NULL;

    /* Team TODO: implement the required finite-state scanner manually. */
    return COMPILER_INTERNAL_ERROR;
}
