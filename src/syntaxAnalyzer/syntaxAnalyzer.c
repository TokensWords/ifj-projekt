/*
 * Project: IFJ26 compiler
 * Authors: TODO(team)
 * AI-assisted scaffold.
 * Prompt: Create syntax-analyzer lifecycle scaffolding without grammar logic.
 */

#include "syntaxAnalyzer.h"

#include "memory.h"

#include <stdlib.h>

struct SyntaxAnalyzer {
    LexicalAnalyzer *lexicalAnalyzer;
    Token *currentToken;
    Token *nextToken;
};

SyntaxAnalyzer *syntaxAnalyzerCreate(LexicalAnalyzer *lexicalAnalyzer)
{
    if (lexicalAnalyzer == NULL) {
        return NULL;
    }

    SyntaxAnalyzer *syntaxAnalyzer = memoryAllocate(sizeof(SyntaxAnalyzer));
    if (syntaxAnalyzer == NULL) {
        return NULL;
    }

    syntaxAnalyzer->lexicalAnalyzer = lexicalAnalyzer;
    syntaxAnalyzer->currentToken = NULL;
    syntaxAnalyzer->nextToken = NULL;
    return syntaxAnalyzer;
}

void syntaxAnalyzerDestroy(SyntaxAnalyzer *syntaxAnalyzer)
{
    if (syntaxAnalyzer == NULL) {
        return;
    }

    tokenDestroy(syntaxAnalyzer->currentToken);
    tokenDestroy(syntaxAnalyzer->nextToken);
    free(syntaxAnalyzer);
}

CompilerStatus syntaxAnalyzerParseProgram(SyntaxAnalyzer *syntaxAnalyzer, SourceProgram **outProgram)
{
    if (syntaxAnalyzer == NULL || outProgram == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    *outProgram = NULL;

    /* Team TODO: implement recursive descent and precedence parsing. */
    return COMPILER_INTERNAL_ERROR;
}
