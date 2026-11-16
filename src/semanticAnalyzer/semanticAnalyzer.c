/*
 * Project: IFJ26 compiler
 * Authors: Václav Krytinář xkrytiv00
 * AI-assisted scaffold.
 * Prompt: Create a semantic-analysis integration stub.
 */

#include "semanticAnalyzer.h"
#include "abstractSyntaxTree.h"
#include "compilerTypes.h"
#include "memory.h"
#include "symbolTable.h"
#include "syntax.h"
#include "typeDescriptor.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct SemAnalData {
    size_t variableIdCounter;
    SymbolTable *functionSymbols;
    SymbolTable *algebdtSymbols;
    size_t numberOfTables;
    SymbolTable **symbolTables;
    const char **recursionList;
    AbstractSyntaxNode *root;
};

// type descriptor helpers
TypeDescriptor *semAnalInitTypeDescriptor(DataType type)
{
    TypeDescriptor *desc = memoryAllocate(sizeof(TypeDescriptor));
    if (desc != NULL) {
        typeDescriptorInit(desc);
        desc->datatypeBitflag = type;
    }
    return desc;
}

TypeDescriptor *semAnalCopyTypeDescriptor(const TypeDescriptor *src)
{
    if (src == NULL) {
        return NULL;
    }
    TypeDescriptor *desc = memoryAllocate(sizeof(TypeDescriptor));
    if (desc != NULL) {
        typeDescriptorInit(desc);
        desc->datatypeBitflag = src->datatypeBitflag;

        for (TypeDescriptorAliasNode *node = src->aliases; node != NULL; node = node->next) {
            TypeDescriptorAliasNode *newNode = NULL;
            if (typeDescriptorAliasNodeInit(&newNode, node->name) == COMPILER_OK) {
                typeDescriptorAddAlias(desc, newNode);
            }
        }
        for (TypeDescriptorAliasNode *node = src->classes; node != NULL; node = node->next) {
            TypeDescriptorAliasNode *newNode = NULL;
            if (typeDescriptorAliasNodeInit(&newNode, node->name) == COMPILER_OK) {
                typeDescriptorAddClass(desc, newNode);
            }
        }
        for (TypeDescriptorRecursiveNode *node = src->recursiveTypes; node != NULL; node = node->next) {
            TypeDescriptorRecursiveNode *newNode = NULL;
            if (typeDescriptorRecursiveNodeInit(&newNode, &node->elementType) == COMPILER_OK) {
                typeDescriptorAddRecursiveType(desc, newNode);
            }
        }
    }
    return desc;
}

void semAnalDestroyTypeDescriptor(TypeDescriptor *desc)
{
    if (desc != NULL) {
        typeDescriptorFree(desc);
        free(desc);
    }
}

// normalization helper to resolve alias vs case class ambiguity
CompilerStatus semAnalNormalizeTypeDescriptor(TypeDescriptor *desc, SemAnalData *data) {
    if (desc == NULL || data == NULL) {
        return COMPILER_OK;
    }

    if (desc->datatypeBitflag & DATA_TYPE_ALIAS) {
        size_t expansions = 0;
        TypeDescriptorAliasNode *prev = NULL;
        TypeDescriptorAliasNode *current = desc->aliases;

        while (current != NULL) {
            Symbol *typeSymbol = symbolTableFind(data->algebdtSymbols, current->name);

            if (typeSymbol != NULL && typeSymbol->kind == SYMBOL_TYPE) {
                // if it is also in functions - case class constructor -> case class not an alias
                Symbol *funcSymbol = symbolTableFind(data->functionSymbols, current->name);
                if (funcSymbol != NULL && funcSymbol->kind == SYMBOL_TYPE) {

                    // add to classes list
                    TypeDescriptorAliasNode *newNode = NULL;
                    CompilerStatus status = typeDescriptorAliasNodeInit(&newNode, current->name);
                    if (status == COMPILER_OK) {
                        typeDescriptorAddClass(desc, newNode);
                    }

                    // remove from aliases list
                    TypeDescriptorAliasNode *toRemove = current;
                    if (prev == NULL) {
                        desc->aliases = current->next;
                    } else {
                        prev->next = current->next;
                    }
                    current = current->next;

                    // free the old node
                    typeDescriptorAliasNodeFree(toRemove);

                    // no alias left
                    if (desc->aliases == NULL) {
                        desc->datatypeBitflag &= ~DATA_TYPE_ALIAS;
                    }
                    continue; // we have already moved to next node because of the removal
                } else if (typeSymbol->dataType != NULL) {
                    if (++expansions > data->root->childCount) {
                        return COMPILER_TYPE_ERROR;
                    }
                    TypeDescriptor *expanded = semAnalUnifyTypes(desc, typeSymbol->dataType);
                    if (expanded == NULL) {
                        return COMPILER_INTERNAL_ERROR;
                    }
                    TypeDescriptorAliasNode **link = &expanded->aliases;
                    while (*link != NULL && strcmp((*link)->name, current->name) != 0) {
                        link = &(*link)->next;
                    }
                    if (*link == NULL) {
                        semAnalDestroyTypeDescriptor(expanded);
                        return COMPILER_INTERNAL_ERROR;
                    }
                    TypeDescriptorAliasNode *resolved = *link;
                    *link = resolved->next;
                    resolved->next = NULL;
                    typeDescriptorAliasNodeFree(resolved);
                    if (expanded->aliases == NULL) {
                        expanded->datatypeBitflag &= ~DATA_TYPE_ALIAS;
                    }
                    typeDescriptorFree(desc);
                    *desc = *expanded;
                    free(expanded);
                    prev = NULL;
                    current = desc->aliases;
                    continue;
                }
            }
            prev = current;
            current = current->next;
        }
    }

    // recursively normalize lists
    for (TypeDescriptorRecursiveNode *recNode = desc->recursiveTypes; recNode != NULL; recNode = recNode->next) {
        CompilerStatus status = semAnalNormalizeTypeDescriptor(&recNode->elementType, data);
        if (status != COMPILER_OK) {
            return status;
        }
    }

    return COMPILER_OK;
}

