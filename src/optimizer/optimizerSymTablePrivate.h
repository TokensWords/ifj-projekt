/*
 * Project: IFJ26 compiler
 * Authors: Václav Krytinář xkrytiv00
 */

#ifndef OPTIMIZER_SYM_TABLE_PRIVATE_H
#define OPTIMIZER_SYM_TABLE_PRIVATE_H


typedef struct Key Key;

/**
 * @brief Action to be performed when a symbol is removed from the symbol table.
 * This function is called with the data and key of the removed symbol.
 * It can be used to free associated resources or perform other cleanup tasks.
 * @param data The data associated with the removed symbol.
 * @param key The key of the removed symbol.
 */
void optimizerSymTableOnRemoveAction(void *data, void *key);

/**
 * @brief Creates a key for the symbol table based on the provided name.
 * The caller is responsible for freeing the allocated memory when it is no longer needed.
 * @param name The name to be used as the key.
 * @return A pointer to the newly created key, or NULL if memory allocation fails.
 */
void *optimizerSymTableMakeKey(const char *name);

/**
 * @brief Compares two keys in the symbol table.
 * @param key1 The first key to compare.
 * @param key2 The second key to compare.
 * @return A negative value if key1 < key2, 0 if key1 == key2, and a positive value if key1 > key2.
 */
int optimizerSymTableCompareKeys(const void *key1, const void *key2);

#endif // OPTIMIZER_SYM_TABLE_PRIVATE_H