#ifndef KNOWN_CONSTANTS_TABLE_PRIVATE_H
#define KNOWN_CONSTANTS_TABLE_PRIVATE_H

#include "knownConstantsTable.h"

typedef struct KnownConstantKey KnownConstantKey;

/**
 * @brief Action to be performed when a known constant is removed from the table.
 * @param data The known constant associated with the removed entry.
 * @param key The key of the removed entry.
 */
void knownConstantsTableOnRemoveAction(void *data, void *key);

/**
 * @brief Creates a key for the table based on the provided name.
 * The caller is responsible for freeing the allocated memory when it is no longer needed.
 * @param name The name to be used as the key.
 * @return A pointer to the newly created key, or NULL if memory allocation fails.
 */
void *knownConstantsTableMakeKey(const char *name);

/**
 * @brief Compares two keys in the table.
 * @param key1 The first key to compare.
 * @param key2 The second key to compare.
 * @return A negative value if key1 < key2, 0 if key1 == key2, and a positive value if key1 > key2.
 */
int knownConstantsTableCompareKeys(const void *key1, const void *key2);

#endif // KNOWN_CONSTANTS_TABLE_PRIVATE_H
