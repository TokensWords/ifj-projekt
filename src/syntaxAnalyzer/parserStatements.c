/**
 * @file syntaxAnalyzer.c
 * @brief Implements parsing of statements and blocks of statements.
 *
 * @project IFJ26 compiler
 * @author Matúš Čík
 */

#include "compilerTypes.h"
#include "syntaxAnalyzer.h"
#include "syntaxAnalyzerUtils.h"
#include "syntax.h"


static CompilerStatus expectBlockHeaderEnd(SyntaxAnalyzer *syntaxAnalyzer)
{
    if (!currentIsType(syntaxAnalyzer, TOKEN_NEWLINE)) {
        return COMPILER_SYNTAX_ERROR;
    }
    CompilerStatus status = advanceToken(syntaxAnalyzer);
    if (status == COMPILER_OK) {
        status = skipNewlines(syntaxAnalyzer);
    }
    return status;
}

CompilerStatus parseIf(SyntaxAnalyzer *syntaxAnalyzer,
                              AbstractSyntaxNode **outNode)
{
    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    SourceLocation location = syntaxAnalyzer->currentToken->location;
    AbstractSyntaxNode *ifNode = createNode(ABSTRACT_SYNTAX_IF, location);
    if (ifNode == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    status = advanceToken(syntaxAnalyzer);
    AbstractSyntaxNode *condition = NULL;
    if (status == COMPILER_OK) {
        status = analyzerExpressionsParse(syntaxAnalyzer, &condition);
    }
    if (status == COMPILER_OK) {
        status = appendOwnedChild(ifNode, condition);
    } else {
        abstractSyntaxNodeDestroy(condition);
    }
    if (status == COMPILER_OK) {
        status = expectWord(syntaxAnalyzer, SYNTAX_KW_THEN);
    }
    if (status == COMPILER_OK) {
        status = expectBlockHeaderEnd(syntaxAnalyzer);
    }
    AbstractSyntaxNode *thenBlock = NULL;
    if (status == COMPILER_OK) {
        status = analyzerStatementsParseBlock(syntaxAnalyzer, &thenBlock);
    }
    if (status == COMPILER_OK) {
        status = appendOwnedChild(ifNode, thenBlock);
    } else {
        abstractSyntaxNodeDestroy(thenBlock);
    }
    if (status == COMPILER_OK) {
        status = skipNewlines(syntaxAnalyzer);
    }
    if (status == COMPILER_OK) {
        status = expectWord(syntaxAnalyzer, SYNTAX_KW_ELSE);
    }
    if (status == COMPILER_OK) {
        status = expectBlockHeaderEnd(syntaxAnalyzer);
    }
    AbstractSyntaxNode *elseBlock = NULL;
    if (status == COMPILER_OK) {
        status = analyzerStatementsParseBlock(syntaxAnalyzer, &elseBlock);
    }
    if (status == COMPILER_OK) {
        status = appendOwnedChild(ifNode, elseBlock);
    } else {
        abstractSyntaxNodeDestroy(elseBlock);
    }
    if (status != COMPILER_OK) {
        abstractSyntaxNodeDestroy(ifNode);
        return status;
    }
    *outNode = ifNode;
    return COMPILER_OK;
}

CompilerStatus parseWhile(SyntaxAnalyzer *syntaxAnalyzer,
                                 AbstractSyntaxNode **outNode)
{
    CompilerStatus status = ensureCurrentToken(syntaxAnalyzer);
    if (status != COMPILER_OK) {
        return status;
    }
    SourceLocation location = syntaxAnalyzer->currentToken->location;
    AbstractSyntaxNode *whileNode = createNode(ABSTRACT_SYNTAX_WHILE, location);
    if (whileNode == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    status = advanceToken(syntaxAnalyzer);
    AbstractSyntaxNode *condition = NULL;
    if (status == COMPILER_OK) {
        status = analyzerExpressionsParse(syntaxAnalyzer, &condition);
    }
    if (status == COMPILER_OK) {
        status = appendOwnedChild(whileNode, condition);
    } else {
        abstractSyntaxNodeDestroy(condition);
    }
    if (status == COMPILER_OK) {
        status = expectWord(syntaxAnalyzer, SYNTAX_KW_DO);
    }
    if (status == COMPILER_OK) {
        status = expectBlockHeaderEnd(syntaxAnalyzer);
    }
    AbstractSyntaxNode *body = NULL;
    if (status == COMPILER_OK) {
        status = analyzerStatementsParseBlock(syntaxAnalyzer, &body);
    }
    if (status == COMPILER_OK) {
        status = appendOwnedChild(whileNode, body);
    } else {
        abstractSyntaxNodeDestroy(body);
    }
    if (status != COMPILER_OK) {
        abstractSyntaxNodeDestroy(whileNode);
        return status;
    }
    *outNode = whileNode;
    return COMPILER_OK;
}

/* Statement dispatch and indentation-delimited blocks. */

CompilerStatus parseStatement(SyntaxAnalyzer *syntaxAnalyzer,
                                     AbstractSyntaxNode **outNode)
{
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
    if (isIdentifierToken(syntaxAnalyzer->currentToken)) {
        status = ensureNextToken(syntaxAnalyzer);
        if (status != COMPILER_OK) {
            return status;
        }
        if (syntaxAnalyzer->nextToken != NULL &&
            tokenHasText(syntaxAnalyzer->nextToken, SYNTAX_OP_ASSIGN)) {
            return parseAssignment(syntaxAnalyzer, outNode);
        }
    }

    status = analyzerExpressionsParseValueOrCall(syntaxAnalyzer, outNode);
    if (status != COMPILER_OK || (*outNode)->kind == ABSTRACT_SYNTAX_FUNCTION_CALL) {
        return status;
    }

    AbstractSyntaxNode *expression = *outNode;
    AbstractSyntaxNode *statement = createNode(ABSTRACT_SYNTAX_EXPRESSION_STATEMENT,
                                               expression->location);
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

//TODO parse block