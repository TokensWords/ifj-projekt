/*
 * Project: IFJ26 compiler
 * Authors: Václav Krytinář xkrytiv00
 * This file intentionally contains no AI-generated tree implementation.
 * Team TODO: implement the required height-balanced search tree manually.
 */

#include "knownConstantsTable.h"
#include "knownConstantsTablePrivate.h"
#include "avlTree.h"

#include "memory.h"
#include "compilerTypes.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct KnownConstantsTable {
    TAvlTree *tree;
};

struct KnownConstantKey {
    const char *name;
};

KnownConstantsTable *knownConstantsTableCreate(void)
{
    KnownConstantsTable *table = memoryAllocate(sizeof(KnownConstantsTable));
    if (table == NULL) {
        return NULL;
    }

    table->tree = avlTreeInit(knownConstantsTableCompareKeys, knownConstantsTableOnRemoveAction);
    if (table->tree == NULL) {
        free(table);
        return NULL;
    }

    return table;
}

void knownConstantsTableDestroy(KnownConstantsTable *table)
{
    if (table == NULL) {
        return;
    }

    avlTreeDestroy(table->tree);
    free(table);
}

CompilerStatus knownConstantsTableInsert(KnownConstantsTable *table, KnownConstant *constant)
{
    if (table == NULL || constant == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    KnownConstantKey *key = knownConstantsTableMakeKey(constant->name);
    if (key == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    if (avlTreeInsert(table->tree, key, (void *)constant) == NULL) {
        free(key);
        return COMPILER_INTERNAL_ERROR;
    }

    return COMPILER_OK;
}

CompilerStatus knownConstantsTableRemove(KnownConstantsTable *table, const char *name)
{
    KnownConstantKey *key = knownConstantsTableMakeKey(name);
    if (key == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    TAvlTree* result = avlTreeRemove(table->tree, key);
    if (result == NULL) {
        free(key);
        return COMPILER_INTERNAL_ERROR;
    }

    table->tree = result;

    free(key);
    return COMPILER_OK;
}

KnownConstant *knownConstantsTableFind(const KnownConstantsTable *table, const char *name)
{
    if (table == NULL || name == NULL) {
        return NULL;
    }

    KnownConstantKey *key = knownConstantsTableMakeKey(name);
    if (key == NULL) {
        return NULL;
    }

    if (avlTreeSearch(table->tree, key)) {
        KnownConstant *constant = avlTreeGetData(table->tree, key);
        free(key);
        return constant;
    }

    free(key);
    return NULL;
}

void knownConstantsTableOnRemoveAction(void *data, void *key)
{
    free(((KnownConstant*)data)->name);
    free(((KnownConstant*)data)->stringValue);
    free(data);
    free(key);
}

int knownConstantsTableCompareKeys(const void *key1, const void *key2)
{
    const KnownConstantKey *k1 = (const KnownConstantKey *)key1;
    const KnownConstantKey *k2 = (const KnownConstantKey *)key2;

    return strcmp(k1->name, k2->name);
}

void *knownConstantsTableMakeKey(const char *name)
{
    if (name == NULL) {
        return NULL;
    }

    KnownConstantKey *key = memoryAllocate(sizeof(KnownConstantKey));
    if (key == NULL) {
        return NULL;
    }

    key->name = name;

    return (void *)key;
}
