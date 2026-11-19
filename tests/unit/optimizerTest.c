/*
 * Project: IFJ26 compiler
 * Author: AI testing agent.
 * Prompt: Test optimizer results, observable instructions, block boundaries and
 * owned operands. Report all failures and release test data after each case.
 */

#include "optimizer.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const char *currentTest;
static size_t testCount;
static size_t failedTests;
static size_t failedChecks;

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL [%s] line %d: %s\n", currentTest, __LINE__, #condition); \
        ++failedChecks; \
    } \
} while (0)

static size_t beginTest(const char *name)
{
    currentTest = name;
    ++testCount;
    return failedChecks;
}

static void endTest(size_t failuresBefore)
{
    if (failedChecks != failuresBefore) {
        ++failedTests;
    }
}

/* Append copies text, so these operands borrow their string arguments. */
static IntermediateOperand namedOperand(IntermediateOperandKind kind, const char *text)
{
    return (IntermediateOperand){.kind = kind, .text = (char *)text};
}

static IntermediateOperand integerOperand(int64_t value)
{
    return (IntermediateOperand){.kind = INTERMEDIATE_OPERAND_INT, .value.intValue = value};
}

static IntermediateOperand doubleOperand(double value)
{
    return (IntermediateOperand){.kind = INTERMEDIATE_OPERAND_DOUBLE, .value.doubleValue = value};
}

static IntermediateOperand booleanOperand(bool value)
{
    return (IntermediateOperand){.kind = INTERMEDIATE_OPERAND_BOOLEAN, .value.boolValue = value};
}

static bool appendInstruction(IntermediateProgram *program, IntermediateOpcode opcode,
                              size_t count, IntermediateOperand a,
                              IntermediateOperand b, IntermediateOperand c)
{
    IntermediateInstruction instruction = {0};
    instruction.opcode = opcode;
    instruction.operandCount = count;
    instruction.origin = (SourceLocation){.line = 11, .column = 2};
    instruction.operands[0] = a;
    instruction.operands[1] = b;
    instruction.operands[2] = c;
    CompilerStatus status = intermediateProgramAppend(program, &instruction);
    CHECK(status == COMPILER_OK);
    return status == COMPILER_OK;
}

static IntermediateInstruction *findInstruction(IntermediateProgram *program,
                                               IntermediateOpcode opcode)
{
    for (size_t i = 0; i < program->count; ++i) {
        if (program->instructions[i].opcode == opcode) {
            return &program->instructions[i];
        }
    }
    return NULL;
}

static void checkOperand(const IntermediateOperand *actual, IntermediateOperand expected)
{
    CHECK(actual->kind == expected.kind);
    if (actual->kind != expected.kind) {
        return;
    }
    switch (expected.kind) {
        case INTERMEDIATE_OPERAND_INT:
            CHECK(actual->value.intValue == expected.value.intValue);
            break;
        case INTERMEDIATE_OPERAND_DOUBLE:
            CHECK(actual->value.doubleValue == expected.value.doubleValue);
            break;
        case INTERMEDIATE_OPERAND_BOOLEAN:
            CHECK(actual->value.boolValue == expected.value.boolValue);
            break;
        case INTERMEDIATE_OPERAND_STRING:
        case INTERMEDIATE_OPERAND_VARIABLE:
        case INTERMEDIATE_OPERAND_LABEL:
            CHECK(actual->text != NULL && strcmp(actual->text, expected.text) == 0);
            break;
        default:
            CHECK(false);
            break;
    }
}

static void testFold(const char *name, IntermediateOpcode opcode,
                     IntermediateOperand left, IntermediateOperand right,
                     IntermediateOperand expected, bool binary)
{
    size_t failuresBefore = beginTest(name);
    IntermediateProgram *program = intermediateProgramCreate();
    CHECK(program != NULL);
    if (program != NULL) {
        IntermediateOperand destination = namedOperand(INTERMEDIATE_OPERAND_VARIABLE, "$temporary_result");
        if (appendInstruction(program, opcode, binary ? 3 : 2, destination, left, right)) {
            IntermediateInstruction *instruction = &program->instructions[0];
            CompilerStatus status = optimizerConstantFold(instruction, optimizerGetOpcodeFlags(opcode));
            CHECK(status == COMPILER_OK);
            if (status == COMPILER_OK) {
                CHECK(instruction->opcode == INTERMEDIATE_MOVE);
                CHECK(instruction->operandCount == 2);
                checkOperand(&instruction->operands[0], destination);
                checkOperand(&instruction->operands[1], expected);
                if (binary) {
                    CHECK(instruction->operands[2].kind == INTERMEDIATE_OPERAND_NONE);
                    CHECK(instruction->operands[2].text == NULL);
                }
                CHECK(instruction->origin.line == 11 && instruction->origin.column == 2);
            }
        }
        intermediateProgramDestroy(program);
    }
    endTest(failuresBefore);
}

