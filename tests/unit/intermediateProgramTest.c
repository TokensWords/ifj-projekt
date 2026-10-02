/*
 * Project: IFJ26 compiler
 * Author: AI testing agent.
 * Prompt: Test instruction appending and owned operand copies.
 */

#include "intermediateCode.h"

#include <assert.h>
#include <string.h>

int main(void)
{
    IntermediateProgram *program = intermediateProgramCreate();
    assert(program != NULL);
    assert(program->instructions == NULL);
    assert(program->count == 0);
    assert(program->capacity == 0);

    char destination[] = "global@result";
    char source[] = "int@26";
    IntermediateInstruction instruction = {0};
    instruction.opcode = INTERMEDIATE_MOVE;
    instruction.operandCount = 2;
    instruction.origin.line = 7;
    instruction.origin.column = 9;
    instruction.operands[0].kind = INTERMEDIATE_OPERAND_VARIABLE;
    instruction.operands[0].text = destination;
    instruction.operands[1].kind = INTERMEDIATE_OPERAND_INT;
    instruction.operands[1].text = source;
    instruction.operands[1].value.intValue = 26;

    assert(intermediateProgramAppend(program, &instruction) == COMPILER_OK);
    assert(program->count == 1);
    assert(program->capacity >= 1);
    assert(program->instructions[0].opcode == INTERMEDIATE_MOVE);
    assert(program->instructions[0].origin.line == 7);
    assert(program->instructions[0].origin.column == 9);
    assert(program->instructions[0].operands[0].text != destination);
    assert(program->instructions[0].operands[1].text != source);
    assert(strcmp(program->instructions[0].operands[0].text, "global@result") == 0);
    assert(strcmp(program->instructions[0].operands[1].text, "int@26") == 0);

    destination[0] = 'G';
    source[0] = 'I';
    assert(strcmp(program->instructions[0].operands[0].text, "global@result") == 0);
    assert(strcmp(program->instructions[0].operands[1].text, "int@26") == 0);

    for (size_t index = 1; index < 18; index++) {
        assert(intermediateProgramAppend(program, &instruction) == COMPILER_OK);
    }

    assert(program->count == 18);
    assert(program->capacity >= 18);
    assert(program->instructions[1].operands[0].text !=
           program->instructions[2].operands[0].text);

    size_t countBeforeInvalidAppend = program->count;
    instruction.operandCount = 4;
    assert(intermediateProgramAppend(program, &instruction) == COMPILER_INTERNAL_ERROR);
    assert(program->count == countBeforeInvalidAppend);
    assert(intermediateProgramAppend(NULL, &instruction) == COMPILER_INTERNAL_ERROR);
    assert(intermediateProgramAppend(program, NULL) == COMPILER_INTERNAL_ERROR);

    intermediateProgramDestroy(program);
    intermediateProgramDestroy(NULL);
    return 0;
}
