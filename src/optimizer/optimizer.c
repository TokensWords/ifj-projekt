/*
 * Project: IFJ26 compiler
 * Authors: Václav Krytinář xkrytiv00
 * AI-assisted scaffold.
 * Prompt: Create an optional optimizer integration stub.
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "intermediateCode.h"

#include "compilerTypes.h"
#include "memory.h"

#include "optimizer.h"
#include "optimizerSymTable.h"
#include "knownConstantsTable.h"

struct BbtElem {
    size_t line;
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


enum OpcodeFlags {
    EVAL0 = 1,
    EVAL1 = 1 << 1,
    EVAL2 = 1 << 2,
    LIVE0 = 1 << 3,
    LIVE1 = 1 << 4,
    LIVE2 = 1 << 5
};

uint8_t optimizerGetOpcodeFlags(IntermediateOpcode opcode)
{
    switch (opcode) {
        case INTERMEDIATE_ADD:
        case INTERMEDIATE_SUBTRACT:
        case INTERMEDIATE_MULTIPLY:
        case INTERMEDIATE_DIVIDE:
        case INTERMEDIATE_INTEGER_DIVIDE:
        case INTERMEDIATE_COMPARE_LESS:
        case INTERMEDIATE_COMPARE_GREATER:
        case INTERMEDIATE_COMPARE_EQUAL:
        case INTERMEDIATE_AND:
        case INTERMEDIATE_OR:
        case INTERMEDIATE_CONCATENATE:
        case INTERMEDIATE_STRING_GET_CHARACTER:
        case INTERMEDIATE_STRING_CHARACTER_CODE:
            return EVAL0 | EVAL1 | EVAL2 | LIVE1 | LIVE2; // 0 written; 1,2 read

        case INTERMEDIATE_MOVE:
        case INTERMEDIATE_NOT:
        case INTERMEDIATE_CONVERT_INT_TO_DOUBLE:
        case INTERMEDIATE_CONVERT_DOUBLE_TO_INT:
        case INTERMEDIATE_CONVERT_INT_TO_STRING:
        case INTERMEDIATE_CONVERT_DOUBLE_TO_STRING:
        case INTERMEDIATE_CONVERT_INT_TO_CHARACTER:
        case INTERMEDIATE_STRING_LENGTH:
            return EVAL0 | EVAL1 | LIVE1; // 0 written; 1 read

        case INTERMEDIATE_POP:
        case INTERMEDIATE_READ:
        case INTERMEDIATE_DEFINE_VARIABLE:
            return EVAL0; // 0 written

        case INTERMEDIATE_WRITE:
        case INTERMEDIATE_PUSH:
        case INTERMEDIATE_EXIT:
            return EVAL0 | LIVE0; // 0 read

        case INTERMEDIATE_JUMP_IF_EQUAL:
        case INTERMEDIATE_JUMP_IF_NOT_EQUAL:
            return EVAL1 | EVAL2 | LIVE1 | LIVE2; // 1,2 read

        default:
            return 0;
    }
}

void optimizerBbtTableSetDefault(BbtElem table[], size_t index, size_t line)
{
    table[index].line = line;

    for (size_t i = 0; i < 3; ++i)
    {
        table[index].live[i] = false;
        table[index].nextUse[i] = 0;
    }
}

CompilerStatus optimizerBackwardPass(BbtElem bbtTable[], size_t start, size_t end, IntermediateProgram *program)
{
    OptimizerSymTable *symbols = optimizerSymTableCreate();
    uint8_t flags = 0;

    while (end >= start) {
        OptimizerSymbol *symbol;

        flags = optimizerGetOpcodeFlags(program->instructions[end].opcode);

        optimizerBbtTableSetDefault(bbtTable, end - start, end);

        for (size_t i = 0; i < 3; ++i) {
            if (flags & 1 << i && program->instructions[end].operands[i].kind == INTERMEDIATE_OPERAND_VARIABLE) {
                if ((symbol = optimizerSymTableFind(symbols, program->instructions[end].operands[i].text)) != NULL) {
                    bbtTable[end-start].live[i] = symbol->live;
                    bbtTable[end-start].nextUse[i] = symbol->nextUse;
                } else if (strncmp("$variable", program->instructions[end].operands[i].text, 9) == 0) {
                    bbtTable[end-start].live[i] = true;
                }

                if (symbol == NULL) {
                    symbol = memoryAllocate(sizeof(OptimizerSymbol));
                    if (symbol == NULL) {
                        return COMPILER_INTERNAL_ERROR;
                    }
                }

                symbol->name = program->instructions[end].operands[i].text;
                symbol->live = flags & 1 << i+3;
                symbol->nextUse = end;

                CompilerStatus status = optimizerSymTableInsert(symbols, symbol);
                if (status != COMPILER_OK) {
                    return status;
                }
            }
        }
    }

    optimizerSymTableDestroy(symbols);

    return COMPILER_OK;
}


CompilerStatus optimizerForwardPass(BbtElem bbtTable[], size_t start, size_t end, IntermediateProgram *program)
{

}

