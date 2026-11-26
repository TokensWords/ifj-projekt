/*
 * Project: IFJ26 compiler
 * Authors: Albert Hajek xhajeka00
 * AI-assisted scaffold.
 * Prompt: Implement intermediate-code helpers and a generator boundary.
 */

#include "intermediateCode.h"

#include "memory.h"
#include "typeDescriptor.h"

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
    bool needsListString;
} IntermediateGenerator;

/**
 * @brief gets flags stored in a type descriptor
 * @param type type descriptor
 * @return type flags or DATA_TYPE_UNKNOWN
 */
static DataType descriptorDataType(const TypeDescriptor *type) {
    return type == NULL ? DATA_TYPE_UNKNOWN : type->datatypeBitflag;
}

/**
 * @brief frees operand text and resets its kind
 * @param operand intermediate operand
 */
static void intermediateOperandClear(IntermediateOperand *operand) {
    free(operand->text);
    operand->text = NULL;
    operand->kind = INTERMEDIATE_OPERAND_NONE;
}

/**
 * @brief clears every operand in instruction
 * @param instruction intermediate instruction
 */
static void intermediateInstructionClear(IntermediateInstruction *instruction) {
    for (size_t index = 0; index < instruction->operandCount; index++) {
        intermediateOperandClear(&instruction->operands[index]);
    }
}

/**
 * @brief creates an empty intermediate program
 * @return pointer to program or NULL
 */
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

/**
 * @brief frees intermediate program and its instructions
 * @param program intermediate program
 */
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

/**
 * @brief appends a copy of an instruction and its operand text
 * @param program intermediate program
 * @param instruction intermediate instruction
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
CompilerStatus intermediateProgramAppend(IntermediateProgram *program, const IntermediateInstruction *instruction) {
    if (program == NULL || instruction == NULL || instruction->operandCount > 3) {
        return COMPILER_INTERNAL_ERROR;
    }

    if (program->count == program->capacity) {
        size_t newCapacity = program->capacity == 0 ? 16 : program->capacity * 2;
        IntermediateInstruction *newInstructions = memoryResizeArray(program->instructions, newCapacity, sizeof(IntermediateInstruction));
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

/**
 * @brief initializes an instruction with its opcode, operand count and source location
 * @param opcode intermediate opcode
 * @param operandCount number of operands
 * @param origin source location
 * @return initialized instruction
 */
static IntermediateInstruction fillInstruction(IntermediateOpcode opcode, size_t operandCount, SourceLocation origin) {
    IntermediateInstruction instruction = {0};

    instruction.opcode = opcode;
    instruction.operandCount = operandCount;
    instruction.origin = origin;

    return instruction;
}

/**
 * @brief counts decimal digits in a number
 * @param number number to count digits on
 * @return digit count
 */
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

/**
 * @brief converts a number to a string
 * @param number number to convert
 * @return allocated string or NULL
 */
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

/**
 * @brief creates a name from a prefix and a number
 * @param type name prefix
 * @param number number to append to prefix
 * @return allocated name or NULL
 */
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

/**
 * @brief adds a variable declaration to function
 * @param generator generator state
 * @param operand intermediate operand
 * @param origin source location
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
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

/**
 * @brief creates a temporary variable operand and adds its declaration
 * @param generator generator state
 * @param operand intermediate operand
 * @param origin source location
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
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

/**
 * @brief creates a unique label operand
 * @param generator generator state
 * @param operand intermediate operand
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
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

/**
 * @brief creates a variable operand from variable ID on an AST node
 * @param operand intermediate operand
 * @param node source AST node
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
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

/**
 * @brief checks if type is Int or Double
 * @param dataType type flags
 * @return true or false
 */
static bool isNumericType(DataType dataType) {
    return dataType == DATA_TYPE_INT || dataType == DATA_TYPE_DOUBLE;
}

/**
 * @brief generates conversion from Int oprand to Double
 * @param generator generator state
 * @param operand intermediate operand
 * @param origin source location
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
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

/**
 * @brief clears operands in an array
 * @param operands operands
 * @param count number of entries
 */
static void clearOperandArray(IntermediateOperand *operands, size_t count) {
    if (operands == NULL) {
        return;
    }

    size_t counter = 0;

    while (counter < count) {
        intermediateOperandClear(&operands[counter]);
        counter++;
    }
}

/**
 * @brief clears operands referenced by an array of pointers
 * @param operands operands
 * @param count number of entries
 */
static void clearOperandPointers(IntermediateOperand *const *operands, size_t count) {
    size_t counter = 0;
    while (counter < count) {
        intermediateOperandClear(operands[counter]);
        counter++;
    }
}

/**
 * @brief appends instructions from an array
 * @param generator generator state
 * @param instructions instructions to append
 * @param count number of entries
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus appendInstructionArray(IntermediateGenerator *generator, const IntermediateInstruction *instructions, size_t count) {
    if (generator == NULL || generator->program == NULL || (count > 0 && instructions == NULL)) {
        return COMPILER_INTERNAL_ERROR;
    }

    size_t counter = 0;

    while (counter < count) {
        CompilerStatus status = intermediateProgramAppend(generator->program, &instructions[counter]);
        if (status != COMPILER_OK) {
            return status;
        }

        counter++;
    }

    return COMPILER_OK;
}

/**
 * @brief generates code for ord, returns -1 if the index is out of bounds
 * @param generator generator state
 * @param resultOperand destination operand
 * @param stringOperand string operand
 * @param indexOperand character index operand
 * @param origin source location
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateOrdinal(IntermediateGenerator *generator, const IntermediateOperand *resultOperand, const IntermediateOperand *stringOperand, const IntermediateOperand *indexOperand, SourceLocation origin) {
    if (generator == NULL || resultOperand == NULL || stringOperand == NULL || indexOperand == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    IntermediateOperand temporaryOperands[2] = {0};
    size_t counter = 0;

    while (counter < 2) {
        CompilerStatus status = generateTemporary(generator, &temporaryOperands[counter], origin);
        if (status != COMPILER_OK) {
            clearOperandArray(temporaryOperands, 2);
            return status;
        }

        counter++;
    }

    IntermediateOperand *lengthOperand = &temporaryOperands[0];
    IntermediateOperand *conditionOperand = &temporaryOperands[1];
    IntermediateOperand endLabelOperand = {0};
    CompilerStatus status = generateLabel(generator, &endLabelOperand);
    if (status != COMPILER_OK) {
        clearOperandArray(temporaryOperands, 2);
        return status;
    }

    IntermediateOperand negativeOperand = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = -1};
    IntermediateOperand zeroOperand = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 0};
    IntermediateOperand trueOperand = {.kind = INTERMEDIATE_OPERAND_BOOLEAN, .value.boolValue = true};
    IntermediateOperand falseOperand = {.kind = INTERMEDIATE_OPERAND_BOOLEAN, .value.boolValue = false};

    IntermediateInstruction resultInstruction = fillInstruction(INTERMEDIATE_MOVE, 2, origin);
    resultInstruction.operands[0] = *resultOperand;
    resultInstruction.operands[1] = negativeOperand;

    IntermediateInstruction lengthInstruction = fillInstruction(INTERMEDIATE_STRING_LENGTH, 2, origin);
    lengthInstruction.operands[0] = *lengthOperand;
    lengthInstruction.operands[1] = *stringOperand;

    IntermediateInstruction negativeInstruction = fillInstruction(INTERMEDIATE_COMPARE_LESS, 3, origin);
    negativeInstruction.operands[0] = *conditionOperand;
    negativeInstruction.operands[1] = *indexOperand;
    negativeInstruction.operands[2] = zeroOperand;

    IntermediateInstruction negativeJumpInstruction = fillInstruction(INTERMEDIATE_JUMP_IF_EQUAL, 3, origin);
    negativeJumpInstruction.operands[0] = endLabelOperand;
    negativeJumpInstruction.operands[1] = *conditionOperand;
    negativeJumpInstruction.operands[2] = trueOperand;

    IntermediateInstruction rangeInstruction = fillInstruction(INTERMEDIATE_COMPARE_LESS, 3, origin);
    rangeInstruction.operands[0] = *conditionOperand;
    rangeInstruction.operands[1] = *indexOperand;
    rangeInstruction.operands[2] = *lengthOperand;

    IntermediateInstruction rangeJumpInstruction = fillInstruction(INTERMEDIATE_JUMP_IF_EQUAL, 3, origin);
    rangeJumpInstruction.operands[0] = endLabelOperand;
    rangeJumpInstruction.operands[1] = *conditionOperand;
    rangeJumpInstruction.operands[2] = falseOperand;

    IntermediateInstruction characterInstruction = fillInstruction(INTERMEDIATE_STRING_CHARACTER_CODE, 3, origin);
    characterInstruction.operands[0] = *resultOperand;
    characterInstruction.operands[1] = *stringOperand;
    characterInstruction.operands[2] = *indexOperand;

    IntermediateInstruction endInstruction = fillInstruction(INTERMEDIATE_LABEL, 1, origin);
    endInstruction.operands[0] = endLabelOperand;

    IntermediateInstruction instructions[] = {resultInstruction, lengthInstruction, negativeInstruction, negativeJumpInstruction, rangeInstruction, rangeJumpInstruction, characterInstruction, endInstruction};
    status = appendInstructionArray(generator, instructions, sizeof(instructions) / sizeof(instructions[0]));
    intermediateOperandClear(&endLabelOperand);
    clearOperandArray(temporaryOperands, 2);

    return status;
}

/**
 * @brief generates code for subStr that checks the range before copying the characters
 * @param generator generator state
 * @param resultOperand destination operand
 * @param stringOperand string operand
 * @param startOperand inclusive start index
 * @param endOperand exclusive end index
 * @param origin source location
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateSubstring(IntermediateGenerator *generator, const IntermediateOperand *resultOperand, const IntermediateOperand *stringOperand, const IntermediateOperand *startOperand, const IntermediateOperand *endOperand, SourceLocation origin) {
    if (generator == NULL || resultOperand == NULL || stringOperand == NULL || startOperand == NULL || endOperand == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    IntermediateOperand temporaryOperands[4] = {0};
    size_t counter = 0;

    while (counter < 4) {
        CompilerStatus status = generateTemporary(generator, &temporaryOperands[counter], origin);
        if (status != COMPILER_OK) {
            clearOperandArray(temporaryOperands, 4);
            return status;
        }

        counter++;
    }

    IntermediateOperand *lengthOperand = &temporaryOperands[0];
    IntermediateOperand *indexOperand = &temporaryOperands[1];
    IntermediateOperand *characterOperand = &temporaryOperands[2];
    IntermediateOperand *conditionOperand = &temporaryOperands[3];
    IntermediateOperand labelOperands[3] = {0};

    counter = 0;

    while (counter < 3) {
        CompilerStatus status = generateLabel(generator, &labelOperands[counter]);
        if (status != COMPILER_OK) {
            clearOperandArray(labelOperands, 3);
            clearOperandArray(temporaryOperands, 4);
            return status;
        }

        counter++;
    }

    IntermediateOperand *loopLabelOperand = &labelOperands[0];
    IntermediateOperand *errorLabelOperand = &labelOperands[1];
    IntermediateOperand *endLabelOperand = &labelOperands[2];
    IntermediateOperand zeroOperand = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 0};
    IntermediateOperand oneOperand = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 1};
    IntermediateOperand errorOperand = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 40};
    IntermediateOperand trueOperand = {.kind = INTERMEDIATE_OPERAND_BOOLEAN, .value.boolValue = true};
    IntermediateOperand falseOperand = {.kind = INTERMEDIATE_OPERAND_BOOLEAN, .value.boolValue = false};

    IntermediateOperand emptyOperand = {.kind = INTERMEDIATE_OPERAND_STRING, .text = ""};

    IntermediateInstruction lengthInstruction = fillInstruction(INTERMEDIATE_STRING_LENGTH, 2, origin);
    lengthInstruction.operands[0] = *lengthOperand;
    lengthInstruction.operands[1] = *stringOperand;

    IntermediateInstruction startNegativeInstruction = fillInstruction(INTERMEDIATE_COMPARE_LESS, 3, origin);
    startNegativeInstruction.operands[0] = *conditionOperand;
    startNegativeInstruction.operands[1] = *startOperand;
    startNegativeInstruction.operands[2] = zeroOperand;

    IntermediateInstruction startNegativeJumpInstruction = fillInstruction(INTERMEDIATE_JUMP_IF_EQUAL, 3, origin);
    startNegativeJumpInstruction.operands[0] = *errorLabelOperand;
    startNegativeJumpInstruction.operands[1] = *conditionOperand;
    startNegativeJumpInstruction.operands[2] = trueOperand;

    IntermediateInstruction endNegativeInstruction = fillInstruction(INTERMEDIATE_COMPARE_LESS, 3, origin);
    endNegativeInstruction.operands[0] = *conditionOperand;
    endNegativeInstruction.operands[1] = *endOperand;
    endNegativeInstruction.operands[2] = zeroOperand;

    IntermediateInstruction endNegativeJumpInstruction = fillInstruction(INTERMEDIATE_JUMP_IF_EQUAL, 3, origin);
    endNegativeJumpInstruction.operands[0] = *errorLabelOperand;
    endNegativeJumpInstruction.operands[1] = *conditionOperand;
    endNegativeJumpInstruction.operands[2] = trueOperand;

    IntermediateInstruction orderInstruction = fillInstruction(INTERMEDIATE_COMPARE_GREATER, 3, origin);
    orderInstruction.operands[0] = *conditionOperand;
    orderInstruction.operands[1] = *startOperand;
    orderInstruction.operands[2] = *endOperand;

    IntermediateInstruction orderJumpInstruction = fillInstruction(INTERMEDIATE_JUMP_IF_EQUAL, 3, origin);
    orderJumpInstruction.operands[0] = *errorLabelOperand;
    orderJumpInstruction.operands[1] = *conditionOperand;
    orderJumpInstruction.operands[2] = trueOperand;

    IntermediateInstruction startRangeInstruction = fillInstruction(INTERMEDIATE_COMPARE_LESS, 3, origin);
    startRangeInstruction.operands[0] = *conditionOperand;
    startRangeInstruction.operands[1] = *startOperand;
    startRangeInstruction.operands[2] = *lengthOperand;

    IntermediateInstruction startRangeJumpInstruction = fillInstruction(INTERMEDIATE_JUMP_IF_EQUAL, 3, origin);
    startRangeJumpInstruction.operands[0] = *errorLabelOperand;
    startRangeJumpInstruction.operands[1] = *conditionOperand;
    startRangeJumpInstruction.operands[2] = falseOperand;

    IntermediateInstruction endRangeInstruction = fillInstruction(INTERMEDIATE_COMPARE_GREATER, 3, origin);
    endRangeInstruction.operands[0] = *conditionOperand;
    endRangeInstruction.operands[1] = *endOperand;
    endRangeInstruction.operands[2] = *lengthOperand;

    IntermediateInstruction endRangeJumpInstruction = fillInstruction(INTERMEDIATE_JUMP_IF_EQUAL, 3, origin);
    endRangeJumpInstruction.operands[0] = *errorLabelOperand;
    endRangeJumpInstruction.operands[1] = *conditionOperand;
    endRangeJumpInstruction.operands[2] = trueOperand;

    IntermediateInstruction resultInstruction = fillInstruction(INTERMEDIATE_MOVE, 2, origin);
    resultInstruction.operands[0] = *resultOperand;
    resultInstruction.operands[1] = emptyOperand;

    IntermediateInstruction indexInstruction = fillInstruction(INTERMEDIATE_MOVE, 2, origin);
    indexInstruction.operands[0] = *indexOperand;
    indexInstruction.operands[1] = *startOperand;

    IntermediateInstruction loopLabelInstruction = fillInstruction(INTERMEDIATE_LABEL, 1, origin);
    loopLabelInstruction.operands[0] = *loopLabelOperand;

    IntermediateInstruction loopConditionInstruction = fillInstruction(INTERMEDIATE_COMPARE_LESS, 3, origin);
    loopConditionInstruction.operands[0] = *conditionOperand;
    loopConditionInstruction.operands[1] = *indexOperand;
    loopConditionInstruction.operands[2] = *endOperand;

    IntermediateInstruction loopEndJumpInstruction = fillInstruction(INTERMEDIATE_JUMP_IF_EQUAL, 3, origin);
    loopEndJumpInstruction.operands[0] = *endLabelOperand;
    loopEndJumpInstruction.operands[1] = *conditionOperand;
    loopEndJumpInstruction.operands[2] = falseOperand;

    IntermediateInstruction characterInstruction = fillInstruction(INTERMEDIATE_STRING_GET_CHARACTER, 3, origin);
    characterInstruction.operands[0] = *characterOperand;
    characterInstruction.operands[1] = *stringOperand;
    characterInstruction.operands[2] = *indexOperand;

    IntermediateInstruction concatenateInstruction = fillInstruction(INTERMEDIATE_CONCATENATE, 3, origin);
    concatenateInstruction.operands[0] = *resultOperand;
    concatenateInstruction.operands[1] = *resultOperand;
    concatenateInstruction.operands[2] = *characterOperand;

    IntermediateInstruction incrementInstruction = fillInstruction(INTERMEDIATE_ADD, 3, origin);
    incrementInstruction.operands[0] = *indexOperand;
    incrementInstruction.operands[1] = *indexOperand;
    incrementInstruction.operands[2] = oneOperand;

    IntermediateInstruction loopJumpInstruction = fillInstruction(INTERMEDIATE_JUMP, 1, origin);
    loopJumpInstruction.operands[0] = *loopLabelOperand;

    IntermediateInstruction errorLabelInstruction = fillInstruction(INTERMEDIATE_LABEL, 1, origin);
    errorLabelInstruction.operands[0] = *errorLabelOperand;

    IntermediateInstruction exitInstruction = fillInstruction(INTERMEDIATE_EXIT, 1, origin);
    exitInstruction.operands[0] = errorOperand;

    IntermediateInstruction endLabelInstruction = fillInstruction(INTERMEDIATE_LABEL, 1, origin);
    endLabelInstruction.operands[0] = *endLabelOperand;

    IntermediateInstruction instructions[] = {lengthInstruction, startNegativeInstruction, startNegativeJumpInstruction, endNegativeInstruction, endNegativeJumpInstruction, orderInstruction, orderJumpInstruction, startRangeInstruction, startRangeJumpInstruction, endRangeInstruction, endRangeJumpInstruction, resultInstruction, indexInstruction, loopLabelInstruction, loopConditionInstruction, loopEndJumpInstruction, characterInstruction, concatenateInstruction, incrementInstruction, loopJumpInstruction, errorLabelInstruction, exitInstruction, endLabelInstruction};
    CompilerStatus status = appendInstructionArray(generator, instructions, sizeof(instructions) / sizeof(instructions[0]));
    clearOperandArray(labelOperands, 3);
    clearOperandArray(temporaryOperands, 4);

    return status;
}

/**
 * @brief creates intermediate label name for source function
 * @param functionName source function name
 * @return allocated label name or NULL
 */
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

/**
 * @brief finds top-level function by name
 * @param program source program
 * @param functionName source function name
 * @return function node or NULL
 */
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

            if (function != NULL && function->kind == ABSTRACT_SYNTAX_FUNCTION && function->text != NULL && strcmp(function->text, functionName) == 0) {
                return function;
            }

            counter++;
        }
    }

    return NULL;
}

/**
 * @brief finds top-level type declaration by name and kind
 * @param program source program
 * @param name declaration name
 * @param kind declaration kind
 * @return type declaration node or NULL
 */
static const AbstractSyntaxNode *findTypeDeclaration(const SourceProgram *program, const char *name, AbstractSyntaxNodeKind kind) {
    size_t counter = 0;
    while (counter < program->root->childCount) {
        const AbstractSyntaxNode *node = program->root->children[counter];
        if (node != NULL && node->kind == kind && node->text != NULL && strcmp(node->text, name) == 0) {
            return node;
        }
        counter++;
    }

    return NULL;
}

