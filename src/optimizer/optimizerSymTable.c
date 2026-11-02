/*
 * Project: IFJ26 compiler
 * Authors: Václav Krytinář xkrytiv00
 * This file intentionally contains no AI-generated tree implementation.
 * Team TODO: implement the required height-balanced search tree manually.
 */

#include "optimizerSymTable.h"
#include "optimizerSymTablePrivate.h"
#include "avlTree.h"

#include "memory.h"
#include "compilerTypes.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

struct OptimizerSymTable {
    TAvlTree *tree;
};

struct Key {
    const char *name;
};

OptimizerSymTable *optimizerSymTableCreate(void)
{
    OptimizerSymTable *table = memoryAllocate(sizeof(OptimizerSymTable));
    if (table == NULL) {
        return NULL;
    }

    table->tree = avlTreeInit(optimizerSymTableCompareKeys, optimizerSymTableOnRemoveAction);
    if (table->tree == NULL) {
        free(table);
        return NULL;
    }

    return table;
}

void optimizerSymTableDestroy(OptimizerSymTable *table)
{
    if (table == NULL) {
        return;
    }

    avlTreeDestroy(table->tree);
    free(table);
}

CompilerStatus optimizerSymTableInsert(OptimizerSymTable *table, OptimizerSymbol *symbol)
{
    if (table == NULL || symbol == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    Key *key = optimizerSymTableMakeKey(symbol->name);
    if (key == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    if (avlTreeInsert(table->tree, key, (void *)symbol) == NULL) {
        free(key);
        return COMPILER_INTERNAL_ERROR;
    }

    return COMPILER_OK;
}

OptimizerSymbol *optimizerSymTableFind(const OptimizerSymTable *table, const char *name)
{
    if (table == NULL || name == NULL) {
        return NULL;
    }

    Key *key = optimizerSymTableMakeKey(name);
    if (key == NULL) {
        return NULL;
    }

    if (avlTreeSearch(table->tree, key)) {
        OptimizerSymbol *symbol = avlTreeGetData(table->tree, key);
        free(key);
        return symbol;
    }

    free(key);
    return NULL;
}

void optimizerSymTableOnRemoveAction(void *data, void *key)
{
    free(data);
    free(key);
}

int optimizerSymTableCompareKeys(const void *key1, const void *key2)
{
    const struct Key *k1 = (const struct Key *)key1;
    const struct Key *k2 = (const struct Key *)key2;

    return strcmp(k1->name, k2->name);
}

void *optimizerSymTableMakeKey(const char *name)
{
    if (name == NULL) {
        return NULL;
    }

    struct Key *key = memoryAllocate(sizeof(struct Key));
    if (key == NULL) {
        return NULL;
    }

    key->name = name;

    return (void*)key;
}
