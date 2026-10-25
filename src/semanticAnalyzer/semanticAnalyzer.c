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

#include <stdbool.h>
#include <stdlib.h>



struct SemAnalData
{
    size_t variableIdCounter;
    SymbolTable *functionSymbols;
    size_t numberOfTables;
    SymbolTable **symbolTables;
};

CompilerStatus semAnalDataInit(SemAnalData *data)
{
    if (data == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    data->variableIdCounter = 0;
    data->numberOfTables = 0;
    data->symbolTables = NULL;
    data->functionSymbols = symbolTableCreate();
    if (data->functionSymbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    return COMPILER_OK;
}

void semAnalDataDestroy(SemAnalData *data)
{
    if (data == NULL) {
        return;
    }

    for (size_t tableI = 0; tableI < data->numberOfTables; ++tableI) {
        symbolTableDestroy(data->symbolTables[tableI]);
    }
    free(data->symbolTables);
    symbolTableDestroy(data->functionSymbols);
    data->variableIdCounter = 0;
    data->functionSymbols = NULL;
    data->numberOfTables = 0;
    data->symbolTables = NULL;
}


CompilerStatus semanticAnalyzerAnalyze(SourceProgram *program, SymbolTable *symbols) {
    if (program == NULL || symbols == NULL || program->root == NULL || program->root->type != ABSTRACT_SYNTAX_PROGRAM) {
        return COMPILER_INTERNAL_ERROR;
    }

    CompilerStatus status = COMPILER_OK;
    AbstractSyntaxNode *root = program->root;

    SemAnalData data;
    status = semAnalDataInit(&data);
    if (status != COMPILER_OK) {
        return status;
    }

    status = semAnalDefineBuiltinFunctions(data.functionSymbols);
    if (status != COMPILER_OK) {
        semAnalDataDestroy(&data);
        return status;
    }

    status = semAnalDeclareFunctions(root, data.functionSymbols);
    if (status != COMPILER_OK) {
        semAnalDataDestroy(&data);
        return status;
    }

    semAnalDataDestroy(&data);
    return status;
}


CompilerStatus semAnalDeclareFunctions(AbstractSyntaxNode *rootNode, SymbolTable *symbols)
{
    if (rootNode == NULL || symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    bool hasMain = false;

    for (size_t funI = 0; funI < rootNode->childCount; ++funI) {
        // multiple mains
        if (hasMain && rootNode->children[funI]->isEntryFunction) {
            return COMPILER_REDEFINITION_ERROR;
        }

        // main not propper main
        CompilerStatus status;
        if (hasMain = rootNode->children[funI]->isEntryFunction) {
            status = semAnalCheckMain(rootNode->children[funI]);
            if (status != COMPILER_OK) {
                return status;
            }
        }

        // create symbol
        Symbol *symbolTmp = semAnalAstNodeToSymbol(rootNode->children[funI]);
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
    if (functionNode == NULL || functionNode->type != ABSTRACT_SYNTAX_FUNCTION ||
        functionNode->childCount == 0) {
        return COMPILER_INTERNAL_ERROR;
    }

    if (!functionNode->isEntryFunction) {
        return COMPILER_OK;
    }

    size_t parameterCount = functionNode->childCount - 1;
    if (parameterCount != 0) {
        return COMPILER_ARGUMENT_ERROR;
    }

    if (functionNode->variableType != DATA_TYPE_UNIT) {
        return COMPILER_TYPE_ERROR;
    }

    return COMPILER_OK;
}


Symbol *semAnalCreateSymbolDefault()
{
    Symbol *symbol = memoryAllocate(sizeof(Symbol));
    if (symbol == NULL) {
        return NULL;
    }

    symbol->name = NULL;
    symbol->kind = SYMBOL_FUNCTION;
    symbol->dataType = DATA_TYPE_UNKNOWN;
    symbol->mutable = false;
    symbol->defined = false;
    symbol->scopeDepth = 1;
    symbol->function.parameterTypes = NULL;
    symbol->function.parameterCount = 0;
    symbol->function.returnType = DATA_TYPE_UNKNOWN;

    return symbol;
}

void semAnalFreeSymbol(Symbol *symbol)
{
    free(symbol->function.parameterTypes);
    free(symbol);
}


DataType *semAnalAstGetParameterTypes(AbstractSyntaxNode *node) {
    if (node == NULL || node->type != ABSTRACT_SYNTAX_FUNCTION) {
        return NULL;
    }

    DataType *types = memoryAllocate(sizeof(DataType) * (node->childCount - 1));
    if (types == NULL) {
        return NULL;
    }

    for (size_t parameterI = 0; parameterI < node->childCount - 1; parameterI++) {
        if (node->children[parameterI] == NULL || node->children[parameterI]->type != ABSTRACT_SYNTAX_PARAMETER) {
            free(types);
            return NULL;
        }

        types[parameterI] = node->children[parameterI]->variableType;
    }

    return types;
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
    } else if (node->kind == ABSTRACT_SYNTAX_PARAMETER ||
               node->kind == ABSTRACT_SYNTAX_VARIABLE_DECLARATION) {
        symbol->kind = SYMBOL_VARIABLE;
    } else {
        free(symbol);
        return NULL;
    }

    symbol->name = node->text;
    symbol->dataType = node->variableType;
    symbol->mutable = node->isMutable;

    if (symbol->kind == ABSTRACT_SYNTAX_FUNCTION) {
        symbol->function.parameterTypes = astGetParameterTypes(node);
        if (symbol->function.parameterTypes == NULL) {
            free(symbol);
            return NULL;
        }
        symbol->function.parameterCount = node->childCount - 1;
        symbol->function.returnType = node->variableType;
    }

    return symbol;
}

/**
 * @brief Adds a new function symbol to the symbol table and enters new scope.
 *
 * @param functionNode pointer to a AST node type function
 * @param symbols pointer to the existing symbol table
 *
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveFunction(AbstractSyntaxNode *functionNode, SymbolTable *symbols) {
    if (functionNode == NULL || symbols == NULL || functionNode->type != ABSTRACT_SYNTAX_FUNCTION) {
        return COMPILER_INTERNAL_ERROR;
    }

    CompilerStatus status = COMPILER_OK;

    Symbol *functionSymbol = astNodeToSymbol(functionNode);
    if (functionSymbol == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    functionSymbol->scopeDepth = symbolTableGetCurrentScope(symbols);
    functionSymbol->defined = true;

    status = symbolTableInsert(symbols, functionSymbol);
    if (status != COMPILER_OK) {
        free(functionSymbol->function.parameterTypes);
        free(functionSymbol);
        return status;
    }

    status = symbolTableEnterScope(symbols);
    if (status != COMPILER_OK) {
        free(functionSymbol->function.parameterTypes);
        free(functionSymbol);
        return status;
    }

    // TODO cycle through parameters
    // TODO resolve body

    return status;
}

/**
 * @brief Adds a new variable(parameter) symbol to the symbol table.
 *
 * @param parameterNode pointer to a AST node type parameter
 * @param symbols pointer to the existing symbol table
 *
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveParameter(AbstractSyntaxNode *parameterNode, SymbolTable *symbols) {
    if (parameterNode == NULL || symbols == NULL ||
        parameterNode->type != ABSTRACT_SYNTAX_PARAMETER) {
        return COMPILER_INTERNAL_ERROR;
    }

    CompilerStatus status = COMPILER_OK;

    Symbol *parameterSymbol = astNodeToSymbol(parameterNode);
    if (parameterSymbol == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    parameterSymbol->scopeDepth = symbolTableGetCurrentScope(symbols);
    parameterSymbol->defined = true;

    status = symbolTableInsert(symbols, parameterSymbol);
    if (status != COMPILER_OK) {
        free(parameterSymbol);
        return status;
    }

    return status;
}

CompilerStatus semAnalResolveBlock(AbstractSyntaxNode *blockNode, SymbolTable *symbols) {
    if (blockNode == NULL || symbols == NULL || blockNode->type != ABSTRACT_SYNTAX_BLOCK) {
        return COMPILER_INTERNAL_ERROR;
    }
    CompilerStatus status = COMPILER_OK;

    // TODO

    return status;
}

CompilerStatus semAnalResolveIf(AbstractSyntaxNode *ifNode, SymbolTable *symbols) {
    if (ifNode == NULL || symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    CompilerStatus status = COMPILER_OK;

    // TODO

    return status;
}

CompilerStatus semAnalResolveWhile(AbstractSyntaxNode *whileNode, SymbolTable *symbols) {
    if (whileNode == NULL || symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    CompilerStatus status = COMPILER_OK;

    // TODO

    return status;
}

CompilerStatus semAnalResolveReturn(AbstractSyntaxNode *returnNode, SymbolTable *symbols) {
    if (returnNode == NULL || symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    CompilerStatus status = COMPILER_OK;

    // TODO

    return status;
}

CompilerStatus semAnalResolveFunctionCall(AbstractSyntaxNode *functionCallNode,
                                          SymbolTable *symbols) {
    if (functionCallNode == NULL || symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    CompilerStatus status = COMPILER_OK;

    // TODO

    return status;
}

CompilerStatus semAnalResolveVariableDeclaration(AbstractSyntaxNode *variableDeclarationNode,
                                                 SymbolTable *symbols) {
    if (variableDeclarationNode == NULL || symbols == NULL ||
        variableDeclarationNode->type != ABSTRACT_SYNTAX_VARIABLE_DECLARATION) {
        return COMPILER_INTERNAL_ERROR;
    }

    CompilerStatus status = COMPILER_OK;

    Symbol *variableSymbol = astNodeToSymbol(variableDeclarationNode);
    if (variableSymbol == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    variableSymbol->scopeDepth = symbolTableGetCurrentScope(symbols);
    variableSymbol->defined = false;

    status = symbolTableInsert(symbols, variableSymbol);
    if (status != COMPILER_OK) {
        free(variableSymbol);
        return status;
    }

    // TODO resolve initialization

    return status;
}

CompilerStatus semAnalResolveAssignment(AbstractSyntaxNode *assignmentNode, SymbolTable *symbols) {
    if (assignmentNode == NULL || symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    CompilerStatus status = COMPILER_OK;

    // TODO

    return status;
}

CompilerStatus semAnalResolveExpressionStatement(AbstractSyntaxNode *expressionStatementNode,
                                                 SymbolTable *symbols) {
    if (expressionStatementNode == NULL || symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    CompilerStatus status = COMPILER_OK;

    // TODO

    return status;
}

CompilerStatus semAnalResolveExpressionBinary(AbstractSyntaxNode *expressionBinaryNode,
                                              SymbolTable *symbols) {
    if (expressionBinaryNode == NULL || symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    CompilerStatus status = COMPILER_OK;

    // TODO

    return status;
}

CompilerStatus semAnalResolveUnaryExpression(AbstractSyntaxNode *unaryExpressionNode,
                                             SymbolTable *symbols) {
    if (unaryExpressionNode == NULL || symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    CompilerStatus status = COMPILER_OK;

    // TODO

    return status;
}

CompilerStatus semAnalResolveIdentifier(AbstractSyntaxNode *identifierNode, SymbolTable *symbols) {
    if (identifierNode == NULL || symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    CompilerStatus status = COMPILER_OK;

    // TODO

    return status;
}





#define BUILTIN_FUN_MACRO(funName, paramCount, retType, param1, param2, param3)\
do{\
    DataType *tmpTypes = NULL;\
    if ((paramCount) > 0) {\
        tmpTypes = memoryAllocateArray((paramCount), sizeof(DataType));\
        if(tmpTypes == NULL) {\
            return COMPILER_INTERNAL_ERROR;\
        }\
    }\
    \
    switch ((paramCount))\
    {\
    case 3:\
        tmpTypes[2] = (param3);\
    case 2:\
        tmpTypes[1] = (param2);\
    case 1:\
        tmpTypes[0] = (param1);\
    default:\
        break;\
    }\
    \
    Symbol *tmpSymbol = semAnalCreateSymbolDefault();\
    if (tmpSymbol == NULL) {\
        free(tmpTypes);\
        return COMPILER_INTERNAL_ERROR;\
    }\
    \
    tmpSymbol->name = (funName);\
    tmpSymbol->defined = true;\
    tmpSymbol->kind = SYMBOL_FUNCTION;\
    tmpSymbol->function.parameterTypes = tmpTypes;\
    tmpSymbol->function.parameterCount = (paramCount);\
    tmpSymbol->function.returnType = (retType);\
    \
    if (symbolTableInsert(symbols, tmpSymbol) != COMPILER_OK) {\
        free(tmpTypes);\
        free(tmpSymbol);\
        return COMPILER_INTERNAL_ERROR;\
    }\
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
    BUILTIN_FUN_MACRO("subStr", 3, DATA_TYPE_STRING, DATA_TYPE_STRING, DATA_TYPE_INT, DATA_TYPE_INT);
    BUILTIN_FUN_MACRO("ord", 2, DATA_TYPE_INT, DATA_TYPE_STRING, DATA_TYPE_INT, 0);
    BUILTIN_FUN_MACRO("chr", 1, DATA_TYPE_STRING, DATA_TYPE_INT, 0, 0);

    return COMPILER_OK;
}