/**
 * @brief resolves type aliases and collects their type flags
 * @param program source program
 * @param type type descriptor
 * @param remaining remaining recursion depth
 * @return resolved type flags or DATA_TYPE_UNKNOWN
 */
static DataType resolvedDataTypes(const SourceProgram *program, const TypeDescriptor *type, size_t remaining) {
    if (remaining == 0) {
        return DATA_TYPE_UNKNOWN;
    }

    DataType flags = type->datatypeBitflag & ~DATA_TYPE_ALIAS;
    const TypeDescriptorAliasNode *alias = type->aliases;
    while (alias != NULL) {
        const AbstractSyntaxNode *declaration = findTypeDeclaration(program, alias->name, ABSTRACT_SYNTAX_TYPE_ALIAS);
        if (declaration != NULL) {
            flags |= resolvedDataTypes(program, &declaration->type, remaining - 1);
        } else if (findTypeDeclaration(program, alias->name, ABSTRACT_SYNTAX_CASE_CLASS) != NULL) {
            flags |= DATA_TYPE_CASE_CLASS;
        }
        alias = alias->next;
    }

    return flags;
}

/**
 * @brief generates code to convert a Boolean to string saying "true" or "false"
 * @param generator generator state
 * @param resultOperand destination operand
 * @param valueOperand value to convert
 * @param origin source location
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateBooleanString(IntermediateGenerator *generator, const IntermediateOperand *resultOperand, const IntermediateOperand *valueOperand, SourceLocation origin) {
    IntermediateOperand endLabel = {0};
    CompilerStatus status = generateLabel(generator, &endLabel);
    if (status != COMPILER_OK) {
        return status;
    }

    IntermediateOperand trueOperand = {.kind = INTERMEDIATE_OPERAND_BOOLEAN, .value.boolValue = true};
    IntermediateOperand trueText = {.kind = INTERMEDIATE_OPERAND_STRING, .text = "true"};
    IntermediateOperand falseText = {.kind = INTERMEDIATE_OPERAND_STRING, .text = "false"};

    IntermediateInstruction trueInstruction = fillInstruction(INTERMEDIATE_MOVE, 2, origin);
    trueInstruction.operands[0] = *resultOperand;
    trueInstruction.operands[1] = trueText;

    IntermediateInstruction jumpInstruction = fillInstruction(INTERMEDIATE_JUMP_IF_EQUAL, 3, origin);
    jumpInstruction.operands[0] = endLabel;
    jumpInstruction.operands[1] = *valueOperand;
    jumpInstruction.operands[2] = trueOperand;

    IntermediateInstruction falseInstruction = fillInstruction(INTERMEDIATE_MOVE, 2, origin);
    falseInstruction.operands[0] = *resultOperand;
    falseInstruction.operands[1] = falseText;

    IntermediateInstruction endInstruction = fillInstruction(INTERMEDIATE_LABEL, 1, origin);
    endInstruction.operands[0] = endLabel;

    IntermediateInstruction instructions[] = {trueInstruction, jumpInstruction, falseInstruction, endInstruction};
    status = appendInstructionArray(generator, instructions, sizeof(instructions) / sizeof(instructions[0]));
    intermediateOperandClear(&endLabel);

    return status;
}

/**
 * @brief builds instruction from opcode and its operands
 * @param opcode intermediate opcode
 * @param origin source location
 * @param operandCount number of operands
 * @param operands operands
 * @return initialized instruction
 */
static IntermediateInstruction controlInstruction(IntermediateOpcode opcode, SourceLocation origin, size_t operandCount, const IntermediateOperand *operands) {
    IntermediateInstruction instruction = fillInstruction(opcode, operandCount, origin);

    size_t counter = 0;
    while (counter < operandCount) {
        instruction.operands[counter] = operands[counter];
        counter++;
    }

    return instruction;
}

/**
 * @brief builds and appends instruction with the given operands
 * @param generator generator state
 * @param opcode intermediate opcode
 * @param origin source location
 * @param operandCount number of operands
 * @param operands operands
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus appendControlInstruction(IntermediateGenerator *generator, IntermediateOpcode opcode, SourceLocation origin, size_t operandCount, const IntermediateOperand *operands) {
    IntermediateInstruction instruction = controlInstruction(opcode, origin, operandCount, operands);

    return intermediateProgramAppend(generator->program, &instruction);
}

/*
* AI-generated code
* could you please give me the necessary remaining construction for this part so that i can finish it faster
*/
/**
 * @brief generates code for integer division that rounds towards zero
 * @param generator generator state
 * @param resultOperand destination operand
 * @param leftOperand left operand
 * @param rightOperand right operand
 * @param origin source location
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateIntegerDivision(IntermediateGenerator *generator, const IntermediateOperand *resultOperand, const IntermediateOperand *leftOperand, const IntermediateOperand *rightOperand, SourceLocation origin) {
    if (generator == NULL || resultOperand == NULL || leftOperand == NULL || rightOperand == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    IntermediateOperand remainder = {0};
    IntermediateOperand remainderNegative = {0};
    IntermediateOperand dividendNegative = {0};
    IntermediateOperand end = {0};
    IntermediateOperand *ownedOperands[] = {&remainder, &remainderNegative, &dividendNegative, &end};
    size_t ownedOperandCount = sizeof(ownedOperands) / sizeof(ownedOperands[0]);

    CompilerStatus status = generateTemporary(generator, &remainder, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(generator, &remainderNegative, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(generator, &dividendNegative, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateLabel(generator, &end);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    IntermediateOperand zero = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 0};
    IntermediateOperand one = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 1};
    IntermediateOperand no = {.kind = INTERMEDIATE_OPERAND_BOOLEAN, .value.boolValue = false};

    status = appendControlInstruction(generator, INTERMEDIATE_INTEGER_DIVIDE, origin, 3, (IntermediateOperand[]){*resultOperand, *leftOperand, *rightOperand});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_COMPARE_LESS, origin, 3, (IntermediateOperand[]){remainderNegative, *resultOperand, zero});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){end, remainderNegative, no});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){remainder, *resultOperand, one});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_MULTIPLY, origin, 3, (IntermediateOperand[]){remainder, remainder, *rightOperand});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_SUBTRACT, origin, 3, (IntermediateOperand[]){remainder, *leftOperand, remainder});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){remainder, remainder, *rightOperand});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){end, remainder, zero});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_COMPARE_LESS, origin, 3, (IntermediateOperand[]){remainderNegative, remainder, zero});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_COMPARE_LESS, origin, 3, (IntermediateOperand[]){dividendNegative, *leftOperand, zero});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){end, remainderNegative, dividendNegative});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){*resultOperand, *resultOperand, one});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &end);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    clearOperandPointers(ownedOperands, ownedOperandCount);

    return COMPILER_OK;
}

/*
* AI-generated code
* could you please create me the skeleton for the class jump 
*/
/**
 * @brief generates code to jump if the subject has the expected array tag
 * @param generator generator state
 * @param subject value being matched
 * @param runtimeType runtime type operand
 * @param className expected class tag
 * @param matchedLabel label for a successful match
 * @param origin source location
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateClassJump(IntermediateGenerator *generator, const IntermediateOperand *subject, const IntermediateOperand *runtimeType, const char *className, const IntermediateOperand *matchedLabel, SourceLocation origin) {
    IntermediateOperand tag = {0};
    IntermediateOperand skipLabel = {0};
    IntermediateOperand *ownedOperands[] = {&tag, &skipLabel};
    size_t ownedOperandCount = sizeof(ownedOperands) / sizeof(ownedOperands[0]);

    CompilerStatus status = generateTemporary(generator, &tag, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateLabel(generator, &skipLabel);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    IntermediateOperand arrayName = {.kind = INTERMEDIATE_OPERAND_STRING, .text = "array"};
    IntermediateOperand classTag = {.kind = INTERMEDIATE_OPERAND_STRING, .text = (char *)className};
    IntermediateOperand zero = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 0};
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_NOT_EQUAL, origin, 3, (IntermediateOperand[]){skipLabel, *runtimeType, arrayName});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_ARRAY_GET, origin, 3, (IntermediateOperand[]){tag, *subject, zero});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){*matchedLabel, tag, classTag});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &skipLabel);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    clearOperandPointers(ownedOperands, ownedOperandCount);
    return COMPILER_OK;
}

/**
 * @brief resolves type flags for descriptor in the current source program
 * @param generator generator state
 * @param type type descriptor
 * @return resolved type flags
 */
static DataType valueTypes(IntermediateGenerator *generator, const TypeDescriptor *type) {
    return resolvedDataTypes(generator->sourceProgram, type, generator->sourceProgram->root->childCount + 1);
}

static const struct {
    DataType type;
    const char *name;
} runtimeTypes[] = {
    {DATA_TYPE_INT, "int"}, {DATA_TYPE_DOUBLE, "float"}, {DATA_TYPE_STRING, "string"}, {DATA_TYPE_BOOLEAN, "bool"}, {DATA_TYPE_UNIT, "nil"}, {DATA_TYPE_LIST, "array"},
};

/**
 * @brief generates a conditional jump that checks runtime type name
 * @param generator generator state
 * @param opcode intermediate opcode
 * @param label jump target
 * @param typeOperand runtime type operand
 * @param typeName expected runtime type name
 * @param origin source location
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus appendTypeJump(IntermediateGenerator *generator, IntermediateOpcode opcode, const IntermediateOperand *label, const IntermediateOperand *typeOperand, const char *typeName, SourceLocation origin) {
    IntermediateOperand name = {.kind = INTERMEDIATE_OPERAND_STRING, .text = (char *)typeName};
    return appendControlInstruction(generator, opcode, origin, 3, (IntermediateOperand[]){*label, *typeOperand, name});
}

/**
 * @brief checks if given flags represent supported value types
 * @param type type flags to check
 * @return true or false
 */
static bool isValueType(DataType type) {
    DataType supported = DATA_TYPE_INT | DATA_TYPE_DOUBLE | DATA_TYPE_STRING | DATA_TYPE_UNIT | DATA_TYPE_BOOLEAN | DATA_TYPE_CASE_CLASS | DATA_TYPE_LIST;
    return type != DATA_TYPE_UNKNOWN && (type & supported) == type;
}

/**
 * @brief checks if the destination accepts the values as-is or after converting from Int to Double
 * @param from source type flags
 * @param to destination type flags
 * @return true or false
 */
static bool canConvertValue(DataType from, DataType to) {
    if (!isValueType(from) || !isValueType(to)) {
        return false;
    }
    if ((from & to) == from) {
        return true;
    }

    return (to & DATA_TYPE_DOUBLE) && (from & DATA_TYPE_INT) && ((from & ~DATA_TYPE_INT) & to) == (from & ~DATA_TYPE_INT);
}

/*
* AI-generated code
* ok can you write the convert value function, should be really simple, just checking compatibility, leave as is if accepted, resute other convert functon for int, do the union correctly with all scenarios considered, will check afterwards
*/
/**
 * @brief generates code to convert Int to Double when needed and check union types at runtime
 * @param generator generator state
 * @param operand intermediate operand
 * @param from source type flags
 * @param to destination type flags
 * @param origin source location
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus convertValue(IntermediateGenerator *generator, IntermediateOperand *operand, DataType from, DataType to, SourceLocation origin) {
    if (!canConvertValue(from, to)) {
        return COMPILER_INTERNAL_ERROR;
    }
    if ((from & to) == from) {
        return COMPILER_OK;
    }

    if (from == DATA_TYPE_INT) {
        return convertIntToDouble(generator, operand, origin);
    }

    IntermediateOperand result = {0};
    IntermediateOperand type = {0};
    IntermediateOperand end = {0};
    IntermediateOperand *ownedOperands[] = {&result, &type, &end};
    size_t ownedOperandCount = sizeof(ownedOperands) / sizeof(ownedOperands[0]);

    CompilerStatus status = generateTemporary(generator, &result, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(generator, &type, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateLabel(generator, &end);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    IntermediateOperand intName = {.kind = INTERMEDIATE_OPERAND_STRING, .text = "int"};
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){result, *operand});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_GET_TYPE, origin, 2, (IntermediateOperand[]){type, *operand});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_NOT_EQUAL, origin, 3, (IntermediateOperand[]){end, type, intName});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_CONVERT_INT_TO_DOUBLE, origin, 2, (IntermediateOperand[]){result, *operand});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &end);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    intermediateOperandClear(operand);
    *operand = result;
    result.text = NULL;
    clearOperandPointers(ownedOperands, ownedOperandCount);

    return COMPILER_OK;
}

/**
 * @brief generates jumps to check if the value matches a typed pattern
 * @param generator generator state
 * @param type type descriptor
 * @param subject value being matched
 * @param runtimeType runtime type operand
 * @param matchedLabel label for a successful match
 * @param origin source location
 * @param remaining remaining recursion depth
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateTypedPatternJumps(IntermediateGenerator *generator, const TypeDescriptor *type, const IntermediateOperand *subject, const IntermediateOperand *runtimeType, const IntermediateOperand *matchedLabel, SourceLocation origin, size_t remaining) {
    if (remaining == 0) {
        return COMPILER_INTERNAL_ERROR;
    }

    size_t counter = 0;
    while (counter < sizeof(runtimeTypes) / sizeof(runtimeTypes[0])) {
        if (runtimeTypes[counter].type != DATA_TYPE_LIST && (type->datatypeBitflag & runtimeTypes[counter].type)) {
            CompilerStatus status = appendTypeJump(generator, INTERMEDIATE_JUMP_IF_EQUAL, matchedLabel, runtimeType, runtimeTypes[counter].name, origin);
            if (status != COMPILER_OK) {
                return status;
            }
        }
        counter++;
    }
    if (type->datatypeBitflag & DATA_TYPE_LIST) {
        CompilerStatus status = generateClassJump(generator, subject, runtimeType, "$List", matchedLabel, origin);
        if (status != COMPILER_OK) {
            return status;
        }
    }

    const TypeDescriptorAliasNode *name = type->classes;
    while (name != NULL) {
        CompilerStatus status = generateClassJump(generator, subject, runtimeType, name->name, matchedLabel, origin);
        if (status != COMPILER_OK) {
            return status;
        }
        name = name->next;
    }

    name = type->aliases;
    while (name != NULL) {
        const AbstractSyntaxNode *alias = findTypeDeclaration(generator->sourceProgram, name->name, ABSTRACT_SYNTAX_TYPE_ALIAS);
        CompilerStatus status;
        if (alias != NULL) {
            status = generateTypedPatternJumps(generator, &alias->type, subject, runtimeType, matchedLabel, origin, remaining - 1);
        } else if (findTypeDeclaration(generator->sourceProgram, name->name, ABSTRACT_SYNTAX_CASE_CLASS) != NULL) {
            status = generateClassJump(generator, subject, runtimeType, name->name, matchedLabel, origin);
        } else {
            return COMPILER_INTERNAL_ERROR;
        }
        if (status != COMPILER_OK) {
            return status;
        }
        name = name->next;
    }

    return COMPILER_OK;
}

/**
 * @brief assigns the matched value or one of its fields to the pattern variable
 * @param generator generator state
 * @param pattern pattern AST node
 * @param subject value being matched
 * @param fieldIndex field index, or zero to bind the whole subject
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus bindPatternValue(IntermediateGenerator *generator, const AbstractSyntaxNode *pattern, const IntermediateOperand *subject, size_t fieldIndex) {
    if (pattern->kind != ABSTRACT_SYNTAX_IDENTIFIER || pattern->text == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    if (strcmp(pattern->text, "_") == 0) {
        return COMPILER_OK;
    }

    IntermediateOperand binding = {0};
    CompilerStatus status = generateVariable(&binding, pattern);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendVariableDeclaration(generator, &binding, pattern->location);
    if (status == COMPILER_OK) {
        if (fieldIndex == 0) {
            status = appendControlInstruction(generator, INTERMEDIATE_MOVE, pattern->location, 2, (IntermediateOperand[]){binding, *subject});
        } else {
            IntermediateOperand index = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = (int64_t)fieldIndex};
            status = appendControlInstruction(generator, INTERMEDIATE_ARRAY_GET, pattern->location, 3, (IntermediateOperand[]){binding, *subject, index});
        }
    }
    intermediateOperandClear(&binding);

    return status;
}

static CompilerStatus generateListString(IntermediateGenerator *generator, const IntermediateOperand *result, const IntermediateOperand *value, SourceLocation origin);

/*
* AI-generated code
* could you please make me a skeleton for the instructions that will turn some given value into a string even tho the original value type is not necessarilly known
*/
/**
 * @brief generates code to convert a value to string based on its runtime type
 * @param generator generator state
 * @param result destination operand
 * @param value value operand
 * @param types supported type flags
 * @param origin source location
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateUnionString(IntermediateGenerator *generator, IntermediateOperand *result, const IntermediateOperand *value, DataType types, SourceLocation origin) {
    IntermediateOperand type = {0};
    IntermediateOperand endLabel = {0};
    IntermediateOperand nextLabel = {0};
    IntermediateOperand *ownedOperands[] = {&type, &endLabel, &nextLabel};
    size_t ownedOperandCount = sizeof(ownedOperands) / sizeof(ownedOperands[0]);

    CompilerStatus status = generateTemporary(generator, &type, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateLabel(generator, &endLabel);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_GET_TYPE, origin, 2, (IntermediateOperand[]){type, *value});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    size_t counter = 0;
    while (counter < sizeof(runtimeTypes) / sizeof(runtimeTypes[0])) {
        DataType current = runtimeTypes[counter].type;
        if (!(types & current)) {
            counter++;
            continue;
        }

        status = generateLabel(generator, &nextLabel);
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }
        status = appendTypeJump(generator, INTERMEDIATE_JUMP_IF_NOT_EQUAL, &nextLabel, &type, runtimeTypes[counter].name, origin);
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }

        if (current == DATA_TYPE_LIST) {
            status = generateListString(generator, result, value, origin);
        } else if (current == DATA_TYPE_BOOLEAN) {
            status = generateBooleanString(generator, result, value, origin);
        } else if (current == DATA_TYPE_UNIT) {
            IntermediateOperand unit = {.kind = INTERMEDIATE_OPERAND_STRING, .text = "()"};
            status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){*result, unit});
        } else {
            IntermediateOpcode opcode = INTERMEDIATE_MOVE;
            if (current == DATA_TYPE_INT) {
                opcode = INTERMEDIATE_CONVERT_INT_TO_STRING;
            } else if (current == DATA_TYPE_DOUBLE) {
                opcode = INTERMEDIATE_CONVERT_DOUBLE_TO_STRING;
            }
            status = appendControlInstruction(generator, opcode, origin, 2, (IntermediateOperand[]){*result, *value});
        }
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }

        status = appendControlInstruction(generator, INTERMEDIATE_JUMP, origin, 1, &endLabel);
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }

        status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &nextLabel);
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }
        intermediateOperandClear(&nextLabel);
        counter++;
    }

    IntermediateOperand error = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 40};
    status = appendControlInstruction(generator, INTERMEDIATE_EXIT, origin, 1, &error);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &endLabel);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    clearOperandPointers(ownedOperands, ownedOperandCount);
    return COMPILER_OK;
}

/*
* AI-generated code
* ok, could you now please make me the blueprint for the fun tht makes the print(val) instructions
*/
/**
 * @brief generates code to print a value according to its runtime type
 * @param generator generator state
 * @param value value operand
 * @param type supported type flags
 * @param origin source location
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateUnionPrint(IntermediateGenerator *generator, const IntermediateOperand *value, DataType type, SourceLocation origin) {
    if (type & DATA_TYPE_LIST) {
        IntermediateOperand translated = {0};
        IntermediateOperand runtimeType = {0};
        IntermediateOperand readyLabel = {0};
        IntermediateOperand *ownedOperands[] = {&translated, &runtimeType, &readyLabel};
        size_t ownedOperandCount = sizeof(ownedOperands) / sizeof(ownedOperands[0]);

        CompilerStatus status = generateTemporary(generator, &translated, origin);
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }

        status = generateTemporary(generator, &runtimeType, origin);
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }

        status = generateLabel(generator, &readyLabel);
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }

        IntermediateOperand arrayType = {.kind = INTERMEDIATE_OPERAND_STRING, .text = "array"};
        status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){translated, *value});
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }

        status = appendControlInstruction(generator, INTERMEDIATE_GET_TYPE, origin, 2, (IntermediateOperand[]){runtimeType, *value});
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }

        status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_NOT_EQUAL, origin, 3, (IntermediateOperand[]){readyLabel, runtimeType, arrayType});
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }

        status = generateListString(generator, &translated, value, origin);
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }

        status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &readyLabel);
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }

        status = generateUnionPrint(generator, &translated, type & ~DATA_TYPE_LIST, origin);
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return COMPILER_OK;
    }

    if (!(type & DATA_TYPE_UNIT)) {
        return appendControlInstruction(generator, INTERMEDIATE_WRITE, origin, 1, value);
    }

    IntermediateOperand valueLabel = {0};
    IntermediateOperand endLabel = {0};
    IntermediateOperand *ownedOperands[] = {&valueLabel, &endLabel};
    size_t ownedOperandCount = sizeof(ownedOperands) / sizeof(ownedOperands[0]);

    CompilerStatus status = generateLabel(generator, &valueLabel);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateLabel(generator, &endLabel);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    IntermediateOperand unit = {.kind = INTERMEDIATE_OPERAND_STRING, .text = "()"};
    IntermediateOperand nil = {.kind = INTERMEDIATE_OPERAND_UNIT};
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_NOT_EQUAL, origin, 3, (IntermediateOperand[]){valueLabel, *value, nil});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_WRITE, origin, 1, &unit);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_JUMP, origin, 1, &endLabel);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &valueLabel);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_WRITE, origin, 1, value);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &endLabel);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    clearOperandPointers(ownedOperands, ownedOperandCount);
    return COMPILER_OK;
}

/*
* AI-generated code
* please make me a function skelet which runs after the generated read for reading ints and doubles and replaces nils with either 0 or 0.0
*/
/**
 * @brief generates code to use zero if numeric read fails or integer is out of range
 * @param generator generator state
 * @param result destination operand
 * @param type numeric input type
 * @param origin source location
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateNumericReadDefault(IntermediateGenerator *generator, const IntermediateOperand *result, DataType type, SourceLocation origin) {
    IntermediateOperand end = {0};
    IntermediateOperand useDefault = {0};
    IntermediateOperand outside = {0};
    IntermediateOperand *ownedOperands[] = {&end, &useDefault, &outside};
    size_t ownedOperandCount = sizeof(ownedOperands) / sizeof(ownedOperands[0]);

    CompilerStatus status = generateLabel(generator, &end);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    IntermediateOperand nil = {.kind = INTERMEDIATE_OPERAND_UNIT};
    IntermediateOperand zero = {.kind = INTERMEDIATE_OPERAND_DOUBLE, .value.doubleValue = 0.0};
    if (type == DATA_TYPE_INT) {
        status = generateLabel(generator, &useDefault);
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }

        status = generateTemporary(generator, &outside, origin);
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }

        zero = (IntermediateOperand){.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 0};
        IntermediateOperand minimum = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = INT32_MIN};
        IntermediateOperand maximum = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = INT32_MAX};
        IntermediateOperand yes = {.kind = INTERMEDIATE_OPERAND_BOOLEAN, .value.boolValue = true};

        status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){useDefault, *result, nil});
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }

        status = appendControlInstruction(generator, INTERMEDIATE_COMPARE_LESS, origin, 3, (IntermediateOperand[]){outside, *result, minimum});
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }

        status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){useDefault, outside, yes});
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }

        status = appendControlInstruction(generator, INTERMEDIATE_COMPARE_GREATER, origin, 3, (IntermediateOperand[]){outside, *result, maximum});
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }

        status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_NOT_EQUAL, origin, 3, (IntermediateOperand[]){end, outside, yes});
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }

        status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &useDefault);
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }
    } else {
        status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_NOT_EQUAL, origin, 3, (IntermediateOperand[]){end, *result, nil});
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }
    }

    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){*result, zero});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &end);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    clearOperandPointers(ownedOperands, ownedOperandCount);
    return COMPILER_OK;
}

/*
* AI-generated code
* ok, we will need to implement the maybe reads for int and double datatypes, could you please make me the common ancestor function prototype for those so i can reuse at least the first step of them in one function
*/
/**
 * @brief generates code to read a line and handle an optional sign
 * @param generator generator state
 * @param result destination operand
 * @param origin source location
 * @param variables temporary operands
 * @param labels control-flow labels
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateOptionalReadInput(IntermediateGenerator *generator, const IntermediateOperand *result, SourceLocation origin, const IntermediateOperand *variables, const IntermediateOperand *labels) {
    IntermediateOperand line = variables[0];
    IntermediateOperand length = variables[1];
    IntermediateOperand index = variables[2];
    IntermediateOperand character = variables[3];
    IntermediateOperand negative = variables[4];
    IntermediateOperand noNewline = labels[0];
    IntermediateOperand afterSign = labels[1];
    IntermediateOperand digitsStart = labels[2];
    IntermediateOperand end = labels[3];

    IntermediateOperand stringType = {.kind = INTERMEDIATE_OPERAND_TYPE, .value.dataType = DATA_TYPE_STRING};
    IntermediateOperand unit = {.kind = INTERMEDIATE_OPERAND_UNIT};
    IntermediateOperand zero = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 0};
    IntermediateOperand one = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 1};
    IntermediateOperand newline = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = '\n'};
    IntermediateOperand minus = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = '-'};
    IntermediateOperand plus = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = '+'};
    IntermediateOperand trueValue = {.kind = INTERMEDIATE_OPERAND_BOOLEAN, .value.boolValue = true};

    CompilerStatus status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){*result, unit});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_READ, origin, 2, (IntermediateOperand[]){line, stringType});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_STRING_LENGTH, origin, 2, (IntermediateOperand[]){length, line});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){end, length, zero});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_SUBTRACT, origin, 3, (IntermediateOperand[]){index, length, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_STRING_CHARACTER_CODE, origin, 3, (IntermediateOperand[]){character, line, index});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_NOT_EQUAL, origin, 3, (IntermediateOperand[]){noNewline, character, newline});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){length, index});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){end, length, zero});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &noNewline);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){index, zero});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_STRING_CHARACTER_CODE, origin, 3, (IntermediateOperand[]){character, line, index});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_COMPARE_EQUAL, origin, 3, (IntermediateOperand[]){negative, character, minus});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){afterSign, negative, trueValue});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_NOT_EQUAL, origin, 3, (IntermediateOperand[]){digitsStart, character, plus});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &afterSign);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){index, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){end, index, length});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &digitsStart);
    if (status != COMPILER_OK) {
        return status;
    }

    return COMPILER_OK;
}

/*
 * AI-generated code
 * so, i believe that generateReadIntMaybeBody will be kind of similar to the other maybe functions so could you once again please write me the skeleton and necessary vars in there
 */
