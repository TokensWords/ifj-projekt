/*
 * Project: IFJ26 compiler
 * Authors: xhajeka00
 * AI-assisted scaffold.
 * Prompt: Create an IFJcode26 code-generation integration stub.
 */

#include "generator.h"
#include <inttypes.h>

static CompilerStatus writeEscapedString(const char *text, FILE *output) {
    if (text == NULL || output == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    size_t counter = 0;

    while (text[counter] != '\0') {
        unsigned char current = text[counter];

        if (current < 33 || current == '#' || current == '\\') {
            if (fprintf(output, "\\%03d", current) < 0) {
                return COMPILER_INTERNAL_ERROR;
            }
        } else {
            if (fprintf(output, "%c", current) < 0) {
                return COMPILER_INTERNAL_ERROR;
            }
        }

        counter++;
    }

    return COMPILER_OK;
}

static CompilerStatus writeOperand(const IntermediateOperand *operand, FILE *output) {
    if (operand == NULL || output == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    switch (operand->kind) {
    case INTERMEDIATE_OPERAND_VARIABLE: {
        if (operand->text == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        if (fprintf(output, "LF@%s", operand->text) < 0) {
            return COMPILER_INTERNAL_ERROR;
        }

        return COMPILER_OK;
    }
    case INTERMEDIATE_OPERAND_LABEL: {
        if (operand->text == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        if (fprintf(output, "%s", operand->text) < 0) {
            return COMPILER_INTERNAL_ERROR;
        }

        return COMPILER_OK;
    }
    case INTERMEDIATE_OPERAND_INT: {
        if (fprintf(output, "int@%" PRId64, operand->value.intValue) < 0) {
            return COMPILER_INTERNAL_ERROR;
        }

        return COMPILER_OK;
    }
    case INTERMEDIATE_OPERAND_DOUBLE: {
        if (fprintf(output, "float@%a", operand->value.doubleValue) < 0) {
            return COMPILER_INTERNAL_ERROR;
        }

        return COMPILER_OK;
    }
    case INTERMEDIATE_OPERAND_STRING: {
        if (operand->text == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        if (fprintf(output, "string@") < 0) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeEscapedString(operand->text, output);
    }
    case INTERMEDIATE_OPERAND_BOOLEAN: {
        if (fprintf(output, "bool@%s", operand->value.boolValue ? "true" : "false") < 0) {
            return COMPILER_INTERNAL_ERROR;
        }

        return COMPILER_OK;
    }
    case INTERMEDIATE_OPERAND_TYPE: {
        const char *typeName = NULL;
        switch (operand->value.dataType) {
        case DATA_TYPE_INT: {
            typeName = "int";
            break;
        }
        case DATA_TYPE_DOUBLE: {
            typeName = "float";
            break;
        }
        case DATA_TYPE_STRING: {
            typeName = "string";
            break;
        }
        case DATA_TYPE_BOOLEAN: {
            typeName = "bool";
            break;
        }
        default:
            return COMPILER_INTERNAL_ERROR;
        }

        if (fprintf(output, "%s", typeName) < 0) {
            return COMPILER_INTERNAL_ERROR;
        }

        return COMPILER_OK;
    }
    case INTERMEDIATE_OPERAND_UNIT: {
        if (fprintf(output, "nil@nil") < 0) {
            return COMPILER_INTERNAL_ERROR;
        }

        return COMPILER_OK;
    }
    case INTERMEDIATE_OPERAND_NONE:
    default:
        return COMPILER_INTERNAL_ERROR;
    }
}

static CompilerStatus writeInstructionLine(const char *instructionName, const IntermediateInstruction *instruction, FILE *output) {
    if (instructionName == NULL || instruction == NULL || output == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    if (instruction->operandCount > 3) {
        return COMPILER_INTERNAL_ERROR;
    }

    if (fprintf(output, "%s", instructionName) < 0) {
        return COMPILER_INTERNAL_ERROR;
    }

    size_t counter = 0;

    while (counter < instruction->operandCount) {
        if (fprintf(output, " ") < 0) {
            return COMPILER_INTERNAL_ERROR;
        }

        CompilerStatus status = writeOperand(&instruction->operands[counter], output);
        if (status != COMPILER_OK) {
            return status;
        }

        counter++;
    }

    if (fprintf(output, "\n") < 0) {
        return COMPILER_INTERNAL_ERROR;
    }

    return COMPILER_OK;
}

static bool isValueOperand(IntermediateOperandKind kind) {
    switch (kind) {
    case INTERMEDIATE_OPERAND_VARIABLE:
    case INTERMEDIATE_OPERAND_INT:
    case INTERMEDIATE_OPERAND_DOUBLE:
    case INTERMEDIATE_OPERAND_STRING:
    case INTERMEDIATE_OPERAND_BOOLEAN:
    case INTERMEDIATE_OPERAND_UNIT:
        return true;
    default:
        return false;
    }
}

static CompilerStatus writeInstruction(const IntermediateInstruction *instruction, FILE *output) {
    if (instruction == NULL || output == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    switch (instruction->opcode) {
    case INTERMEDIATE_CREATE_FRAME: {
        if (instruction->operandCount != 0) {
            return COMPILER_INTERNAL_ERROR;
        }

        if (fprintf(output, "CREATEFRAME\n") < 0) {
            return COMPILER_INTERNAL_ERROR;
        }

        return COMPILER_OK;
    }
    case INTERMEDIATE_PUSH_FRAME: {
        if (instruction->operandCount != 0) {
            return COMPILER_INTERNAL_ERROR;
        }

        if (fprintf(output, "PUSHFRAME\n") < 0) {
            return COMPILER_INTERNAL_ERROR;
        }

        return COMPILER_OK;
    }
    case INTERMEDIATE_POP_FRAME: {
        if (instruction->operandCount != 0) {
            return COMPILER_INTERNAL_ERROR;
        }

        if (fprintf(output, "POPFRAME\n") < 0) {
            return COMPILER_INTERNAL_ERROR;
        }

        return COMPILER_OK;
    }
    case INTERMEDIATE_RETURN: {
        if (instruction->operandCount != 0) {
            return COMPILER_INTERNAL_ERROR;
        }

        if (fprintf(output, "RETURN\n") < 0) {
            return COMPILER_INTERNAL_ERROR;
        }

        return COMPILER_OK;
    }
    case INTERMEDIATE_NOP: {
        if (instruction->operandCount != 0) {
            return COMPILER_INTERNAL_ERROR;
        }

        return COMPILER_OK;
    }
    case INTERMEDIATE_LABEL: {
        if (instruction->operandCount != 1 || instruction->operands[0].kind != INTERMEDIATE_OPERAND_LABEL) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeInstructionLine("LABEL", instruction, output);
    }
    case INTERMEDIATE_CALL: {
        if (instruction->operandCount != 1 || instruction->operands[0].kind != INTERMEDIATE_OPERAND_LABEL) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeInstructionLine("CALL", instruction, output);
    }
    case INTERMEDIATE_JUMP: {
        if (instruction->operandCount != 1 || instruction->operands[0].kind != INTERMEDIATE_OPERAND_LABEL) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeInstructionLine("JUMP", instruction, output);
    }
    case INTERMEDIATE_DEFINE_VARIABLE: {
        if (instruction->operandCount != 1 || instruction->operands[0].kind != INTERMEDIATE_OPERAND_VARIABLE) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeInstructionLine("DEFVAR", instruction, output);
    }
    case INTERMEDIATE_POP: {
        if (instruction->operandCount != 1 || instruction->operands[0].kind != INTERMEDIATE_OPERAND_VARIABLE) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeInstructionLine("POPS", instruction, output);
    }
    case INTERMEDIATE_PUSH: {
        if (instruction->operandCount != 1 || !isValueOperand(instruction->operands[0].kind)) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeInstructionLine("PUSHS", instruction, output);
    }
    case INTERMEDIATE_WRITE: {
        if (instruction->operandCount != 1 || !isValueOperand(instruction->operands[0].kind)) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeInstructionLine("WRITE", instruction, output);
    }
    case INTERMEDIATE_EXIT: {
        if (instruction->operandCount != 1 || (instruction->operands[0].kind != INTERMEDIATE_OPERAND_VARIABLE && instruction->operands[0].kind != INTERMEDIATE_OPERAND_INT)) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeInstructionLine("EXIT", instruction, output);
    }
    case INTERMEDIATE_MOVE: {
        if (instruction->operandCount != 2 || instruction->operands[0].kind != INTERMEDIATE_OPERAND_VARIABLE || !isValueOperand(instruction->operands[1].kind)) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeInstructionLine("MOVE", instruction, output);
    }
    case INTERMEDIATE_NOT: {
        if (instruction->operandCount != 2 || instruction->operands[0].kind != INTERMEDIATE_OPERAND_VARIABLE || !isValueOperand(instruction->operands[1].kind)) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeInstructionLine("NOT", instruction, output);
    }
    case INTERMEDIATE_CONVERT_INT_TO_DOUBLE: {
        if (instruction->operandCount != 2 || instruction->operands[0].kind != INTERMEDIATE_OPERAND_VARIABLE || !isValueOperand(instruction->operands[1].kind)) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeInstructionLine("INT2FLOAT", instruction, output);
    }
    case INTERMEDIATE_CONVERT_DOUBLE_TO_INT: {
        if (instruction->operandCount != 2 || instruction->operands[0].kind != INTERMEDIATE_OPERAND_VARIABLE || !isValueOperand(instruction->operands[1].kind)) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeInstructionLine("FLOAT2INT", instruction, output);
    }
    case INTERMEDIATE_CONVERT_INT_TO_STRING: {
        if (instruction->operandCount != 2 || instruction->operands[0].kind != INTERMEDIATE_OPERAND_VARIABLE || !isValueOperand(instruction->operands[1].kind)) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeInstructionLine("INT2STR", instruction, output);
    }
    case INTERMEDIATE_CONVERT_DOUBLE_TO_STRING: {
        if (instruction->operandCount != 2 || instruction->operands[0].kind != INTERMEDIATE_OPERAND_VARIABLE || !isValueOperand(instruction->operands[1].kind)) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeInstructionLine("FLOAT2STR", instruction, output);
    }
    case INTERMEDIATE_CONVERT_INT_TO_CHARACTER: {
        if (instruction->operandCount != 2 || instruction->operands[0].kind != INTERMEDIATE_OPERAND_VARIABLE || !isValueOperand(instruction->operands[1].kind)) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeInstructionLine("INT2CHAR", instruction, output);
    }
    case INTERMEDIATE_STRING_LENGTH: {
        if (instruction->operandCount != 2 || instruction->operands[0].kind != INTERMEDIATE_OPERAND_VARIABLE || !isValueOperand(instruction->operands[1].kind)) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeInstructionLine("STRLEN", instruction, output);
    }
    case INTERMEDIATE_READ: {
        if (instruction->operandCount != 2 || instruction->operands[0].kind != INTERMEDIATE_OPERAND_VARIABLE || instruction->operands[1].kind != INTERMEDIATE_OPERAND_TYPE) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeInstructionLine("READ", instruction, output);
    }
    case INTERMEDIATE_ADD: {
        if (instruction->operandCount != 3 || instruction->operands[0].kind != INTERMEDIATE_OPERAND_VARIABLE || !isValueOperand(instruction->operands[1].kind) || !isValueOperand(instruction->operands[2].kind)) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeInstructionLine("ADD", instruction, output);
    }
    case INTERMEDIATE_SUBTRACT: {
        if (instruction->operandCount != 3 || instruction->operands[0].kind != INTERMEDIATE_OPERAND_VARIABLE || !isValueOperand(instruction->operands[1].kind) || !isValueOperand(instruction->operands[2].kind)) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeInstructionLine("SUB", instruction, output);
    }
    case INTERMEDIATE_MULTIPLY: {
        if (instruction->operandCount != 3 || instruction->operands[0].kind != INTERMEDIATE_OPERAND_VARIABLE || !isValueOperand(instruction->operands[1].kind) || !isValueOperand(instruction->operands[2].kind)) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeInstructionLine("MUL", instruction, output);
    }
    case INTERMEDIATE_DIVIDE: {
        if (instruction->operandCount != 3 || instruction->operands[0].kind != INTERMEDIATE_OPERAND_VARIABLE || !isValueOperand(instruction->operands[1].kind) || !isValueOperand(instruction->operands[2].kind)) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeInstructionLine("DIV", instruction, output);
    }
    case INTERMEDIATE_INTEGER_DIVIDE: {
        if (instruction->operandCount != 3 || instruction->operands[0].kind != INTERMEDIATE_OPERAND_VARIABLE || !isValueOperand(instruction->operands[1].kind) || !isValueOperand(instruction->operands[2].kind)) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeInstructionLine("IDIV", instruction, output);
    }
    case INTERMEDIATE_COMPARE_LESS: {
        if (instruction->operandCount != 3 || instruction->operands[0].kind != INTERMEDIATE_OPERAND_VARIABLE || !isValueOperand(instruction->operands[1].kind) || !isValueOperand(instruction->operands[2].kind)) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeInstructionLine("LT", instruction, output);
    }
    case INTERMEDIATE_COMPARE_GREATER: {
        if (instruction->operandCount != 3 || instruction->operands[0].kind != INTERMEDIATE_OPERAND_VARIABLE || !isValueOperand(instruction->operands[1].kind) || !isValueOperand(instruction->operands[2].kind)) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeInstructionLine("GT", instruction, output);
    }
    case INTERMEDIATE_COMPARE_EQUAL: {
        if (instruction->operandCount != 3 || instruction->operands[0].kind != INTERMEDIATE_OPERAND_VARIABLE || !isValueOperand(instruction->operands[1].kind) || !isValueOperand(instruction->operands[2].kind)) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeInstructionLine("EQ", instruction, output);
    }
    case INTERMEDIATE_AND: {
        if (instruction->operandCount != 3 || instruction->operands[0].kind != INTERMEDIATE_OPERAND_VARIABLE || !isValueOperand(instruction->operands[1].kind) || !isValueOperand(instruction->operands[2].kind)) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeInstructionLine("AND", instruction, output);
    }
    case INTERMEDIATE_OR: {
        if (instruction->operandCount != 3 || instruction->operands[0].kind != INTERMEDIATE_OPERAND_VARIABLE || !isValueOperand(instruction->operands[1].kind) || !isValueOperand(instruction->operands[2].kind)) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeInstructionLine("OR", instruction, output);
    }
    case INTERMEDIATE_CONCATENATE: {
        if (instruction->operandCount != 3 || instruction->operands[0].kind != INTERMEDIATE_OPERAND_VARIABLE || !isValueOperand(instruction->operands[1].kind) || !isValueOperand(instruction->operands[2].kind)) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeInstructionLine("CONCAT", instruction, output);
    }
    case INTERMEDIATE_STRING_CHARACTER_CODE: {
        if (instruction->operandCount != 3 || instruction->operands[0].kind != INTERMEDIATE_OPERAND_VARIABLE || !isValueOperand(instruction->operands[1].kind) || !isValueOperand(instruction->operands[2].kind)) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeInstructionLine("STRI2INT", instruction, output);
    }
    case INTERMEDIATE_STRING_GET_CHARACTER: {
        if (instruction->operandCount != 3 || instruction->operands[0].kind != INTERMEDIATE_OPERAND_VARIABLE || !isValueOperand(instruction->operands[1].kind) || !isValueOperand(instruction->operands[2].kind)) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeInstructionLine("GETCHAR", instruction, output);
    }
    case INTERMEDIATE_JUMP_IF_EQUAL: {
        if (instruction->operandCount != 3 || instruction->operands[0].kind != INTERMEDIATE_OPERAND_LABEL || !isValueOperand(instruction->operands[1].kind) || !isValueOperand(instruction->operands[2].kind)) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeInstructionLine("JUMPIFEQ", instruction, output);
    }
    case INTERMEDIATE_JUMP_IF_NOT_EQUAL: {
        if (instruction->operandCount != 3 || instruction->operands[0].kind != INTERMEDIATE_OPERAND_LABEL || !isValueOperand(instruction->operands[1].kind) || !isValueOperand(instruction->operands[2].kind)) {
            return COMPILER_INTERNAL_ERROR;
        }

        return writeInstructionLine("JUMPIFNEQ", instruction, output);
    }
    default:
        return COMPILER_INTERNAL_ERROR;
    }
}

CompilerStatus generatorWrite(const IntermediateProgram *program, FILE *output) {
    if (program == NULL || output == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    if (program->count > 0) {
        if (program->instructions == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }
    }

    if (fprintf(output, ".IFJcode26\n") < 0) {
        return COMPILER_INTERNAL_ERROR;
    }

    size_t counter = 0;

    while (counter < program->count) {
        CompilerStatus status = writeInstruction(&program->instructions[counter], output);
        if (status != COMPILER_OK) {
            return status;
        }

        counter++;
    }

    if (fflush(output) == EOF) {
        return COMPILER_INTERNAL_ERROR;    
    }

    return COMPILER_OK;
}
