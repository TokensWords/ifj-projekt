/**
 * @file  parserStatements.c
 *
 * @brief Implements parsing of statements and blocks of statements.
 *
 * @project IFJ26 compiler
 * @author Matúš Čík
 */

#include "parserStatements.h"
#include "parserExpressions.h"
#include "parserTypes.h"
#include "syntax.h"
#include "syntaxAnalyzerUtils.h"

#include <string.h>

static CompilerStatus parseVariableDeclaration(SyntaxAnalyzer *syntaxAnalyzer,
                                               AbstractSyntaxNode **outNode) {
    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    if (outNode == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    *outNode = NULL;

    bool isMutable = currentIsWord(syntaxAnalyzer, SYNTAX_KW_VAR);
    SourceLocation location = syntaxAnalyzer->currentToken->location;
    status = advanceToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    if (!isIdentifierToken(syntaxAnalyzer->currentToken)) {
        return COMPILER_SYNTAX_ERROR;
    }

    AbstractSyntaxNode *declaration = createNode(ABSTRACT_SYNTAX_VARIABLE_DECLARATION, location);
    AbstractSyntaxNode *initializer = NULL;
    if (declaration == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    declaration->isMutable = isMutable;

    status = setNodeText(declaration, syntaxAnalyzer->currentToken->lexeme);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = advanceToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    if (currentIsType(syntaxAnalyzer, TOKEN_COLON)) {
        status = advanceToken(syntaxAnalyzer);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        status = analyzerTypesParseType(syntaxAnalyzer, &declaration->type);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
    }
    status = expectWord(syntaxAnalyzer, SYNTAX_OP_ASSIGN);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = analyzerExpressionsParseValueOrCall(syntaxAnalyzer, &initializer);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = appendOwnedChild(declaration, initializer);
    initializer = NULL;
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    *outNode = declaration;
    return COMPILER_OK;

cleanup:
    abstractSyntaxNodeDestroy(initializer);
    abstractSyntaxNodeDestroy(declaration);
    return status;
}

static CompilerStatus expectBlockHeaderEnd(SyntaxAnalyzer *syntaxAnalyzer) {
    if (!currentIsType(syntaxAnalyzer, TOKEN_NEWLINE)) {
        return COMPILER_SYNTAX_ERROR;
    }
    CompilerStatus status = advanceToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }

    return skipNewlines(syntaxAnalyzer);
}

static CompilerStatus parseIfAtColumn(SyntaxAnalyzer *syntaxAnalyzer, AbstractSyntaxNode **outNode,
                                      size_t elseColumn) {
    if (syntaxAnalyzer == NULL || outNode == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    *outNode = NULL;

    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    SourceLocation location = syntaxAnalyzer->currentToken->location;
    AbstractSyntaxNode *ifNode = createNode(ABSTRACT_SYNTAX_IF, location);
    if (ifNode == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    AbstractSyntaxNode *condition = NULL;
    AbstractSyntaxNode *thenBlock = NULL;
    AbstractSyntaxNode *elseBlock = NULL;
    AbstractSyntaxNode *elseIfNode = NULL;

    status = advanceToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    status = analyzerExpressionsParse(syntaxAnalyzer, &condition);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = appendOwnedChild(ifNode, condition);
    condition = NULL;
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    status = expectWord(syntaxAnalyzer, SYNTAX_KW_THEN);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = expectBlockHeaderEnd(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    status = parseBlock(syntaxAnalyzer, &thenBlock);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = appendOwnedChild(ifNode, thenBlock);
    thenBlock = NULL;
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    status = skipNewlines(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    if (!currentIsWord(syntaxAnalyzer, SYNTAX_KW_ELSE) ||
        syntaxAnalyzer->currentToken->location.column != elseColumn) {
        elseBlock = createNode(ABSTRACT_SYNTAX_BLOCK, location);
        if (elseBlock == NULL) {
            status = COMPILER_INTERNAL_ERROR;
            goto cleanup;
        }
    } else {
        SourceLocation elseLocation = syntaxAnalyzer->currentToken->location;
        status = advanceToken(syntaxAnalyzer);
        if (status != COMPILER_OK) {
            goto cleanup;
        }

        if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_IF)) {
            elseBlock = createNode(ABSTRACT_SYNTAX_BLOCK, elseLocation);
            if (elseBlock == NULL) {
                status = COMPILER_INTERNAL_ERROR;
                goto cleanup;
            }
            status = parseIfAtColumn(syntaxAnalyzer, &elseIfNode, elseColumn);
            if (status != COMPILER_OK) {
                goto cleanup;
            }
            status = appendOwnedChild(elseBlock, elseIfNode);
            elseIfNode = NULL;
        } else {
            status = expectBlockHeaderEnd(syntaxAnalyzer);
            if (status == COMPILER_OK) {
                status = parseBlock(syntaxAnalyzer, &elseBlock);
            }
        }
        if (status != COMPILER_OK) {
            goto cleanup;
        }
    }
    status = appendOwnedChild(ifNode, elseBlock);
    elseBlock = NULL;
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    *outNode = ifNode;
    return COMPILER_OK;

cleanup:
    abstractSyntaxNodeDestroy(condition);
    abstractSyntaxNodeDestroy(thenBlock);
    abstractSyntaxNodeDestroy(elseBlock);
    abstractSyntaxNodeDestroy(elseIfNode);
    abstractSyntaxNodeDestroy(ifNode);
    return status;
}

CompilerStatus parseIf(SyntaxAnalyzer *syntaxAnalyzer, AbstractSyntaxNode **outNode) {
    if (syntaxAnalyzer == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    return parseIfAtColumn(syntaxAnalyzer, outNode, syntaxAnalyzer->currentToken->location.column);
}

CompilerStatus parseWhile(SyntaxAnalyzer *syntaxAnalyzer, AbstractSyntaxNode **outNode) {
    if (syntaxAnalyzer == NULL || outNode == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    *outNode = NULL;

    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    SourceLocation location = syntaxAnalyzer->currentToken->location;
    AbstractSyntaxNode *whileNode = createNode(ABSTRACT_SYNTAX_WHILE, location);
    if (whileNode == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    AbstractSyntaxNode *condition = NULL;
    AbstractSyntaxNode *body = NULL;

    status = advanceToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    status = analyzerExpressionsParse(syntaxAnalyzer, &condition);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = appendOwnedChild(whileNode, condition);
    condition = NULL;
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    status = expectWord(syntaxAnalyzer, SYNTAX_KW_DO);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = expectBlockHeaderEnd(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    status = parseBlock(syntaxAnalyzer, &body);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = appendOwnedChild(whileNode, body);
    body = NULL;
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    *outNode = whileNode;
    return COMPILER_OK;

cleanup:
    abstractSyntaxNodeDestroy(condition);
    abstractSyntaxNodeDestroy(body);
    abstractSyntaxNodeDestroy(whileNode);
    return status;
}

static CompilerStatus parseForGenerator(SyntaxAnalyzer *syntaxAnalyzer,
                                        AbstractSyntaxNode **outNode) {
    if (syntaxAnalyzer == NULL || outNode == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    *outNode = NULL;

    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    if (!isIdentifierToken(syntaxAnalyzer->currentToken)) {
        return COMPILER_SYNTAX_ERROR;
    }

    SourceLocation generatorLocation = syntaxAnalyzer->currentToken->location;
    AbstractSyntaxNode *generator = createNode(ABSTRACT_SYNTAX_FOR_GENERATOR, generatorLocation);
    AbstractSyntaxNode *source = NULL;
    if (generator == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    status = setNodeText(generator, syntaxAnalyzer->currentToken->lexeme);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = advanceToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = expectWord(syntaxAnalyzer, SYNTAX_OP_GENERATOR_ARROW);
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    status = analyzerExpressionsParseForClause(syntaxAnalyzer, &source);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = appendOwnedChild(generator, source);
    source = NULL;
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    *outNode = generator;
    return COMPILER_OK;

cleanup:
    abstractSyntaxNodeDestroy(source);
    abstractSyntaxNodeDestroy(generator);
    return status;
}

static CompilerStatus parseForGuard(SyntaxAnalyzer *syntaxAnalyzer, AbstractSyntaxNode **outNode) {
    if (syntaxAnalyzer == NULL || outNode == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    *outNode = NULL;

    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    AbstractSyntaxNode *guard =
        createNode(ABSTRACT_SYNTAX_FOR_GUARD, syntaxAnalyzer->currentToken->location);
    AbstractSyntaxNode *condition = NULL;
    if (guard == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    status = advanceToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = analyzerExpressionsParseForClause(syntaxAnalyzer, &condition);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = appendOwnedChild(guard, condition);
    condition = NULL;
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    *outNode = guard;
    return COMPILER_OK;

cleanup:
    abstractSyntaxNodeDestroy(condition);
    abstractSyntaxNodeDestroy(guard);
    return status;
}

CompilerStatus parseFor(SyntaxAnalyzer *syntaxAnalyzer, AbstractSyntaxNode **outNode) {
    if (syntaxAnalyzer == NULL || outNode == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    *outNode = NULL;

    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    SourceLocation location = syntaxAnalyzer->currentToken->location;
    AbstractSyntaxNode *forNode = createNode(ABSTRACT_SYNTAX_FOR, location);
    AbstractSyntaxNode *clauses = createNode(ABSTRACT_SYNTAX_FOR_CLAUSES, location);
    AbstractSyntaxNode *body = NULL;
    AbstractSyntaxNode *clause = NULL;
    if (forNode == NULL || clauses == NULL) {
        abstractSyntaxNodeDestroy(forNode);
        abstractSyntaxNodeDestroy(clauses);
        return COMPILER_INTERNAL_ERROR;
    }

    bool headerIndented = false;
    status = advanceToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    if (currentIsType(syntaxAnalyzer, TOKEN_NEWLINE)) {
        status = skipNewlines(syntaxAnalyzer);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        if (currentIsType(syntaxAnalyzer, TOKEN_INDENT)) {
            status = advanceToken(syntaxAnalyzer);
            if (status != COMPILER_OK) {
                goto cleanup;
            }
            headerIndented = true;
        }
    }

    while (!currentIsWord(syntaxAnalyzer, SYNTAX_KW_DO) &&
           !currentIsType(syntaxAnalyzer, TOKEN_EOF)) {
        if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_IF)) {
            status = parseForGuard(syntaxAnalyzer, &clause);
        } else if (isIdentifierToken(syntaxAnalyzer->currentToken)) {
            status = parseForGenerator(syntaxAnalyzer, &clause);
        } else {
            status = COMPILER_SYNTAX_ERROR;
        }
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        status = appendOwnedChild(clauses, clause);
        clause = NULL;
        if (status != COMPILER_OK) {
            goto cleanup;
        }

        if (currentIsType(syntaxAnalyzer, TOKEN_NEWLINE)) {
            status = skipNewlines(syntaxAnalyzer);
            if (status != COMPILER_OK) {
                goto cleanup;
            }
            if (currentIsType(syntaxAnalyzer, TOKEN_INDENT)) {
                status = advanceToken(syntaxAnalyzer);
                if (status != COMPILER_OK) {
                    goto cleanup;
                }
                headerIndented = true;
            } else if (headerIndented && currentIsType(syntaxAnalyzer, TOKEN_DEDENT)) {
                status = advanceToken(syntaxAnalyzer);
                if (status != COMPILER_OK) {
                    goto cleanup;
                }
                headerIndented = false;
            }
        } else if (currentIsType(syntaxAnalyzer, TOKEN_SEMICOLON)) {
            status = advanceToken(syntaxAnalyzer);
            if (status != COMPILER_OK) {
                goto cleanup;
            }
        } else if (!currentIsWord(syntaxAnalyzer, SYNTAX_KW_DO) &&
                   !currentIsWord(syntaxAnalyzer, SYNTAX_KW_IF) &&
                   !isIdentifierToken(syntaxAnalyzer->currentToken)) {
            status = COMPILER_SYNTAX_ERROR;
            goto cleanup;
        }
    }

    if (clauses->childCount == 0) {
        status = COMPILER_SYNTAX_ERROR;
        goto cleanup;
    }
    status = expectWord(syntaxAnalyzer, SYNTAX_KW_DO);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = expectBlockHeaderEnd(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = parseBlock(syntaxAnalyzer, &body);
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    status = appendOwnedChild(forNode, clauses);
    clauses = NULL;
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = appendOwnedChild(forNode, body);
    body = NULL;
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    *outNode = forNode;
    return COMPILER_OK;

cleanup:
    abstractSyntaxNodeDestroy(clause);
    abstractSyntaxNodeDestroy(body);
    abstractSyntaxNodeDestroy(clauses);
    abstractSyntaxNodeDestroy(forNode);
    return status;
}

static CompilerStatus parseReturn(SyntaxAnalyzer *syntaxAnalyzer, AbstractSyntaxNode **outNode) {
    if (syntaxAnalyzer == NULL || outNode == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    *outNode = NULL;

    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    AbstractSyntaxNode *returnNode =
        createNode(ABSTRACT_SYNTAX_RETURN, syntaxAnalyzer->currentToken->location);
    if (returnNode == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    AbstractSyntaxNode *value = NULL;
    status = advanceToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    if (!isExpressionTerminator(syntaxAnalyzer->currentToken)) {
        status = analyzerExpressionsParseValueOrCall(syntaxAnalyzer, &value);
        if (status != COMPILER_OK) {
            goto cleanup;
        }
        status = appendOwnedChild(returnNode, value);
        value = NULL;
        if (status != COMPILER_OK) {
            goto cleanup;
        }
    }

    *outNode = returnNode;
    return COMPILER_OK;

cleanup:
    abstractSyntaxNodeDestroy(value);
    abstractSyntaxNodeDestroy(returnNode);
    return status;
}

CompilerStatus parseAssignment(SyntaxAnalyzer *syntaxAnalyzer, AbstractSyntaxNode **outNode) {
    if (syntaxAnalyzer == NULL || outNode == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    *outNode = NULL;

    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    if (!isIdentifierToken(syntaxAnalyzer->currentToken)) {
        return COMPILER_SYNTAX_ERROR;
    }

    AbstractSyntaxNode *assignment =
        createNode(ABSTRACT_SYNTAX_ASSIGNMENT, syntaxAnalyzer->currentToken->location);
    AbstractSyntaxNode *value = NULL;
    if (assignment == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    status = setNodeText(assignment, syntaxAnalyzer->currentToken->lexeme);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = advanceToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = expectWord(syntaxAnalyzer, SYNTAX_OP_ASSIGN);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = analyzerExpressionsParseValueOrCall(syntaxAnalyzer, &value);
    if (status != COMPILER_OK) {
        goto cleanup;
    }
    status = appendOwnedChild(assignment, value);
    value = NULL;
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    *outNode = assignment;
    return COMPILER_OK;

cleanup:
    abstractSyntaxNodeDestroy(value);
    abstractSyntaxNodeDestroy(assignment);
    return status;
}

CompilerStatus parseStatement(SyntaxAnalyzer *syntaxAnalyzer, AbstractSyntaxNode **outNode) {
    if (syntaxAnalyzer == NULL || outNode == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    *outNode = NULL;

    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }

    if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_VAL) ||
        currentIsWord(syntaxAnalyzer, SYNTAX_KW_VAR)) {
        return parseVariableDeclaration(syntaxAnalyzer, outNode);
    }
    if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_RETURN)) {
        return parseReturn(syntaxAnalyzer, outNode);
    }
    if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_IF)) {
        return parseIf(syntaxAnalyzer, outNode);
    }
    if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_WHILE)) {
        return parseWhile(syntaxAnalyzer, outNode);
    }
    if (currentIsWord(syntaxAnalyzer, SYNTAX_KW_FOR)) {
        return parseFor(syntaxAnalyzer, outNode);
    }
    if (isIdentifierToken(syntaxAnalyzer->currentToken)) {
        status = ensureNextToken(syntaxAnalyzer);
        if (status != COMPILER_OK) {
            return status;
        }
        if (syntaxAnalyzer->nextToken != NULL && syntaxAnalyzer->nextToken->lexeme != NULL &&
            strcmp(syntaxAnalyzer->nextToken->lexeme, SYNTAX_OP_ASSIGN) == 0) {
            return parseAssignment(syntaxAnalyzer, outNode);
        }
    }

    status = analyzerExpressionsParseValueOrCall(syntaxAnalyzer, outNode);
    if (status != COMPILER_OK) {
        return status;
    }
    if (*outNode == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    if ((*outNode)->kind == ABSTRACT_SYNTAX_FUNCTION_CALL) {
        return COMPILER_OK;
    }

    AbstractSyntaxNode *expression = *outNode;
    AbstractSyntaxNode *statement =
        createNode(ABSTRACT_SYNTAX_EXPRESSION_STATEMENT, expression->location);
    if (statement == NULL) {
        abstractSyntaxNodeDestroy(expression);
        *outNode = NULL;
        return COMPILER_INTERNAL_ERROR;
    }
    status = appendOwnedChild(statement, expression);
    if (status != COMPILER_OK) {
        abstractSyntaxNodeDestroy(statement);
        *outNode = NULL;
        return status;
    }
    *outNode = statement;
    return COMPILER_OK;
}

CompilerStatus parseBlock(SyntaxAnalyzer *syntaxAnalyzer, AbstractSyntaxNode **outNode) {
    if (syntaxAnalyzer == NULL || outNode == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    *outNode = NULL;

    CompilerStatus status = expectType(syntaxAnalyzer, TOKEN_INDENT);
    if (status != COMPILER_OK) {
        return status;
    }

    SourceLocation location = syntaxAnalyzer->currentToken->location;
    AbstractSyntaxNode *block = createNode(ABSTRACT_SYNTAX_BLOCK, location);
    if (block == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    AbstractSyntaxNode *statement = NULL;
    status = skipNewlines(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    while (!currentIsType(syntaxAnalyzer, TOKEN_DEDENT) &&
           !currentIsType(syntaxAnalyzer, TOKEN_EOF)) {
        bool previousConsumedNewline = syntaxAnalyzer->newlineConsumed;
        syntaxAnalyzer->newlineConsumed = false;
        status = parseStatement(syntaxAnalyzer, &statement);
        bool statementConsumedNewline = syntaxAnalyzer->newlineConsumed;
        syntaxAnalyzer->newlineConsumed = previousConsumedNewline;
        if (status != COMPILER_OK) {
            goto cleanup;
        }

        bool isCompoundStatement = statement->kind == ABSTRACT_SYNTAX_IF ||
                                   statement->kind == ABSTRACT_SYNTAX_WHILE ||
                                   statement->kind == ABSTRACT_SYNTAX_FOR;
        status = appendOwnedChild(block, statement);
        statement = NULL;
        if (status != COMPILER_OK) {
            goto cleanup;
        }

        if (currentIsType(syntaxAnalyzer, TOKEN_NEWLINE)) {
            status = skipNewlines(syntaxAnalyzer);
            if (status != COMPILER_OK) {
                goto cleanup;
            }
        } else if (!currentIsType(syntaxAnalyzer, TOKEN_DEDENT) && !isCompoundStatement &&
                   !statementConsumedNewline) {
            status = COMPILER_SYNTAX_ERROR;
            goto cleanup;
        }
    }

    if (block->childCount == 0) {
        status = COMPILER_SYNTAX_ERROR;
        goto cleanup;
    }

    status = expectType(syntaxAnalyzer, TOKEN_DEDENT);
    if (status != COMPILER_OK) {
        goto cleanup;
    }

    *outNode = block;
    return COMPILER_OK;

cleanup:
    abstractSyntaxNodeDestroy(statement);
    abstractSyntaxNodeDestroy(block);
    return status;
}
