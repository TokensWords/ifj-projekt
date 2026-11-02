/*
 * Project: IFJ26 compiler
 * Authors: Václav Krytinář xkrytiv00
 * AI-assisted interface only.
 * Prompt: Define the symbol-table boundary without tree algorithms.
 */

#ifndef OPTIMIZER_SYM_TABLE_H
#define OPTIMIZER_SYM_TABLE_H


#include "compilerTypes.h"

#include <stddef.h>
#include <stdbool.h>

typedef struct {
    const char *name;
    bool live;
    size_t nextUse;
} OptimizerSymbol;

typedef struct OptimizerSymTable OptimizerSymTable;

/**
 * @brief Creates a new symbol table.
 * @return A pointer to the newly created symbol table, or NULL if creation failed.
 */
OptimizerSymTable *optimizerSymTableCreate(void);

/**
 * @brief Destroys the symbol table and frees associated memory.
 * @param table The symbol table to destroy.
 */
void optimizerSymTableDestroy(OptimizerSymTable *table);

/**
 * @brief Inserts a new symbol into the symbol table.
 * @param table The symbol table to operate on.
 * @param symbol The symbol to insert.
 * @return A CompilerStatus indicating success or failure.
 */
CompilerStatus optimizerSymTableInsert(OptimizerSymTable *table, OptimizerSymbol *symbol);

/**
 * @brief Finds a symbol in the symbol table by name.
 * @param table The symbol table to search.
 * @param name The name of the symbol to find.
 * @return A pointer to the found symbol, or NULL if not found.
 */
OptimizerSymbol *optimizerSymTableFind(const OptimizerSymTable *table, const char *name);

#endif // OPTIMIZER_SYM_TABLE_H