bool semAnalIsSubtype(TypeDescriptor *superType, TypeDescriptor *subType)
{
    if (superType == NULL || subType == NULL) {
        return false;
    }

    // primitive types evaluation
    if ((superType->datatypeBitflag & subType->datatypeBitflag) != subType->datatypeBitflag) {
        // int to double allowed
        if (!((superType->datatypeBitflag & DATA_TYPE_DOUBLE) && (subType->datatypeBitflag == DATA_TYPE_INT))) {
            return false;
        }
    }

    // class evaluation
    if (subType->datatypeBitflag & DATA_TYPE_CASE_CLASS) {
        for (TypeDescriptorAliasNode *subClassNode = subType->classes; subClassNode != NULL; subClassNode = subClassNode->next) {
            bool found = false;
            for (TypeDescriptorAliasNode *superClassNode = superType->classes; superClassNode != NULL; superClassNode = superClassNode->next) {
                if (strcmp(subClassNode->name, superClassNode->name) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                return false;
            }
        }
    }

    // alias evaluation
    if (subType->datatypeBitflag & DATA_TYPE_ALIAS) {
        for (TypeDescriptorAliasNode *subAliasNode = subType->aliases; subAliasNode != NULL; subAliasNode = subAliasNode->next) {
            bool found = false;
            for (TypeDescriptorAliasNode *superAliasNode = superType->aliases; superAliasNode != NULL; superAliasNode = superAliasNode->next) {
                if (strcmp(subAliasNode->name, superAliasNode->name) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                return false;
            }
        }
    }

    // recursive list type evaluation
    if (subType->datatypeBitflag & DATA_TYPE_LIST) {
        for (TypeDescriptorRecursiveNode *subRecursiveNode = subType->recursiveTypes; subRecursiveNode != NULL; subRecursiveNode = subRecursiveNode->next) {
            bool foundSuperList = false;
            for (TypeDescriptorRecursiveNode *superRecursiveNode = superType->recursiveTypes; superRecursiveNode != NULL; superRecursiveNode = superRecursiveNode->next) {
                if (semAnalIsSubtype(&superRecursiveNode->elementType, &subRecursiveNode->elementType)) {
                    foundSuperList = true;
                    break;
                }
            }
            if (!foundSuperList) {
                return false;
            }
        }
    }

    return true;
}

TypeDescriptor *semAnalUnifyTypes(TypeDescriptor *typeA, TypeDescriptor *typeB)
{
    if (typeA == NULL) {
        return semAnalCopyTypeDescriptor(typeB);
    }
    if (typeB == NULL) {
        return semAnalCopyTypeDescriptor(typeA);
    }

    TypeDescriptor *unified = semAnalInitTypeDescriptor(typeA->datatypeBitflag | typeB->datatypeBitflag);
    if (unified == NULL) {
        return NULL;
    }

    // unify classes
    for (TypeDescriptorAliasNode *classANode = typeA->classes; classANode != NULL; classANode = classANode->next) {
        TypeDescriptorAliasNode *newNode = NULL;
        if (typeDescriptorAliasNodeInit(&newNode, classANode->name) == COMPILER_OK) {
            typeDescriptorAddClass(unified, newNode);
        }
    }
    for (TypeDescriptorAliasNode *classBNode = typeB->classes; classBNode != NULL; classBNode = classBNode->next) {
        bool exists = false;
        for (TypeDescriptorAliasNode *uniClassNode = unified->classes; uniClassNode != NULL; uniClassNode = uniClassNode->next) {
            if (strcmp(classBNode->name, uniClassNode->name) == 0) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            TypeDescriptorAliasNode *newNode = NULL;
            if (typeDescriptorAliasNodeInit(&newNode, classBNode->name) == COMPILER_OK) {
                typeDescriptorAddClass(unified, newNode);
            }
        }
    }

    // unify aliases
    for (TypeDescriptorAliasNode *aliasANode = typeA->aliases; aliasANode != NULL; aliasANode = aliasANode->next) {
        TypeDescriptorAliasNode *newNode = NULL;
        if (typeDescriptorAliasNodeInit(&newNode, aliasANode->name) == COMPILER_OK) {
            typeDescriptorAddAlias(unified, newNode);
        }
    }
    for (TypeDescriptorAliasNode *aliasBNode = typeB->aliases; aliasBNode != NULL; aliasBNode = aliasBNode->next) {
        bool exists = false;
        for (TypeDescriptorAliasNode *uniAliasNode = unified->aliases; uniAliasNode != NULL; uniAliasNode = uniAliasNode->next) {
            if (strcmp(aliasBNode->name, uniAliasNode->name) == 0) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            TypeDescriptorAliasNode *newNode = NULL;
            if (typeDescriptorAliasNodeInit(&newNode, aliasBNode->name) == COMPILER_OK) {
                typeDescriptorAddAlias(unified, newNode);
            }
        }
    }

    // recursively unify lists
    for (TypeDescriptorRecursiveNode *recANode = typeA->recursiveTypes; recANode != NULL; recANode = recANode->next) {
        TypeDescriptorRecursiveNode *newNode = NULL;
        if (typeDescriptorRecursiveNodeInit(&newNode, &recANode->elementType) == COMPILER_OK) {
            typeDescriptorAddRecursiveType(unified, newNode);
        }
    }
    for (TypeDescriptorRecursiveNode *recBNode = typeB->recursiveTypes; recBNode != NULL; recBNode = recBNode->next) {
        bool exists = false;
        for (TypeDescriptorRecursiveNode *uniRecNode = unified->recursiveTypes; uniRecNode != NULL; uniRecNode = uniRecNode->next) {
            if (semAnalIsSubtype(&uniRecNode->elementType, &recBNode->elementType) && semAnalIsSubtype(&recBNode->elementType, &uniRecNode->elementType)) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            TypeDescriptorRecursiveNode *newNode = NULL;
            if (typeDescriptorRecursiveNodeInit(&newNode, &recBNode->elementType) == COMPILER_OK) {
                typeDescriptorAddRecursiveType(unified, newNode);
            }
        }
    }

    return unified;
}

// semantic data hlepers

CompilerStatus semAnalDataInit(SemAnalData *data)
{
    if (data == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    data->root = NULL;
    data->variableIdCounter = 0;
    data->numberOfTables = 0;
    data->symbolTables = NULL;
    data->recursionList = NULL;
    data->functionSymbols = symbolTableCreate();
    if (data->functionSymbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    data->algebdtSymbols = symbolTableCreate();
    if (data->algebdtSymbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    return COMPILER_OK;
}

CompilerStatus semAnalListsBigger(SemAnalData *data) {
    if (data == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    SymbolTable *newTable = symbolTableCreate();
    if (newTable == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    SymbolTable **newTables = memoryResizeArray(data->symbolTables, data->numberOfTables + 1, sizeof(SymbolTable *));
    if (newTables == NULL) {
        symbolTableDestroy(newTable);
        return COMPILER_INTERNAL_ERROR;
    }
    data->symbolTables = newTables;
    data->symbolTables[data->numberOfTables] = newTable;

    const char **newRecList =
        memoryResizeArray(data->recursionList, data->numberOfTables + 1, sizeof(const char *));
    if (newRecList == NULL) {
        symbolTableDestroy(newTable);
        data->symbolTables[data->numberOfTables] = NULL;
        return COMPILER_INTERNAL_ERROR;
    }
    data->recursionList = newRecList;
    data->recursionList[data->numberOfTables] = NULL;

    ++data->numberOfTables;
    return COMPILER_OK;
}

CompilerStatus semAnalListsSmaller(SemAnalData *data) {
    if (data == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    if (data->numberOfTables == 0) {
        return COMPILER_OK;
    }

    size_t lastTableI = data->numberOfTables - 1;
    symbolTableDestroy(data->symbolTables[lastTableI]);
    data->symbolTables[lastTableI] = NULL;
    data->recursionList[lastTableI] = NULL;
    --data->numberOfTables;

    SymbolTable **smallerTables =
        memoryResizeArray(data->symbolTables, data->numberOfTables, sizeof(SymbolTable *));
    if (smallerTables == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    data->symbolTables = smallerTables;

    const char **smallerRecList =
        memoryResizeArray(data->recursionList, data->numberOfTables, sizeof(const char *));
    if (smallerRecList == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    data->recursionList = smallerRecList;

    return COMPILER_OK;
}

void semAnalDataDestroy(SemAnalData *data) {
    if (data == NULL) {
        return;
    }

    for (size_t tableI = 0; tableI < data->numberOfTables; ++tableI) {
        symbolTableDestroy(data->symbolTables[tableI]);
    }
    free(data->symbolTables);
    free(data->recursionList);
    symbolTableDestroy(data->functionSymbols);
    symbolTableDestroy(data->algebdtSymbols);
    data->variableIdCounter = 0;
    data->functionSymbols = NULL;
    data->algebdtSymbols = NULL;
    data->numberOfTables = 0;
    data->symbolTables = NULL;
    data->recursionList = NULL;
    data->root = NULL;
}

// core anlysis functions

CompilerStatus semanticAnalyzerAnalyze(SourceProgram *program) {
    if (program == NULL || program->root == NULL || program->root->kind != ABSTRACT_SYNTAX_PROGRAM) {
        return COMPILER_INTERNAL_ERROR;
    }

    CompilerStatus status = COMPILER_OK;
    AbstractSyntaxNode *root = program->root;

    SemAnalData data;
    status = semAnalDataInit(&data);
    if (status != COMPILER_OK) {
        return status;
    }
    data.root = root;

    status = semAnalDefineBuiltinFunctions(data.functionSymbols);
    if (status != COMPILER_OK) {
        semAnalDataDestroy(&data);
        return status;
    }

    status = semAnalDeclareFunctions(root, &data);
    if (status != COMPILER_OK) {
        semAnalDataDestroy(&data);
        return status;
    }

    // code analysis, loop to define all functions
    for (size_t funI = 0; funI < root->childCount; ++funI) {
        if (root->children[funI]->kind != ABSTRACT_SYNTAX_FUNCTION) {
            continue;
        }

        Symbol *tmpSymbol = symbolTableFindCurrentScope(data.functionSymbols, root->children[funI]->text);
        if (tmpSymbol == NULL) {
            semAnalDataDestroy(&data);
            return COMPILER_INTERNAL_ERROR;
        }

        if (!tmpSymbol->defined) {
            status = semAnalDefineFunction(root->children[funI], &data);
            if (status != COMPILER_OK) {
                semAnalDataDestroy(&data);
                return status;
            }
        }
    }

    semAnalDataDestroy(&data);
    return status;
}


CompilerStatus semAnalResolveGlobalAlgebdt(AbstractSyntaxNode *algebdtNode, SemAnalData *data)
{
    if (algebdtNode == NULL || data == NULL || data->algebdtSymbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    CompilerStatus status = COMPILER_OK;

    // redefinition check
    Symbol *varSymbol = symbolTableFind(data->algebdtSymbols, algebdtNode->text);
    if (varSymbol != NULL) {
        return COMPILER_REDEFINITION_ERROR;
    }

    if (algebdtNode->kind == ABSTRACT_SYNTAX_CASE_CLASS && algebdtNode->type.classes == NULL) {
        TypeDescriptorAliasNode *className = NULL;
        status = typeDescriptorAliasNodeInit(&className, algebdtNode->text);
        if (status != COMPILER_OK) {
            return status;
        }
        status = typeDescriptorAddClass(&algebdtNode->type, className);
        if (status != COMPILER_OK) {
            typeDescriptorAliasNodeFree(className);
            return status;
        }
    }

    Symbol *algebdtSymbol = semAnalAstNodeToSymbol(algebdtNode);
    if (algebdtSymbol == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    status = semAnalNormalizeTypeDescriptor(algebdtSymbol->dataType, data);
    if (status != COMPILER_OK) {
        semAnalFreeSymbol(algebdtSymbol);
        return status;
    }

    status = symbolTableInsert(data->algebdtSymbols, algebdtSymbol);
    if (status != COMPILER_OK) {
        semAnalFreeSymbol(algebdtSymbol);
        return status;
    }

    // adding the class constructor to functions
    if (algebdtNode->kind == ABSTRACT_SYNTAX_CASE_CLASS) {
        // redefinition check
        Symbol *varSymbolConstructor = symbolTableFind(data->functionSymbols, algebdtNode->text);
        if (varSymbolConstructor != NULL) {
            return COMPILER_REDEFINITION_ERROR;
        }

        Symbol *classConstructorSymbol = semAnalAstNodeToSymbol(algebdtNode);
        if (classConstructorSymbol == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        classConstructorSymbol->kind = SYMBOL_TYPE;
        classConstructorSymbol->defined = true;

        status = symbolTableInsert(data->functionSymbols, classConstructorSymbol);
        if (status != COMPILER_OK) {
            semAnalFreeSymbol(classConstructorSymbol);
            return status;
        }
    }

    return COMPILER_OK;
}

CompilerStatus semAnalDeclareFunctions(AbstractSyntaxNode *rootNode, SemAnalData *data)
{
    if (rootNode == NULL || data == NULL || data->functionSymbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    SymbolTable *symbols = data->functionSymbols;
    CompilerStatus status = COMPILER_OK;

    bool hasMain = false;

    for (size_t funI = 0; funI < rootNode->childCount; ++funI) {
        AbstractSyntaxNode *currentNode = rootNode->children[funI];

        // check for global scope algebraic types definitions (types/classes)
        if (currentNode->kind != ABSTRACT_SYNTAX_FUNCTION) {
            status = semAnalResolveGlobalAlgebdt(currentNode, data);
            if (status != COMPILER_OK) {
                return status;
            }
            continue; // node not a function - continuing
        }

        // multiple mains
        if (hasMain && currentNode->isEntryFunction) {
            return COMPILER_REDEFINITION_ERROR;
        }

        // check if main is main, if not main then ok
        status = semAnalCheckMain(currentNode);
        if (status != COMPILER_OK) {
            return status;
        }

        // update hasMain
        hasMain = hasMain || currentNode->isEntryFunction;

        // create symbol
        Symbol *symbolTmp = semAnalAstNodeToSymbol(currentNode);
        if (symbolTmp == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        // symbol already exists
        if (symbolTableFindCurrentScope(symbols, symbolTmp->name) != NULL) {
            semAnalFreeSymbol(symbolTmp);
            return COMPILER_REDEFINITION_ERROR;
        }

        // insert
        status = symbolTableInsert(symbols, symbolTmp);
        if (status != COMPILER_OK) {
            semAnalFreeSymbol(symbolTmp);
            return status;
        }
    }

    // no main
    if (!hasMain) {
        return COMPILER_UNDEFINED_SYMBOL_ERROR;
    }

    return COMPILER_OK;
}

CompilerStatus semAnalCheckMain(AbstractSyntaxNode *functionNode)
{
    if (functionNode == NULL || functionNode->childCount == 0) {
        return COMPILER_INTERNAL_ERROR;
    }

    if (!functionNode->isEntryFunction) {
        return COMPILER_OK;
    }

    size_t parameterCount = functionNode->childCount - 1;
    if (parameterCount != 0) {
        return COMPILER_ARGUMENT_ERROR;
    }

    if (functionNode->type.datatypeBitflag != DATA_TYPE_UNKNOWN && functionNode->type.datatypeBitflag != DATA_TYPE_UNIT) {
        return COMPILER_TYPE_ERROR;
    }

    return COMPILER_OK;
}

Symbol *semAnalCreateSymbolDefault() {
    Symbol *symbol = memoryAllocate(sizeof(Symbol));
    if (symbol == NULL) {
        return NULL;
    }

    symbol->name = NULL;
    symbol->kind = SYMBOL_FUNCTION;
    symbol->dataType = NULL;
    symbol->mutable = false;
    symbol->defined = false;
    symbol->function.parameterTypes = NULL;
    symbol->function.parameterNames = NULL;
    symbol->function.parameterCount = 0;
    symbol->function.returnType = NULL;

    return symbol;
}

void semAnalFreeSymbol(Symbol *symbol)
{
    if (symbol == NULL) {
        return;
    }

    // free parameters
    if (symbol->kind == SYMBOL_FUNCTION || symbol->kind == SYMBOL_TYPE) {
        TypeDescriptor **params = symbol->function.parameterTypes;
        if (params != NULL) {
            for (size_t paramI = 0; paramI < symbol->function.parameterCount; ++paramI) {
                semAnalDestroyTypeDescriptor(params[paramI]);
            }
            free(params);
        }
    }

    semAnalDestroyTypeDescriptor(symbol->dataType);
    semAnalDestroyTypeDescriptor(symbol->function.returnType);

    free(symbol->function.parameterNames);
    free(symbol);
}


TypeDescriptor **semAnalAstGetParameterTypes(AbstractSyntaxNode *node) {
    if (node == NULL) {
        return NULL;
    }

    size_t count = 0;
    if (node->kind == ABSTRACT_SYNTAX_FUNCTION) {
        if (node->childCount > 0) {
            count = node->childCount - 1;
        }
    } else if (node->kind == ABSTRACT_SYNTAX_CASE_CLASS) {
        count = node->childCount;
    } else {
        return NULL;
    }

    if (count == 0) {
        return NULL;
    }

    TypeDescriptor **types = memoryAllocate(sizeof(TypeDescriptor *) * count);
    if (types == NULL) {
        return NULL;
    }

    for (size_t paramI = 0; paramI < count; paramI++) {
        AbstractSyntaxNode *child = node->children[paramI];
        if (child == NULL || (child->kind != ABSTRACT_SYNTAX_PARAMETER && child->kind != ABSTRACT_SYNTAX_CASE_CLASS_FIELD)) {
            for (size_t cleanupI = 0; cleanupI < paramI; ++cleanupI) {
                semAnalDestroyTypeDescriptor(types[cleanupI]);
            }
            free(types);
            return NULL;
        }

        types[paramI] = semAnalCopyTypeDescriptor(&child->type);
    }

    return types;
}

char **semAnalAstGetParameterNames(AbstractSyntaxNode *node) {
    if (node == NULL) {
        return NULL;
    }

    size_t count = 0;
    if (node->kind == ABSTRACT_SYNTAX_FUNCTION) {
        if (node->childCount > 0) {
            count = node->childCount - 1;
        }
    } else if (node->kind == ABSTRACT_SYNTAX_CASE_CLASS) {
        count = node->childCount;
    } else {
        return NULL;
    }

    if (count == 0) {
        return NULL;
    }

    char **names = memoryAllocate(sizeof(char *) * count);
    if (names == NULL) {
        return NULL;
    }

    for (size_t paramI = 0; paramI < count; paramI++) {
        AbstractSyntaxNode *child = node->children[paramI];
        if (child == NULL || (child->kind != ABSTRACT_SYNTAX_PARAMETER && child->kind != ABSTRACT_SYNTAX_CASE_CLASS_FIELD)) {
            free(names);
            return NULL;
        }

        names[paramI] = child->text;
    }

    return names;
}


Symbol *semAnalAstNodeToSymbol(AbstractSyntaxNode *node) {
    if (node == NULL) {
        return NULL;
    }

    Symbol *symbol = semAnalCreateSymbolDefault();
    if (symbol == NULL) {
        return NULL;
    }

    // resolve kind
    if (node->kind == ABSTRACT_SYNTAX_FUNCTION) {
        symbol->kind = SYMBOL_FUNCTION;
    } else if (node->kind == ABSTRACT_SYNTAX_PARAMETER || node->kind == ABSTRACT_SYNTAX_VARIABLE_DECLARATION) {
        symbol->kind = SYMBOL_VARIABLE;
    } else if (node->kind == ABSTRACT_SYNTAX_TYPE_ALIAS || node->kind == ABSTRACT_SYNTAX_CASE_CLASS) {
        symbol->kind = SYMBOL_TYPE;
    } else {
        free(symbol);
        return NULL;
    }

    symbol->name = node->text;
    symbol->dataType = semAnalCopyTypeDescriptor(&node->type);
    symbol->mutable = node->isMutable;

    // case classes are treated structurally like functions here for parameter extraction
    if (symbol->kind == SYMBOL_FUNCTION || node->kind == ABSTRACT_SYNTAX_CASE_CLASS) {
        size_t paramCount = 0;
        if (node->kind == ABSTRACT_SYNTAX_FUNCTION) {
            if (node->childCount > 0) {
                paramCount = node->childCount - 1;
            }
        } else if (node->kind == ABSTRACT_SYNTAX_CASE_CLASS) {
            paramCount = node->childCount;
        }

        symbol->function.parameterTypes = semAnalAstGetParameterTypes(node);
        if (paramCount > 0 && symbol->function.parameterTypes == NULL) {
            free(symbol);
            return NULL;
        }

        symbol->function.parameterNames = semAnalAstGetParameterNames(node);
        if (paramCount > 0 && symbol->function.parameterNames == NULL) {
            free(symbol->function.parameterTypes);
            free(symbol);
            return NULL;
        }

        symbol->function.parameterCount = paramCount;
        symbol->function.returnType = semAnalCopyTypeDescriptor(&node->type);
    }

    return symbol;
}


bool semAnalIsIfExpression(AbstractSyntaxNode *ifNode)
{
    if (ifNode == NULL || (ifNode->kind != ABSTRACT_SYNTAX_IF && ifNode->kind != ABSTRACT_SYNTAX_IF_EXPRESSION) || ifNode->childCount < 2) {
        return false;
    }

    if (ifNode->childCount < 3) {
        return false; // missing else branch - not a expression
    }

    TypeDescriptor *inferred = &ifNode->type;
    if (inferred == NULL || inferred->datatypeBitflag == DATA_TYPE_UNKNOWN) {
        return false; // branches do not return same types
    }

    return true;
}

bool semAnalCheckIfReturns(AbstractSyntaxNode *ifNode) {
    if (ifNode == NULL || (ifNode->kind != ABSTRACT_SYNTAX_IF && ifNode->kind != ABSTRACT_SYNTAX_IF_EXPRESSION) || ifNode->childCount < 2) {
        return false;
    }

    if (ifNode->childCount < 3) {
        return false; // missing else branch evaluates to unit in return evaluations
    }

    bool allBranchesReturn = true;
    for (size_t branchI = 1; branchI < ifNode->childCount; ++branchI) {
        if (!semAnalNodeEndsWithReturn(ifNode->children[branchI])) {
            allBranchesReturn = false;
        }
    }

    return allBranchesReturn;
}

bool semAnalNodeEndsWithReturn(AbstractSyntaxNode *node) {
    if (node == NULL) {
        return false;
    }

    if (node->kind == ABSTRACT_SYNTAX_RETURN) {
        return true;
    }

    if (node->kind == ABSTRACT_SYNTAX_BLOCK) {
        if (node->childCount == 0) {
            return false;
        }

        AbstractSyntaxNode *lastStatement = node->children[node->childCount - 1];
        return semAnalNodeEndsWithReturn(lastStatement);
    }

    if (node->kind == ABSTRACT_SYNTAX_IF || node->kind == ABSTRACT_SYNTAX_IF_EXPRESSION) {
        return semAnalCheckIfReturns(node);
    }

    return false;
}

static CompilerStatus setListType(AbstractSyntaxNode *node, const TypeDescriptor *elementType) {
    TypeDescriptorRecursiveNode *element = NULL;
    CompilerStatus status = typeDescriptorRecursiveNodeInit(&element, elementType);
    if (status != COMPILER_OK) {
        return status;
    }
    typeDescriptorFree(&node->type);
    status = typeDescriptorAddRecursiveType(&node->type, element);
    if (status != COMPILER_OK) {
        typeDescriptorRecursiveNodeFree(element);
    }
    return status;
}

static bool isUntypedList(const TypeDescriptor *type) {
    return type->datatypeBitflag == DATA_TYPE_LIST && type->recursiveTypes == NULL;
}

static CompilerStatus applyListContext(AbstractSyntaxNode *node, const TypeDescriptor *expected) {
    if (!isUntypedList(&node->type)) {
        return COMPILER_OK;
    }
    if (expected == NULL || expected->recursiveTypes == NULL || expected->recursiveTypes->next != NULL) {
        return COMPILER_TYPE_ERROR;
    }
    return setListType(node, &expected->recursiveTypes->elementType);
}

static CompilerStatus resolveBlock(AbstractSyntaxNode *blockNode, SemAnalData *data, TypeDescriptor *expected);

static bool isListConstructor(const AbstractSyntaxNode *node) {
    if (node->kind != ABSTRACT_SYNTAX_CALL_EXPRESSION || node->childCount == 0) {
        return false;
    }
    const AbstractSyntaxNode *callee = node->children[0];
    return callee != NULL && callee->kind == ABSTRACT_SYNTAX_IDENTIFIER && callee->text != NULL &&
           strcmp(callee->text, "List") == 0;
}

static bool isListConcatenation(const AbstractSyntaxNode *node) {
    return node->kind == ABSTRACT_SYNTAX_EXPRESSION_BINARY && node->text != NULL && strcmp(node->text, "++") == 0;
}

static CompilerStatus resolveWithListContext(AbstractSyntaxNode *node, TypeDescriptor *expected, SemAnalData *data) {
    bool hasExpectedElement = expected != NULL && expected->recursiveTypes != NULL && expected->recursiveTypes->next == NULL;
    if (hasExpectedElement && node->kind == ABSTRACT_SYNTAX_IF_EXPRESSION && node->childCount == 3) {
        CompilerStatus status = semAnalResolveNode(node->children[0], data);
        if (status != COMPILER_OK) {
            return status;
        }
        if (node->children[0]->type.datatypeBitflag != DATA_TYPE_BOOLEAN) {
            return COMPILER_TYPE_ERROR;
        }
        for (size_t counter = 1; counter < node->childCount; counter++) {
            AbstractSyntaxNode *branch = node->children[counter];
            status = resolveWithListContext(branch, expected, data);
            if (status != COMPILER_OK) {
                return status;
            }
            if (!semAnalIsSubtype(expected, &branch->type)) {
                return COMPILER_TYPE_ERROR;
            }
        }
        return abstractSyntaxNodeCopyInferredType(node, expected);
    }
    if (hasExpectedElement && expected->datatypeBitflag == DATA_TYPE_LIST && isListConcatenation(node)) {
        for (size_t counter = 0; counter < node->childCount; counter++) {
            AbstractSyntaxNode *part = node->children[counter];
            CompilerStatus status = resolveWithListContext(part, expected, data);
            if (status != COMPILER_OK) {
                return status;
            }
            if (!semAnalIsSubtype(expected, &part->type)) {
                return COMPILER_TYPE_ERROR;
            }
        }
        return abstractSyntaxNodeCopyInferredType(node, expected);
    }
    if (!hasExpectedElement || !isListConstructor(node)) {
        CompilerStatus status = semAnalResolveNode(node, data);
        if (status != COMPILER_OK) {
            return status;
        }
        return applyListContext(node, expected);
    }

    TypeDescriptor *element = &expected->recursiveTypes->elementType;
    for (size_t counter = 1; counter < node->childCount; counter++) {
        AbstractSyntaxNode *argument = node->children[counter];
        CompilerStatus status = resolveWithListContext(argument, element, data);
        if (status != COMPILER_OK) {
            return status;
        }
        if (!semAnalIsSubtype(element, &argument->type)) {
            return COMPILER_TYPE_ERROR;
        }
    }
    return setListType(node, element);
}

CompilerStatus semAnalResolveFunctionReturnType(AbstractSyntaxNode *functionNode,
                                                Symbol *functionSymbol) {
    AbstractSyntaxNode *lastNode = functionNode->children[functionNode->childCount - 1];

    // get the last type in the body
    TypeDescriptor *lastType = semAnalInitTypeDescriptor(DATA_TYPE_UNKNOWN);
    if (lastType == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    if (lastNode->kind == ABSTRACT_SYNTAX_BLOCK && lastNode->childCount != 0) {
        lastNode = lastNode->children[lastNode->childCount - 1];
    }

    AbstractSyntaxNode *value = lastNode;
    if (lastNode->kind == ABSTRACT_SYNTAX_EXPRESSION_STATEMENT && lastNode->childCount == 1) {
        value = lastNode->children[0];
    }
    if (isUntypedList(&value->type)) {
        CompilerStatus status = applyListContext(value, functionSymbol->function.returnType);
        if (status == COMPILER_OK && value != lastNode) {
            status = abstractSyntaxNodeCopyInferredType(lastNode, &value->type);
        }
        if (status != COMPILER_OK) {
            semAnalDestroyTypeDescriptor(lastType);
            return status;
        }
    }

    // if statement last logic
    if (lastNode->kind == ABSTRACT_SYNTAX_IF || lastNode->kind == ABSTRACT_SYNTAX_IF_EXPRESSION) {
        if ((functionNode->type.datatypeBitflag == DATA_TYPE_UNIT) || semAnalCheckIfReturns(lastNode) || (lastNode->type.datatypeBitflag == DATA_TYPE_UNIT) || semAnalIsIfExpression(lastNode)) {
            // explicit Unit return type - if is always valid
            // if has explicit return in all branches
            // all branches evaluate to Unit
            // if is a vlaid expression
            semAnalDestroyTypeDescriptor(lastType);
            lastType = semAnalEvaluatesToType(lastNode);
        } else {
            semAnalDestroyTypeDescriptor(lastType);
            return COMPILER_TYPE_ERROR; // invalid if at the end
        }
    } else {
        semAnalDestroyTypeDescriptor(lastType);
        lastType = semAnalEvaluatesToType(lastNode);
    }

    // get the correct function type
    TypeDescriptor *funcRetType = functionSymbol->function.returnType;
    if (funcRetType == NULL || funcRetType->datatypeBitflag == DATA_TYPE_UNKNOWN) {
        semAnalDestroyTypeDescriptor(functionSymbol->function.returnType);
        functionSymbol->function.returnType = semAnalCopyTypeDescriptor(lastType);
    } else if (functionNode->type.datatypeBitflag != DATA_TYPE_UNIT && !semAnalIsSubtype(funcRetType, lastType)) {
        semAnalDestroyTypeDescriptor(lastType);
        return COMPILER_TYPE_ERROR;
    }

    abstractSyntaxNodeCopyInferredType(functionNode, functionSymbol->function.returnType);

    semAnalDestroyTypeDescriptor(lastType);
    return COMPILER_OK;
}

CompilerStatus semAnalDefineFunction(AbstractSyntaxNode *functionNode, SemAnalData *data) {
    if (functionNode == NULL || data == NULL || data->functionSymbols == NULL || functionNode->kind != ABSTRACT_SYNTAX_FUNCTION) {
        return COMPILER_INTERNAL_ERROR;
    }

    CompilerStatus status = COMPILER_OK;

    Symbol *functionSymbol = symbolTableFind(data->functionSymbols, functionNode->text);
    if (functionSymbol == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    status = semAnalListsBigger(data);
    if (status != COMPILER_OK) {
        return status;
    }

    data->recursionList[data->numberOfTables - 1] = functionNode->text;

    status = semAnalNormalizeTypeDescriptor(&functionNode->type, data);
    if (status != COMPILER_OK) {
        return status;
    }
    status = semAnalNormalizeTypeDescriptor(functionSymbol->function.returnType, data);
    if (status != COMPILER_OK) {
        return status;
    }

    // declaring parameters - adding to symbol table
    for (size_t paramI = 0; paramI < functionNode->childCount - 1; ++paramI) {
        status = semAnalDeclareParameter(functionNode->children[paramI], data);
        if (status != COMPILER_OK) {
            return status;
        }
    }

    AbstractSyntaxNode *lastNode = functionNode->children[functionNode->childCount - 1];

    // resolving the body
    status = resolveBlock(lastNode, data, functionSymbol->function.returnType);
    if (status != COMPILER_OK) {
        return status;
    }

    status = semAnalListsSmaller(data);
    if (status != COMPILER_OK) {
        return status;
    }

    status = semAnalResolveFunctionReturnType(functionNode, functionSymbol);
    if (status != COMPILER_OK) {
        return status;
    }
    functionSymbol->defined = true;
    return COMPILER_OK;
}

CompilerStatus semAnalDeclareParameter(AbstractSyntaxNode *parameterNode, SemAnalData *data) {
    if (parameterNode == NULL || data == NULL || data->symbolTables[data->numberOfTables - 1] == NULL || parameterNode->kind != ABSTRACT_SYNTAX_PARAMETER) {
        return COMPILER_INTERNAL_ERROR;
    }

    SymbolTable *currentTable = data->symbolTables[data->numberOfTables - 1];

    CompilerStatus status = COMPILER_OK;

    // move class cases form the aliases
    status = semAnalNormalizeTypeDescriptor(&parameterNode->type, data);
    if (status != COMPILER_OK) {
        return status;
    }

    Symbol *symbolTmp = symbolTableFind(currentTable, parameterNode->text);
    if (symbolTmp == NULL) {
        return COMPILER_REDEFINITION_ERROR;
    }

    Symbol *parameterSymbol = semAnalAstNodeToSymbol(parameterNode);
    if (parameterSymbol == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    parameterSymbol->variableId = ++data->variableIdCounter;

    status = symbolTableInsert(currentTable, parameterSymbol);
    if (status != COMPILER_OK) {
        semAnalFreeSymbol(parameterSymbol);
        return status;
    }

    parameterNode->variableId = parameterSymbol->variableId;

    return status;
}

static CompilerStatus resolveRange(AbstractSyntaxNode *node, SemAnalData *data) {
    for (size_t counter = 0; counter < node->childCount; counter++) {
        AbstractSyntaxNode *part = node->children[counter];
        CompilerStatus status = semAnalResolveNode(part, data);
        if (status != COMPILER_OK) {
            return status;
        }
        if (part->type.datatypeBitflag != DATA_TYPE_INT) {
            return COMPILER_TYPE_ERROR;
        }
    }

    if (node->childCount == 3) {
        AbstractSyntaxNode *stepNode = node->children[2];
        if (stepNode->kind == ABSTRACT_SYNTAX_INT_LITERAL && stepNode->value.intValue <= 0) {
            return COMPILER_SEMANTIC_ERROR;
        }
    }

    return COMPILER_OK;
}

CompilerStatus semAnalResolveNode(AbstractSyntaxNode *node, SemAnalData *data) {
    if (node == NULL) {
        return COMPILER_OK;
    }

    switch (node->kind) {
        case ABSTRACT_SYNTAX_BLOCK: return semAnalResolveBlock(node, data);
        case ABSTRACT_SYNTAX_IF:
        case ABSTRACT_SYNTAX_IF_EXPRESSION: return semAnalResolveIf(node, data);
        case ABSTRACT_SYNTAX_WHILE: return semAnalResolveWhile(node, data);
        case ABSTRACT_SYNTAX_RETURN: return semAnalResolveReturn(node, data);
        case ABSTRACT_SYNTAX_CALL_EXPRESSION: return semAnalResolveFunctionCall(node, data);
        case ABSTRACT_SYNTAX_VARIABLE_DECLARATION: return semAnalResolveVariableDeclaration(node, data);
        case ABSTRACT_SYNTAX_ASSIGNMENT: return semAnalResolveAssignment(node, data);
        case ABSTRACT_SYNTAX_EXPRESSION_STATEMENT: return semAnalResolveExpressionStatement(node, data);
        case ABSTRACT_SYNTAX_EXPRESSION_BINARY: return semAnalResolveExpressionBinary(node, data);
        case ABSTRACT_SYNTAX_UNARY_EXPRESSION: return semAnalResolveUnaryExpression(node, data);
        case ABSTRACT_SYNTAX_IDENTIFIER: return semAnalResolveIdentifier(node, data);
        case ABSTRACT_SYNTAX_FOR: return semAnalResolveFor(node, data);
        case ABSTRACT_SYNTAX_MEMBER_ACCESS: return semAnalResolveMemberAccess(node, data);
        case ABSTRACT_SYNTAX_MATCH_EXPRESSION: return semAnalResolveMatchExpression(node, data);
        case ABSTRACT_SYNTAX_CASE_CLASS: return semAnalResolvecCaseClass(node, data);
        case ABSTRACT_SYNTAX_INT_LITERAL:
            abstractSyntaxNodeSetInferredType(node, DATA_TYPE_INT);
            return COMPILER_OK;
        case ABSTRACT_SYNTAX_DOUBLE_LITERAL:
            abstractSyntaxNodeSetInferredType(node, DATA_TYPE_DOUBLE);
            return COMPILER_OK;
        case ABSTRACT_SYNTAX_STRING_LITERAL:
            abstractSyntaxNodeSetInferredType(node, DATA_TYPE_STRING);
            return COMPILER_OK;
        case ABSTRACT_SYNTAX_BOOLEAN_LITERAL:
            abstractSyntaxNodeSetInferredType(node, DATA_TYPE_BOOLEAN);
            return COMPILER_OK;
        case ABSTRACT_SYNTAX_UNIT_LITERAL:
            abstractSyntaxNodeSetInferredType(node, DATA_TYPE_UNIT);
            return COMPILER_OK;
        default:
            return COMPILER_INTERNAL_ERROR;
    }
}

CompilerStatus semAnalResolvecCaseClass(AbstractSyntaxNode *classNode, SemAnalData *data)
{
    if (symbolTableFindCurrentScope(data->algebdtSymbols, classNode->text) != NULL) {
        return COMPILER_REDEFINITION_ERROR;
    }

    Symbol *algebdtSymbol = semAnalAstNodeToSymbol(classNode);
    if (algebdtSymbol == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    CompilerStatus status = symbolTableInsert(data->algebdtSymbols, algebdtSymbol);
    if (status != COMPILER_OK) {
        semAnalFreeSymbol(algebdtSymbol);
        return status;
    }

    if (symbolTableFindCurrentScope(data->functionSymbols, classNode->text) != NULL) {
        return COMPILER_REDEFINITION_ERROR;
    }

    classNode->kind = ABSTRACT_SYNTAX_FUNCTION;
    Symbol *classConstructorSymbol = semAnalAstNodeToSymbol(classNode);
    if (classConstructorSymbol == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    classConstructorSymbol->kind = SYMBOL_TYPE;
    classConstructorSymbol->defined = true;

    status = symbolTableInsert(data->functionSymbols, classConstructorSymbol);
    if (status != COMPILER_OK) {
        semAnalFreeSymbol(classConstructorSymbol);
        return status;
    }
    classNode->kind = ABSTRACT_SYNTAX_CASE_CLASS;

    abstractSyntaxNodeSetInferredType(classNode, DATA_TYPE_UNKNOWN);

    return COMPILER_OK;
}

static CompilerStatus resolveBlock(AbstractSyntaxNode *blockNode, SemAnalData *data, TypeDescriptor *expected) {
    if (blockNode == NULL || data == NULL || data->functionSymbols == NULL || blockNode->kind != ABSTRACT_SYNTAX_BLOCK) {
        return COMPILER_INTERNAL_ERROR;
    }

    // enter local scope
    SymbolTable *currentTable = data->symbolTables[data->numberOfTables - 1];
    CompilerStatus status = symbolTableEnterScope(currentTable);
    if (status != COMPILER_OK) {
        return status;
    }

    // enter local scopes for classes and functions
    status = symbolTableEnterScope(data->algebdtSymbols);
    if (status != COMPILER_OK) {
        symbolTableLeaveScope(currentTable);
        return status;
    }

    status = symbolTableEnterScope(data->functionSymbols);
    if (status != COMPILER_OK) {
        symbolTableLeaveScope(data->algebdtSymbols);
        symbolTableLeaveScope(currentTable);
        return status;
    }

    for (size_t blockChildI = 0; blockChildI < blockNode->childCount; ++blockChildI) {
        AbstractSyntaxNode *child = blockNode->children[blockChildI];
        AbstractSyntaxNode *value = child;
        bool isListResult = expected != NULL && expected->recursiveTypes != NULL &&
                            blockChildI + 1 == blockNode->childCount;
        if (isListResult && child->kind == ABSTRACT_SYNTAX_EXPRESSION_STATEMENT && child->childCount == 1) {
            value = child->children[0];
        }
        if (isListResult && (isListConstructor(value) || isListConcatenation(value) || value->kind == ABSTRACT_SYNTAX_IF_EXPRESSION)) {
            status = resolveWithListContext(value, expected, data);
            if (status == COMPILER_OK && child != value) {
                status = abstractSyntaxNodeCopyInferredType(child, &value->type);
            }
        } else {
            status = semAnalResolveNode(child, data);
        }
        if (status != COMPILER_OK) {
            symbolTableLeaveScope(data->functionSymbols);
            symbolTableLeaveScope(data->algebdtSymbols);
            symbolTableLeaveScope(currentTable);
            return status;
        }
    }

    TypeDescriptor *evaluated = semAnalEvaluatesToType(blockNode);
    if (evaluated != NULL) {
        abstractSyntaxNodeCopyInferredType(blockNode, evaluated);
        semAnalDestroyTypeDescriptor(evaluated);
    } else {
        abstractSyntaxNodeSetInferredType(blockNode, DATA_TYPE_UNKNOWN);
    }

    symbolTableLeaveScope(data->functionSymbols);
    symbolTableLeaveScope(data->algebdtSymbols);
    symbolTableLeaveScope(currentTable);

    return COMPILER_OK;
}

CompilerStatus semAnalResolveBlock(AbstractSyntaxNode *blockNode, SemAnalData *data) {
    return resolveBlock(blockNode, data, NULL);
}

TypeDescriptor *semAnalEvaluatesToType(AbstractSyntaxNode *node)
{
    if (node->kind == ABSTRACT_SYNTAX_BLOCK) {
        if (node->childCount == 0) {
            return semAnalInitTypeDescriptor(DATA_TYPE_UNIT);
        } else {
            node = node->children[node->childCount - 1];
        }
    }

    if (node->kind == ABSTRACT_SYNTAX_EXPRESSION_STATEMENT || node->kind == ABSTRACT_SYNTAX_CALL_EXPRESSION
        || node->kind == ABSTRACT_SYNTAX_RETURN || node->kind == ABSTRACT_SYNTAX_IF || node->kind == ABSTRACT_SYNTAX_IF_EXPRESSION
        || node->kind == ABSTRACT_SYNTAX_MATCH_EXPRESSION || node->kind == ABSTRACT_SYNTAX_MEMBER_ACCESS
        || ((node->type.datatypeBitflag & DATA_TYPE_LIST) && node->kind != ABSTRACT_SYNTAX_VARIABLE_DECLARATION
            && node->kind != ABSTRACT_SYNTAX_ASSIGNMENT)) {
        return semAnalCopyTypeDescriptor(&node->type);
    }

    return semAnalInitTypeDescriptor(DATA_TYPE_UNIT);
}

CompilerStatus semAnalResolveIf(AbstractSyntaxNode *ifNode, SemAnalData *data) {
    if (ifNode == NULL || data == NULL || data->functionSymbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    CompilerStatus status = COMPILER_OK;

    // resolve condition
    AbstractSyntaxNode *condition = ifNode->children[0];
    status = semAnalResolveNode(condition, data);
    if (status != COMPILER_OK) {
        return status;
    }

    // if needs bool as condition
    TypeDescriptor *condType = &condition->type;
    if (condType->datatypeBitflag != DATA_TYPE_BOOLEAN) {
        return COMPILER_TYPE_ERROR;
    }

    // resolve then block
    status = semAnalResolveNode(ifNode->children[1], data);
    if (status != COMPILER_OK) {
        return status;
    }

    // resolve else blocks (if they exist)
    for (size_t elseI = 2; elseI < ifNode->childCount; ++elseI) {
        status = semAnalResolveNode(ifNode->children[elseI], data);
        if (status != COMPILER_OK) {
            return status;
        }
    }

    TypeDescriptor *allBranchesType = NULL;
    for (size_t branchI = 1; branchI < ifNode->childCount; ++branchI) {
        AbstractSyntaxNode *branch = ifNode->children[branchI];
        TypeDescriptor *branchType = ifNode->kind == ABSTRACT_SYNTAX_IF_EXPRESSION ?
                                     semAnalCopyTypeDescriptor(&branch->type) : semAnalEvaluatesToType(branch);
        if (branchType == NULL || branchType->datatypeBitflag == DATA_TYPE_UNKNOWN) {
            semAnalDestroyTypeDescriptor(allBranchesType);
            semAnalDestroyTypeDescriptor(branchType);
            allBranchesType = semAnalInitTypeDescriptor(DATA_TYPE_UNKNOWN); // if one branch evaluates to unknown then also the if statement
            break;
        }

        TypeDescriptor *merged = semAnalUnifyTypes(allBranchesType, branchType);
        semAnalDestroyTypeDescriptor(allBranchesType);
        semAnalDestroyTypeDescriptor(branchType);
        allBranchesType = merged;
    }

    // not a expression evaluating as non Unit type
    if (ifNode->kind == ABSTRACT_SYNTAX_IF && allBranchesType != NULL &&
        allBranchesType->datatypeBitflag != DATA_TYPE_UNIT && !semAnalIsIfExpression(ifNode)) {
        semAnalDestroyTypeDescriptor(allBranchesType);
        allBranchesType = semAnalInitTypeDescriptor(DATA_TYPE_UNKNOWN);
    }

    if (allBranchesType != NULL) {
        abstractSyntaxNodeCopyInferredType(ifNode, allBranchesType);
        semAnalDestroyTypeDescriptor(allBranchesType);
    } else {
        abstractSyntaxNodeSetInferredType(ifNode, DATA_TYPE_UNKNOWN);
    }

    return COMPILER_OK;
}


CompilerStatus semAnalResolveForGenerator(AbstractSyntaxNode *clause, SemAnalData *data)
{
    SymbolTable *currentTable = data->symbolTables[data->numberOfTables - 1];
    AbstractSyntaxNode *source = clause->children[0];
    bool isRange = source->kind == ABSTRACT_SYNTAX_RANGE_EXPRESSION;
    CompilerStatus status = isRange ? resolveRange(source, data) : semAnalResolveNode(source, data);
    if (status != COMPILER_OK) {
        return status;
    }

    TypeDescriptor *generatorType;
    if (isRange) {
        generatorType = semAnalInitTypeDescriptor(DATA_TYPE_INT);
    } else {
        TypeDescriptor *sourceType = &source->type;
        if (sourceType->datatypeBitflag != DATA_TYPE_LIST || sourceType->recursiveTypes == NULL ||
            sourceType->recursiveTypes->next != NULL) {
            return COMPILER_TYPE_ERROR;
        }
        generatorType = semAnalCopyTypeDescriptor(&sourceType->recursiveTypes->elementType);
    }
    if (generatorType == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    // declare the generator variable
    if (symbolTableFindCurrentScope(currentTable, clause->text) != NULL) {
        semAnalDestroyTypeDescriptor(generatorType);
        return COMPILER_REDEFINITION_ERROR; // variable name already exists
    }

    // generator symbol creation
    Symbol *generatorSymbol = semAnalCreateSymbolDefault();
    if (generatorSymbol == NULL) {
        semAnalDestroyTypeDescriptor(generatorType);
        return COMPILER_INTERNAL_ERROR;
    }

    // setup the symbol
    generatorSymbol->kind = SYMBOL_VARIABLE;
    generatorSymbol->name = clause->text;
    generatorSymbol->dataType = generatorType;
    generatorSymbol->variableId = ++data->variableIdCounter;

    clause->variableId = generatorSymbol->variableId;
    status = abstractSyntaxNodeCopyInferredType(clause, generatorType);
    if (status != COMPILER_OK) {
        semAnalFreeSymbol(generatorSymbol);
        return status;
    }

    status = symbolTableInsert(currentTable, generatorSymbol);
    if (status != COMPILER_OK) {
        semAnalFreeSymbol(generatorSymbol);
        return status;
    }

    return COMPILER_OK;
}

CompilerStatus semAnalResolveForGuard(AbstractSyntaxNode *clause, SemAnalData *data)
{
    AbstractSyntaxNode *condition = clause->children[0];
    CompilerStatus status = semAnalResolveNode(condition, data);
    if (status != COMPILER_OK) {
        return status;
    }

    TypeDescriptor *condType = &condition->type;
    if (condType->datatypeBitflag != DATA_TYPE_BOOLEAN) {
        return COMPILER_TYPE_ERROR;
    }

    return COMPILER_OK;
}

CompilerStatus semAnalResolveFor(AbstractSyntaxNode *forNode, SemAnalData *data) {
    if (forNode == NULL || data == NULL || data->functionSymbols == NULL || forNode->childCount < 2) {
        return COMPILER_INTERNAL_ERROR;
    }

    CompilerStatus status = COMPILER_OK;

    AbstractSyntaxNode *clauses = forNode->children[0];
    AbstractSyntaxNode *body = forNode->children[1];

    // enter new scope for the loop body and guards
    SymbolTable *currentTable = data->symbolTables[data->numberOfTables - 1];
    status = symbolTableEnterScope(currentTable);
    if (status != COMPILER_OK) {
        return status;
    }

    // enter local scopes for classes and functions
    status = symbolTableEnterScope(data->algebdtSymbols);
    if (status != COMPILER_OK) {
        symbolTableLeaveScope(currentTable);
        return status;
    }

    status = symbolTableEnterScope(data->functionSymbols);
    if (status != COMPILER_OK) {
        symbolTableLeaveScope(data->algebdtSymbols);
        symbolTableLeaveScope(currentTable);
        return status;
    }

    for (size_t clauseI = 0; clauseI < clauses->childCount; ++clauseI) {
        AbstractSyntaxNode *clause = clauses->children[clauseI];

        if (clause->kind == ABSTRACT_SYNTAX_FOR_GENERATOR) {
            status = semAnalResolveForGenerator(clause, data);
            if (status != COMPILER_OK) {
                symbolTableLeaveScope(data->functionSymbols);
                symbolTableLeaveScope(data->algebdtSymbols);
                symbolTableLeaveScope(currentTable);
                return status;
            }
        } else if (clause->kind == ABSTRACT_SYNTAX_FOR_GUARD) {
            status = semAnalResolveForGuard(clause, data);
            if (status != COMPILER_OK) {
                symbolTableLeaveScope(data->functionSymbols);
                symbolTableLeaveScope(data->algebdtSymbols);
                symbolTableLeaveScope(currentTable);
                return status;
            }
        }
    }

    // resolve the loop body
    status = semAnalResolveNode(body, data);
    if (status != COMPILER_OK) {
        symbolTableLeaveScope(data->functionSymbols);
        symbolTableLeaveScope(data->algebdtSymbols);
        symbolTableLeaveScope(currentTable);
        return status;
    }

    // clean up scope and set final type
    symbolTableLeaveScope(data->functionSymbols);
    symbolTableLeaveScope(data->algebdtSymbols);
    symbolTableLeaveScope(currentTable);

    abstractSyntaxNodeSetInferredType(forNode, DATA_TYPE_UNIT);

    return COMPILER_OK;
}

CompilerStatus semAnalResolveWhile(AbstractSyntaxNode *whileNode, SemAnalData *data) {
    if (whileNode == NULL || data == NULL || data->functionSymbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    CompilerStatus status = COMPILER_OK;

    // resolve condition
    AbstractSyntaxNode *condition = whileNode->children[0];
    status = semAnalResolveNode(condition, data);
    if (status != COMPILER_OK) {
        return status;
    }

    // while needs bool as condition
    TypeDescriptor *condType = &condition->type;
    if (condType->datatypeBitflag != DATA_TYPE_BOOLEAN) {
        return COMPILER_TYPE_ERROR;
    }

    // resolve body block
    status = semAnalResolveNode(whileNode->children[1], data);
    if (status != COMPILER_OK) {
        return status;
    }

    abstractSyntaxNodeSetInferredType(whileNode, DATA_TYPE_UNIT);

    return COMPILER_OK;
}

CompilerStatus semAnalResolveReturn(AbstractSyntaxNode *returnNode, SemAnalData *data) {
    if (returnNode == NULL || data == NULL || data->functionSymbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    Symbol *functionSymbol = symbolTableFind(
        data->functionSymbols, data->recursionList[data->numberOfTables - 1]);
    if (functionSymbol == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    TypeDescriptor *funcRetType = functionSymbol->function.returnType;
    CompilerStatus status;
    if (returnNode->childCount > 0 && returnNode->children[0] != NULL) {
        AbstractSyntaxNode *value = returnNode->children[0];
        status = resolveWithListContext(value, funcRetType, data);
        if (status == COMPILER_OK) {
            status = abstractSyntaxNodeCopyInferredType(returnNode, &value->type);
        }
    } else {
        status = abstractSyntaxNodeSetInferredType(returnNode, DATA_TYPE_UNIT);
    }
    if (status != COMPILER_OK) {
        return status;
    }
    if (funcRetType == NULL || funcRetType->datatypeBitflag == DATA_TYPE_UNKNOWN || (funcRetType->datatypeBitflag != DATA_TYPE_UNIT && !semAnalIsSubtype(funcRetType, &returnNode->type))) {
        return COMPILER_TYPE_ERROR;
    }

    return COMPILER_OK;
}


CompilerStatus semAnalResolveCaseClassPattern(AbstractSyntaxNode *patternNode, SemAnalData *data) {
    SymbolTable *currentTable = data->symbolTables[data->numberOfTables - 1];

    AbstractSyntaxNode *identifier = patternNode->children[0];
    if (identifier == NULL || identifier->kind != ABSTRACT_SYNTAX_IDENTIFIER) {
        return COMPILER_TYPE_ERROR;
    }

    Symbol *caseClassSymbol = symbolTableFind(data->functionSymbols, identifier->text);
    if (caseClassSymbol == NULL) {
        return COMPILER_UNDEFINED_SYMBOL_ERROR;
    }

    TypeDescriptor *classRetType = caseClassSymbol->function.returnType;
    if (classRetType == NULL || !(classRetType->datatypeBitflag & DATA_TYPE_CASE_CLASS)) {
        return COMPILER_TYPE_ERROR; // pattern is a normal function not a case class
    }

    if (patternNode->childCount - 1 != caseClassSymbol->function.parameterCount) {
        return COMPILER_ARGUMENT_ERROR; // wrong number of arguments
    }

    for (size_t paramI = 0; paramI < patternNode->childCount - 1; ++paramI) {
        AbstractSyntaxNode *argNode = patternNode->children[paramI + 1];

        if (argNode == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        TypeDescriptor *extractedType = caseClassSymbol->function.parameterTypes[paramI];

        CompilerStatus status = semAnalNormalizeTypeDescriptor(extractedType, data);
        if (status != COMPILER_OK) {
            return status;
        }

        // class patterns bind all members via identifiers exclusively
        if (argNode->kind != ABSTRACT_SYNTAX_IDENTIFIER) {
            return COMPILER_TYPE_ERROR;
        }

        // ignore the wildcard '_' from symbol table insertion, just set its type
        if (argNode->text != NULL && strcmp(argNode->text, SYNTAX_PATTERN_WILDCARD) == 0) {
            abstractSyntaxNodeCopyInferredType(argNode, extractedType);
            continue;
        }

        // standard variable binding in pattern
        Symbol *existingSymbol = symbolTableFindCurrentScope(currentTable, argNode->text);
        if (existingSymbol != NULL) {
            return COMPILER_REDEFINITION_ERROR;
        }

        // temporarily fake the kind so semAnalAstNodeToSymbol maps it to SYMBOL_VARIABLE
        AbstractSyntaxNodeKind originalKind = argNode->kind;
        argNode->kind = ABSTRACT_SYNTAX_VARIABLE_DECLARATION;

        abstractSyntaxNodeCopyInferredType(argNode, extractedType);
        argNode->isMutable = false;

        Symbol *newSymbol = semAnalAstNodeToSymbol(argNode);

        // restore the original kind
        argNode->kind = originalKind;

        if (newSymbol == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        newSymbol->defined = true;
        newSymbol->variableId = ++data->variableIdCounter;
        argNode->variableId = newSymbol->variableId;

        status = symbolTableInsert(currentTable, newSymbol);
        if (status != COMPILER_OK) {
            semAnalFreeSymbol(newSymbol);
            return status;
        }
    }

    return COMPILER_OK;
}

CompilerStatus semAnalResolveMatchPattern(AbstractSyntaxNode *patternNode, AbstractSyntaxNode *scrutinee, SemAnalData *data) {
    SymbolTable *currentTable = data->symbolTables[data->numberOfTables - 1];

    if (patternNode->kind == ABSTRACT_SYNTAX_IDENTIFIER) {
        // wildcard '_' matches everything, no bindings needed
        if (patternNode->text != NULL && strcmp(patternNode->text, SYNTAX_PATTERN_WILDCARD) == 0) {
            return abstractSyntaxNodeCopyInferredType(patternNode, &scrutinee->type);
        }

        if (patternNode->type.datatypeBitflag != DATA_TYPE_UNKNOWN) {
            CompilerStatus status = semAnalNormalizeTypeDescriptor(&patternNode->type, data);
            if (status != COMPILER_OK) {
                return status;
            }

            TypeDescriptor *scrutineeType = &scrutinee->type;
            TypeDescriptor *patternType = &patternNode->type;

            // One runtime tag represents all lists. A list pattern must cover every possible list element type.
            if ((scrutineeType->datatypeBitflag & DATA_TYPE_LIST) && (patternType->datatypeBitflag & DATA_TYPE_LIST)) {
                TypeDescriptor lists = {.datatypeBitflag = DATA_TYPE_LIST, .recursiveTypes = scrutineeType->recursiveTypes};
                if (!semAnalIsSubtype(patternType, &lists)) {
                    return COMPILER_TYPE_ERROR;
                }
            }

            // check overlap with the scrutinee
            if ((scrutineeType->datatypeBitflag & patternType->datatypeBitflag) == 0) {
                // allow Int to match a scrutinee that expects Double
                if (!((scrutineeType->datatypeBitflag & DATA_TYPE_DOUBLE) && patternType->datatypeBitflag == DATA_TYPE_INT)) {
                    return COMPILER_TYPE_ERROR;
                }
            }

            // custom insertion for pattern variables (no initExpr to evaluate)
            Symbol *existingSymbol = symbolTableFindCurrentScope(currentTable, patternNode->text);
            if (existingSymbol != NULL) {
                return COMPILER_REDEFINITION_ERROR;
            }

            AbstractSyntaxNodeKind originalKind = patternNode->kind;
            patternNode->kind = ABSTRACT_SYNTAX_VARIABLE_DECLARATION;
            Symbol *newSymbol = semAnalAstNodeToSymbol(patternNode);
            patternNode->kind = originalKind;

            if (newSymbol == NULL) {
                return COMPILER_INTERNAL_ERROR;
            }

            newSymbol->defined = true;
            newSymbol->variableId = ++data->variableIdCounter;
            patternNode->variableId = newSymbol->variableId;

            status = symbolTableInsert(currentTable, newSymbol);
            if (status != COMPILER_OK) {
                semAnalFreeSymbol(newSymbol);
                return status;
            }
            return COMPILER_OK;
        }
        else {
            return COMPILER_TYPE_ERROR;
        }

    } else if (patternNode->kind == ABSTRACT_SYNTAX_CALL_EXPRESSION) {
        return semAnalResolveCaseClassPattern(patternNode, data);
    } else if (patternNode->kind == ABSTRACT_SYNTAX_INT_LITERAL ||
               patternNode->kind == ABSTRACT_SYNTAX_DOUBLE_LITERAL ||
               patternNode->kind == ABSTRACT_SYNTAX_STRING_LITERAL ||
               patternNode->kind == ABSTRACT_SYNTAX_BOOLEAN_LITERAL ||
               patternNode->kind == ABSTRACT_SYNTAX_UNIT_LITERAL ||
               patternNode->kind == ABSTRACT_SYNTAX_UNARY_EXPRESSION) {

        // literal exact matching
        CompilerStatus status = semAnalResolveNode(patternNode, data);
        if (status != COMPILER_OK) {
            return status;
        }

        // literal must overlap with the scrutinee type
        TypeDescriptor *scrutineeType = &scrutinee->type;
        TypeDescriptor *patternType = &patternNode->type;
        if ((scrutineeType->datatypeBitflag & patternType->datatypeBitflag) == 0) {
            // allow Int to match a scrutinee that expects Double
            if (!((scrutineeType->datatypeBitflag & DATA_TYPE_DOUBLE) && patternType->datatypeBitflag == DATA_TYPE_INT)) {
                return COMPILER_TYPE_ERROR;
            }
        }
        return COMPILER_OK;
    }

    // unsupported pattern type
    return COMPILER_TYPE_ERROR;
}

static CompilerStatus semAnalResolveMatchBranch(AbstractSyntaxNode *branchNode, AbstractSyntaxNode *scrutinee, SemAnalData *data) {
    if (branchNode == NULL || branchNode->children == NULL || (branchNode->childCount != 2 && branchNode->childCount != 3) ||
        branchNode->children[0] == NULL || branchNode->children[branchNode->childCount - 1] == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    CompilerStatus status = semAnalResolveMatchPattern(branchNode->children[0], scrutinee, data);
    if (status != COMPILER_OK) {
        return status;
    }

    if (branchNode->childCount == 3) {
        AbstractSyntaxNode *guard = branchNode->children[1];
        if (guard == NULL || guard->kind != ABSTRACT_SYNTAX_MATCH_GUARD || guard->childCount != 1 ||
            guard->children == NULL || guard->children[0] == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }
        status = semAnalResolveNode(guard->children[0], data);
        if (status != COMPILER_OK) {
            return status;
        }
        if (guard->children[0]->type.datatypeBitflag != DATA_TYPE_BOOLEAN) {
            return COMPILER_TYPE_ERROR;
        }
    }

    return semAnalResolveNode(branchNode->children[branchNode->childCount - 1], data);
}

CompilerStatus semAnalResolveMatchExpression(AbstractSyntaxNode *matchNode, SemAnalData *data) {
    if (matchNode == NULL || data == NULL || data->functionSymbols == NULL || matchNode->children == NULL || matchNode->childCount < 2) {
        return COMPILER_INTERNAL_ERROR;
    }

    AbstractSyntaxNode *scrutinee = matchNode->children[0];
    CompilerStatus status = semAnalResolveNode(scrutinee, data);
    if (status != COMPILER_OK) {
        return status;
    }

    SymbolTable *currentTable = data->symbolTables[data->numberOfTables - 1];
    TypeDescriptor *resultType = NULL;
    for (size_t branchI = 1; branchI < matchNode->childCount; ++branchI) {
        status = symbolTableEnterScope(currentTable);
        if (status != COMPILER_OK) {
            return status;
        }
        status = symbolTableEnterScope(data->algebdtSymbols);
        if (status != COMPILER_OK) {
            symbolTableLeaveScope(currentTable);
            return status;
        }
        status = symbolTableEnterScope(data->functionSymbols);
        if (status != COMPILER_OK) {
            symbolTableLeaveScope(data->algebdtSymbols);
            symbolTableLeaveScope(currentTable);
            return status;
        }

        AbstractSyntaxNode *branch = matchNode->children[branchI];
        status = semAnalResolveMatchBranch(branch, scrutinee, data);
        symbolTableLeaveScope(data->functionSymbols);
        symbolTableLeaveScope(data->algebdtSymbols);
        symbolTableLeaveScope(currentTable);
        if (status != COMPILER_OK) {
            return status;
        }

        TypeDescriptor *branchType = &branch->children[branch->childCount - 1]->type;
        if (branchType->datatypeBitflag == DATA_TYPE_UNKNOWN) {
            return COMPILER_TYPE_ERROR;
        }
        if (resultType == NULL || semAnalIsSubtype(branchType, resultType)) {
            resultType = branchType;
        }
    }

    // An existing union can cover narrower branches. Do not infer a new union here.
    for (size_t branchI = 1; branchI < matchNode->childCount; ++branchI) {
        AbstractSyntaxNode *branch = matchNode->children[branchI];
        if (!semAnalIsSubtype(resultType, &branch->children[branch->childCount - 1]->type)) {
            return COMPILER_TYPE_ERROR;
        }
    }

    AbstractSyntaxNode *lastBranch = matchNode->children[matchNode->childCount - 1];
    AbstractSyntaxNode *pattern = lastBranch->children[0];
    if (lastBranch->childCount != 2 || pattern->kind != ABSTRACT_SYNTAX_IDENTIFIER ||
        pattern->text == NULL || strcmp(pattern->text, SYNTAX_PATTERN_WILDCARD) != 0) {
        return COMPILER_SEMANTIC_ERROR;
    }

    return abstractSyntaxNodeCopyInferredType(matchNode, resultType);
}


int semAnalInRecursion(const char* funName, SemAnalData *data)
{
    if (funName == NULL || data == NULL || data->recursionList == NULL) {
        return -2; // internal error
    }

    size_t recursionI;
    for (recursionI = 0; recursionI < data->numberOfTables && strcmp(data->recursionList[recursionI], funName) != 0; ++recursionI) {}
    if (recursionI == data->numberOfTables) {
        return -1; // not a reccurent function
    } else {
        return recursionI; // recursion function index
    }
}

CompilerStatus semAnalCheckRecursion(size_t recursionI, SemAnalData *data) {
    while (recursionI < data->numberOfTables) {
        Symbol *funSymbolTmp =
            symbolTableFind(data->functionSymbols, data->recursionList[recursionI]);
        if (funSymbolTmp == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        // function inside recursion does not have explicit return type
        TypeDescriptor *funcRetType = funSymbolTmp->function.returnType;
        if (funcRetType == NULL || funcRetType->datatypeBitflag == DATA_TYPE_UNKNOWN) {
            return COMPILER_TYPE_ERROR;
        }

        ++recursionI;
    }

    return COMPILER_OK;
}


static CompilerStatus resolveListAccess(AbstractSyntaxNode *call, SemAnalData *data) {
    if (call->childCount != 2) {
        return COMPILER_ARGUMENT_ERROR;
    }
    AbstractSyntaxNode *index = call->children[1];
    CompilerStatus status = semAnalResolveNode(index, data);
    if (status != COMPILER_OK) {
        return status;
    }
    TypeDescriptor *receiver = &call->children[0]->type;
    if (index->type.datatypeBitflag != DATA_TYPE_INT || receiver->recursiveTypes == NULL ||
        receiver->recursiveTypes->next != NULL) {
        return COMPILER_TYPE_ERROR;
    }
    return abstractSyntaxNodeCopyInferredType(call, &receiver->recursiveTypes->elementType);
}

CompilerStatus semAnalResolveListConstructor(AbstractSyntaxNode *call, SemAnalData *data) {
    TypeDescriptor *element = NULL;
    for (size_t counter = 1; counter < call->childCount; counter++) {
        AbstractSyntaxNode *argument = call->children[counter];
        CompilerStatus status = semAnalResolveNode(argument, data);
        if (status != COMPILER_OK) {
            return status;
        }
        if (!isUntypedList(&argument->type) &&
            (element == NULL || semAnalIsSubtype(&argument->type, element))) {
            element = &argument->type;
        }
    }
    if (call->childCount == 1) {
        return abstractSyntaxNodeSetInferredType(call, DATA_TYPE_LIST);
    }
    if (element == NULL) {
        return COMPILER_TYPE_ERROR;
    }
    for (size_t counter = 1; counter < call->childCount; counter++) {
        AbstractSyntaxNode *argument = call->children[counter];
        CompilerStatus status = applyListContext(argument, element);
        if (status != COMPILER_OK) {
            return status;
        }
        if (!semAnalIsSubtype(element, &argument->type)) {
            return COMPILER_TYPE_ERROR;
        }
    }
    return setListType(call, element);
}

CompilerStatus semAnalResolveStandardCall(AbstractSyntaxNode *functionCallNode, SemAnalData *data, AbstractSyntaxNode *callee) {
    Symbol *funcSymbol = symbolTableFind(data->functionSymbols, callee->text);
    if (funcSymbol == NULL) {
        return COMPILER_UNDEFINED_SYMBOL_ERROR;
    }

    if (functionCallNode->childCount - 1 != funcSymbol->function.parameterCount) {
        return COMPILER_ARGUMENT_ERROR;
    }

    CompilerStatus typeStatus = semAnalNormalizeTypeDescriptor(funcSymbol->function.returnType, data);
    if (typeStatus != COMPILER_OK) {
        return typeStatus;
    }

    // check the arguments
    TypeDescriptor **paramTypes = funcSymbol->function.parameterTypes;
    for (size_t paramI = 1; paramI < functionCallNode->childCount; ++paramI) {
        AbstractSyntaxNode *argNode = functionCallNode->children[paramI];

        TypeDescriptor *expectedType = paramTypes[paramI - 1];
        CompilerStatus status = semAnalNormalizeTypeDescriptor(expectedType, data);
        if (status != COMPILER_OK) {
            return status;
        }
        status = resolveWithListContext(argNode, expectedType, data);
        if (status != COMPILER_OK) {
            return status;
        }
        TypeDescriptor *argType = &argNode->type;
        if (expectedType != NULL && expectedType->datatypeBitflag != DATA_TYPE_UNKNOWN && !semAnalIsSubtype(expectedType, argType)) {
            if ((expectedType->datatypeBitflag & DATA_TYPE_DOUBLE) && argType->datatypeBitflag == DATA_TYPE_INT) {
                // implicit conversion later
                abstractSyntaxNodeSetInferredType(argNode, DATA_TYPE_INT);
            } else {
                return COMPILER_ARGUMENT_ERROR;
            }
        }
    }

    // defining a function if not defined, checking explicit returns if reccurent
    if (!funcSymbol->defined) {
        int recursionResult = semAnalInRecursion(callee->text, data);
        if (recursionResult < -1) {
            return COMPILER_INTERNAL_ERROR;
        }

        if (recursionResult == -1) {
            size_t funI;
            for (funI = 0; strcmp(data->root->children[funI]->text, callee->text) != 0; ++funI) {}
            CompilerStatus status = semAnalDefineFunction(data->root->children[funI], data);
            if (status != COMPILER_OK) {
                return status;
            }
        } else {
            CompilerStatus status = semAnalCheckRecursion(recursionResult, data);
            if (status != COMPILER_OK) {
                return status;
            }
        }
    }

    return abstractSyntaxNodeCopyInferredType(functionCallNode, funcSymbol->function.returnType);
}

CompilerStatus semAnalResolveListMethodCall(AbstractSyntaxNode *callNode, SemAnalData *data, AbstractSyntaxNode *receiver, const char *methodName) {
    CompilerStatus status = semAnalResolveNode(receiver, data);
    if (status != COMPILER_OK) {
        return status;
    }

    const TypeDescriptor *type = &receiver->type;
    if (type->datatypeBitflag != DATA_TYPE_LIST || type->recursiveTypes == NULL) {
        return COMPILER_TYPE_ERROR;
    }

    size_t argumentCount = strcmp(methodName, "slice") == 0 ? 2 : 1;
    if (callNode->childCount != argumentCount + 1) {
        return COMPILER_ARGUMENT_ERROR;
    }
    for (size_t counter = 1; counter <= argumentCount; counter++) {
        AbstractSyntaxNode *argument = callNode->children[counter];
        status = semAnalResolveNode(argument, data);
        if (status != COMPILER_OK) {
            return status;
        }
        if (argument->type.datatypeBitflag != DATA_TYPE_INT) {
            return COMPILER_TYPE_ERROR;
        }
    }
    return abstractSyntaxNodeCopyInferredType(callNode, type);
}

CompilerStatus semAnalResolveFunctionCall(AbstractSyntaxNode *functionCallNode, SemAnalData *data) {
    if (functionCallNode == NULL || data == NULL || data->functionSymbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    AbstractSyntaxNode *callee = functionCallNode->children[0];

    // resolve member access like myList.drop(5) or List.range(1, 10)
    if (callee->kind == ABSTRACT_SYNTAX_MEMBER_ACCESS) {
        AbstractSyntaxNode *receiver = callee->children[0];
        const char *methodName = callee->text;

        // constructor: List.range(...)
        if (receiver->kind == ABSTRACT_SYNTAX_IDENTIFIER && strcmp(receiver->text, "List") == 0) {
            if (strcmp(methodName, "range") == 0) {
                if (functionCallNode->childCount != 3 && functionCallNode->childCount != 4) {
                    return COMPILER_ARGUMENT_ERROR;
                }
                for (size_t i = 1; i < functionCallNode->childCount; i++) {
                    CompilerStatus status = semAnalResolveNode(functionCallNode->children[i], data);
                    if (status != COMPILER_OK) {
                        return status;
                    }
                    TypeDescriptor *argType = &functionCallNode->children[i]->type;
                    if (argType == NULL || argType->datatypeBitflag != DATA_TYPE_INT) {
                        return COMPILER_TYPE_ERROR;
                    }
                }
                TypeDescriptor element = {.datatypeBitflag = DATA_TYPE_INT};
                return setListType(functionCallNode, &element);
            }
            return COMPILER_UNDEFINED_SYMBOL_ERROR;
        }

        // instance methods: myList.drop(5)
        if (strcmp(methodName, "drop") == 0 || strcmp(methodName, "take") == 0 || strcmp(methodName, "slice") == 0) {
            return semAnalResolveListMethodCall(functionCallNode, data, receiver, methodName);
        }
    }

    if (callee->kind == ABSTRACT_SYNTAX_IDENTIFIER) {
        if (strcmp(callee->text, "List") == 0) {
            return semAnalResolveListConstructor(functionCallNode, data);
        }
        SymbolTable *currentTable = data->symbolTables[data->numberOfTables - 1];
        if (symbolTableFind(currentTable, callee->text) == NULL) {
            return semAnalResolveStandardCall(functionCallNode, data, callee);
        }
    }
    CompilerStatus status = semAnalResolveNode(callee, data);
    if (status != COMPILER_OK) {
        return status;
    }
    if (callee->type.datatypeBitflag == DATA_TYPE_LIST) {
        return resolveListAccess(functionCallNode, data);
    }
    return COMPILER_TYPE_ERROR;

}

CompilerStatus semAnalResolveMemberAccess(AbstractSyntaxNode *accessNode, SemAnalData *data) {
    if (accessNode == NULL || data == NULL || accessNode->childCount != 1) {
        return COMPILER_INTERNAL_ERROR;
    }

    AbstractSyntaxNode *objectNode = accessNode->children[0];
    const char *memberName = accessNode->text;
    if (objectNode->kind == ABSTRACT_SYNTAX_RANGE_EXPRESSION && strcmp(memberName, "toList") == 0) {
        CompilerStatus status = resolveRange(objectNode, data);
        if (status != COMPILER_OK) {
            return status;
        }
        TypeDescriptor element = {.datatypeBitflag = DATA_TYPE_INT};
        return setListType(accessNode, &element);
    }

    CompilerStatus status = semAnalResolveNode(objectNode, data);
    if (status != COMPILER_OK) {
        return status;
    }

    TypeDescriptor *objType = &objectNode->type;

    if (objType->datatypeBitflag == DATA_TYPE_LIST) {
        if (objType->recursiveTypes == NULL || objType->recursiveTypes->next != NULL) {
            return COMPILER_TYPE_ERROR;
        }
        if (strcmp(memberName, "head") == 0) {
            return abstractSyntaxNodeCopyInferredType(accessNode, &objType->recursiveTypes->elementType);
        }
        if (strcmp(memberName, "tail") == 0) {
            return abstractSyntaxNodeCopyInferredType(accessNode, objType);
        }
        if (strcmp(memberName, "toString") == 0) {
            return abstractSyntaxNodeSetInferredType(accessNode, DATA_TYPE_STRING);
        }
        return COMPILER_TYPE_ERROR;
    }

    // dynamic class lookup
    if (objType != NULL && (objType->datatypeBitflag & DATA_TYPE_CASE_CLASS)) {
        TypeDescriptor *unifiedMemberType = NULL;

        // check all possible classes in possible union type
        for (TypeDescriptorAliasNode *classNode = objType->classes; classNode != NULL; classNode = classNode->next) {
            Symbol *classSymbol = symbolTableFind(data->algebdtSymbols, classNode->name);
            if (classSymbol == NULL) {
                semAnalDestroyTypeDescriptor(unifiedMemberType);
                return COMPILER_TYPE_ERROR;
            }

            // search for the member
            bool memberFound = false;
            TypeDescriptor *memberType = NULL;
            for (size_t paramI = 0; paramI < classSymbol->function.parameterCount; ++paramI) {
                if (strcmp(classSymbol->function.parameterNames[paramI], memberName) == 0) {
                    memberFound = true;
                    memberType = classSymbol->function.parameterTypes[paramI];
                    break;
                }
            }

            if (!memberFound) {
                semAnalDestroyTypeDescriptor(unifiedMemberType);
                return COMPILER_TYPE_ERROR;
            }

            // unify the found type with the types from other classes in the union
            TypeDescriptor *merged = semAnalUnifyTypes(unifiedMemberType, memberType);
            semAnalDestroyTypeDescriptor(unifiedMemberType);
            unifiedMemberType = merged;
        }

        if (unifiedMemberType != NULL) {
            abstractSyntaxNodeCopyInferredType(accessNode, unifiedMemberType);
            semAnalDestroyTypeDescriptor(unifiedMemberType);
        }

        return COMPILER_OK;
    }

    return COMPILER_TYPE_ERROR;
}

CompilerStatus semAnalResolveVariableDeclaration(AbstractSyntaxNode *variableDeclarationNode,
                                                 SemAnalData *data) {
    if (variableDeclarationNode == NULL || data == NULL || data->functionSymbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    SymbolTable *currentTable = data->symbolTables[data->numberOfTables - 1];
    AbstractSyntaxNode *initExpr = variableDeclarationNode->children[0];

    // redefinition check
    Symbol *varSymbol = symbolTableFindCurrentScope(currentTable, variableDeclarationNode->text);
    if (varSymbol != NULL) {
        return COMPILER_REDEFINITION_ERROR;
    }

    TypeDescriptor *initType = &initExpr->type;
    TypeDescriptor *declType = &variableDeclarationNode->type;
    CompilerStatus status = semAnalNormalizeTypeDescriptor(declType, data);
    if (status != COMPILER_OK) {
        return status;
    }
    status = resolveWithListContext(initExpr, declType, data);
    if (status != COMPILER_OK) {
        return status;
    }

    // check for wrong types
    if (declType != NULL && declType->datatypeBitflag != DATA_TYPE_UNKNOWN && !semAnalIsSubtype(declType, initType)) {
        if ((declType->datatypeBitflag & DATA_TYPE_DOUBLE) && initType->datatypeBitflag == DATA_TYPE_INT) {
            abstractSyntaxNodeSetInferredType(initExpr, DATA_TYPE_INT); // implicit conversion later
        } else {
            return COMPILER_TYPE_ERROR;
        }
    }

    if (declType == NULL || declType->datatypeBitflag == DATA_TYPE_UNKNOWN) {
        abstractSyntaxNodeCopyInferredType(variableDeclarationNode, initType);

        if (variableDeclarationNode->type.datatypeBitflag == DATA_TYPE_UNKNOWN) {
            return COMPILER_TYPE_ERROR; //unable to get type
        }
    }

    // create symbol
    Symbol *variableSymbol = semAnalAstNodeToSymbol(variableDeclarationNode);
    if (variableSymbol == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    ++data->variableIdCounter;
    variableSymbol->variableId = data->variableIdCounter;

    status = symbolTableInsert(currentTable, variableSymbol);
    if (status != COMPILER_OK) {
        semAnalFreeSymbol(variableSymbol);
        return status;
    }

    variableDeclarationNode->variableId = variableSymbol->variableId;

    return COMPILER_OK;
}

CompilerStatus semAnalResolveAssignment(AbstractSyntaxNode *assignmentNode, SemAnalData *data) {
    if (assignmentNode == NULL || data == NULL || data->functionSymbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    // check variable declaration
    SymbolTable *currentTable = data->symbolTables[data->numberOfTables - 1];
    Symbol *varSymbol = symbolTableFind(currentTable, assignmentNode->text);
    if (varSymbol == NULL) {
        return COMPILER_UNDEFINED_SYMBOL_ERROR;
    }
    if (!varSymbol->mutable) {
        return COMPILER_IMMUTABLE_WRITE_ERROR;
    }

    AbstractSyntaxNode *assignExpr = assignmentNode->children[0];
    CompilerStatus status = resolveWithListContext(assignExpr, varSymbol->dataType, data);
    if (status != COMPILER_OK) {
        return status;
    }

    // check types
    if (!semAnalIsSubtype(varSymbol->dataType, &assignExpr->type)) {
        TypeDescriptor *assignType = &assignExpr->type;
        TypeDescriptor *varType = varSymbol->dataType;

        if ((varType->datatypeBitflag & DATA_TYPE_DOUBLE) && assignType->datatypeBitflag == DATA_TYPE_INT) {
            abstractSyntaxNodeSetInferredType(assignExpr, DATA_TYPE_INT); // implicit conversion later
        } else {
            return COMPILER_TYPE_ERROR;
        }
    }

    abstractSyntaxNodeCopyInferredType(assignmentNode, varSymbol->dataType);
    assignmentNode->variableId = varSymbol->variableId;

    return COMPILER_OK;
}

CompilerStatus semAnalResolveExpressionStatement(AbstractSyntaxNode *expressionStatementNode,
                                                 SemAnalData *data) {
    if (expressionStatementNode == NULL || data == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    CompilerStatus status = semAnalResolveNode(expressionStatementNode->children[0], data);
    if (status != COMPILER_OK) {
        return status;
    }

    abstractSyntaxNodeCopyInferredType(expressionStatementNode, &expressionStatementNode->children[0]->type);

    return COMPILER_OK;
}

CompilerStatus semAnalResolveExpressionBinary(AbstractSyntaxNode *expressionBinaryNode,
                                              SemAnalData *data) {
    if (expressionBinaryNode == NULL || data == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    AbstractSyntaxNode *left = expressionBinaryNode->children[0];
    AbstractSyntaxNode *right = expressionBinaryNode->children[1];

    CompilerStatus status = semAnalResolveNode(left, data);
    if (status != COMPILER_OK) {
        return status;
    }
    status = semAnalResolveNode(right, data);
    if (status != COMPILER_OK) {
        return status;
    }

    TypeDescriptor *leftType = &left->type;
    TypeDescriptor *rightType = &right->type;

    if (leftType->datatypeBitflag == DATA_TYPE_UNIT || rightType->datatypeBitflag == DATA_TYPE_UNIT) {
        return COMPILER_TYPE_ERROR;
    }

    // bool logical operands
    if (strcmp(expressionBinaryNode->text, "&&") == 0 || strcmp(expressionBinaryNode->text, "||") == 0) {
       if (leftType->datatypeBitflag == DATA_TYPE_BOOLEAN && rightType->datatypeBitflag == DATA_TYPE_BOOLEAN) {
            abstractSyntaxNodeSetInferredType(expressionBinaryNode, DATA_TYPE_BOOLEAN);
            return COMPILER_OK;
       }
       return COMPILER_TYPE_ERROR;
    }

    else if (strcmp(expressionBinaryNode->text, "++") == 0) {
        if (leftType->datatypeBitflag != DATA_TYPE_LIST || rightType->datatypeBitflag != DATA_TYPE_LIST) {
            return COMPILER_TYPE_ERROR;
        }
        status = applyListContext(left, rightType);
        if (status != COMPILER_OK) {
            return status;
        }
        status = applyListContext(right, leftType);
        if (status != COMPILER_OK) {
            return status;
        }
        if (semAnalIsSubtype(leftType, rightType)) {
            return abstractSyntaxNodeCopyInferredType(expressionBinaryNode, leftType);
        }
        if (semAnalIsSubtype(rightType, leftType)) {
            return abstractSyntaxNodeCopyInferredType(expressionBinaryNode, rightType);
        }
        return COMPILER_TYPE_ERROR;
    }

    // only numbers expression
    else if (expressionBinaryNode->text[0] == '-' || expressionBinaryNode->text[0] == '*' || expressionBinaryNode->text[0] == '/') {
        if (leftType->datatypeBitflag == DATA_TYPE_STRING || rightType->datatypeBitflag == DATA_TYPE_STRING) {
            return COMPILER_TYPE_ERROR;
        } else if (leftType->datatypeBitflag == DATA_TYPE_INT && rightType->datatypeBitflag == DATA_TYPE_INT) {
            abstractSyntaxNodeSetInferredType(expressionBinaryNode, DATA_TYPE_INT);
            return COMPILER_OK;
        } else {
            abstractSyntaxNodeSetInferredType(expressionBinaryNode, DATA_TYPE_DOUBLE);
            return COMPILER_OK;
        }
    }

    // numbers + concatenation
    else if (expressionBinaryNode->text[0] == '+') {
        if (leftType->datatypeBitflag == DATA_TYPE_BOOLEAN || rightType->datatypeBitflag == DATA_TYPE_BOOLEAN) {
            return COMPILER_TYPE_ERROR;
        } else if (leftType->datatypeBitflag == rightType->datatypeBitflag) {
            abstractSyntaxNodeSetInferredType(expressionBinaryNode, leftType->datatypeBitflag);
            return COMPILER_OK;
        } else if (leftType->datatypeBitflag == DATA_TYPE_STRING || rightType->datatypeBitflag == DATA_TYPE_STRING) {
            return COMPILER_TYPE_ERROR;
        } else {
            abstractSyntaxNodeSetInferredType(expressionBinaryNode, DATA_TYPE_DOUBLE);
            return COMPILER_OK;
        }
    }

    // comparison
    else {
        if (leftType->datatypeBitflag == rightType->datatypeBitflag) {
            // comparing the same
            if ((strcmp(expressionBinaryNode->text, "==") != 0 && strcmp(expressionBinaryNode->text, "!=") != 0) && leftType->datatypeBitflag == DATA_TYPE_BOOLEAN) {
                return COMPILER_TYPE_ERROR; // comparing booleans < > <= >=
            }
            abstractSyntaxNodeSetInferredType(expressionBinaryNode, DATA_TYPE_BOOLEAN);
            return COMPILER_OK;
        } else if (leftType->datatypeBitflag == DATA_TYPE_STRING || rightType->datatypeBitflag == DATA_TYPE_STRING || leftType->datatypeBitflag == DATA_TYPE_BOOLEAN || rightType->datatypeBitflag == DATA_TYPE_BOOLEAN) {
            return COMPILER_TYPE_ERROR; // comparing string or bool with something else
        } else {
            // comparing ints and doubles
            abstractSyntaxNodeSetInferredType(expressionBinaryNode, DATA_TYPE_BOOLEAN);
            return COMPILER_OK;
        }
    }

}

CompilerStatus semAnalResolveUnaryExpression(AbstractSyntaxNode *unaryExpressionNode,
                                             SemAnalData *data) {
    if (unaryExpressionNode == NULL || data == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    AbstractSyntaxNode *operand = unaryExpressionNode->children[0];
    CompilerStatus status = semAnalResolveNode(operand, data);
    if (status != COMPILER_OK) {
        return status;
    }

    TypeDescriptor *opType = &operand->type;

    // unary operator int or double or bool only
    if (opType->datatypeBitflag != DATA_TYPE_INT && opType->datatypeBitflag != DATA_TYPE_DOUBLE && opType->datatypeBitflag != DATA_TYPE_BOOLEAN) {
        return COMPILER_TYPE_ERROR;
    }

    // non bool type for bool unary operation
    if (unaryExpressionNode->text[0] == '!' && opType->datatypeBitflag != DATA_TYPE_BOOLEAN) {
        return COMPILER_TYPE_ERROR;
    }

    // bool type for non bool unary operations
    if (unaryExpressionNode->text[0] != '!' && opType->datatypeBitflag == DATA_TYPE_BOOLEAN) {
        return COMPILER_TYPE_ERROR;
    }

    abstractSyntaxNodeCopyInferredType(unaryExpressionNode, opType);

    return COMPILER_OK;
}

CompilerStatus semAnalResolveIdentifier(AbstractSyntaxNode *identifierNode, SemAnalData *data) {
    if (identifierNode == NULL || data == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    SymbolTable *currentTable = data->symbolTables[data->numberOfTables - 1];
    Symbol *symbol = symbolTableFind(currentTable, identifierNode->text);

    if (symbol == NULL) {
        return COMPILER_UNDEFINED_SYMBOL_ERROR;
    }

    abstractSyntaxNodeCopyInferredType(identifierNode, symbol->dataType);
    identifierNode->variableId = symbol->variableId;

    return COMPILER_OK;
}


#define BUILTIN_FUN_MACRO(funName, paramCount, retTypeFlag, param1Flag, param2Flag, param3Flag) \
do { \
    TypeDescriptor **tmpTypes = NULL; \
    if ((paramCount) > 0) { \
        tmpTypes = memoryAllocateArray((paramCount), sizeof(TypeDescriptor*)); \
        if (tmpTypes == NULL) { \
            return COMPILER_INTERNAL_ERROR; \
        } \
    } \
    if ((paramCount) >= 1) { \
        tmpTypes[0] = semAnalInitTypeDescriptor(param1Flag); \
    } \
    if ((paramCount) >= 2) { \
        tmpTypes[1] = semAnalInitTypeDescriptor(param2Flag); \
    } \
    if ((paramCount) >= 3) { \
        tmpTypes[2] = semAnalInitTypeDescriptor(param3Flag); \
    } \
    Symbol *tmpSymbol = semAnalCreateSymbolDefault(); \
    if (tmpSymbol == NULL) { \
        return COMPILER_INTERNAL_ERROR; \
    } \
    tmpSymbol->name = (funName); \
    tmpSymbol->defined = true; \
    tmpSymbol->kind = SYMBOL_FUNCTION; \
    tmpSymbol->function.parameterTypes = tmpTypes; \
    tmpSymbol->function.parameterCount = (paramCount); \
    tmpSymbol->function.returnType = semAnalInitTypeDescriptor(retTypeFlag); \
    if (symbolTableInsert(symbols, tmpSymbol) != COMPILER_OK) { \
        return COMPILER_INTERNAL_ERROR; \
    } \
} while (0)

CompilerStatus semAnalDefineBuiltinFunctions(SymbolTable *symbols)
{
    if (symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    BUILTIN_FUN_MACRO("print", 1, DATA_TYPE_UNIT, DATA_TYPE_UNKNOWN, 0, 0);
    BUILTIN_FUN_MACRO("readLine", 0, DATA_TYPE_STRING, 0, 0, 0);
    BUILTIN_FUN_MACRO("readInt", 0, DATA_TYPE_INT, 0, 0, 0);
    BUILTIN_FUN_MACRO("readDouble", 0, DATA_TYPE_DOUBLE, 0, 0, 0);
    BUILTIN_FUN_MACRO("toStr", 1, DATA_TYPE_STRING, DATA_TYPE_UNKNOWN, 0, 0);
    BUILTIN_FUN_MACRO("toInt", 1, DATA_TYPE_INT, DATA_TYPE_DOUBLE, 0, 0);
    BUILTIN_FUN_MACRO("strLength", 1, DATA_TYPE_INT, DATA_TYPE_STRING, 0, 0);
    BUILTIN_FUN_MACRO("subStr", 3, DATA_TYPE_STRING, DATA_TYPE_STRING, DATA_TYPE_INT,
                      DATA_TYPE_INT);
    BUILTIN_FUN_MACRO("ord", 2, DATA_TYPE_INT, DATA_TYPE_STRING, DATA_TYPE_INT, 0);
    BUILTIN_FUN_MACRO("chr", 1, DATA_TYPE_STRING, DATA_TYPE_INT, 0, 0);

    BUILTIN_FUN_MACRO("readBoolean", 0, DATA_TYPE_BOOLEAN, 0, 0, 0);
    BUILTIN_FUN_MACRO("readIntMaybe", 0, DATA_TYPE_INT | DATA_TYPE_UNIT, 0, 0, 0);
    BUILTIN_FUN_MACRO("readDoubleMaybe", 0, DATA_TYPE_DOUBLE | DATA_TYPE_UNIT, 0, 0, 0);

    return COMPILER_OK;
}
