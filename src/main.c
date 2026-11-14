/*
 * Project: IFJ26 compiler
 * Authors: TODO(team)
 * AI-assisted scaffold.
 * Prompt: Create one root entrypoint for the modular compiler pipeline.
 */

#include "abstractSyntaxTree.h"
#include "compilerTypes.h"
#include "lexicalAnalyzer.h"
#include "syntaxAnalyzer.h"

#include <stdio.h>

int main(void) {
    LexicalAnalyzer *lexicalAnalyzer = lexicalAnalyzerCreate(stdin);
    if (lexicalAnalyzer == NULL) {
        fprintf(stderr, "Internal compiler allocation failure.\n");
        return COMPILER_INTERNAL_ERROR;
    }

    SyntaxAnalyzer *syntaxAnalyzer = syntaxAnalyzerCreate(lexicalAnalyzer);
    if (syntaxAnalyzer == NULL) {
        lexicalAnalyzerDestroy(lexicalAnalyzer);
        fprintf(stderr, "Internal compiler allocation failure.\n");
        return COMPILER_INTERNAL_ERROR;
    }

    SourceProgram *program = NULL;
    CompilerStatus status = syntaxAnalyzerParseProgram(syntaxAnalyzer, &program);
    if (status != COMPILER_OK) {
        fprintf(stderr, "Compiler stopped with status %d.\n", (int)status);
    }

    sourceProgramDestroy(program);
    syntaxAnalyzerDestroy(syntaxAnalyzer);
    lexicalAnalyzerDestroy(lexicalAnalyzer);
    return status;
}