/**
 * @brief generates code for readIntMaybe to parse integer and check its range
 * @param generator generator state
 * @param result destination operand
 * @param origin source location
 * @param variables temporary operands
 * @param labels control-flow labels
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateReadIntMaybeBody(IntermediateGenerator *generator, const IntermediateOperand *result, SourceLocation origin, const IntermediateOperand *variables, const IntermediateOperand *labels) {
    IntermediateOperand line = variables[0];
    IntermediateOperand length = variables[1];
    IntermediateOperand index = variables[2];
    IntermediateOperand digit = variables[3];
    IntermediateOperand negative = variables[4];
    IntermediateOperand number = variables[5];
    IntermediateOperand condition = variables[6];

    IntermediateOperand loop = labels[4];
    IntermediateOperand finish = labels[5];
    IntermediateOperand store = labels[6];
    IntermediateOperand end = labels[3];

    IntermediateOperand zero = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 0};
    IntermediateOperand one = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 1};
    IntermediateOperand ten = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 10};
    IntermediateOperand firstDigit = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = '0'};
    IntermediateOperand lastDigit = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = '9'};
    IntermediateOperand trueValue = {.kind = INTERMEDIATE_OPERAND_BOOLEAN, .value.boolValue = true};
    IntermediateOperand minValue = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = INT32_MIN};

    CompilerStatus status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){number, zero});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &loop);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){finish, index, length});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_STRING_CHARACTER_CODE, origin, 3, (IntermediateOperand[]){digit, line, index});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_COMPARE_LESS, origin, 3, (IntermediateOperand[]){condition, digit, firstDigit});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){end, condition, trueValue});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_COMPARE_LESS, origin, 3, (IntermediateOperand[]){condition, lastDigit, digit});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){end, condition, trueValue});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_SUBTRACT, origin, 3, (IntermediateOperand[]){digit, digit, firstDigit});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MULTIPLY, origin, 3, (IntermediateOperand[]){number, number, ten});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_SUBTRACT, origin, 3, (IntermediateOperand[]){number, number, digit});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_COMPARE_LESS, origin, 3, (IntermediateOperand[]){condition, number, minValue});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){end, condition, trueValue});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){index, index, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP, origin, 1, &loop);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &finish);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){store, negative, trueValue});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){end, number, minValue});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_SUBTRACT, origin, 3, (IntermediateOperand[]){number, zero, number});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &store);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){*result, number});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &end);
    if (status != COMPILER_OK) {
        return status;
    }

    return COMPILER_OK;
}

/*
 * AI-generated code
 * ok could you please create the skeleton for generateReadDoubleMaybeBody fun so i can go in and just finish the logic in there pls
 */
