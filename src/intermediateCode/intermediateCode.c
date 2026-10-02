/*
 * Project: IFJ26 compiler
 * Authors: TODO(team)
 * AI-assisted scaffold.
 * Prompt: Implement intermediate-code helpers and a generator boundary.
 */

#include "intermediateCode.h"

#include "memory.h"

#include <stdlib.h>

static void intermediateOperandClear(IntermediateOperand *operand)
{
    free(operand->text);
    operand->text = NULL;
    operand->kind = INTERMEDIATE_OPERAND_NONE;
}

static void intermediateInstructionClear(IntermediateInstruction *instruction)
{
    for (size_t index = 0; index < instruction->operandCount; index++) {
        intermediateOperandClear(&instruction->operands[index]);
    }
}

IntermediateProgram *intermediateProgramCreate(void)
{
    IntermediateProgram *program = memoryAllocate(sizeof(IntermediateProgram));
    if (program == NULL) {
        return NULL;
    }

    program->instructions = NULL;
    program->count = 0;
    program->capacity = 0;
    return program;
}

void intermediateProgramDestroy(IntermediateProgram *program)
{
    if (program == NULL) {
        return;
    }

    for (size_t index = 0; index < program->count; index++) {
        intermediateInstructionClear(&program->instructions[index]);
    }

    free(program->instructions);
    free(program);
}

CompilerStatus intermediateProgramAppend(IntermediateProgram *program, const IntermediateInstruction *instruction)
{
    if (program == NULL || instruction == NULL || instruction->operandCount > 3) {
        return COMPILER_INTERNAL_ERROR;
    }

    if (program->count == program->capacity) {
        size_t newCapacity = program->capacity == 0 ? 16 : program->capacity * 2;
        IntermediateInstruction *newInstructions = memoryResizeArray(
            program->instructions, newCapacity, sizeof(IntermediateInstruction));
        if (newInstructions == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        program->instructions = newInstructions;
        program->capacity = newCapacity;
    }

    IntermediateInstruction copy = *instruction;
    for (size_t index = 0; index < copy.operandCount; index++) {
        copy.operands[index].text = NULL;
        if (instruction->operands[index].text != NULL) {
            copy.operands[index].text = memoryDuplicateString(instruction->operands[index].text);
            if (copy.operands[index].text == NULL) {
                intermediateInstructionClear(&copy);
                return COMPILER_INTERNAL_ERROR;
            }
        }
    }

    program->instructions[program->count] = copy;
    program->count++;
    return COMPILER_OK;
}

CompilerStatus intermediateCodeGenerate(const SourceProgram *program, IntermediateProgram *outProgram)
{
    if (program == NULL || outProgram == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    /* Team TODO: lower the typed tree into three-address instructions. */
    return COMPILER_INTERNAL_ERROR;
}