static void testApi(void)
{
    size_t failuresBefore = beginTest("null arguments and empty program");
    CHECK(optimizerRun(NULL) == COMPILER_INTERNAL_ERROR);
    CHECK(optimizerConstantFold(NULL, 0) == COMPILER_INTERNAL_ERROR);
    IntermediateProgram *program = intermediateProgramCreate();
    CHECK(program != NULL);
    if (program != NULL) {
        size_t start = 0;
        size_t end = SIZE_MAX;
        CHECK(!optimizerGetNextBlock(NULL, &end, program));
        CHECK(!optimizerGetNextBlock(&start, NULL, program));
        CHECK(!optimizerGetNextBlock(&start, &end, NULL));
        CHECK(optimizerRun(program) == COMPILER_OK);
        intermediateProgramDestroy(program);
    }
    endTest(failuresBefore);
}

static void testLabels(bool trailing)
{
    size_t failuresBefore = beginTest(trailing ? "consecutive and trailing labels" : "single label");
    IntermediateProgram *program = intermediateProgramCreate();
    CHECK(program != NULL);
    if (program != NULL) {
        IntermediateOperand none = {0};
        bool built = appendInstruction(program, INTERMEDIATE_LABEL, 1,
            namedOperand(INTERMEDIATE_OPERAND_LABEL, "entry"), none, none);
        if (built && trailing) {
            built = appendInstruction(program, INTERMEDIATE_LABEL, 1,
                namedOperand(INTERMEDIATE_OPERAND_LABEL, "next"), none, none) &&
                appendInstruction(program, INTERMEDIATE_RETURN, 0, none, none, none) &&
                appendInstruction(program, INTERMEDIATE_LABEL, 1,
                    namedOperand(INTERMEDIATE_OPERAND_LABEL, "end"), none, none);
        }
        if (built) {
            CHECK(optimizerRun(program) == COMPILER_OK);
            size_t labels = 0;
            for (size_t i = 0; i < program->count; ++i) {
                if (program->instructions[i].opcode == INTERMEDIATE_LABEL) {
                    CHECK(program->instructions[i].origin.line == 11);
                    CHECK(program->instructions[i].origin.column == 2);
                    ++labels;
                }
            }
            CHECK(labels == (trailing ? 3u : 1u));
        }
        intermediateProgramDestroy(program);
    }
    endTest(failuresBefore);
}

typedef enum {
    PROGRAM_READ_ONLY,
    PROGRAM_READ_UNKNOWN,
    PROGRAM_PROPAGATE,
    PROGRAM_OVERWRITE,
    PROGRAM_READ_INVALIDATES,
    PROGRAM_ZERO_DIVISOR
} ProgramCase;

