/*
 * Project: IFJ26 compiler
 * Authors: Václav Krytinář xkrytiv00
 * AI-assisted scaffold.
 * Prompt: Define the optional intermediate-code optimizer boundary.
 */

#ifndef OPTIMIZER_H
#define OPTIMIZER_H


#include <stddef.h>
#include <stdbool.h>

#include "compilerTypes.h"
#include "intermediateCode.h"

typedef struct BbtElem BbtElem;
typedef enum OpcodeFlags OpcodeFlags;
typedef enum OpcodeCategories OpcodeCategories;

/**
 * @brief Runs optimization on the intermediate program.
 * @param program The intermediate program to optimize.
 * @return A CompilerStatus indicating success or failure.
 */
CompilerStatus optimizerRun(IntermediateProgram *program);

/**
 * @brief Finds the next basic block in the intermediate program.
 * @param start Receives the first instruction index of the block.
 * @param end Receives the last instruction index of the block.
 * @param program The intermediate program to search.
 * @return true if another block was found, false otherwise.
 */
bool optimizerGetNextBlock(size_t *start, size_t *end, IntermediateProgram *program);

/**
 * @brief Optimizes a basic block in the intermediate program.
 * @param start The first instruction index of the block.
 * @param end The last instruction index of the block.
 * @param program The intermediate program containing the block.
 * @return A CompilerStatus indicating success or failure.
 */
CompilerStatus optimizerOptimizeBlock(size_t start, size_t end, IntermediateProgram *program);

/**
 * @brief Gets the evaluation and liveness flags for an opcode.
 * @param opcode The opcode to inspect.
 * @return A bitmask describing the opcode's operand behavior, or 0 if it is unclassified.
 */
uint8_t optimizerGetOpcodeFlags(IntermediateOpcode opcode);

/**
 * @brief Initializes a basic-block table entry with default values.
 * @param table The table to update.
 * @param index The entry index to initialize.
 * @param line The instruction index to store in the entry.
 */
void optimizerBbtTableSetDefault(BbtElem table[], size_t index, size_t line);

/**
 * @brief Runs the backward liveness analysis pass over a basic block.
 * @param bbtTable The table in which to store liveness and next-use information.
 * @param start The first instruction index of the block.
 * @param end The last instruction index of the block.
 * @param program The intermediate program containing the block.
 * @return A CompilerStatus indicating success or failure.
 */
CompilerStatus optimizerBackwardPass(BbtElem bbtTable[], size_t start, size_t end, IntermediateProgram *program);

/**
 * @brief Checks whether an operand contains a compile-time constant.
 * @param operand The operand to inspect.
 * @return true if the operand is constant, false otherwise.
 */
bool optimizerIsOperandConst(IntermediateOperand operand);

/**
 * @brief Replaces an instruction with its constant-folded equivalent.
 * @param instruction The instruction to fold.
 * @param flags The opcode's evaluation and liveness flags.
 * @return A CompilerStatus indicating success or failure.
 */
CompilerStatus optimizerConstantFold(IntermediateInstruction *instruction, uint8_t flags);

/**
 * @brief Replaces an instruction with a no-op and clears its operands.
 * @param instruction The instruction to replace.
 */
void optimizerNopInstruction(IntermediateInstruction *instruction);

/**
 * @brief Runs constant propagation, constant folding, and dead-code removal.
 * @param bbtTable The table containing liveness information for the block.
 * @param start The first instruction index of the block.
 * @param end The last instruction index of the block.
 * @param program The intermediate program containing the block.
 * @param changed Receives whether the pass modified the program.
 * @return A CompilerStatus indicating success or failure.
 */
CompilerStatus optimizerForwardPass(BbtElem bbtTable[], size_t start, size_t end, IntermediateProgram *program, bool *changed);

#endif // OPTIMIZER_H
