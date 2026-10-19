/*
 * Project: IFJ26 compiler
 * Authors: Václav Krytinář xkrytiv00
 * AI-assisted scaffold.
 * Prompt: Create a semantic-analysis integration stub.
 */

#include "semanticAnalyzer.h"
#include "abstractSyntaxTree.h"
#include "compilerTypes.h"
#include "symbolTable.h"
#include <stdbool.h>

/**
 * @brief goes through functions in AST and checks semantics
 *
 * @param program sorce program AST got from syntax analyser
 * @param symbols symbol table to work with
 *
 * @return status of the program - error codes
 */
CompilerStatus semanticAnalyzerAnalyze(SourceProgram *program, SymbolTable *symbols)
{
    if (program == NULL || symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

		if(program->root == NULL) {
				return COMPILER_UNDEFINED_SYMBOL_ERROR;		//no source code (main)
		}

		if(program->root->type != ABSTRACT_SYNTAX_PROGRAM) {
				return COMPILER_INTERNAL_ERROR;
		}

		CompilerStatus status = COMPILER_OK;
		AbstractSyntaxNode root = program->root;
		bool hasMain = false;

		//loop thorough functins
		for(size_t currentFunI = 0; currentFunI < root->childCount; currentFunI++) {
				if(hasMain && root->children[currentFunI]->isEntryFunction) {
						return COMPILER_REDEFINITION_ERROR;		//multiple mains
				}
				hasMain = hasMain || root->children[currentFunI]->isEntryFunction;

				status = semAnalResolveFunction(root->children[currentFunI]);
				if(status != COMPILER_OK) {
						return status;
				}
		}

		if(!hasMain) status = COMPILER_UNDEFINED_SYMBOL_ERROR;

    return status;
}
