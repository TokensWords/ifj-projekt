#ifndef SYMBOL_TABLE_PRIVATE_H
#define SYMBOL_TABLE_PRIVATE_H

#include "symbolTable.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct Key Key;

/**
 * @brief Action to be performed when a symbol is removed from the symbol table.
 * This function is called with the data and key of the removed symbol.
 * It can be used to free associated resources or perform other cleanup tasks.
 * @param data The data associated with the removed symbol.
 * @param key The key of the removed symbol.
 */
void symbolTableOnRemoveAction(void *data, void *key);

/**
 * @brief Creates a key for the symbol table based on the provided name and current scope depth.
 * This function allocates memory for the key and creates a unique representation of the symbol's name and scope depth.
 * The caller is responsible for freeing the allocated memory when it is no longer needed.
 * @param name The name to be used as the key.
 * @param scopeDepth The current scope depth.
 * @return A pointer to the newly created key, or NULL if memory allocation fails.
 */
void *symbolTableMakeKey(const char *name, size_t scopeDepth);

/**
 * @brief Compares two keys in the symbol table.
 * This function compares the names and scope depths of the two keys to determine their ordering.
 * It is used for maintaining the order of symbols in the symbol table.
 * @param key1 The first key to compare.
 * @param key2 The second key to compare.
 * @return A negative value if key1 < key2, 0 if key1 == key2, and a positive value if key1 > key2.
 */
int symbolTableCompareKeys(const void *key1, const void *key2);

/**
 * @brief Adds a key to the remove list.
 * @param table The symbol table to which the key should be added to the remove list.
 * @param key The key to be added to the remove list.
 * @return true if key was added to the remove list, false otherwise
 */
bool symbolTableAddToRemoveList(SymbolTable *table, void *key);

#endif // SYMBOL_TABLE_PRIVATE_H