/**
 * @brief generates code for readDoubleMaybe to parse and round floating-point number
 * @param generator generator state
 * @param result destination operand
 * @param origin source location
 * @param variables temporary operands
 * @param labels control-flow labels
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateReadDoubleMaybeBody(IntermediateGenerator *generator, const IntermediateOperand *result, SourceLocation origin, const IntermediateOperand *variables, const IntermediateOperand *labels) {
    IntermediateOperand line = variables[0];
    IntermediateOperand length = variables[1];
    IntermediateOperand index = variables[2];
    IntermediateOperand ch = variables[3];
    IntermediateOperand negative = variables[4];
    IntermediateOperand base = variables[5];
    IntermediateOperand digit = variables[6];
    IntermediateOperand exponent = variables[7];
    IntermediateOperand seen = variables[8];
    IntermediateOperand dotted = variables[9];
    IntermediateOperand digits = variables[10];
    IntermediateOperand value = variables[11];
    IntermediateOperand head = variables[12];
    IntermediateOperand tail = variables[13];
    IntermediateOperand point = variables[14];
    IntermediateOperand exponentLimit = variables[15];
    IntermediateOperand condition = variables[16];
    IntermediateOperand exponentNegative = variables[17];
    IntermediateOperand binaryExponent = variables[18];
    IntermediateOperand extracting = variables[19];
    IntermediateOperand carry = variables[20];
    IntermediateOperand cursor = variables[21];
    IntermediateOperand quotient = variables[22];
    IntermediateOperand significand = variables[23];
    IntermediateOperand bitCount = variables[24];
    IntermediateOperand weight = variables[25];

    IntermediateOperand end = labels[3];
    IntermediateOperand hex = labels[4];
    IntermediateOperand init = labels[5];
    IntermediateOperand scan = labels[6];
    IntermediateOperand checkExponent = labels[7];
    IntermediateOperand hexExponent = labels[8];
    IntermediateOperand readDigit = labels[9];
    IntermediateOperand letter = labels[10];
    IntermediateOperand upperLetter = labels[11];
    IntermediateOperand checkLetter = labels[12];
    IntermediateOperand accumulate = labels[13];
    IntermediateOperand keepDigit = labels[14];
    IntermediateOperand saveDigit = labels[15];
    IntermediateOperand consume = labels[16];
    IntermediateOperand exponentStart = labels[17];
    IntermediateOperand exponentSigned = labels[18];
    IntermediateOperand exponentLoop = labels[19];
    IntermediateOperand exponentAdvance = labels[20];
    IntermediateOperand exponentDigits = labels[21];
    IntermediateOperand combine = labels[22];
    IntermediateOperand hexOffset = labels[23];
    IntermediateOperand normalize = labels[24];
    IntermediateOperand normalizeUp = labels[25];
    IntermediateOperand normalizeDown = labels[26];
    IntermediateOperand divideStart = labels[27];
    IntermediateOperand divideLoop = labels[28];
    IntermediateOperand divideDone = labels[29];
    IntermediateOperand divideTrim = labels[30];
    IntermediateOperand divideNext = labels[31];
    IntermediateOperand normalized = labels[32];
    IntermediateOperand removeLeadingBit = labels[33];
    IntermediateOperand roundLoop = labels[34];
    IntermediateOperand bitReady = labels[35];
    IntermediateOperand roundGuard = labels[36];
    IntermediateOperand stickyLoop = labels[37];
    IntermediateOperand roundUp = labels[38];
    IntermediateOperand buildValue = labels[39];
    IntermediateOperand convert = labels[40];
    IntermediateOperand applyScale = labels[41];
    IntermediateOperand scaleDown = labels[42];
    IntermediateOperand signedResult = labels[43];
    IntermediateOperand store = labels[44];
    IntermediateOperand multiplyStart = labels[45];
    IntermediateOperand multiplyLoop = labels[46];
    IntermediateOperand multiplyDone = labels[47];

    IntermediateOperand zero = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 0};
    IntermediateOperand one = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 1};
    IntermediateOperand four = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 4};
    IntermediateOperand five = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 5};
    IntermediateOperand nine = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 9};
    IntermediateOperand ten = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 10};
    IntermediateOperand sixteen = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 16};
    IntermediateOperand minus = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = '-'};
    IntermediateOperand plus = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = '+'};
    IntermediateOperand dot = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = '.'};
    IntermediateOperand d0 = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = '0'};
    IntermediateOperand d9 = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = '9'};
    IntermediateOperand a = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 'a'};
    IntermediateOperand A = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 'A'};
    IntermediateOperand e = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 'e'};
    IntermediateOperand E = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 'E'};
    IntermediateOperand p = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 'p'};
    IntermediateOperand P = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 'P'};
    IntermediateOperand x = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 'x'};
    IntermediateOperand X = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 'X'};
    IntermediateOperand floatZero = {.kind = INTERMEDIATE_OPERAND_DOUBLE, .value.doubleValue = 0.0};
    IntermediateOperand negativeOne = {.kind = INTERMEDIATE_OPERAND_DOUBLE, .value.doubleValue = -1.0};
    IntermediateOperand twoFloat = {.kind = INTERMEDIATE_OPERAND_DOUBLE, .value.doubleValue = 2.0};
    IntermediateOperand yes = {.kind = INTERMEDIATE_OPERAND_BOOLEAN, .value.boolValue = true};
    IntermediateOperand no = {.kind = INTERMEDIATE_OPERAND_BOOLEAN, .value.boolValue = false};
    IntermediateOperand extraSpace = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 2200};
    IntermediateOperand headroom = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 1080};
    IntermediateOperand maxDecimalPoint = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 309};
    IntermediateOperand minDecimalPoint = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = -323};
    IntermediateOperand two = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 2};
    IntermediateOperand maximumExponent = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 1023};
    IntermediateOperand halfMinimumExponent = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = -1075};
    IntermediateOperand minimumNormalExponent = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = -1022};
    IntermediateOperand subnormalOffset = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 1074};
    IntermediateOperand fractionBits = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 52};
    IntermediateOperand carryLimit = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = INT64_C(9007199254740992)};

    // initialize the base and digit buffer
    CompilerStatus status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){base, ten});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_STRING_CHARACTER_CODE, origin, 3, (IntermediateOperand[]){ch, line, index});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_NOT_EQUAL, origin, 3, (IntermediateOperand[]){init, ch, d0});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){digit, index, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){init, digit, length});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_STRING_CHARACTER_CODE, origin, 3, (IntermediateOperand[]){ch, line, digit});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){hex, ch, x});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_NOT_EQUAL, origin, 3, (IntermediateOperand[]){init, ch, X});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &hex);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){base, sixteen});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){index, digit, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &init);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){exponent, zero});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){seen, no});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){dotted, no});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){digit, length, extraSpace});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ARRAY_CREATE, origin, 2, (IntermediateOperand[]){digits, digit});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){value, floatZero});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){head, headroom});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){tail, head});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){point, zero});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MULTIPLY, origin, 3, (IntermediateOperand[]){exponentLimit, length, four});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){exponentLimit, exponentLimit, extraSpace});
    if (status != COMPILER_OK) {
        return status;
    }

    // scan the mantissa
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &scan);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){combine, index, length});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_STRING_CHARACTER_CODE, origin, 3, (IntermediateOperand[]){ch, line, index});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_NOT_EQUAL, origin, 3, (IntermediateOperand[]){checkExponent, ch, dot});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){end, dotted, yes});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){dotted, yes});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP, origin, 1, &consume);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &checkExponent);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){hexExponent, base, sixteen});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){exponentStart, ch, e});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){exponentStart, ch, E});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP, origin, 1, &readDigit);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &hexExponent);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){exponentStart, ch, p});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){exponentStart, ch, P});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &readDigit);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_COMPARE_LESS, origin, 3, (IntermediateOperand[]){condition, ch, d0});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){end, condition, yes});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_COMPARE_GREATER, origin, 3, (IntermediateOperand[]){condition, ch, d9});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){letter, condition, yes});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_SUBTRACT, origin, 3, (IntermediateOperand[]){digit, ch, d0});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP, origin, 1, &accumulate);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &letter);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_NOT_EQUAL, origin, 3, (IntermediateOperand[]){end, base, sixteen});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_COMPARE_LESS, origin, 3, (IntermediateOperand[]){condition, ch, a});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){upperLetter, condition, yes});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_SUBTRACT, origin, 3, (IntermediateOperand[]){digit, ch, a});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP, origin, 1, &checkLetter);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &upperLetter);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_SUBTRACT, origin, 3, (IntermediateOperand[]){digit, ch, A});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &checkLetter);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_COMPARE_LESS, origin, 3, (IntermediateOperand[]){condition, digit, zero});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){end, condition, yes});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_COMPARE_GREATER, origin, 3, (IntermediateOperand[]){condition, digit, five});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){end, condition, yes});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){digit, digit, ten});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &accumulate);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){seen, yes});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){keepDigit, dotted, no});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_SUBTRACT, origin, 3, (IntermediateOperand[]){point, point, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &keepDigit);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_NOT_EQUAL, origin, 3, (IntermediateOperand[]){saveDigit, tail, head});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){consume, digit, zero});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &saveDigit);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ARRAY_SET, origin, 3, (IntermediateOperand[]){digits, tail, digit});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){tail, tail, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &consume);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){index, index, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP, origin, 1, &scan);
    if (status != COMPILER_OK) {
        return status;
    }

    // parse the exponent
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &exponentStart);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){end, seen, no});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){index, index, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){end, index, length});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_STRING_CHARACTER_CODE, origin, 3, (IntermediateOperand[]){ch, line, index});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_COMPARE_EQUAL, origin, 3, (IntermediateOperand[]){exponentNegative, ch, minus});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){exponentSigned, exponentNegative, yes});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_NOT_EQUAL, origin, 3, (IntermediateOperand[]){exponentLoop, ch, plus});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &exponentSigned);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){index, index, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){end, index, length});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &exponentLoop);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){exponentDigits, index, length});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_STRING_CHARACTER_CODE, origin, 3, (IntermediateOperand[]){digit, line, index});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_SUBTRACT, origin, 3, (IntermediateOperand[]){digit, digit, d0});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_COMPARE_LESS, origin, 3, (IntermediateOperand[]){condition, digit, zero});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){end, condition, yes});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_COMPARE_GREATER, origin, 3, (IntermediateOperand[]){condition, digit, nine});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){end, condition, yes});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_COMPARE_GREATER, origin, 3, (IntermediateOperand[]){condition, exponent, exponentLimit});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){exponentAdvance, condition, yes});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MULTIPLY, origin, 3, (IntermediateOperand[]){exponent, exponent, ten});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){exponent, exponent, digit});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &exponentAdvance);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){index, index, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP, origin, 1, &exponentLoop);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &exponentDigits);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){combine, exponentNegative, no});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_SUBTRACT, origin, 3, (IntermediateOperand[]){exponent, zero, exponent});
    if (status != COMPILER_OK) {
        return status;
    }

    // normalize and round the parsed value
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &combine);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){end, seen, no});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){signedResult, tail, head});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_SUBTRACT, origin, 3, (IntermediateOperand[]){digit, tail, head});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){point, point, digit});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){binaryExponent, zero});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){hexOffset, base, sixteen});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){point, point, exponent});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_COMPARE_GREATER, origin, 3, (IntermediateOperand[]){condition, point, maxDecimalPoint});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){end, condition, yes});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_COMPARE_LESS, origin, 3, (IntermediateOperand[]){condition, point, minDecimalPoint});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){end, condition, yes});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP, origin, 1, &normalize);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &hexOffset);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_SUBTRACT, origin, 3, (IntermediateOperand[]){binaryExponent, point, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MULTIPLY, origin, 3, (IntermediateOperand[]){binaryExponent, binaryExponent, four});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){binaryExponent, binaryExponent, exponent});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){point, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &normalize);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ARRAY_GET, origin, 3, (IntermediateOperand[]){digit, digits, head});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){normalizeDown, point, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_COMPARE_GREATER, origin, 3, (IntermediateOperand[]){condition, point, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){divideStart, condition, yes});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_SUBTRACT, origin, 3, (IntermediateOperand[]){binaryExponent, binaryExponent, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){extracting, no});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP, origin, 1, &multiplyStart);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &normalizeUp);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){normalize, carry, zero});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_SUBTRACT, origin, 3, (IntermediateOperand[]){head, head, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ARRAY_SET, origin, 3, (IntermediateOperand[]){digits, head, carry});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){point, point, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP, origin, 1, &normalize);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &normalizeDown);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){normalized, digit, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &divideStart);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){cursor, head});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){carry, zero});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &divideLoop);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){divideDone, cursor, tail});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ARRAY_GET, origin, 3, (IntermediateOperand[]){digit, digits, cursor});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MULTIPLY, origin, 3, (IntermediateOperand[]){carry, carry, base});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){digit, digit, carry});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_INTEGER_DIVIDE, origin, 3, (IntermediateOperand[]){quotient, digit, two});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ARRAY_SET, origin, 3, (IntermediateOperand[]){digits, cursor, quotient});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MULTIPLY, origin, 3, (IntermediateOperand[]){carry, quotient, two});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_SUBTRACT, origin, 3, (IntermediateOperand[]){carry, digit, carry});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){cursor, cursor, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP, origin, 1, &divideLoop);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &divideDone);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){divideTrim, carry, zero});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_INTEGER_DIVIDE, origin, 3, (IntermediateOperand[]){quotient, base, two});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ARRAY_SET, origin, 3, (IntermediateOperand[]){digits, tail, quotient});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){tail, tail, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &divideTrim);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ARRAY_GET, origin, 3, (IntermediateOperand[]){digit, digits, head});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_NOT_EQUAL, origin, 3, (IntermediateOperand[]){divideNext, digit, zero});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){head, head, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_SUBTRACT, origin, 3, (IntermediateOperand[]){point, point, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &divideNext);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){binaryExponent, binaryExponent, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP, origin, 1, &normalize);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &normalized);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_COMPARE_GREATER, origin, 3, (IntermediateOperand[]){condition, binaryExponent, maximumExponent});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){end, condition, yes});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_COMPARE_LESS, origin, 3, (IntermediateOperand[]){condition, binaryExponent, halfMinimumExponent});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){end, condition, yes});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){significand, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){bitCount, fractionBits});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){weight, binaryExponent});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_COMPARE_LESS, origin, 3, (IntermediateOperand[]){condition, binaryExponent, minimumNormalExponent});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){removeLeadingBit, condition, no});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){bitCount, binaryExponent, subnormalOffset});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){weight, minimumNormalExponent});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_NOT_EQUAL, origin, 3, (IntermediateOperand[]){removeLeadingBit, binaryExponent, halfMinimumExponent});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){significand, zero});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ARRAY_SET, origin, 3, (IntermediateOperand[]){digits, head, zero});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){carry, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP, origin, 1, &roundGuard);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &removeLeadingBit);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ARRAY_SET, origin, 3, (IntermediateOperand[]){digits, head, zero});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){head, head, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &roundLoop);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){extracting, yes});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP, origin, 1, &multiplyStart);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &bitReady);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){roundGuard, bitCount, zero});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MULTIPLY, origin, 3, (IntermediateOperand[]){significand, significand, two});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){significand, significand, carry});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_SUBTRACT, origin, 3, (IntermediateOperand[]){bitCount, bitCount, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP, origin, 1, &roundLoop);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &roundGuard);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){buildValue, carry, zero});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_INTEGER_DIVIDE, origin, 3, (IntermediateOperand[]){quotient, significand, two});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MULTIPLY, origin, 3, (IntermediateOperand[]){quotient, quotient, two});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_NOT_EQUAL, origin, 3, (IntermediateOperand[]){roundUp, quotient, significand});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){cursor, head});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &stickyLoop);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){buildValue, cursor, tail});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ARRAY_GET, origin, 3, (IntermediateOperand[]){digit, digits, cursor});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_NOT_EQUAL, origin, 3, (IntermediateOperand[]){roundUp, digit, zero});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){cursor, cursor, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP, origin, 1, &stickyLoop);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &roundUp);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){significand, significand, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &buildValue);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){end, significand, zero});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_NOT_EQUAL, origin, 3, (IntermediateOperand[]){convert, binaryExponent, maximumExponent});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){end, significand, carryLimit});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &convert);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_CONVERT_INT_TO_DOUBLE, origin, 2, (IntermediateOperand[]){value, significand});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_SUBTRACT, origin, 3, (IntermediateOperand[]){weight, weight, fractionBits});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &applyScale);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){signedResult, weight, zero});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_COMPARE_LESS, origin, 3, (IntermediateOperand[]){condition, weight, zero});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){scaleDown, condition, yes});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MULTIPLY, origin, 3, (IntermediateOperand[]){value, value, twoFloat});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_SUBTRACT, origin, 3, (IntermediateOperand[]){weight, weight, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP, origin, 1, &applyScale);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &scaleDown);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_DIVIDE, origin, 3, (IntermediateOperand[]){value, value, twoFloat});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){weight, weight, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP, origin, 1, &applyScale);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &signedResult);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){store, negative, no});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MULTIPLY, origin, 3, (IntermediateOperand[]){value, value, negativeOne});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &store);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){*result, value});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP, origin, 1, &end);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &multiplyStart);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){cursor, tail});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){carry, zero});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &multiplyLoop);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){multiplyDone, cursor, head});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_SUBTRACT, origin, 3, (IntermediateOperand[]){cursor, cursor, one});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ARRAY_GET, origin, 3, (IntermediateOperand[]){digit, digits, cursor});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MULTIPLY, origin, 3, (IntermediateOperand[]){digit, digit, two});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){digit, digit, carry});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_INTEGER_DIVIDE, origin, 3, (IntermediateOperand[]){carry, digit, base});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_MULTIPLY, origin, 3, (IntermediateOperand[]){quotient, carry, base});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_SUBTRACT, origin, 3, (IntermediateOperand[]){digit, digit, quotient});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_ARRAY_SET, origin, 3, (IntermediateOperand[]){digits, cursor, digit});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP, origin, 1, &multiplyLoop);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &multiplyDone);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){bitReady, extracting, yes});
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_JUMP, origin, 1, &normalizeUp);
    if (status != COMPILER_OK) {
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &end);
    if (status != COMPILER_OK) {
        return status;
    }

    return COMPILER_OK;
}

/**
 * @brief generates code to read optional Int or Double
 * @param generator generator state
 * @param operand intermediate operand
 * @param origin source location
 * @param isDouble whether to read a Double
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateOptionalRead(IntermediateGenerator *generator, IntermediateOperand *operand, SourceLocation origin, bool isDouble) {
    IntermediateOperand variables[26] = {0};
    IntermediateOperand labels[48] = {0};
    IntermediateOperand result = {0};
    CompilerStatus status = generateTemporary(generator, &result, origin);
    if (status != COMPILER_OK) {
        return status;
    }

    size_t variableCount = isDouble ? 26 : 7;
    size_t labelCount = isDouble ? 48 : 7;
    size_t counter = 0;
    while (counter < variableCount) {
        status = generateTemporary(generator, &variables[counter++], origin);
        if (status != COMPILER_OK) {
            clearOperandArray(variables, variableCount);
            intermediateOperandClear(&result);
            return status;
        }
    }

    counter = 0;
    while (counter < labelCount) {
        status = generateLabel(generator, &labels[counter++]);
        if (status != COMPILER_OK) {
            clearOperandArray(variables, variableCount);
            clearOperandArray(labels, labelCount);
            intermediateOperandClear(&result);
            return status;
        }
    }

    status = generateOptionalReadInput(generator, &result, origin, variables, labels);
    if (status == COMPILER_OK) {
        status = isDouble ? generateReadDoubleMaybeBody(generator, &result, origin, variables, labels) : generateReadIntMaybeBody(generator, &result, origin, variables, labels);
    }
    clearOperandArray(variables, variableCount);
    clearOperandArray(labels, labelCount);
    if (status != COMPILER_OK) {
        intermediateOperandClear(&result);
        return status;
    }

    *operand = result;
    return COMPILER_OK;
}

static CompilerStatus generateExpression(IntermediateGenerator *generator, IntermediateOperand *operand, const AbstractSyntaxNode *node);

/**
 * @brief generates code to check a match pattern and bind its variables
 * @param generator generator state
 * @param pattern pattern AST node
 * @param subject value being matched
 * @param runtimeType runtime type operand
 * @param nextLabel label for the next match case
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateMatchPattern(IntermediateGenerator *generator, const AbstractSyntaxNode *pattern, const IntermediateOperand *subject, const IntermediateOperand *runtimeType, const IntermediateOperand *nextLabel) {
    SourceLocation origin = pattern->location;
    if (pattern->kind == ABSTRACT_SYNTAX_IDENTIFIER && pattern->text != NULL && strcmp(pattern->text, "_") == 0) {
        return COMPILER_OK;
    }

    if (pattern->kind == ABSTRACT_SYNTAX_IDENTIFIER || pattern->kind == ABSTRACT_SYNTAX_CALL_EXPRESSION) {
        IntermediateOperand matchedLabel = {0};
        CompilerStatus status = generateLabel(generator, &matchedLabel);
        if (status != COMPILER_OK) {
            return status;
        }
        if (pattern->kind == ABSTRACT_SYNTAX_IDENTIFIER) {
            status = generateTypedPatternJumps(generator, &pattern->type, subject, runtimeType, &matchedLabel, origin, generator->sourceProgram->root->childCount + 1);
        } else if (pattern->childCount == 0 || pattern->children == NULL || pattern->children[0] == NULL || pattern->children[0]->text == NULL) {
            status = COMPILER_INTERNAL_ERROR;
        } else {
            const char *name = pattern->children[0]->text;
            const AbstractSyntaxNode *declaration = findTypeDeclaration(generator->sourceProgram, name, ABSTRACT_SYNTAX_CASE_CLASS);
            if (declaration == NULL || declaration->childCount != pattern->childCount - 1) {
                status = COMPILER_INTERNAL_ERROR;
            } else {
                status = generateClassJump(generator, subject, runtimeType, name, &matchedLabel, origin);
            }
        }
        if (status != COMPILER_OK) {
            intermediateOperandClear(&matchedLabel);
            return status;
        }

        status = appendControlInstruction(generator, INTERMEDIATE_JUMP, origin, 1, nextLabel);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&matchedLabel);
            return status;
        }

        status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &matchedLabel);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&matchedLabel);
            return status;
        }
        intermediateOperandClear(&matchedLabel);

        if (pattern->kind == ABSTRACT_SYNTAX_IDENTIFIER) {
            return bindPatternValue(generator, pattern, subject, 0);
        }

        size_t counter = 1;
        while (counter < pattern->childCount) {
            if (pattern->children[counter] == NULL) {
                return COMPILER_INTERNAL_ERROR;
            }
            status = bindPatternValue(generator, pattern->children[counter], subject, counter);
            if (status != COMPILER_OK) {
                return status;
            }
            counter++;
        }
        return COMPILER_OK;
    }

    DataType patternType = descriptorDataType(&pattern->type);
    size_t counter = 0;
    while (counter < sizeof(runtimeTypes) / sizeof(runtimeTypes[0]) && runtimeTypes[counter].type != patternType) {
        counter++;
    }
    if (counter == sizeof(runtimeTypes) / sizeof(runtimeTypes[0])) {
        return COMPILER_INTERNAL_ERROR;
    }
    CompilerStatus status = appendTypeJump(generator, INTERMEDIATE_JUMP_IF_NOT_EQUAL, nextLabel, runtimeType, runtimeTypes[counter].name, origin);
    if (status != COMPILER_OK) {
        return status;
    }

    IntermediateOperand literal = {0};
    status = generateExpression(generator, &literal, pattern);
    if (status != COMPILER_OK) {
        intermediateOperandClear(&literal);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_NOT_EQUAL, origin, 3, (IntermediateOperand[]){*nextLabel, *subject, literal});
    if (status != COMPILER_OK) {
        intermediateOperandClear(&literal);
        return status;
    }
    intermediateOperandClear(&literal);

    return COMPILER_OK;
}

static CompilerStatus generateStatement(IntermediateGenerator *generator, const AbstractSyntaxNode *node);

/**
 * @brief generates code for a block and provides its final value or Unit
 * @param generator generator state
 * @param operand intermediate operand
 * @param body body AST node
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateBlockValue(IntermediateGenerator *generator, IntermediateOperand *operand, const AbstractSyntaxNode *body) {
    if (body == NULL || body->kind != ABSTRACT_SYNTAX_BLOCK) {
        return generateExpression(generator, operand, body);
    }
    if (body->childCount == 0) {
        *operand = (IntermediateOperand){.kind = INTERMEDIATE_OPERAND_UNIT};
        return COMPILER_OK;
    }
    if (body->children == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    size_t counter = 0;
    while (counter + 1 < body->childCount) {
        CompilerStatus status = generateStatement(generator, body->children[counter]);
        if (status != COMPILER_OK) {
            return status;
        }
        counter++;
    }

    const AbstractSyntaxNode *last = body->children[counter];
    if (last == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    if (last->kind == ABSTRACT_SYNTAX_EXPRESSION_STATEMENT) {
        if (last->childCount != 1 || last->children == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }
        return generateExpression(generator, operand, last->children[0]);
    }
    if (last->kind == ABSTRACT_SYNTAX_CALL_EXPRESSION || last->kind == ABSTRACT_SYNTAX_IF_EXPRESSION || (last->kind == ABSTRACT_SYNTAX_IF && isValueType(valueTypes(generator, &last->type)))) {
        return generateExpression(generator, operand, last);
    }

    CompilerStatus status = generateStatement(generator, last);
    if (status == COMPILER_OK) {
        *operand = (IntermediateOperand){.kind = INTERMEDIATE_OPERAND_UNIT};
    }
    return status;
}

/*
 * AI-generated code
 * ok could you please generate one match case, if pattern matches then jump, if not false, in other cases create the body value and convert it to match and jump
 */
/**
 * @brief generates code for a match case
 * @param generator generator state
 * @param branch match case AST node
 * @param subject value being matched
 * @param runtimeType runtime type operand
 * @param result destination operand
 * @param resultType expected result type
 * @param endLabel label after the match
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateMatchCase(IntermediateGenerator *generator, const AbstractSyntaxNode *branch, const IntermediateOperand *subject, const IntermediateOperand *runtimeType, const IntermediateOperand *result, DataType resultType, const IntermediateOperand *endLabel) {
    if (branch == NULL || branch->kind != ABSTRACT_SYNTAX_MATCH_CASE || (branch->childCount != 2 && branch->childCount != 3) || branch->children == NULL || branch->children[0] == NULL || branch->children[branch->childCount - 1] == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    IntermediateOperand nextLabel = {0};
    CompilerStatus status = generateLabel(generator, &nextLabel);
    if (status != COMPILER_OK) {
        return status;
    }
    status = generateMatchPattern(generator, branch->children[0], subject, runtimeType, &nextLabel);
    if (status != COMPILER_OK) {
        intermediateOperandClear(&nextLabel);
        return status;
    }

    if (branch->childCount == 3) {
        const AbstractSyntaxNode *guard = branch->children[1];
        if (guard == NULL || guard->kind != ABSTRACT_SYNTAX_MATCH_GUARD || guard->childCount != 1 || guard->children == NULL || guard->children[0] == NULL || descriptorDataType(&guard->children[0]->type) != DATA_TYPE_BOOLEAN) {
            intermediateOperandClear(&nextLabel);
            return COMPILER_INTERNAL_ERROR;
        }

        IntermediateOperand condition = {0};
        status = generateExpression(generator, &condition, guard->children[0]);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&condition);
            intermediateOperandClear(&nextLabel);
            return status;
        }

        IntermediateOperand falseOperand = {.kind = INTERMEDIATE_OPERAND_BOOLEAN, .value.boolValue = false};
        status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, guard->location, 3, (IntermediateOperand[]){nextLabel, condition, falseOperand});
        if (status != COMPILER_OK) {
            intermediateOperandClear(&condition);
            intermediateOperandClear(&nextLabel);
            return status;
        }
        intermediateOperandClear(&condition);
    }

    const AbstractSyntaxNode *expression = branch->children[branch->childCount - 1];
    IntermediateOperand value = {0};
    status = generateBlockValue(generator, &value, expression);
    if (status != COMPILER_OK) {
        intermediateOperandClear(&value);
        intermediateOperandClear(&nextLabel);
        return status;
    }

    status = convertValue(generator, &value, valueTypes(generator, &expression->type), resultType, expression->location);
    if (status != COMPILER_OK) {
        intermediateOperandClear(&value);
        intermediateOperandClear(&nextLabel);
        return status;
    }

    SourceLocation origin = branch->location;
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){*result, value});
    if (status != COMPILER_OK) {
        intermediateOperandClear(&value);
        intermediateOperandClear(&nextLabel);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_JUMP, origin, 1, endLabel);
    if (status != COMPILER_OK) {
        intermediateOperandClear(&value);
        intermediateOperandClear(&nextLabel);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &nextLabel);
    if (status != COMPILER_OK) {
        intermediateOperandClear(&value);
        intermediateOperandClear(&nextLabel);
        return status;
    }

    intermediateOperandClear(&value);
    intermediateOperandClear(&nextLabel);

    return COMPILER_OK;
}

/*
 * AI-generated code
 * please ake me the scaffold for a function that checks for a matching expression
 */
/**
 * @brief generates code for a match expression and stores the selected case result
 * @param generator generator state
 * @param operand intermediate operand
 * @param node source AST node
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateMatch(IntermediateGenerator *generator, IntermediateOperand *operand, const AbstractSyntaxNode *node) {
    if (node->childCount < 2 || node->children == NULL || node->children[0] == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    IntermediateOperand subject = {0};
    IntermediateOperand runtimeType = {0};
    IntermediateOperand result = {0};
    IntermediateOperand endLabel = {0};
    IntermediateOperand *ownedOperands[] = {&subject, &runtimeType, &result, &endLabel};
    size_t ownedOperandCount = sizeof(ownedOperands) / sizeof(ownedOperands[0]);

    CompilerStatus status = generateExpression(generator, &subject, node->children[0]);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }
    status = generateTemporary(generator, &runtimeType, node->location);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }
    status = generateTemporary(generator, &result, node->location);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }
    status = generateLabel(generator, &endLabel);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }
    status = appendControlInstruction(generator, INTERMEDIATE_GET_TYPE, node->location, 2, (IntermediateOperand[]){runtimeType, subject});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    size_t counter = 1;
    DataType resultType = valueTypes(generator, &node->type);
    while (counter < node->childCount) {
        status = generateMatchCase(generator, node->children[counter], &subject, &runtimeType, &result, resultType, &endLabel);
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }
        counter++;
    }

    IntermediateOperand error = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 40};
    status = appendControlInstruction(generator, INTERMEDIATE_EXIT, node->location, 1, &error);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, node->location, 1, &endLabel);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    *operand = result;
    result.text = NULL;
    clearOperandPointers(ownedOperands, ownedOperandCount);

    return COMPILER_OK;
}

/*
 * AI-generated code
 * make me a scaffolding for a function that generates the constructor for a given class please
 */
