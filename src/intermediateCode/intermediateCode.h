/*
 * Project: IFJ26 compiler
 * Authors: Albert Hajek xhajeka00
 * AI-assisted scaffold.
 * Prompt: Define owned intermediate-code structures and generator boundaries.
 */

#ifndef INTERMEDIATE_CODE_H
#define INTERMEDIATE_CODE_H

#include "abstractSyntaxTree.h"
#include "compilerTypes.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    INTERMEDIATE_NOP,
    INTERMEDIATE_LABEL,
    INTERMEDIATE_MOVE,
    INTERMEDIATE_DEFINE_VARIABLE,
    INTERMEDIATE_CREATE_FRAME,
    INTERMEDIATE_PUSH_FRAME,
    INTERMEDIATE_POP_FRAME,
    INTERMEDIATE_CALL,
    INTERMEDIATE_RETURN,
    INTERMEDIATE_PUSH,
    INTERMEDIATE_POP,
    INTERMEDIATE_ADD,
    INTERMEDIATE_SUBTRACT,
    INTERMEDIATE_MULTIPLY,
    INTERMEDIATE_DIVIDE,
    INTERMEDIATE_INTEGER_DIVIDE,
    INTERMEDIATE_COMPARE_LESS,
    INTERMEDIATE_COMPARE_GREATER,
    INTERMEDIATE_COMPARE_EQUAL,
    INTERMEDIATE_AND,
    INTERMEDIATE_OR,
    INTERMEDIATE_NOT,
    INTERMEDIATE_CONVERT_INT_TO_DOUBLE,
    INTERMEDIATE_CONVERT_DOUBLE_TO_INT,
    INTERMEDIATE_CONVERT_INT_TO_STRING,
    INTERMEDIATE_CONVERT_DOUBLE_TO_STRING,
    INTERMEDIATE_CONVERT_INT_TO_CHARACTER,
    INTERMEDIATE_READ,
    INTERMEDIATE_WRITE,
    INTERMEDIATE_CONCATENATE,
    INTERMEDIATE_STRING_LENGTH,
    INTERMEDIATE_STRING_CHARACTER_CODE,
    INTERMEDIATE_STRING_GET_CHARACTER,
    INTERMEDIATE_JUMP,
    INTERMEDIATE_JUMP_IF_EQUAL,
    INTERMEDIATE_JUMP_IF_NOT_EQUAL,
    INTERMEDIATE_EXIT,
    INTERMEDIATE_ARRAY_CREATE,
    INTERMEDIATE_ARRAY_GET,
    INTERMEDIATE_ARRAY_SET,
    INTERMEDIATE_GET_TYPE
} IntermediateOpcode;

typedef enum {
    INTERMEDIATE_OPERAND_NONE,
    INTERMEDIATE_OPERAND_VARIABLE,
    INTERMEDIATE_OPERAND_LABEL,
    INTERMEDIATE_OPERAND_INT,
    INTERMEDIATE_OPERAND_DOUBLE,
    INTERMEDIATE_OPERAND_STRING,
    INTERMEDIATE_OPERAND_BOOLEAN,
    INTERMEDIATE_OPERAND_TYPE,
    INTERMEDIATE_OPERAND_UNIT
} IntermediateOperandKind;

typedef union {
    int64_t intValue;
    double doubleValue;
    bool boolValue;
    DataType dataType;
} IntermediateOperandValue;

typedef struct {
    IntermediateOperandKind kind;
    char *text;
    IntermediateOperandValue value;
} IntermediateOperand;

typedef struct {
    IntermediateOpcode opcode;
    IntermediateOperand operands[3];
    size_t operandCount;
    SourceLocation origin;
} IntermediateInstruction;

typedef struct {
    IntermediateInstruction *instructions;
    size_t count;
    size_t capacity;
} IntermediateProgram;

IntermediateProgram *intermediateProgramCreate(void);
void intermediateProgramDestroy(IntermediateProgram *program);
CompilerStatus intermediateProgramAppend(IntermediateProgram *program, const IntermediateInstruction *instruction);
CompilerStatus intermediateCodeGenerate(const SourceProgram *program, IntermediateProgram *outProgram);

#endif
