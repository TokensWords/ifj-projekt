/**
 * @file  parserTypes.c
 *
 * @project IFJ26 compiler
 * @author Matúš Čík
 */

#include "parserTypes.h"

#include "syntax.h"
#include "syntaxAnalyzerUtils.h"
#include "typeDescriptor.h"

#include <string.h>

static CompilerStatus appendUnionMember(TypeDescriptor *unionType, TypeDescriptor *member) {
    for (size_t index = 0; index < typeDescriptorGetMemberCount(unionType); index++) {
        if (typeDescriptorEquals(typeDescriptorGetMember(unionType, index), member)) {
            typeDescriptorDestroy(member);
            return COMPILER_OK;
        }
    }

    if (typeDescriptorGetMemberCount(member) == 0) {
        CompilerStatus status = typeDescriptorAddMember(unionType, member);
        if (status != COMPILER_OK) {
            typeDescriptorDestroy(member);
        }
        return status;
    }

    while (typeDescriptorGetMemberCount(member) > 0) {
        TypeDescriptor *nestedMember = typeDescriptorRemoveMember(member, 0);
        if (nestedMember == NULL) {
            typeDescriptorDestroy(member);
            return COMPILER_INTERNAL_ERROR;
        }
        CompilerStatus status = appendUnionMember(unionType, nestedMember);
        if (status != COMPILER_OK) {
            typeDescriptorDestroy(member);
            return status;
        }
    }
    typeDescriptorDestroy(member);
    return COMPILER_OK;
}

static CompilerStatus parseTypeExpression(SyntaxAnalyzer *syntaxAnalyzer, TypeDescriptor **outType);

static CompilerStatus parseTypePrimary(SyntaxAnalyzer *syntaxAnalyzer, TypeDescriptor **outType) {
    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    *outType = NULL;

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
            typeDescriptorDestroy(*outType);
            *outType = NULL;
        }
        return status;
    }

    TypeDescriptor *type = NULL;
    TypeDescriptor *elementType = NULL;
    if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_LIST)) {
        type = typeDescriptorCreate(DATA_TYPE_LIST);
        if (type == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }
        status = advanceToken(syntaxAnalyzer);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        status = expectType(syntaxAnalyzer, TOKEN_LEFT_SHARP_PAREN);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        status = parseTypeExpression(syntaxAnalyzer, &elementType);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        status = typeDescriptorSetElementType(type, elementType);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        elementType = NULL;
        status = expectType(syntaxAnalyzer, TOKEN_RIGHT_SHARP_PAREN);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
    } else {
        type = typeDescriptorCreate(DATA_TYPE_UNKNOWN);
        if (type == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }
        if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_INT)) {
            status = typeDescriptorSetMask(type, DATA_TYPE_INT);
        } else if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_DOUBLE)) {
            status = typeDescriptorSetMask(type, DATA_TYPE_DOUBLE);
        } else if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_STRING)) {
            status = typeDescriptorSetMask(type, DATA_TYPE_STRING);
        } else if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_UNIT)) {
            status = typeDescriptorSetMask(type, DATA_TYPE_UNIT);
        } else if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_BOOLEAN)) {
            status = typeDescriptorSetMask(type, DATA_TYPE_BOOLEAN);
        } else if (isIdentifierToken(syntaxAnalyzer->currentToken)) {
            status = typeDescriptorSetMask(type, DATA_TYPE_ALIAS);
            if (status != COMPILER_OK) {
                goto cleanup;
            }
            status = typeDescriptorSetName(type, syntaxAnalyzer->currentToken->lexeme);
            if (status != COMPILER_OK) {
                goto cleanup;
            }
        } else {
            status = COMPILER_SYNTAX_ERROR;
            goto cleanup;
        }
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        status = advanceToken(syntaxAnalyzer);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
    }

    *outType = type;
    return COMPILER_OK;

cleanup:
    typeDescriptorDestroy(elementType);
    typeDescriptorDestroy(type);
    return status;
}

static CompilerStatus parseTypeExpression(SyntaxAnalyzer *syntaxAnalyzer,
                                          TypeDescriptor **outType) {
    TypeDescriptor *first = NULL;
    CompilerStatus status = parseTypePrimary(syntaxAnalyzer, &first);
    if (status != COMPILER_OK) {
        return status;
    }
    if (!currentIsType(syntaxAnalyzer, TOKEN_OPERATOR) ||
        !tokenHasText(syntaxAnalyzer->currentToken, "|")) {
        *outType = first;
        return COMPILER_OK;
    }

    TypeDescriptor *unionType = typeDescriptorCreate(DATA_TYPE_UNKNOWN);
    if (unionType == NULL) {
        typeDescriptorDestroy(first);
        return COMPILER_INTERNAL_ERROR;
    }
    status = appendUnionMember(unionType, first);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    while (currentIsType(syntaxAnalyzer, TOKEN_OPERATOR) &&
           tokenHasText(syntaxAnalyzer->currentToken, "|")) {
        status = advanceToken(syntaxAnalyzer);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        TypeDescriptor *member = NULL;
        status = parseTypePrimary(syntaxAnalyzer, &member);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        status = appendUnionMember(unionType, member);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
    }

    if (typeDescriptorGetMemberCount(unionType) == 1) {
        *outType = typeDescriptorRemoveMember(unionType, 0);
        typeDescriptorDestroy(unionType);
    } else {
        *outType = unionType;
    }
    return COMPILER_OK;

cleanup:
    typeDescriptorDestroy(unionType);
    return status;
}

CompilerStatus analyzerTypesParseType(SyntaxAnalyzer *syntaxAnalyzer, TypeDescriptor **outType) {
    if (syntaxAnalyzer == NULL || outType == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    *outType = NULL;
    return parseTypeExpression(syntaxAnalyzer, outType);
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
    status = analyzerTypesParseType(syntaxAnalyzer, &alias->variableType);
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
        status = analyzerTypesParseType(syntaxAnalyzer, &field->variableType);
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