static void testProgram(const char *name, ProgramCase scenario)
{
    size_t failuresBefore = beginTest(name);
    IntermediateProgram *program = intermediateProgramCreate();
    CHECK(program != NULL);
    if (program == NULL) {
        endTest(failuresBefore);
        return;
    }

    IntermediateOperand none = {0};
    IntermediateOperand variable = namedOperand(INTERMEDIATE_OPERAND_VARIABLE, "$variable_value");
    IntermediateOperand type = {.kind = INTERMEDIATE_OPERAND_TYPE, .value.dataType = DATA_TYPE_INT};
    bool built = true;
    if (scenario != PROGRAM_READ_ONLY) {
        built = appendInstruction(program, INTERMEDIATE_DEFINE_VARIABLE, 1, variable, none, none);
    }
    if (built && (scenario == PROGRAM_OVERWRITE || scenario == PROGRAM_READ_INVALIDATES)) {
        built = appendInstruction(program, INTERMEDIATE_MOVE, 2, variable, integerOperand(1), none);
    }
    if (built && scenario == PROGRAM_OVERWRITE) {
        built = appendInstruction(program, INTERMEDIATE_MOVE, 2, variable, integerOperand(2), none);
    }
    if (built && (scenario == PROGRAM_READ_UNKNOWN || scenario == PROGRAM_READ_INVALIDATES)) {
        built = appendInstruction(program, INTERMEDIATE_READ, 2, variable, type, none);
    }
    if (built && scenario == PROGRAM_PROPAGATE) {
        built = appendInstruction(program, INTERMEDIATE_ADD, 3, variable, integerOperand(7), integerOperand(5));
    }
    if (built && scenario == PROGRAM_ZERO_DIVISOR) {
        built = appendInstruction(program, INTERMEDIATE_INTEGER_DIVIDE, 3, variable, integerOperand(1), integerOperand(0));
    }
    if (built) {
        built = appendInstruction(program, INTERMEDIATE_WRITE, 1,
            scenario == PROGRAM_READ_ONLY ? integerOperand(26) : variable, none, none);
    }
    if (built) {
        built = appendInstruction(program, INTERMEDIATE_EXIT, 1, integerOperand(0), none, none);
    }
    if (built) {
        CompilerStatus status = optimizerRun(program);
        CHECK(status == COMPILER_OK);
        if (status == COMPILER_OK) {
            IntermediateInstruction *write = findInstruction(program, INTERMEDIATE_WRITE);
            IntermediateInstruction *exit = findInstruction(program, INTERMEDIATE_EXIT);
            CHECK(write != NULL && exit != NULL);
            if (write != NULL) {
                IntermediateOperand expected = scenario == PROGRAM_READ_ONLY ? integerOperand(26)
                    : scenario == PROGRAM_PROPAGATE ? integerOperand(12)
                    : scenario == PROGRAM_OVERWRITE ? integerOperand(2) : variable;
                checkOperand(&write->operands[0], expected);
            }
            if (exit != NULL) {
                checkOperand(&exit->operands[0], integerOperand(0));
            }
            if (scenario == PROGRAM_READ_UNKNOWN || scenario == PROGRAM_READ_INVALIDATES) {
                CHECK(findInstruction(program, INTERMEDIATE_READ) != NULL);
            }
            if (scenario == PROGRAM_ZERO_DIVISOR) {
                CHECK(findInstruction(program, INTERMEDIATE_INTEGER_DIVIDE) != NULL);
            }
            CHECK(optimizerRun(program) == COMPILER_OK);
        }
    }
    intermediateProgramDestroy(program);
    endTest(failuresBefore);
}

static void testNop(void)
{
    size_t failuresBefore = beginTest("NOP releases owned operands");
    IntermediateProgram *program = intermediateProgramCreate();
    CHECK(program != NULL);
    if (program != NULL) {
        IntermediateOperand none = {0};
        if (appendInstruction(program, INTERMEDIATE_MOVE, 2,
                namedOperand(INTERMEDIATE_OPERAND_VARIABLE, "$temporary_dead"),
                namedOperand(INTERMEDIATE_OPERAND_STRING, "owned string"), none)) {
            optimizerNopInstruction(&program->instructions[0]);
            CHECK(program->instructions[0].opcode == INTERMEDIATE_NOP);
            CHECK(program->instructions[0].operandCount == 0);
            for (size_t i = 0; i < 3; ++i) {
                CHECK(program->instructions[0].operands[i].text == NULL);
                CHECK(program->instructions[0].operands[i].kind == INTERMEDIATE_OPERAND_NONE);
            }
        }
        intermediateProgramDestroy(program);
    }
    endTest(failuresBefore);
}

