/*
 * Project: IFJ26 compiler
 * Authors: xkubanm00, xhajeka00
 * AI-assisted scaffold.
 * Prompt: Create one root entrypoint for the modular compiler pipeline.
 */

#include "abstractSyntaxTree.h"
#include "compilerTypes.h"
#include "generator.h"
#include "intermediateCode.h"
#include "lexicalAnalyzer.h"
#include "semanticAnalyzer.h"
#include "syntaxAnalyzer.h"

#include <stdio.h>

int main(void) {
    LexicalAnalyzer *lexicalAnalyzer = lexicalAnalyzerCreate(stdin);
    SyntaxAnalyzer *syntaxAnalyzer = NULL;
    SourceProgram *program = NULL;
    IntermediateProgram *intermediateProgram = NULL;
    CompilerStatus status = COMPILER_INTERNAL_ERROR;

    if (lexicalAnalyzer != NULL) {
        syntaxAnalyzer = syntaxAnalyzerCreate(lexicalAnalyzer);
    }
    if (syntaxAnalyzer != NULL) {
        status = syntaxAnalyzerParseProgram(syntaxAnalyzer, &program);
    }
    if (status == COMPILER_OK) {
        status = semanticAnalyzerAnalyze(program);
    }
    if (status == COMPILER_OK) {
        intermediateProgram = intermediateProgramCreate();
        status = intermediateProgram == NULL ? COMPILER_INTERNAL_ERROR :
                 intermediateCodeGenerate(program, intermediateProgram);
    }
    if (status == COMPILER_OK) {
        status = generatorWrite(intermediateProgram, stdout);
    }
    if (status != COMPILER_OK) {
        fprintf(stderr, "Compiler stopped with status %d.\n", (int)status);
    }

    intermediateProgramDestroy(intermediateProgram);
    sourceProgramDestroy(program);
    syntaxAnalyzerDestroy(syntaxAnalyzer);
    lexicalAnalyzerDestroy(lexicalAnalyzer);
    return status;
}