/**
 * @brief generates code to build case-class array with its tag and field values
 * @param generator generator state
 * @param operand intermediate operand
 * @param node source AST node
 * @param declaration case-class declaration
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateClassConstructor(IntermediateGenerator *generator, IntermediateOperand *operand, const AbstractSyntaxNode *node, const AbstractSyntaxNode *declaration) {
    if (declaration->childCount != node->childCount - 1 || declaration->childCount > INT64_MAX - 1 || (declaration->childCount != 0 && declaration->children == NULL)) {
        return COMPILER_INTERNAL_ERROR;
    }

    IntermediateOperand result = {0};
    CompilerStatus status = generateTemporary(generator, &result, node->location);
    if (status != COMPILER_OK) {
        return status;
    }

    IntermediateOperand size = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = (int64_t)node->childCount};
    IntermediateOperand zero = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 0};
    IntermediateOperand tag = {.kind = INTERMEDIATE_OPERAND_STRING, .text = declaration->text};
    status = appendControlInstruction(generator, INTERMEDIATE_ARRAY_CREATE, node->location, 2, (IntermediateOperand[]){result, size});
    if (status != COMPILER_OK) {
        intermediateOperandClear(&result);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_ARRAY_SET, node->location, 3, (IntermediateOperand[]){result, zero, tag});
    if (status != COMPILER_OK) {
        intermediateOperandClear(&result);
        return status;
    }

    size_t counter = 0;
    while (counter < declaration->childCount) {
        const AbstractSyntaxNode *field = declaration->children[counter];
        const AbstractSyntaxNode *argument = node->children[counter + 1];
        if (field == NULL || field->kind != ABSTRACT_SYNTAX_CASE_CLASS_FIELD || argument == NULL) {
            intermediateOperandClear(&result);
            return COMPILER_INTERNAL_ERROR;
        }

        IntermediateOperand value = {0};
        status = generateExpression(generator, &value, argument);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&value);
            intermediateOperandClear(&result);
            return status;
        }

        status = convertValue(generator, &value, valueTypes(generator, &argument->type), valueTypes(generator, &field->type), argument->location);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&value);
            intermediateOperandClear(&result);
            return status;
        }

        IntermediateOperand index = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = (int64_t)(counter + 1)};
        status = appendControlInstruction(generator, INTERMEDIATE_ARRAY_SET, argument->location, 3, (IntermediateOperand[]){result, index, value});
        if (status != COMPILER_OK) {
            intermediateOperandClear(&value);
            intermediateOperandClear(&result);
            return status;
        }
        intermediateOperandClear(&value);
        counter++;
    }

    *operand = result;

    return COMPILER_OK;
}

/**
 * @brief evaluates a range component and stores it in the destination
 * @param generator generator state
 * @param destination destination operand
 * @param node source AST node
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateRangeValue(IntermediateGenerator *generator, const IntermediateOperand *destination, const AbstractSyntaxNode *node) {
    IntermediateOperand valueOperand = {0};
    CompilerStatus status = generateExpression(generator, &valueOperand, node);
    if (status != COMPILER_OK) {
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, node->location, 2, (IntermediateOperand[]){*destination, valueOperand});
    intermediateOperandClear(&valueOperand);

    return status;
}

/**
 * @brief creates an integer literal operand
 * @param value value operand
 * @return integer operand
 */
static IntermediateOperand integerOperand(int64_t value) {
    return (IntermediateOperand){.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = value};
}

/**
 * @brief creates a string operand that references the given text
 * @param value value operand
 * @return string operand
 */
static IntermediateOperand stringOperand(char *value) {
    return (IntermediateOperand){.kind = INTERMEDIATE_OPERAND_STRING, .text = value};
}

/*
 * AI-generated code
 * ok, can you please write the scaffold for generateListArray so that i can only add the necessary thing in there and not bother with bs checks
 */
/**
 * @brief generates code to allocate list array and set its tag and length
 * @param generator generator state
 * @param result destination operand
 * @param length list length operand
 * @param origin source location
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateListArray(IntermediateGenerator *generator, const IntermediateOperand *result, const IntermediateOperand *length, SourceLocation origin) {
    IntermediateOperand size = {0};
    CompilerStatus status = generateTemporary(generator, &size, origin);
    if (status != COMPILER_OK) {
        return status;
    }

    // Slot 0 identifies the array as a list. Slot 1 holds its length.
    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){size, *length, integerOperand(2)});
    if (status != COMPILER_OK) {
        intermediateOperandClear(&size);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_ARRAY_CREATE, origin, 2, (IntermediateOperand[]){*result, size});
    if (status != COMPILER_OK) {
        intermediateOperandClear(&size);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_ARRAY_SET, origin, 3, (IntermediateOperand[]){*result, integerOperand(0), stringOperand("$List")});
    if (status != COMPILER_OK) {
        intermediateOperandClear(&size);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_ARRAY_SET, origin, 3, (IntermediateOperand[]){*result, integerOperand(1), *length});
    if (status != COMPILER_OK) {
        intermediateOperandClear(&size);
        return status;
    }
    intermediateOperandClear(&size);

    return COMPILER_OK;
}

/**
 * @brief generates code to read the list length from its header
 * @param generator generator state
 * @param result destination operand
 * @param list list operand
 * @param origin source location
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateListLength(IntermediateGenerator *generator, const IntermediateOperand *result, const IntermediateOperand *list, SourceLocation origin) {
    return appendControlInstruction(generator, INTERMEDIATE_ARRAY_GET, origin, 3, (IntermediateOperand[]){*result, *list, integerOperand(1)});
}

/**
 * @brief generates code to build a list and convert its elements as needed
 * @param generator generator state
 * @param operand intermediate operand
 * @param node source AST node
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateListConstructor(IntermediateGenerator *generator, IntermediateOperand *operand, const AbstractSyntaxNode *node) {
    IntermediateOperand result = {0};
    CompilerStatus status = generateTemporary(generator, &result, node->location);
    if (status != COMPILER_OK) {
        return status;
    }
    IntermediateOperand length = integerOperand((int64_t)(node->childCount - 1));
    status = generateListArray(generator, &result, &length, node->location);
    size_t counter = 1;
    while (status == COMPILER_OK && counter < node->childCount) {
        const AbstractSyntaxNode *element = node->children[counter];
        IntermediateOperand value = {0};
        status = generateExpression(generator, &value, element);
        if (status == COMPILER_OK) {
            const TypeDescriptorRecursiveNode *elementType = node->type.recursiveTypes;
            DataType target = elementType == NULL ? DATA_TYPE_UNKNOWN : valueTypes(generator, &elementType->elementType);
            status = convertValue(generator, &value, valueTypes(generator, &element->type), target, element->location);
        }
        if (status == COMPILER_OK) {
            status = appendControlInstruction(generator, INTERMEDIATE_ARRAY_SET, element->location, 3, (IntermediateOperand[]){result, integerOperand((int64_t)counter + 1), value});
        }
        intermediateOperandClear(&value);
        counter++;
    }
    if (status != COMPILER_OK) {
        intermediateOperandClear(&result);
        return status;
    }
    *operand = result;
    return COMPILER_OK;
}

/*
 * AI-generated code
 * ok could you please implement list indexing and head? use index 0 for head, emit EXIT 40 for out-of-range indices, and read from index + 2. widen Int to Double if the result type needs it.
 */
/**
 * @brief generates code to check bounds and read a list element by index or via head
 * @param generator generator state
 * @param operand intermediate operand
 * @param receiver receiver AST node
 * @param indexNode index expression or NULL for head
 * @param type expected element type
 * @param origin source location
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateListIndex(IntermediateGenerator *generator, IntermediateOperand *operand, const AbstractSyntaxNode *receiver, const AbstractSyntaxNode *indexNode, const TypeDescriptor *type, SourceLocation origin) {
    IntermediateOperand list = {0};
    IntermediateOperand index = integerOperand(0);
    IntermediateOperand result = {0};
    IntermediateOperand length = {0};
    IntermediateOperand condition = {0};
    IntermediateOperand invalid = {0};
    IntermediateOperand valid = {0};
    IntermediateOperand *ownedOperands[] = {&list, &index, &result, &length, &condition, &invalid, &valid};
    size_t ownedOperandCount = sizeof(ownedOperands) / sizeof(ownedOperands[0]);

    CompilerStatus status = generateExpression(generator, &list, receiver);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    if (indexNode != NULL) {
        status = generateExpression(generator, &index, indexNode);
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }
    }

    status = generateTemporary(generator, &result, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(generator, &length, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(generator, &condition, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateLabel(generator, &invalid);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateLabel(generator, &valid);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateListLength(generator, &length, &list, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    IntermediateOperand yes = {.kind = INTERMEDIATE_OPERAND_BOOLEAN, .value.boolValue = true};
    IntermediateOperand error = integerOperand(40);
    status = appendControlInstruction(generator, INTERMEDIATE_COMPARE_LESS, origin, 3, (IntermediateOperand[]){condition, index, integerOperand(0)});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){invalid, condition, yes});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_COMPARE_LESS, origin, 3, (IntermediateOperand[]){condition, index, length});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){valid, condition, yes});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &invalid);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_EXIT, origin, 1, &error);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &valid);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){length, index, integerOperand(2)});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_ARRAY_GET, origin, 3, (IntermediateOperand[]){result, list, length});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    DataType target = valueTypes(generator, type);
    if ((target & DATA_TYPE_DOUBLE) && !(target & DATA_TYPE_INT)) {
        status = convertValue(generator, &result, target | DATA_TYPE_INT, target, origin);
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }
    }

    *operand = result;
    result.text = NULL;
    clearOperandPointers(ownedOperands, ownedOperandCount);

    return COMPILER_OK;
}

/**
 * @brief counts the list levels for an Int to Double conversion
 * @param generator generator state
 * @param source source type descriptor
 * @param destination destination type descriptor
 * @return nesting depth or zero if no conversion is needed
 */
static size_t listWideningDepth(IntermediateGenerator *generator, const TypeDescriptor *source, const TypeDescriptor *destination) {
    size_t depth = 0;

    while (valueTypes(generator, source) == DATA_TYPE_LIST && (valueTypes(generator, destination) & DATA_TYPE_LIST) != 0) {
        const TypeDescriptorRecursiveNode *sourceElement = source->recursiveTypes;
        const TypeDescriptorRecursiveNode *destinationElement = destination->recursiveTypes;
        if (sourceElement == NULL || sourceElement->next != NULL || destinationElement == NULL || destinationElement->next != NULL) {
            return 0;
        }

        source = &sourceElement->elementType;
        destination = &destinationElement->elementType;
        depth++;
    }

    DataType destinationType = valueTypes(generator, destination);
    if (valueTypes(generator, source) == DATA_TYPE_INT && (destinationType & (DATA_TYPE_INT | DATA_TYPE_DOUBLE)) == DATA_TYPE_DOUBLE) {
        return depth;
    }

    return 0;
}

static CompilerStatus generateWidenedList(IntermediateGenerator *generator, IntermediateOperand *operand, size_t wideningDepth, SourceLocation origin);

/**
 * @brief generates a loop to copy part of a list and realocate elements as needed
 * @param generator generator state
 * @param result destination operand
 * @param destinationStart starting index in destination list
 * @param source source operand
 * @param start inclusive source index
 * @param end exclusive source index
 * @param wideningDepth remaining list levels to widen
 * @param origin source location
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateListCopy(IntermediateGenerator *generator, const IntermediateOperand *result, const IntermediateOperand *destinationStart, const IntermediateOperand *source, const IntermediateOperand *start, const IntermediateOperand *end, size_t wideningDepth, SourceLocation origin) {
    IntermediateOperand position = {0};
    IntermediateOperand destination = {0};
    IntermediateOperand sourceIndex = {0};
    IntermediateOperand element = {0};
    IntermediateOperand condition = {0};
    IntermediateOperand loop = {0};
    IntermediateOperand done = {0};
    IntermediateOperand *ownedOperands[] = {&position, &destination, &sourceIndex, &element, &condition, &loop, &done};
    size_t ownedOperandCount = sizeof(ownedOperands) / sizeof(ownedOperands[0]);

    CompilerStatus status = generateTemporary(generator, &position, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(generator, &destination, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(generator, &sourceIndex, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(generator, &element, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(generator, &condition, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateLabel(generator, &loop);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateLabel(generator, &done);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    IntermediateOperand one = integerOperand(1);
    IntermediateOperand two = integerOperand(2);
    IntermediateOperand no = {.kind = INTERMEDIATE_OPERAND_BOOLEAN, .value.boolValue = false};
    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){position, *start});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){destination, *destinationStart, two});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &loop);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_COMPARE_LESS, origin, 3, (IntermediateOperand[]){condition, position, *end});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){done, condition, no});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){sourceIndex, position, two});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_ARRAY_GET, origin, 3, (IntermediateOperand[]){element, *source, sourceIndex});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    if (wideningDepth == 1) {
        status = convertIntToDouble(generator, &element, origin);
    } else if (wideningDepth > 1) {
        status = generateWidenedList(generator, &element, wideningDepth - 1, origin);
    }

    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_ARRAY_SET, origin, 3, (IntermediateOperand[]){*result, destination, element});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){position, position, one});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){destination, destination, one});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_JUMP, origin, 1, &loop);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &done);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    clearOperandPointers(ownedOperands, ownedOperandCount);
    return COMPILER_OK;
}

/**
 * @brief generates code to copy a list and widen its elements
 * @param generator generator state
 * @param operand intermediate operand
 * @param wideningDepth remaining list levels to widen
 * @param origin source location
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateWidenedList(IntermediateGenerator *generator, IntermediateOperand *operand, size_t wideningDepth, SourceLocation origin) {
    IntermediateOperand result = {0};
    IntermediateOperand length = {0};
    IntermediateOperand *ownedOperands[] = {&result, &length};
    size_t ownedOperandCount = sizeof(ownedOperands) / sizeof(ownedOperands[0]);

    CompilerStatus status = generateTemporary(generator, &result, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(generator, &length, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateListLength(generator, &length, operand, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateListArray(generator, &result, &length, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    IntermediateOperand zero = integerOperand(0);
    status = generateListCopy(generator, &result, &zero, operand, &zero, &length, wideningDepth, origin);
    intermediateOperandClear(&length);
    if (status != COMPILER_OK) {
        intermediateOperandClear(&result);
        return status;
    }

    intermediateOperandClear(operand);
    *operand = result;
    return COMPILER_OK;
}

/**
 * @brief generates code to convert value or list for initialization or assignment
 * @param generator generator state
 * @param operand intermediate operand
 * @param source source type descriptor
 * @param destination destination type descriptor
 * @param origin source location
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus convertAssignedValue(IntermediateGenerator *generator, IntermediateOperand *operand, const TypeDescriptor *source, const TypeDescriptor *destination, SourceLocation origin) {
    DataType destinationType = valueTypes(generator, destination);
    if (destinationType == DATA_TYPE_UNKNOWN) {
        return COMPILER_OK;
    }

    size_t wideningDepth = listWideningDepth(generator, source, destination);
    if (wideningDepth != 0) {
        return generateWidenedList(generator, operand, wideningDepth, origin);
    }
    return convertValue(generator, operand, valueTypes(generator, source), destinationType, origin);
}

/*
 * AI-generated code
 * ok could you please implement ++? evaluate both lists left to right, allocate a list with length of first plus second, and copy the left and right lists into it at 0 and leftLength.
 */
/**
 * @brief generates code to join two lists and widen elements as needed
 * @param generator generator state
 * @param operand intermediate operand
 * @param node source AST node
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateListConcat(IntermediateGenerator *generator, IntermediateOperand *operand, const AbstractSyntaxNode *node) {
    SourceLocation origin = node->location;
    IntermediateOperand left = {0};
    IntermediateOperand right = {0};
    IntermediateOperand result = {0};
    IntermediateOperand leftLength = {0};
    IntermediateOperand rightLength = {0};
    IntermediateOperand total = {0};
    IntermediateOperand *ownedOperands[] = {&left, &right, &result, &leftLength, &rightLength, &total};
    size_t ownedOperandCount = sizeof(ownedOperands) / sizeof(ownedOperands[0]);

    CompilerStatus status = generateExpression(generator, &left, node->children[0]);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateExpression(generator, &right, node->children[1]);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(generator, &result, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(generator, &leftLength, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(generator, &rightLength, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(generator, &total, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateListLength(generator, &leftLength, &left, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateListLength(generator, &rightLength, &right, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){total, leftLength, rightLength});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateListArray(generator, &result, &total, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    IntermediateOperand zero = integerOperand(0);
    size_t leftWideningDepth = listWideningDepth(generator, &node->children[0]->type, &node->type);
    status = generateListCopy(generator, &result, &zero, &left, &zero, &leftLength, leftWideningDepth, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    size_t rightWideningDepth = listWideningDepth(generator, &node->children[1]->type, &node->type);
    status = generateListCopy(generator, &result, &leftLength, &right, &zero, &rightLength, rightWideningDepth, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    *operand = result;
    result.text = NULL;

    clearOperandPointers(ownedOperands, ownedOperandCount);
    return COMPILER_OK;
}

/*
 * AI-generated code
 * ok could you please clamp the index in place? use the limit as a maximum when upperLimit is true and as a minimum otherwise.
 */
/**
 * @brief generates code to keep a list index within the given range
 * @param generator generator state
 * @param index index operand to clamp
 * @param limit range operand
 * @param upperLimit if the range is the upper limit
 * @param origin source location
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus capListIndex(IntermediateGenerator *generator, const IntermediateOperand *index, const IntermediateOperand *limit, bool upperLimit, SourceLocation origin) {
    IntermediateOperand condition = {0};
    IntermediateOperand done = {0};
    IntermediateOperand *ownedOperands[] = {&condition, &done};
    size_t ownedOperandCount = sizeof(ownedOperands) / sizeof(ownedOperands[0]);

    CompilerStatus status = generateTemporary(generator, &condition, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateLabel(generator, &done);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    IntermediateOperand no = {.kind = INTERMEDIATE_OPERAND_BOOLEAN, .value.boolValue = false};
    IntermediateOpcode comparison = upperLimit ? INTERMEDIATE_COMPARE_GREATER : INTERMEDIATE_COMPARE_LESS;
    status = appendControlInstruction(generator, comparison, origin, 3, (IntermediateOperand[]){condition, *index, *limit});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){done, condition, no});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){*index, *limit});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &done);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    clearOperandPointers(ownedOperands, ownedOperandCount);
    return COMPILER_OK;
}

/*
 * AI-generated code
 * could you please finish the function so that there are the necessary list caps, instructions and lists generated, also make sure there are all necessary operands
 */
