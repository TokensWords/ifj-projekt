/**
 * @file symtable.h
 * @brief Public interface and data types for scoped symbol tables.
 *
 * @details Provides symbol and function-signature records together with
 * operations for creating a table, managing nested scopes, and looking up
 * symbols. The table implementation and its backing tree remain opaque.
 * @author Václav Krytinář xkrytiv00
 */

#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "compilerTypes.h"
#include "typeDescriptor.h"

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    SYMBOL_VARIABLE,
    SYMBOL_FUNCTION,
    SYMBOL_TYPE
} SymbolKind;

typedef struct {
    TypeDescriptor **parameterTypes;
    char **parameterNames;
    size_t parameterCount;
    TypeDescriptor *returnType;
} FunctionSignature;

typedef struct {
    const char *name;
    SymbolKind kind;
    TypeDescriptor *dataType;
    DataType variableCurrentType;
    bool mutable;
    bool defined;
    FunctionSignature function;
    size_t variableId;
} Symbol;

typedef struct SymbolTable SymbolTable;

/**
 * @brief Creates a new symbol table.
 * @return A pointer to the newly created symbol table, or NULL if creation failed.
 */
SymbolTable *symbolTableCreate(void);

/**
 * @brief Destroys the symbol table and frees associated memory.
 * @param table The symbol table to destroy.
 */
void symbolTableDestroy(SymbolTable *table);

/**
 * @brief Enters a new scope in the symbol table.
 * @param table The symbol table to operate on.
 * @return A CompilerStatus indicating success or failure.
 */
CompilerStatus symbolTableEnterScope(SymbolTable *table);

/**
 * @brief Leaves the current scope in the symbol table.
 * @param table The symbol table to operate on.
 * @return A CompilerStatus indicating success or failure.
 */
CompilerStatus symbolTableLeaveScope(SymbolTable *table);

/**
 * @brief Inserts a new symbol into the symbol table.
 * @param table The symbol table to operate on.
 * @param symbol The symbol to insert.
 * @return A CompilerStatus indicating success or failure.
 */
CompilerStatus symbolTableInsert(SymbolTable *table, Symbol *symbol);

/**
 * @brief Finds a symbol in the symbol table by name.
 * @param table The symbol table to search.
 * @param name The name of the symbol to find.
 * @return A pointer to the found symbol, or NULL if not found.
 */
Symbol *symbolTableFind(const SymbolTable *table, const char *name);

/**
 * @brief Finds a symbol in the current scope of the symbol table by name.
 * @param table The symbol table to search.
 * @param name The name of the symbol to find.
 * @return A pointer to the found symbol, or NULL if not found.
 */
Symbol *symbolTableFindCurrentScope(const SymbolTable *table, const char *name);

#endif

