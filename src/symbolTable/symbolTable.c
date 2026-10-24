/*
 * Project: IFJ26 compiler
 * Authors: Václav Krytinář xkrytiv00
 * This file intentionally contains no AI-generated tree implementation.
 * Team TODO: implement the required height-balanced search tree manually.
 */

#include "symbolTable.h"
#include "avlTree.h"
#include "memory.h"

#include <string.h>

struct SymbolTable {
    size_t scopeDepth;
    void **toRemoveKeys;    //Array of arrays of keys to be removed when leaving a scope (with a NULL terminator)
    TAvlTree *tree;
};

struct Key {
    const char *name;
    size_t scopeDepth;
};


SymbolTable *symbolTableCreate(void)
{
    SymbolTable *table = memoryAlloc(sizeof(SymbolTable));
    if (!table) {
        return NULL;
    }

    table->scopeDepth = 0;
    table->toRemoveKeys = NULL;

    table->tree = avlTreeInit(symbolTableCompareKeys, symbolTableOnRemoveAction);
    if (table->tree == NULL) {
        free(table);
        return NULL;
    }

    return table;
}

void symbolTableDestroy(SymbolTable *table)
{
    avlTreeDestroy(table->tree);
    free(table);
}

CompilerStatus symbolTableEnterScope(SymbolTable *table)
{

}

CompilerStatus symbolTableLeaveScope(SymbolTable *table)
{

}

CompilerStatus symbolTableInsert(SymbolTable *table, const Symbol *symbol)
{

}

const Symbol *symbolTableFind(const SymbolTable *table, const char *name)
{

}

const Symbol *symbolTableFindCurrentScope(const SymbolTable *table, const char *name)
{

}

void *symbolTableOnRemoveAction(void *data, void *key)
{
    free(((Symbol *)data)->function.parameterTypes);
    free(data);
    free(key);
}

int symbolTableCompareKeys(const void *key1, const void *key2)
{
    const struct Key *k1 = (const struct Key *)key1;
    const struct Key *k2 = (const struct Key *)key2;

    int nameComparison = strcmp(k1->name, k2->name);
    if (nameComparison != 0) {
        return nameComparison;
    }

    return (int)(k1->scopeDepth - k2->scopeDepth);
}

void *symbolTableMakeKey(const char *name, size_t scopeDepth)
{
    struct Key *key = memoryAlloc(sizeof(struct Key));
    if (key == NULL) {
        return NULL;
    }

    key->name = name;
    key->scopeDepth = scopeDepth;

    return key;
}