/**
 * @brief generates code to copy the part of a list needed by slice, take, drop or tail
 * @param generator generator state
 * @param operand intermediate operand
 * @param receiver receiver AST node
 * @param from starting index expression or NULL for the default start
 * @param until ending index expression or NULL for the list length
 * @param isTail whether to generate tail access
 * @param origin source location
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateListSlice(IntermediateGenerator *generator, IntermediateOperand *operand, const AbstractSyntaxNode *receiver, const AbstractSyntaxNode *from, const AbstractSyntaxNode *until, bool isTail, SourceLocation origin) {
    IntermediateOperand list = {0};
    IntermediateOperand result = {0};
    IntermediateOperand length = {0};
    IntermediateOperand start = {0};
    IntermediateOperand end = {0};
    IntermediateOperand count = {0};
    IntermediateOperand nonempty = {0};
    IntermediateOperand *ownedOperands[] = {&list, &result, &length, &start, &end, &count, &nonempty};
    size_t ownedOperandCount = sizeof(ownedOperands) / sizeof(ownedOperands[0]);

    CompilerStatus status = generateExpression(generator, &list, receiver);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(generator, &result, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(generator, &length, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(generator, &start, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(generator, &end, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(generator, &count, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    if (isTail) {
        status = generateLabel(generator, &nonempty);
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }
    }

    status = generateListLength(generator, &length, &list, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    IntermediateOperand zero = integerOperand(0);
    if (isTail) {
        IntermediateOperand error = integerOperand(40);
        status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_NOT_EQUAL, origin, 3, (IntermediateOperand[]){nonempty, length, zero});
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }
        status = appendControlInstruction(generator, INTERMEDIATE_EXIT, origin, 1, &error);
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }
        status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &nonempty);
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }
    }

    if (from == NULL) {
        status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){start, integerOperand(isTail ? 1 : 0)});
    } else {
        status = generateRangeValue(generator, &start, from);
    }
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    if (until == NULL) {
        status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){end, length});
    } else {
        status = generateRangeValue(generator, &end, until);
    }
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = capListIndex(generator, &start, &zero, false, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = capListIndex(generator, &start, &length, true, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = capListIndex(generator, &end, &length, true, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = capListIndex(generator, &end, &start, false, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_SUBTRACT, origin, 3, (IntermediateOperand[]){count, end, start});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateListArray(generator, &result, &count, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateListCopy(generator, &result, &zero, &list, &start, &end, 0, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    *operand = result;
    result.text = NULL;

    clearOperandPointers(ownedOperands, ownedOperandCount);
    return COMPILER_OK;
}

/*
 * AI-generated code
 * ok can you please give me the scaffold for the generateListRange function and necessary operands/instructions that will be needed there
 */
/**
 * @brief generates code to build a list from a range with the given bounds and step
 * @param generator generator state
 * @param operand intermediate operand
 * @param parts range bounds and optional step
 * @param count number of range components
 * @param inclusive whether the end is inclusive
 * @param origin source location
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateListRange(IntermediateGenerator *generator, IntermediateOperand *operand, AbstractSyntaxNode *const *parts, size_t count, bool inclusive, SourceLocation origin) {
    if (parts == NULL || (count != 2 && count != 3)) {
        return COMPILER_INTERNAL_ERROR;
    }
    IntermediateOperand result = {0};
    IntermediateOperand value = {0};
    IntermediateOperand end = {0};
    IntermediateOperand step = {0};
    IntermediateOperand distance = {0};
    IntermediateOperand stride = {0};
    IntermediateOperand length = {0};
    IntermediateOperand nonzero = {0};
    IntermediateOperand positive = {0};
    IntermediateOperand allocate = {0};
    IntermediateOperand loop = {0};
    IntermediateOperand done = {0};
    IntermediateOperand *ownedOperands[] = {
        &result, &value, &end, &step, &distance, &stride, &length, &nonzero, &positive, &allocate, &loop, &done,
    };
    size_t ownedOperandCount = sizeof(ownedOperands) / sizeof(ownedOperands[0]);

    CompilerStatus status = generateTemporary(generator, &result, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(generator, &value, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(generator, &end, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(generator, &step, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(generator, &distance, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(generator, &stride, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(generator, &length, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateLabel(generator, &nonzero);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateLabel(generator, &positive);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateLabel(generator, &allocate);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateLabel(generator, &loop);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateLabel(generator, &done);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateRangeValue(generator, &value, parts[0]);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateRangeValue(generator, &end, parts[1]);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = count == 3 ? generateRangeValue(generator, &step, parts[2]) : appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){step, integerOperand(1)});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    IntermediateOperand zero = integerOperand(0);
    IntermediateOperand one = integerOperand(1);
    IntermediateOperand two = integerOperand(2);
    IntermediateOperand error = integerOperand(40);
    IntermediateOperand no = {.kind = INTERMEDIATE_OPERAND_BOOLEAN, .value.boolValue = false};

    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_NOT_EQUAL, origin, 3, (IntermediateOperand[]){nonzero, step, zero});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_EXIT, origin, 1, &error);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &nonzero);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_SUBTRACT, origin, 3, (IntermediateOperand[]){distance, end, value});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){stride, step});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_COMPARE_LESS, origin, 3, (IntermediateOperand[]){end, step, zero});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){positive, end, no});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_SUBTRACT, origin, 3, (IntermediateOperand[]){distance, zero, distance});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_SUBTRACT, origin, 3, (IntermediateOperand[]){stride, zero, step});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &positive);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){length, zero});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_SUBTRACT, origin, 3, (IntermediateOperand[]){distance, distance, integerOperand(inclusive ? 0 : 1)});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_COMPARE_LESS, origin, 3, (IntermediateOperand[]){end, distance, zero});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_NOT_EQUAL, origin, 3, (IntermediateOperand[]){allocate, end, no});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_INTEGER_DIVIDE, origin, 3, (IntermediateOperand[]){length, distance, stride});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){length, length, one});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &allocate);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateListArray(generator, &result, &length, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){length, length, two});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){distance, two});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &loop);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){done, distance, length});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_ARRAY_SET, origin, 3, (IntermediateOperand[]){result, distance, value});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){distance, distance, one});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){value, value, step});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_JUMP, origin, 1, &loop);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &done);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    *operand = result;
    result.text = NULL;

    clearOperandPointers(ownedOperands, ownedOperandCount);
    return COMPILER_OK;
}

/**
 * @brief calls the list formatter in the generated code and marks it for including
 * @param generator generator state
 * @param result destination operand
 * @param value value operand
 * @param origin source location
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateListString(IntermediateGenerator *generator, const IntermediateOperand *result, const IntermediateOperand *value, SourceLocation origin) {
    generator->needsListString = true;
    IntermediateOperand label = {.kind = INTERMEDIATE_OPERAND_LABEL, .text = "$listToString"};

    CompilerStatus status = appendControlInstruction(generator, INTERMEDIATE_PUSH, origin, 1, value);
    if (status != COMPILER_OK) {
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_CREATE_FRAME, origin, 0, NULL);
    if (status != COMPILER_OK) {
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_CALL, origin, 1, &label);
    if (status != COMPILER_OK) {
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_POP, origin, 1, result);
    if (status != COMPILER_OK) {
        return status;
    }

    return COMPILER_OK;
}

/**
 * @brief appends copies of instructions from another intermediate program
 * @param destination destination program
 * @param source source program
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
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

/**
 * @brief builds the function that formats lists and adds it to the program
 * @param generator generator state
 * @param function generator state for the helper function
 * @param origin source location
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus appendListStringFunction(IntermediateGenerator *generator, IntermediateGenerator *function, SourceLocation origin) {
    IntermediateOperand list = {0};
    IntermediateOperand result = {0};
    IntermediateOperand length = {0};
    IntermediateOperand position = {0};
    IntermediateOperand item = {0};
    IntermediateOperand text = {0};
    IntermediateOperand condition = {0};
    IntermediateOperand loop = {0};
    IntermediateOperand done = {0};
    IntermediateOperand *ownedOperands[] = {&list, &result, &length, &position, &item, &text, &condition, &loop, &done};
    size_t ownedOperandCount = sizeof(ownedOperands) / sizeof(ownedOperands[0]);

    CompilerStatus status = generateTemporary(function, &list, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(function, &result, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(function, &length, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(function, &position, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(function, &item, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(function, &text, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(function, &condition, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateLabel(function, &loop);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateLabel(function, &done);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(function, INTERMEDIATE_POP, origin, 1, &list);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateListLength(function, &length, &list, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    IntermediateOperand no = {.kind = INTERMEDIATE_OPERAND_BOOLEAN, .value.boolValue = false};
    status = appendControlInstruction(function, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){length, length, integerOperand(2)});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(function, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){position, integerOperand(2)});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(function, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){result, stringOperand("List(")});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(function, INTERMEDIATE_LABEL, origin, 1, &loop);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(function, INTERMEDIATE_COMPARE_LESS, origin, 3, (IntermediateOperand[]){condition, position, length});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(function, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){done, condition, no});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(function, INTERMEDIATE_ARRAY_GET, origin, 3, (IntermediateOperand[]){item, list, position});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    DataType printable = DATA_TYPE_INT | DATA_TYPE_DOUBLE | DATA_TYPE_BOOLEAN | DATA_TYPE_STRING | DATA_TYPE_UNIT | DATA_TYPE_LIST;
    status = generateUnionString(function, &text, &item, printable, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(function, INTERMEDIATE_CONCATENATE, origin, 3, (IntermediateOperand[]){result, result, text});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(function, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){position, position, integerOperand(1)});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(function, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){done, position, length});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(function, INTERMEDIATE_CONCATENATE, origin, 3, (IntermediateOperand[]){result, result, stringOperand(", ")});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(function, INTERMEDIATE_JUMP, origin, 1, &loop);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(function, INTERMEDIATE_LABEL, origin, 1, &done);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(function, INTERMEDIATE_CONCATENATE, origin, 3, (IntermediateOperand[]){result, result, stringOperand(")")});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(function, INTERMEDIATE_PUSH, origin, 1, &result);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(function, INTERMEDIATE_POP_FRAME, origin, 0, NULL);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(function, INTERMEDIATE_RETURN, origin, 0, NULL);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    IntermediateOperand label = {.kind = INTERMEDIATE_OPERAND_LABEL, .text = "$listToString"};
    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &label);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_PUSH_FRAME, origin, 0, NULL);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendProgramInstructions(generator->program, function->functionDeclarations);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendProgramInstructions(generator->program, function->program);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    clearOperandPointers(ownedOperands, ownedOperandCount);
    return COMPILER_OK;
}

/*
 * AI-generated code
 * ok could you please make me the scaffold for generateListStringFunction so that i can jsut finish the logic in there
 */
/**
 * @brief sets up and generates the list-to-string helper
 * @param generator generator state
 * @param origin source location
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateListStringFunction(IntermediateGenerator *generator, SourceLocation origin) {
    IntermediateGenerator function = *generator;
    function.functionDeclarations = intermediateProgramCreate();
    function.program = intermediateProgramCreate();
    if (function.functionDeclarations == NULL || function.program == NULL) {
        intermediateProgramDestroy(function.functionDeclarations);
        intermediateProgramDestroy(function.program);
        return COMPILER_INTERNAL_ERROR;
    }

    CompilerStatus status = appendListStringFunction(generator, &function, origin);
    if (status != COMPILER_OK) {
        intermediateProgramDestroy(function.program);
        intermediateProgramDestroy(function.functionDeclarations);
        return status;
    }

    intermediateProgramDestroy(function.program);
    intermediateProgramDestroy(function.functionDeclarations);
    return COMPILER_OK;
}

/**
 * @brief generates code for head, tail or toString on a list
 * @param generator generator state
 * @param operand intermediate operand
 * @param node source AST node
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateListMember(IntermediateGenerator *generator, IntermediateOperand *operand, const AbstractSyntaxNode *node) {
    const AbstractSyntaxNode *receiver = node->children[0];
    if (strcmp(node->text, "head") == 0) {
        return generateListIndex(generator, operand, receiver, NULL, &node->type, node->location);
    }
    if (strcmp(node->text, "tail") == 0) {
        return generateListSlice(generator, operand, receiver, NULL, NULL, true, node->location);
    }
    if (strcmp(node->text, "toString") != 0) {
        return COMPILER_INTERNAL_ERROR;
    }

    IntermediateOperand value = {0};
    IntermediateOperand result = {0};
    CompilerStatus status = generateExpression(generator, &value, receiver);
    if (status == COMPILER_OK) {
        status = generateTemporary(generator, &result, node->location);
    }
    if (status == COMPILER_OK) {
        status = generateListString(generator, &result, &value, node->location);
    }
    intermediateOperandClear(&value);
    if (status != COMPILER_OK) {
        intermediateOperandClear(&result);
        return status;
    }
    *operand = result;
    return COMPILER_OK;
}

/**
 * @brief generates code for list indexing, List.range or a list method call
 * @param generator generator state
 * @param operand intermediate operand
 * @param node source AST node
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateListCall(IntermediateGenerator *generator, IntermediateOperand *operand, const AbstractSyntaxNode *node) {
    const AbstractSyntaxNode *callee = node->children[0];
    if (valueTypes(generator, &callee->type) == DATA_TYPE_LIST) {
        if (node->childCount != 2 || node->children[1] == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }
        return generateListIndex(generator, operand, callee, node->children[1], &node->type, node->location);
    }

    if (callee->kind != ABSTRACT_SYNTAX_MEMBER_ACCESS || callee->text == NULL || callee->childCount != 1 || callee->children == NULL || callee->children[0] == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    const AbstractSyntaxNode *receiver = callee->children[0];
    if (receiver->kind == ABSTRACT_SYNTAX_IDENTIFIER && receiver->text != NULL && strcmp(receiver->text, "List") == 0 && strcmp(callee->text, "range") == 0) {
        if (node->childCount != 3 && node->childCount != 4) {
            return COMPILER_INTERNAL_ERROR;
        }
        return generateListRange(generator, operand, node->children + 1, node->childCount - 1, false, node->location);
    }
    if (strcmp(callee->text, "slice") == 0 && node->childCount == 3) {
        return generateListSlice(generator, operand, receiver, node->children[1], node->children[2], false, node->location);
    }
    if (strcmp(callee->text, "drop") == 0 && node->childCount == 2) {
        return generateListSlice(generator, operand, receiver, node->children[1], NULL, false, node->location);
    }
    if (strcmp(callee->text, "take") == 0 && node->childCount == 2) {
        return generateListSlice(generator, operand, receiver, NULL, node->children[1], false, node->location);
    }
    return COMPILER_INTERNAL_ERROR;
}

/*
 * AI-generated code
 * alright, please write me the generateMemberAccess function, for case class reading from 0, find requested field in matching class and read from there +1, reuse existing functions respectively
 */
