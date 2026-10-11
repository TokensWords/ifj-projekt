/*
 * Project: IFJ26 compiler
 * Authors: xhajeka00
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
    IntermediateProgram *functionDeclarations;
    const SourceProgram *sourceProgram;
    DataType returnType;
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

static IntermediateInstruction fillInstruction(IntermediateOpcode opcode, size_t operandCount, SourceLocation origin) {
    IntermediateInstruction instruction = {0};

    instruction.opcode = opcode;
    instruction.operandCount = operandCount;
    instruction.origin = origin;

    return instruction;
}

static int numberLength(size_t number) {
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

static char *numberToString(size_t number) {
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

static char *generateStringName(const char *type, size_t number) {
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

static CompilerStatus appendVariableDeclaration(IntermediateGenerator *generator, const IntermediateOperand *operand, SourceLocation origin) {
    if (generator == NULL || operand == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    if (generator->functionDeclarations == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    if (operand->kind != INTERMEDIATE_OPERAND_VARIABLE || operand->text == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    IntermediateInstruction instruction = fillInstruction(INTERMEDIATE_DEFINE_VARIABLE, 1, origin);
    instruction.operands[0] = *operand;

    return intermediateProgramAppend(generator->functionDeclarations, &instruction);
}

static CompilerStatus generateTemporary(IntermediateGenerator *generator, IntermediateOperand *operand, SourceLocation origin) {
    if (generator == NULL || operand == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    char *name = generateStringName("$temporary", generator->temporaryCounter);
    if (name == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    operand->kind = INTERMEDIATE_OPERAND_VARIABLE;
    operand->text = name;
    operand->value.intValue = 0;

    CompilerStatus status = appendVariableDeclaration(generator, operand, origin);
    if (status != COMPILER_OK) {
        intermediateOperandClear(operand);
        return status;
    }

    generator->temporaryCounter++;

    return COMPILER_OK;
}

static CompilerStatus generateLabel(IntermediateGenerator *generator, IntermediateOperand *operand) {
    if (generator == NULL || operand == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    char *name = generateStringName("$label", generator->labelCounter);
    if (name == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    operand->kind = INTERMEDIATE_OPERAND_LABEL;
    operand->text = name;
    operand->value.intValue = 0;

    generator->labelCounter++;

    return COMPILER_OK;
}

static CompilerStatus generateVariable(IntermediateOperand *operand, const AbstractSyntaxNode *node) {
    if (operand == NULL || node == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    if (node->variableId == 0) {
        return COMPILER_INTERNAL_ERROR;
    }

    char *name = generateStringName("$variable", node->variableId);
    if (name == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    operand->kind = INTERMEDIATE_OPERAND_VARIABLE;
    operand->text = name;
    operand->value.dataType = 0;

    return COMPILER_OK;
}

static bool isNumericType(DataType dataType) {
    return dataType == DATA_TYPE_INT || dataType == DATA_TYPE_DOUBLE;
}

static CompilerStatus convertIntToDouble(IntermediateGenerator *generator, IntermediateOperand *operand, SourceLocation origin) {
    if (generator == NULL || operand == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    IntermediateOperand convertedOperand = {0};
    CompilerStatus status = generateTemporary(generator, &convertedOperand, origin);
    if (status != COMPILER_OK) {
        return status;
    }

    IntermediateInstruction instruction = fillInstruction(INTERMEDIATE_CONVERT_INT_TO_DOUBLE, 2, origin);
    instruction.operands[0] = convertedOperand;
    instruction.operands[1] = *operand;

    status = intermediateProgramAppend(generator->program, &instruction);
    if (status != COMPILER_OK) {
        intermediateOperandClear(&convertedOperand);
        return status;
    }

    intermediateOperandClear(operand);
    *operand = convertedOperand;

    return COMPILER_OK;
}

static CompilerStatus generateIntegerDivision(IntermediateGenerator *generator, const IntermediateOperand *resultOperand, const IntermediateOperand *leftOperand, const IntermediateOperand *rightOperand, SourceLocation origin) {
    if (generator == NULL || resultOperand == NULL || leftOperand == NULL || rightOperand == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    IntermediateInstruction instruction = fillInstruction(INTERMEDIATE_INTEGER_DIVIDE, 3, origin);
    instruction.operands[0] = *resultOperand;
    instruction.operands[1] = *leftOperand;
    instruction.operands[2] = *rightOperand;

    CompilerStatus status = intermediateProgramAppend(generator->program, &instruction);
    if (status != COMPILER_OK) {
        return status;
    }

    IntermediateOperand negativeOperand = {0};
    IntermediateOperand zeroOperand = {0};
    zeroOperand.kind = INTERMEDIATE_OPERAND_INT;
    zeroOperand.value.intValue = 0;
    status = generateTemporary(generator, &negativeOperand, origin);
    if (status != COMPILER_OK) {
        return status;
    }

    IntermediateInstruction compareInstruction = fillInstruction(INTERMEDIATE_COMPARE_LESS, 3, origin);
    compareInstruction.operands[0] = negativeOperand;
    compareInstruction.operands[1] = *resultOperand;
    compareInstruction.operands[2] = zeroOperand;

    status = intermediateProgramAppend(generator->program, &compareInstruction);
    if (status != COMPILER_OK) {
        intermediateOperandClear(&negativeOperand);
        return status;
    }

    IntermediateOperand endLabelOperand = {0};
    status = generateLabel(generator, &endLabelOperand);
    if (status != COMPILER_OK) {
        intermediateOperandClear(&negativeOperand);
        return status;
    }

    IntermediateOperand falseOperand = {0};
    falseOperand.kind = INTERMEDIATE_OPERAND_BOOLEAN;
    falseOperand.value.boolValue = false;

    IntermediateInstruction jumpInstruction = fillInstruction(INTERMEDIATE_JUMP_IF_EQUAL, 3, origin);
    jumpInstruction.operands[0] = endLabelOperand;
    jumpInstruction.operands[1] = negativeOperand;
    jumpInstruction.operands[2] = falseOperand;

    status = intermediateProgramAppend(generator->program, &jumpInstruction);
    if (status != COMPILER_OK) {
        intermediateOperandClear(&endLabelOperand);
        intermediateOperandClear(&negativeOperand);
        return status;
    }

    IntermediateOperand divisionCheckOperand = {0};
    status = generateTemporary(generator, &divisionCheckOperand, origin);
    if (status != COMPILER_OK) {
        intermediateOperandClear(&endLabelOperand);
        intermediateOperandClear(&negativeOperand);
        return status;
    }

    IntermediateOperand oneOperand = {0};
    oneOperand.kind = INTERMEDIATE_OPERAND_INT;
    oneOperand.value.intValue = 1;

    IntermediateInstruction addInstruction = fillInstruction(INTERMEDIATE_ADD, 3, origin);
    addInstruction.operands[0] = divisionCheckOperand;
    addInstruction.operands[1] = *resultOperand;
    addInstruction.operands[2] = oneOperand;

    status = intermediateProgramAppend(generator->program, &addInstruction);
    if (status != COMPILER_OK) {
        intermediateOperandClear(&divisionCheckOperand);
        intermediateOperandClear(&endLabelOperand);
        intermediateOperandClear(&negativeOperand);
        return status;
    }

    IntermediateInstruction multiplyInstruction = fillInstruction(INTERMEDIATE_MULTIPLY, 3, origin);
    multiplyInstruction.operands[0] = divisionCheckOperand;
    multiplyInstruction.operands[1] = divisionCheckOperand;
    multiplyInstruction.operands[2] = *rightOperand;

    status = intermediateProgramAppend(generator->program, &multiplyInstruction);
    if (status != COMPILER_OK) {
        intermediateOperandClear(&divisionCheckOperand);
        intermediateOperandClear(&endLabelOperand);
        intermediateOperandClear(&negativeOperand);
        return status;
    }

    IntermediateInstruction subtractInstruction = fillInstruction(INTERMEDIATE_SUBTRACT, 3, origin);
    subtractInstruction.operands[0] = divisionCheckOperand;
    subtractInstruction.operands[1] = *leftOperand;
    subtractInstruction.operands[2] = divisionCheckOperand;

    status = intermediateProgramAppend(generator->program, &subtractInstruction);
    if (status != COMPILER_OK) {
        intermediateOperandClear(&divisionCheckOperand);
        intermediateOperandClear(&endLabelOperand);
        intermediateOperandClear(&negativeOperand);
        return status;
    }

    IntermediateInstruction checkAddInstruction = fillInstruction(INTERMEDIATE_ADD, 3, origin);
    checkAddInstruction.operands[0] = divisionCheckOperand;
    checkAddInstruction.operands[1] = divisionCheckOperand;
    checkAddInstruction.operands[2] = *rightOperand;

    status = intermediateProgramAppend(generator->program, &checkAddInstruction);
    if (status != COMPILER_OK) {
        intermediateOperandClear(&divisionCheckOperand);
        intermediateOperandClear(&endLabelOperand);
        intermediateOperandClear(&negativeOperand);
        return status;
    }

    IntermediateInstruction exactDivisionJumpInstruction = fillInstruction(INTERMEDIATE_JUMP_IF_EQUAL, 3, origin);
    exactDivisionJumpInstruction.operands[0] = endLabelOperand;
    exactDivisionJumpInstruction.operands[1] = divisionCheckOperand;
    exactDivisionJumpInstruction.operands[2] = zeroOperand;

    status = intermediateProgramAppend(generator->program, &exactDivisionJumpInstruction);
    if (status != COMPILER_OK) {
        intermediateOperandClear(&divisionCheckOperand);
        intermediateOperandClear(&endLabelOperand);
        intermediateOperandClear(&negativeOperand);
        return status;
    }

    IntermediateInstruction correctionInstruction = fillInstruction(INTERMEDIATE_ADD, 3, origin);
    correctionInstruction.operands[0] = *resultOperand;
    correctionInstruction.operands[1] = *resultOperand;
    correctionInstruction.operands[2] = oneOperand;

    status = intermediateProgramAppend(generator->program, &correctionInstruction);
    if (status != COMPILER_OK) {
        intermediateOperandClear(&divisionCheckOperand);
        intermediateOperandClear(&endLabelOperand);
        intermediateOperandClear(&negativeOperand);
        return status;
    }

    IntermediateInstruction endLabelInstruction = fillInstruction(INTERMEDIATE_LABEL, 1, origin);
    endLabelInstruction.operands[0] = endLabelOperand;
    status = intermediateProgramAppend(generator->program, &endLabelInstruction);
    if (status != COMPILER_OK) {
        intermediateOperandClear(&divisionCheckOperand);
        intermediateOperandClear(&endLabelOperand);
        intermediateOperandClear(&negativeOperand);
        return status;
    }

    intermediateOperandClear(&divisionCheckOperand);
    intermediateOperandClear(&endLabelOperand);
    intermediateOperandClear(&negativeOperand);

    return COMPILER_OK;
}

static char *createFunctionLabel(const char *functionName) {
    if (functionName == NULL) {
        return NULL;
    }

    size_t length = strlen(functionName);
    length += strlen("$function_");
    length++;

    char *labelName = malloc(length * sizeof(char));
    if (labelName == NULL) {
        return NULL;
    }

    strcpy(labelName, "$function_");
    strncat(labelName, functionName, (length - strlen("$function_")));

    return labelName;
}

static const AbstractSyntaxNode *findFunction(const SourceProgram *program, const char *functionName) {
    if (program == NULL || program->root == NULL || functionName == NULL) {
        return NULL;
    }

    if (program->root->kind != ABSTRACT_SYNTAX_PROGRAM) {
        return NULL;
    }

    if (program->root->childCount > 0) {
        if (program->root->children == NULL) {
            return NULL;
        }

        size_t counter = 0;

        while (counter < program->root->childCount) {
            const AbstractSyntaxNode *function = program->root->children[counter];

            if (function == NULL || function->kind != ABSTRACT_SYNTAX_FUNCTION || function->text == NULL) {
                return NULL;
            }

            if (strcmp(function->text, functionName) == 0) {
                return function;
            }

            counter++;
        }
    }

    return NULL;
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
        bool negateResult = false;
        bool includeEquality = false;

        if (strcmp(node->text, "+") == 0) {
            operationCode = INTERMEDIATE_ADD;
        } else if (strcmp(node->text, "-") == 0) {
            operationCode = INTERMEDIATE_SUBTRACT;
        } else if (strcmp(node->text, "*") == 0) {
            operationCode = INTERMEDIATE_MULTIPLY;
        } else if (strcmp(node->text, "/") == 0) {
            if (node->children[0]->inferredType == DATA_TYPE_INT && node->children[1]->inferredType == DATA_TYPE_INT) {
                operationCode = INTERMEDIATE_INTEGER_DIVIDE;
            } else {
                operationCode = INTERMEDIATE_DIVIDE;
            }
        } else if (strcmp(node->text, "==") == 0) {
            operationCode = INTERMEDIATE_COMPARE_EQUAL;
        } else if (strcmp(node->text, "!=") == 0) {
            operationCode = INTERMEDIATE_COMPARE_EQUAL;
            negateResult = true;
        } else if (strcmp(node->text, "<") == 0) {
            operationCode = INTERMEDIATE_COMPARE_LESS;
        } else if (strcmp(node->text, ">") == 0) {
            operationCode = INTERMEDIATE_COMPARE_GREATER;
        } else if (strcmp(node->text, "<=") == 0) {
            operationCode = INTERMEDIATE_COMPARE_LESS;
            includeEquality = true;
        } else if (strcmp(node->text, ">=") == 0) {
            operationCode = INTERMEDIATE_COMPARE_GREATER;
            includeEquality = true;
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

        bool isComparison = operationCode == INTERMEDIATE_COMPARE_EQUAL || operationCode == INTERMEDIATE_COMPARE_LESS || operationCode == INTERMEDIATE_COMPARE_GREATER;

        if (leftType == DATA_TYPE_UNIT || rightType == DATA_TYPE_UNIT) {
            if (leftType != DATA_TYPE_UNIT || rightType != DATA_TYPE_UNIT || operationCode != INTERMEDIATE_COMPARE_EQUAL) {
                intermediateOperandClear(&leftOperand);
                intermediateOperandClear(&rightOperand);
                return COMPILER_INTERNAL_ERROR;
            }
        } else if (leftType == DATA_TYPE_STRING && rightType == DATA_TYPE_STRING) {
            if (operationCode == INTERMEDIATE_ADD) {
                operationCode = INTERMEDIATE_CONCATENATE;
            } else if (!isComparison) {
                intermediateOperandClear(&leftOperand);
                intermediateOperandClear(&rightOperand);
                return COMPILER_INTERNAL_ERROR;
            }
        } else if (!isNumericType(leftType) || !isNumericType(rightType)) {
            intermediateOperandClear(&leftOperand);
            intermediateOperandClear(&rightOperand);
            return COMPILER_INTERNAL_ERROR;
        }

        if (leftType != rightType) {
            IntermediateOperand *integerOperand = leftType == DATA_TYPE_INT ? &leftOperand : &rightOperand;
            status = convertIntToDouble(generator, integerOperand, node->location);
            if (status != COMPILER_OK) {
                intermediateOperandClear(&leftOperand);
                intermediateOperandClear(&rightOperand);
                return status;
            }

            leftType = DATA_TYPE_DOUBLE;
            rightType = DATA_TYPE_DOUBLE;
        }

        IntermediateOperand resultOperand = {0};
        status = generateTemporary(generator, &resultOperand, node->location);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&leftOperand);
            intermediateOperandClear(&rightOperand);
            return status;
        }

        if (operationCode == INTERMEDIATE_INTEGER_DIVIDE) {
            status = generateIntegerDivision(generator, &resultOperand, &leftOperand, &rightOperand, node->location);
            if (status != COMPILER_OK) {
                intermediateOperandClear(&leftOperand);
                intermediateOperandClear(&rightOperand);
                intermediateOperandClear(&resultOperand);
                return status;
            }

            intermediateOperandClear(&leftOperand);
            intermediateOperandClear(&rightOperand);
            *operand = resultOperand;
        } else {
            IntermediateInstruction instruction = fillInstruction(operationCode, 3, node->location);
            instruction.operands[0] = resultOperand;
            instruction.operands[1] = leftOperand;
            instruction.operands[2] = rightOperand;

            status = intermediateProgramAppend(generator->program, &instruction);
            if (status != COMPILER_OK) {
                intermediateOperandClear(&leftOperand);
                intermediateOperandClear(&rightOperand);
                intermediateOperandClear(&resultOperand);
                return status;
            }

            if (negateResult) {
                IntermediateInstruction negateInstruction = fillInstruction(INTERMEDIATE_NOT, 2, node->location);
                negateInstruction.operands[0] = resultOperand;
                negateInstruction.operands[1] = resultOperand;

                status = intermediateProgramAppend(generator->program, &negateInstruction);
                if (status != COMPILER_OK) {
                    intermediateOperandClear(&leftOperand);
                    intermediateOperandClear(&rightOperand);
                    intermediateOperandClear(&resultOperand);
                    return status;
                }
            }

            if (includeEquality) {
                IntermediateOperand equalityOperand = {0};

                status = generateTemporary(generator, &equalityOperand, node->location);
                if (status != COMPILER_OK) {
                    intermediateOperandClear(&leftOperand);
                    intermediateOperandClear(&rightOperand);
                    intermediateOperandClear(&resultOperand);
                    return status;
                }

                IntermediateInstruction equalityInstruction = fillInstruction(INTERMEDIATE_COMPARE_EQUAL, 3, node->location);
                equalityInstruction.operands[0] = equalityOperand;
                equalityInstruction.operands[1] = leftOperand;
                equalityInstruction.operands[2] = rightOperand;

                status = intermediateProgramAppend(generator->program, &equalityInstruction);
                if (status != COMPILER_OK) {
                    intermediateOperandClear(&equalityOperand);
                    intermediateOperandClear(&leftOperand);
                    intermediateOperandClear(&rightOperand);
                    intermediateOperandClear(&resultOperand);
                    return status;
                }

                IntermediateInstruction orInstruction = fillInstruction(INTERMEDIATE_OR, 3, node->location);
                orInstruction.operands[0] = resultOperand;
                orInstruction.operands[1] = resultOperand;
                orInstruction.operands[2] = equalityOperand;

                status = intermediateProgramAppend(generator->program, &orInstruction);
                if (status != COMPILER_OK) {
                    intermediateOperandClear(&equalityOperand);
                    intermediateOperandClear(&leftOperand);
                    intermediateOperandClear(&rightOperand);
                    intermediateOperandClear(&resultOperand);
                    return status;
                }

                intermediateOperandClear(&equalityOperand);
            }

            intermediateOperandClear(&leftOperand);
            intermediateOperandClear(&rightOperand);
            *operand = resultOperand;
        }

        break;
    }
    case ABSTRACT_SYNTAX_IDENTIFIER: {
        CompilerStatus status = generateVariable(operand, node);
        if (status != COMPILER_OK) {
            return status;
        }

        break;
    }
    case ABSTRACT_SYNTAX_UNARY_EXPRESSION: {
        if (node->childCount != 1 || node->children == NULL || node->children[0] == NULL || node->text == NULL || strcmp(node->text, "-") != 0 || !isNumericType(node->children[0]->inferredType)) {
            return COMPILER_INTERNAL_ERROR;
        }

        IntermediateOperand valueOperand = {0};
        CompilerStatus status = generateExpression(generator, &valueOperand, node->children[0]);
        if (status != COMPILER_OK) {
            return status;
        }

        IntermediateOperand negativeOperand = {0};

        if (node->children[0]->inferredType == DATA_TYPE_INT) {
            negativeOperand.kind = INTERMEDIATE_OPERAND_INT;
            negativeOperand.value.intValue = -1;
        } else if (node->children[0]->inferredType == DATA_TYPE_DOUBLE) {
            negativeOperand.kind = INTERMEDIATE_OPERAND_DOUBLE;
            negativeOperand.value.doubleValue = -1.0;
        } else {
            intermediateOperandClear(&valueOperand);
            return COMPILER_INTERNAL_ERROR;
        }

        IntermediateOperand resultOperand = {0};
        status = generateTemporary(generator, &resultOperand, node->location);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&valueOperand);
            return status;
        }

        IntermediateInstruction instruction = fillInstruction(INTERMEDIATE_MULTIPLY, 3, node->location);
        instruction.operands[0] = resultOperand;
        instruction.operands[1] = valueOperand;
        instruction.operands[2] = negativeOperand;

        status = intermediateProgramAppend(generator->program, &instruction);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&resultOperand);
            intermediateOperandClear(&valueOperand);
            return status;
        }

        *operand = resultOperand;

        intermediateOperandClear(&valueOperand);

        break;
    }
    case ABSTRACT_SYNTAX_UNIT_LITERAL: {
        operand->kind = INTERMEDIATE_OPERAND_UNIT;
        operand->text = NULL;
        operand->value = (IntermediateOperandValue){0};

        break;
    }
    case ABSTRACT_SYNTAX_FUNCTION_CALL: {
        if (node->text == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        const AbstractSyntaxNode *function = findFunction(generator->sourceProgram, node->text);
        if (function == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        if (!(function->childCount > 0) || function->children == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        if (function->children[function->childCount - 1] == NULL || function->children[function->childCount - 1]->kind != ABSTRACT_SYNTAX_BLOCK) {
            return COMPILER_INTERNAL_ERROR;
        }

        size_t parameterCount = function->childCount - 1;

        if (node->childCount != parameterCount) {
            return COMPILER_INTERNAL_ERROR;
        }

        if (node->childCount != 0) {
            if (node->children == NULL) {
                return COMPILER_INTERNAL_ERROR;
            }
        }

        size_t counter = 0;

        while (counter < parameterCount) {
            const AbstractSyntaxNode *parameter = function->children[counter];
            const AbstractSyntaxNode *argument = node->children[counter];

            if (parameter == NULL || argument == NULL) {
                return COMPILER_INTERNAL_ERROR;
            }

            if (parameter->kind != ABSTRACT_SYNTAX_PARAMETER || parameter->variableId == 0) {
                return COMPILER_INTERNAL_ERROR;
            }

            if (parameter->variableType != DATA_TYPE_INT && parameter->variableType != DATA_TYPE_DOUBLE && parameter->variableType != DATA_TYPE_STRING && parameter->variableType != DATA_TYPE_UNIT) {
                return COMPILER_INTERNAL_ERROR;
            }

            if (argument->inferredType != parameter->variableType) {
                if (argument->inferredType != DATA_TYPE_INT || parameter->variableType != DATA_TYPE_DOUBLE) {
                    return COMPILER_INTERNAL_ERROR;
                }
            }

            counter++;
        }

        counter = 0;

        while (counter < parameterCount) {
            const AbstractSyntaxNode *parameter = function->children[counter];
            const AbstractSyntaxNode *argument = node->children[counter];

            IntermediateOperand argumentOperand = {0};
            CompilerStatus status = generateExpression(generator, &argumentOperand, argument);
            if (status != COMPILER_OK) {
                return status;
            }

            if (argument->inferredType == DATA_TYPE_INT && parameter->variableType == DATA_TYPE_DOUBLE) {
                status = convertIntToDouble(generator, &argumentOperand, argument->location);
                if (status != COMPILER_OK) {
                    intermediateOperandClear(&argumentOperand);
                    return status;
                }
            }

            IntermediateInstruction argumentInstruction = fillInstruction(INTERMEDIATE_PUSH, 1, argument->location);
            argumentInstruction.operands[0] = argumentOperand;

            status = intermediateProgramAppend(generator->program, &argumentInstruction);
            intermediateOperandClear(&argumentOperand);
            if (status != COMPILER_OK) {
                return status;
            }

            counter++;
        }

        IntermediateInstruction createFrameInstruction = fillInstruction(INTERMEDIATE_CREATE_FRAME, 0, node->location);
        CompilerStatus status = intermediateProgramAppend(generator->program, &createFrameInstruction);
        if (status != COMPILER_OK) {
            return status;
        }

        IntermediateOperand functionLabelOperand = {0};
        functionLabelOperand.kind = INTERMEDIATE_OPERAND_LABEL;
        functionLabelOperand.text = createFunctionLabel(function->text);
        if (functionLabelOperand.text == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        IntermediateInstruction callInstruction = fillInstruction(INTERMEDIATE_CALL, 1, node->location);
        callInstruction.operands[0] = functionLabelOperand;

        status = intermediateProgramAppend(generator->program, &callInstruction);
        intermediateOperandClear(&functionLabelOperand);
        if (status != COMPILER_OK) {
            return status;
        }

        IntermediateOperand resultOperand = {0};
        status = generateTemporary(generator, &resultOperand, node->location);
        if (status != COMPILER_OK) {
            return status;
        }

        IntermediateInstruction resultInstruction = fillInstruction(INTERMEDIATE_POP, 1, node->location);
        resultInstruction.operands[0] = resultOperand;
        status = intermediateProgramAppend(generator->program, &resultInstruction);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&resultOperand);
            return status;
        }

        *operand = resultOperand;

        break;
    }
    default:
        return COMPILER_INTERNAL_ERROR;
    }

    return COMPILER_OK;
}

static CompilerStatus generateStatement(IntermediateGenerator *generator, const AbstractSyntaxNode *node) {
    if (generator == NULL || node == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    switch (node->kind) {
    case ABSTRACT_SYNTAX_BLOCK: {
        if (node->childCount > 0 && node->children == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        size_t counter = 0;
        CompilerStatus status = {0};

        while (counter < node->childCount) {
            status = generateStatement(generator, node->children[counter]);
            if (status != COMPILER_OK) {
                return status;
            }

            counter++;
        }

        return COMPILER_OK;
    }
    case ABSTRACT_SYNTAX_VARIABLE_DECLARATION: {
        if (node->text == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        if (node->childCount != 1) {
            return COMPILER_INTERNAL_ERROR;
        }

        if (node->children == NULL || node->children[0] == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        IntermediateOperand initializerOperand = {0};
        CompilerStatus status = {0};

        status = generateExpression(generator, &initializerOperand, node->children[0]);
        if (status != COMPILER_OK) {
            return status;
        }

        if (node->variableType == DATA_TYPE_DOUBLE && node->children[0]->inferredType == DATA_TYPE_INT) {
            status = convertIntToDouble(generator, &initializerOperand, node->location);
            if (status != COMPILER_OK) {
                intermediateOperandClear(&initializerOperand);
                return status;
            }
        }

        IntermediateOperand variableOperand = {0};
        status = generateVariable(&variableOperand, node);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&initializerOperand);
            return status;
        }

        status = appendVariableDeclaration(generator, &variableOperand, node->location);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&variableOperand);
            intermediateOperandClear(&initializerOperand);
            return status;
        }

        IntermediateInstruction instruction = fillInstruction(INTERMEDIATE_MOVE, 2, node->location);
        instruction.operands[0] = variableOperand;
        instruction.operands[1] = initializerOperand;

        status = intermediateProgramAppend(generator->program, &instruction);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&variableOperand);
            intermediateOperandClear(&initializerOperand);
            return COMPILER_INTERNAL_ERROR;
        }

        intermediateOperandClear(&variableOperand);
        intermediateOperandClear(&initializerOperand);

        return status;
    }
    case ABSTRACT_SYNTAX_ASSIGNMENT: {
        if (node->childCount != 1 || node->children == NULL || node->children[0] == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        IntermediateOperand valueOperand = {0};
        CompilerStatus status = generateExpression(generator, &valueOperand, node->children[0]);
        if (status != COMPILER_OK) {
            return status;
        }

        if (node->variableType == DATA_TYPE_DOUBLE && node->children[0]->inferredType == DATA_TYPE_INT) {
            status = convertIntToDouble(generator, &valueOperand, node->location);
            if (status != COMPILER_OK) {
                intermediateOperandClear(&valueOperand);
                return status;
            }
        }

        IntermediateOperand variableOperand = {0};
        status = generateVariable(&variableOperand, node);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&valueOperand);
            return status;
        }

        IntermediateInstruction instruction = fillInstruction(INTERMEDIATE_MOVE, 2, node->location);
        instruction.operands[0] = variableOperand;
        instruction.operands[1] = valueOperand;

        status = intermediateProgramAppend(generator->program, &instruction);

        intermediateOperandClear(&variableOperand);
        intermediateOperandClear(&valueOperand);
        return status;
    }
    case ABSTRACT_SYNTAX_IF: {
        if (node->childCount != 3 || node->children == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        if (node->children[0] == NULL || node->children[1] == NULL || node->children[2] == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        if (node->children[0]->inferredType != DATA_TYPE_BOOLEAN) {
            return COMPILER_INTERNAL_ERROR;
        }

        IntermediateOperand elseLabelOperand = {0};
        CompilerStatus status = generateLabel(generator, &elseLabelOperand);
        if (status != COMPILER_OK) {
            return status;
        }

        IntermediateOperand endLabelOperand = {0};
        status = generateLabel(generator, &endLabelOperand);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&elseLabelOperand);
            return status;
        }

        IntermediateOperand conditionOperand = {0};
        status = generateExpression(generator, &conditionOperand, node->children[0]);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&elseLabelOperand);
            intermediateOperandClear(&endLabelOperand);
            return status;
        }

        IntermediateOperand falseOperand = {0};
        falseOperand.kind = INTERMEDIATE_OPERAND_BOOLEAN;
        falseOperand.value.boolValue = false;

        IntermediateInstruction jumpInstruction = fillInstruction(INTERMEDIATE_JUMP_IF_EQUAL, 3, node->location);
        jumpInstruction.operands[0] = elseLabelOperand;
        jumpInstruction.operands[1] = conditionOperand;
        jumpInstruction.operands[2] = falseOperand;

        status = intermediateProgramAppend(generator->program, &jumpInstruction);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&conditionOperand);
            intermediateOperandClear(&elseLabelOperand);
            intermediateOperandClear(&endLabelOperand);
            return status;
        }

        status = generateStatement(generator, node->children[1]);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&conditionOperand);
            intermediateOperandClear(&elseLabelOperand);
            intermediateOperandClear(&endLabelOperand);
            return status;
        }

        IntermediateInstruction endJumpInstruction = fillInstruction(INTERMEDIATE_JUMP, 1, node->location);
        endJumpInstruction.operands[0] = endLabelOperand;

        status = intermediateProgramAppend(generator->program, &endJumpInstruction);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&conditionOperand);
            intermediateOperandClear(&elseLabelOperand);
            intermediateOperandClear(&endLabelOperand);
            return status;
        }

        IntermediateInstruction elseLabelInstruction = fillInstruction(INTERMEDIATE_LABEL, 1, node->location);
        elseLabelInstruction.operands[0] = elseLabelOperand;

        status = intermediateProgramAppend(generator->program, &elseLabelInstruction);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&conditionOperand);
            intermediateOperandClear(&elseLabelOperand);
            intermediateOperandClear(&endLabelOperand);
            return status;
        }

        status = generateStatement(generator, node->children[2]);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&conditionOperand);
            intermediateOperandClear(&elseLabelOperand);
            intermediateOperandClear(&endLabelOperand);
            return status;
        }

        IntermediateInstruction endLabelInstruction = fillInstruction(INTERMEDIATE_LABEL, 1, node->location);
        endLabelInstruction.operands[0] = endLabelOperand;
        status = intermediateProgramAppend(generator->program, &endLabelInstruction);

        intermediateOperandClear(&conditionOperand);
        intermediateOperandClear(&elseLabelOperand);
        intermediateOperandClear(&endLabelOperand);

        return status;
    }
    case ABSTRACT_SYNTAX_WHILE: {
        if (node->childCount != 2 || node->children == NULL || node->children[0] == NULL || node->children[1] == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        if (node->children[0]->inferredType != DATA_TYPE_BOOLEAN) {
            return COMPILER_INTERNAL_ERROR;
        }

        IntermediateOperand startLabelOperand = {0};
        CompilerStatus status = generateLabel(generator, &startLabelOperand);
        if (status != COMPILER_OK) {
            return status;
        }

        IntermediateOperand endLabelOperand = {0};
        status = generateLabel(generator, &endLabelOperand);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&startLabelOperand);
            return status;
        }

        IntermediateInstruction startLabelInstruction = fillInstruction(INTERMEDIATE_LABEL, 1, node->location);
        startLabelInstruction.operands[0] = startLabelOperand;

        status = intermediateProgramAppend(generator->program, &startLabelInstruction);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&endLabelOperand);
            intermediateOperandClear(&startLabelOperand);
            return status;
        }

        IntermediateOperand conditionOperand = {0};
        status = generateExpression(generator, &conditionOperand, node->children[0]);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&endLabelOperand);
            intermediateOperandClear(&startLabelOperand);
            return status;
        }

        IntermediateOperand falseOperand = {0};
        falseOperand.kind = INTERMEDIATE_OPERAND_BOOLEAN;
        falseOperand.value.boolValue = false;

        IntermediateInstruction jumpInstruction = fillInstruction(INTERMEDIATE_JUMP_IF_EQUAL, 3, node->location);
        jumpInstruction.operands[0] = endLabelOperand;
        jumpInstruction.operands[1] = conditionOperand;
        jumpInstruction.operands[2] = falseOperand;

        status = intermediateProgramAppend(generator->program, &jumpInstruction);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&conditionOperand);
            intermediateOperandClear(&endLabelOperand);
            intermediateOperandClear(&startLabelOperand);
            return status;
        }

        status = generateStatement(generator, node->children[1]);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&conditionOperand);
            intermediateOperandClear(&endLabelOperand);
            intermediateOperandClear(&startLabelOperand);
            return status;
        }

        IntermediateInstruction startJumpInstruction = fillInstruction(INTERMEDIATE_JUMP, 1, node->location);
        startJumpInstruction.operands[0] = startLabelOperand;

        status = intermediateProgramAppend(generator->program, &startJumpInstruction);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&conditionOperand);
            intermediateOperandClear(&endLabelOperand);
            intermediateOperandClear(&startLabelOperand);
            return status;
        }

        IntermediateInstruction endLabelInstruction = fillInstruction(INTERMEDIATE_LABEL, 1, node->location);
        endLabelInstruction.operands[0] = endLabelOperand;
        status = intermediateProgramAppend(generator->program, &endLabelInstruction);

        intermediateOperandClear(&conditionOperand);
        intermediateOperandClear(&endLabelOperand);
        intermediateOperandClear(&startLabelOperand);

        return status;
    }
    case ABSTRACT_SYNTAX_RETURN: {
        if (node->childCount > 1) {
            return COMPILER_INTERNAL_ERROR;
        }

        if (node->childCount == 1) {
            if (node->children == NULL || node->children[0] == NULL) {
                return COMPILER_INTERNAL_ERROR;
            }
        }

        if (generator->returnType != DATA_TYPE_INT && generator->returnType != DATA_TYPE_DOUBLE && generator->returnType != DATA_TYPE_STRING && generator->returnType != DATA_TYPE_UNIT) {
            return COMPILER_INTERNAL_ERROR;
        }

        if (node->childCount == 0) {
            if (generator->returnType != DATA_TYPE_UNIT) {
                return COMPILER_INTERNAL_ERROR;
            }
        }

        IntermediateOperand returnOperand = {0};
        CompilerStatus status = COMPILER_OK;

        if (node->childCount == 0) {
            returnOperand.kind = INTERMEDIATE_OPERAND_UNIT;
        } else {
            status = generateExpression(generator, &returnOperand, node->children[0]);
            if (status != COMPILER_OK) {
                return status;
            }
        }

        if (generator->returnType == DATA_TYPE_UNIT) {
            intermediateOperandClear(&returnOperand);
            returnOperand.kind = INTERMEDIATE_OPERAND_UNIT;
            returnOperand.value = (IntermediateOperandValue){0};
        } else {
            DataType valueType = node->children[0]->inferredType;
            if (valueType != generator->returnType) {
                if (valueType == DATA_TYPE_INT && generator->returnType == DATA_TYPE_DOUBLE) {
                    status = convertIntToDouble(generator, &returnOperand, node->location);
                    if (status != COMPILER_OK) {
                        intermediateOperandClear(&returnOperand);
                        return status;
                    }
                } else {
                    intermediateOperandClear(&returnOperand);
                    return COMPILER_INTERNAL_ERROR;
                }
            }
        }

        IntermediateInstruction returnValueInstruction = fillInstruction(INTERMEDIATE_PUSH, 1, node->location);
        returnValueInstruction.operands[0] = returnOperand;

        status = intermediateProgramAppend(generator->program, &returnValueInstruction);
        intermediateOperandClear(&returnOperand);
        if (status != COMPILER_OK) {
            return status;
        }

        IntermediateInstruction popFrameInstruction = fillInstruction(INTERMEDIATE_POP_FRAME, 0, node->location);

        status = intermediateProgramAppend(generator->program, &popFrameInstruction);
        if (status != COMPILER_OK) {
            return status;
        }

        IntermediateInstruction returnInstruction = fillInstruction(INTERMEDIATE_RETURN, 0, node->location);
        status = intermediateProgramAppend(generator->program, &returnInstruction);

        return status;
    }
    case ABSTRACT_SYNTAX_FUNCTION_CALL: {
        const AbstractSyntaxNode *expressionNode = node;

        if (node->kind == ABSTRACT_SYNTAX_EXPRESSION_STATEMENT) {
            if (node->childCount != 1 || node->children == NULL || node->children[0] == NULL) {
                return COMPILER_INTERNAL_ERROR;
            }

            expressionNode = node->children[0];
        }

        IntermediateOperand resultOperand = {0};
        CompilerStatus status = generateExpression(generator, &resultOperand, expressionNode);

        intermediateOperandClear(&resultOperand);

        return status;
    }
    case ABSTRACT_SYNTAX_EXPRESSION_STATEMENT: {
        const AbstractSyntaxNode *expressionNode = node;

        if (node->kind == ABSTRACT_SYNTAX_EXPRESSION_STATEMENT) {
            if (node->childCount != 1 || node->children == NULL || node->children[0] == NULL) {
                return COMPILER_INTERNAL_ERROR;
            }

            expressionNode = node->children[0];
        }

        IntermediateOperand resultOperand = {0};
        CompilerStatus status = generateExpression(generator, &resultOperand, expressionNode);

        intermediateOperandClear(&resultOperand);

        return status;
    }
    default:
        return COMPILER_INTERNAL_ERROR;
    }
}

static CompilerStatus appendProgramInstructions(IntermediateProgram *destination, const IntermediateProgram *source) {
    if (destination == NULL || source == NULL || destination == source) {
        return COMPILER_INTERNAL_ERROR;
    }

    if (source->count > 0) {
        if (source->instructions == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }
    }

    size_t counter = 0;
    CompilerStatus status = {0};

    while (counter < source->count) {
        status = intermediateProgramAppend(destination, &source->instructions[counter]);
        if (status != COMPILER_OK) {
            return status;
        }

        counter++;
    }

    return COMPILER_OK;
}

static CompilerStatus generateParameters(IntermediateGenerator *generator, const AbstractSyntaxNode *function) {
    if (generator == NULL || function == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    if (function->kind != ABSTRACT_SYNTAX_FUNCTION) {
        return COMPILER_INTERNAL_ERROR;
    }

    if (!(function->childCount > 0) || function->children == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    size_t counter = function->childCount - 1;

    while (counter > 0) {
        counter--;
        const AbstractSyntaxNode *parameter = function->children[counter];
        if (parameter == NULL || parameter->kind != ABSTRACT_SYNTAX_PARAMETER || parameter->variableId == 0) {
            return COMPILER_INTERNAL_ERROR;
        }

        if (parameter->variableType != DATA_TYPE_INT && parameter->variableType != DATA_TYPE_DOUBLE && parameter->variableType != DATA_TYPE_STRING && parameter->variableType != DATA_TYPE_UNIT) {
            return COMPILER_INTERNAL_ERROR;
        }

        IntermediateOperand parameterOperand = {0};
        CompilerStatus status = generateVariable(&parameterOperand, parameter);
        if (status != COMPILER_OK) {
            return status;
        }

        status = appendVariableDeclaration(generator, &parameterOperand, parameter->location);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&parameterOperand);
            return status;
        }

        IntermediateInstruction parameterInstruction = fillInstruction(INTERMEDIATE_POP, 1, parameter->location);
        parameterInstruction.operands[0] = parameterOperand;

        status = intermediateProgramAppend(generator->program, &parameterInstruction);
        intermediateOperandClear(&parameterOperand);
        if (status != COMPILER_OK) {
            return status;
        }
    }

    return COMPILER_OK;
}

static CompilerStatus generateFunctionBody(IntermediateGenerator *generator, const AbstractSyntaxNode *body) {
    if (generator == NULL || body == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    if (body->kind != ABSTRACT_SYNTAX_BLOCK) {
        return COMPILER_INTERNAL_ERROR;
    }

    if (body->childCount > 0 && body->children == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    if (generator->returnType == DATA_TYPE_UNIT) {
        return generateStatement(generator, body);
    }

    if (body->childCount == 0) {
        return COMPILER_INTERNAL_ERROR;
    }

    if (body->children[body->childCount - 1] == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    const AbstractSyntaxNode *lastStatement = body->children[body->childCount - 1];
    const AbstractSyntaxNode *expressionNode = NULL;

    if (lastStatement->kind == ABSTRACT_SYNTAX_EXPRESSION_STATEMENT) {
        if (lastStatement->childCount != 1 || lastStatement->children == NULL || lastStatement->children[0] == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        expressionNode = lastStatement->children[0];
    } else if (lastStatement->kind == ABSTRACT_SYNTAX_FUNCTION_CALL) {
        expressionNode = lastStatement;
    } else {
        return generateStatement(generator, body);
    }

    size_t counter = 0;
    CompilerStatus status = COMPILER_OK;

    while (counter < body->childCount - 1) {
        status = generateStatement(generator, body->children[counter]);
        if (status != COMPILER_OK) {
            return status;
        }

        counter++;
    }

    IntermediateOperand returnOperand = {0};
    status = generateExpression(generator, &returnOperand, expressionNode);
    if (status != COMPILER_OK) {
        return status;
    }

    if (expressionNode->inferredType != generator->returnType) {
        if (expressionNode->inferredType == DATA_TYPE_INT && generator->returnType == DATA_TYPE_DOUBLE) {
            status = convertIntToDouble(generator, &returnOperand, expressionNode->location);
            if (status != COMPILER_OK) {
                intermediateOperandClear(&returnOperand);
                return status;
            }
        } else {
            intermediateOperandClear(&returnOperand);
            return COMPILER_INTERNAL_ERROR;
        }
    }

    IntermediateInstruction returnValueInstruction = fillInstruction(INTERMEDIATE_PUSH, 1, expressionNode->location);
    returnValueInstruction.operands[0] = returnOperand;

    status = intermediateProgramAppend(generator->program, &returnValueInstruction);
    intermediateOperandClear(&returnOperand);

    return status;
}

static CompilerStatus generateFunction(IntermediateGenerator *generator, const AbstractSyntaxNode *node) {
    if (generator == NULL || node == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    if (node->kind != ABSTRACT_SYNTAX_FUNCTION || node->text == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    if (node->childCount == 0 || node->children == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    const AbstractSyntaxNode *body = node->children[node->childCount - 1];
    if (body == NULL || body->kind != ABSTRACT_SYNTAX_BLOCK) {
        return COMPILER_INTERNAL_ERROR;
    }

    IntermediateProgram *functionDeclarations = intermediateProgramCreate();
    if (functionDeclarations == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    IntermediateProgram *functionBody = intermediateProgramCreate();
    if (functionBody == NULL) {
        intermediateProgramDestroy(functionDeclarations);
        return COMPILER_INTERNAL_ERROR;
    }

    IntermediateGenerator functionGenerator = *generator;
    functionGenerator.program = functionBody;
    functionGenerator.functionDeclarations = functionDeclarations;
    functionGenerator.returnType = node->inferredType;

    CompilerStatus status = generateParameters(&functionGenerator, node);
    if (status != COMPILER_OK) {
        intermediateProgramDestroy(functionBody);
        intermediateProgramDestroy(functionDeclarations);
        return status;
    }

    status = generateFunctionBody(&functionGenerator, body);

    generator->temporaryCounter = functionGenerator.temporaryCounter;
    generator->labelCounter = functionGenerator.labelCounter;

    if (status != COMPILER_OK) {
        intermediateProgramDestroy(functionBody);
        intermediateProgramDestroy(functionDeclarations);
        return status;
    }

    IntermediateOperand functionLabel = {0};
    functionLabel.kind = INTERMEDIATE_OPERAND_LABEL;
    functionLabel.text = createFunctionLabel(node->text);
    if (functionLabel.text == NULL) {
        intermediateProgramDestroy(functionBody);
        intermediateProgramDestroy(functionDeclarations);
        return COMPILER_INTERNAL_ERROR;
    }

    IntermediateInstruction labelInstruction = fillInstruction(INTERMEDIATE_LABEL, 1, node->location);
    labelInstruction.operands[0] = functionLabel;

    status = intermediateProgramAppend(generator->program, &labelInstruction);
    intermediateOperandClear(&functionLabel);
    if (status != COMPILER_OK) {
        intermediateProgramDestroy(functionBody);
        intermediateProgramDestroy(functionDeclarations);
        return status;
    }

    IntermediateInstruction pushFrameInstruction = fillInstruction(INTERMEDIATE_PUSH_FRAME, 0, node->location);

    status = intermediateProgramAppend(generator->program, &pushFrameInstruction);
    if (status != COMPILER_OK) {
        intermediateProgramDestroy(functionBody);
        intermediateProgramDestroy(functionDeclarations);
        return status;
    }

    status = appendProgramInstructions(generator->program, functionDeclarations);
    intermediateProgramDestroy(functionDeclarations);
    if (status != COMPILER_OK) {
        intermediateProgramDestroy(functionBody);
        return status;
    }

    status = appendProgramInstructions(generator->program, functionBody);
    intermediateProgramDestroy(functionBody);
    if (status != COMPILER_OK) {
        return status;
    }

    if (functionGenerator.returnType == DATA_TYPE_UNIT) {
        IntermediateOperand returnOperand = {0};
        returnOperand.kind = INTERMEDIATE_OPERAND_UNIT;

        IntermediateInstruction returnValueInstruction = fillInstruction(INTERMEDIATE_PUSH, 1, node->location);
        returnValueInstruction.operands[0] = returnOperand;

        status = intermediateProgramAppend(generator->program, &returnValueInstruction);
        if (status != COMPILER_OK) {
            return status;
        }
    }

    IntermediateInstruction popFrameInstruction = fillInstruction(INTERMEDIATE_POP_FRAME, 0, node->location);

    status = intermediateProgramAppend(generator->program, &popFrameInstruction);
    if (status != COMPILER_OK) {
        return status;
    }

    IntermediateInstruction returnInstruction = fillInstruction(INTERMEDIATE_RETURN, 0, node->location);

    status = intermediateProgramAppend(generator->program, &returnInstruction);
    if (status != COMPILER_OK) {
        return status;
    }

    return COMPILER_OK;
}

CompilerStatus intermediateCodeGenerate(const SourceProgram *program, IntermediateProgram *outProgram) {
    if (program == NULL || outProgram == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    if (program->root == NULL || program->root->kind != ABSTRACT_SYNTAX_PROGRAM) {
        return COMPILER_INTERNAL_ERROR;
    }

    if (program->root->childCount > 0) {
        if (program->root->children == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }
    }

    const AbstractSyntaxNode *entryFunction = NULL;
    size_t counter = 0;

    while (counter < program->root->childCount) {
        const AbstractSyntaxNode *function = program->root->children[counter];

        if (function == NULL ||
            function->kind != ABSTRACT_SYNTAX_FUNCTION ||
            function->text == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        if (function->isEntryFunction) {
            if (entryFunction != NULL) {
                return COMPILER_INTERNAL_ERROR;
            }

            entryFunction = function;
        }

        counter++;
    }

    if (entryFunction == NULL || entryFunction->childCount != 1 || entryFunction->inferredType != DATA_TYPE_UNIT) {
        return COMPILER_INTERNAL_ERROR;
    }

    IntermediateGenerator generator = {0};
    generator.program = outProgram;
    generator.sourceProgram = program;

    IntermediateInstruction createFrameInstruction = fillInstruction(INTERMEDIATE_CREATE_FRAME, 0, entryFunction->location);

    CompilerStatus status = intermediateProgramAppend(generator.program, &createFrameInstruction);
    if (status != COMPILER_OK) {
        return status;
    }

    IntermediateOperand entryLabel = {0};
    entryLabel.kind = INTERMEDIATE_OPERAND_LABEL;
    entryLabel.text = createFunctionLabel(entryFunction->text);
    if (entryLabel.text == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    IntermediateInstruction callInstruction = fillInstruction(INTERMEDIATE_CALL, 1, entryFunction->location);
    callInstruction.operands[0] = entryLabel;

    status = intermediateProgramAppend(generator.program, &callInstruction);
    intermediateOperandClear(&entryLabel);
    if (status != COMPILER_OK) {
        return status;
    }

    IntermediateInstruction exitInstruction = fillInstruction(INTERMEDIATE_EXIT, 1, entryFunction->location);
    exitInstruction.operands[0].kind = INTERMEDIATE_OPERAND_INT;
    exitInstruction.operands[0].value.intValue = 0;

    status = intermediateProgramAppend(generator.program, &exitInstruction);
    if (status != COMPILER_OK) {
        return status;
    }

    counter = 0;

    while (counter < program->root->childCount) {
        CompilerStatus status = generateFunction(&generator, program->root->children[counter]);
        if (status != COMPILER_OK) {
            return status;
        }

        counter++;
    }

    return COMPILER_OK;
}
// Boeing
