#include "syntaxAnalyzer.h"
#include "parserExpressions.h"
#include "syntaxOperators.h"

#include <assert.h>
#include <stddef.h>

typedef struct {
    TokenType type;
    const char *lexeme;
    size_t line;
    size_t column;
    int64_t intValue;
} TestToken;

struct LexicalAnalyzer {
    const TestToken *tokens;
    size_t tokenCount;
    size_t nextToken;
};

CompilerStatus lexicalAnalyzerNextToken(LexicalAnalyzer *lexicalAnalyzer, Token **outToken)
{
    if (lexicalAnalyzer == NULL || outToken == NULL ||
        lexicalAnalyzer->nextToken >= lexicalAnalyzer->tokenCount) {
        return COMPILER_INTERNAL_ERROR;
    }

    const TestToken *testToken = &lexicalAnalyzer->tokens[lexicalAnalyzer->nextToken++];
    Token *token = tokenCreate();
    if (token == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    token->type = testToken->type;
    token->location.line = testToken->line;
    token->location.column = testToken->column;
    token->value.intValue = testToken->intValue;
    if (testToken->lexeme != NULL && tokenSetLexeme(token, testToken->lexeme) != COMPILER_OK) {
        tokenDestroy(token);
        return COMPILER_INTERNAL_ERROR;
    }
    *outToken = token;
    return COMPILER_OK;
}

static CompilerStatus parseExpressionTokens(const TestToken *tokens, size_t tokenCount,
                                            AbstractSyntaxNode **outNode)
{
    struct LexicalAnalyzer lexerState = {
        .tokens = tokens,
        .tokenCount = tokenCount,
        .nextToken = 0
    };
    SyntaxAnalyzer *syntaxAnalyzer = syntaxAnalyzerCreate((LexicalAnalyzer *)&lexerState);
    assert(syntaxAnalyzer != NULL);
    CompilerStatus status = analyzerExpressionsParse(syntaxAnalyzer, outNode);
    syntaxAnalyzerDestroy(syntaxAnalyzer);
    return status;
}

#define TEST_TOKEN(tokenType, tokenText, tokenLine, tokenColumn) \
    {tokenType, tokenText, tokenLine, tokenColumn, 0}

int main(void)
{
    static const TestToken tokens[] = {
        TEST_TOKEN(TOKEN_KEYWORD, "import", 1, 1),
        TEST_TOKEN(TOKEN_IDENTIFIER, "ifj26", 1, 8),
        TEST_TOKEN(TOKEN_DOT, ".", 1, 13),
        TEST_TOKEN(TOKEN_OPERATOR, "*", 1, 14),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 1, 15),
        TEST_TOKEN(TOKEN_AT, "@", 2, 1),
        TEST_TOKEN(TOKEN_KEYWORD, "main", 2, 2),
        TEST_TOKEN(TOKEN_KEYWORD, "def", 2, 7),
        TEST_TOKEN(TOKEN_IDENTIFIER, "main", 2, 11),
        TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 2, 15),
        TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 2, 16),
        TEST_TOKEN(TOKEN_COLON, ":", 2, 17),
        TEST_TOKEN(TOKEN_KEYWORD, "Unit", 2, 19),
        TEST_TOKEN(TOKEN_OPERATOR, "=", 2, 24),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 2, 25),
        TEST_TOKEN(TOKEN_INDENT, NULL, 3, 3),
        TEST_TOKEN(TOKEN_KEYWORD, "var", 3, 3),
        TEST_TOKEN(TOKEN_IDENTIFIER, "total", 3, 7),
        TEST_TOKEN(TOKEN_COLON, ":", 3, 12),
        TEST_TOKEN(TOKEN_KEYWORD, "Int", 3, 14),
        TEST_TOKEN(TOKEN_OPERATOR, "=", 3, 18),
        {TOKEN_INT_LITERAL, "1", 3, 20, 1},
        TEST_TOKEN(TOKEN_OPERATOR, "+", 3, 22),
        {TOKEN_INT_LITERAL, "2", 3, 24, 2},
        TEST_TOKEN(TOKEN_OPERATOR, "*", 3, 26),
        {TOKEN_INT_LITERAL, "3", 3, 28, 3},
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 3, 29),
        TEST_TOKEN(TOKEN_KEYWORD, "val", 4, 3),
        TEST_TOKEN(TOKEN_IDENTIFIER, "fetched", 4, 7),
        TEST_TOKEN(TOKEN_OPERATOR, "=", 4, 15),
        TEST_TOKEN(TOKEN_IDENTIFIER, "readValue", 4, 17),
        TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 4, 26),
        TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 4, 27),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 4, 28),
        TEST_TOKEN(TOKEN_IDENTIFIER, "total", 5, 3),
        TEST_TOKEN(TOKEN_OPERATOR, "=", 5, 9),
        TEST_TOKEN(TOKEN_IDENTIFIER, "updateTotal", 5, 11),
        TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 5, 22),
        TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 5, 23),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 5, 24),
        TEST_TOKEN(TOKEN_KEYWORD, "return", 6, 3),
        TEST_TOKEN(TOKEN_IDENTIFIER, "finish", 6, 10),
        TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 6, 16),
        TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 6, 17),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 6, 18),
        TEST_TOKEN(TOKEN_KEYWORD, "while", 8, 3),
        TEST_TOKEN(TOKEN_IDENTIFIER, "total", 8, 9),
        TEST_TOKEN(TOKEN_OPERATOR, ">", 8, 15),
        {TOKEN_INT_LITERAL, "0", 8, 17, 0},
        TEST_TOKEN(TOKEN_KEYWORD, "do", 8, 19),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 8, 21),
        TEST_TOKEN(TOKEN_INDENT, NULL, 9, 5),
        TEST_TOKEN(TOKEN_KEYWORD, "if", 9, 5),
        TEST_TOKEN(TOKEN_IDENTIFIER, "total", 9, 8),
        TEST_TOKEN(TOKEN_OPERATOR, "<", 9, 14),
        {TOKEN_INT_LITERAL, "10", 9, 16, 10},
        TEST_TOKEN(TOKEN_KEYWORD, "then", 9, 19),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 9, 23),
        TEST_TOKEN(TOKEN_INDENT, NULL, 10, 7),
        TEST_TOKEN(TOKEN_UNIT_LITERAL, "()", 10, 7),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 10, 9),
        TEST_TOKEN(TOKEN_DEDENT, NULL, 11, 5),
        TEST_TOKEN(TOKEN_KEYWORD, "else", 11, 5),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 11, 9),
        TEST_TOKEN(TOKEN_INDENT, NULL, 12, 7),
        TEST_TOKEN(TOKEN_UNIT_LITERAL, "()", 12, 7),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 12, 9),
        TEST_TOKEN(TOKEN_DEDENT, NULL, 13, 5),
        TEST_TOKEN(TOKEN_DEDENT, NULL, 13, 3),
        TEST_TOKEN(TOKEN_DEDENT, NULL, 14, 1),
        TEST_TOKEN(TOKEN_EOF, NULL, 14, 1)
    };
        assert(syntaxPrecedenceRelation(EXPR_TERMINAL_MULTIPLY, EXPR_TERMINAL_ADD) ==
            PRECEDENCE_REDUCE);
        assert(syntaxPrecedenceRelation(EXPR_TERMINAL_ADD, EXPR_TERMINAL_MULTIPLY) ==
            PRECEDENCE_SHIFT);
        assert(syntaxPrecedenceRelation(EXPR_TERMINAL_ADD, EXPR_TERMINAL_SUBTRACT) ==
            PRECEDENCE_REDUCE);
        assert(syntaxPrecedenceRelation(EXPR_TERMINAL_LESS, EXPR_TERMINAL_LESS_EQUAL) ==
            PRECEDENCE_ERROR);
        assert(syntaxPrecedenceRelation(EXPR_TERMINAL_EQUAL, EXPR_TERMINAL_NOT_EQUAL) ==
            PRECEDENCE_ERROR);
        assert(syntaxPrecedenceRelation(EXPR_TERMINAL_END, EXPR_TERMINAL_OPERAND) ==
               PRECEDENCE_SHIFT);
        assert(syntaxPrecedenceRelation(EXPR_TERMINAL_LEFT_PAREN,
                                        EXPR_TERMINAL_RIGHT_PAREN) == PRECEDENCE_EQUAL);

        Token comparisonToken = {
            .type = TOKEN_OPERATOR,
            .lexeme = ">=",
            .lexemeLength = 2
        };
        ExpressionTerminal comparisonTerminal;
        assert(syntaxOperatorFromToken(&comparisonToken, &comparisonTerminal));
        assert(comparisonTerminal == EXPR_TERMINAL_GREATER_EQUAL);

    static const TestToken parenthesizedExpressionTokens[] = {
        {TOKEN_INT_LITERAL, "1", 1, 1, 1},
        TEST_TOKEN(TOKEN_OPERATOR, "+", 1, 3),
        TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 1, 5),
        {TOKEN_INT_LITERAL, "2", 1, 6, 2},
        TEST_TOKEN(TOKEN_OPERATOR, "*", 1, 8),
        {TOKEN_INT_LITERAL, "3", 1, 10, 3},
        TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 1, 11),
        TEST_TOKEN(TOKEN_EOF, NULL, 1, 12)
    };
    AbstractSyntaxNode *expression = NULL;
    assert(parseExpressionTokens(parenthesizedExpressionTokens,
                                 sizeof(parenthesizedExpressionTokens) /
                                     sizeof(parenthesizedExpressionTokens[0]),
                                 &expression) == COMPILER_OK);
    assert(expression->kind == ABSTRACT_SYNTAX_EXPRESSION_BINARY);
    assert(expression->text != NULL && expression->text[0] == '+');
    assert(expression->children[1]->kind == ABSTRACT_SYNTAX_EXPRESSION_BINARY);
    assert(expression->children[1]->text != NULL && expression->children[1]->text[0] == '*');
    abstractSyntaxNodeDestroy(expression);

    static const TestToken booleanExpressionTokens[] = {
        TEST_TOKEN(TOKEN_IDENTIFIER, "left", 1, 1),
        TEST_TOKEN(TOKEN_OPERATOR, "<=", 1, 6),
        TEST_TOKEN(TOKEN_IDENTIFIER, "right", 1, 9),
        TEST_TOKEN(TOKEN_EOF, NULL, 1, 14)
    };
    expression = NULL;
    assert(parseExpressionTokens(booleanExpressionTokens,
                                 sizeof(booleanExpressionTokens) /
                                     sizeof(booleanExpressionTokens[0]),
                                 &expression) == COMPILER_OK);
    assert(expression->kind == ABSTRACT_SYNTAX_EXPRESSION_BINARY);
    assert(expression->text != NULL && expression->text[0] == '<');
    abstractSyntaxNodeDestroy(expression);

    static const TestToken chainedComparisonTokens[] = {
        TEST_TOKEN(TOKEN_IDENTIFIER, "a", 1, 1),
        TEST_TOKEN(TOKEN_OPERATOR, "<", 1, 3),
        TEST_TOKEN(TOKEN_IDENTIFIER, "b", 1, 5),
        TEST_TOKEN(TOKEN_OPERATOR, "<", 1, 7),
        TEST_TOKEN(TOKEN_IDENTIFIER, "c", 1, 9),
        TEST_TOKEN(TOKEN_EOF, NULL, 1, 10)
    };
    expression = NULL;
    assert(parseExpressionTokens(chainedComparisonTokens,
                                 sizeof(chainedComparisonTokens) /
                                     sizeof(chainedComparisonTokens[0]),
                                 &expression) == COMPILER_SYNTAX_ERROR);
    assert(expression == NULL);

    static const TestToken callOperandTokens[] = {
        TEST_TOKEN(TOKEN_IDENTIFIER, "readValue", 1, 1),
        TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 1, 10),
        TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 1, 11),
        TEST_TOKEN(TOKEN_OPERATOR, "+", 1, 13),
        {TOKEN_INT_LITERAL, "1", 1, 15, 1},
        TEST_TOKEN(TOKEN_EOF, NULL, 1, 16)
    };
    expression = NULL;
    assert(parseExpressionTokens(callOperandTokens,
                                 sizeof(callOperandTokens) /
                                     sizeof(callOperandTokens[0]),
                                 &expression) == COMPILER_SYNTAX_ERROR);
    assert(expression == NULL);

    struct LexicalAnalyzer lexerState = {
        .tokens = tokens,
        .tokenCount = sizeof(tokens) / sizeof(tokens[0]),
        .nextToken = 0
    };

    LexicalAnalyzer *lexer = (LexicalAnalyzer *)&lexerState;
    SyntaxAnalyzer *syntaxAnalyzer = syntaxAnalyzerCreate(lexer);
    assert(syntaxAnalyzer != NULL);

    SourceProgram *program = NULL;
    assert(syntaxAnalyzerParseProgram(syntaxAnalyzer, &program) == COMPILER_OK);
    assert(program != NULL);
    assert(program->root != NULL);
    assert(program->root->kind == ABSTRACT_SYNTAX_PROGRAM);
    assert(program->root->childCount == 1);

    AbstractSyntaxNode *function = program->root->children[0];
    assert(function->kind == ABSTRACT_SYNTAX_FUNCTION);
    assert(function->isEntryFunction);
    assert(function->childCount == 1);

    AbstractSyntaxNode *body = function->children[0];
    assert(body->kind == ABSTRACT_SYNTAX_BLOCK);
    assert(body->childCount == 5);

    AbstractSyntaxNode *declaration = body->children[0];
    assert(declaration->kind == ABSTRACT_SYNTAX_VARIABLE_DECLARATION);
    assert(declaration->isMutable);
    assert(declaration->variableType == DATA_TYPE_INT);
    assert(declaration->childCount == 1);

    AbstractSyntaxNode *addition = declaration->children[0];
    assert(addition->kind == ABSTRACT_SYNTAX_EXPRESSION_BINARY);
    assert(addition->text != NULL && addition->text[0] == '+');
    assert(addition->childCount == 2);
    assert(addition->children[0]->kind == ABSTRACT_SYNTAX_INT_LITERAL);
    assert(addition->children[1]->kind == ABSTRACT_SYNTAX_EXPRESSION_BINARY);
    assert(addition->children[1]->text != NULL && addition->children[1]->text[0] == '*');

    AbstractSyntaxNode *callDeclaration = body->children[1];
    assert(callDeclaration->kind == ABSTRACT_SYNTAX_VARIABLE_DECLARATION);
    assert(callDeclaration->children[0]->kind == ABSTRACT_SYNTAX_FUNCTION_CALL);

    AbstractSyntaxNode *assignment = body->children[2];
    assert(assignment->kind == ABSTRACT_SYNTAX_ASSIGNMENT);
    assert(assignment->children[0]->kind == ABSTRACT_SYNTAX_FUNCTION_CALL);

    AbstractSyntaxNode *returnNode = body->children[3];
    assert(returnNode->kind == ABSTRACT_SYNTAX_RETURN);
    assert(returnNode->children[0]->kind == ABSTRACT_SYNTAX_FUNCTION_CALL);

    AbstractSyntaxNode *call = body->children[4];
    assert(call->kind == ABSTRACT_SYNTAX_FUNCTION_CALL);
    assert(call->childCount == 1);
    assert(call->children[0]->kind == ABSTRACT_SYNTAX_IDENTIFIER);

    AbstractSyntaxNode *whileNode = body->children[4];
    assert(whileNode->kind == ABSTRACT_SYNTAX_WHILE);
    assert(whileNode->childCount == 2);
    assert(whileNode->children[0]->kind == ABSTRACT_SYNTAX_EXPRESSION_BINARY);
    AbstractSyntaxNode *whileBody = whileNode->children[1];
    assert(whileBody->kind == ABSTRACT_SYNTAX_BLOCK);
    assert(whileBody->childCount == 1);

    AbstractSyntaxNode *ifNode = whileBody->children[0];
    assert(ifNode->kind == ABSTRACT_SYNTAX_IF);
    assert(ifNode->childCount == 3);
    assert(ifNode->children[1]->kind == ABSTRACT_SYNTAX_BLOCK);
    assert(ifNode->children[2]->kind == ABSTRACT_SYNTAX_BLOCK);
    assert(ifNode->children[2]->children[0]->kind == ABSTRACT_SYNTAX_EXPRESSION_STATEMENT);

    sourceProgramDestroy(program);
    syntaxAnalyzerDestroy(syntaxAnalyzer);

    static const TestToken missingPrologueTokens[] = {
        TEST_TOKEN(TOKEN_KEYWORD, "def", 1, 1),
        TEST_TOKEN(TOKEN_EOF, NULL, 1, 4)
    };
    struct LexicalAnalyzer invalidLexerState = {
        .tokens = missingPrologueTokens,
        .tokenCount = sizeof(missingPrologueTokens) / sizeof(missingPrologueTokens[0]),
        .nextToken = 0
    };
    syntaxAnalyzer = syntaxAnalyzerCreate((LexicalAnalyzer *)&invalidLexerState);
    assert(syntaxAnalyzer != NULL);
    program = NULL;
    assert(syntaxAnalyzerParseProgram(syntaxAnalyzer, &program) == COMPILER_SYNTAX_ERROR);
    assert(program == NULL);
    syntaxAnalyzerDestroy(syntaxAnalyzer);

    static const TestToken missingEntryFunctionTokens[] = {
        TEST_TOKEN(TOKEN_KEYWORD, "import", 1, 1),
        TEST_TOKEN(TOKEN_IDENTIFIER, "ifj26", 1, 8),
        TEST_TOKEN(TOKEN_DOT, ".", 1, 13),
        TEST_TOKEN(TOKEN_OPERATOR, "*", 1, 14),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 1, 15),
        TEST_TOKEN(TOKEN_KEYWORD, "def", 2, 1),
        TEST_TOKEN(TOKEN_IDENTIFIER, "helper", 2, 5),
        TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 2, 11),
        TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 2, 12),
        TEST_TOKEN(TOKEN_OPERATOR, "=", 2, 14),
        TEST_TOKEN(TOKEN_UNIT_LITERAL, "()", 2, 16),
        TEST_TOKEN(TOKEN_EOF, NULL, 2, 18)
    };
    struct LexicalAnalyzer missingEntryLexerState = {
        .tokens = missingEntryFunctionTokens,
        .tokenCount = sizeof(missingEntryFunctionTokens) / sizeof(missingEntryFunctionTokens[0]),
        .nextToken = 0
    };
    syntaxAnalyzer = syntaxAnalyzerCreate((LexicalAnalyzer *)&missingEntryLexerState);
    assert(syntaxAnalyzer != NULL);
    program = NULL;
    assert(syntaxAnalyzerParseProgram(syntaxAnalyzer, &program) ==
           COMPILER_UNDEFINED_SYMBOL_ERROR);
    assert(program == NULL);
    syntaxAnalyzerDestroy(syntaxAnalyzer);

    static const TestToken duplicateEntryFunctionTokens[] = {
        TEST_TOKEN(TOKEN_KEYWORD, "import", 1, 1),
        TEST_TOKEN(TOKEN_IDENTIFIER, "ifj26", 1, 8),
        TEST_TOKEN(TOKEN_DOT, ".", 1, 13),
        TEST_TOKEN(TOKEN_OPERATOR, "*", 1, 14),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 1, 15),
        TEST_TOKEN(TOKEN_AT, "@", 2, 1),
        TEST_TOKEN(TOKEN_KEYWORD, "main", 2, 2),
        TEST_TOKEN(TOKEN_KEYWORD, "def", 2, 7),
        TEST_TOKEN(TOKEN_IDENTIFIER, "first", 2, 11),
        TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 2, 16),
        TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 2, 17),
        TEST_TOKEN(TOKEN_OPERATOR, "=", 2, 19),
        TEST_TOKEN(TOKEN_UNIT_LITERAL, "()", 2, 21),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 2, 23),
        TEST_TOKEN(TOKEN_AT, "@", 3, 1),
        TEST_TOKEN(TOKEN_KEYWORD, "main", 3, 2),
        TEST_TOKEN(TOKEN_KEYWORD, "def", 3, 7),
        TEST_TOKEN(TOKEN_IDENTIFIER, "second", 3, 11),
        TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 3, 17),
        TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 3, 18),
        TEST_TOKEN(TOKEN_OPERATOR, "=", 3, 20),
        TEST_TOKEN(TOKEN_UNIT_LITERAL, "()", 3, 22),
        TEST_TOKEN(TOKEN_EOF, NULL, 3, 24)
    };
    struct LexicalAnalyzer duplicateEntryLexerState = {
        .tokens = duplicateEntryFunctionTokens,
        .tokenCount = sizeof(duplicateEntryFunctionTokens) /
                      sizeof(duplicateEntryFunctionTokens[0]),
        .nextToken = 0
    };
    syntaxAnalyzer = syntaxAnalyzerCreate((LexicalAnalyzer *)&duplicateEntryLexerState);
    assert(syntaxAnalyzer != NULL);
    program = NULL;
    assert(syntaxAnalyzerParseProgram(syntaxAnalyzer, &program) ==
           COMPILER_REDEFINITION_ERROR);
    assert(program == NULL);
    syntaxAnalyzerDestroy(syntaxAnalyzer);

    static const TestToken unaryExpressionTokens[] = {
        TEST_TOKEN(TOKEN_KEYWORD, "import", 1, 1),
        TEST_TOKEN(TOKEN_IDENTIFIER, "ifj26", 1, 8),
        TEST_TOKEN(TOKEN_DOT, ".", 1, 13),
        TEST_TOKEN(TOKEN_OPERATOR, "*", 1, 14),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 1, 15),
        TEST_TOKEN(TOKEN_AT, "@", 2, 1),
        TEST_TOKEN(TOKEN_KEYWORD, "main", 2, 2),
        TEST_TOKEN(TOKEN_KEYWORD, "def", 2, 7),
        TEST_TOKEN(TOKEN_IDENTIFIER, "main", 2, 11),
        TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 2, 15),
        TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 2, 16),
        TEST_TOKEN(TOKEN_COLON, ":", 2, 17),
        TEST_TOKEN(TOKEN_KEYWORD, "Unit", 2, 19),
        TEST_TOKEN(TOKEN_OPERATOR, "=", 2, 24),
        TEST_TOKEN(TOKEN_OPERATOR, "-", 2, 26),
        {TOKEN_INT_LITERAL, "1", 2, 27, 1},
        TEST_TOKEN(TOKEN_EOF, NULL, 2, 28)
    };
    struct LexicalAnalyzer unaryLexerState = {
        .tokens = unaryExpressionTokens,
        .tokenCount = sizeof(unaryExpressionTokens) / sizeof(unaryExpressionTokens[0]),
        .nextToken = 0
    };
    syntaxAnalyzer = syntaxAnalyzerCreate((LexicalAnalyzer *)&unaryLexerState);
    assert(syntaxAnalyzer != NULL);
    program = NULL;
    assert(syntaxAnalyzerParseProgram(syntaxAnalyzer, &program) == COMPILER_SYNTAX_ERROR);
    assert(program == NULL);
    syntaxAnalyzerDestroy(syntaxAnalyzer);
    return 0;
}

#undef TEST_TOKEN
