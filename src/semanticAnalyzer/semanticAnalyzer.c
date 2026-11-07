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
#include <stddef.h>
#include <stdlib.h>
#include <string.h>



struct SemAnalData
{
    size_t variableIdCounter;
    SymbolTable *functionSymbols;
    size_t numberOfTables;
    SymbolTable **symbolTables;
    const char **recursionList;
    AbstractSyntaxNode *root;
};

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

    return COMPILER_OK;
}

CompilerStatus semAnalListsBigger(SemAnalData *data)
{
    if (data == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    SymbolTable *newTable = symbolTableCreate();
    if (newTable == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    SymbolTable **newTables = memoryResizeArray(
        data->symbolTables, data->numberOfTables + 1, sizeof(SymbolTable *));
    if (newTables == NULL) {
        symbolTableDestroy(newTable);
        return COMPILER_INTERNAL_ERROR;
    }
    data->symbolTables = newTables;
    data->symbolTables[data->numberOfTables] = newTable;

    const char **newRecList = memoryResizeArray(data->recursionList, data->numberOfTables + 1, sizeof(const char *));
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

CompilerStatus semAnalListsSmaller(SemAnalData *data)
{
    if (data == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    if (data->numberOfTables == 0) {
        return COMPILER_OK;
    }

    size_t lastTable = data->numberOfTables - 1;
    symbolTableDestroy(data->symbolTables[lastTable]);
    data->symbolTables[lastTable] = NULL;
    data->recursionList[lastTable] = NULL;
    --data->numberOfTables;

    SymbolTable **smallerTables = memoryResizeArray(
        data->symbolTables, data->numberOfTables, sizeof(SymbolTable *));
    if (smallerTables == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    data->symbolTables = smallerTables;

    const char **smallerRecList = memoryResizeArray(
        data->recursionList, data->numberOfTables, sizeof(const char *));
    if (smallerRecList == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    data->recursionList = smallerRecList;

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
    free(data->recursionList);
    symbolTableDestroy(data->functionSymbols);
    data->variableIdCounter = 0;
    data->functionSymbols = NULL;
    data->numberOfTables = 0;
    data->symbolTables = NULL;
    data->recursionList = NULL;
    data->root = NULL;
}


CompilerStatus semanticAnalyzerAnalyze(SourceProgram *program) {
    if (program == NULL || program->root == NULL || program->root->type != ABSTRACT_SYNTAX_PROGRAM) {
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

    status = semAnalDeclareFunctions(root, data.functionSymbols);
    if (status != COMPILER_OK) {
        semAnalDataDestroy(&data);
        return status;
    }

    // code analysis, loop to define all functions
    for (size_t funI = 0; funI < root->childCount; ++funI) {
        Symbol *tmpSymbol = NULL;
        if ((tmpSymbol = symbolTableFindCurrentScope(data.functionSymbols, root->children[funI]->text)) == NULL) {
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


CompilerStatus semAnalDeclareFunctions(AbstractSyntaxNode *rootNode, SymbolTable *symbols)
{
    if (rootNode == NULL || symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    CompilerStatus status;

    bool hasMain = false;

    for (size_t funI = 0; funI < rootNode->childCount; ++funI) {
        // multiple mains
        if (hasMain && rootNode->children[funI]->isEntryFunction) {
            return COMPILER_REDEFINITION_ERROR;
        }

        // check if main is main, if not main then ok
        status = semAnalCheckMain(rootNode->children[funI]);
        if (status != COMPILER_OK) {
            return status;
        }

        // update hasMain
        hasMain = hasMain || rootNode->children[funI]->isEntryFunction;

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

    if (symbol->kind == SYMBOL_FUNCTION) {
        symbol->function.parameterTypes = semAnalAstGetParameterTypes(node);
        if (symbol->function.parameterTypes == NULL) {
            free(symbol);
            return NULL;
        }
        symbol->function.parameterCount = node->childCount - 1;
        symbol->function.returnType = node->variableType;
    }

    return symbol;
}


bool semAnalIsIfExpression(AbstractSyntaxNode *ifNode)
{
    if (ifNode == NULL || ifNode->type != ABSTRACT_SYNTAX_IF || ifNode->childCount < 2) {
        return false;
    }

    if (ifNode->childCount < 3) {
        return false; // missing else branch - not a expression
    }

    if (ifNode->inferredType == DATA_TYPE_UNKNOWN) {
        return false; // branches do not return same types
    }

    return true;
}

bool semAnalCheckIfReturns(AbstractSyntaxNode *ifNode) {
    if (ifNode == NULL || ifNode->type != ABSTRACT_SYNTAX_IF || ifNode->childCount < 2) {
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

    if (node->type == ABSTRACT_SYNTAX_RETURN) {
        return true;
    }

    if (node->type == ABSTRACT_SYNTAX_BLOCK) {
        if (node->childCount == 0) {
            return false;
        }

        AbstractSyntaxNode *lastStatement = node->children[node->childCount - 1];
        return semAnalNodeEndsWithReturn(lastStatement);
    }

    if (node->type == ABSTRACT_SYNTAX_IF) {
        return semAnalCheckIfReturns(node);
    }

    return false;
}


CompilerStatus semAnalResolveFunctionReturnType(AbstractSyntaxNode *functionNode, Symbol *functionSymbol) {
    AbstractSyntaxNode *lastNode = functionNode->children[functionNode->childCount - 1];

    // get the last type in the body
    DataType lastType = DATA_TYPE_UNKNOWN;
    if (lastNode->type == ABSTRACT_SYNTAX_BLOCK && lastNode->childCount != 0) {
        lastNode = lastNode->children[lastNode->childCount - 1];
    }
    // if statement last logic
    if (lastNode->type == ABSTRACT_SYNTAX_IF) {
        if (functionNode->variableType == DATA_TYPE_UNIT || semAnalCheckIfReturns(lastNode)
        || lastNode->inferredType == DATA_TYPE_UNIT || semAnalIsIfExpression(lastNode)) {
            // explicit Unit return type - if is always valid
            // if has explicit return in all branches
            // all branches evaluate to Unit
            // if is a vlaid expression
            lastType = semAnalEvaluatesToType(lastNode);
        } else {
            return COMPILER_TYPE_ERROR; // invalid if at the end
        }
    }

    // get the correct function type
    if (functionSymbol->function.returnType == DATA_TYPE_UNKNOWN) {
        functionSymbol->function.returnType = lastType;
    } else if (functionNode->variableType != DATA_TYPE_UNIT && functionSymbol->function.returnType != lastType) {
        return COMPILER_TYPE_ERROR;
    }

    functionNode->inferredType = functionSymbol->function.returnType;

    return COMPILER_OK;
}


CompilerStatus semAnalDefineFunction(AbstractSyntaxNode *functionNode, SemAnalData *data) {
    if (functionNode == NULL || data == NULL || data->functionSymbols == NULL ||
        functionNode->type != ABSTRACT_SYNTAX_FUNCTION) {
        return COMPILER_INTERNAL_ERROR;
    }

    CompilerStatus status = COMPILER_OK;

    Symbol *functionSymbol = symbolTableFindCurrentScope(data->functionSymbols, functionNode->text);
    if (functionSymbol == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    status = semAnalListsBigger(data);
    if (status != COMPILER_OK) {
        return status;
    }

    data->recursionList[data->numberOfTables - 1] = functionNode->text;


    // declaring parameters - adding to symbol table
    for (size_t parmI = 0; parmI < functionNode->childCount - 1; ++parmI) {
        status = semAnalDeclareParameter(functionNode->children[parmI], data);
        if (status != COMPILER_OK) {
            return status;
        }
    }

    AbstractSyntaxNode *lastNode = functionNode->children[functionNode->childCount - 1];

    // resolving the body
    status = semAnalResolveNode(lastNode, data);
    if (status != COMPILER_OK) {
        return status;
    }

    status = semAnalListsSmaller(data);
    if (status != COMPILER_OK) {
        return status;
    }

    return semAnalResolveFunctionReturnType(functionNode, functionSymbol);
}


CompilerStatus semAnalDeclareParameter(AbstractSyntaxNode *parameterNode, SemAnalData *data) {
    if (parameterNode == NULL || data == NULL || data->symbolTables[data->numberOfTables - 1] == NULL ||
        parameterNode->type != ABSTRACT_SYNTAX_PARAMETER) {
        return COMPILER_INTERNAL_ERROR;
    }

    CompilerStatus status = COMPILER_OK;

    Symbol *parameterSymbol = semAnalAstNodeToSymbol(parameterNode);
    if (parameterSymbol == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    ++data->variableIdCounter;
    parameterSymbol->variableId = data->variableIdCounter;

    status = symbolTableInsert(data->symbolTables[data->numberOfTables - 1], parameterSymbol);
    if (status != COMPILER_OK) {
        semAnalFreeSymbol(parameterSymbol);
        return status;
    }

    parameterNode->variableId = parameterSymbol->variableId;

    return status;
}


CompilerStatus semAnalResolveNode(AbstractSyntaxNode *node, SemAnalData *data) {
    if (node == NULL) {
        return COMPILER_OK;
    }
    switch (node->type) {
        case ABSTRACT_SYNTAX_BLOCK: return semAnalResolveBlock(node, data);
        case ABSTRACT_SYNTAX_IF: return semAnalResolveIf(node, data);
        case ABSTRACT_SYNTAX_WHILE: return semAnalResolveWhile(node, data);
        case ABSTRACT_SYNTAX_RETURN: return semAnalResolveReturn(node, data);
        case ABSTRACT_SYNTAX_FUNCTION_CALL: return semAnalResolveFunctionCall(node, data);
        case ABSTRACT_SYNTAX_VARIABLE_DECLARATION: return semAnalResolveVariableDeclaration(node, data);
        case ABSTRACT_SYNTAX_ASSIGNMENT: return semAnalResolveAssignment(node, data);
        case ABSTRACT_SYNTAX_EXPRESSION_STATEMENT: return semAnalResolveExpressionStatement(node, data);
        case ABSTRACT_SYNTAX_EXPRESSION_BINARY: return semAnalResolveExpressionBinary(node, data);
        case ABSTRACT_SYNTAX_UNARY_EXPRESSION: return semAnalResolveUnaryExpression(node, data);
        case ABSTRACT_SYNTAX_IDENTIFIER: return semAnalResolveIdentifier(node, data);
        case ABSTRACT_SYNTAX_FOR: return semAnalResolveFor(node, data);
        case ABSTRACT_SYNTAX_INT_LITERAL:
            node->inferredType = DATA_TYPE_INT;
            return COMPILER_OK;
        case ABSTRACT_SYNTAX_DOUBLE_LITERAL:
            node->inferredType = DATA_TYPE_DOUBLE;
            return COMPILER_OK;
        case ABSTRACT_SYNTAX_STRING_LITERAL:
            node->inferredType = DATA_TYPE_STRING;
            return COMPILER_OK;
        case ABSTRACT_SYNTAX_BOOLEAN_LITERAL:
            node->inferredType = DATA_TYPE_BOOLEAN;
            return COMPILER_OK;
        case ABSTRACT_SYNTAX_UNIT_LITERAL:
            node->inferredType = DATA_TYPE_UNIT;
            return COMPILER_OK;
        default:
            return COMPILER_INTERNAL_ERROR;
    }
}

CompilerStatus semAnalResolveBlock(AbstractSyntaxNode *blockNode, SemAnalData *data) {
    if (blockNode == NULL || data == NULL || data->functionSymbols == NULL ||
        blockNode->type != ABSTRACT_SYNTAX_BLOCK) {
        return COMPILER_INTERNAL_ERROR;
    }

    SymbolTable *currentTable = data->symbolTables[data->numberOfTables - 1];
    CompilerStatus status = symbolTableEnterScope(currentTable);
    if (status != COMPILER_OK) {
        return status;
    }

    for (size_t i = 0; i < blockNode->childCount; ++i) {
        status = semAnalResolveNode(blockNode->children[i], data);
        if (status != COMPILER_OK) {
            symbolTableLeaveScope(currentTable);
            return status;
        }
    }

    blockNode->inferredType = semAnalEvaluatesToType(blockNode);

    symbolTableLeaveScope(currentTable);

    return COMPILER_OK;
}

DataType semAnalEvaluatesToType(AbstractSyntaxNode *node)
{
    if (node->type == ABSTRACT_SYNTAX_BLOCK) {
        if (node->childCount == 0) {
            return DATA_TYPE_UNIT;
        } else {
            node = node->children[node->childCount - 1];
        }
    }

    if (node->type == ABSTRACT_SYNTAX_EXPRESSION_STATEMENT || node->type == ABSTRACT_SYNTAX_FUNCTION_CALL
        || node->type == ABSTRACT_SYNTAX_RETURN || node->type == ABSTRACT_SYNTAX_IF) {
        return node->inferredType;
    }

    return DATA_TYPE_UNIT;
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
    if (condition->inferredType != DATA_TYPE_BOOLEAN) {
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

    DataType allBranchesType = DATA_TYPE_UNKNOWN;
    for (size_t branchI = 1; branchI < ifNode->childCount; ++branchI) {
        DataType branchType = semAnalEvaluatesToType(ifNode->children[branchI]);
        if (branchType == DATA_TYPE_UNKNOWN) {
            allBranchesType = DATA_TYPE_UNKNOWN; // when a branch evaluates to Unknown, then also the whole if statement
            break;
        }
        allBranchesType |= branchType;
    }

    // get the evaluation type of the if statement
    if ((allBranchesType & (allBranchesType - 1)) == 0) { // only one data type (or zero)
        ifNode->inferredType = allBranchesType;
    } else if (allBranchesType == (DATA_TYPE_INT | DATA_TYPE_DOUBLE)) { // only int or double
        ifNode->inferredType = DATA_TYPE_DOUBLE;
    } else {
        ifNode->inferredType = DATA_TYPE_UNKNOWN;
    }

    // not a expression evaluating as non Unit type
    if (ifNode->inferredType != DATA_TYPE_UNIT && !semAnalIsIfExpression(ifNode)) {
        ifNode->inferredType = DATA_TYPE_UNKNOWN;
    }

    return COMPILER_OK;
}

CompilerStatus semAnalResolveFor(AbstractSyntaxNode *forNode, SemAnalData *data) {
    if (forNode == NULL || data == NULL || data->functionSymbols == NULL || forNode->childCount < 5) {
        return COMPILER_INTERNAL_ERROR;
    }

    CompilerStatus status = COMPILER_OK;

    // resolve generator bounds in the outer scope
    status = semAnalResolveNode(forNode->children[1], data);
    if (status != COMPILER_OK) {
        return status;
    }
    if (forNode->children[1]->inferredType != DATA_TYPE_INT) {
        return COMPILER_TYPE_ERROR;
    }

    status = semAnalResolveNode(forNode->children[2], data);
    if (status != COMPILER_OK) {
        return status;
    }
    if (forNode->children[2]->inferredType != DATA_TYPE_INT) {
        return COMPILER_TYPE_ERROR;
    }

    // resolve optional step
    if (forNode->children[3] != NULL) {
        status = semAnalResolveNode(forNode->children[3], data);
        if (status != COMPILER_OK) {
            return status;
        }
        if (forNode->children[3]->inferredType != DATA_TYPE_INT) {
            return COMPILER_TYPE_ERROR;
        }
    }

    // push the inner scope for the loop body and guards
    SymbolTable *currentTable = data->symbolTables[data->numberOfTables - 1];
    status = symbolTableEnterScope(currentTable);
    if (status != COMPILER_OK) {
        return status;
    }

    // declare the generator variable inside the new scope
    AbstractSyntaxNode *generatorNode = forNode->children[0];
    if (generatorNode == NULL || generatorNode->type != ABSTRACT_SYNTAX_VARIABLE_DECLARATION || generatorNode->text == NULL) {
        symbolTableLeaveScope(currentTable);
        return COMPILER_INTERNAL_ERROR;
    }

    Symbol *tmpSymbol = symbolTableFindCurrentScope(currentTable, generatorNode->text);
    if (tmpSymbol != NULL) {
        symbolTableLeaveScope(currentTable);
        return COMPILER_REDEFINITION_ERROR; // variable name already exists
    }

    // explicitly set type and mutability on node before converting to symbol
    generatorNode->variableType = DATA_TYPE_INT;
    generatorNode->isMutable = false;

    // generator symbol creation
    Symbol *generatorSymbol = semAnalAstNodeToSymbol(generatorNode);
    if (generatorSymbol == NULL) {
        symbolTableLeaveScope(currentTable);
        return COMPILER_INTERNAL_ERROR;
    }

    ++data->variableIdCounter;
    generatorSymbol->variableId = data->variableIdCounter;
    generatorNode->variableId = generatorSymbol->variableId;
    generatorNode->inferredType = DATA_TYPE_INT;

    status = symbolTableInsert(currentTable, generatorSymbol);
    if (status != COMPILER_OK) {
        semAnalFreeSymbol(generatorSymbol);
        symbolTableLeaveScope(currentTable);
        return status;
    }

    // resolve guards in the inner scope
    for (size_t guardI = 4; guardI < forNode->childCount - 1; ++guardI) {
        AbstractSyntaxNode *guardNode = forNode->children[guardI];
        status = semAnalResolveNode(guardNode, data);
        if (status != COMPILER_OK) {
            symbolTableLeaveScope(currentTable);
            return status;
        }
        if (guardNode->inferredType != DATA_TYPE_BOOLEAN) {
            symbolTableLeaveScope(currentTable);
            return COMPILER_TYPE_ERROR;
        }
    }

    // resolve the loop body
    AbstractSyntaxNode *bodyNode = forNode->children[forNode->childCount - 1];
    status = semAnalResolveNode(bodyNode, data);
    if (status != COMPILER_OK) {
        symbolTableLeaveScope(currentTable);
        return status;
    }

    // clean up scope and set final type
    symbolTableLeaveScope(currentTable);

    forNode->inferredType = DATA_TYPE_UNIT;

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
    if (condition->inferredType != DATA_TYPE_BOOLEAN) {
        return COMPILER_TYPE_ERROR;
    }

    // resolve body block
    status = semAnalResolveNode(whileNode->children[1], data);
    if (status != COMPILER_OK) {
        return status;
    }

    whileNode->inferredType = DATA_TYPE_UNIT;
    return COMPILER_OK;
}

CompilerStatus semAnalResolveReturn(AbstractSyntaxNode *returnNode, SemAnalData *data) {
    if (returnNode == NULL || data == NULL || data->functionSymbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    if (returnNode->childCount > 0 && returnNode->children[0] != NULL) {
        CompilerStatus status = semAnalResolveNode(returnNode->children[0], data);
        if (status != COMPILER_OK) {
            return status;
        }
        returnNode->inferredType = returnNode->children[0]->inferredType;
    } else {
        returnNode->inferredType = DATA_TYPE_UNIT;
    }

    // resolve if function has declared data type
    Symbol *functionSymbol = symbolTableFindCurrentScope(data->functionSymbols,data->recursionList[data->numberOfTables - 1]);
    if (functionSymbol == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    if (functionSymbol->function.returnType == DATA_TYPE_UNKNOWN
        || (functionSymbol->function.returnType != DATA_TYPE_UNIT && functionSymbol->function.returnType != returnNode->inferredType)) {
        return COMPILER_TYPE_ERROR;
    }

    return COMPILER_OK;
}

int semAnalInRecursion(const char* funName, SemAnalData *data)
{
    if (funName == NULL || data == NULL || data->recursionList == NULL) {
        return -2; // internal error
    }

    size_t i;
    for (i = 0; i < data->numberOfTables && strcmp(data->recursionList[i], funName) != 0; ++i) {}
    if (i == data->numberOfTables) {
        return -1; // not a reccurent function
    } else {
        return i; // recursion function index
    }
}

CompilerStatus semAnalCheckRecursion(size_t recursionI, SemAnalData *data)
{
    while (recursionI < data->numberOfTables) {
        Symbol *funSymbolTmp = symbolTableFindCurrentScope(data->functionSymbols, data->recursionList[recursionI]);
        if (funSymbolTmp == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }
        // function inside recursion does not have explicit return type
        if (funSymbolTmp->function.returnType == DATA_TYPE_UNKNOWN) {
            return COMPILER_TYPE_ERROR;
        }

        ++recursionI;
    }

    return COMPILER_OK;
}

CompilerStatus semAnalResolveFunctionCall(AbstractSyntaxNode *functionCallNode, SemAnalData *data) {
    if (functionCallNode == NULL || data == NULL || data->functionSymbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    Symbol *funcSymbol = symbolTableFind(data->functionSymbols, functionCallNode->text);
    if (funcSymbol == NULL || funcSymbol->kind != SYMBOL_FUNCTION) {
        return COMPILER_UNDEFINED_SYMBOL_ERROR;
    }

    if (functionCallNode->childCount != funcSymbol->function.parameterCount) {
        return COMPILER_ARGUMENT_ERROR;
    }

    // check the arguments
    CompilerStatus status = COMPILER_OK;
    for (size_t i = 0; i < functionCallNode->childCount; ++i) {
        AbstractSyntaxNode *argNode = functionCallNode->children[i];

        status = semAnalResolveNode(argNode, data);
        if (status != COMPILER_OK) {
            return status;
        }

        DataType argType = argNode->inferredType;
        DataType expectedType = funcSymbol->function.parameterTypes[i];

        if (expectedType != DATA_TYPE_UNKNOWN && expectedType != argType) {
            if (expectedType == DATA_TYPE_DOUBLE && argType == DATA_TYPE_INT) {
                argNode->inferredType = DATA_TYPE_INT; // implicit conversion later
            } else {
                return COMPILER_ARGUMENT_ERROR;
            }
        }
    }

    // defining a function if not defined, checking explicit returns if reccurent
    if (!funcSymbol->defined) {
        int recursionResult = semAnalInRecursion(functionCallNode->text, data);
        if (recursionResult < -1) {
            return COMPILER_INTERNAL_ERROR;
        }

        if (recursionResult == -1) {
            size_t funI;
            for (funI = 0; strcmp(data->root->children[funI]->text, functionCallNode->text) != 0; ++funI) {}
            status = semAnalDefineFunction(data->root->children[funI], data);
        } else {
            status = semAnalCheckRecursion(recursionResult, data);
        }

        if (status != COMPILER_OK) {
            return status;
        }
    }


    functionCallNode->inferredType = funcSymbol->function.returnType;

    return COMPILER_OK;
}

CompilerStatus semAnalResolveVariableDeclaration(AbstractSyntaxNode *variableDeclarationNode, SemAnalData *data) {
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

    CompilerStatus status = semAnalResolveNode(initExpr, data);
    if (status != COMPILER_OK) {
        return status;
    }

    // check for wrong types
    if (variableDeclarationNode->variableType != DATA_TYPE_UNKNOWN &&
        initExpr->inferredType != variableDeclarationNode->variableType) {
        if (variableDeclarationNode->variableType == DATA_TYPE_DOUBLE && initExpr->inferredType == DATA_TYPE_INT) {
            initExpr->inferredType = DATA_TYPE_INT; // implicit conversion later
        } else {
            return COMPILER_TYPE_ERROR;
        }
    }

    if (variableDeclarationNode->variableType == DATA_TYPE_UNKNOWN) {
        variableDeclarationNode->variableType = initExpr->inferredType;
        if (variableDeclarationNode->variableType == DATA_TYPE_UNKNOWN) {
            return COMPILER_TYPE_ERROR; //unable to get type
        }
    }

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
    CompilerStatus status = semAnalResolveNode(assignExpr, data);
    if (status != COMPILER_OK) {
        return status;
    }

    // check types
    if (varSymbol->dataType != assignExpr->inferredType) {
        if (varSymbol->dataType == DATA_TYPE_DOUBLE && assignExpr->inferredType == DATA_TYPE_INT) {
            assignExpr->inferredType = DATA_TYPE_INT; // implicit conversion later
        } else {
            return COMPILER_TYPE_ERROR;
        }
    }

    assignmentNode->inferredType = varSymbol->dataType;
    assignmentNode->variableId = varSymbol->variableId;

    return COMPILER_OK;
}

CompilerStatus semAnalResolveExpressionStatement(AbstractSyntaxNode *expressionStatementNode, SemAnalData *data) {
    if (expressionStatementNode == NULL || data == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    CompilerStatus status = semAnalResolveNode(expressionStatementNode->children[0], data);
    if (status != COMPILER_OK) {
        return status;
    }

    expressionStatementNode->inferredType = expressionStatementNode->children[0]->inferredType;

    return COMPILER_OK;
}

CompilerStatus semAnalResolveExpressionBinary(AbstractSyntaxNode *expressionBinaryNode, SemAnalData *data) {
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

    if (left->inferredType == DATA_TYPE_UNIT || right->inferredType == DATA_TYPE_UNIT) {
        return COMPILER_TYPE_ERROR;
    }

    // bool logical operands
    if (strcmp(expressionBinaryNode->text, "&&") == 0 || strcmp(expressionBinaryNode->text, "||") == 0) {
       if (left->inferredType == DATA_TYPE_BOOLEAN && right->inferredType == DATA_TYPE_BOOLEAN) {
            expressionBinaryNode->inferredType = DATA_TYPE_BOOLEAN;
       } else {
            return COMPILER_TYPE_ERROR;
       }
    }

    // only numbers expression
    else if (expressionBinaryNode->text[0] == '-' || expressionBinaryNode->text[0] == '*' || expressionBinaryNode->text[0] == '/') {
        if (left->inferredType == DATA_TYPE_STRING || right->inferredType == DATA_TYPE_STRING) {
            return COMPILER_TYPE_ERROR;
        } else if (left->inferredType == DATA_TYPE_INT && right->inferredType == DATA_TYPE_INT){
            expressionBinaryNode->inferredType = DATA_TYPE_INT;
        } else{
            expressionBinaryNode->inferredType = DATA_TYPE_DOUBLE;
        }
    }

    // numbers + concatenation
    else if (expressionBinaryNode->text[0] == '+') {
        if (left->inferredType == right->inferredType) {
            expressionBinaryNode->inferredType = left->inferredType;
        } else if (left->inferredType == DATA_TYPE_STRING || right->inferredType == DATA_TYPE_STRING){
            return COMPILER_TYPE_ERROR;
        } else{
            expressionBinaryNode->inferredType = DATA_TYPE_DOUBLE;
        }
    }

    // comparison
    else{
        if (left->inferredType == right->inferredType) { // comparing the same
            if ((strcmp(expressionBinaryNode->text, "==") != 0 && strcmp(expressionBinaryNode->text, "!=") != 0)
            && left->inferredType == DATA_TYPE_BOOLEAN) {
                return COMPILER_TYPE_ERROR; // comparing booleans < > <= >=
            }
            expressionBinaryNode->inferredType = DATA_TYPE_BOOLEAN;
        } else if (left->inferredType == DATA_TYPE_STRING || right->inferredType == DATA_TYPE_STRING
        || right->inferredType == DATA_TYPE_BOOLEAN || left->inferredType == DATA_TYPE_BOOLEAN) {
            return COMPILER_TYPE_ERROR; // comparing string or bool with something else
        } else{
            expressionBinaryNode->inferredType = DATA_TYPE_BOOLEAN; // comparing ints and doubles
        }
    }

    return COMPILER_OK;
}

CompilerStatus semAnalResolveUnaryExpression(AbstractSyntaxNode *unaryExpressionNode, SemAnalData *data) {
    if (unaryExpressionNode == NULL || data == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    AbstractSyntaxNode *operand = unaryExpressionNode->children[0];
    CompilerStatus status = semAnalResolveNode(operand, data);
    if (status != COMPILER_OK) {
        return status;
    }

    // unary operator int or double or bool only
    if (operand->inferredType != DATA_TYPE_INT && operand->inferredType != DATA_TYPE_DOUBLE && operand->inferredType != DATA_TYPE_BOOLEAN) {
        return COMPILER_TYPE_ERROR;
    }

    // non bool type for bool unary operation
    if (unaryExpressionNode->text[0] == '!' && operand->inferredType != DATA_TYPE_BOOLEAN) {
        return COMPILER_TYPE_ERROR;
    }

    // bool type for non bool unary operations
    if (unaryExpressionNode->text[0] != '!' && operand->inferredType == DATA_TYPE_BOOLEAN) {
        return COMPILER_TYPE_ERROR;
    }

    unaryExpressionNode->inferredType = operand->inferredType;

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

    identifierNode->inferredType = symbol->dataType;
    identifierNode->variableId = symbol->variableId;

    return COMPILER_OK;
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
        __attribute__ ((fallthrough));\
    case 2:\
        tmpTypes[1] = (param2);\
        __attribute__ ((fallthrough));\
    case 1:\
        tmpTypes[0] = (param1);\
        __attribute__ ((fallthrough));\
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

    BUILTIN_FUN_MACRO("readBoolean", 0, DATA_TYPE_BOOLEAN, 0, 0, 0);

    return COMPILER_OK;
}