/**
 * @brief generates code for list members, range conversion or case-class fields
 * @param generator generator state
 * @param operand intermediate operand
 * @param node source AST node
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateMemberAccess(IntermediateGenerator *generator, IntermediateOperand *operand, const AbstractSyntaxNode *node) {
    if (node->text == NULL || node->childCount != 1 || node->children == NULL || node->children[0] == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    const AbstractSyntaxNode *object = node->children[0];
    if (object->kind == ABSTRACT_SYNTAX_RANGE_EXPRESSION && strcmp(node->text, "toList") == 0 && object->text != NULL && (object->childCount == 2 || object->childCount == 3)) {
        return generateListRange(generator, operand, object->children, object->childCount, strcmp(object->text, "to") == 0, node->location);
    }
    DataType receiverType = valueTypes(generator, &object->type);
    if (receiverType == DATA_TYPE_LIST) {
        return generateListMember(generator, operand, node);
    }
    if (receiverType != DATA_TYPE_CASE_CLASS) {
        return COMPILER_INTERNAL_ERROR;
    }

    SourceLocation origin = node->location;
    IntermediateOperand receiver = {0};
    IntermediateOperand result = {0};
    IntermediateOperand tag = {0};
    IntermediateOperand endLabel = {0};
    IntermediateOperand nextLabel = {0};
    IntermediateOperand *ownedOperands[] = {&receiver, &result, &tag, &endLabel, &nextLabel};
    size_t ownedOperandCount = sizeof(ownedOperands) / sizeof(ownedOperands[0]);

    CompilerStatus status = generateExpression(generator, &receiver, object);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(generator, &result, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(generator, &tag, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateLabel(generator, &endLabel);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_ARRAY_GET, origin, 3, (IntermediateOperand[]){tag, receiver, integerOperand(0)});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    const SourceProgram *program = generator->sourceProgram;
    size_t counter = 0;
    bool foundField = false;
    while (counter < program->root->childCount) {
        const AbstractSyntaxNode *declaration = program->root->children[counter++];
        if (declaration->kind != ABSTRACT_SYNTAX_CASE_CLASS) {
            continue;
        }
        size_t fieldIndex = 0;
        while (fieldIndex < declaration->childCount && strcmp(declaration->children[fieldIndex]->text, node->text) != 0) {
            fieldIndex++;
        }
        if (fieldIndex == declaration->childCount) {
            continue;
        }
        foundField = true;

        status = generateLabel(generator, &nextLabel);
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }
        IntermediateOperand className = {.kind = INTERMEDIATE_OPERAND_STRING, .text = declaration->text};
        IntermediateOperand index = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = (int64_t)(fieldIndex + 1)};
        status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_NOT_EQUAL, origin, 3, (IntermediateOperand[]){nextLabel, tag, className});
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }
        status = appendControlInstruction(generator, INTERMEDIATE_ARRAY_GET, origin, 3, (IntermediateOperand[]){result, receiver, index});
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }
        status = appendControlInstruction(generator, INTERMEDIATE_JUMP, origin, 1, &endLabel);
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }
        status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &nextLabel);
        if (status != COMPILER_OK) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }
        intermediateOperandClear(&nextLabel);
    }
    if (!foundField) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return COMPILER_INTERNAL_ERROR;
    }

    IntermediateOperand error = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 40};
    status = appendControlInstruction(generator, INTERMEDIATE_EXIT, origin, 1, &error);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &endLabel);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    *operand = result;
    result.text = NULL;

    clearOperandPointers(ownedOperands, ownedOperandCount);
    return COMPILER_OK;
}

/**
 * @brief generates code for an expression and provides its result operand
 * @param generator generator state
 * @param operand intermediate operand
 * @param node source AST node
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
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
    case ABSTRACT_SYNTAX_BOOLEAN_LITERAL:
        operand->kind = INTERMEDIATE_OPERAND_BOOLEAN;
        operand->text = NULL;
        operand->value.boolValue = node->value.boolValue;

        break;
    case ABSTRACT_SYNTAX_MATCH_EXPRESSION:
        return generateMatch(generator, operand, node);
    case ABSTRACT_SYNTAX_MEMBER_ACCESS:
        return generateMemberAccess(generator, operand, node);
    case ABSTRACT_SYNTAX_IF:
    case ABSTRACT_SYNTAX_IF_EXPRESSION: {
        if (node->childCount != 3 || node->children == NULL || node->children[0] == NULL || node->children[1] == NULL || node->children[2] == NULL || descriptorDataType(&node->children[0]->type) != DATA_TYPE_BOOLEAN) {
            return COMPILER_INTERNAL_ERROR;
        }

        DataType thenType = valueTypes(generator, &node->children[1]->type);
        DataType elseType = valueTypes(generator, &node->children[2]->type);
        DataType resultType = valueTypes(generator, &node->type);

        if (!canConvertValue(thenType, resultType) || !canConvertValue(elseType, resultType)) {
            return COMPILER_INTERNAL_ERROR;
        }

        IntermediateOperand operands[4] = {0};
        IntermediateOperand *resultOperand = &operands[0];
        IntermediateOperand *elseLabel = &operands[1];
        IntermediateOperand *endLabel = &operands[2];
        IntermediateOperand *conditionOperand = &operands[3];

        CompilerStatus status = generateTemporary(generator, resultOperand, node->location);
        if (status != COMPILER_OK) {
            clearOperandArray(operands, 4);
            return status;
        }

        status = generateLabel(generator, elseLabel);
        if (status != COMPILER_OK) {
            clearOperandArray(operands, 4);
            return status;
        }

        status = generateLabel(generator, endLabel);
        if (status != COMPILER_OK) {
            clearOperandArray(operands, 4);
            return status;
        }

        status = generateExpression(generator, conditionOperand, node->children[0]);
        if (status != COMPILER_OK) {
            clearOperandArray(operands, 4);
            return status;
        }

        IntermediateOperand falseOperand = {.kind = INTERMEDIATE_OPERAND_BOOLEAN, .value.boolValue = false};
        status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, node->location, 3, (IntermediateOperand[]){*elseLabel, *conditionOperand, falseOperand});
        if (status != COMPILER_OK) {
            clearOperandArray(operands, 4);
            return status;
        }

        size_t counter = 1;
        while (counter < 3) {
            const AbstractSyntaxNode *branch = node->children[counter];
            IntermediateOperand branchOperand = {0};
            status = generateBlockValue(generator, &branchOperand, branch);
            if (status != COMPILER_OK) {
                intermediateOperandClear(&branchOperand);
                clearOperandArray(operands, 4);
                return status;
            }

            status = convertValue(generator, &branchOperand, valueTypes(generator, &branch->type), resultType, branch->location);
            if (status != COMPILER_OK) {
                intermediateOperandClear(&branchOperand);
                clearOperandArray(operands, 4);
                return status;
            }

            status = appendControlInstruction(generator, INTERMEDIATE_MOVE, branch->location, 2, (IntermediateOperand[]){*resultOperand, branchOperand});
            intermediateOperandClear(&branchOperand);
            if (status != COMPILER_OK) {
                clearOperandArray(operands, 4);
                return status;
            }

            if (counter == 1) {
                status = appendControlInstruction(generator, INTERMEDIATE_JUMP, node->location, 1, endLabel);
                if (status != COMPILER_OK) {
                    clearOperandArray(operands, 4);
                    return status;
                }

                status = appendControlInstruction(generator, INTERMEDIATE_LABEL, node->location, 1, elseLabel);
                if (status != COMPILER_OK) {
                    clearOperandArray(operands, 4);
                    return status;
                }
            }

            counter++;
        }

        status = appendControlInstruction(generator, INTERMEDIATE_LABEL, node->location, 1, endLabel);
        if (status != COMPILER_OK) {
            clearOperandArray(operands, 4);
            return status;
        }

        *operand = *resultOperand;
        resultOperand->text = NULL;

        clearOperandArray(operands, 4);

        return status;
    }
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

        if (strcmp(node->text, "++") == 0) {
            return generateListConcat(generator, operand, node);
        }

        bool isAnd = strcmp(node->text, "&&") == 0;
        bool isOr = strcmp(node->text, "||") == 0;
        if (isAnd || isOr) {
            const AbstractSyntaxNode *left = node->children[0];
            const AbstractSyntaxNode *right = node->children[1];
            if (descriptorDataType(&left->type) != DATA_TYPE_BOOLEAN || descriptorDataType(&right->type) != DATA_TYPE_BOOLEAN) {
                return COMPILER_INTERNAL_ERROR;
            }

            IntermediateOperand leftOperand = {0};
            CompilerStatus status = generateExpression(generator, &leftOperand, left);
            if (status != COMPILER_OK) {
                return status;
            }

            IntermediateOperand resultOperand = {0};
            status = generateTemporary(generator, &resultOperand, node->location);
            if (status != COMPILER_OK) {
                intermediateOperandClear(&leftOperand);
                return status;
            }

            IntermediateOperand endLabel = {0};
            status = generateLabel(generator, &endLabel);
            if (status != COMPILER_OK) {
                intermediateOperandClear(&leftOperand);
                intermediateOperandClear(&resultOperand);
                return status;
            }

            IntermediateOperand shortCircuitOperand = {.kind = INTERMEDIATE_OPERAND_BOOLEAN, .value.boolValue = isOr};

            IntermediateInstruction leftInstruction = fillInstruction(INTERMEDIATE_MOVE, 2, node->location);
            leftInstruction.operands[0] = resultOperand;
            leftInstruction.operands[1] = leftOperand;

            IntermediateInstruction jumpInstruction = fillInstruction(INTERMEDIATE_JUMP_IF_EQUAL, 3, node->location);
            jumpInstruction.operands[0] = endLabel;
            jumpInstruction.operands[1] = resultOperand;
            jumpInstruction.operands[2] = shortCircuitOperand;

            IntermediateInstruction leftInstructions[] = {leftInstruction, jumpInstruction};
            status = appendInstructionArray(generator, leftInstructions, sizeof(leftInstructions) / sizeof(leftInstructions[0]));
            intermediateOperandClear(&leftOperand);
            if (status != COMPILER_OK) {
                intermediateOperandClear(&endLabel);
                intermediateOperandClear(&resultOperand);
                return status;
            }

            IntermediateOperand rightOperand = {0};
            status = generateExpression(generator, &rightOperand, right);
            if (status != COMPILER_OK) {
                intermediateOperandClear(&endLabel);
                intermediateOperandClear(&resultOperand);
                return status;
            }

            IntermediateInstruction rightInstruction = fillInstruction(INTERMEDIATE_MOVE, 2, node->location);
            rightInstruction.operands[0] = resultOperand;
            rightInstruction.operands[1] = rightOperand;

            IntermediateInstruction endInstruction = fillInstruction(INTERMEDIATE_LABEL, 1, node->location);
            endInstruction.operands[0] = endLabel;

            IntermediateInstruction rightInstructions[] = {rightInstruction, endInstruction};
            status = appendInstructionArray(generator, rightInstructions, sizeof(rightInstructions) / sizeof(rightInstructions[0]));
            intermediateOperandClear(&rightOperand);
            intermediateOperandClear(&endLabel);
            if (status != COMPILER_OK) {
                intermediateOperandClear(&resultOperand);
                return status;
            }

            *operand = resultOperand;

            return status;
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
            if (descriptorDataType(&node->children[0]->type) == DATA_TYPE_INT && descriptorDataType(&node->children[1]->type) == DATA_TYPE_INT) {
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
        DataType leftType = descriptorDataType(&node->children[0]->type);

        IntermediateOperand rightOperand = {0};
        status = generateExpression(generator, &rightOperand, node->children[1]);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&leftOperand);
            return status;
        }
        DataType rightType = descriptorDataType(&node->children[1]->type);

        bool isComparison = operationCode == INTERMEDIATE_COMPARE_EQUAL || operationCode == INTERMEDIATE_COMPARE_LESS || operationCode == INTERMEDIATE_COMPARE_GREATER;

        if (leftType == DATA_TYPE_UNIT || rightType == DATA_TYPE_UNIT || leftType == DATA_TYPE_BOOLEAN || rightType == DATA_TYPE_BOOLEAN) {
            if (leftType != rightType || operationCode != INTERMEDIATE_COMPARE_EQUAL) {
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
        if (node->childCount != 1 || node->children == NULL || node->children[0] == NULL || node->text == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        DataType valueType = descriptorDataType(&node->children[0]->type);
        bool isNot = strcmp(node->text, "!") == 0;
        bool isNegation = strcmp(node->text, "-") == 0;
        if (!(isNot && valueType == DATA_TYPE_BOOLEAN) && !(isNegation && isNumericType(valueType))) {
            return COMPILER_INTERNAL_ERROR;
        }

        IntermediateOperand valueOperand = {0};
        CompilerStatus status = generateExpression(generator, &valueOperand, node->children[0]);
        if (status != COMPILER_OK) {
            return status;
        }

        IntermediateOperand resultOperand = {0};
        status = generateTemporary(generator, &resultOperand, node->location);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&valueOperand);
            return status;
        }

        IntermediateOpcode operationCode = isNot ? INTERMEDIATE_NOT : INTERMEDIATE_MULTIPLY;
        size_t operandCount = isNot ? 2 : 3;
        IntermediateInstruction instruction = fillInstruction(operationCode, operandCount, node->location);
        instruction.operands[0] = resultOperand;
        instruction.operands[1] = valueOperand;
        if (!isNot) {
            if (valueType == DATA_TYPE_INT) {
                instruction.operands[2].kind = INTERMEDIATE_OPERAND_INT;
                instruction.operands[2].value.intValue = -1;
            } else {
                instruction.operands[2].kind = INTERMEDIATE_OPERAND_DOUBLE;
                instruction.operands[2].value.doubleValue = -1.0;
            }
        }

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
    case ABSTRACT_SYNTAX_CALL_EXPRESSION: {
        if (node->childCount == 0 || node->children == NULL || node->children[0] == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        const AbstractSyntaxNode *callee = node->children[0];
        if (callee->kind != ABSTRACT_SYNTAX_IDENTIFIER || valueTypes(generator, &callee->type) == DATA_TYPE_LIST) {
            return generateListCall(generator, operand, node);
        }
        if (callee->text == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        const char *functionName = callee->text;
        size_t argumentCount = node->childCount - 1;
        AbstractSyntaxNode *const *arguments = node->children + 1;

        if (strcmp(functionName, "List") == 0) {
            return generateListConstructor(generator, operand, node);
        }

        bool isDoubleMaybe = strcmp(functionName, "readDoubleMaybe") == 0;
        if (strcmp(functionName, "readIntMaybe") == 0 || isDoubleMaybe) {
            if (argumentCount != 0) {
                return COMPILER_INTERNAL_ERROR;
            }
            return generateOptionalRead(generator, operand, node->location, isDoubleMaybe);
        }

        if (strcmp(functionName, "print") == 0) {
            if (argumentCount != 1 || arguments[0] == NULL) {
                return COMPILER_INTERNAL_ERROR;
            }

            const AbstractSyntaxNode *argument = arguments[0];
            DataType argumentType = valueTypes(generator, &argument->type);
            if (!isValueType(argumentType) || (argumentType & DATA_TYPE_CASE_CLASS) || ((argumentType & DATA_TYPE_LIST) && argument->type.recursiveTypes == NULL)) {
                return COMPILER_INTERNAL_ERROR;
            }

            IntermediateOperand argumentOperand = {0};

            CompilerStatus status = generateExpression(generator, &argumentOperand, argument);
            if (status != COMPILER_OK) {
                return status;
            }

            IntermediateInstruction printInstruction = fillInstruction(INTERMEDIATE_WRITE, 1, node->location);
            printInstruction.operands[0] = argumentOperand;

            if (argumentType == DATA_TYPE_UNIT) {
                printInstruction.operands[0] = (IntermediateOperand){
                    .kind = INTERMEDIATE_OPERAND_STRING,
                    .text = "()",
                };
            }

            if ((argumentType & DATA_TYPE_LIST) || ((argumentType & DATA_TYPE_UNIT) && argumentType != DATA_TYPE_UNIT)) {
                status = generateUnionPrint(generator, &argumentOperand, argumentType, node->location);
            } else {
                status = intermediateProgramAppend(generator->program, &printInstruction);
            }

            operand->kind = INTERMEDIATE_OPERAND_UNIT;
            operand->text = NULL;
            operand->value = (IntermediateOperandValue){0};

            intermediateOperandClear(&argumentOperand);

            return status;
        } else if (strcmp(functionName, "readLine") == 0 || strcmp(functionName, "readInt") == 0 || strcmp(functionName, "readDouble") == 0 || strcmp(functionName, "readBoolean") == 0) {
            if (argumentCount != 0) {
                return COMPILER_INTERNAL_ERROR;
            }

            DataType readType = {0};

            if (strcmp(functionName, "readLine") == 0) {
                readType = DATA_TYPE_STRING;
            }

            if (strcmp(functionName, "readInt") == 0) {
                readType = DATA_TYPE_INT;
            }

            if (strcmp(functionName, "readDouble") == 0) {
                readType = DATA_TYPE_DOUBLE;
            }

            if (strcmp(functionName, "readBoolean") == 0) {
                readType = DATA_TYPE_BOOLEAN;
            }

            IntermediateOperand resultOperand = {0};
            CompilerStatus status = generateTemporary(generator, &resultOperand, node->location);
            if (status != COMPILER_OK) {
                return status;
            }

            IntermediateOperand typeOperand = {0};
            typeOperand.kind = INTERMEDIATE_OPERAND_TYPE;
            typeOperand.value.dataType = readType;

            IntermediateInstruction readInstruction = fillInstruction(INTERMEDIATE_READ, 2, node->location);
            readInstruction.operands[0] = resultOperand;
            readInstruction.operands[1] = typeOperand;

            status = intermediateProgramAppend(generator->program, &readInstruction);
            if (status == COMPILER_OK && isNumericType(readType)) {
                status = generateNumericReadDefault(generator, &resultOperand, readType, node->location);
            }
            if (status != COMPILER_OK) {
                intermediateOperandClear(&resultOperand);
                return status;
            }

            *operand = resultOperand;

            return status;
        } else if (strcmp(functionName, "toInt") == 0) {
            if (argumentCount != 1 || arguments[0] == NULL) {
                return COMPILER_INTERNAL_ERROR;
            }

            const AbstractSyntaxNode *argument = arguments[0];
            DataType argumentType = valueTypes(generator, &argument->type);
            if (!canConvertValue(argumentType, DATA_TYPE_DOUBLE)) {
                return COMPILER_INTERNAL_ERROR;
            }

            IntermediateOperand argumentOperand = {0};
            CompilerStatus status = generateExpression(generator, &argumentOperand, argument);
            if (status != COMPILER_OK) {
                return status;
            }

            status = convertValue(generator, &argumentOperand, argumentType, DATA_TYPE_DOUBLE, node->location);
            if (status != COMPILER_OK) {
                intermediateOperandClear(&argumentOperand);
                return status;
            }

            IntermediateOperand resultOperand = {0};
            status = generateTemporary(generator, &resultOperand, node->location);
            if (status != COMPILER_OK) {
                intermediateOperandClear(&argumentOperand);
                return status;
            }

            IntermediateInstruction conversionInstruction = fillInstruction(INTERMEDIATE_CONVERT_DOUBLE_TO_INT, 2, node->location);
            conversionInstruction.operands[0] = resultOperand;
            conversionInstruction.operands[1] = argumentOperand;

            status = intermediateProgramAppend(generator->program, &conversionInstruction);
            intermediateOperandClear(&argumentOperand);
            if (status != COMPILER_OK) {
                intermediateOperandClear(&resultOperand);
                return status;
            }

            *operand = resultOperand;

            return status;
        } else if (strcmp(functionName, "toStr") == 0) {
            if (argumentCount != 1 || arguments[0] == NULL) {
                return COMPILER_INTERNAL_ERROR;
            }

            const AbstractSyntaxNode *argument = arguments[0];
            DataType argumentType = valueTypes(generator, &argument->type);
            if (!isValueType(argumentType) || (argumentType & DATA_TYPE_CASE_CLASS) || ((argumentType & DATA_TYPE_LIST) && argument->type.recursiveTypes == NULL)) {
                return COMPILER_INTERNAL_ERROR;
            }

            IntermediateOperand argumentOperand = {0};
            CompilerStatus status = generateExpression(generator, &argumentOperand, argument);
            if (status != COMPILER_OK) {
                return status;
            }

            if (argumentType == DATA_TYPE_STRING) {
                *operand = argumentOperand;

                return status;
            }

            if (argumentType == DATA_TYPE_UNIT) {
                intermediateOperandClear(&argumentOperand);

                char *unitText = malloc(3 * sizeof(char));
                if (unitText == NULL) {
                    return COMPILER_INTERNAL_ERROR;
                }

                strcpy(unitText, "()");

                argumentOperand.kind = INTERMEDIATE_OPERAND_STRING;
                argumentOperand.text = unitText;
                argumentOperand.value = (IntermediateOperandValue){0};

                *operand = argumentOperand;

                return status;
            }

            IntermediateOperand resultOperand = {0};
            status = generateTemporary(generator, &resultOperand, node->location);
            if (status != COMPILER_OK) {
                intermediateOperandClear(&argumentOperand);
                return status;
            }

            if (argumentType == DATA_TYPE_LIST) {
                status = generateListString(generator, &resultOperand, &argumentOperand, node->location);
            } else if (argumentType == DATA_TYPE_BOOLEAN) {
                status = generateBooleanString(generator, &resultOperand, &argumentOperand, node->location);
            } else if (argumentType != DATA_TYPE_INT && argumentType != DATA_TYPE_DOUBLE) {
                status = generateUnionString(generator, &resultOperand, &argumentOperand, argumentType, node->location);
            } else {
                IntermediateOpcode conversionOpcode = INTERMEDIATE_CONVERT_INT_TO_STRING;
                if (argumentType == DATA_TYPE_DOUBLE) {
                    conversionOpcode = INTERMEDIATE_CONVERT_DOUBLE_TO_STRING;
                }

                IntermediateInstruction conversionInstruction = fillInstruction(conversionOpcode, 2, node->location);
                conversionInstruction.operands[0] = resultOperand;
                conversionInstruction.operands[1] = argumentOperand;

                status = intermediateProgramAppend(generator->program, &conversionInstruction);
            }
            intermediateOperandClear(&argumentOperand);
            if (status != COMPILER_OK) {
                intermediateOperandClear(&resultOperand);
                return status;
            }

            *operand = resultOperand;

            return status;
        } else if (strcmp(functionName, "chr") == 0) {
            if (argumentCount != 1 || arguments[0] == NULL) {
                return COMPILER_INTERNAL_ERROR;
            }

            const AbstractSyntaxNode *argument = arguments[0];
            if (descriptorDataType(&argument->type) != DATA_TYPE_INT) {
                return COMPILER_INTERNAL_ERROR;
            }

            IntermediateOperand argumentOperand = {0};
            CompilerStatus status = generateExpression(generator, &argumentOperand, argument);
            if (status != COMPILER_OK) {
                return status;
            }

            IntermediateOperand resultOperand = {0};
            status = generateTemporary(generator, &resultOperand, node->location);
            if (status != COMPILER_OK) {
                intermediateOperandClear(&argumentOperand);
                return status;
            }

            IntermediateInstruction characterInstruction = fillInstruction(INTERMEDIATE_CONVERT_INT_TO_CHARACTER, 2, node->location);
            characterInstruction.operands[0] = resultOperand;
            characterInstruction.operands[1] = argumentOperand;

            status = intermediateProgramAppend(generator->program, &characterInstruction);
            intermediateOperandClear(&argumentOperand);
            if (status != COMPILER_OK) {
                intermediateOperandClear(&resultOperand);
                return status;
            }

            *operand = resultOperand;

            return status;
        } else if (strcmp(functionName, "strLength") == 0) {
            if (argumentCount != 1 || arguments[0] == NULL) {
                return COMPILER_INTERNAL_ERROR;
            }

            const AbstractSyntaxNode *argument = arguments[0];
            if (descriptorDataType(&argument->type) != DATA_TYPE_STRING) {
                return COMPILER_INTERNAL_ERROR;
            }

            IntermediateOperand argumentOperand = {0};
            CompilerStatus status = generateExpression(generator, &argumentOperand, argument);
            if (status != COMPILER_OK) {
                return status;
            }

            IntermediateOperand resultOperand = {0};
            status = generateTemporary(generator, &resultOperand, node->location);
            if (status != COMPILER_OK) {
                intermediateOperandClear(&argumentOperand);
                return status;
            }

            IntermediateInstruction lengthInstruction = fillInstruction(INTERMEDIATE_STRING_LENGTH, 2, node->location);
            lengthInstruction.operands[0] = resultOperand;
            lengthInstruction.operands[1] = argumentOperand;

            status = intermediateProgramAppend(generator->program, &lengthInstruction);
            intermediateOperandClear(&argumentOperand);
            if (status != COMPILER_OK) {
                intermediateOperandClear(&resultOperand);
                return status;
            }

            *operand = resultOperand;

            return status;
        }

        if (strcmp(functionName, "ord") == 0 || strcmp(functionName, "subStr") == 0) {
            bool isOrdinal = strcmp(functionName, "ord") == 0;
            if (argumentCount != (isOrdinal ? 2 : 3)) {
                return COMPILER_INTERNAL_ERROR;
            }

            size_t counter = 0;
            while (counter < argumentCount) {
                const AbstractSyntaxNode *argument = arguments[counter];
                DataType argumentType = counter == 0 ? DATA_TYPE_STRING : DATA_TYPE_INT;
                if (argument == NULL || descriptorDataType(&argument->type) != argumentType) {
                    return COMPILER_INTERNAL_ERROR;
                }

                counter++;
            }

            IntermediateOperand argumentOperands[3] = {0};
            counter = 0;
            while (counter < argumentCount) {
                CompilerStatus status = generateExpression(generator, &argumentOperands[counter], arguments[counter]);
                if (status != COMPILER_OK) {
                    clearOperandArray(argumentOperands, 3);
                    return status;
                }

                counter++;
            }

            IntermediateOperand resultOperand = {0};
            CompilerStatus status = generateTemporary(generator, &resultOperand, node->location);
            if (status != COMPILER_OK) {
                clearOperandArray(argumentOperands, 3);
                return status;
            }

            if (isOrdinal) {
                status = generateOrdinal(generator, &resultOperand, &argumentOperands[0], &argumentOperands[1], node->location);
            } else {
                status = generateSubstring(generator, &resultOperand, &argumentOperands[0], &argumentOperands[1], &argumentOperands[2], node->location);
            }
            clearOperandArray(argumentOperands, 3);
            if (status != COMPILER_OK) {
                intermediateOperandClear(&resultOperand);
                return status;
            }

            *operand = resultOperand;

            return status;
        }

        const AbstractSyntaxNode *declaration = findTypeDeclaration(generator->sourceProgram, functionName, ABSTRACT_SYNTAX_CASE_CLASS);
        if (declaration != NULL) {
            return generateClassConstructor(generator, operand, node, declaration);
        }

        const AbstractSyntaxNode *function = findFunction(generator->sourceProgram, functionName);
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

        if (argumentCount != parameterCount) {
            return COMPILER_INTERNAL_ERROR;
        }

        size_t counter = 0;

        while (counter < parameterCount) {
            const AbstractSyntaxNode *parameter = function->children[counter];
            const AbstractSyntaxNode *argument = arguments[counter];

            if (parameter == NULL || argument == NULL) {
                return COMPILER_INTERNAL_ERROR;
            }

            if (parameter->kind != ABSTRACT_SYNTAX_PARAMETER || parameter->variableId == 0) {
                return COMPILER_INTERNAL_ERROR;
            }

            if (!canConvertValue(valueTypes(generator, &argument->type), valueTypes(generator, &parameter->type))) {
                return COMPILER_INTERNAL_ERROR;
            }

            counter++;
        }

        counter = 0;

        while (counter < parameterCount) {
            const AbstractSyntaxNode *parameter = function->children[counter];
            const AbstractSyntaxNode *argument = arguments[counter];

            IntermediateOperand argumentOperand = {0};
            CompilerStatus status = generateExpression(generator, &argumentOperand, argument);
            if (status != COMPILER_OK) {
                return status;
            }

            status = convertValue(generator, &argumentOperand, valueTypes(generator, &argument->type), valueTypes(generator, &parameter->type), argument->location);
            if (status != COMPILER_OK) {
                intermediateOperandClear(&argumentOperand);
                return status;
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

/**
 * @brief generates code for an expression statement and discards its result
 * @param generator generator state
 * @param node source AST node
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateExpressionStatement(IntermediateGenerator *generator, const AbstractSyntaxNode *node) {
    if (generator == NULL || node == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

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

/**
 * @brief generates code to skip a for-loop iteration if a guard is false
 * @param generator generator state
 * @param clauses for-loop clauses
 * @param advanceLabel label for the next iteration
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateForGuards(IntermediateGenerator *generator, const AbstractSyntaxNode *clauses, const IntermediateOperand *advanceLabel) {
    size_t counter = 1;
    while (counter < clauses->childCount) {
        const AbstractSyntaxNode *guard = clauses->children[counter];
        if (guard == NULL || guard->kind != ABSTRACT_SYNTAX_FOR_GUARD || guard->childCount != 1 || guard->children == NULL || guard->children[0] == NULL || descriptorDataType(&guard->children[0]->type) != DATA_TYPE_BOOLEAN) {
            return COMPILER_INTERNAL_ERROR;
        }

        IntermediateOperand guardOperand = {0};
        CompilerStatus status = generateExpression(generator, &guardOperand, guard->children[0]);
        if (status != COMPILER_OK) {
            return status;
        }

        IntermediateOperand falseOperand = {.kind = INTERMEDIATE_OPERAND_BOOLEAN, .value.boolValue = false};
        status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, guard->location, 3, (IntermediateOperand[]){*advanceLabel, guardOperand, falseOperand});
        intermediateOperandClear(&guardOperand);
        if (status != COMPILER_OK) {
            return status;
        }

        counter++;
    }

    return COMPILER_OK;
}

/*
 * AI-generated code
 * ok could you please generate the for loop for a list function scaffold and oprands and checks in there, also please tell me how you think the instructions should be handled
 */