int main(void)
{
    IntermediateOperand none = {0};
    testApi();
    testLabels(false);
    testLabels(true);
    testProgram("WRITE and EXIT with literal operands", PROGRAM_READ_ONLY);
    testProgram("READ without a previously known constant", PROGRAM_READ_UNKNOWN);
    testProgram("constant folding, propagation and repeated optimization", PROGRAM_PROPAGATE);
    testProgram("constant reassignment", PROGRAM_OVERWRITE);
    testProgram("READ invalidates an earlier constant", PROGRAM_READ_INVALIDATES);
    testProgram("zero divisor remains a runtime operation", PROGRAM_ZERO_DIVISOR);
    testNop();

    testFold("integer addition", INTERMEDIATE_ADD, integerOperand(7), integerOperand(5), integerOperand(12), true);
    testFold("integer subtraction", INTERMEDIATE_SUBTRACT, integerOperand(7), integerOperand(9), integerOperand(-2), true);
    testFold("integer multiplication", INTERMEDIATE_MULTIPLY, integerOperand(-3), integerOperand(7), integerOperand(-21), true);
    testFold("integer division toward zero", INTERMEDIATE_INTEGER_DIVIDE, integerOperand(-7), integerOperand(2), integerOperand(-3), true);
    testFold("double addition", INTERMEDIATE_ADD, doubleOperand(1.5), doubleOperand(2.0), doubleOperand(3.5), true);
    testFold("double division", INTERMEDIATE_DIVIDE, doubleOperand(7.0), doubleOperand(2.0), doubleOperand(3.5), true);
    testFold("integer less-than uses full value", INTERMEDIATE_COMPARE_LESS, integerOperand(256), integerOperand(512), booleanOperand(true), true);
    testFold("integer greater-than uses full value", INTERMEDIATE_COMPARE_GREATER, integerOperand(512), integerOperand(256), booleanOperand(true), true);
    testFold("integer equality uses full value", INTERMEDIATE_COMPARE_EQUAL, integerOperand(256), integerOperand(512), booleanOperand(false), true);
    testFold("double comparison", INTERMEDIATE_COMPARE_LESS, doubleOperand(1.5), doubleOperand(2.5), booleanOperand(true), true);
    testFold("double equality", INTERMEDIATE_COMPARE_EQUAL, doubleOperand(1.5), doubleOperand(2.5), booleanOperand(false), true);
    testFold("string comparison", INTERMEDIATE_COMPARE_LESS,
        namedOperand(INTERMEDIATE_OPERAND_STRING, "alpha"), namedOperand(INTERMEDIATE_OPERAND_STRING, "beta"), booleanOperand(true), true);
    testFold("string equality", INTERMEDIATE_COMPARE_EQUAL,
        namedOperand(INTERMEDIATE_OPERAND_STRING, "same"), namedOperand(INTERMEDIATE_OPERAND_STRING, "other"), booleanOperand(false), true);
    testFold("boolean AND", INTERMEDIATE_AND, booleanOperand(true), booleanOperand(false), booleanOperand(false), true);
    testFold("boolean OR", INTERMEDIATE_OR, booleanOperand(false), booleanOperand(true), booleanOperand(true), true);
    testFold("boolean NOT", INTERMEDIATE_NOT, booleanOperand(true), none, booleanOperand(false), false);
    testFold("Int to Double", INTERMEDIATE_CONVERT_INT_TO_DOUBLE, integerOperand(12), none, doubleOperand(12.0), false);
    testFold("Double to Int", INTERMEDIATE_CONVERT_DOUBLE_TO_INT, doubleOperand(-3.75), none, integerOperand(-3), false);
    testFold("64-bit Int to String", INTERMEDIATE_CONVERT_INT_TO_STRING, integerOperand(INT64_C(4294967297)), none,
        namedOperand(INTERMEDIATE_OPERAND_STRING, "4294967297"), false);
    testFold("Double to String has two decimal places", INTERMEDIATE_CONVERT_DOUBLE_TO_STRING, doubleOperand(3.5), none,
        namedOperand(INTERMEDIATE_OPERAND_STRING, "3.50"), false);
    testFold("Int to character", INTERMEDIATE_CONVERT_INT_TO_CHARACTER, integerOperand(65), none,
        namedOperand(INTERMEDIATE_OPERAND_STRING, "A"), false);
    testFold("string concatenation", INTERMEDIATE_CONCATENATE,
        namedOperand(INTERMEDIATE_OPERAND_STRING, "hello"), namedOperand(INTERMEDIATE_OPERAND_STRING, " world"),
        namedOperand(INTERMEDIATE_OPERAND_STRING, "hello world"), true);
    testFold("string length", INTERMEDIATE_STRING_LENGTH, namedOperand(INTERMEDIATE_OPERAND_STRING, "abc"), none, integerOperand(3), false);
    testFold("string character", INTERMEDIATE_STRING_GET_CHARACTER,
        namedOperand(INTERMEDIATE_OPERAND_STRING, "abc"), integerOperand(1), namedOperand(INTERMEDIATE_OPERAND_STRING, "b"), true);
    testFold("string character code", INTERMEDIATE_STRING_CHARACTER_CODE,
        namedOperand(INTERMEDIATE_OPERAND_STRING, "abc"), integerOperand(1), integerOperand(98), true);

    printf("optimizerTest: %zu tests, %zu passed, %zu failed (%zu failed checks)\n",
           testCount, testCount - failedTests, failedTests, failedChecks);
    return failedTests == 0 ? 0 : 1;
}

#undef CHECK
