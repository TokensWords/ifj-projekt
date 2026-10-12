/**
 * @file  parserLiterals.c
 *
 * @project IFJ26 compiler
 * @author Matúš Čík
 */

#include "parserLiterals.h"
#include "syntaxAnalyzerUtils.h"
#include "syntax.h"

/* Shared parsing for declared types in signatures and declarations. */
CompilerStatus analyzerTypesParseDataType(SyntaxAnalyzer *syntaxAnalyzer, DataType *outType)
{
    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_INT)) {
        *outType = DATA_TYPE_INT;
    } else if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_DOUBLE)) {
        *outType = DATA_TYPE_DOUBLE;
    } else if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_STRING)) {
        *outType = DATA_TYPE_STRING;
    } else if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_UNIT)) {
        *outType = DATA_TYPE_UNIT;
    } else {
        return COMPILER_SYNTAX_ERROR;
    }
    return advanceToken(syntaxAnalyzer);
}
