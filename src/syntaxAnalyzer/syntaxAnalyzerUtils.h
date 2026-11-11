/**
 * @file  syntaxAnalyzerUtils.h
 *
 * @brief Collection of utility functions for the syntax analyzer (parser). Includes token management, AST node creation, and parsing helpers.
 *
 * @project IFJ26 compiler
 * @author Matúš Čík
 */

#ifndef SYNTAX_ANALYZER_UTILS_H
#define SYNTAX_ANALYZER_UTILS_H

#include "abstractSyntaxTree.h"
#include "lexicalAnalyzer.h"

/** @brief State holder representing parser. 
 * @newlineConsumed is needed to parse inline 'if' expressions correctly, because
 * it's body expression can be on a new line.
*/
typedef struct SyntaxAnalyzer {
    LexicalAnalyzer *lexicalAnalyzer;
    Token *currentToken;
    Token *nextToken;
    bool newlineConsumed;
} SyntaxAnalyzer;

/* --- Loading and Advancing Tokens Functions --- */

/** @brief If currentToken is NULL, load next token from lexicalAnalyzer, else does nothing. */
CompilerStatus ensureCurrentToken(SyntaxAnalyzer *syntaxAnalyzer);

/** @brief If nextToken is NULL, runs ensureCurrentToken then loads next one from lexicalAnalyzer, else does nothing. */
CompilerStatus ensureNextToken(SyntaxAnalyzer *syntaxAnalyzer);

/** @brief Consumes the current token and loads next. */
CompilerStatus advanceToken(SyntaxAnalyzer *syntaxAnalyzer);

/** @brief Consumes consecutive newline tokens. */
CompilerStatus skipNewlines(SyntaxAnalyzer *syntaxAnalyzer);


/* --- Current Token Validation Functions --- */

/** @brief Tests the current token's lexeme without consuming it. */
bool currentIsWord(const SyntaxAnalyzer *syntaxAnalyzer, const char *word);

/** @brief Tests the current token's operator spelling without consuming it. */
bool currentIsOperator(const SyntaxAnalyzer *syntaxAnalyzer, const char *operatorText);

/** @brief Tests the current token category without consuming it. */
bool currentIsType(const SyntaxAnalyzer *syntaxAnalyzer, TokenType type);

/** @brief Requires the current lexeme to match word and consumes it. */
CompilerStatus expectWord(SyntaxAnalyzer *syntaxAnalyzer, const char *word);

/** @brief Requires the current token type and consumes the token. */
CompilerStatus expectType(SyntaxAnalyzer *syntaxAnalyzer, TokenType type);

/* --- Token Validation Functions --- */

/** @brief Returns whether token is a valid identifier token with a lexeme. */
bool isIdentifierToken(const Token *token);

/** @brief Tests whether a token has the given lexeme. */
bool tokenHasText(const Token *token, const char *text);

/** @brief Tests whether token terminates an expression. */
bool isExpressionTerminator(const Token *token);

/* --- AST Node Functions wrappers with guaranteed cleanup after failure --- */

/**
 * @brief Appends child to parent and transfers ownership on success.
 * @note Destroys child if appending fails.
 */
CompilerStatus appendOwnedChild(AbstractSyntaxNode *parent, AbstractSyntaxNode *child);

/** @brief Creates an AST node with kind and source location initialized. */
AbstractSyntaxNode *createNode(AbstractSyntaxNodeType type, SourceLocation location);

/** @brief Copies text into the node using AST-owned storage. */
CompilerStatus setNodeText(AbstractSyntaxNode *node, const char *text);

#endif
