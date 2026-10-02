/*
 * Project: IFJ26 compiler
 * Author: AI testing agent.
 * Prompt: Test the optimizer's documented no-op behavior.
 */

#include "optimizer.h"

#include <assert.h>

int main(void)
{
    IntermediateProgram *program = intermediateProgramCreate();
    assert(program != NULL);

    IntermediateInstruction instruction = {0};
    instruction.opcode = INTERMEDIATE_LABEL;
    instruction.operandCount = 1;
    instruction.origin.line = 11;
    instruction.origin.column = 2;
    instruction.operands[0].kind = INTERMEDIATE_OPERAND_LABEL;
    instruction.operands[0].text = "main";
    assert(intermediateProgramAppend(program, &instruction) == COMPILER_OK);

    IntermediateInstruction *instructionsBefore = program->instructions;
    size_t countBefore = program->count;
    size_t capacityBefore = program->capacity;
    assert(optimizerRun(program) == COMPILER_OK);
    assert(program->instructions == instructionsBefore);
    assert(program->count == countBefore);
    assert(program->capacity == capacityBefore);
    assert(program->instructions[0].opcode == INTERMEDIATE_LABEL);
    assert(program->instructions[0].operandCount == 1);
    assert(program->instructions[0].origin.line == 11);
    assert(program->instructions[0].origin.column == 2);

    assert(optimizerRun(NULL) == COMPILER_INTERNAL_ERROR);
    intermediateProgramDestroy(program);
    return 0;
}
