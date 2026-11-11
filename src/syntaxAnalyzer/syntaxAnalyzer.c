/**
 * @file  syntaxAnalyzer.c
 *
 * @brief Implements the syntax analyzer (parser) interface and lifecycle.
 *
 * @project IFJ26 compiler
 * @author Matúš Čík
 */

#include "syntaxAnalyzer.h"
#include "parserFunctions.h"
#include "syntax.h"
#include "syntaxAnalyzerUtils.h"

#include "memory.h"

#include <stdlib.h>
#include <string.h>

/* --- Lifecycle --- */

SyntaxAnalyzer *syntaxAnalyzerCreate(LexicalAnalyzer *lexicalAnalyzer) {
    if (lexicalAnalyzer == NULL) {
        return NULL;
    }

    SyntaxAnalyzer *syntaxAnalyzer = memoryAllocate(sizeof(SyntaxAnalyzer));
    if (syntaxAnalyzer == NULL) {
        return NULL;
    }

    syntaxAnalyzer->lexicalAnalyzer = lexicalAnalyzer;
    syntaxAnalyzer->currentToken = NULL;
    syntaxAnalyzer->nextToken = NULL;
    syntaxAnalyzer->newlineConsumed = false;
    return syntaxAnalyzer;
}

void syntaxAnalyzerDestroy(SyntaxAnalyzer *syntaxAnalyzer) {
    if (syntaxAnalyzer == NULL) {
        return;
    }

    tokenDestroy(syntaxAnalyzer->currentToken);
    tokenDestroy(syntaxAnalyzer->nextToken);
    free(syntaxAnalyzer);
}

static CompilerStatus parsePrologue(SyntaxAnalyzer *syntaxAnalyzer) {
    CompilerStatus status = expectWord(syntaxAnalyzer, SYNTAX_KW_IMPORT);
    if (status != COMPILER_OK) {
        return status;
    }

    status = expectWord(syntaxAnalyzer, SYNTAX_PROLOG_NAME);
    if (status != COMPILER_OK) {
        return status;
    }
    status = expectType(syntaxAnalyzer, TOKEN_DOT);
    if (status != COMPILER_OK) {
        return status;
    }

    if (!currentIsOperator(syntaxAnalyzer, SYNTAX_WILDCARD)) {
        return COMPILER_SYNTAX_ERROR;
    }

    status = advanceToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }

    if (!currentIsType(syntaxAnalyzer, TOKEN_NEWLINE)) {
        return COMPILER_SYNTAX_ERROR;
    }

    return COMPILER_OK;
}

/* --- Main parsing function --- */

CompilerStatus syntaxAnalyzerParseProgram(SyntaxAnalyzer *syntaxAnalyzer,
                                          SourceProgram **outProgram) {
    if (syntaxAnalyzer == NULL || outProgram == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    *outProgram = NULL;

    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }

    status = skipNewlines(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }

    status = parsePrologue(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }

    SourceProgram *program = sourceProgramCreate();
    if (program == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    program->root = createNode(ABSTRACT_SYNTAX_PROGRAM, syntaxAnalyzer->currentToken->location);
    if (program->root == NULL) {
        sourceProgramDestroy(program);
        return COMPILER_INTERNAL_ERROR;
    }

    bool hasEntryFunction = false;
    while (status == COMPILER_OK && !currentIsType(syntaxAnalyzer, TOKEN_EOF)) {

        if (currentIsType(syntaxAnalyzer, TOKEN_NEWLINE)) {
            status = skipNewlines(syntaxAnalyzer);
            continue;
        }

        AbstractSyntaxNode *functionNode = NULL;
        status = parseFunction(syntaxAnalyzer, &functionNode);
        if (status != COMPILER_OK) {
            abstractSyntaxNodeDestroy(functionNode);
            break;
        }

        if (functionNode->isEntryFunction) {
            if (hasEntryFunction) {
                abstractSyntaxNodeDestroy(functionNode);
                status = COMPILER_REDEFINITION_ERROR;
                break;
            }
            hasEntryFunction = true;
        }

        status = appendOwnedChild(program->root, functionNode);
        if (status != COMPILER_OK) {
            break;
        }
    }

    if (status == COMPILER_OK && !hasEntryFunction) {
        status = COMPILER_UNDEFINED_SYMBOL_ERROR;
    }
    if (status != COMPILER_OK) {
        sourceProgramDestroy(program);
        return status;
    }

    *outProgram = program;

    return COMPILER_OK;
}
