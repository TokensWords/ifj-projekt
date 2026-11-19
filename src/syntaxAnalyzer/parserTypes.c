/**
 * @file  parserTypes.c
 *
 * @project IFJ26 compiler
 * @author Matúš Čík xcikmat00
 */

#include "parserTypes.h"

#include "syntax.h"
#include "syntaxAnalyzerUtils.h"
#include "typeDescriptor.h"

#include <string.h>

static CompilerStatus parseTypeExpression(SyntaxAnalyzer *syntaxAnalyzer, TypeDescriptor *outType);

static CompilerStatus parseTypePrimary(SyntaxAnalyzer *syntaxAnalyzer, TypeDescriptor *outType) {
    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    typeDescriptorInit(outType);

    if (currentIsType(syntaxAnalyzer, TOKEN_LEFT_PAREN)) {
        status = advanceToken(syntaxAnalyzer);
        if (status != COMPILER_OK) {
            return status;
        }
        status = parseTypeExpression(syntaxAnalyzer, outType);
        if (status != COMPILER_OK) {
            return status;
        }
        status = expectType(syntaxAnalyzer, TOKEN_RIGHT_PAREN);
        if (status != COMPILER_OK) {
            typeDescriptorFree(outType);
        }
        return status;
    }

    if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_LIST)) {
        status = advanceToken(syntaxAnalyzer);
        if (status != COMPILER_OK) {
            return status;
        }
        status = expectType(syntaxAnalyzer, TOKEN_LEFT_SHARP_PAREN);
        if (status != COMPILER_OK) {
            return status;
        }
        TypeDescriptor elementType;
        typeDescriptorInit(&elementType);
        status = parseTypeExpression(syntaxAnalyzer, &elementType);
        if (status != COMPILER_OK) {
            typeDescriptorFree(&elementType);
            return status;
        }
        TypeDescriptorRecursiveNode *recursiveNode = NULL;
        status = typeDescriptorRecursiveNodeInit(&recursiveNode, &elementType);
        typeDescriptorFree(&elementType);
        if (status == COMPILER_OK) {
            status = typeDescriptorAddRecursiveType(outType, recursiveNode);
        }
        if (status != COMPILER_OK) {
            typeDescriptorRecursiveNodeFree(recursiveNode);
        }
        if (status != COMPILER_OK) {
            return status;
        }
        status = expectType(syntaxAnalyzer, TOKEN_RIGHT_SHARP_PAREN);
        if (status != COMPILER_OK) {
            typeDescriptorFree(outType);
            return status;
        }
        return COMPILER_OK;
    }

    DataType basicType = DATA_TYPE_UNKNOWN;
    if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_INT)) {
        basicType = DATA_TYPE_INT;
    } else if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_DOUBLE)) {
        basicType = DATA_TYPE_DOUBLE;
    } else if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_STRING)) {
        basicType = DATA_TYPE_STRING;
    } else if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_UNIT)) {
        basicType = DATA_TYPE_UNIT;
    } else if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_BOOLEAN)) {
        basicType = DATA_TYPE_BOOLEAN;
    } else if (isIdentifierToken(syntaxAnalyzer->currentToken)) {
        TypeDescriptorAliasNode *aliasNode = NULL;
        status = typeDescriptorAliasNodeInit(&aliasNode, syntaxAnalyzer->currentToken->lexeme);
        if (status == COMPILER_OK) {
            status = typeDescriptorAddAlias(outType, aliasNode);
        }
        if (status != COMPILER_OK) {
            typeDescriptorAliasNodeFree(aliasNode);
            return status;
        }
    } else {
        return COMPILER_SYNTAX_ERROR;
    }
    if (basicType != DATA_TYPE_UNKNOWN) {
        status = typeDescriptorAddLiteralType(outType, basicType);
        if (status != COMPILER_OK) {
            return status;
        }
    }
    return advanceToken(syntaxAnalyzer);
}

static CompilerStatus parseTypeExpression(SyntaxAnalyzer *syntaxAnalyzer, TypeDescriptor *outType) {
    TypeDescriptor first;
    typeDescriptorInit(&first);
    CompilerStatus status = parseTypePrimary(syntaxAnalyzer, &first);
    if (status != COMPILER_OK) {
        typeDescriptorFree(&first);
        return status;
    }
    if (!currentIsType(syntaxAnalyzer, TOKEN_OPERATOR) ||
        !tokenHasText(syntaxAnalyzer->currentToken, "|")) {
        *outType = first;
        return COMPILER_OK;
    }

    TypeDescriptor unionType;
    typeDescriptorInit(&unionType);
    unionType.datatypeBitflag = first.datatypeBitflag;
    while (first.aliases != NULL) {
        TypeDescriptorAliasNode *node = typeDescriptorPopAlias(&first);
        status = typeDescriptorAddAlias(&unionType, node);
        if (status != COMPILER_OK) {
            typeDescriptorAliasNodeFree(node);
            break;
        }
    }
    while (status == COMPILER_OK && first.classes != NULL) {
        TypeDescriptorAliasNode *node = typeDescriptorPopClass(&first);
        status = typeDescriptorAddClass(&unionType, node);
        if (status != COMPILER_OK) {
            typeDescriptorAliasNodeFree(node);
            break;
        }
    }
    while (status == COMPILER_OK && first.recursiveTypes != NULL) {
        TypeDescriptorRecursiveNode *node = typeDescriptorPopRecursiveType(&first);
        status = typeDescriptorAddRecursiveType(&unionType, node);
        if (status != COMPILER_OK) {
            typeDescriptorRecursiveNodeFree(node);
            break;
        }
    }
    typeDescriptorFree(&first);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    while (currentIsType(syntaxAnalyzer, TOKEN_OPERATOR) &&
           tokenHasText(syntaxAnalyzer->currentToken, "|")) {
        status = advanceToken(syntaxAnalyzer);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        TypeDescriptor member;
        typeDescriptorInit(&member);
        status = parseTypePrimary(syntaxAnalyzer, &member);
        if (status != COMPILER_OK) {
            typeDescriptorFree(&member);
            goto cleanup;
        }
        unionType.datatypeBitflag |= member.datatypeBitflag;
        while (status == COMPILER_OK && member.aliases != NULL) {
            TypeDescriptorAliasNode *node = typeDescriptorPopAlias(&member);
            status = typeDescriptorAddAlias(&unionType, node);
            if (status != COMPILER_OK) {
                typeDescriptorAliasNodeFree(node);
                break;
            }
        }
        while (status == COMPILER_OK && member.classes != NULL) {
            TypeDescriptorAliasNode *node = typeDescriptorPopClass(&member);
            status = typeDescriptorAddClass(&unionType, node);
            if (status != COMPILER_OK) {
                typeDescriptorAliasNodeFree(node);
                break;
            }
        }
        while (status == COMPILER_OK && member.recursiveTypes != NULL) {
            TypeDescriptorRecursiveNode *node = typeDescriptorPopRecursiveType(&member);
            status = typeDescriptorAddRecursiveType(&unionType, node);
            if (status != COMPILER_OK) {
                typeDescriptorRecursiveNodeFree(node);
                break;
            }
        }
        typeDescriptorFree(&member);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
    }

    *outType = unionType;
    return COMPILER_OK;

cleanup:
    typeDescriptorFree(&unionType);
    return status;
}

