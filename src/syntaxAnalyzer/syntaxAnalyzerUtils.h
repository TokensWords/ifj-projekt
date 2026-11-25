/**
 * @file  syntaxAnalyzerUtils.h
 *
 * @brief Collection of utility functions for the syntax analyzer (parser). Includes token
 * management, AST node creation, and parsing helpers.
 *
 * @project IFJ26 compiler
 * @author Matúš Čík xcikmat00
 */

#ifndef SYNTAX_ANALYZER_UTILS_H
#define SYNTAX_ANALYZER_UTILS_H

#include "abstractSyntaxTree.h"
#include "lexicalAnalyzer.h"

/** @brief Parser state and its buffered tokens. */
typedef struct SyntaxAnalyzer {
    LexicalAnalyzer *lexicalAnalyzer;
    Token *currentToken;
    Token *nextToken;
    bool newlineConsumed; /**< Tracks a newline already consumed by expression parsing. */
} SyntaxAnalyzer;

/* --- Loading and Advancing Tokens Functions --- */

/**
 * @brief Loads the current token if the buffer is empty.
 * @param syntaxAnalyzer The syntax analyzer to use.
 * @return COMPILER_OK on success, or the corresponding compiler error code.
 */
CompilerStatus ensureCurrentToken(SyntaxAnalyzer *syntaxAnalyzer);

/**
 * @brief Loads the current token if needed and buffers the next token.
 * @param syntaxAnalyzer The syntax analyzer to use.
 * @return COMPILER_OK on success, or the corresponding compiler error code.
 * @note At EOF, the next token stays empty.
 */
CompilerStatus ensureNextToken(SyntaxAnalyzer *syntaxAnalyzer);

/**
 * @brief Destroys the current token and replaces it with the next one.
 * @param syntaxAnalyzer The syntax analyzer to use.
 * @return COMPILER_OK on success, COMPILER_SYNTAX_ERROR at EOF, or a token read error.
 * @note If reading fails, the current token may be NULL.
 */
CompilerStatus advanceToken(SyntaxAnalyzer *syntaxAnalyzer);

/**
 * @brief Consumes consecutive newline tokens.
 * @param syntaxAnalyzer The syntax analyzer to use.
 * @return COMPILER_OK with the first non-newline token buffered, or a token read error.
 */
CompilerStatus skipNewlines(SyntaxAnalyzer *syntaxAnalyzer);

/* --- Current Token Validation Functions --- */

/**
 * @brief Compares the buffered token's spelling to a word.
 * @param syntaxAnalyzer The syntax analyzer to use.
 * @param word NUL-terminated spelling to match.
 * @return True if the current token has that spelling, or false otherwise.
 * @note The check uses the token text only.
 */
bool currentIsWord(const SyntaxAnalyzer *syntaxAnalyzer, const char *word);

/**
 * @brief Compares the buffered token's spelling to an operator.
 * @param syntaxAnalyzer The syntax analyzer to use.
 * @param operatorText NUL-terminated operator spelling to match.
 * @return True if the current token has that spelling, or false otherwise.
 * @note The check uses the token text only.
 */
bool currentIsOperator(const SyntaxAnalyzer *syntaxAnalyzer, const char *operatorText);

/**
 * @brief Checks the category of the buffered current token.
 * @param syntaxAnalyzer The syntax analyzer to use.
 * @param type Expected token category.
 * @return True if a current token is buffered and has the given category.
 */
bool currentIsType(const SyntaxAnalyzer *syntaxAnalyzer, TokenType type);

/**
 * @brief Loads the current token if needed and consumes it when its spelling matches.
 * @param syntaxAnalyzer The syntax analyzer to use.
 * @param word NUL-terminated spelling to match.
 * @return COMPILER_OK on success, COMPILER_SYNTAX_ERROR on a mismatch or at EOF,
 *     or a token read error.
 */
CompilerStatus expectWord(SyntaxAnalyzer *syntaxAnalyzer, const char *word);

/**
 * @brief Loads the current token if needed and consumes it when its category matches.
 * @param syntaxAnalyzer The syntax analyzer to use.
 * @param type Expected token category.
 * @return COMPILER_OK on success, COMPILER_SYNTAX_ERROR on a mismatch or at EOF,
 *     or a token read error.
 */
CompilerStatus expectType(SyntaxAnalyzer *syntaxAnalyzer, TokenType type);

/* --- Token Validation Functions --- */

/**
 * @brief Checks for an identifier token with a lexeme.
 * @param token The token to check. Can be NULL.
 * @return True if the token is a TOKEN_IDENTIFIER and has a lexeme.
 */
bool isIdentifierToken(const Token *token);

/**
 * @brief Compares a token's lexeme with the given text.
 * @param token The token to check. Can be NULL.
 * @param text NUL-terminated spelling to match.
 * @return True if the token has a lexeme equal to text.
 */
bool tokenHasText(const Token *token, const char *text);

/**
 * @brief Checks for a delimiter that can end an expression.
 * @param token Token to inspect. NULL is treated as a terminator.
 * @return True for an absent token, EOF, a newline, a dedent, a comma, a semicolon,
 *     a closing parenthesis, or the spelling `then` or `do`.
 */
bool isExpressionTerminator(const Token *token);

/* --- AST Node Functions wrappers with guaranteed cleanup after failure --- */

/**
 * @brief Adds a child node to its parent and takes ownership of the child.
 * @param parent Node that takes ownership on success.
 * @param child Caller-owned node to append.
 * @return COMPILER_OK on success, or COMPILER_INTERNAL_ERROR on failure.
 * @note If adding the child fails, this function destroys it.
 */
CompilerStatus appendOwnedChild(AbstractSyntaxNode *parent, AbstractSyntaxNode *child);

/**
 * @brief Creates an AST node with the given kind and source location.
 * @param type Kind of node to create.
 * @param location Source location to store.
 * @return Caller-owned node, or NULL if allocation fails.
 */
AbstractSyntaxNode *createNode(AbstractSyntaxNodeKind type, SourceLocation location);

/**
 * @brief Copies the text and stores it in the AST node.
 * @param node Node whose text is updated.
 * @param text NUL-terminated source text to copy.
 * @return COMPILER_OK on success, or COMPILER_INTERNAL_ERROR on failure.
 * @note If copying fails, the node keeps its previous text.
 */
CompilerStatus setNodeText(AbstractSyntaxNode *node, const char *text);

#endif
