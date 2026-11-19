/*
 * Project: IFJ26 compiler
 * Authors: Václav Krytinář xkrytiv00
 * This file intentionally contains no AI-generated tree implementation.
 */

#include "symtable.h"
#include "symtablePrivate.h"
#include "bst.h"
#include "memory.h"
#include "typeDescriptor.h"

#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

struct SymbolTable {
    size_t scopeDepth;
    size_t removeListLen;
    void **removeList;
    TAvlTree *tree;
};

struct Key {
    const char *name;
    size_t scopeDepth;
};

SymbolTable *symbolTableCreate(void)
{
    SymbolTable *table = memoryAllocate(sizeof(SymbolTable));
    if (table == NULL) {
        return NULL;
    }

    table->scopeDepth = 1;

    table->removeListLen = 1;
    table->removeList = memoryAllocateArray(table->removeListLen, sizeof(void*));
    if (table->removeList == NULL) {
        free(table);
        return NULL;
    }

    table->removeList[0] = NULL;


    table->tree = avlTreeInit(symbolTableCompareKeys, symbolTableOnRemoveAction);
    if (table->tree == NULL) {
        free(table);
        return NULL;
    }

    return table;
}

void symbolTableDestroy(SymbolTable *table)
{
    if (table == NULL) {
        return;
    }

    avlTreeDestroy(table->tree);
    free(table->removeList);
    free(table);
}

CompilerStatus symbolTableEnterScope(SymbolTable *table)
{
    if (table == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    if(!symbolTableAddToRemoveList(table, NULL)) {
        return COMPILER_INTERNAL_ERROR;
    }

    ++table->scopeDepth;

    return COMPILER_OK;
}

CompilerStatus symbolTableLeaveScope(SymbolTable *table)
{
    if (table == NULL || table->scopeDepth <= 1) {
        return COMPILER_INTERNAL_ERROR;
    }

    for (size_t i = table->removeListLen - 2; table->removeList[i] != NULL; --i) {
        TAvlTree* result = avlTreeRemove(table->tree, table->removeList[i]);
        if (result == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        table->tree = result;
        --table->removeListLen;
    }

    --table->removeListLen;
    --table->scopeDepth;

    return COMPILER_OK;
}

CompilerStatus symbolTableInsert(SymbolTable *table, Symbol *symbol)
{
    if (table == NULL || symbol == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    Key *key = symbolTableMakeKey(symbol->name, table->scopeDepth);
    if (key == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    if (avlTreeInsert(table->tree, key, (void *)symbol) == NULL) {
        free(key);
        return COMPILER_INTERNAL_ERROR;
    }

    if (!symbolTableAddToRemoveList(table, key)) {
        return COMPILER_INTERNAL_ERROR;
    }

    return COMPILER_OK;
}

Symbol *symbolTableFind(const SymbolTable *table, const char *name)
{
    if (table == NULL || name == NULL) {
        return NULL;
    }

    size_t depth = table->scopeDepth;
    Key *key = symbolTableMakeKey(name, depth);
    if (key == NULL) {
        return NULL;
    }

    while (depth > 0) {
        if (avlTreeSearch(table->tree, key)) {
            Symbol *symbol = avlTreeGetData(table->tree, key);
            free(key);
            return symbol;
        }

        key->scopeDepth = --depth;
    }

    free(key);
    return NULL;
}

Symbol *symbolTableFindCurrentScope(const SymbolTable *table, const char *name)
{
    if (table == NULL || name == NULL) {
        return NULL;
    }

    Key *key = symbolTableMakeKey(name, table->scopeDepth);
    if (key == NULL) {
        return NULL;
    }

    if (avlTreeSearch(table->tree, key)) {
        Symbol *symbol = avlTreeGetData(table->tree, key);
        free(key);
        return symbol;
    }

    free(key);

    return NULL;
}

void symbolTableOnRemoveAction(void *data, void *key)
{
    Symbol *symbol = (Symbol *)data;

    if (symbol != NULL) {
        // free the base data type
        if (symbol->dataType != NULL) {
            typeDescriptorFree(symbol->dataType);
            free(symbol->dataType);
        }

        // free function parameters and return type
        if (symbol->kind == SYMBOL_FUNCTION || symbol->kind == SYMBOL_TYPE) {
            if (symbol->function.parameterTypes != NULL) {
                for (size_t i = 0; i < symbol->function.parameterCount; i++) {
                    if (symbol->function.parameterTypes[i] != NULL) {
                        typeDescriptorFree(symbol->function.parameterTypes[i]);
                        free(symbol->function.parameterTypes[i]);
                    }
                }
                free(symbol->function.parameterTypes);
            }

            if (symbol->function.returnType != NULL) {
                typeDescriptorFree(symbol->function.returnType);
                free(symbol->function.returnType);
            }

            free(symbol->function.parameterNames);
        }
    }

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
    if (name == NULL) {
        return NULL;
    }

    struct Key *key = memoryAllocate(sizeof(struct Key));
    if (key == NULL) {
        return NULL;
    }

    key->name = name;
    key->scopeDepth = scopeDepth;

    return (void*)key;
}

bool symbolTableAddToRemoveList(SymbolTable *table, void *key)
{
    if (table == NULL) {
        return false;
    }

    void **listTmp = memoryResizeArray(table->removeList, table->removeListLen + 1, sizeof(void*));
    if (listTmp == NULL) {
        return false;
    }

    table->removeList = listTmp;
    ++table->removeListLen;
    table->removeList[table->removeListLen - 2] = key;
    table->removeList[table->removeListLen - 1] = NULL;

    return true;
}

