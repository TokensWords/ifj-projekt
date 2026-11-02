/*
 * Project: IFJ26 compiler
 * Authors: Václav Krytinář xkrytiv00
 * AI-assisted scaffold.
 * Prompt: Create an optional optimizer integration stub.
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
extern char* strdup(const char*);

#include "intermediateCode.h"

#include "compilerTypes.h"
#include "memory.h"

#include "optimizer.h"
#include "optimizerSymTable.h"
#include "knownConstantsTable.h"

#define OPTIMIZER_MAX_INT 64
#define OPTIMIZER_MAX_DOUBLE 64


CompilerStatus optimizerRun(IntermediateProgram *program)
{
    if (program == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }


    size_t start = 0;
    size_t end = (size_t)-1;

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
    if (program->instructions[*end].opcode == INTERMEDIATE_LABEL) {
        ++(*end);
        if (*end == program->count) {
            return false; // label last
        }
    }

    while (program->count - 1 != *end
        && program->instructions[*end].opcode != INTERMEDIATE_LABEL
        && program->instructions[*end].opcode != INTERMEDIATE_JUMP
        && program->instructions[*end].opcode != INTERMEDIATE_JUMP_IF_EQUAL
        && program->instructions[*end].opcode != INTERMEDIATE_JUMP_IF_NOT_EQUAL
        && program->instructions[*end].opcode != INTERMEDIATE_RETURN
        && program->instructions[*end].opcode != INTERMEDIATE_EXIT)
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
    bool changed = true;

    while(changed) {
        changed = false;

        status = optimizerBackwardPass(basicBlockTable, start, end, program);
        if (status != COMPILER_OK) {
            return status;
        }

        status = optimizerForwardPass(basicBlockTable, start, end, program, &changed);
        if (status != COMPILER_OK) {
            return status;
        }
    }

    return COMPILER_OK;
}


typedef enum {
    OPTIMIZER_EVAL0 = 1,
    OPTIMIZER_EVAL1 = 1 << 1,
    OPTIMIZER_EVAL2 = 1 << 2,
    OPTIMIZER_LIVE0 = 1 << 3,
    OPTIMIZER_LIVE1 = 1 << 4,
    OPTIMIZER_LIVE2 = 1 << 5
} OptimizerOpcodeFlags;

typedef enum {
    OPTIMIZER_THREE_OPERAND = OPTIMIZER_EVAL0 | OPTIMIZER_EVAL1 | OPTIMIZER_EVAL2 | OPTIMIZER_LIVE1 | OPTIMIZER_LIVE2,
    OPTIMIZER_TWO_OPERAND = OPTIMIZER_EVAL0 | OPTIMIZER_EVAL1 | OPTIMIZER_LIVE1,
    OPTIMIZER_ONE_WRITE = OPTIMIZER_EVAL0,
    OPTIMIZER_ONE_READ = OPTIMIZER_EVAL0 | OPTIMIZER_LIVE0,
    OPTIMIZER_JUMP_READ = OPTIMIZER_EVAL1 | OPTIMIZER_EVAL2 | OPTIMIZER_LIVE1 | OPTIMIZER_LIVE2
} OptimizerOpcodeCategories;

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
            return OPTIMIZER_THREE_OPERAND; // 0 written; 1,2 read

        case INTERMEDIATE_MOVE:
        case INTERMEDIATE_NOT:
        case INTERMEDIATE_CONVERT_INT_TO_DOUBLE:
        case INTERMEDIATE_CONVERT_DOUBLE_TO_INT:
        case INTERMEDIATE_CONVERT_INT_TO_STRING:
        case INTERMEDIATE_CONVERT_DOUBLE_TO_STRING:
        case INTERMEDIATE_CONVERT_INT_TO_CHARACTER:
        case INTERMEDIATE_STRING_LENGTH:
            return OPTIMIZER_TWO_OPERAND; // 0 written; 1 read

        case INTERMEDIATE_POP:
        case INTERMEDIATE_READ:
        case INTERMEDIATE_DEFINE_VARIABLE:
            return OPTIMIZER_ONE_WRITE; // 0 written

        case INTERMEDIATE_WRITE:
        case INTERMEDIATE_PUSH:
        case INTERMEDIATE_EXIT:
            return OPTIMIZER_ONE_READ; // 0 read

        case INTERMEDIATE_JUMP_IF_EQUAL:
        case INTERMEDIATE_JUMP_IF_NOT_EQUAL:
            return OPTIMIZER_JUMP_READ; // 1,2 read

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
    if (symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    uint8_t flags = 0;

    int64_t current = (int64_t)end;

    while (current >= (int64_t)start) {
        OptimizerSymbol *symbol;

        flags = optimizerGetOpcodeFlags(program->instructions[current].opcode);

        optimizerBbtTableSetDefault(bbtTable, current - start, current);

        for (size_t i = 0; i < 3; ++i) {
            IntermediateOperand *curretnOperandPtr = &program->instructions[current].operands[i];

            if (flags & 1 << i && (*curretnOperandPtr).kind == INTERMEDIATE_OPERAND_VARIABLE) {
                if ((symbol = optimizerSymTableFind(symbols, (*curretnOperandPtr).text)) != NULL) {
                    bbtTable[current-start].live[i] = symbol->live;
                    bbtTable[current-start].nextUse[i] = symbol->nextUse;
                } else if (strncmp("$variable", (*curretnOperandPtr).text, 9) == 0) {
                    bbtTable[current-start].live[i] = true;
                }

                if (symbol == NULL) {
                    symbol = memoryAllocate(sizeof(OptimizerSymbol));
                    if (symbol == NULL) {
                        optimizerSymTableDestroy(symbols);
                        return COMPILER_INTERNAL_ERROR;
                    }

                    symbol->name = (*curretnOperandPtr).text;

                    CompilerStatus status = optimizerSymTableInsert(symbols, symbol);
                    if (status != COMPILER_OK) {
                        free(symbol);
                        optimizerSymTableDestroy(symbols);
                        return status;
                    }
                }

                symbol->live = flags & 1 << (i+3);
                symbol->nextUse = current;
            }
        }

        --current;
    }

    optimizerSymTableDestroy(symbols);

    return COMPILER_OK;
}


bool optimizerIsOperandConst(IntermediateOperand operand)
{
    switch (operand.kind)
    {
    case INTERMEDIATE_OPERAND_INT:
    case INTERMEDIATE_OPERAND_DOUBLE:
    case INTERMEDIATE_OPERAND_STRING:
    case INTERMEDIATE_OPERAND_BOOLEAN:
        return true;
    default:
        return false;
    }
}

CompilerStatus optimizerConstantFold(IntermediateInstruction *instruction, uint8_t flags)
{
    if (instruction == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    IntermediateOpcode originalOpcode = instruction->opcode;
    IntermediateOperand *left = &instruction->operands[1];
    IntermediateOperand *right = &instruction->operands[2];

    if ((flags & OPTIMIZER_THREE_OPERAND) == OPTIMIZER_THREE_OPERAND) {
        int64_t leftInt = left->value.intValue;
        int64_t rightInt = right->value.intValue;
        double leftDouble = left->value.doubleValue;
        double rightDouble = right->value.doubleValue;
        bool leftBool = left->value.boolValue;
        bool rightBool = right->value.boolValue;

        switch (originalOpcode) {
            case INTERMEDIATE_ADD:
                if (left->kind == INTERMEDIATE_OPERAND_INT) {
                    left->value.intValue = leftInt + rightInt;
                } else {
                    left->value.doubleValue = leftDouble + rightDouble;
                }
                break;
            case INTERMEDIATE_SUBTRACT:
                if (left->kind == INTERMEDIATE_OPERAND_INT) {
                    left->value.intValue = leftInt - rightInt;
                } else {
                    left->value.doubleValue = leftDouble - rightDouble;
                }
                break;
            case INTERMEDIATE_MULTIPLY:
                if (left->kind == INTERMEDIATE_OPERAND_INT) {
                    left->value.intValue = leftInt * rightInt;
                } else {
                    left->value.doubleValue = leftDouble * rightDouble;
                }
                break;
            case INTERMEDIATE_DIVIDE:
                if (rightDouble == 0.0) {
                    return COMPILER_INTERNAL_ERROR;
                }
                left->value.doubleValue = leftDouble / rightDouble;
                break;
            case INTERMEDIATE_INTEGER_DIVIDE:
                if (rightInt == 0) {
                    return COMPILER_INTERNAL_ERROR;
                }
                left->value.intValue = leftInt / rightInt;
                break;
            case INTERMEDIATE_COMPARE_LESS:
                left->kind = INTERMEDIATE_OPERAND_BOOLEAN;
                if (left->kind == INTERMEDIATE_OPERAND_INT) {
                    left->value.boolValue = leftInt < rightInt;
                } else if (left->kind == INTERMEDIATE_OPERAND_DOUBLE) {
                    left->value.boolValue = leftDouble < rightDouble;
                } else if (left->kind == INTERMEDIATE_OPERAND_STRING) {
                    left->value.boolValue = strcmp(left->text, right->text) < 0;
                } else {
                    left->value.boolValue = leftBool < rightBool;
                }
                break;
            case INTERMEDIATE_COMPARE_GREATER:
                left->kind = INTERMEDIATE_OPERAND_BOOLEAN;
                if (left->kind == INTERMEDIATE_OPERAND_INT) {
                    left->value.boolValue = leftInt > rightInt;
                } else if (left->kind == INTERMEDIATE_OPERAND_DOUBLE) {
                    left->value.boolValue = leftDouble > rightDouble;
                } else if (left->kind == INTERMEDIATE_OPERAND_STRING) {
                    left->value.boolValue = strcmp(left->text, right->text) > 0;
                } else {
                    left->value.boolValue = leftBool > rightBool;
                }
                break;
            case INTERMEDIATE_COMPARE_EQUAL:
                left->kind = INTERMEDIATE_OPERAND_BOOLEAN;
                if (left->kind == INTERMEDIATE_OPERAND_INT) {
                    left->value.boolValue = leftInt == rightInt;
                } else if (left->kind == INTERMEDIATE_OPERAND_DOUBLE) {
                    left->value.boolValue = leftDouble == rightDouble;
                } else if (left->kind == INTERMEDIATE_OPERAND_STRING) {
                    left->value.boolValue = strcmp(left->text, right->text) == 0;
                } else {
                    left->value.boolValue = leftBool == rightBool;
                }
                break;
            case INTERMEDIATE_AND:
                left->value.boolValue = leftBool && rightBool;
                break;
            case INTERMEDIATE_OR:
                left->value.boolValue = leftBool || rightBool;
                break;
            case INTERMEDIATE_CONCATENATE: {
                size_t len = strlen(left->text) + strlen(right->text) + 1;
                char *newStr = memoryAllocate(len * sizeof(char));
                if (newStr == NULL) {
                    return COMPILER_INTERNAL_ERROR;
                }
                strcpy(newStr, left->text);
                strcat(newStr, right->text);
                free(left->text);
                left->text = newStr;
                break;
            }
            case INTERMEDIATE_STRING_GET_CHARACTER: {
                if (rightInt < 0 || rightInt >= (int64_t)strlen(left->text)) {
                    return COMPILER_INTERNAL_ERROR;
                }
                char *newStr = memoryAllocate(2 * sizeof(char));
                if (newStr == NULL) {
                    return COMPILER_INTERNAL_ERROR;
                }
                newStr[0] = left->text[rightInt];
                newStr[1] = '\0';
                free(left->text);
                left->text = newStr;
                left->kind = INTERMEDIATE_OPERAND_STRING;
                break;
            }
            case INTERMEDIATE_STRING_CHARACTER_CODE: {
                if (rightInt < 0 || rightInt >= (int64_t)strlen(left->text)) {
                    return COMPILER_INTERNAL_ERROR;
                }
                left->value.intValue = (unsigned char)left->text[rightInt];
                left->kind = INTERMEDIATE_OPERAND_INT;
                free(left->text);
                left->text = NULL;
                break;
            }
            default:
                return COMPILER_INTERNAL_ERROR;
        }

        if (right->kind == INTERMEDIATE_OPERAND_STRING) {
            free(right->text);
        }

        right->text = NULL;
        right->kind = INTERMEDIATE_OPERAND_NONE;
        instruction->operandCount = 2;
    }

    else if ((flags & OPTIMIZER_TWO_OPERAND) == OPTIMIZER_TWO_OPERAND) {
        int64_t leftInt = left->value.intValue;
        double leftDouble = left->value.doubleValue;

        switch (originalOpcode) {
            case INTERMEDIATE_MOVE:
                break;
            case INTERMEDIATE_NOT:
                left->value.boolValue = !left->value.boolValue;
                break;
            case INTERMEDIATE_CONVERT_INT_TO_DOUBLE:
                left->value.doubleValue = (double)leftInt;
                left->kind = INTERMEDIATE_OPERAND_DOUBLE;
                break;
            case INTERMEDIATE_CONVERT_DOUBLE_TO_INT:
                left->value.intValue = (int64_t)leftDouble;
                left->kind = INTERMEDIATE_OPERAND_INT;
                break;
            case INTERMEDIATE_CONVERT_INT_TO_STRING: {
                char buffer[OPTIMIZER_MAX_INT];
                snprintf(buffer, sizeof(buffer), "%ld", leftInt);
                left->text = strdup(buffer);
                if (left->text == NULL) {
                    return COMPILER_INTERNAL_ERROR;
                }
                left->kind = INTERMEDIATE_OPERAND_STRING;
                break;
            }
            case INTERMEDIATE_CONVERT_DOUBLE_TO_STRING: {
                char buffer[OPTIMIZER_MAX_DOUBLE];
                snprintf(buffer, sizeof(buffer), "%g", leftDouble);
                left->text = strdup(buffer);
                if (left->text == NULL) {
                    return COMPILER_INTERNAL_ERROR;
                }
                left->kind = INTERMEDIATE_OPERAND_STRING;
                break;
            }
            case INTERMEDIATE_CONVERT_INT_TO_CHARACTER: {
                char *newStr = memoryAllocate(2 * sizeof(char));
                if (newStr == NULL) {
                    return COMPILER_INTERNAL_ERROR;
                }
                newStr[0] = (char)leftInt;
                newStr[1] = '\0';
                left->text = newStr;
                left->kind = INTERMEDIATE_OPERAND_STRING;
                break;
            }
            case INTERMEDIATE_STRING_LENGTH:
                if (left->text != NULL) {
                    left->value.intValue = strlen(left->text);
                } else {
                    left->value.intValue = 0;
                }
                left->kind = INTERMEDIATE_OPERAND_INT;
                free(left->text);
                left->text = NULL;
                break;
            default:
                return COMPILER_INTERNAL_ERROR;
        }
    }

    else {
        return COMPILER_INTERNAL_ERROR;
    }

    instruction->opcode = INTERMEDIATE_MOVE;

    return COMPILER_OK;
}

void optimizerNopInstruction(IntermediateInstruction *instruction)
{
    for (size_t i = 0; i < instruction->operandCount; ++i) {
        free(instruction->operands[i].text);
        instruction->operands[i].text = NULL;
        instruction->operands[i].kind = INTERMEDIATE_OPERAND_NONE;
    }

    instruction->operandCount = 0;
    instruction->opcode = INTERMEDIATE_NOP;
}

CompilerStatus optimizerForwardPass(BbtElem bbtTable[], size_t start, size_t end, IntermediateProgram *program, bool *changed)
{
    KnownConstantsTable *constants = knownConstantsTableCreate();
    if (constants == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    uint8_t flags = 0;
    KnownConstant *constant = NULL;
    size_t current = start;
    CompilerStatus status = COMPILER_OK;

    while(current <= end) {
        IntermediateInstruction *currentInstrPtr = &program->instructions[current];

        // constant propagation
        flags = optimizerGetOpcodeFlags((*currentInstrPtr).opcode);
        for (size_t i = 0; i < 3; ++i) {
            IntermediateOperand *curretnOperandPtr = &(*currentInstrPtr).operands[i];
            // is a read variable
            if (flags & 1 << i && (*curretnOperandPtr).kind == INTERMEDIATE_OPERAND_VARIABLE && flags & 1 << (i+3)) {
                constant = knownConstantsTableFind(constants, (*curretnOperandPtr).text);
                if (constant != NULL) {
                    (*curretnOperandPtr).kind = constant->kind; // the kind
                    (*curretnOperandPtr).value = constant->value; // int/double/bool
                    free((*curretnOperandPtr).text); // old text (name)
                    (*curretnOperandPtr).text = NULL;
                    if (constant->stringValue != NULL) {
                        (*curretnOperandPtr).text = strdup(constant->stringValue); // possible string value
                        if ((*curretnOperandPtr).text == NULL) {
                            knownConstantsTableDestroy(constants);
                            return COMPILER_INTERNAL_ERROR;
                        }
                    }
                    *changed = true;
                }
            }
        }

        // constant folding
        flags = optimizerGetOpcodeFlags((*currentInstrPtr).opcode);
        if (((flags & OPTIMIZER_THREE_OPERAND) == OPTIMIZER_THREE_OPERAND && optimizerIsOperandConst((*currentInstrPtr).operands[1]) && optimizerIsOperandConst((*currentInstrPtr).operands[2]))
        || ((flags & OPTIMIZER_TWO_OPERAND) == OPTIMIZER_TWO_OPERAND && optimizerIsOperandConst((*currentInstrPtr).operands[1]))) {
            if ((*currentInstrPtr).opcode != INTERMEDIATE_MOVE) {
                *changed = true;
            }
            status = optimizerConstantFold(currentInstrPtr, flags); // folding
            if (status != COMPILER_OK) {
                knownConstantsTableDestroy(constants);
                return status;
            }
        }

        // dead code removal
        flags = optimizerGetOpcodeFlags((*currentInstrPtr).opcode);
        if (((flags & OPTIMIZER_THREE_OPERAND) == OPTIMIZER_THREE_OPERAND || (flags & OPTIMIZER_TWO_OPERAND) == OPTIMIZER_TWO_OPERAND) && !bbtTable[current - start].live[0]) {
            optimizerNopInstruction(currentInstrPtr); // making instruction no-operation
            *changed = true;
        }

        // known constants update
        flags = optimizerGetOpcodeFlags((*currentInstrPtr).opcode);
        // add to constants
        if ((*currentInstrPtr).opcode == INTERMEDIATE_MOVE && optimizerIsOperandConst((*currentInstrPtr).operands[1])) {
            constant = memoryAllocate(sizeof(KnownConstant)); // creating constant
            if (constant == NULL) {
                knownConstantsTableDestroy(constants);
                return COMPILER_INTERNAL_ERROR;
            }
            constant->name = strdup((*currentInstrPtr).operands[0].text); // variable name
            if (constant->name == NULL) {
                free(constant);
                knownConstantsTableDestroy(constants);
                return COMPILER_INTERNAL_ERROR;
            }
            constant->kind = (*currentInstrPtr).operands[1].kind; // the kind
            constant->value = (*currentInstrPtr).operands[1].value; // int/double/bool value
            constant->stringValue = NULL;
            if ((*currentInstrPtr).operands[1].kind == INTERMEDIATE_OPERAND_STRING) {
                constant->stringValue = strdup((*currentInstrPtr).operands[1].text); // string vlaue
                if (constant->stringValue == NULL) {
                    free(constant->name);
                    free(constant);
                    knownConstantsTableDestroy(constants);
                    return COMPILER_INTERNAL_ERROR;
                }
            }

            status = knownConstantsTableInsert(constants, constant); // inserting to table
            if (status != COMPILER_OK) {
                free(constant->name);
                free(constant->stringValue);
                free(constant);
                knownConstantsTableDestroy(constants);
                return status;
            }
        }
        // invalidate constant
        else if ((flags & OPTIMIZER_THREE_OPERAND) == OPTIMIZER_THREE_OPERAND || (flags & OPTIMIZER_TWO_OPERAND) == OPTIMIZER_TWO_OPERAND || (flags & OPTIMIZER_ONE_WRITE) == OPTIMIZER_ONE_WRITE) {
            status = knownConstantsTableRemove(constants, (*currentInstrPtr).operands[0].text);
            if (status != COMPILER_OK) {
                knownConstantsTableDestroy(constants);
                return status;
            }
        }

        ++current;
    }


    knownConstantsTableDestroy(constants);

    return COMPILER_OK;
}
