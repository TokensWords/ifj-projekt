/**
 * @file  parserLiterals.c
 *
 * @project IFJ26 compiler
 * @author Matúš Čík
 */

#include "parserLiterals.h"
#include "syntax.h"
#include "syntaxAnalyzerUtils.h"

/* Shared recursive type parsing for declarations and function signatures. */
CompilerStatus analyzerTypesParseType(SyntaxAnalyzer *syntaxAnalyzer, TypeDescriptor **outType) {
    if (syntaxAnalyzer == NULL || outType == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    *outType = NULL;
    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_LIST)) {
        TypeDescriptor *listType = NULL;
        TypeDescriptor *elementType = NULL;
        status = advanceToken(syntaxAnalyzer);
        if (status != COMPILER_OK) {
            goto listCleanup;
        }
        status = expectType(syntaxAnalyzer, TOKEN_LEFT_SHARP_PAREN);
        if (status != COMPILER_OK) {
            goto listCleanup;
        }
        status = analyzerTypesParseType(syntaxAnalyzer, &elementType);
        if (status != COMPILER_OK) {
            goto listCleanup;
        }
        status = expectType(syntaxAnalyzer, TOKEN_RIGHT_SHARP_PAREN);
        if (status != COMPILER_OK) {
            goto listCleanup;
        }
        listType = abstractSyntaxTypeCreate(DATA_TYPE_LIST);
        if (listType == NULL) {
            status = COMPILER_INTERNAL_ERROR;
            goto listCleanup;
        }
        listType->elementType = elementType;
        elementType = NULL;
        *outType = listType;
        return COMPILER_OK;

    listCleanup:
        abstractSyntaxTypeDestroy(elementType);
        abstractSyntaxTypeDestroy(listType);
        return status;
    }

    DataType typeKind;
    if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_INT)) {
        typeKind = DATA_TYPE_INT;
    } else if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_DOUBLE)) {
        typeKind = DATA_TYPE_DOUBLE;
    } else if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_STRING)) {
        typeKind = DATA_TYPE_STRING;
    } else if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_UNIT)) {
        typeKind = DATA_TYPE_UNIT;
    } else if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_BOOLEAN)) {
        typeKind = DATA_TYPE_BOOLEAN;
    } else {
        return COMPILER_SYNTAX_ERROR;
    }
    TypeDescriptor *type = abstractSyntaxTypeCreate(typeKind);
    if (type == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    status = advanceToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        abstractSyntaxTypeDestroy(type);
        return status;
    }
    *outType = type;
    return COMPILER_OK;
}