/**
 * @brief generates a loop that connects each list element, checks guards and runs the body
 * @param generator generator state
 * @param binding loop variable AST node
 * @param clauses for-loop clauses
 * @param body body AST node
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
static CompilerStatus generateListFor(IntermediateGenerator *generator, const AbstractSyntaxNode *binding, const AbstractSyntaxNode *clauses, const AbstractSyntaxNode *body) {
    SourceLocation origin = binding->location;
    IntermediateOperand list = {0};
    IntermediateOperand iterator = {0};
    IntermediateOperand length = {0};
    IntermediateOperand position = {0};
    IntermediateOperand condition = {0};
    IntermediateOperand loop = {0};
    IntermediateOperand advance = {0};
    IntermediateOperand end = {0};
    IntermediateOperand *ownedOperands[] = {&list, &iterator, &length, &position, &condition, &loop, &advance, &end};
    size_t ownedOperandCount = sizeof(ownedOperands) / sizeof(ownedOperands[0]);

    CompilerStatus status = generateTemporary(generator, &list, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateRangeValue(generator, &list, binding->children[0]);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateVariable(&iterator, binding);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendVariableDeclaration(generator, &iterator, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(generator, &length, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(generator, &position, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateTemporary(generator, &condition, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateLabel(generator, &loop);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateLabel(generator, &advance);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateLabel(generator, &end);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateListLength(generator, &length, &list, origin);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    IntermediateOperand no = {.kind = INTERMEDIATE_OPERAND_BOOLEAN, .value.boolValue = false};
    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){length, length, integerOperand(2)});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){position, integerOperand(2)});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &loop);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_COMPARE_LESS, origin, 3, (IntermediateOperand[]){condition, position, length});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){end, condition, no});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_ARRAY_GET, origin, 3, (IntermediateOperand[]){iterator, list, position});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    DataType target = valueTypes(generator, &binding->type);
    if ((target & DATA_TYPE_DOUBLE) && !(target & DATA_TYPE_INT)) {
        IntermediateOperand runtimeValue = {.kind = iterator.kind, .text = memoryDuplicateString(iterator.text)};
        if (runtimeValue.text == NULL) {
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return COMPILER_INTERNAL_ERROR;
        }

        status = convertValue(generator, &runtimeValue, target | DATA_TYPE_INT, target, origin);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&runtimeValue);
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }

        status = appendControlInstruction(generator, INTERMEDIATE_MOVE, origin, 2, (IntermediateOperand[]){iterator, runtimeValue});
        if (status != COMPILER_OK) {
            intermediateOperandClear(&runtimeValue);
            clearOperandPointers(ownedOperands, ownedOperandCount);
            return status;
        }
        intermediateOperandClear(&runtimeValue);
    }

    status = generateForGuards(generator, clauses, &advance);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = generateStatement(generator, body);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &advance);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){position, position, integerOperand(1)});
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_JUMP, origin, 1, &loop);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    status = appendControlInstruction(generator, INTERMEDIATE_LABEL, origin, 1, &end);
    if (status != COMPILER_OK) {
        clearOperandPointers(ownedOperands, ownedOperandCount);
        return status;
    }

    clearOperandPointers(ownedOperands, ownedOperandCount);

    return COMPILER_OK;
}

/**
 * @brief generates intermediate code for a statement
 * @param generator generator state
 * @param node source AST node
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
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

        status = convertAssignedValue(generator, &initializerOperand, &node->children[0]->type, &node->type, node->location);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&initializerOperand);
            return status;
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

        status = convertAssignedValue(generator, &valueOperand, &node->children[0]->type, &node->type, node->location);
        if (status != COMPILER_OK) {
            intermediateOperandClear(&valueOperand);
            return status;
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

        if (descriptorDataType(&node->children[0]->type) != DATA_TYPE_BOOLEAN) {
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
    case ABSTRACT_SYNTAX_FOR: {
        if (node->childCount != 2 || node->children == NULL || node->children[0] == NULL || node->children[1] == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        const AbstractSyntaxNode *clauses = node->children[0];
        if (clauses->kind != ABSTRACT_SYNTAX_FOR_CLAUSES || clauses->childCount == 0 || clauses->children == NULL || clauses->children[0] == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        const AbstractSyntaxNode *binding = clauses->children[0];
        if (binding->kind != ABSTRACT_SYNTAX_FOR_GENERATOR || binding->childCount != 1 || binding->children == NULL || binding->children[0] == NULL || binding->variableId == 0) {
            return COMPILER_INTERNAL_ERROR;
        }

        const AbstractSyntaxNode *range = binding->children[0];
        if (valueTypes(generator, &range->type) == DATA_TYPE_LIST) {
            return generateListFor(generator, binding, clauses, node->children[1]);
        }
        if (descriptorDataType(&binding->type) != DATA_TYPE_INT || range->kind != ABSTRACT_SYNTAX_RANGE_EXPRESSION || range->text == NULL || (range->childCount != 2 && range->childCount != 3) || range->children == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        bool isInclusive = strcmp(range->text, "to") == 0;
        if (!isInclusive && strcmp(range->text, "until") != 0) {
            return COMPILER_INTERNAL_ERROR;
        }

        size_t counter = 0;
        while (counter < range->childCount) {
            const AbstractSyntaxNode *part = range->children[counter];
            if (part == NULL || descriptorDataType(&part->type) != DATA_TYPE_INT) {
                return COMPILER_INTERNAL_ERROR;
            }

            counter++;
        }

        IntermediateOperand operands[11] = {0};
        IntermediateOperand *iteratorOperand = &operands[0];
        IntermediateOperand *endOperand = &operands[1];
        IntermediateOperand *stepOperand = &operands[2];
        IntermediateOperand *conditionOperand = &operands[3];
        IntermediateOperand *positiveOperand = &operands[4];
        IntermediateOperand *stepLabel = &operands[5];
        IntermediateOperand *loopLabel = &operands[6];
        IntermediateOperand *bodyLabel = &operands[7];
        IntermediateOperand *positiveLabel = &operands[8];
        IntermediateOperand *advanceLabel = &operands[9];
        IntermediateOperand *endLabel = &operands[10];
        IntermediateOperand *temporaries[] = {conditionOperand, positiveOperand};
        IntermediateOperand *labels[] = {stepLabel, loopLabel, bodyLabel, positiveLabel, advanceLabel, endLabel};
        SourceLocation origin = node->location;

        CompilerStatus status = generateVariable(iteratorOperand, binding);
        if (status != COMPILER_OK) {
            clearOperandArray(operands, 11);
            return status;
        }

        status = appendVariableDeclaration(generator, iteratorOperand, binding->location);
        if (status != COMPILER_OK) {
            clearOperandArray(operands, 11);
            return status;
        }

        status = generateRangeValue(generator, iteratorOperand, range->children[0]);
        if (status != COMPILER_OK) {
            clearOperandArray(operands, 11);
            return status;
        }

        status = generateTemporary(generator, endOperand, range->children[1]->location);
        if (status != COMPILER_OK) {
            clearOperandArray(operands, 11);
            return status;
        }

        status = generateRangeValue(generator, endOperand, range->children[1]);
        if (status != COMPILER_OK) {
            clearOperandArray(operands, 11);
            return status;
        }

        stepOperand->kind = INTERMEDIATE_OPERAND_INT;
        stepOperand->value.intValue = 1;
        if (range->childCount == 3) {
            status = generateTemporary(generator, stepOperand, range->children[2]->location);
            if (status != COMPILER_OK) {
                clearOperandArray(operands, 11);
                return status;
            }

            status = generateRangeValue(generator, stepOperand, range->children[2]);
            if (status != COMPILER_OK) {
                clearOperandArray(operands, 11);
                return status;
            }
        }

        counter = 0;
        while (counter < sizeof(temporaries) / sizeof(temporaries[0])) {
            status = generateTemporary(generator, temporaries[counter], origin);
            if (status != COMPILER_OK) {
                clearOperandArray(operands, 11);
                return status;
            }

            counter++;
        }

        counter = 0;
        while (counter < sizeof(labels) / sizeof(labels[0])) {
            status = generateLabel(generator, labels[counter]);
            if (status != COMPILER_OK) {
                clearOperandArray(operands, 11);
                return status;
            }

            counter++;
        }

        IntermediateOperand zeroOperand = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 0};
        IntermediateOperand errorOperand = {.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = 40};
        IntermediateOperand trueOperand = {.kind = INTERMEDIATE_OPERAND_BOOLEAN, .value.boolValue = true};

        if (range->childCount == 3) {
            status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_NOT_EQUAL, origin, 3, (IntermediateOperand[]){*stepLabel, *stepOperand, zeroOperand});
            if (status != COMPILER_OK) {
                clearOperandArray(operands, 11);
                return status;
            }

            status = appendControlInstruction(generator, INTERMEDIATE_EXIT, origin, 1, &errorOperand);
            if (status != COMPILER_OK) {
                clearOperandArray(operands, 11);
                return status;
            }
        }

        IntermediateInstruction loopInstructions[] = {
            controlInstruction(INTERMEDIATE_LABEL, origin, 1, stepLabel), controlInstruction(INTERMEDIATE_COMPARE_LESS, origin, 3, (IntermediateOperand[]){*positiveOperand, zeroOperand, *stepOperand}), controlInstruction(INTERMEDIATE_LABEL, origin, 1, loopLabel), controlInstruction(INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){*positiveLabel, *positiveOperand, trueOperand}), controlInstruction(INTERMEDIATE_COMPARE_LESS, origin, 3, (IntermediateOperand[]){*conditionOperand, *iteratorOperand, *endOperand}), controlInstruction(INTERMEDIATE_JUMP, origin, 1, bodyLabel), controlInstruction(INTERMEDIATE_LABEL, origin, 1, positiveLabel), controlInstruction(INTERMEDIATE_COMPARE_LESS, origin, 3, (IntermediateOperand[]){*conditionOperand, *endOperand, *iteratorOperand}), controlInstruction(INTERMEDIATE_LABEL, origin, 1, bodyLabel), controlInstruction(INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){*endLabel, *conditionOperand, trueOperand}),
        };
        status = appendInstructionArray(generator, loopInstructions, sizeof(loopInstructions) / sizeof(loopInstructions[0]));
        if (status != COMPILER_OK) {
            clearOperandArray(operands, 11);
            return status;
        }

        if (!isInclusive) {
            status = appendControlInstruction(generator, INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){*endLabel, *iteratorOperand, *endOperand});
            if (status != COMPILER_OK) {
                clearOperandArray(operands, 11);
                return status;
            }
        }

        status = generateForGuards(generator, clauses, advanceLabel);
        if (status != COMPILER_OK) {
            clearOperandArray(operands, 11);
            return status;
        }

        status = generateStatement(generator, node->children[1]);
        if (status != COMPILER_OK) {
            clearOperandArray(operands, 11);
            return status;
        }

        IntermediateInstruction advanceInstructions[] = {
            controlInstruction(INTERMEDIATE_LABEL, origin, 1, advanceLabel), controlInstruction(INTERMEDIATE_JUMP_IF_EQUAL, origin, 3, (IntermediateOperand[]){*endLabel, *iteratorOperand, *endOperand}), controlInstruction(INTERMEDIATE_ADD, origin, 3, (IntermediateOperand[]){*iteratorOperand, *iteratorOperand, *stepOperand}), controlInstruction(INTERMEDIATE_JUMP, origin, 1, loopLabel), controlInstruction(INTERMEDIATE_LABEL, origin, 1, endLabel),
        };
        status = appendInstructionArray(generator, advanceInstructions, sizeof(advanceInstructions) / sizeof(advanceInstructions[0]));

        clearOperandArray(operands, 11);

        return status;
    }
    case ABSTRACT_SYNTAX_WHILE: {
        if (node->childCount != 2 || node->children == NULL || node->children[0] == NULL || node->children[1] == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        if (descriptorDataType(&node->children[0]->type) != DATA_TYPE_BOOLEAN) {
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

        if (!isValueType(generator->returnType)) {
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
            status = convertValue(generator, &returnOperand, valueTypes(generator, &node->children[0]->type), generator->returnType, node->location);
            if (status != COMPILER_OK) {
                intermediateOperandClear(&returnOperand);
                return status;
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
    case ABSTRACT_SYNTAX_CALL_EXPRESSION: {
        return generateExpressionStatement(generator, node);
    }
    case ABSTRACT_SYNTAX_EXPRESSION_STATEMENT: {
        return generateExpressionStatement(generator, node);
    }
    default:
        return COMPILER_INTERNAL_ERROR;
    }
}

/**
 * @brief declares parameters and pops their arguments in reverse order
 * @param generator generator state
 * @param function function AST node
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
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

        if (!isValueType(valueTypes(generator, &parameter->type))) {
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

/**
 * @brief generates code for a function body and pushes any implicit return value
 * @param generator generator state
 * @param body body AST node
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
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
    } else if (lastStatement->kind == ABSTRACT_SYNTAX_CALL_EXPRESSION || lastStatement->kind == ABSTRACT_SYNTAX_IF_EXPRESSION || (lastStatement->kind == ABSTRACT_SYNTAX_IF && isValueType(valueTypes(generator, &lastStatement->type)) && valueTypes(generator, &lastStatement->type) != DATA_TYPE_UNIT)) {
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

    status = convertValue(generator, &returnOperand, valueTypes(generator, &expressionNode->type), generator->returnType, expressionNode->location);
    if (status != COMPILER_OK) {
        intermediateOperandClear(&returnOperand);
        return status;
    }

    IntermediateInstruction returnValueInstruction = fillInstruction(INTERMEDIATE_PUSH, 1, expressionNode->location);
    returnValueInstruction.operands[0] = returnOperand;

    status = intermediateProgramAppend(generator->program, &returnValueInstruction);
    intermediateOperandClear(&returnOperand);

    return status;
}

/**
 * @brief generates the function label, frame setup, declarations, body and return
 * @param generator generator state
 * @param node source AST node
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
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
    functionGenerator.returnType = valueTypes(generator, &node->type);

    CompilerStatus status = generateParameters(&functionGenerator, node);
    if (status != COMPILER_OK) {
        intermediateProgramDestroy(functionBody);
        intermediateProgramDestroy(functionDeclarations);
        return status;
    }

    status = generateFunctionBody(&functionGenerator, body);

    generator->temporaryCounter = functionGenerator.temporaryCounter;
    generator->labelCounter = functionGenerator.labelCounter;
    generator->needsListString = functionGenerator.needsListString;

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

/**
 * @brief generates the startup instructions, functions and any required list-to-string helper
 * @param program typed source program
 * @param outProgram destination intermediate program
 * @return COMPILER_OK or COMPILER_INTERNAL_ERROR
 */
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

        if (function == NULL || function->text == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }
        if (function->kind == ABSTRACT_SYNTAX_TYPE_ALIAS || function->kind == ABSTRACT_SYNTAX_CASE_CLASS) {
            if (function->kind == ABSTRACT_SYNTAX_CASE_CLASS) {
                if (function->childCount != 0 && function->children == NULL) {
                    return COMPILER_INTERNAL_ERROR;
                }
                size_t fieldIndex = 0;
                while (fieldIndex < function->childCount) {
                    const AbstractSyntaxNode *field = function->children[fieldIndex++];
                    if (field == NULL || field->kind != ABSTRACT_SYNTAX_CASE_CLASS_FIELD || field->text == NULL) {
                        return COMPILER_INTERNAL_ERROR;
                    }
                }
            }
            counter++;
            continue;
        }
        if (function->kind != ABSTRACT_SYNTAX_FUNCTION) {
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

    if (entryFunction == NULL || entryFunction->childCount != 1 || descriptorDataType(&entryFunction->type) != DATA_TYPE_UNIT) {
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
        const AbstractSyntaxNode *node = program->root->children[counter];
        if (node->kind == ABSTRACT_SYNTAX_FUNCTION) {
            status = generateFunction(&generator, node);
            if (status != COMPILER_OK) {
                return status;
            }
        }

        counter++;
    }

    if (generator.needsListString) {
        return generateListStringFunction(&generator, entryFunction->location);
    }
    return COMPILER_OK;
}
// Boeing
