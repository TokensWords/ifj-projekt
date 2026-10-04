/*
 * Project: IFJ26 compiler
 * Authors: TODO(team)
 * AI-assisted scaffold.
 * Prompt: Implement intermediate-code helpers and a generator boundary.
 */

#include "intermediateCode.h"

#include "memory.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    IntermediateProgram *program;
    size_t temporaryCounter;
    size_t labelCounter;
} IntermediateGenerator;

static void intermediateOperandClear(IntermediateOperand *operand) {
    free(operand->text);
    operand->text = NULL;
    operand->kind = INTERMEDIATE_OPERAND_NONE;
}

static void intermediateInstructionClear(IntermediateInstruction *instruction) {
    for (size_t index = 0; index < instruction->operandCount; index++) {
        intermediateOperandClear(&instruction->operands[index]);
    }
}

IntermediateProgram *intermediateProgramCreate(void) {
    IntermediateProgram *program = memoryAllocate(sizeof(IntermediateProgram));
    if (program == NULL) {
        return NULL;
    }

    program->instructions = NULL;
    program->count = 0;
    program->capacity = 0;
    return program;
}

void intermediateProgramDestroy(IntermediateProgram *program) {
    if (program == NULL) {
        return;
    }

    for (size_t index = 0; index < program->count; index++) {
        intermediateInstructionClear(&program->instructions[index]);
    }

    free(program->instructions);
    free(program);
}

CompilerStatus intermediateProgramAppend(IntermediateProgram *program, const IntermediateInstruction *instruction) {
    if (program == NULL || instruction == NULL || instruction->operandCount > 3) {
        return COMPILER_INTERNAL_ERROR;
    }

    if (program->count == program->capacity) {
        size_t newCapacity = program->capacity == 0 ? 16 : program->capacity * 2;
        IntermediateInstruction *newInstructions =
            memoryResizeArray(program->instructions, newCapacity, sizeof(IntermediateInstruction));
        if (newInstructions == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        program->instructions = newInstructions;
        program->capacity = newCapacity;
    }

    IntermediateInstruction copy = *instruction;
    for (size_t index = 0; index < copy.operandCount; index++) {
        copy.operands[index].text = NULL;
    }

    for (size_t index = 0; index < copy.operandCount; index++) {
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

int numberLength(size_t number) {
    size_t counter = 0;

    if (number == 0) {
        return 1;
    }

    while (number >= 1) {
        number /= 10;
        counter++;
    }

    return counter;
}

char *numberToString(size_t number) {
    size_t counter = 0;
    int length = numberLength(number);
    char *string = malloc((length + 1) * sizeof(char));
    if (string == NULL) {
        return NULL;
    }

    if (number == 0) {
        string[counter] = '0';
        counter++;
    }

    while (number >= 1) {
        string[length - counter - 1] = number % 10 + '0';
        number /= 10;
        counter++;
    }

    string[counter] = '\0';

    return string;
}

char *generateStringName(const char *type, size_t number) {
    if (type == NULL) {
        return NULL;
    }

    char *name = malloc((strlen(type) + numberLength(number) + 1) * sizeof(char));
    if (name == NULL) {
        return NULL;
    }

    strcpy(name, type);

    char *numberString = numberToString(number);
    if (numberString == NULL) {
        free(name);
        return NULL;
    }

    strncat(name, numberString, numberLength(number));
    free(numberString);

    return name;
}

static CompilerStatus generateTemporary(IntermediateGenerator *generator, IntermediateOperand *operand) {
    if (generator == NULL || operand == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    char *name = generateStringName("temporary", generator->temporaryCounter);
    if (name == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    operand->kind = INTERMEDIATE_OPERAND_VARIABLE;
    operand->text = name;
    operand->value.intValue = 0;

    generator->temporaryCounter++;

    return COMPILER_OK;
}

static CompilerStatus generateLabel(IntermediateGenerator *generator, IntermediateOperand *operand) {
    if (generator == NULL || operand == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    char *name = generateStringName("label", generator->labelCounter);
    if (name == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    operand->kind = INTERMEDIATE_OPERAND_LABEL;
    operand->text = name;
    operand->value.intValue = 0;

    generator->labelCounter++;

    return COMPILER_OK;
}

static CompilerStatus generateExpression(IntermediateGenerator *generator, IntermediateOperand *operand, const AbstractSyntaxNode *node) {
    if (generator == NULL || operand == NULL || node == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    switch (node->kind) {
    case ABSTRACT_SYNTAX_INT_LITERAL:
        operand->kind = INTERMEDIATE_OPERAND_INT;
        operand->text = NULL;
        operand->value.intValue = node->value.intValue;

        break;
    case ABSTRACT_SYNTAX_DOUBLE_LITERAL:
        operand->kind = INTERMEDIATE_OPERAND_DOUBLE;
        operand->text = NULL;
        operand->value.doubleValue = node->value.doubleValue;

        break;
    case ABSTRACT_SYNTAX_STRING_LITERAL:
        operand->kind = INTERMEDIATE_OPERAND_STRING;
        if (node->text == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        operand->text = malloc(strlen(node->text) + 1);
        if (operand->text == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        strcpy(operand->text, node->text);
        operand->value.dataType = 0;

        break;
    case ABSTRACT_SYNTAX_BINARY_EXPRESSION: {
        if (node->childCount != 2) {
            return COMPILER_INTERNAL_ERROR;
        }

        if (node->children == NULL || node->children[0] == NULL || node->children[1] == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        if (node->text == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        IntermediateOpcode operationCode = 0;
        if (strcmp(node->text, "+") == 0) {
            operationCode = INTERMEDIATE_ADD;
        } else if (strcmp(node->text, "-") == 0) {
            operationCode = INTERMEDIATE_SUBTRACT;
        } else if (strcmp(node->text, "*") == 0) {
            operationCode = INTERMEDIATE_MULTIPLY;
        } else {
            return COMPILER_INTERNAL_ERROR;
        }

        IntermediateOperand leftOperand = {0};
        CompilerStatus status = generateExpression(generator, &leftOperand, node->children[0]);
        if (status != COMPILER_OK) {
            return status;
        }
        DataType leftType = node->children[0]->inferredType;

        IntermediateOperand rightOperand = {0};
        status = generateExpression(generator, &rightOperand, node->children[1]);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&leftOperand);
            return status;
        }
        DataType rightType = node->children[1]->inferredType;

        IntermediateOperand resultOperand = {0};
        status = generateTemporary(generator, &resultOperand);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&leftOperand);
            intermediateOperandClear(&rightOperand);
            return status;
        }

        IntermediateInstruction instruction = {0};
        instruction.opcode = operationCode;
        instruction.operandCount = 3;
        instruction.operands[0] = resultOperand;
        instruction.operands[1] = leftOperand;
        instruction.operands[2] = rightOperand;
        instruction.origin = node->location;

        status = intermediateProgramAppend(generator->program, &instruction);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&leftOperand);
            intermediateOperandClear(&rightOperand);
            intermediateOperandClear(&resultOperand);
            return status;
        } else {
            intermediateOperandClear(&leftOperand);
            intermediateOperandClear(&rightOperand);
        }

        *operand = resultOperand;

        break;
    }
    default:
        return COMPILER_INTERNAL_ERROR;
    }

    return COMPILER_OK;
}

CompilerStatus intermediateCodeGenerate(const SourceProgram *program, IntermediateProgram *outProgram) {
    if (program == NULL || outProgram == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    /* Team TODO: lower the typed tree into three-address instructions. */
    return COMPILER_INTERNAL_ERROR;
}
