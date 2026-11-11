#include "parserExpressions.h"
#include "parserStatements.h"
#include "parserTypes.h"
#include "syntaxAnalyzer.h"
#include "syntaxOperators.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

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

CompilerStatus lexicalAnalyzerNextToken(LexicalAnalyzer *lexicalAnalyzer, Token **outToken) {
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
    if (testToken->type == TOKEN_BOOL_LITERAL) {
        token->value.boolValue = testToken->intValue != 0;
    } else {
        token->value.intValue = testToken->intValue;
    }
    if (testToken->lexeme != NULL && tokenSetLexeme(token, testToken->lexeme) != COMPILER_OK) {
        tokenDestroy(token);
        return COMPILER_INTERNAL_ERROR;
    }
    *outToken = token;
    return COMPILER_OK;
}

static CompilerStatus parseExpressionTokens(const TestToken *tokens, size_t tokenCount,
                                            AbstractSyntaxNode **outNode) {
    struct LexicalAnalyzer lexerState = {
        .tokens = tokens, .tokenCount = tokenCount, .nextToken = 0};
    SyntaxAnalyzer *syntaxAnalyzer = syntaxAnalyzerCreate((LexicalAnalyzer *)&lexerState);
    assert(syntaxAnalyzer != NULL);
    CompilerStatus status = analyzerExpressionsParse(syntaxAnalyzer, outNode);
    syntaxAnalyzerDestroy(syntaxAnalyzer);
    return status;
}

static CompilerStatus parseIfTokens(const TestToken *tokens, size_t tokenCount,
                                    AbstractSyntaxNode **outNode) {
    struct LexicalAnalyzer lexerState = {
        .tokens = tokens, .tokenCount = tokenCount, .nextToken = 0};
    SyntaxAnalyzer *syntaxAnalyzer = syntaxAnalyzerCreate((LexicalAnalyzer *)&lexerState);
    assert(syntaxAnalyzer != NULL);
    CompilerStatus status = parseIf(syntaxAnalyzer, outNode);
    syntaxAnalyzerDestroy(syntaxAnalyzer);
    return status;
}

static CompilerStatus parseForTokens(const TestToken *tokens, size_t tokenCount,
                                     AbstractSyntaxNode **outNode) {
    struct LexicalAnalyzer lexerState = {
        .tokens = tokens, .tokenCount = tokenCount, .nextToken = 0};
    SyntaxAnalyzer *syntaxAnalyzer = syntaxAnalyzerCreate((LexicalAnalyzer *)&lexerState);
    assert(syntaxAnalyzer != NULL);
    CompilerStatus status = parseFor(syntaxAnalyzer, outNode);
    syntaxAnalyzerDestroy(syntaxAnalyzer);
    return status;
}

static CompilerStatus parseBlockTokens(const TestToken *tokens, size_t tokenCount,
                                       AbstractSyntaxNode **outNode) {
    struct LexicalAnalyzer lexerState = {
        .tokens = tokens, .tokenCount = tokenCount, .nextToken = 0};
    SyntaxAnalyzer *syntaxAnalyzer = syntaxAnalyzerCreate((LexicalAnalyzer *)&lexerState);
    assert(syntaxAnalyzer != NULL);
    CompilerStatus status = parseBlock(syntaxAnalyzer, outNode);
    syntaxAnalyzerDestroy(syntaxAnalyzer);
    return status;
}

#define TEST_TOKEN(tokenType, tokenText, tokenLine, tokenColumn)                                   \
    {tokenType, tokenText, tokenLine, tokenColumn, 0}

static CompilerStatus parseTypeTokens(const TestToken *tokens, size_t tokenCount,
                                      TypeDescriptor **outType) {
    struct LexicalAnalyzer lexerState = {
        .tokens = tokens, .tokenCount = tokenCount, .nextToken = 0};
    SyntaxAnalyzer *syntaxAnalyzer = syntaxAnalyzerCreate((LexicalAnalyzer *)&lexerState);
    assert(syntaxAnalyzer != NULL);
    CompilerStatus status = analyzerTypesParseType(syntaxAnalyzer, outType);
    syntaxAnalyzerDestroy(syntaxAnalyzer);
    return status;
}

