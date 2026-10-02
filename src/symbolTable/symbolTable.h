/*
 * Project: IFJ26 compiler
 * Authors: TODO(team)
 * AI-assisted interface only.
 * Prompt: Define the symbol-table boundary without tree algorithms.
 */

#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "compilerTypes.h"

#include <stdbool.h>

typedef enum {
    SYMBOL_VARIABLE,
    SYMBOL_FUNCTION,
    SYMBOL_TYPE
} SymbolKind;

typedef struct {
    DataType *parameterTypes;
    size_t parameterCount;
    DataType returnType;
} FunctionSignature;

typedef struct {
    const char *name;
    SymbolKind kind;
    DataType dataType;
    bool mutable;
    bool defined;
    size_t scopeDepth;
    FunctionSignature function;
} Symbol;

typedef struct SymbolTable SymbolTable;

SymbolTable *symbolTableCreate(void);
void symbolTableDestroy(SymbolTable *table);
CompilerStatus symbolTableEnterScope(SymbolTable *table);
CompilerStatus symbolTableLeaveScope(SymbolTable *table);
CompilerStatus symbolTableInsert(SymbolTable *table, const Symbol *symbol);
const Symbol *symbolTableFind(const SymbolTable *table, const char *name);
const Symbol *symbolTableFindCurrentScope(const SymbolTable *table, const char *name);

#endif

