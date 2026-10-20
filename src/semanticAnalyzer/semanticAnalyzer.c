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
#include "memory.h"

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


/**
 * @brief gets parameter types from AST node
 *
 * @param node pointer to AST function node to get parameter types from
 *
 * @return array of data types
 */
DataType *astGetParameterTypes(AbstractSyntaxNode *node)
{
		if(node == NULL) {
				return NULL;
		}
		if(node->type != ABSTRACT_SYNTAX_FUNCTION) {
				return NULL;
		}
		
		DataType *types = memoryAllocate(sizeof(DataType) * (node->childCount - 1));
		if(types == NULL) {
				return NULL;
		}

		for(size_t parameterI = 0; parameterI < node->childCount - 1; parameterI++) {
				if(node->children[parameterI] == NULL) {
						free(types);
						return NULL;
				}
				types[parameterI] = node->children[parameterI]->variableType;
		}

		return types;
}

/**
 * @brief Extracts information from current ast node to a new symbol.
 *
 * @param node node to be extracted from
 *
 * @return status of the program - error codes
 */
Symbol *astNodeToSymbol(AbstractSyntaxNode *node)
{
		if(node == NULL) {
				return NULL
		}

		Symbol *symbol = memoryAllocate(sizeof(Symbol));
		if(symbol == NULL) {
				return NULL;
		}

		//resolve kind
		if(node->kind == ABSTRACT_SYNTAX_FUNCTION) {
				symbol->kind = SYMBOL_FUNCTION;
		} else if(node->kind == ABSTRACT_SYNTAX_PARAMETER || node->kind == ABSTRACT_SYNTAX_VARIABLE_DECLARATION) {
				symbol->kind = SYMBOL_VARIABLE;
		} else{
				free(symbol);
				return NULL;
		}

		symbol->name = node->text;
		symbol->dataType = node->variableType;
		symbol->mutable = node->mutable;
		symbol->defined = false;
		symbol->scopeDepth = 0;

		if(symbol->kind == ABSTRACT_SYNTAX_FUNCTION) {
				symbol->function->parameterTypes = astGetParameterTypes(node);
				if(symbol->function->parameterTypes == NULL) {
						free(symbol);
						return NULL;
				}
				symbol->function->parameterCount = node->variableCount;
				symbol->function.returnType = variableType;
		} else{
				symbol->function->parameterTypes = NULL;
				symbol->function->parameterCount = 0;
				symbol->function.returnType = DATA_TYPE_UNKNOWN;
		}

		return symbol;
}


/**
 * @brief TODO
 *
 *
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveFunction(AbstractSyntaxNode *functionNode, SymbolTable *symbols)
{
    if(functionNode == NULL || symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    CompilerStatus status = COMPILER_OK;

		Symbol *functionSymbol = astNodeToSymbol(functionNode);

		status = symbolTableInsert(symbols, functionSymbol);
		if(status != COMPILER_OK) {
				return status;
		}
    
    return status;
}


CompilerStatus semAnalResolveParameter(AbstractSyntaxNode *parameterNode, SymbolTable *symbols)
{
    if(parameterNode == NULL || symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    CompilerStatus status = COMPILER_OK;

    //TODO
    
    return status;
}


CompilerStatus semAnalResolveBlock(AbstractSyntaxNode *blockNode, SymbolTable *symbols)
{
    if(blockNode == NULL || symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    CompilerStatus status = COMPILER_OK;

    //TODO
    
    return status;
}


CompilerStatus semAnalResolveIf(AbstractSyntaxNode *ifNode, SymbolTable *symbols)
{
    if(ifNode == NULL || symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    CompilerStatus status = COMPILER_OK;

    //TODO
    
    return status;
}


CompilerStatus semAnalResolveWhile(AbstractSyntaxNode *whileNode, SymbolTable *symbols)
{
    if(whileNode == NULL || symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    CompilerStatus status = COMPILER_OK;

    //TODO
    
    return status;
}


CompilerStatus semAnalResolveReturn(AbstractSyntaxNode *returnNode, SymbolTable *symbols)
{
    if(returnNode == NULL || symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    CompilerStatus status = COMPILER_OK;

    //TODO
    
    return status;
}


CompilerStatus semAnalResolveFunctionCall(AbstractSyntaxNode *functionCallNode, SymbolTable *symbols)
{
    if(functionCallNode == NULL || symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    CompilerStatus status = COMPILER_OK;

    //TODO
    
    return status;
}


CompilerStatus semAnalResolveVariableDeclaration(AbstractSyntaxNode *variableDeclarationNode, SymbolTable *symbols)
{
    if(variableDeclarationNode == NULL || symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    CompilerStatus status = COMPILER_OK;

    //TODO
    
    return status;
}


CompilerStatus semAnalResolveAssignment(AbstractSyntaxNode *assignmentNode, SymbolTable *symbols)
{
    if(assignmentNode == NULL || symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    CompilerStatus status = COMPILER_OK;

    //TODO
    
    return status;
}


CompilerStatus semAnalResolveExpressionStatement(AbstractSyntaxNode *expressionStatementNode, SymbolTable *symbols)
{
    if(expressionStatementNode == NULL || symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    CompilerStatus status = COMPILER_OK;

    //TODO
    
    return status;
}


CompilerStatus semAnalResolveExpressionBinary(AbstractSyntaxNode *expressionBinaryNode, SymbolTable *symbols)
{
    if(expressionBinaryNode == NULL || symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    CompilerStatus status = COMPILER_OK;

    //TODO
    
    return status;
}


CompilerStatus semAnalResolveUnaryExpression(AbstractSyntaxNode *unaryExpressionNode, SymbolTable *symbols)
{
    if(unaryExpressionNode == NULL || symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    CompilerStatus status = COMPILER_OK;

    //TODO
    
    return status;
}


CompilerStatus semAnalResolveIdentifier(AbstractSyntaxNode *identifierNode, SymbolTable *symbols)
{
    if(identifierNode == NULL || symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    CompilerStatus status = COMPILER_OK;

    //TODO
    
    return status;
}


CompilerStatus semAnalResolveIntLiteral(AbstractSyntaxNode *intLiteralNode, SymbolTable *symbols)
{
    if(intLiteralNode == NULL || symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    CompilerStatus status = COMPILER_OK;

    //TODO
    
    return status;
}


CompilerStatus semAnalResolveDoubleLiteral(AbstractSyntaxNode *doubleLiteralNode, SymbolTable *symbols)
{
    if(doubleLiteralNode == NULL || symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    CompilerStatus status = COMPILER_OK;

    //TODO
    
    return status;
}


CompilerStatus semAnalResolveStringLiteral(AbstractSyntaxNode *stringLiteralNode, SymbolTable *symbols)
{
    if(stringLiteralNode == NULL || symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    CompilerStatus status = COMPILER_OK;

    //TODO
    
    return status;
}


CompilerStatus semAnalResolveBooleanLiteral(AbstractSyntaxNode *booleanLiteralNode, SymbolTable *symbols)
{
    if(booleanLiteralNode == NULL || symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    CompilerStatus status = COMPILER_OK;

    //TODO
    
    return status;
}


CompilerStatus semAnalResolveUnitLiteral(AbstractSyntaxNode *unitLiteralNode, SymbolTable *symbols)
{
    if(unitLiteralNode == NULL || symbols == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    CompilerStatus status = COMPILER_OK;

    //TODO
    
    return status;
}