CompilerStatus analyzerTypesParseType(SyntaxAnalyzer *syntaxAnalyzer, TypeDescriptor *outType) {
    if (syntaxAnalyzer == NULL || outType == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    typeDescriptorFree(outType);
    CompilerStatus status = parseTypeExpression(syntaxAnalyzer, outType);
    if (status != COMPILER_OK) {
        typeDescriptorFree(outType);
    }
    return status;
}

CompilerStatus analyzerTypesParseAlias(SyntaxAnalyzer *syntaxAnalyzer,
                                       AbstractSyntaxNode **outNode) {
    if (syntaxAnalyzer == NULL || outNode == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    *outNode = NULL;

    CompilerStatus status = expectWord(syntaxAnalyzer, SYNTAX_KW_TYPE);
    if (status != COMPILER_OK) {
        return status;
    }
    if (!isIdentifierToken(syntaxAnalyzer->currentToken)) {
        return COMPILER_SYNTAX_ERROR;
    }

    AbstractSyntaxNode *alias =
        createNode(ABSTRACT_SYNTAX_TYPE_ALIAS, syntaxAnalyzer->currentToken->location);
    if (alias == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    status = setNodeText(alias, syntaxAnalyzer->currentToken->lexeme);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = advanceToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = expectWord(syntaxAnalyzer, "=");
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = analyzerTypesParseType(syntaxAnalyzer, &alias->type);
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    *outNode = alias;
    return COMPILER_OK;

cleanup:
    abstractSyntaxNodeDestroy(alias);
    return status;
}

CompilerStatus analyzerTypesParseCaseClass(SyntaxAnalyzer *syntaxAnalyzer,
                                           AbstractSyntaxNode **outNode) {
    if (syntaxAnalyzer == NULL || outNode == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    *outNode = NULL;

    CompilerStatus status = expectWord(syntaxAnalyzer, SYNTAX_KW_CASE);
    if (status != COMPILER_OK) {
        return status;
    }
    status = expectWord(syntaxAnalyzer, SYNTAX_KW_CLASS);
    if (status != COMPILER_OK) {
        return status;
    }
    if (!isIdentifierToken(syntaxAnalyzer->currentToken)) {
        return COMPILER_SYNTAX_ERROR;
    }

    AbstractSyntaxNode *caseClass =
        createNode(ABSTRACT_SYNTAX_CASE_CLASS, syntaxAnalyzer->currentToken->location);
    if (caseClass == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    AbstractSyntaxNode *field = NULL;
    status = setNodeText(caseClass, syntaxAnalyzer->currentToken->lexeme);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = advanceToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = expectType(syntaxAnalyzer, TOKEN_LEFT_PAREN);
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    while (!currentIsType(syntaxAnalyzer, TOKEN_RIGHT_PAREN)) {
        if (!isIdentifierToken(syntaxAnalyzer->currentToken)) {
            status = COMPILER_SYNTAX_ERROR;
            goto cleanup;
        }

        field =
            createNode(ABSTRACT_SYNTAX_CASE_CLASS_FIELD, syntaxAnalyzer->currentToken->location);
        if (field == NULL) {
            status = COMPILER_INTERNAL_ERROR;
            goto cleanup;
        }
        status = setNodeText(field, syntaxAnalyzer->currentToken->lexeme);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        status = advanceToken(syntaxAnalyzer);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        status = expectType(syntaxAnalyzer, TOKEN_COLON);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        status = analyzerTypesParseType(syntaxAnalyzer, &field->type);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        status = appendOwnedChild(caseClass, field);
        field = NULL;
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        if (currentIsType(syntaxAnalyzer, TOKEN_RIGHT_PAREN)) {
            break;
        }
        status = expectType(syntaxAnalyzer, TOKEN_COMMA);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
    }

    status = expectType(syntaxAnalyzer, TOKEN_RIGHT_PAREN);
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    *outNode = caseClass;
    return COMPILER_OK;

cleanup:
    abstractSyntaxNodeDestroy(field);
    abstractSyntaxNodeDestroy(caseClass);
    return status;
}