int main(void) {
    static const TestToken flattenedUnionTokens[] = {
        TEST_TOKEN(TOKEN_KEYWORD, "Int", 1, 1),    TEST_TOKEN(TOKEN_OPERATOR, "|", 1, 5),
        TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 1, 7),   TEST_TOKEN(TOKEN_KEYWORD, "Double", 1, 8),
        TEST_TOKEN(TOKEN_OPERATOR, "|", 1, 15),    TEST_TOKEN(TOKEN_KEYWORD, "String", 1, 17),
        TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 1, 23), TEST_TOKEN(TOKEN_OPERATOR, "|", 1, 25),
        TEST_TOKEN(TOKEN_KEYWORD, "Int", 1, 27),   TEST_TOKEN(TOKEN_EOF, NULL, 1, 30)};
    TypeDescriptor *parsedType = NULL;
    assert(parseTypeTokens(flattenedUnionTokens,
                           sizeof(flattenedUnionTokens) / sizeof(flattenedUnionTokens[0]),
                           &parsedType) == COMPILER_OK);
    assert(typeDescriptorGetMask(parsedType) ==
           (DATA_TYPE_INT | DATA_TYPE_DOUBLE | DATA_TYPE_STRING));
    assert(typeDescriptorGetMemberCount(parsedType) == 3);
    assert(typeDescriptorGetMask(typeDescriptorGetMember(parsedType, 0)) == DATA_TYPE_INT);
    assert(typeDescriptorGetMask(typeDescriptorGetMember(parsedType, 1)) == DATA_TYPE_DOUBLE);
    assert(typeDescriptorGetMask(typeDescriptorGetMember(parsedType, 2)) == DATA_TYPE_STRING);
    TypeDescriptor *typeCopy = typeDescriptorClone(parsedType);
    assert(typeCopy != NULL);
    assert(typeCopy != parsedType);
    assert(typeDescriptorEquals(typeCopy, parsedType));
    AbstractSyntaxNode *typedNode =
        abstractSyntaxNodeCreate(ABSTRACT_SYNTAX_IDENTIFIER, (SourceLocation){0});
    assert(typedNode != NULL);
    assert(abstractSyntaxNodeCopyInferredType(typedNode, parsedType) == COMPILER_OK);
    assert(typedNode->inferredType != parsedType);
    assert(typeDescriptorEquals(typedNode->inferredType, parsedType));
    abstractSyntaxNodeDestroy(typedNode);
    typeDescriptorDestroy(typeCopy);

    TypeDescriptor *narrowUnion = typeDescriptorCreate(DATA_TYPE_UNKNOWN);
    assert(narrowUnion != NULL);
    TypeDescriptor *intType = typeDescriptorCreate(DATA_TYPE_INT);
    TypeDescriptor *stringType = typeDescriptorCreate(DATA_TYPE_STRING);
    assert(intType != NULL && stringType != NULL);
    assert(typeDescriptorAddMember(narrowUnion, intType) == COMPILER_OK);
    assert(typeDescriptorAddMember(narrowUnion, stringType) == COMPILER_OK);
    assert(typeDescriptorIsSubset(narrowUnion, parsedType));
    assert(!typeDescriptorIsSubset(parsedType, narrowUnion));
    typeDescriptorDestroy(narrowUnion);
    typeDescriptorDestroy(parsedType);

    TypeDescriptor *firstAlias = typeDescriptorCreate(DATA_TYPE_ALIAS);
    TypeDescriptor *sameAlias = typeDescriptorCreate(DATA_TYPE_ALIAS);
    TypeDescriptor *differentAlias = typeDescriptorCreate(DATA_TYPE_ALIAS);
    assert(firstAlias != NULL && sameAlias != NULL && differentAlias != NULL);
    assert(typeDescriptorSetName(firstAlias, "First") == COMPILER_OK);
    assert(typeDescriptorSetName(sameAlias, "First") == COMPILER_OK);
    assert(typeDescriptorSetName(differentAlias, "Second") == COMPILER_OK);
    assert(typeDescriptorIsSubset(firstAlias, sameAlias));
    assert(!typeDescriptorIsSubset(firstAlias, differentAlias));
    typeDescriptorDestroy(firstAlias);
    typeDescriptorDestroy(sameAlias);
    typeDescriptorDestroy(differentAlias);

    TypeDescriptor *intList = typeDescriptorCreate(DATA_TYPE_LIST);
    TypeDescriptor *unionList = typeDescriptorCreate(DATA_TYPE_LIST);
    TypeDescriptor *intElement = typeDescriptorCreate(DATA_TYPE_INT);
    TypeDescriptor *unionElement = typeDescriptorCreate(DATA_TYPE_UNKNOWN);
    assert(intList != NULL && unionList != NULL && intElement != NULL && unionElement != NULL);
    assert(typeDescriptorAddMember(unionElement, typeDescriptorCreate(DATA_TYPE_INT)) ==
           COMPILER_OK);
    assert(typeDescriptorAddMember(unionElement, typeDescriptorCreate(DATA_TYPE_STRING)) ==
           COMPILER_OK);
    assert(typeDescriptorSetElementType(intList, intElement) == COMPILER_OK);
    assert(typeDescriptorSetElementType(unionList, unionElement) == COMPILER_OK);
    assert(typeDescriptorIsSubset(intList, unionList));
    typeDescriptorDestroy(intList);
    typeDescriptorDestroy(unionList);

    static const TestToken listUnionTypeTokens[] = {TEST_TOKEN(TOKEN_KEYWORD, "List", 1, 1),
                                                    TEST_TOKEN(TOKEN_LEFT_SHARP_PAREN, "[", 1, 5),
                                                    TEST_TOKEN(TOKEN_KEYWORD, "Int", 1, 6),
                                                    TEST_TOKEN(TOKEN_OPERATOR, "|", 1, 10),
                                                    TEST_TOKEN(TOKEN_IDENTIFIER, "Scalar", 1, 12),
                                                    TEST_TOKEN(TOKEN_RIGHT_SHARP_PAREN, "]", 1, 18),
                                                    TEST_TOKEN(TOKEN_EOF, NULL, 1, 19)};
    parsedType = NULL;
    assert(parseTypeTokens(listUnionTypeTokens,
                           sizeof(listUnionTypeTokens) / sizeof(listUnionTypeTokens[0]),
                           &parsedType) == COMPILER_OK);
    assert(typeDescriptorGetMask(parsedType) == DATA_TYPE_LIST);
    const TypeDescriptor *elementType = typeDescriptorGetElementType(parsedType);
    assert(elementType != NULL);
    assert(typeDescriptorGetMemberCount(elementType) == 2);
    assert(typeDescriptorGetMask(typeDescriptorGetMember(elementType, 1)) == DATA_TYPE_ALIAS);
    assert(strcmp(typeDescriptorGetName(typeDescriptorGetMember(elementType, 1)), "Scalar") == 0);
    typeDescriptorDestroy(parsedType);

    static const TestToken typeAliasTokens[] = {
        TEST_TOKEN(TOKEN_KEYWORD, "type", 1, 1), TEST_TOKEN(TOKEN_IDENTIFIER, "Scalar", 1, 6),
        TEST_TOKEN(TOKEN_OPERATOR, "=", 1, 13),  TEST_TOKEN(TOKEN_KEYWORD, "Int", 1, 15),
        TEST_TOKEN(TOKEN_OPERATOR, "|", 1, 19),  TEST_TOKEN(TOKEN_KEYWORD, "String", 1, 21),
        TEST_TOKEN(TOKEN_EOF, NULL, 1, 27)};
    struct LexicalAnalyzer aliasLexerState = {.tokens = typeAliasTokens,
                                              .tokenCount = sizeof(typeAliasTokens) /
                                                            sizeof(typeAliasTokens[0]),
                                              .nextToken = 0};
    SyntaxAnalyzer *aliasSyntaxAnalyzer = syntaxAnalyzerCreate((LexicalAnalyzer *)&aliasLexerState);
    assert(aliasSyntaxAnalyzer != NULL);
    AbstractSyntaxNode *typeAlias = NULL;
    assert(analyzerTypesParseAlias(aliasSyntaxAnalyzer, &typeAlias) == COMPILER_OK);
    assert(typeAlias->kind == ABSTRACT_SYNTAX_TYPE_ALIAS);
    assert(strcmp(typeAlias->text, "Scalar") == 0);
    assert(typeDescriptorGetMemberCount(typeAlias->variableType) == 2);
    assert(typeDescriptorGetMask(typeAlias->variableType) == (DATA_TYPE_INT | DATA_TYPE_STRING));
    abstractSyntaxNodeDestroy(typeAlias);
    syntaxAnalyzerDestroy(aliasSyntaxAnalyzer);

    static const TestToken programWithAliasTokens[] = {TEST_TOKEN(TOKEN_KEYWORD, "import", 1, 1),
                                                       TEST_TOKEN(TOKEN_IDENTIFIER, "ifj26", 1, 8),
                                                       TEST_TOKEN(TOKEN_DOT, ".", 1, 13),
                                                       TEST_TOKEN(TOKEN_OPERATOR, "*", 1, 14),
                                                       TEST_TOKEN(TOKEN_NEWLINE, NULL, 1, 15),
                                                       TEST_TOKEN(TOKEN_KEYWORD, "type", 2, 1),
                                                       TEST_TOKEN(TOKEN_IDENTIFIER, "Scalar", 2, 6),
                                                       TEST_TOKEN(TOKEN_OPERATOR, "=", 2, 13),
                                                       TEST_TOKEN(TOKEN_KEYWORD, "Int", 2, 15),
                                                       TEST_TOKEN(TOKEN_OPERATOR, "|", 2, 19),
                                                       TEST_TOKEN(TOKEN_KEYWORD, "String", 2, 21),
                                                       TEST_TOKEN(TOKEN_NEWLINE, NULL, 2, 27),
                                                       TEST_TOKEN(TOKEN_AT, "@", 3, 1),
                                                       TEST_TOKEN(TOKEN_KEYWORD, "main", 3, 2),
                                                       TEST_TOKEN(TOKEN_KEYWORD, "def", 3, 7),
                                                       TEST_TOKEN(TOKEN_IDENTIFIER, "main", 3, 11),
                                                       TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 3, 15),
                                                       TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 3, 16),
                                                       TEST_TOKEN(TOKEN_COLON, ":", 3, 17),
                                                       TEST_TOKEN(TOKEN_KEYWORD, "Unit", 3, 19),
                                                       TEST_TOKEN(TOKEN_OPERATOR, "=", 3, 24),
                                                       TEST_TOKEN(TOKEN_IDENTIFIER, "print", 3, 26),
                                                       TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 3, 31),
                                                       TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 3, 32),
                                                       TEST_TOKEN(TOKEN_EOF, NULL, 3, 33)};
    struct LexicalAnalyzer programLexerState = {.tokens = programWithAliasTokens,
                                                .tokenCount = sizeof(programWithAliasTokens) /
                                                              sizeof(programWithAliasTokens[0]),
                                                .nextToken = 0};
    SyntaxAnalyzer *programSyntaxAnalyzer =
        syntaxAnalyzerCreate((LexicalAnalyzer *)&programLexerState);
    assert(programSyntaxAnalyzer != NULL);
    SourceProgram *programWithAlias = NULL;
    assert(syntaxAnalyzerParseProgram(programSyntaxAnalyzer, &programWithAlias) == COMPILER_OK);
    assert(programWithAlias->root->childCount == 2);
    assert(programWithAlias->root->children[0]->kind == ABSTRACT_SYNTAX_TYPE_ALIAS);
    assert(programWithAlias->root->children[1]->kind == ABSTRACT_SYNTAX_FUNCTION);
    sourceProgramDestroy(programWithAlias);
    syntaxAnalyzerDestroy(programSyntaxAnalyzer);

    static const TestToken tokens[] = {TEST_TOKEN(TOKEN_KEYWORD, "import", 1, 1),
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
                                       TEST_TOKEN(TOKEN_IDENTIFIER, "print", 7, 3),
                                       TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 7, 8),
                                       TEST_TOKEN(TOKEN_IDENTIFIER, "total", 7, 9),
                                       TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 7, 14),
                                       TEST_TOKEN(TOKEN_NEWLINE, NULL, 7, 15),
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
                                       TEST_TOKEN(TOKEN_IDENTIFIER, "print", 13, 5),
                                       TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 13, 10),
                                       {TOKEN_INT_LITERAL, "4", 13, 11, 4},
                                       TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 13, 12),
                                       TEST_TOKEN(TOKEN_NEWLINE, NULL, 13, 13),
                                       TEST_TOKEN(TOKEN_DEDENT, NULL, 13, 3),
                                       TEST_TOKEN(TOKEN_IDENTIFIER, "print", 14, 3),
                                       TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 14, 8),
                                       {TOKEN_INT_LITERAL, "5", 14, 9, 5},
                                       TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 14, 10),
                                       TEST_TOKEN(TOKEN_NEWLINE, NULL, 14, 11),
                                       TEST_TOKEN(TOKEN_DEDENT, NULL, 15, 1),
                                       TEST_TOKEN(TOKEN_EOF, NULL, 15, 1)};
    assert(syntaxPrecedenceRelation(EXPR_TERMINAL_MULTIPLY, EXPR_TERMINAL_ADD) ==
           PRECEDENCE_REDUCE);
    assert(syntaxPrecedenceRelation(EXPR_TERMINAL_ADD, EXPR_TERMINAL_MULTIPLY) == PRECEDENCE_SHIFT);
    assert(syntaxPrecedenceRelation(EXPR_TERMINAL_ADD, EXPR_TERMINAL_SUBTRACT) ==
           PRECEDENCE_REDUCE);
    assert(syntaxPrecedenceRelation(EXPR_TERMINAL_LESS, EXPR_TERMINAL_LESS_EQUAL) ==
           PRECEDENCE_ERROR);
    assert(syntaxPrecedenceRelation(EXPR_TERMINAL_EQUAL, EXPR_TERMINAL_NOT_EQUAL) ==
           PRECEDENCE_ERROR);
    assert(syntaxPrecedenceRelation(EXPR_TERMINAL_END, EXPR_TERMINAL_OPERAND) == PRECEDENCE_SHIFT);
    assert(syntaxPrecedenceRelation(EXPR_TERMINAL_LEFT_PAREN, EXPR_TERMINAL_RIGHT_PAREN) ==
           PRECEDENCE_EQUAL);

    Token comparisonToken = {.type = TOKEN_OPERATOR, .lexeme = ">=", .lexemeLength = 2};
    ExpressionTerminal comparisonTerminal;
    assert(syntaxOperatorFromToken(&comparisonToken, &comparisonTerminal));
    assert(comparisonTerminal == EXPR_TERMINAL_GREATER_EQUAL);
    assert(syntaxOperatorFromToken(&(Token){.type = TOKEN_OPERATOR, .lexeme = "&&"},
                                   &comparisonTerminal));
    assert(comparisonTerminal == EXPR_TERMINAL_LOGICAL_AND);
    assert(syntaxPrecedenceRelation(EXPR_TERMINAL_LESS, EXPR_TERMINAL_LOGICAL_AND) ==
           PRECEDENCE_REDUCE);
    assert(syntaxPrecedenceRelation(EXPR_TERMINAL_LOGICAL_AND, EXPR_TERMINAL_LOGICAL_OR) ==
           PRECEDENCE_REDUCE);
    assert(syntaxPrecedenceRelation(EXPR_TERMINAL_LOGICAL_OR, EXPR_TERMINAL_LOGICAL_AND) ==
           PRECEDENCE_SHIFT);
    assert(syntaxPrecedenceRelation(EXPR_TERMINAL_LOGICAL_NOT, EXPR_TERMINAL_MULTIPLY) ==
           PRECEDENCE_REDUCE);

    static const TestToken parenthesizedExpressionTokens[] = {
        {TOKEN_INT_LITERAL, "1", 1, 1, 1},         TEST_TOKEN(TOKEN_OPERATOR, "+", 1, 3),
        TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 1, 5),   {TOKEN_INT_LITERAL, "2", 1, 6, 2},
        TEST_TOKEN(TOKEN_OPERATOR, "*", 1, 8),     {TOKEN_INT_LITERAL, "3", 1, 10, 3},
        TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 1, 11), TEST_TOKEN(TOKEN_EOF, NULL, 1, 12)};
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

    static const TestToken booleanOperatorExpressionTokens[] = {
        {TOKEN_BOOL_LITERAL, "true", 1, 1, 1},   TEST_TOKEN(TOKEN_OPERATOR, "&&", 1, 6),
        TEST_TOKEN(TOKEN_OPERATOR, "!", 1, 9),   {TOKEN_BOOL_LITERAL, "false", 1, 10, 0},
        TEST_TOKEN(TOKEN_OPERATOR, "||", 1, 16), {TOKEN_BOOL_LITERAL, "false", 1, 19, 0},
        TEST_TOKEN(TOKEN_EOF, NULL, 1, 24)};
    expression = NULL;
    assert(parseExpressionTokens(booleanOperatorExpressionTokens,
                                 sizeof(booleanOperatorExpressionTokens) /
                                     sizeof(booleanOperatorExpressionTokens[0]),
                                 &expression) == COMPILER_OK);
    assert(expression->kind == ABSTRACT_SYNTAX_EXPRESSION_BINARY);
    assert(strcmp(expression->text, "||") == 0);
    assert(expression->children[0]->kind == ABSTRACT_SYNTAX_EXPRESSION_BINARY);
    assert(strcmp(expression->children[0]->text, "&&") == 0);
    assert(expression->children[0]->children[0]->kind == ABSTRACT_SYNTAX_BOOLEAN_LITERAL);
    assert(expression->children[0]->children[0]->value.boolValue);
    assert(expression->children[0]->children[1]->kind == ABSTRACT_SYNTAX_UNARY_EXPRESSION);
    assert(strcmp(expression->children[0]->children[1]->text, "!") == 0);
    assert(expression->children[0]->children[1]->children[0]->kind ==
           ABSTRACT_SYNTAX_BOOLEAN_LITERAL);
    assert(!expression->children[0]->children[1]->children[0]->value.boolValue);
    abstractSyntaxNodeDestroy(expression);

    static const TestToken notPrecedenceTokens[] = {TEST_TOKEN(TOKEN_OPERATOR, "!", 1, 1),
                                                    {TOKEN_BOOL_LITERAL, "true", 1, 2, 1},
                                                    TEST_TOKEN(TOKEN_OPERATOR, "*", 1, 7),
                                                    {TOKEN_INT_LITERAL, "2", 1, 9, 2},
                                                    TEST_TOKEN(TOKEN_EOF, NULL, 1, 10)};
    expression = NULL;
    assert(parseExpressionTokens(notPrecedenceTokens,
                                 sizeof(notPrecedenceTokens) / sizeof(notPrecedenceTokens[0]),
                                 &expression) == COMPILER_OK);
    assert(expression->kind == ABSTRACT_SYNTAX_EXPRESSION_BINARY);
    assert(strcmp(expression->text, "*") == 0);
    assert(expression->children[0]->kind == ABSTRACT_SYNTAX_UNARY_EXPRESSION);
    abstractSyntaxNodeDestroy(expression);

    static const TestToken ifWithoutElseTokens[] = {
        TEST_TOKEN(TOKEN_KEYWORD, "if", 1, 1),     {TOKEN_BOOL_LITERAL, "true", 1, 4, 1},
        TEST_TOKEN(TOKEN_KEYWORD, "then", 1, 9),   TEST_TOKEN(TOKEN_NEWLINE, NULL, 1, 13),
        TEST_TOKEN(TOKEN_INDENT, NULL, 2, 5),      TEST_TOKEN(TOKEN_IDENTIFIER, "print", 2, 5),
        TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 2, 10),  {TOKEN_INT_LITERAL, "1", 2, 11, 1},
        TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 2, 12), TEST_TOKEN(TOKEN_NEWLINE, NULL, 2, 13),
        TEST_TOKEN(TOKEN_DEDENT, NULL, 3, 1),      TEST_TOKEN(TOKEN_EOF, NULL, 3, 1)};
    AbstractSyntaxNode *parsedIfNode = NULL;
    assert(parseIfTokens(ifWithoutElseTokens,
                         sizeof(ifWithoutElseTokens) / sizeof(ifWithoutElseTokens[0]),
                         &parsedIfNode) == COMPILER_OK);
    assert(parsedIfNode->kind == ABSTRACT_SYNTAX_IF);
    assert(parsedIfNode->childCount == 3);
    assert(parsedIfNode->children[2]->kind == ABSTRACT_SYNTAX_BLOCK);
    assert(parsedIfNode->children[2]->childCount == 0);
    abstractSyntaxNodeDestroy(parsedIfNode);

    static const TestToken elseIfTokens[] = {TEST_TOKEN(TOKEN_KEYWORD, "if", 1, 1),
                                             {TOKEN_BOOL_LITERAL, "true", 1, 4, 1},
                                             TEST_TOKEN(TOKEN_KEYWORD, "then", 1, 9),
                                             TEST_TOKEN(TOKEN_NEWLINE, NULL, 1, 13),
                                             TEST_TOKEN(TOKEN_INDENT, NULL, 2, 5),
                                             TEST_TOKEN(TOKEN_IDENTIFIER, "print", 2, 5),
                                             TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 2, 10),
                                             {TOKEN_INT_LITERAL, "1", 2, 11, 1},
                                             TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 2, 12),
                                             TEST_TOKEN(TOKEN_NEWLINE, NULL, 2, 13),
                                             TEST_TOKEN(TOKEN_DEDENT, NULL, 3, 1),
                                             TEST_TOKEN(TOKEN_KEYWORD, "else", 3, 1),
                                             TEST_TOKEN(TOKEN_KEYWORD, "if", 3, 6),
                                             {TOKEN_BOOL_LITERAL, "false", 3, 9, 0},
                                             TEST_TOKEN(TOKEN_KEYWORD, "then", 3, 15),
                                             TEST_TOKEN(TOKEN_NEWLINE, NULL, 3, 19),
                                             TEST_TOKEN(TOKEN_INDENT, NULL, 4, 5),
                                             TEST_TOKEN(TOKEN_IDENTIFIER, "print", 4, 5),
                                             TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 4, 10),
                                             {TOKEN_INT_LITERAL, "2", 4, 11, 2},
                                             TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 4, 12),
                                             TEST_TOKEN(TOKEN_NEWLINE, NULL, 4, 13),
                                             TEST_TOKEN(TOKEN_DEDENT, NULL, 5, 1),
                                             TEST_TOKEN(TOKEN_KEYWORD, "else", 5, 1),
                                             TEST_TOKEN(TOKEN_NEWLINE, NULL, 5, 5),
                                             TEST_TOKEN(TOKEN_INDENT, NULL, 6, 5),
                                             TEST_TOKEN(TOKEN_IDENTIFIER, "print", 6, 5),
                                             TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 6, 10),
                                             {TOKEN_INT_LITERAL, "3", 6, 11, 3},
                                             TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 6, 12),
                                             TEST_TOKEN(TOKEN_NEWLINE, NULL, 6, 13),
                                             TEST_TOKEN(TOKEN_DEDENT, NULL, 7, 1),
                                             TEST_TOKEN(TOKEN_EOF, NULL, 7, 1)};
    parsedIfNode = NULL;
    assert(parseIfTokens(elseIfTokens, sizeof(elseIfTokens) / sizeof(elseIfTokens[0]),
                         &parsedIfNode) == COMPILER_OK);
    assert(parsedIfNode->kind == ABSTRACT_SYNTAX_IF);
    assert(parsedIfNode->children[2]->kind == ABSTRACT_SYNTAX_BLOCK);
    assert(parsedIfNode->children[2]->childCount == 1);
    assert(parsedIfNode->children[2]->children[0]->kind == ABSTRACT_SYNTAX_IF);
    assert(parsedIfNode->children[2]->children[0]->children[2]->kind == ABSTRACT_SYNTAX_BLOCK);
    assert(parsedIfNode->children[2]->children[0]->children[2]->childCount == 1);
    abstractSyntaxNodeDestroy(parsedIfNode);

    static const TestToken nestedFunctionCallTokens[] = {
        {TOKEN_INT_LITERAL, "2", 1, 1, 2},
        TEST_TOKEN(TOKEN_OPERATOR, "*", 1, 3),
        TEST_TOKEN(TOKEN_IDENTIFIER, "factorial", 1, 5),
        TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 1, 14),
        TEST_TOKEN(TOKEN_IDENTIFIER, "readInt", 1, 15),
        TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 1, 22),
        TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 1, 23),
        TEST_TOKEN(TOKEN_OPERATOR, "+", 1, 25),
        {TOKEN_INT_LITERAL, "1", 1, 27, 1},
        TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 1, 28),
        TEST_TOKEN(TOKEN_EOF, NULL, 1, 29)};
    expression = NULL;
    assert(parseExpressionTokens(nestedFunctionCallTokens,
                                 sizeof(nestedFunctionCallTokens) /
                                     sizeof(nestedFunctionCallTokens[0]),
                                 &expression) == COMPILER_OK);
    assert(expression->kind == ABSTRACT_SYNTAX_EXPRESSION_BINARY);
    assert(expression->text[0] == '*');
    AbstractSyntaxNode *factorialCall = expression->children[1];
    assert(factorialCall->kind == ABSTRACT_SYNTAX_FUNCTION_CALL);
    assert(factorialCall->childCount == 2);
    assert(factorialCall->children[0]->kind == ABSTRACT_SYNTAX_IDENTIFIER);
    assert(strcmp(factorialCall->children[0]->text, "factorial") == 0);
    AbstractSyntaxNode *argumentExpression = factorialCall->children[1];
    assert(argumentExpression->kind == ABSTRACT_SYNTAX_EXPRESSION_BINARY);
    assert(argumentExpression->text[0] == '+');
    assert(argumentExpression->children[0]->kind == ABSTRACT_SYNTAX_FUNCTION_CALL);
    assert(argumentExpression->children[0]->children[0]->kind == ABSTRACT_SYNTAX_IDENTIFIER);
    assert(strcmp(argumentExpression->children[0]->children[0]->text, "readInt") == 0);
    assert(argumentExpression->children[0]->childCount == 1);
    assert(argumentExpression->children[1]->kind == ABSTRACT_SYNTAX_INT_LITERAL);
    abstractSyntaxNodeDestroy(expression);

    static const TestToken expressionArgumentsTokens[] = {
        TEST_TOKEN(TOKEN_IDENTIFIER, "subStr", 1, 1),
        TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 1, 7),
        TEST_TOKEN(TOKEN_STRING_LITERAL, "abcd", 1, 8),
        TEST_TOKEN(TOKEN_COMMA, ",", 1, 14),
        {TOKEN_INT_LITERAL, "1", 1, 16, 1},
        TEST_TOKEN(TOKEN_OPERATOR, "+", 1, 18),
        {TOKEN_INT_LITERAL, "1", 1, 20, 1},
        TEST_TOKEN(TOKEN_COMMA, ",", 1, 21),
        {TOKEN_INT_LITERAL, "3", 1, 23, 3},
        TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 1, 24),
        TEST_TOKEN(TOKEN_EOF, NULL, 1, 25)};
    expression = NULL;
    assert(parseExpressionTokens(expressionArgumentsTokens,
                                 sizeof(expressionArgumentsTokens) /
                                     sizeof(expressionArgumentsTokens[0]),
                                 &expression) == COMPILER_OK);
    assert(expression->kind == ABSTRACT_SYNTAX_FUNCTION_CALL);
    assert(expression->children[0]->kind == ABSTRACT_SYNTAX_IDENTIFIER);
    assert(strcmp(expression->children[0]->text, "subStr") == 0);
    assert(expression->childCount == 4);
    assert(expression->children[1]->kind == ABSTRACT_SYNTAX_STRING_LITERAL);
    assert(expression->children[2]->kind == ABSTRACT_SYNTAX_EXPRESSION_BINARY);
    assert(expression->children[2]->text[0] == '+');
    assert(expression->children[3]->kind == ABSTRACT_SYNTAX_INT_LITERAL);
    abstractSyntaxNodeDestroy(expression);

    static const TestToken listConstructorTokens[] = {
        TEST_TOKEN(TOKEN_KEYWORD, "List", 1, 1), TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 1, 5),
        {TOKEN_INT_LITERAL, "1", 1, 6, 1},       TEST_TOKEN(TOKEN_COMMA, ",", 1, 7),
        {TOKEN_INT_LITERAL, "2", 1, 9, 2},       TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 1, 10),
        TEST_TOKEN(TOKEN_EOF, NULL, 1, 11)};
    expression = NULL;
    assert(parseExpressionTokens(listConstructorTokens,
                                 sizeof(listConstructorTokens) / sizeof(listConstructorTokens[0]),
                                 &expression) == COMPILER_OK);
    assert(expression->kind == ABSTRACT_SYNTAX_CALL_EXPRESSION);
    assert(expression->childCount == 3);
    assert(expression->children[0]->kind == ABSTRACT_SYNTAX_IDENTIFIER);
    assert(strcmp(expression->children[0]->text, "List") == 0);
    assert(expression->children[1]->kind == ABSTRACT_SYNTAX_INT_LITERAL);
    assert(expression->children[2]->kind == ABSTRACT_SYNTAX_INT_LITERAL);
    abstractSyntaxNodeDestroy(expression);

    static const TestToken listRangeTokens[] = {
        TEST_TOKEN(TOKEN_KEYWORD, "List", 1, 1),     TEST_TOKEN(TOKEN_DOT, ".", 1, 5),
        TEST_TOKEN(TOKEN_IDENTIFIER, "range", 1, 6), TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 1, 11),
        {TOKEN_INT_LITERAL, "1", 1, 12, 1},          TEST_TOKEN(TOKEN_COMMA, ",", 1, 13),
        {TOKEN_INT_LITERAL, "10", 1, 15, 10},        TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 1, 17),
        TEST_TOKEN(TOKEN_EOF, NULL, 1, 18)};
    expression = NULL;
    assert(parseExpressionTokens(listRangeTokens,
                                 sizeof(listRangeTokens) / sizeof(listRangeTokens[0]),
                                 &expression) == COMPILER_OK);
    assert(expression->kind == ABSTRACT_SYNTAX_CALL_EXPRESSION);
    assert(expression->children[0]->kind == ABSTRACT_SYNTAX_MEMBER_ACCESS);
    assert(strcmp(expression->children[0]->text, "range") == 0);
    assert(expression->children[0]->children[0]->kind == ABSTRACT_SYNTAX_IDENTIFIER);
    abstractSyntaxNodeDestroy(expression);

    static const TestToken memberOnCallTokens[] = {
        TEST_TOKEN(TOKEN_IDENTIFIER, "factory", 1, 1), TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 1, 8),
        TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 1, 9),      TEST_TOKEN(TOKEN_DOT, ".", 1, 10),
        TEST_TOKEN(TOKEN_IDENTIFIER, "head", 1, 11),   TEST_TOKEN(TOKEN_EOF, NULL, 1, 15)};
    expression = NULL;
    assert(parseExpressionTokens(memberOnCallTokens,
                                 sizeof(memberOnCallTokens) / sizeof(memberOnCallTokens[0]),
                                 &expression) == COMPILER_OK);
    assert(expression->kind == ABSTRACT_SYNTAX_MEMBER_ACCESS);
    assert(strcmp(expression->text, "head") == 0);
    assert(expression->children[0]->kind == ABSTRACT_SYNTAX_CALL_EXPRESSION);
    abstractSyntaxNodeDestroy(expression);

    static const TestToken chainedCallTokens[] = {
        TEST_TOKEN(TOKEN_IDENTIFIER, "lst", 1, 1),  TEST_TOKEN(TOKEN_DOT, ".", 1, 4),
        TEST_TOKEN(TOKEN_IDENTIFIER, "drop", 1, 5), TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 1, 9),
        {TOKEN_INT_LITERAL, "2", 1, 10, 2},         TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 1, 11),
        TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 1, 12),   {TOKEN_INT_LITERAL, "0", 1, 13, 0},
        TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 1, 14),  TEST_TOKEN(TOKEN_EOF, NULL, 1, 15)};
    expression = NULL;
    assert(parseExpressionTokens(chainedCallTokens,
                                 sizeof(chainedCallTokens) / sizeof(chainedCallTokens[0]),
                                 &expression) == COMPILER_SYNTAX_ERROR);
    assert(expression == NULL);

    static const TestToken groupedChainedCallTokens[] = {
        TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 1, 1),   TEST_TOKEN(TOKEN_IDENTIFIER, "lst", 1, 2),
        TEST_TOKEN(TOKEN_DOT, ".", 1, 5),          TEST_TOKEN(TOKEN_IDENTIFIER, "drop", 1, 6),
        TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 1, 10),  {TOKEN_INT_LITERAL, "2", 1, 11, 2},
        TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 1, 12), TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 1, 13),
        TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 1, 14),  {TOKEN_INT_LITERAL, "0", 1, 15, 0},
        TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 1, 16), TEST_TOKEN(TOKEN_EOF, NULL, 1, 17)};
    expression = NULL;
    assert(parseExpressionTokens(groupedChainedCallTokens,
                                 sizeof(groupedChainedCallTokens) /
                                     sizeof(groupedChainedCallTokens[0]),
                                 &expression) == COMPILER_OK);
    assert(expression->kind == ABSTRACT_SYNTAX_CALL_EXPRESSION);
    assert(expression->children[0]->kind == ABSTRACT_SYNTAX_CALL_EXPRESSION);
    abstractSyntaxNodeDestroy(expression);

    static const TestToken rangeToListTokens[] = {
        TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 1, 1),       {TOKEN_INT_LITERAL, "1", 1, 2, 1},
        TEST_TOKEN(TOKEN_KEYWORD, "to", 1, 4),         {TOKEN_INT_LITERAL, "5", 1, 7, 5},
        TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 1, 8),      TEST_TOKEN(TOKEN_DOT, ".", 1, 9),
        TEST_TOKEN(TOKEN_IDENTIFIER, "toList", 1, 10), TEST_TOKEN(TOKEN_EOF, NULL, 1, 16)};
    expression = NULL;
    assert(parseExpressionTokens(rangeToListTokens,
                                 sizeof(rangeToListTokens) / sizeof(rangeToListTokens[0]),
                                 &expression) == COMPILER_OK);
    assert(expression->kind == ABSTRACT_SYNTAX_MEMBER_ACCESS);
    assert(strcmp(expression->text, "toList") == 0);
    assert(expression->children[0]->kind == ABSTRACT_SYNTAX_RANGE_EXPRESSION);
    abstractSyntaxNodeDestroy(expression);

    static const TestToken steppedRangeExpressionTokens[] = {
        {TOKEN_INT_LITERAL, "1", 1, 1, 1},   TEST_TOKEN(TOKEN_KEYWORD, "to", 1, 3),
        {TOKEN_INT_LITERAL, "10", 1, 6, 10}, TEST_TOKEN(TOKEN_KEYWORD, "by", 1, 9),
        {TOKEN_INT_LITERAL, "2", 1, 12, 2},  TEST_TOKEN(TOKEN_EOF, NULL, 1, 13)};
    expression = NULL;
    assert(parseExpressionTokens(steppedRangeExpressionTokens,
                                 sizeof(steppedRangeExpressionTokens) /
                                     sizeof(steppedRangeExpressionTokens[0]),
                                 &expression) == COMPILER_OK);
    assert(expression->kind == ABSTRACT_SYNTAX_RANGE_EXPRESSION);
    assert(expression->childCount == 3);
    assert(expression->children[2]->kind == ABSTRACT_SYNTAX_INT_LITERAL);
    assert(expression->children[2]->value.intValue == 2);
    abstractSyntaxNodeDestroy(expression);

    static const TestToken steppedToListTokens[] = {
        TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 1, 1),       {TOKEN_INT_LITERAL, "2", 1, 2, 2},
        TEST_TOKEN(TOKEN_KEYWORD, "to", 1, 4),         {TOKEN_INT_LITERAL, "20", 1, 7, 20},
        TEST_TOKEN(TOKEN_KEYWORD, "by", 1, 10),        {TOKEN_INT_LITERAL, "4", 1, 13, 4},
        TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 1, 14),     TEST_TOKEN(TOKEN_DOT, ".", 1, 15),
        TEST_TOKEN(TOKEN_IDENTIFIER, "toList", 1, 16), TEST_TOKEN(TOKEN_EOF, NULL, 1, 22)};
    expression = NULL;
    assert(parseExpressionTokens(steppedToListTokens,
                                 sizeof(steppedToListTokens) / sizeof(steppedToListTokens[0]),
                                 &expression) == COMPILER_OK);
    assert(expression->kind == ABSTRACT_SYNTAX_MEMBER_ACCESS);
    assert(expression->children[0]->kind == ABSTRACT_SYNTAX_RANGE_EXPRESSION);
    assert(expression->children[0]->childCount == 3);
    abstractSyntaxNodeDestroy(expression);

    static const TestToken steppedUntilToListTokens[] = {
        TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 1, 1),       {TOKEN_INT_LITERAL, "2", 1, 2, 2},
        TEST_TOKEN(TOKEN_KEYWORD, "until", 1, 4),      {TOKEN_INT_LITERAL, "20", 1, 10, 20},
        TEST_TOKEN(TOKEN_KEYWORD, "by", 1, 13),        {TOKEN_INT_LITERAL, "4", 1, 16, 4},
        TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 1, 17),     TEST_TOKEN(TOKEN_DOT, ".", 1, 18),
        TEST_TOKEN(TOKEN_IDENTIFIER, "toList", 1, 19), TEST_TOKEN(TOKEN_EOF, NULL, 1, 25)};
    expression = NULL;
    assert(parseExpressionTokens(steppedUntilToListTokens,
                                 sizeof(steppedUntilToListTokens) /
                                     sizeof(steppedUntilToListTokens[0]),
                                 &expression) == COMPILER_OK);
    assert(expression->kind == ABSTRACT_SYNTAX_MEMBER_ACCESS);
    assert(expression->children[0]->kind == ABSTRACT_SYNTAX_RANGE_EXPRESSION);
    assert(expression->children[0]->childCount == 3);
    abstractSyntaxNodeDestroy(expression);

    static const TestToken unaryPrefixTokens[] = {
        TEST_TOKEN(TOKEN_OPERATOR, "+", 1, 1),    TEST_TOKEN(TOKEN_OPERATOR, "-", 1, 2),
        TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 1, 3),  {TOKEN_INT_LITERAL, "5", 1, 4, 5},
        TEST_TOKEN(TOKEN_OPERATOR, "+", 1, 6),    {TOKEN_INT_LITERAL, "3", 1, 8, 3},
        TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 1, 9), TEST_TOKEN(TOKEN_EOF, NULL, 1, 10)};
    expression = NULL;
    assert(parseExpressionTokens(unaryPrefixTokens,
                                 sizeof(unaryPrefixTokens) / sizeof(unaryPrefixTokens[0]),
                                 &expression) == COMPILER_OK);
    assert(expression->kind == ABSTRACT_SYNTAX_UNARY_EXPRESSION);
    assert(expression->text != NULL && expression->text[0] == '+');
    assert(expression->childCount == 1);
    AbstractSyntaxNode *unaryMinus = expression->children[0];
    assert(unaryMinus->kind == ABSTRACT_SYNTAX_UNARY_EXPRESSION);
    assert(unaryMinus->text != NULL && unaryMinus->text[0] == '-');
    assert(unaryMinus->childCount == 1);
    assert(unaryMinus->children[0]->kind == ABSTRACT_SYNTAX_EXPRESSION_BINARY);
    assert(unaryMinus->children[0]->text != NULL && unaryMinus->children[0]->text[0] == '+');
    abstractSyntaxNodeDestroy(expression);

    static const TestToken unaryPrecedenceTokens[] = {
        {TOKEN_INT_LITERAL, "2", 1, 1, 2},     TEST_TOKEN(TOKEN_OPERATOR, "*", 1, 3),
        TEST_TOKEN(TOKEN_OPERATOR, "-", 1, 5), {TOKEN_INT_LITERAL, "3", 1, 6, 3},
        TEST_TOKEN(TOKEN_OPERATOR, "+", 1, 8), {TOKEN_INT_LITERAL, "4", 1, 10, 4},
        TEST_TOKEN(TOKEN_EOF, NULL, 1, 11)};
    expression = NULL;
    assert(parseExpressionTokens(unaryPrecedenceTokens,
                                 sizeof(unaryPrecedenceTokens) / sizeof(unaryPrecedenceTokens[0]),
                                 &expression) == COMPILER_OK);
    assert(expression->kind == ABSTRACT_SYNTAX_EXPRESSION_BINARY);
    assert(expression->text != NULL && expression->text[0] == '+');
    assert(expression->children[0]->kind == ABSTRACT_SYNTAX_EXPRESSION_BINARY);
    assert(expression->children[0]->text != NULL && expression->children[0]->text[0] == '*');
    assert(expression->children[0]->children[1]->kind == ABSTRACT_SYNTAX_UNARY_EXPRESSION);
    abstractSyntaxNodeDestroy(expression);

    static const TestToken booleanExpressionTokens[] = {
        TEST_TOKEN(TOKEN_IDENTIFIER, "left", 1, 1), TEST_TOKEN(TOKEN_OPERATOR, "<=", 1, 6),
        TEST_TOKEN(TOKEN_IDENTIFIER, "right", 1, 9), TEST_TOKEN(TOKEN_EOF, NULL, 1, 14)};
    expression = NULL;
    assert(
        parseExpressionTokens(booleanExpressionTokens,
                              sizeof(booleanExpressionTokens) / sizeof(booleanExpressionTokens[0]),
                              &expression) == COMPILER_OK);
    assert(expression->kind == ABSTRACT_SYNTAX_EXPRESSION_BINARY);
    assert(expression->text != NULL && expression->text[0] == '<');
    abstractSyntaxNodeDestroy(expression);

    static const TestToken chainedComparisonTokens[] = {
        TEST_TOKEN(TOKEN_IDENTIFIER, "a", 1, 1), TEST_TOKEN(TOKEN_OPERATOR, "<", 1, 3),
        TEST_TOKEN(TOKEN_IDENTIFIER, "b", 1, 5), TEST_TOKEN(TOKEN_OPERATOR, "<", 1, 7),
        TEST_TOKEN(TOKEN_IDENTIFIER, "c", 1, 9), TEST_TOKEN(TOKEN_EOF, NULL, 1, 10)};
    expression = NULL;
    assert(
        parseExpressionTokens(chainedComparisonTokens,
                              sizeof(chainedComparisonTokens) / sizeof(chainedComparisonTokens[0]),
                              &expression) == COMPILER_SYNTAX_ERROR);
    assert(expression == NULL);

    static const TestToken callOperandTokens[] = {TEST_TOKEN(TOKEN_IDENTIFIER, "readValue", 1, 1),
                                                  TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 1, 10),
                                                  TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 1, 11),
                                                  TEST_TOKEN(TOKEN_OPERATOR, "+", 1, 13),
                                                  {TOKEN_INT_LITERAL, "1", 1, 15, 1},
                                                  TEST_TOKEN(TOKEN_EOF, NULL, 1, 16)};
    expression = NULL;
    assert(parseExpressionTokens(callOperandTokens,
                                 sizeof(callOperandTokens) / sizeof(callOperandTokens[0]),
                                 &expression) == COMPILER_OK);
    assert(expression->kind == ABSTRACT_SYNTAX_EXPRESSION_BINARY);
    assert(expression->text[0] == '+');
    assert(expression->children[0]->kind == ABSTRACT_SYNTAX_FUNCTION_CALL);
    assert(expression->children[0]->childCount == 1);
    assert(expression->children[0]->children[0]->kind == ABSTRACT_SYNTAX_IDENTIFIER);
    assert(expression->children[1]->kind == ABSTRACT_SYNTAX_INT_LITERAL);
    abstractSyntaxNodeDestroy(expression);

    static const TestToken singleLineForTokens[] = {
        TEST_TOKEN(TOKEN_KEYWORD, "for", 1, 1),    TEST_TOKEN(TOKEN_IDENTIFIER, "i", 1, 5),
        TEST_TOKEN(TOKEN_OPERATOR, "<-", 1, 7),    {TOKEN_INT_LITERAL, "1", 1, 10, 1},
        TEST_TOKEN(TOKEN_KEYWORD, "to", 1, 12),    {TOKEN_INT_LITERAL, "10", 1, 15, 10},
        TEST_TOKEN(TOKEN_KEYWORD, "by", 1, 18),    {TOKEN_INT_LITERAL, "2", 1, 21, 2},
        TEST_TOKEN(TOKEN_KEYWORD, "do", 1, 23),    TEST_TOKEN(TOKEN_NEWLINE, NULL, 1, 25),
        TEST_TOKEN(TOKEN_INDENT, NULL, 2, 5),      TEST_TOKEN(TOKEN_IDENTIFIER, "print", 2, 5),
        TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 2, 10),  TEST_TOKEN(TOKEN_IDENTIFIER, "i", 2, 11),
        TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 2, 12), TEST_TOKEN(TOKEN_NEWLINE, NULL, 2, 13),
        TEST_TOKEN(TOKEN_DEDENT, NULL, 3, 1),      TEST_TOKEN(TOKEN_EOF, NULL, 3, 1)};
    AbstractSyntaxNode *forNode = NULL;
    assert(parseForTokens(singleLineForTokens,
                          sizeof(singleLineForTokens) / sizeof(singleLineForTokens[0]),
                          &forNode) == COMPILER_OK);
    assert(forNode->kind == ABSTRACT_SYNTAX_FOR);
    assert(forNode->childCount == 2);
    AbstractSyntaxNode *forClauses = forNode->children[0];
    assert(forClauses->kind == ABSTRACT_SYNTAX_FOR_CLAUSES);
    assert(forClauses->childCount == 1);
    AbstractSyntaxNode *generator = forClauses->children[0];
    assert(generator->kind == ABSTRACT_SYNTAX_FOR_GENERATOR);
    assert(strcmp(generator->text, "i") == 0);
    assert(generator->childCount == 1);
    AbstractSyntaxNode *range = generator->children[0];
    assert(range->kind == ABSTRACT_SYNTAX_RANGE_EXPRESSION);
    assert(strcmp(range->text, "to") == 0);
    assert(range->childCount == 3);
    assert(range->children[0]->kind == ABSTRACT_SYNTAX_INT_LITERAL);
    assert(range->children[0]->value.intValue == 1);
    assert(range->children[1]->kind == ABSTRACT_SYNTAX_INT_LITERAL);
    assert(range->children[1]->value.intValue == 10);
    assert(range->children[2]->kind == ABSTRACT_SYNTAX_INT_LITERAL);
    assert(range->children[2]->value.intValue == 2);
    assert(forNode->children[1]->kind == ABSTRACT_SYNTAX_BLOCK);
    assert(forNode->children[1]->childCount == 1);
    assert(forNode->children[1]->children[0]->kind == ABSTRACT_SYNTAX_FUNCTION_CALL);
    abstractSyntaxNodeDestroy(forNode);

    static const TestToken multiLineForTokens[] = {
        TEST_TOKEN(TOKEN_KEYWORD, "for", 1, 1),    TEST_TOKEN(TOKEN_NEWLINE, NULL, 1, 4),
        TEST_TOKEN(TOKEN_INDENT, NULL, 2, 5),      TEST_TOKEN(TOKEN_IDENTIFIER, "i", 2, 5),
        TEST_TOKEN(TOKEN_OPERATOR, "<-", 2, 7),    {TOKEN_INT_LITERAL, "0", 2, 10, 0},
        TEST_TOKEN(TOKEN_KEYWORD, "until", 2, 12), TEST_TOKEN(TOKEN_IDENTIFIER, "limit", 2, 18),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 2, 23),    TEST_TOKEN(TOKEN_IDENTIFIER, "j", 3, 5),
        TEST_TOKEN(TOKEN_OPERATOR, "<-", 3, 7),    {TOKEN_INT_LITERAL, "1", 3, 10, 1},
        TEST_TOKEN(TOKEN_KEYWORD, "to", 3, 12),    {TOKEN_INT_LITERAL, "10", 3, 15, 10},
        TEST_TOKEN(TOKEN_KEYWORD, "by", 3, 18),    {TOKEN_INT_LITERAL, "2", 3, 21, 2},
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 3, 22),    TEST_TOKEN(TOKEN_KEYWORD, "if", 4, 5),
        TEST_TOKEN(TOKEN_IDENTIFIER, "i", 4, 8),   TEST_TOKEN(TOKEN_OPERATOR, "!=", 4, 10),
        {TOKEN_INT_LITERAL, "3", 4, 13, 3},        TEST_TOKEN(TOKEN_NEWLINE, NULL, 4, 14),
        TEST_TOKEN(TOKEN_KEYWORD, "if", 5, 5),     TEST_TOKEN(TOKEN_IDENTIFIER, "i", 5, 8),
        TEST_TOKEN(TOKEN_OPERATOR, "<", 5, 10),    {TOKEN_INT_LITERAL, "8", 5, 12, 8},
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 5, 13),    TEST_TOKEN(TOKEN_DEDENT, NULL, 6, 1),
        TEST_TOKEN(TOKEN_KEYWORD, "do", 6, 1),     TEST_TOKEN(TOKEN_NEWLINE, NULL, 6, 3),
        TEST_TOKEN(TOKEN_INDENT, NULL, 7, 5),      TEST_TOKEN(TOKEN_IDENTIFIER, "print", 7, 5),
        TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 7, 10),  TEST_TOKEN(TOKEN_IDENTIFIER, "i", 7, 11),
        TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 7, 12), TEST_TOKEN(TOKEN_NEWLINE, NULL, 7, 13),
        TEST_TOKEN(TOKEN_DEDENT, NULL, 8, 1),      TEST_TOKEN(TOKEN_EOF, NULL, 8, 1)};
    forNode = NULL;
    assert(parseForTokens(multiLineForTokens,
                          sizeof(multiLineForTokens) / sizeof(multiLineForTokens[0]),
                          &forNode) == COMPILER_OK);
    assert(forNode->kind == ABSTRACT_SYNTAX_FOR);
    forClauses = forNode->children[0];
    assert(forClauses->kind == ABSTRACT_SYNTAX_FOR_CLAUSES);
    assert(forClauses->childCount == 4);
    assert(forClauses->children[0]->kind == ABSTRACT_SYNTAX_FOR_GENERATOR);
    assert(strcmp(forClauses->children[0]->text, "i") == 0);
    assert(strcmp(forClauses->children[0]->children[0]->text, "until") == 0);
    assert(forClauses->children[0]->children[0]->childCount == 2);
    assert(forClauses->children[1]->kind == ABSTRACT_SYNTAX_FOR_GENERATOR);
    assert(strcmp(forClauses->children[1]->text, "j") == 0);
    assert(forClauses->children[1]->children[0]->childCount == 3);
    assert(forClauses->children[2]->kind == ABSTRACT_SYNTAX_FOR_GUARD);
    assert(forClauses->children[3]->kind == ABSTRACT_SYNTAX_FOR_GUARD);
    assert(forNode->children[1]->kind == ABSTRACT_SYNTAX_BLOCK);
    abstractSyntaxNodeDestroy(forNode);

    static const TestToken ifExpressionBlockTokens[] = {
        TEST_TOKEN(TOKEN_INDENT, NULL, 1, 1),
        TEST_TOKEN(TOKEN_KEYWORD, "var", 1, 1),
        TEST_TOKEN(TOKEN_IDENTIFIER, "x", 1, 5),
        TEST_TOKEN(TOKEN_OPERATOR, "=", 1, 7),
        TEST_TOKEN(TOKEN_OPERATOR, "-", 1, 9),
        {TOKEN_INT_LITERAL, "5", 1, 10, 5},
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 1, 11),

        TEST_TOKEN(TOKEN_KEYWORD, "val", 2, 1),
        TEST_TOKEN(TOKEN_IDENTIFIER, "absolute", 2, 5),
        TEST_TOKEN(TOKEN_OPERATOR, "=", 2, 14),
        TEST_TOKEN(TOKEN_KEYWORD, "if", 2, 16),
        TEST_TOKEN(TOKEN_IDENTIFIER, "x", 2, 19),
        TEST_TOKEN(TOKEN_OPERATOR, "<", 2, 21),
        {TOKEN_INT_LITERAL, "0", 2, 23, 0},
        TEST_TOKEN(TOKEN_KEYWORD, "then", 2, 25),
        {TOKEN_INT_LITERAL, "0", 2, 30, 0},
        TEST_TOKEN(TOKEN_OPERATOR, "-", 2, 32),
        TEST_TOKEN(TOKEN_IDENTIFIER, "x", 2, 34),
        TEST_TOKEN(TOKEN_KEYWORD, "else", 2, 36),
        TEST_TOKEN(TOKEN_IDENTIFIER, "x", 2, 41),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 2, 42),

        TEST_TOKEN(TOKEN_KEYWORD, "val", 3, 1),
        TEST_TOKEN(TOKEN_IDENTIFIER, "abs2", 3, 5),
        TEST_TOKEN(TOKEN_OPERATOR, "=", 3, 10),
        TEST_TOKEN(TOKEN_KEYWORD, "if", 3, 12),
        TEST_TOKEN(TOKEN_IDENTIFIER, "x", 3, 15),
        TEST_TOKEN(TOKEN_OPERATOR, "<", 3, 17),
        {TOKEN_INT_LITERAL, "0", 3, 19, 0},
        TEST_TOKEN(TOKEN_KEYWORD, "then", 3, 21),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 3, 25),
        TEST_TOKEN(TOKEN_INDENT, NULL, 4, 3),
        {TOKEN_INT_LITERAL, "0", 4, 3, 0},
        TEST_TOKEN(TOKEN_OPERATOR, "-", 4, 5),
        TEST_TOKEN(TOKEN_IDENTIFIER, "x", 4, 7),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 4, 8),
        TEST_TOKEN(TOKEN_DEDENT, NULL, 5, 1),
        TEST_TOKEN(TOKEN_KEYWORD, "else", 5, 1),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 5, 5),
        TEST_TOKEN(TOKEN_INDENT, NULL, 6, 3),
        TEST_TOKEN(TOKEN_IDENTIFIER, "x", 6, 3),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 6, 4),
        TEST_TOKEN(TOKEN_DEDENT, NULL, 7, 1),

        TEST_TOKEN(TOKEN_KEYWORD, "val", 7, 1),
        TEST_TOKEN(TOKEN_IDENTIFIER, "abs3", 7, 5),
        TEST_TOKEN(TOKEN_OPERATOR, "=", 7, 10),
        TEST_TOKEN(TOKEN_KEYWORD, "if", 7, 12),
        TEST_TOKEN(TOKEN_IDENTIFIER, "x", 7, 15),
        TEST_TOKEN(TOKEN_OPERATOR, "<", 7, 17),
        {TOKEN_INT_LITERAL, "0", 7, 19, 0},
        TEST_TOKEN(TOKEN_KEYWORD, "then", 7, 21),
        {TOKEN_INT_LITERAL, "0", 7, 26, 0},
        TEST_TOKEN(TOKEN_OPERATOR, "-", 7, 28),
        TEST_TOKEN(TOKEN_IDENTIFIER, "x", 7, 30),
        TEST_TOKEN(TOKEN_KEYWORD, "else", 7, 32),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 7, 36),
        TEST_TOKEN(TOKEN_INDENT, NULL, 8, 3),
        TEST_TOKEN(TOKEN_IDENTIFIER, "x", 8, 3),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 8, 4),
        TEST_TOKEN(TOKEN_DEDENT, NULL, 9, 1),

        TEST_TOKEN(TOKEN_KEYWORD, "val", 9, 1),
        TEST_TOKEN(TOKEN_IDENTIFIER, "abs4", 9, 5),
        TEST_TOKEN(TOKEN_OPERATOR, "=", 9, 10),
        TEST_TOKEN(TOKEN_KEYWORD, "if", 9, 12),
        TEST_TOKEN(TOKEN_IDENTIFIER, "x", 9, 15),
        TEST_TOKEN(TOKEN_OPERATOR, "<", 9, 17),
        {TOKEN_INT_LITERAL, "0", 9, 19, 0},
        TEST_TOKEN(TOKEN_KEYWORD, "then", 9, 21),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 9, 25),
        TEST_TOKEN(TOKEN_INDENT, NULL, 10, 3),
        {TOKEN_INT_LITERAL, "0", 10, 3, 0},
        TEST_TOKEN(TOKEN_OPERATOR, "-", 10, 5),
        TEST_TOKEN(TOKEN_IDENTIFIER, "x", 10, 7),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 10, 8),
        TEST_TOKEN(TOKEN_DEDENT, NULL, 11, 1),
        TEST_TOKEN(TOKEN_KEYWORD, "else", 11, 1),
        TEST_TOKEN(TOKEN_IDENTIFIER, "x", 11, 6),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 11, 7),

        TEST_TOKEN(TOKEN_IDENTIFIER, "print", 12, 1),
        TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 12, 6),
        TEST_TOKEN(TOKEN_IDENTIFIER, "absolute", 12, 7),
        TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 12, 15),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 12, 16),
        TEST_TOKEN(TOKEN_DEDENT, NULL, 13, 1),
        TEST_TOKEN(TOKEN_EOF, NULL, 13, 1)};
    AbstractSyntaxNode *ifExpressionBlock = NULL;
    assert(parseBlockTokens(ifExpressionBlockTokens,
                            sizeof(ifExpressionBlockTokens) / sizeof(ifExpressionBlockTokens[0]),
                            &ifExpressionBlock) == COMPILER_OK);
    assert(ifExpressionBlock->kind == ABSTRACT_SYNTAX_BLOCK);
    assert(ifExpressionBlock->childCount == 6);
    for (size_t index = 1; index <= 4; index++) {
        AbstractSyntaxNode *declaration = ifExpressionBlock->children[index];
        assert(declaration->kind == ABSTRACT_SYNTAX_VARIABLE_DECLARATION);
        AbstractSyntaxNode *ifExpression = declaration->children[0];
        assert(ifExpression->kind == ABSTRACT_SYNTAX_IF_EXPRESSION);
        assert(ifExpression->childCount == 3);
        assert(ifExpression->children[0]->kind == ABSTRACT_SYNTAX_EXPRESSION_BINARY);
        assert(ifExpression->children[1] != NULL);
        assert(ifExpression->children[2] != NULL);
    }
    assert(ifExpressionBlock->children[5]->kind == ABSTRACT_SYNTAX_FUNCTION_CALL);
    abstractSyntaxNodeDestroy(ifExpressionBlock);

    static const TestToken missingElseExpressionTokens[] = {TEST_TOKEN(TOKEN_KEYWORD, "if", 1, 1),
                                                            {TOKEN_BOOL_LITERAL, "true", 1, 4, 1},
                                                            TEST_TOKEN(TOKEN_KEYWORD, "then", 1, 9),
                                                            {TOKEN_INT_LITERAL, "1", 1, 14, 1},
                                                            TEST_TOKEN(TOKEN_EOF, NULL, 1, 15)};
    expression = NULL;
    assert(parseExpressionTokens(missingElseExpressionTokens,
                                 sizeof(missingElseExpressionTokens) /
                                     sizeof(missingElseExpressionTokens[0]),
                                 &expression) == COMPILER_SYNTAX_ERROR);
    assert(expression == NULL);

    static const TestToken ifExpressionAsOperandTokens[] = {
        {TOKEN_INT_LITERAL, "1", 1, 1, 1},        TEST_TOKEN(TOKEN_OPERATOR, "+", 1, 3),
        TEST_TOKEN(TOKEN_KEYWORD, "if", 1, 5),    {TOKEN_BOOL_LITERAL, "true", 1, 8, 1},
        TEST_TOKEN(TOKEN_KEYWORD, "then", 1, 13), {TOKEN_INT_LITERAL, "2", 1, 18, 2},
        TEST_TOKEN(TOKEN_KEYWORD, "else", 1, 20), {TOKEN_INT_LITERAL, "3", 1, 25, 3},
        TEST_TOKEN(TOKEN_OPERATOR, "*", 1, 27),   {TOKEN_INT_LITERAL, "4", 1, 29, 4},
        TEST_TOKEN(TOKEN_EOF, NULL, 1, 30)};
    expression = NULL;
    assert(parseExpressionTokens(ifExpressionAsOperandTokens,
                                 sizeof(ifExpressionAsOperandTokens) /
                                     sizeof(ifExpressionAsOperandTokens[0]),
                                 &expression) == COMPILER_OK);
    assert(expression->kind == ABSTRACT_SYNTAX_EXPRESSION_BINARY);
    assert(strcmp(expression->text, "+") == 0);
    assert(expression->children[1]->kind == ABSTRACT_SYNTAX_IF_EXPRESSION);
    assert(expression->children[1]->children[2]->kind == ABSTRACT_SYNTAX_EXPRESSION_BINARY);
    assert(strcmp(expression->children[1]->children[2]->text, "*") == 0);
    abstractSyntaxNodeDestroy(expression);

    struct LexicalAnalyzer lexerState = {
        .tokens = tokens, .tokenCount = sizeof(tokens) / sizeof(tokens[0]), .nextToken = 0};

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
    assert(body->childCount == 7);

    AbstractSyntaxNode *declaration = body->children[0];
    assert(declaration->kind == ABSTRACT_SYNTAX_VARIABLE_DECLARATION);
    assert(declaration->isMutable);
    assert(typeDescriptorGetMask(declaration->variableType) == DATA_TYPE_INT);
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
    assert(call->childCount == 2);
    assert(call->children[0]->kind == ABSTRACT_SYNTAX_IDENTIFIER);
    assert(call->children[1]->kind == ABSTRACT_SYNTAX_IDENTIFIER);

    AbstractSyntaxNode *whileNode = body->children[5];
    assert(whileNode->kind == ABSTRACT_SYNTAX_WHILE);
    assert(whileNode->childCount == 2);
    assert(whileNode->children[0]->kind == ABSTRACT_SYNTAX_EXPRESSION_BINARY);
    AbstractSyntaxNode *whileBody = whileNode->children[1];
    assert(whileBody->kind == ABSTRACT_SYNTAX_BLOCK);
    assert(whileBody->childCount == 2);

    AbstractSyntaxNode *ifNode = whileBody->children[0];
    assert(ifNode->kind == ABSTRACT_SYNTAX_IF);
    assert(ifNode->childCount == 3);
    assert(ifNode->children[1]->kind == ABSTRACT_SYNTAX_BLOCK);
    assert(ifNode->children[2]->kind == ABSTRACT_SYNTAX_BLOCK);
    assert(ifNode->children[2]->children[0]->kind == ABSTRACT_SYNTAX_EXPRESSION_STATEMENT);
    assert(whileBody->children[1]->kind == ABSTRACT_SYNTAX_FUNCTION_CALL);
    assert(body->children[6]->kind == ABSTRACT_SYNTAX_FUNCTION_CALL);

    sourceProgramDestroy(program);
    syntaxAnalyzerDestroy(syntaxAnalyzer);

    static const TestToken missingPrologueTokens[] = {TEST_TOKEN(TOKEN_KEYWORD, "def", 1, 1),
                                                      TEST_TOKEN(TOKEN_EOF, NULL, 1, 4)};
    struct LexicalAnalyzer invalidLexerState = {.tokens = missingPrologueTokens,
                                                .tokenCount = sizeof(missingPrologueTokens) /
                                                              sizeof(missingPrologueTokens[0]),
                                                .nextToken = 0};
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
        TEST_TOKEN(TOKEN_EOF, NULL, 2, 18)};
    struct LexicalAnalyzer missingEntryLexerState = {
        .tokens = missingEntryFunctionTokens,
        .tokenCount = sizeof(missingEntryFunctionTokens) / sizeof(missingEntryFunctionTokens[0]),
        .nextToken = 0};
    syntaxAnalyzer = syntaxAnalyzerCreate((LexicalAnalyzer *)&missingEntryLexerState);
    assert(syntaxAnalyzer != NULL);
    program = NULL;
    assert(syntaxAnalyzerParseProgram(syntaxAnalyzer, &program) == COMPILER_UNDEFINED_SYMBOL_ERROR);
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
        TEST_TOKEN(TOKEN_EOF, NULL, 3, 24)};
    struct LexicalAnalyzer duplicateEntryLexerState = {.tokens = duplicateEntryFunctionTokens,
                                                       .tokenCount =
                                                           sizeof(duplicateEntryFunctionTokens) /
                                                           sizeof(duplicateEntryFunctionTokens[0]),
                                                       .nextToken = 0};
    syntaxAnalyzer = syntaxAnalyzerCreate((LexicalAnalyzer *)&duplicateEntryLexerState);
    assert(syntaxAnalyzer != NULL);
    program = NULL;
    assert(syntaxAnalyzerParseProgram(syntaxAnalyzer, &program) == COMPILER_REDEFINITION_ERROR);
    assert(program == NULL);
    syntaxAnalyzerDestroy(syntaxAnalyzer);

    static const TestToken unaryExpressionTokens[] = {
        TEST_TOKEN(TOKEN_KEYWORD, "import", 1, 1),   TEST_TOKEN(TOKEN_IDENTIFIER, "ifj26", 1, 8),
        TEST_TOKEN(TOKEN_DOT, ".", 1, 13),           TEST_TOKEN(TOKEN_OPERATOR, "*", 1, 14),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 1, 15),      TEST_TOKEN(TOKEN_AT, "@", 2, 1),
        TEST_TOKEN(TOKEN_KEYWORD, "main", 2, 2),     TEST_TOKEN(TOKEN_KEYWORD, "def", 2, 7),
        TEST_TOKEN(TOKEN_IDENTIFIER, "main", 2, 11), TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 2, 15),
        TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 2, 16),   TEST_TOKEN(TOKEN_COLON, ":", 2, 17),
        TEST_TOKEN(TOKEN_KEYWORD, "Unit", 2, 19),    TEST_TOKEN(TOKEN_OPERATOR, "=", 2, 24),
        TEST_TOKEN(TOKEN_OPERATOR, "-", 2, 26),      {TOKEN_INT_LITERAL, "1", 2, 27, 1},
        TEST_TOKEN(TOKEN_EOF, NULL, 2, 28)};
    struct LexicalAnalyzer unaryLexerState = {.tokens = unaryExpressionTokens,
                                              .tokenCount = sizeof(unaryExpressionTokens) /
                                                            sizeof(unaryExpressionTokens[0]),
                                              .nextToken = 0};
    syntaxAnalyzer = syntaxAnalyzerCreate((LexicalAnalyzer *)&unaryLexerState);
    assert(syntaxAnalyzer != NULL);
    program = NULL;
    assert(syntaxAnalyzerParseProgram(syntaxAnalyzer, &program) == COMPILER_OK);
    assert(program != NULL);
    AbstractSyntaxNode *unaryFunction = program->root->children[0];
    AbstractSyntaxNode *unaryBody = unaryFunction->children[0];
    assert(unaryBody->children[0]->kind == ABSTRACT_SYNTAX_EXPRESSION_STATEMENT);
    AbstractSyntaxNode *unaryMinusNode = unaryBody->children[0]->children[0];
    assert(unaryMinusNode->kind == ABSTRACT_SYNTAX_UNARY_EXPRESSION);
    assert(unaryMinusNode->text != NULL && unaryMinusNode->text[0] == '-');
    assert(unaryMinusNode->children[0]->kind == ABSTRACT_SYNTAX_INT_LITERAL);
    sourceProgramDestroy(program);
    syntaxAnalyzerDestroy(syntaxAnalyzer);

    static const TestToken caseClassTokens[] = {TEST_TOKEN(TOKEN_KEYWORD, "import", 1, 1),
                                                TEST_TOKEN(TOKEN_IDENTIFIER, "ifj26", 1, 8),
                                                TEST_TOKEN(TOKEN_DOT, ".", 1, 13),
                                                TEST_TOKEN(TOKEN_OPERATOR, "*", 1, 14),
                                                TEST_TOKEN(TOKEN_NEWLINE, NULL, 1, 15),
                                                TEST_TOKEN(TOKEN_KEYWORD, "case", 2, 1),
                                                TEST_TOKEN(TOKEN_KEYWORD, "class", 2, 6),
                                                TEST_TOKEN(TOKEN_IDENTIFIER, "Point", 2, 12),
                                                TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 2, 17),
                                                TEST_TOKEN(TOKEN_IDENTIFIER, "x", 2, 18),
                                                TEST_TOKEN(TOKEN_COLON, ":", 2, 19),
                                                TEST_TOKEN(TOKEN_KEYWORD, "Int", 2, 21),
                                                TEST_TOKEN(TOKEN_COMMA, ",", 2, 24),
                                                TEST_TOKEN(TOKEN_IDENTIFIER, "y", 2, 26),
                                                TEST_TOKEN(TOKEN_COLON, ":", 2, 27),
                                                TEST_TOKEN(TOKEN_KEYWORD, "Int", 2, 29),
                                                TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 2, 32),
                                                TEST_TOKEN(TOKEN_NEWLINE, NULL, 2, 33),
                                                TEST_TOKEN(TOKEN_KEYWORD, "case", 3, 1),
                                                TEST_TOKEN(TOKEN_KEYWORD, "class", 3, 6),
                                                TEST_TOKEN(TOKEN_IDENTIFIER, "Empty", 3, 12),
                                                TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 3, 17),
                                                TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 3, 18),
                                                TEST_TOKEN(TOKEN_NEWLINE, NULL, 3, 19),
                                                TEST_TOKEN(TOKEN_AT, "@", 4, 1),
                                                TEST_TOKEN(TOKEN_KEYWORD, "main", 4, 2),
                                                TEST_TOKEN(TOKEN_KEYWORD, "def", 4, 7),
                                                TEST_TOKEN(TOKEN_IDENTIFIER, "main", 4, 11),
                                                TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 4, 15),
                                                TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 4, 16),
                                                TEST_TOKEN(TOKEN_COLON, ":", 4, 17),
                                                TEST_TOKEN(TOKEN_KEYWORD, "Unit", 4, 19),
                                                TEST_TOKEN(TOKEN_OPERATOR, "=", 4, 24),
                                                TEST_TOKEN(TOKEN_NEWLINE, NULL, 4, 25),
                                                TEST_TOKEN(TOKEN_INDENT, NULL, 5, 1),
                                                TEST_TOKEN(TOKEN_KEYWORD, "val", 5, 3),
                                                TEST_TOKEN(TOKEN_IDENTIFIER, "i", 5, 7),
                                                TEST_TOKEN(TOKEN_COLON, ":", 5, 9),
                                                TEST_TOKEN(TOKEN_IDENTIFIER, "Point", 5, 11),
                                                TEST_TOKEN(TOKEN_OPERATOR, "=", 5, 17),
                                                TEST_TOKEN(TOKEN_IDENTIFIER, "Point", 5, 19),
                                                TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 5, 24),
                                                {TOKEN_INT_LITERAL, "10", 5, 25, 10},
                                                TEST_TOKEN(TOKEN_COMMA, ",", 5, 27),
                                                {TOKEN_INT_LITERAL, "20", 5, 29, 20},
                                                TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 5, 31),
                                                TEST_TOKEN(TOKEN_NEWLINE, NULL, 5, 32),
                                                TEST_TOKEN(TOKEN_IDENTIFIER, "print", 6, 3),
                                                TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 6, 8),
                                                TEST_TOKEN(TOKEN_IDENTIFIER, "i", 6, 9),
                                                TEST_TOKEN(TOKEN_DOT, ".", 6, 10),
                                                TEST_TOKEN(TOKEN_IDENTIFIER, "x", 6, 11),
                                                TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 6, 12),
                                                TEST_TOKEN(TOKEN_NEWLINE, NULL, 6, 13),
                                                TEST_TOKEN(TOKEN_DEDENT, NULL, 7, 1),
                                                TEST_TOKEN(TOKEN_EOF, NULL, 7, 1)};
    struct LexicalAnalyzer caseClassLexerState = {.tokens = caseClassTokens,
                                                  .tokenCount = sizeof(caseClassTokens) /
                                                                sizeof(caseClassTokens[0]),
                                                  .nextToken = 0};
    syntaxAnalyzer = syntaxAnalyzerCreate((LexicalAnalyzer *)&caseClassLexerState);
    assert(syntaxAnalyzer != NULL);
    program = NULL;
    assert(syntaxAnalyzerParseProgram(syntaxAnalyzer, &program) == COMPILER_OK);
    assert(program->root->childCount == 3);
    AbstractSyntaxNode *pointClass = program->root->children[0];
    assert(pointClass->kind == ABSTRACT_SYNTAX_CASE_CLASS);
    assert(strcmp(pointClass->text, "Point") == 0);
    assert(pointClass->childCount == 2);
    assert(pointClass->children[0]->kind == ABSTRACT_SYNTAX_CASE_CLASS_FIELD);
    assert(strcmp(pointClass->children[0]->text, "x") == 0);
    assert(typeDescriptorGetMask(pointClass->children[0]->variableType) == DATA_TYPE_INT);
    assert(strcmp(pointClass->children[1]->text, "y") == 0);
    AbstractSyntaxNode *emptyClass = program->root->children[1];
    assert(emptyClass->kind == ABSTRACT_SYNTAX_CASE_CLASS);
    assert(strcmp(emptyClass->text, "Empty") == 0 && emptyClass->childCount == 0);
    AbstractSyntaxNode *caseMain = program->root->children[2];
    AbstractSyntaxNode *caseBody = caseMain->children[0];
    AbstractSyntaxNode *pointValue = caseBody->children[0];
    assert(pointValue->kind == ABSTRACT_SYNTAX_VARIABLE_DECLARATION);
    assert(typeDescriptorGetMask(pointValue->variableType) == DATA_TYPE_ALIAS);
    assert(strcmp(typeDescriptorGetName(pointValue->variableType), "Point") == 0);
    assert(pointValue->children[0]->kind == ABSTRACT_SYNTAX_CALL_EXPRESSION);
    assert(pointValue->children[0]->childCount == 3);
    AbstractSyntaxNode *printCall = caseBody->children[1];
    assert(printCall->kind == ABSTRACT_SYNTAX_CALL_EXPRESSION);
    assert(printCall->children[1]->kind == ABSTRACT_SYNTAX_MEMBER_ACCESS);
    assert(strcmp(printCall->children[1]->text, "x") == 0);
    sourceProgramDestroy(program);
    syntaxAnalyzerDestroy(syntaxAnalyzer);

    static const TestToken matchExpressionTokens[] = {
        TEST_TOKEN(TOKEN_IDENTIFIER, "value", 1, 1),
        TEST_TOKEN(TOKEN_KEYWORD, "match", 1, 7),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 1, 12),
        TEST_TOKEN(TOKEN_INDENT, NULL, 2, 1),
        TEST_TOKEN(TOKEN_KEYWORD, "case", 2, 3),
        {TOKEN_INT_LITERAL, "0", 2, 8, 0},
        TEST_TOKEN(TOKEN_OPERATOR, "=>", 2, 10),
        TEST_TOKEN(TOKEN_STRING_LITERAL, "zero", 2, 13),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 2, 19),
        TEST_TOKEN(TOKEN_KEYWORD, "case", 3, 3),
        TEST_TOKEN(TOKEN_IDENTIFIER, "n", 3, 8),
        TEST_TOKEN(TOKEN_COLON, ":", 3, 9),
        TEST_TOKEN(TOKEN_KEYWORD, "Int", 3, 11),
        TEST_TOKEN(TOKEN_KEYWORD, "if", 3, 15),
        TEST_TOKEN(TOKEN_IDENTIFIER, "n", 3, 18),
        TEST_TOKEN(TOKEN_OPERATOR, ">", 3, 20),
        {TOKEN_INT_LITERAL, "0", 3, 22, 0},
        TEST_TOKEN(TOKEN_OPERATOR, "=>", 3, 24),
        TEST_TOKEN(TOKEN_STRING_LITERAL, "positive", 3, 27),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 3, 37),
        TEST_TOKEN(TOKEN_KEYWORD, "case", 4, 3),
        TEST_TOKEN(TOKEN_IDENTIFIER, "Point", 4, 8),
        TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 4, 13),
        TEST_TOKEN(TOKEN_IDENTIFIER, "a", 4, 14),
        TEST_TOKEN(TOKEN_COMMA, ",", 4, 15),
        TEST_TOKEN(TOKEN_IDENTIFIER, "Inner", 4, 17),
        TEST_TOKEN(TOKEN_LEFT_PAREN, "(", 4, 22),
        TEST_TOKEN(TOKEN_INT_LITERAL, "1", 4, 23),
        TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 4, 24),
        TEST_TOKEN(TOKEN_RIGHT_PAREN, ")", 4, 25),
        TEST_TOKEN(TOKEN_OPERATOR, "=>", 4, 27),
        TEST_TOKEN(TOKEN_IDENTIFIER, "a", 4, 30),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 4, 31),
        TEST_TOKEN(TOKEN_KEYWORD, "case", 5, 3),
        TEST_TOKEN(TOKEN_IDENTIFIER, "text", 5, 8),
        TEST_TOKEN(TOKEN_COLON, ":", 5, 12),
        TEST_TOKEN(TOKEN_KEYWORD, "String", 5, 14),
        TEST_TOKEN(TOKEN_KEYWORD, "if", 5, 21),
        TEST_TOKEN(TOKEN_KEYWORD, "if", 5, 24),
        TEST_TOKEN(TOKEN_BOOL_LITERAL, "true", 5, 27),
        TEST_TOKEN(TOKEN_KEYWORD, "then", 5, 32),
        TEST_TOKEN(TOKEN_BOOL_LITERAL, "true", 5, 37),
        TEST_TOKEN(TOKEN_KEYWORD, "else", 5, 42),
        TEST_TOKEN(TOKEN_BOOL_LITERAL, "false", 5, 47),
        TEST_TOKEN(TOKEN_OPERATOR, "=>", 5, 53),
        TEST_TOKEN(TOKEN_STRING_LITERAL, "guarded", 5, 56),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 5, 65),
        TEST_TOKEN(TOKEN_KEYWORD, "case", 6, 3),
        TEST_TOKEN(TOKEN_UNDERSCORE, "_", 6, 8),
        TEST_TOKEN(TOKEN_OPERATOR, "=>", 6, 10),
        TEST_TOKEN(TOKEN_STRING_LITERAL, "other", 6, 13),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 6, 20),
        TEST_TOKEN(TOKEN_DEDENT, NULL, 7, 1),
        TEST_TOKEN(TOKEN_EOF, NULL, 7, 1)};
    AbstractSyntaxNode *matchExpression = NULL;
    assert(parseExpressionTokens(matchExpressionTokens,
                                 sizeof(matchExpressionTokens) / sizeof(matchExpressionTokens[0]),
                                 &matchExpression) == COMPILER_OK);
    assert(matchExpression->kind == ABSTRACT_SYNTAX_MATCH_EXPRESSION);
    assert(matchExpression->childCount == 6);
    assert(matchExpression->children[0]->kind == ABSTRACT_SYNTAX_IDENTIFIER);
    AbstractSyntaxNode *literalCase = matchExpression->children[1];
    assert(literalCase->kind == ABSTRACT_SYNTAX_MATCH_CASE && literalCase->childCount == 2);
    assert(literalCase->children[0]->kind == ABSTRACT_SYNTAX_INT_LITERAL);
    assert(literalCase->children[1]->kind == ABSTRACT_SYNTAX_STRING_LITERAL);
    AbstractSyntaxNode *guardedCase = matchExpression->children[2];
    assert(guardedCase->childCount == 3);
    assert(guardedCase->children[0]->kind == ABSTRACT_SYNTAX_IDENTIFIER);
    assert(strcmp(guardedCase->children[0]->text, "n") == 0);
    assert(typeDescriptorGetMask(guardedCase->children[0]->variableType) == DATA_TYPE_INT);
    assert(guardedCase->children[1]->kind == ABSTRACT_SYNTAX_MATCH_GUARD);
    assert(guardedCase->children[1]->children[0]->kind == ABSTRACT_SYNTAX_EXPRESSION_BINARY);
    AbstractSyntaxNode *constructorCase = matchExpression->children[3];
    assert(constructorCase->children[0]->kind == ABSTRACT_SYNTAX_CALL_EXPRESSION);
    assert(constructorCase->children[0]->childCount == 3);
    assert(constructorCase->children[0]->children[1]->kind == ABSTRACT_SYNTAX_IDENTIFIER);
    assert(constructorCase->children[0]->children[2]->kind == ABSTRACT_SYNTAX_CALL_EXPRESSION);
    AbstractSyntaxNode *ifGuardedCase = matchExpression->children[4];
    assert(ifGuardedCase->children[1]->kind == ABSTRACT_SYNTAX_MATCH_GUARD);
    assert(ifGuardedCase->children[1]->children[0]->kind == ABSTRACT_SYNTAX_IF_EXPRESSION);
    assert(matchExpression->children[5]->children[0]->kind == ABSTRACT_SYNTAX_IDENTIFIER);
    assert(strcmp(matchExpression->children[5]->children[0]->text, "_") == 0);
    abstractSyntaxNodeDestroy(matchExpression);

    static const TestToken matchWithoutDefaultTokens[] = {
        TEST_TOKEN(TOKEN_IDENTIFIER, "value", 1, 1),
        TEST_TOKEN(TOKEN_KEYWORD, "match", 1, 7),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 1, 12),
        TEST_TOKEN(TOKEN_INDENT, NULL, 2, 1),
        TEST_TOKEN(TOKEN_KEYWORD, "case", 2, 3),
        TEST_TOKEN(TOKEN_BOOL_LITERAL, "true", 2, 8),
        TEST_TOKEN(TOKEN_OPERATOR, "=>", 2, 13),
        TEST_TOKEN(TOKEN_BOOL_LITERAL, "false", 2, 16),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 2, 21),
        TEST_TOKEN(TOKEN_DEDENT, NULL, 3, 1),
        TEST_TOKEN(TOKEN_EOF, NULL, 3, 1)};
    assert(parseExpressionTokens(matchWithoutDefaultTokens,
                                 sizeof(matchWithoutDefaultTokens) /
                                     sizeof(matchWithoutDefaultTokens[0]),
                                 &matchExpression) == COMPILER_SYNTAX_ERROR);
    assert(matchExpression == NULL);

    static const TestToken guardedWildcardTokens[] = {
        TEST_TOKEN(TOKEN_IDENTIFIER, "value", 1, 1),
        TEST_TOKEN(TOKEN_KEYWORD, "match", 1, 7),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 1, 12),
        TEST_TOKEN(TOKEN_INDENT, NULL, 2, 1),
        TEST_TOKEN(TOKEN_KEYWORD, "case", 2, 3),
        TEST_TOKEN(TOKEN_UNDERSCORE, "_", 2, 8),
        TEST_TOKEN(TOKEN_KEYWORD, "if", 2, 10),
        TEST_TOKEN(TOKEN_BOOL_LITERAL, "true", 2, 13),
        TEST_TOKEN(TOKEN_OPERATOR, "=>", 2, 18),
        TEST_TOKEN(TOKEN_BOOL_LITERAL, "false", 2, 21),
        TEST_TOKEN(TOKEN_NEWLINE, NULL, 2, 26),
        TEST_TOKEN(TOKEN_DEDENT, NULL, 3, 1),
        TEST_TOKEN(TOKEN_EOF, NULL, 3, 1)};
    assert(parseExpressionTokens(guardedWildcardTokens,
                                 sizeof(guardedWildcardTokens) / sizeof(guardedWildcardTokens[0]),
                                 &matchExpression) == COMPILER_SYNTAX_ERROR);
    assert(matchExpression == NULL);
    return 0;
}

#undef TEST_TOKEN
