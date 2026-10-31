/*
 * Project: IFJ26 compiler
 * Authors: Václav Krytinář xkrytiv00
 * AI-assisted interface only.
 * Prompt: Define the known-constants table boundary without tree algorithms.
 */

#ifndef KNOWN_CONSTANTS_TABLE_H
#define KNOWN_CONSTANTS_TABLE_H

#include "compilerTypes.h"
#include "intermediateCode.h"

#include <stddef.h>
#include <stdbool.h>

typedef struct {
    const char *name;
    IntermediateOperandValue value;
    IntermediateOperandKind kind;
    size_t originIndex;
} KnownConstant;

typedef struct KnownConstantsTable KnownConstantsTable;

/**
 * @brief Creates a new known-constants table.
 * @return A pointer to the newly created table, or NULL if creation failed.
 */
KnownConstantsTable *knownConstantsTableCreate(void);

/**
 * @brief Destroys the known-constants table and frees associated memory.
 * @param table The table to destroy.
 */
void knownConstantsTableDestroy(KnownConstantsTable *table);

/**
 * @brief Inserts a known constant into the table.
 * @param table The table to operate on.
 * @param constant The known constant to insert.
 * @return A CompilerStatus indicating success or failure.
 */
CompilerStatus knownConstantsTableInsert(KnownConstantsTable *table, KnownConstant *constant);

/**
 * @brief Finds a known constant in the table by name.
 * @param table The table to search.
 * @param name The name of the constant to find.
 * @return A pointer to the found constant, or NULL if not found.
 */
const KnownConstant *knownConstantsTableFind(const KnownConstantsTable *table, const char *name);

#endif // KNOWN_CONSTANTS_TABLE_H
