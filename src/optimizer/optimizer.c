/*
 * Project: IFJ26 compiler
 * Authors: Václav Krytinář xkrytiv00
 * AI-assisted scaffold.
 * Prompt: Create an optional optimizer integration stub.
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>

#include "optimizer.h"

#include "compilerTypes.h"
#include "intermediateCode.h"

#include "optimizerSymTable.h"
#include "knownConstantsTable.h"

struct BbtElem {
    size_t line;
    IntermediateInstruction instruction;
    bool live[3];
    size_t nextUse[3];
};

CompilerStatus optimizerRun(IntermediateProgram *program)
{
    if (program == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }


    size_t start = 0;
    size_t end = 0;

    while (optimizerGetNextBlock(&start, &end, program)) {
        CompilerStatus status = COMPILER_OK;
        status = optimizerOptimizeBlock(start, end, program);
        if (status != COMPILER_OK) {
            return status;
        }
    }

    if (end != program->count - 1) {
        return COMPILER_INTERNAL_ERROR;
    }

    return COMPILER_OK;
}


bool optimizerGetNextBlock(size_t *start, size_t *end, IntermediateProgram *program)
{
    if (start == NULL || end == NULL || program == NULL) {
        return false;
    }

    if (program->count - 1 == *end) {
        return false;
    }

    *start = ++(*end);

    while (program->instructions[*end].opcode != INTERMEDIATE_LABEL
        && program->instructions[*end].opcode != INTERMEDIATE_JUMP
        && program->instructions[*end].opcode != INTERMEDIATE_JUMP_IF_EQUAL
        && program->instructions[*end].opcode != INTERMEDIATE_JUMP_IF_NOT_EQUAL
        && program->instructions[*end].opcode != INTERMEDIATE_RETURN
        && program->instructions[*end].opcode != INTERMEDIATE_EXIT
        && program->count - 1 != *end)
    {
        ++(*end);
    }

    if (program->instructions[*end].opcode == INTERMEDIATE_LABEL) {
        --(*end);
    }

    return true;
}


CompilerStatus optimizerOptimizeBlock(size_t start, size_t end, IntermediateProgram *program)
{
    size_t blockSize = end - start + 1;
    BbtElem basicBlockTable[blockSize];

    CompilerStatus status = COMPILER_OK;

    status = optimizerBackwardPass(basicBlockTable, start, end, program);
    if (status != COMPILER_OK) {
        return status;
    }

    status = optimizerForwardPass(basicBlockTable, start, end, program);
    if (status != COMPILER_OK) {
        return status;
    }

    return COMPILER_OK;
}


CompilerStatus optimizerBackwardPass(BbtElem bbtTable[], size_t start, size_t end, IntermediateProgram *program)
{

}


CompilerStatus optimizerForwardPass(BbtElem bbtTable[], size_t start, size_t end, IntermediateProgram *program)
{

}

