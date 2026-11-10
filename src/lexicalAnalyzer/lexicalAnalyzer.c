/*
 * Project: IFJ26 compiler
 * Authors: xkubanm00
 * AI-assisted scaffold.
 * Prompt: Create lexical-analyzer lifecycle scaffolding without automaton logic.
 */

#include "lexicalAnalyzer.h"

#include "memory.h"
#include "stack.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define NUM_OF_KEYWORDS 22
#define LEN_OF_KEYWORDS 10

static int tokenLexemeCapacity = 100; // Default capacity of token lexeme

// Array of keywords
static const char keywords[NUM_OF_KEYWORDS][LEN_OF_KEYWORDS] = {
    "def",    "do",        "Double", "else", "if",    "import", "Int",     "return",
    "String", "then",      "Unit",   "val",  "var",   "while",  "Boolean", "case",
    "class",  "extension", "for",    "List", "match", "type"};

// All possible states of FSM
enum State {
    INVALID,
    START,
    UNDERSCORE_FINAL,
    LEFT_PAREN_FINAL,
    LEFT_SHARP_PAREN_FINAL,
    NEWLINE_FINAL,
    AFTER_COMMENT_CHECK,
    ID_FINAL1,
    ZERO_FINAL,
    ZERO_,
    ZERO_CHECK_ONE,
    ZERO_CHECK_ONE_,
    INT_FINAL,
    DOUBLE,
    COMMENT,
    STRING,
    ID_FINAL2,
    INT_,
    DOUBLE_NO_EXP_FINAL,
    ONE_LINE_COMMENT,
    MULTIPLE_LINE_COMMENT,
    ONE_LINE_STRING,
    ESCAPE_STRING,
    EXPONENTIAL,
    DOUBLE_NO_EXP_,
    ONE_LINE_COMMENT_FINAL,
    MULTIPLE_LINE_POSSIBLE_NESTED_COMMENT,
    MULTIPLE_LINE_COMMENT2,
    DOUBLE_NO_EXP_D_FINAL,
    ONE_LINE_STRING_FINAL,
    MULTIPLE_LINE_STRING_INSIDE,
    EXPONENTIAL_PLUS_MIN,
    EXPONENTIAL_FINAL,
    MULTIPLE_LINE_COMMENT_FINAL,
    MULTIPLE_LINE_STRING_ONE,
    EXPONENTIAL_,
    EXPONENTIAL_FINAL_D,
    MULTIPLE_LINE_STRING_TWO,
    MULTIPLE_LINE_STRING_FINAL,
    ONE_LINE_STRING_NO_EMPTY_FINAL,
    OPERATOR_FINAL,
    SINGLE_OR_DOUBLE_OP_FINAL,
    EQUAL_OPERATOR_FINAL,
    PLUS_OPERATOR_FINAL,
    LESS_OP_FINAL,
    DOUBLE_AND_OPERATOR,
    UNIFICATION_OPERATOR_FINAL,
    DOUBLE_OPERATOR_FINAL,
    RIGHT_PAREN_FINAL,
    RIGHT_SHARP_PAREN_FINAL,
    COMMA_FINAL,
    COLON_FINAL,
    SEMICOLON_FINAL,
    DOT_FINAL,
    AT_FINAL,
    INDENT_COUNT,
    TAB_CHECK
};

struct LexicalAnalyzer {
    FILE *input;             // Input file
    SourceLocation location; // Current location (line and column of lexical analyzer)

    Token *prevToken;                // Previously evaluated token
    int prevChar;                    // Previous character
    bool prevCharRead;               // If lexical analyzer has prevChar - true, otherwise false
    SourceLocation prevCharLocation; // Location of previous character

    int prevIndent;     // Previous indent
    Stack *indentStack; // Stack of previous indents
    int indentCount; // Count of indents/dedents abs(indentCount), if indentCount > 0 - indent, if
                     // indentCount < 0 - dedent
    int currentIndent;
    int parenIndent;          // Indent of logical line
    int parenDepth;           // Number of unclosed '(' or '[' before lexical analyzer head
    bool newlineEnded;        // If '\n' before - true, otherwise false
    bool logicalIndentKnown;  // If logical indent is known logicalIndentKnown = true, otherwise
                              // logicalIndentKnown = false
    bool continuationPending; // If '\n' inside brackets ()/[] - true, otherwise false
    int slashIndent;          // Indent of line with '/'
    bool slashWasPending;     // Copy of continuationPending in the time when '/' was read

    enum State state;  // Current state of lexical analyzer
    bool reachedEof;   // If lexical analyzer reached EOF (end of file) - true, otherwise false
    bool lineHasToken; // If the current line has produced token - true, otherwise false

    SourceLocation newlineLocation; // Location of '\n' - newline
};

/**
 * Creates lexical analyzer
 * @param input input file
 * @return NULL if there was a problem when creating lexical analyzer or its stack or there is no
 * input file, otherwise return pointer to lexical analyzer
 */
LexicalAnalyzer *lexicalAnalyzerCreate(FILE *input) {
    if (input == NULL) {
        return NULL;
    }

    LexicalAnalyzer *lexicalAnalyzer = memoryAllocate(sizeof(LexicalAnalyzer));
    if (lexicalAnalyzer == NULL) {
        return NULL;
    }

    lexicalAnalyzer->indentStack = stackCreate();
    if (lexicalAnalyzer->indentStack == NULL) {
        free(lexicalAnalyzer);
        return NULL;
    }
    if (!stackPush(lexicalAnalyzer->indentStack, 0)) {
        stackDestroy(lexicalAnalyzer->indentStack);
        free(lexicalAnalyzer);
        return NULL;
    }

    lexicalAnalyzer->input = input;
    lexicalAnalyzer->location.line = 1;
    lexicalAnalyzer->location.column = 1;
    lexicalAnalyzer->prevToken = NULL;
    lexicalAnalyzer->prevIndent = 0;
    lexicalAnalyzer->indentCount = 0;
    lexicalAnalyzer->currentIndent = 0;
    lexicalAnalyzer->parenIndent = 0;
    lexicalAnalyzer->parenDepth = 0;
    lexicalAnalyzer->newlineEnded = true; // To start counting currentIndent
    lexicalAnalyzer->logicalIndentKnown = false;
    lexicalAnalyzer->continuationPending = false;
    lexicalAnalyzer->slashIndent = 0;
    lexicalAnalyzer->slashWasPending = false;
    lexicalAnalyzer->state = START;
    lexicalAnalyzer->prevCharRead = false;
    lexicalAnalyzer->prevCharLocation.line = 1;
    lexicalAnalyzer->prevCharLocation.column = 1;
    lexicalAnalyzer->reachedEof = false;
    lexicalAnalyzer->lineHasToken = false;
    lexicalAnalyzer->newlineLocation.line = 1;
    lexicalAnalyzer->newlineLocation.column = 1;
    return lexicalAnalyzer;
}

/**
 * Checks if the current state is final
 * @param state state
 * @return true if the state is final, otherwise false
 */
static bool isFinal(enum State state) {
    return (state == ID_FINAL1 || state == ID_FINAL2 || state == ZERO_FINAL || state == INT_FINAL ||
            state == DOUBLE_NO_EXP_FINAL || state == DOUBLE_NO_EXP_D_FINAL ||
            state == ONE_LINE_COMMENT_FINAL || state == MULTIPLE_LINE_COMMENT_FINAL ||
            state == ONE_LINE_STRING_FINAL || state == ONE_LINE_STRING_NO_EMPTY_FINAL ||
            state == EXPONENTIAL_FINAL || state == EXPONENTIAL_FINAL_D ||
            state == MULTIPLE_LINE_STRING_FINAL || state == NEWLINE_FINAL ||
            state == OPERATOR_FINAL || state == SINGLE_OR_DOUBLE_OP_FINAL ||
            state == DOUBLE_OPERATOR_FINAL || state == LEFT_PAREN_FINAL ||
            state == RIGHT_PAREN_FINAL || state == COMMA_FINAL || state == COLON_FINAL ||
            state == SEMICOLON_FINAL || state == DOT_FINAL || state == AT_FINAL ||
            state == LESS_OP_FINAL || state == UNIFICATION_OPERATOR_FINAL ||
            state == EQUAL_OPERATOR_FINAL || state == UNDERSCORE_FINAL ||
            state == LEFT_SHARP_PAREN_FINAL || state == RIGHT_SHARP_PAREN_FINAL ||
            state == PLUS_OPERATOR_FINAL);
}

/**
 * Copies string source to destination and updates pointer to the length of destination
 * It doesn't allocate or free any memory (meant for copying only "" or "\n")
 * @param destination destination string which is being modified
 * @param source source string which is being copied
 * @param length pointer to the length of the destination string
 */
static void copyString(char *destination, char *source, int *length) {
    strcpy(destination, source);
    *length = (int)strlen(source);
}

/**
 * Removes all white characters from the beginning of the string
 * @param string string which is being modified
 * @param length pointer to a length of string - can be modified
 * @param position pointer to a starting position (column) of the string - can be modified
 * @return false if memory allocation of the new string fails, otherwise true
 */
static bool removeWhiteChars(char *string, int *length, size_t *position) {
    char *clearString = malloc(tokenLexemeCapacity * sizeof(char));
    if (clearString == NULL) {
        return false;
    }
    int idx1 = 0, idx2 = 0;
    while (string[idx1] == '\t' || string[idx1] == ' ' || string[idx1] == '\v' ||
           string[idx1] == '\f' || string[idx1] == '\r') {
        idx1++;
    }
    *position += idx1;
    while (idx1 < *length) {
        clearString[idx2] = string[idx1];
        idx1++;
        idx2++;
    }
    clearString[idx2] = '\0';
    strcpy(string, clearString);
    *length = idx2;
    free(clearString);
    return true;
}

/**
 * Decodes all escape sequences (for one line string "...") eg. "\n" -> '\n'
 * @param string string which is being modified
 * @param length pointer to a length of string - can be modified
 * @return false if memory allocation of the new string fails, otherwise true
 */
static bool decodeEscapeSeq(char *string, int *length) {
    char *clearString = malloc(tokenLexemeCapacity * sizeof(char));
    if (clearString == NULL) {
        return false;
    }
    int idx2 = 0;
    for (int idx1 = 0; idx1 < *length; idx1++) {
        if (string[idx1] == '\\' &&
            (string[idx1 + 1] == '\\' || string[idx1 + 1] == '"' || string[idx1 + 1] == 'n' ||
             string[idx1 + 1] == 'r' || string[idx1 + 1] == 't')) {
            if (string[idx1 + 1] == '\\') {
                clearString[idx2] = '\\';
            } else if (string[idx1 + 1] == '"') {
                clearString[idx2] = '\"';
            } else if (string[idx1 + 1] == 'n') {
                clearString[idx2] = '\n';
            } else if (string[idx1 + 1] == 'r') {
                clearString[idx2] = '\r';
            } else if (string[idx1 + 1] == 't') {
                clearString[idx2] = '\t';
            }
            idx1++;
            idx2++;
        } else {
            clearString[idx2] = string[idx1];
            idx2++;
        }
    }
    clearString[idx2] = '\0';
    strcpy(string, clearString);
    *length = idx2;
    free(clearString);
    return true;
}

/**
 * removes all '"' or '"""' (in multiline strings) from the beginning and end of the string
 * @param string string which is being modified
 * @param length pointer to a length of string - can be modified
 * @return false if memory allocation of the new string fails, otherwise true
 */
static bool removeUnnecessaryFromString(char *string, int *length) {
    char *clearString = malloc(tokenLexemeCapacity * sizeof(char));
    if (clearString == NULL) {
        return false;
    }
    int idx1 = 0, idx2 = 0;
    if (string[idx1] == '"') {
        if (string[idx1 + 1] == '"' && string[idx1 + 2] == '"') {
            idx1 += 3;
            *length -= 3;
        } else {
            idx1 += 1;
            *length -= 1;
        }
    }
    while (idx1 < *length) {
        clearString[idx2] = string[idx1];
        idx1++;
        idx2++;
    }
    clearString[idx2] = '\0';
    strcpy(string, clearString);
    *length = idx2;
    free(clearString);
    return true;
}

/**
 * Removes all '_', 'd', 'D' and whitespaces from number string eg. "9_000_000" -> "9000000"
 * @param string string which is being modified
 * @param length pointer to a length of string - can be modified
 * @return false if memory allocation of the new string fails, otherwise true
 */
static bool removeUnnecessaryFromNumber(char *string, int *length) {
    char *clearString = malloc(tokenLexemeCapacity * sizeof(char));
    if (clearString == NULL) {
        return false;
    }
    int idx2 = 0;
    for (int idx1 = 0; string[idx1] != '\0'; idx1++) {
        if (string[idx1] != '_' && string[idx1] != 'd' && string[idx1] != 'D' &&
            string[idx1] != '\t' && string[idx1] != '\v' && string[idx1] != '\f' &&
            string[idx1] != ' ') {
            clearString[idx2] = string[idx1];
            idx2++;
        }
    }
    clearString[idx2] = '\0';
    strcpy(string, clearString);
    *length = idx2;
    free(clearString);
    return true;
}

/**
 * Makes int from string eg. "9000000" (string) -> 9000000 (int)
 * @param string string number
 * @param endptr pointer to endpointer (for error handling)
 * @return converted int
 */
static int stringToInt(char *string, char **endptr) {
    long longResult = strtol(string, endptr, 10);

    return (int)longResult;
}

/**
 * Makes double from string eg. "54.01" (string) -> 54.01 (double)
 * @param string string number
 * @param endptr pointer to endpointer (for error handling)
 * @return converted double
 */
static double stringToDouble(char *string, char **endptr) {
    return strtod(string, endptr);
}

/**
 * Frees indent stack and previous token from lexical analyzer and frees itself
 * @param lexicalAnalyzer lexical analyzer which is being destroyed
 */
void lexicalAnalyzerDestroy(LexicalAnalyzer *lexicalAnalyzer) {
    if (lexicalAnalyzer == NULL) {
        return;
    }
    stackDestroy(lexicalAnalyzer->indentStack);
    tokenDestroy(lexicalAnalyzer->prevToken);
    free(lexicalAnalyzer);
}

/**
 * Frees token lexeme and token
 * @param token double pointer to the token
 * @param tokenLexeme pointer to the token lexeme
 */
static void tokenAndLexemeDestroy(Token **token, char *tokenLexeme) {
    tokenDestroy(*token);
    *token = NULL;
    free(tokenLexeme);
}

/**
 * Appends character to the string
 * @param string string which is being modified
 * @param lenght pointer to a length of string - will be modified
 * @param character character to append
 * @return false if reallocation of the string fails, otherwise true
 */
static bool stringAppend(char **string, int *length, int character) {
    if ((*length) + 1 >= tokenLexemeCapacity) {
        int tempCapacity = tokenLexemeCapacity * 2;
        char *tmpString = realloc(*string, tempCapacity * sizeof(char));
        if (tmpString == NULL) {
            return false;
        }
        tokenLexemeCapacity = tempCapacity;
        *string = tmpString;
    }
    (*string)[*length] = character;
    (*length)++;
    (*string)[*length] = '\0';
    return true;
}

/**
 * Checks if the lexeme (string) is keyword (is in the keywords array)
 * @param lexeme lexeme to be checked
 * @param keywords array of keywords
 * @return true if the lexeme is keyword, otherwise false
 */
static bool isKeyword(char *lexeme, const char keywords[][LEN_OF_KEYWORDS]) {
    for (int idx = 0; idx < NUM_OF_KEYWORDS; idx++) {
        if (strcmp(lexeme, keywords[idx]) == 0) {
            return true;
        }
    }
    return false;
}

/**
 * Removes the last character from string
 * @param string string which is being modified
 * @param lenght pointer to a length of string - will be modified
 */
static void stringShortenByOne(char *string, int *length) {
    string[(*length) - 1] = '\0';
    (*length)--;
}

/**
 * Sets token location (line and column)
 * @param token double pointer to the token
 * @param location final location (contains line and column)
 */
static void setTokenLocation(Token **token, SourceLocation location) {
    (*token)->location.line = location.line;
    (*token)->location.column = location.column;
}

/**
 * Checks indent of next line inside brackets (parenIndent should be smaller than the current
 * indent) If logical indent isn't known - set parenIndent to indent and logicalIndentKnown to true
 * @param lexicalAnalyzer pointer to the lexical analyzer
 * @param indent number of spaces of the physical line with token (current indent)
 * @param needsCheck if physical line inside brackets ended before this token - true, otherwise
 * false
 * @return true if not waiting check or current indent > paren indent
 */
static bool beginRealToken(LexicalAnalyzer *lexicalAnalyzer, int indent, bool needsCheck) {
    if (needsCheck && (indent <= lexicalAnalyzer->parenIndent)) {
        return false;
    }

    if (!lexicalAnalyzer->logicalIndentKnown) {
        lexicalAnalyzer->parenIndent = indent;

        lexicalAnalyzer->logicalIndentKnown = true;
    }

    return true;
}

/**
 * Calculates indent count using indent stack
 * eg. indent stack [0, 2, 6, 8], 8 -> 2 => indent count = -2 = 2x dedent
 * @param lexicalAnalyzer pointer to the lexical analyzer
 * @param currentIndent current indent (in the example case currentIndent = 2)
 * @param error type of compiler status when calculation fails
 * @return false if push to the indent stack fails or current indent is unknown (not in the indent
 * stack), otherwise true
 */
static bool calculateIndentCount(LexicalAnalyzer *lexicalAnalyzer, int currentIndent,
                                 CompilerStatus *error) {
    if (currentIndent > lexicalAnalyzer->prevIndent) // Indent (stack push new indent)
    {
        lexicalAnalyzer->indentCount++;
        if (!stackPush(lexicalAnalyzer->indentStack, currentIndent)) {
            *error = COMPILER_INTERNAL_ERROR;
            return false;
        }
    } else if (currentIndent <
               lexicalAnalyzer->prevIndent) // Dedent (number of dedents = number of stack pops)
    {
        while (currentIndent < stackTop(lexicalAnalyzer->indentStack)) {
            lexicalAnalyzer->indentCount--;
            stackPop(lexicalAnalyzer->indentStack);
        }
        if (currentIndent != stackTop(lexicalAnalyzer->indentStack)) {
            *error = COMPILER_LEXICAL_ERROR;
            return false;
        }
    }
    return true;
}

/**
 * All possible cases and states from START state of lexical analyzer
 * @param character current character that is being read
 * @param lexicalAnalyzer pointer to the lexical analyzer
 * @param token double pointer to the token
 * @param tokenLexeme current token lexeme
 * @param tokenLexemeLength pointer to the token lexeme length - can be modified
 * @param error type of compiler status when something fails
 * @return next state of lexical analyzer based of the input (current char) and other factors
 */
static enum State startCases(int character, LexicalAnalyzer *lexicalAnalyzer, Token **token,
                             char *tokenLexeme, int *tokenLexemeLength, CompilerStatus *error) {
    // If new line ended either go to indent counting state or tabulator check state
    if (lexicalAnalyzer->newlineEnded) {
        if (character == ' ') {
            lexicalAnalyzer->newlineEnded = false;
            lexicalAnalyzer->currentIndent++;
            return INDENT_COUNT;
        } else if (character == '\v' || character == '\f' || character == '\r') {
            lexicalAnalyzer->newlineEnded = false;
            return INDENT_COUNT;
        } else if (character == '\t') // tabulator
        {
            lexicalAnalyzer->newlineEnded = false;
            return TAB_CHECK;
        }
    }
    lexicalAnalyzer->newlineEnded = false;

    /* If the current character is not whitespace, newline or '/' - can be starting character of
       comment = current character is character of token*/
    if (character != ' ' && character != '\t' && character != '\n' && character != '\v' &&
        character != '\f' && character != '\r' && character != '/') {
        // If not in the brackets and don't know the logical indent yet
        if (lexicalAnalyzer->parenDepth == 0 && !lexicalAnalyzer->logicalIndentKnown) {
            if (!calculateIndentCount(lexicalAnalyzer, lexicalAnalyzer->currentIndent, error)) {
                return INVALID;
            }
            lexicalAnalyzer->prevIndent = lexicalAnalyzer->currentIndent;
        }
        // Check indent inside bracktes
        if (!beginRealToken(lexicalAnalyzer, lexicalAnalyzer->currentIndent,
                            lexicalAnalyzer->continuationPending)) {
            return INVALID;
        }
        lexicalAnalyzer->continuationPending = false;
    }

    // Possible states from START state with different characters
    if (character == '_') {
        return UNDERSCORE_FINAL;
    } else if ((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z')) {
        return ID_FINAL1;
    } else if (character == '(') {
        lexicalAnalyzer->parenDepth++;
        return LEFT_PAREN_FINAL;
    } else if (character == '[') {
        lexicalAnalyzer->parenDepth++;
        return LEFT_SHARP_PAREN_FINAL;
    } else if (character == '0') {
        return ZERO_FINAL;
    } else if (character >= '1' && character <= '9') {
        return INT_FINAL;
    } else if (character == '.') {
        return DOT_FINAL;
    } else if (character == '/') {
        lexicalAnalyzer->slashIndent = lexicalAnalyzer->currentIndent;
        // Not sure if '/' is start of comment or real token - if inside brackets, might need it
        // slashWasPending = continuationPending
        lexicalAnalyzer->slashWasPending = lexicalAnalyzer->continuationPending;
        return COMMENT;
    } else if (character == '"') {
        return STRING;
    } else if (character == '\n') // LF - newline
    {
        lexicalAnalyzer->newlineEnded = true;
        /* If the line on which was token and is not inside brackets create new token of NEWLINE
           else if line hadn't token or is inside brackets clear token lexeme and don't create new
           token*/
        if (lexicalAnalyzer->lineHasToken && lexicalAnalyzer->parenDepth == 0) {
            return NEWLINE_FINAL;
        } else {
            strcpy(tokenLexeme, "");
            *tokenLexemeLength = 0;
            setTokenLocation(token, lexicalAnalyzer->location);
            return START;
        }
    } else if (character == '-' || character == '*') {
        return OPERATOR_FINAL;
    } else if (character == '+') {
        return PLUS_OPERATOR_FINAL;
    } else if (character == '>' || character == '!') {
        return SINGLE_OR_DOUBLE_OP_FINAL;
    } else if (character == '=') {
        return EQUAL_OPERATOR_FINAL;
    } else if (character == '<') {
        return LESS_OP_FINAL;
    } else if (character == '&') {
        return DOUBLE_AND_OPERATOR;
    } else if (character == '|') {
        return UNIFICATION_OPERATOR_FINAL;
    } else if (character == ')') {
        if (lexicalAnalyzer->parenDepth > 0) {
            lexicalAnalyzer->parenDepth--;
        }
        return RIGHT_PAREN_FINAL;
    } else if (character == ']') {
        if (lexicalAnalyzer->parenDepth > 0) {
            lexicalAnalyzer->parenDepth--;
        }
        return RIGHT_SHARP_PAREN_FINAL;
    } else if (character == ',') {
        return COMMA_FINAL;
    } else if (character == ':') {
        return COLON_FINAL;
    } else if (character == ';') {
        return SEMICOLON_FINAL;
    } else if (character == '@') {
        return AT_FINAL;
    } else if (character == ' ' || character == '\t' || character == '\v' || character == '\f' ||
               character == '\r') // Casual white spaces not in the beginning of the line
    {
        return START;
    }
    return INVALID;
}

/**
 * Sets lexical analyzer state to START and saves current character as lookahead (prevChar)
 * @param lexicalAnalyzer pointer to the lexical analyzer
 * @param currentChar current character will be read in the next call
 */
static void setLexicalAnalyzerToStart(LexicalAnalyzer *lexicalAnalyzer, int currentChar) {
    lexicalAnalyzer->state = START;
    lexicalAnalyzer->prevChar = currentChar;
    lexicalAnalyzer->prevCharRead = true;
}

/**
 * Sets token lexeme, token type, (int or double value)
 * @param token double pointer to the token
 * @param tokenLexeme token lexeme
 * @param type token type
 * @return compiler status (COMPILER_INTERNAL_ERROR, COMPILER_OK)
 */
static CompilerStatus setToken(Token **token, char *tokenLexeme, TokenType type) {
    CompilerStatus compilerStatus = tokenSetLexeme(*token, tokenLexeme);
    if (compilerStatus == COMPILER_INTERNAL_ERROR) {
        tokenDestroy(*token);
        *token = NULL;
        return COMPILER_INTERNAL_ERROR;
    }

    char *endptr;
    switch (type) {
    case TOKEN_IDENTIFIER:
        if (isKeyword((*token)->lexeme, keywords)) {
            (*token)->type = TOKEN_KEYWORD;
        } else if (strcmp((*token)->lexeme, "true") == 0 ||
                   strcmp((*token)->lexeme, "false") == 0) {
            (*token)->type = TOKEN_BOOL_LITERAL;
            (*token)->value.boolValue = strcmp((*token)->lexeme, "true") == 0;
        } else {
            (*token)->type = TOKEN_IDENTIFIER;
        }
        break;
    case TOKEN_INT_LITERAL:
        (*token)->value.intValue = stringToInt(tokenLexeme, &endptr);
        if (*endptr != '\0') {
            tokenDestroy(*token);
            *token = NULL;
            return COMPILER_INTERNAL_ERROR;
        }
        (*token)->type = TOKEN_INT_LITERAL;
        break;
    case TOKEN_DOUBLE_LITERAL:
        (*token)->value.doubleValue = stringToDouble(tokenLexeme, &endptr);
        if (*endptr != '\0') {
            tokenDestroy(*token);
            *token = NULL;
            return COMPILER_INTERNAL_ERROR;
        }
        (*token)->type = TOKEN_DOUBLE_LITERAL;
        break;
    case TOKEN_UNDERSCORE:
        (*token)->type = TOKEN_UNDERSCORE;
        break;
    case TOKEN_STRING_LITERAL:
        (*token)->type = TOKEN_STRING_LITERAL;
        break;
    case TOKEN_NEWLINE:
        (*token)->type = TOKEN_NEWLINE;
        break;
    case TOKEN_OPERATOR:
        (*token)->type = TOKEN_OPERATOR;
        break;
    case TOKEN_LEFT_PAREN:
        (*token)->type = TOKEN_LEFT_PAREN;
        break;
    case TOKEN_LEFT_SHARP_PAREN:
        (*token)->type = TOKEN_LEFT_SHARP_PAREN;
        break;
    case TOKEN_RIGHT_PAREN:
        (*token)->type = TOKEN_RIGHT_PAREN;
        break;
    case TOKEN_RIGHT_SHARP_PAREN:
        (*token)->type = TOKEN_RIGHT_SHARP_PAREN;
        break;
    case TOKEN_COMMA:
        (*token)->type = TOKEN_COMMA;
        break;
    case TOKEN_COLON:
        (*token)->type = TOKEN_COLON;
        break;
    case TOKEN_SEMICOLON:
        (*token)->type = TOKEN_SEMICOLON;
        break;
    case TOKEN_DOT:
        (*token)->type = TOKEN_DOT;
        break;
    case TOKEN_AT:
        (*token)->type = TOKEN_AT;
        break;
    default:
        tokenDestroy(*token);
        *token = NULL;
        return COMPILER_INTERNAL_ERROR;
    }
    return compilerStatus;
}

/**
 * Sets token type to TOKEN_INDENT or TOKEN_DEDENT based on indentCount
 * @param lexicalAnalyzer pointer to the lexical analyzer
 * @param token double pointer to the token
 * @return COMPILER_OK
 */
static CompilerStatus indentDedent(LexicalAnalyzer *lexicalAnalyzer, Token **token) {
    if (lexicalAnalyzer->indentCount > 0) {
        lexicalAnalyzer->indentCount--;
        (*token)->type = TOKEN_INDENT;
    } else if (lexicalAnalyzer->indentCount < 0) {
        lexicalAnalyzer->indentCount++;
        (*token)->type = TOKEN_DEDENT;
    }
    return COMPILER_OK;
}

/**
 * Sets token type to TOKEN_DEDENT if previous indent > 0 (current is 0 at end of file), else sets
 * token type to TOKEN_EOF
 * @param lexicalAnalyzer pointer to the lexical analyzer
 * @param token double pointer to the token
 * @return compiler status (COMPILER_INTERNAL_ERROR, COMPILER_OK)
 */
static CompilerStatus finishAtEof(LexicalAnalyzer *lexicalAnalyzer, Token **outToken) {
    if (lexicalAnalyzer->prevIndent > 0) {
        CompilerStatus compilerError;
        if (!calculateIndentCount(lexicalAnalyzer, 0, &compilerError)) {
            return compilerError;
        }
        lexicalAnalyzer->prevIndent = 0;
        return indentDedent(lexicalAnalyzer, outToken);
    }

    (*outToken)->type = TOKEN_EOF;
    return COMPILER_OK;
}

/**
 * Edits token lexeme, creates token value and if necessary slots INDENT/DEDENT in ahead
 * @param token double pointer to the token
 * @param lexicalAnalyzer pointer to the lexical analyzer
 * @param tokenLexeme token lexeme
 * @param tokenLexemeLength pointer of the token lexeme length
 * @param type token type
 * @param reachedEof if at end of file true, otherwise false
 * @param oneLineString if current token is one line string true, otherwise false
 * @return compiler status (COMPILER_INTERNAL_ERROR, COMPILER_OK)
 */
static CompilerStatus editSetToken(Token **token, LexicalAnalyzer *lexicalAnalyzer,
                                   char *tokenLexeme, int *tokenLexemeLength, TokenType type,
                                   bool reachedEof, bool oneLineString) {
    if (!reachedEof) {
        stringShortenByOne(tokenLexeme, tokenLexemeLength);
    }

    if (!removeWhiteChars(tokenLexeme, tokenLexemeLength, &((*token)->location.column))) {
        return COMPILER_INTERNAL_ERROR;
    }

    CompilerStatus compilerStatus;
    if (type == TOKEN_INT_LITERAL || type == TOKEN_DOUBLE_LITERAL) {
        if (!removeUnnecessaryFromNumber(tokenLexeme, tokenLexemeLength)) {
            tokenDestroy(*token);
            *token = NULL;
            return COMPILER_INTERNAL_ERROR;
        }
    } else if (type == TOKEN_STRING_LITERAL) {
        if (!removeUnnecessaryFromString(tokenLexeme, tokenLexemeLength)) {
            tokenDestroy(*token);
            *token = NULL;
            return COMPILER_INTERNAL_ERROR;
        }
    }
    if (oneLineString && !decodeEscapeSeq(tokenLexeme, tokenLexemeLength)) {
        return COMPILER_INTERNAL_ERROR;
    }
    compilerStatus = setToken(token, tokenLexeme, type);

    if (compilerStatus == COMPILER_INTERNAL_ERROR) {
        return COMPILER_INTERNAL_ERROR;
    }

    // Token is recognised but have to return TOKEN_INDENT/TOKEN_DEDENT
    if (lexicalAnalyzer->indentCount != 0) {
        if (lexicalAnalyzer->prevToken != NULL) {
            return COMPILER_INTERNAL_ERROR;
        }
        Token *tempToken = tokenCreate();
        if (tempToken == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }
        lexicalAnalyzer->prevToken = *token;
        *token = tempToken;
        return indentDedent(lexicalAnalyzer, token);
    }
    return compilerStatus;
}

/**
 * Diagnoses the after comment state based on the comment type and previous character (in
 * prevCharRead condition)
 * @param token double pointer to the token
 * @param lexicalAnalyzer pointer to the lexical analyzer
 * @param tokenLexeme token lexeme
 * @param tokenLexemeLength pointer of the token lexeme length
 * @param newlineInComment pointer to the boolean which tells if the previous comment had new line
 * in it
 * @return compiler status (COMPILER_INTERNAL_ERROR, COMPILER_LEXICAL_ERROR, COMPILER_OK)
 */
static CompilerStatus diagnoseAfterComment(Token **token, LexicalAnalyzer *lexicalAnalyzer,
                                           char *tokenLexeme, int *tokenLexemeLength,
                                           bool *newlineInComment) {
    if (*newlineInComment) {
        *newlineInComment = false;
        if (lexicalAnalyzer->prevChar == '\n') {
            if (lexicalAnalyzer->lineHasToken && lexicalAnalyzer->parenDepth == 0) {
                lexicalAnalyzer->state = NEWLINE_FINAL;
                return COMPILER_OK;
            } else {
                copyString(tokenLexeme, "", tokenLexemeLength);
                setTokenLocation(token, lexicalAnalyzer->location);
                lexicalAnalyzer->state = START;
                return COMPILER_OK;
            }
        } else if (lexicalAnalyzer->prevChar == ' ' || lexicalAnalyzer->prevChar == '\t' ||
                   lexicalAnalyzer->prevChar == '\v' || lexicalAnalyzer->prevChar == '\f' ||
                   lexicalAnalyzer->prevChar == '\r') {
            lexicalAnalyzer->state = AFTER_COMMENT_CHECK;
            return COMPILER_OK;
        } else {
            return COMPILER_LEXICAL_ERROR;
        }
    } else {
        CompilerStatus error = COMPILER_LEXICAL_ERROR;
        lexicalAnalyzer->state = startCases(lexicalAnalyzer->prevChar, lexicalAnalyzer, token,
                                            tokenLexeme, tokenLexemeLength, &error);
        if (lexicalAnalyzer->state == INVALID) {
            return error;
        }
        return COMPILER_OK;
    }
}

/**
 * Updtes current and previous location of lexical analyzer and modifies its indent attributes
 * @param lexicalAnalyzer pointer to the lexical analyzer
 * @param currentChar current character
 */
static void updateLALocationAndIndent(LexicalAnalyzer *lexicalAnalyzer, char currentChar) {
    lexicalAnalyzer->prevCharLocation.line = lexicalAnalyzer->location.line;
    lexicalAnalyzer->prevCharLocation.column = lexicalAnalyzer->location.column;
    if (currentChar == '\n') {
        if (lexicalAnalyzer->state != NEWLINE_FINAL &&
            lexicalAnalyzer->state != ONE_LINE_COMMENT_FINAL) {
            lexicalAnalyzer->newlineLocation.line = lexicalAnalyzer->location.line;
            lexicalAnalyzer->newlineLocation.column = lexicalAnalyzer->location.column;
        }
        if (lexicalAnalyzer->parenDepth > 0 &&
            lexicalAnalyzer->state != MULTIPLE_LINE_STRING_INSIDE &&
            lexicalAnalyzer->state != MULTIPLE_LINE_STRING_ONE &&
            lexicalAnalyzer->state != MULTIPLE_LINE_STRING_TWO) {
            // If inside brackets and not inside multiple line string - it will need indent check
            // later
            lexicalAnalyzer->continuationPending = true;
        }
        lexicalAnalyzer->location.column = 1;
        lexicalAnalyzer->location.line++;
        lexicalAnalyzer->newlineEnded = true;
        lexicalAnalyzer->currentIndent = 0;
    } else {
        lexicalAnalyzer->location.column++;
    }
}

/**
 * Returns compiler status and sets next token attributes based on input file
 * @param outToken double pointer to the token which will be modified
 * @param lexicalAnalyzer pointer to the lexical analyzer
 * @return compiler status (COMPILER_INTERNAL_ERROR, COMPILER_LEXICAL_ERROR, COMPILER_OK)
 */
CompilerStatus lexicalAnalyzerNextToken(LexicalAnalyzer *lexicalAnalyzer, Token **outToken) {
    if (lexicalAnalyzer == NULL || outToken == NULL) {
        if (outToken != NULL) {
            *outToken = NULL;
        }
        return COMPILER_INTERNAL_ERROR;
    }

    // If lexical analyzer has some unreturned indents/dedents from previous call
    if (lexicalAnalyzer->indentCount != 0) {
        *outToken = tokenCreate();
        if (*outToken == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }
        return indentDedent(lexicalAnalyzer, outToken);
    } else if (lexicalAnalyzer->prevToken !=
               NULL) // Else if l.a. has unreturned token from previous call
    {
        *outToken = lexicalAnalyzer->prevToken;
        lexicalAnalyzer->prevToken = NULL;
        return COMPILER_OK;
    }

    // Create new token
    *outToken = tokenCreate();
    if (*outToken == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    // If reached end of file
    if (lexicalAnalyzer->reachedEof) {
        return finishAtEof(lexicalAnalyzer, outToken);
    }

    // Initializing variables
    int currentChar = EOF;
    char *tokenLexeme = malloc(tokenLexemeCapacity * sizeof(char));
    if (tokenLexeme == NULL) {
        tokenDestroy(*outToken);
        *outToken = NULL;
        return COMPILER_INTERNAL_ERROR;
    }
    strcpy(tokenLexeme, "");
    int tokenLexemeLength = 0, nestedComments = 0;
    bool commentEnded = false, newlineInComment = false;
    CompilerStatus compilerError = COMPILER_LEXICAL_ERROR;

    if (lexicalAnalyzer->prevCharRead) {
        /* If there is a character that was read set token location to the location of the previous
        character, in the previous call append it in tokenLexeme and set lexical analyzer state to
        the state according to the previous character that was read*/
        setTokenLocation(outToken, lexicalAnalyzer->prevCharLocation);
        lexicalAnalyzer->prevCharRead = false;
        currentChar = lexicalAnalyzer->prevChar;
        if (!stringAppend(&tokenLexeme, &tokenLexemeLength, lexicalAnalyzer->prevChar)) {
            lexicalAnalyzer->state = INVALID;
            tokenAndLexemeDestroy(outToken, tokenLexeme);
            return COMPILER_INTERNAL_ERROR;
        }

        compilerError = COMPILER_LEXICAL_ERROR;
        lexicalAnalyzer->state = startCases(currentChar, lexicalAnalyzer, outToken, tokenLexeme,
                                            &tokenLexemeLength, &compilerError);
        if (lexicalAnalyzer->state == INVALID) {
            tokenAndLexemeDestroy(outToken, tokenLexeme);
            return compilerError;
        }
    } else {
        setTokenLocation(outToken, lexicalAnalyzer->location);
    }

    // Read character one by one from the file until the token is valid or end of file reached
    CompilerStatus compilerStatus = COMPILER_INTERNAL_ERROR;
    while ((*outToken)->type == TOKEN_INVALID &&
           (currentChar = fgetc(lexicalAnalyzer->input)) != EOF) {
        if (commentEnded) {
            /* If comment ended clear the tokenLexeme and if character was read in previous call
            set token location to the location of the previous character and diagnose the state
            after comment */
            copyString(tokenLexeme, "", &tokenLexemeLength);
            commentEnded = false;
            if (lexicalAnalyzer->prevCharRead) {
                setTokenLocation(outToken, lexicalAnalyzer->prevCharLocation);
                lexicalAnalyzer->prevCharRead = false;
                if (!stringAppend(&tokenLexeme, &tokenLexemeLength, lexicalAnalyzer->prevChar)) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }

                compilerStatus = diagnoseAfterComment(outToken, lexicalAnalyzer, tokenLexeme,
                                                      &tokenLexemeLength, &newlineInComment);
                if (compilerStatus != COMPILER_OK) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return compilerStatus;
                }
            } else {
                setTokenLocation(outToken, lexicalAnalyzer->location);
            }
        }
        // Append current character in tokenLexeme
        if (!stringAppend(&tokenLexeme, &tokenLexemeLength, currentChar)) {
            lexicalAnalyzer->state = INVALID;
            tokenAndLexemeDestroy(outToken, tokenLexeme);
            return COMPILER_INTERNAL_ERROR;
        }

        updateLALocationAndIndent(lexicalAnalyzer, currentChar);

        // All possible states switch
        compilerError = COMPILER_LEXICAL_ERROR;
        switch (lexicalAnalyzer->state) {
        case START:
            lexicalAnalyzer->state = startCases(currentChar, lexicalAnalyzer, outToken, tokenLexeme,
                                                &tokenLexemeLength, &compilerError);
            if (lexicalAnalyzer->state == INVALID) {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return compilerError;
            }
            break;
        case UNDERSCORE_FINAL:
            if ((currentChar >= 'a' && currentChar <= 'z') ||
                (currentChar >= 'A' && currentChar <= 'Z') ||
                (currentChar >= '0' && currentChar <= '9') || currentChar == '_') {
                lexicalAnalyzer->state = ID_FINAL2;
            } else {
                setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_UNDERSCORE, false, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                lexicalAnalyzer->lineHasToken = true;
            }
            break;
        case INDENT_COUNT:
            if (currentChar == ' ') {
                lexicalAnalyzer->currentIndent++;
                lexicalAnalyzer->state = INDENT_COUNT;
            } else if (currentChar == '\v' || currentChar == '\f' || currentChar == '\r') {
                lexicalAnalyzer->state = INDENT_COUNT;
            } else if (currentChar == '\t') {
                lexicalAnalyzer->state = TAB_CHECK;
            } else {
                compilerError = COMPILER_LEXICAL_ERROR;
                lexicalAnalyzer->state =
                    startCases(currentChar, lexicalAnalyzer, outToken, tokenLexeme,
                               &tokenLexemeLength, &compilerError);
                if (lexicalAnalyzer->state == INVALID) {
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return compilerError;
                }
            }
            break;
        case TAB_CHECK:
            if (currentChar == '\t' || currentChar == ' ' || currentChar == '\v' ||
                currentChar == '\f' || currentChar == '\r') {
                lexicalAnalyzer->state = TAB_CHECK;
            } else if (currentChar == '\n') {
                copyString(tokenLexeme, "", &tokenLexemeLength);
                setTokenLocation(outToken, lexicalAnalyzer->location);
                lexicalAnalyzer->state = START;
            } else {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case LEFT_PAREN_FINAL:
            setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
            compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                          &tokenLexemeLength, TOKEN_LEFT_PAREN, false, false);
            if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_INTERNAL_ERROR;
            }
            lexicalAnalyzer->lineHasToken = true;
            break;
        case LEFT_SHARP_PAREN_FINAL:
            setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
            compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                          &tokenLexemeLength, TOKEN_LEFT_SHARP_PAREN, false, false);
            if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_INTERNAL_ERROR;
            }
            lexicalAnalyzer->lineHasToken = true;
            break;
        case ID_FINAL1:
            if ((currentChar >= 'a' && currentChar <= 'z') ||
                (currentChar >= 'A' && currentChar <= 'Z') ||
                (currentChar >= '0' && currentChar <= '9') || currentChar == '_') {
                lexicalAnalyzer->state = ID_FINAL1;
            } else {
                setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_IDENTIFIER, false, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                lexicalAnalyzer->lineHasToken = true;
            }
            break;
        case NEWLINE_FINAL:
            // End of logical line
            lexicalAnalyzer->logicalIndentKnown = false;  // Logical indent isn't known
            lexicalAnalyzer->continuationPending = false; // Not waiting for check
            setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
            copyString(tokenLexeme, "\n", &tokenLexemeLength);
            setTokenLocation(
                outToken, lexicalAnalyzer->newlineLocation); // Set location to the position of LF
            compilerStatus = setToken(outToken, tokenLexeme, TOKEN_NEWLINE);
            if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_INTERNAL_ERROR;
            }
            lexicalAnalyzer->lineHasToken = false;
            break;
        case AFTER_COMMENT_CHECK:
            if (currentChar == ' ' || currentChar == '\t' || currentChar == '\v' ||
                currentChar == '\f' || currentChar == '\r') {
                lexicalAnalyzer->state = AFTER_COMMENT_CHECK;
            } else if (currentChar == '\n') {
                if (lexicalAnalyzer->lineHasToken && lexicalAnalyzer->parenDepth == 0) {
                    lexicalAnalyzer->state = NEWLINE_FINAL;
                } else {
                    copyString(tokenLexeme, "", &tokenLexemeLength);
                    setTokenLocation(outToken, lexicalAnalyzer->location);
                    lexicalAnalyzer->state = START;
                }
            } else {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case ZERO_FINAL:
            if (currentChar == 'e' || currentChar == 'E') {
                lexicalAnalyzer->state = EXPONENTIAL;
            } else if (currentChar == '.') {
                lexicalAnalyzer->state = DOUBLE;
            } else if (currentChar == '0') {
                lexicalAnalyzer->state = ZERO_FINAL;
            } else if (currentChar >= '1' && currentChar <= '9') {
                lexicalAnalyzer->state = ZERO_CHECK_ONE;
            } else if (currentChar == '_') {
                lexicalAnalyzer->state = ZERO_;
            } else {
                setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_INT_LITERAL, false, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                lexicalAnalyzer->lineHasToken = true;
            }
            break;
        case ZERO_:
            if (currentChar == '0') {
                lexicalAnalyzer->state = ZERO_FINAL;
            } else if (currentChar >= '1' && currentChar <= '9') {
                lexicalAnalyzer->state = ZERO_CHECK_ONE;
            } else if (currentChar == '_') {
                lexicalAnalyzer->state = ZERO_;
            } else {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case ZERO_CHECK_ONE:
            if (currentChar >= '0' && currentChar <= '9') {
                lexicalAnalyzer->state = ZERO_CHECK_ONE;
            } else if (currentChar == 'e' || currentChar == 'E') {
                lexicalAnalyzer->state = EXPONENTIAL;
            } else if (currentChar == '_') {
                lexicalAnalyzer->state = ZERO_CHECK_ONE_;
            } else if (currentChar == '.') {
                lexicalAnalyzer->state = DOUBLE;
            } else {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case ZERO_CHECK_ONE_:
            if (currentChar >= '0' && currentChar <= '9') {
                lexicalAnalyzer->state = ZERO_CHECK_ONE;
            } else if (currentChar == '_') {
                lexicalAnalyzer->state = ZERO_CHECK_ONE_;
            } else {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case INT_FINAL:
            if (currentChar >= '0' && currentChar <= '9') {
                lexicalAnalyzer->state = INT_FINAL;
            } else if (currentChar == '_') {
                lexicalAnalyzer->state = INT_;
            } else if (currentChar == 'e' || currentChar == 'E') {
                lexicalAnalyzer->state = EXPONENTIAL;
            } else if (currentChar == '.') {
                lexicalAnalyzer->state = DOUBLE;
            } else {
                setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_INT_LITERAL, false, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                lexicalAnalyzer->lineHasToken = true;
            }
            break;
        case DOUBLE:
            if (currentChar >= '0' && currentChar <= '9') {
                lexicalAnalyzer->state = DOUBLE_NO_EXP_FINAL;
            } else {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case DOT_FINAL:
            if (currentChar >= '0' && currentChar <= '9') {
                lexicalAnalyzer->state = DOUBLE_NO_EXP_FINAL;
            } else {
                setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_DOT, false, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                lexicalAnalyzer->lineHasToken = true;
            }
            break;
        case COMMENT:
            if (currentChar == '/') {
                lexicalAnalyzer->state = ONE_LINE_COMMENT;
            } else if (currentChar == '*') {
                lexicalAnalyzer->state = MULTIPLE_LINE_COMMENT;
            } else {
                // '/' is division operator, calculate the indent count if outside brackets and
                // logical indent isn't known
                if (lexicalAnalyzer->parenDepth == 0 && !lexicalAnalyzer->logicalIndentKnown) {
                    if (!calculateIndentCount(lexicalAnalyzer, lexicalAnalyzer->currentIndent,
                                              &compilerError)) {
                        lexicalAnalyzer->state = INVALID;
                        tokenAndLexemeDestroy(outToken, tokenLexeme);
                        return compilerError;
                    }
                    lexicalAnalyzer->prevIndent = lexicalAnalyzer->currentIndent;
                } // beginRealToken edits parenIndent and checks indent if needed
                if (!beginRealToken(lexicalAnalyzer, lexicalAnalyzer->slashIndent,
                                    lexicalAnalyzer->slashWasPending)) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_LEXICAL_ERROR;
                }

                if (currentChar != '\n') {
                    /* Indent check already done, if current character is not new line -
                     * continuationPending = false */
                    lexicalAnalyzer->continuationPending = false;
                }

                setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_OPERATOR, false, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                lexicalAnalyzer->lineHasToken = true;
            }
            break;
        case STRING:
            if (currentChar != '\"' && currentChar != '\\' && currentChar != '\n' &&
                currentChar != '\0') {
                lexicalAnalyzer->state = ONE_LINE_STRING;
            } else if (currentChar == '\"') {
                lexicalAnalyzer->state = ONE_LINE_STRING_FINAL;
            } else if (currentChar == '\\') {
                lexicalAnalyzer->state = ESCAPE_STRING;
            } else {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case ID_FINAL2:
            if ((currentChar >= 'a' && currentChar <= 'z') ||
                (currentChar >= 'A' && currentChar <= 'Z') ||
                (currentChar >= '0' && currentChar <= '9') || currentChar == '_') {
                lexicalAnalyzer->state = ID_FINAL2;
            } else {
                setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_IDENTIFIER, false, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                lexicalAnalyzer->lineHasToken = true;
            }
            break;
        case INT_:
            if (currentChar >= '0' && currentChar <= '9') {
                lexicalAnalyzer->state = INT_FINAL;
            } else if (currentChar == '_') {
                lexicalAnalyzer->state = INT_;
            } else {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case DOUBLE_NO_EXP_FINAL:
            if (currentChar >= '0' && currentChar <= '9') {
                lexicalAnalyzer->state = DOUBLE_NO_EXP_FINAL;
            } else if (currentChar == '_') {
                lexicalAnalyzer->state = DOUBLE_NO_EXP_;
            } else if (currentChar == 'e' || currentChar == 'E') {
                lexicalAnalyzer->state = EXPONENTIAL;
            } else if (currentChar == 'd' || currentChar == 'D') {
                lexicalAnalyzer->state = DOUBLE_NO_EXP_D_FINAL;
            } else {
                setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
                compilerStatus =
                    editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength,
                                 TOKEN_DOUBLE_LITERAL, false, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                lexicalAnalyzer->lineHasToken = true;
            }
            break;
        case ONE_LINE_COMMENT:
            if ((currentChar >= 0 && currentChar < '\n') ||
                (currentChar > '\n' && currentChar <= 127)) {
                lexicalAnalyzer->state = ONE_LINE_COMMENT;
            } else if (currentChar == '\n') {
                lexicalAnalyzer->state = ONE_LINE_COMMENT_FINAL;
            } else {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case MULTIPLE_LINE_COMMENT:
            if (currentChar == '/') {
                lexicalAnalyzer->state = MULTIPLE_LINE_POSSIBLE_NESTED_COMMENT;
            } else if (currentChar == '*') {
                lexicalAnalyzer->state = MULTIPLE_LINE_COMMENT2;
            } else {
                if (currentChar == '\n') {
                    newlineInComment = true;
                }
                lexicalAnalyzer->state = MULTIPLE_LINE_COMMENT;
            }
            break;
        case ONE_LINE_STRING:
            if (currentChar != '\"' && currentChar != '\\' && currentChar != '\n' &&
                currentChar != '\0') {
                lexicalAnalyzer->state = ONE_LINE_STRING;
            } else if (currentChar == '\\') {
                lexicalAnalyzer->state = ESCAPE_STRING;
            } else if (currentChar == '\"') {
                lexicalAnalyzer->state = ONE_LINE_STRING_NO_EMPTY_FINAL;
            } else {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case ONE_LINE_STRING_NO_EMPTY_FINAL:
            setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
            compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                          &tokenLexemeLength, TOKEN_STRING_LITERAL, false, true);
            if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_INTERNAL_ERROR;
            }
            lexicalAnalyzer->lineHasToken = true;
            break;
        case ESCAPE_STRING:
            if (currentChar == '\"' || currentChar == '\\' || currentChar == 'n' ||
                currentChar == 'r' || currentChar == 't') {
                lexicalAnalyzer->state = ONE_LINE_STRING;
            } else {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case EXPONENTIAL:
            if (currentChar == '+' || currentChar == '-') {
                lexicalAnalyzer->state = EXPONENTIAL_PLUS_MIN;
            } else if (currentChar >= '0' && currentChar <= '9') {
                lexicalAnalyzer->state = EXPONENTIAL_FINAL;
            } else {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case DOUBLE_NO_EXP_:
            if (currentChar >= '0' && currentChar <= '9') {
                lexicalAnalyzer->state = DOUBLE_NO_EXP_FINAL;
            } else if (currentChar == '_') {
                lexicalAnalyzer->state = DOUBLE_NO_EXP_;
            } else {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case ONE_LINE_COMMENT_FINAL:
            setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
            if (lexicalAnalyzer->lineHasToken && lexicalAnalyzer->parenDepth == 0) {
                setTokenLocation(outToken, lexicalAnalyzer->newlineLocation);
                compilerStatus = setToken(outToken, "\n", TOKEN_NEWLINE);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                // Set all necessary attributes to false
                lexicalAnalyzer->lineHasToken = false;
                lexicalAnalyzer->logicalIndentKnown = false;
                lexicalAnalyzer->continuationPending = false;
                copyString(tokenLexeme, "", &tokenLexemeLength);
                free(tokenLexeme);
                lexicalAnalyzer->state = START;
                return compilerStatus;
            }
            commentEnded = true;
            break;
        case MULTIPLE_LINE_POSSIBLE_NESTED_COMMENT:
            if (currentChar == '*') {
                lexicalAnalyzer->state = MULTIPLE_LINE_COMMENT;
                nestedComments++;
            } else if (currentChar == '/') {
                lexicalAnalyzer->state = MULTIPLE_LINE_POSSIBLE_NESTED_COMMENT;
            } else {
                if (currentChar == '\n') {
                    newlineInComment = true;
                }
                lexicalAnalyzer->state = MULTIPLE_LINE_COMMENT;
            }
            break;
        case MULTIPLE_LINE_COMMENT2:
            if (currentChar != '/' && currentChar != '*') {
                if (currentChar == '\n') {
                    newlineInComment = true;
                }
                lexicalAnalyzer->state = MULTIPLE_LINE_COMMENT;
            } else if (currentChar == '/') {
                if (nestedComments > 0) {
                    nestedComments--;
                    lexicalAnalyzer->state = MULTIPLE_LINE_COMMENT;
                } else {
                    lexicalAnalyzer->state = MULTIPLE_LINE_COMMENT_FINAL;
                }
            } else if (currentChar == '*') {
                lexicalAnalyzer->state = MULTIPLE_LINE_COMMENT2;
            }
            break;
        case DOUBLE_NO_EXP_D_FINAL:
            setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
            compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                          &tokenLexemeLength, TOKEN_DOUBLE_LITERAL, false, false);
            if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_INTERNAL_ERROR;
            }
            lexicalAnalyzer->lineHasToken = true;
            break;
        case ONE_LINE_STRING_FINAL:
            if (currentChar == '\"') {
                lexicalAnalyzer->state = MULTIPLE_LINE_STRING_INSIDE;
            } else {
                setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
                compilerStatus =
                    editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength,
                                 TOKEN_STRING_LITERAL, false, true);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                lexicalAnalyzer->lineHasToken = true;
            }
            break;
        case MULTIPLE_LINE_STRING_INSIDE:
            if (currentChar == '\"') {
                lexicalAnalyzer->state = MULTIPLE_LINE_STRING_ONE;
            } else if (currentChar != '\"') {
                // Check of unsupported NUL character
                if (currentChar == '\0') {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_LEXICAL_ERROR;
                }
                if (currentChar == '\n') {
                    // Newline is inside string and therfore dont count indent
                    lexicalAnalyzer->newlineEnded = false;
                }
                lexicalAnalyzer->state = MULTIPLE_LINE_STRING_INSIDE;
            }
            break;
        case EXPONENTIAL_PLUS_MIN:
            if (currentChar >= '0' && currentChar <= '9') {
                lexicalAnalyzer->state = EXPONENTIAL_FINAL;
            } else {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case EXPONENTIAL_FINAL:
            if (currentChar >= '0' && currentChar <= '9') {
                lexicalAnalyzer->state = EXPONENTIAL_FINAL;
            } else if (currentChar == '_') {
                lexicalAnalyzer->state = EXPONENTIAL_;
            } else if (currentChar == 'd' || currentChar == 'D') {
                lexicalAnalyzer->state = EXPONENTIAL_FINAL_D;
            } else {
                setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
                compilerStatus =
                    editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength,
                                 TOKEN_DOUBLE_LITERAL, false, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                lexicalAnalyzer->lineHasToken = true;
            }
            break;
        case MULTIPLE_LINE_COMMENT_FINAL:
            setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
            commentEnded = true;
            break;
        case MULTIPLE_LINE_STRING_ONE:
            if (currentChar == '\"') {
                lexicalAnalyzer->state = MULTIPLE_LINE_STRING_TWO;
            } else if (currentChar != '\"') {
                // Check of unsupported NUL character
                if (currentChar == '\0') {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_LEXICAL_ERROR;
                }
                if (currentChar == '\n') {
                    lexicalAnalyzer->newlineEnded = false;
                }
                lexicalAnalyzer->state = MULTIPLE_LINE_STRING_INSIDE;
            }
            break;
        case EXPONENTIAL_:
            if (currentChar >= '0' && currentChar <= '9') {
                lexicalAnalyzer->state = EXPONENTIAL_FINAL;
            } else if (currentChar == '_') {
                lexicalAnalyzer->state = EXPONENTIAL_;
            } else {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case EXPONENTIAL_FINAL_D:
            setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
            compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                          &tokenLexemeLength, TOKEN_DOUBLE_LITERAL, false, false);
            if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_INTERNAL_ERROR;
            }
            lexicalAnalyzer->lineHasToken = true;
            break;
        case MULTIPLE_LINE_STRING_TWO:
            if (currentChar == '\"') {
                lexicalAnalyzer->state = MULTIPLE_LINE_STRING_FINAL;
            } else if (currentChar != '\"') {
                // Check of unsupported NUL character
                if (currentChar == '\0') {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_LEXICAL_ERROR;
                }
                if (currentChar == '\n') {
                    lexicalAnalyzer->newlineEnded = false;
                }
                lexicalAnalyzer->state = MULTIPLE_LINE_STRING_INSIDE;
            }
            break;
        case MULTIPLE_LINE_STRING_FINAL:
            setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
            compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                          &tokenLexemeLength, TOKEN_STRING_LITERAL, false, false);
            if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_INTERNAL_ERROR;
            }
            lexicalAnalyzer->lineHasToken = true;
            break;
        case OPERATOR_FINAL:
            setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
            compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                          &tokenLexemeLength, TOKEN_OPERATOR, false, false);
            if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_INTERNAL_ERROR;
            }
            lexicalAnalyzer->lineHasToken = true;
            break;
        case SINGLE_OR_DOUBLE_OP_FINAL:
            if (currentChar == '=') {
                lexicalAnalyzer->state = DOUBLE_OPERATOR_FINAL;
            } else {
                setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_OPERATOR, false, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                lexicalAnalyzer->lineHasToken = true;
            }
            break;
        case EQUAL_OPERATOR_FINAL:
            if (currentChar == '=' || currentChar == '>') {
                lexicalAnalyzer->state = DOUBLE_OPERATOR_FINAL;
            } else {
                setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_OPERATOR, false, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                lexicalAnalyzer->lineHasToken = true;
            }
            break;
        case PLUS_OPERATOR_FINAL:
            if (currentChar == '+') {
                lexicalAnalyzer->state = OPERATOR_FINAL;
            } else {
                setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_OPERATOR, false, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                lexicalAnalyzer->lineHasToken = true;
                break;
            }
            break;
        case LESS_OP_FINAL:
            if (currentChar == '-' || currentChar == '=') {
                lexicalAnalyzer->state = DOUBLE_OPERATOR_FINAL;
            } else {
                setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_OPERATOR, false, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                lexicalAnalyzer->lineHasToken = true;
            }
            break;
        case DOUBLE_AND_OPERATOR:
            if (currentChar == '&') {
                lexicalAnalyzer->state = DOUBLE_OPERATOR_FINAL;
            } else {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case UNIFICATION_OPERATOR_FINAL:
            if (currentChar == '|') {
                lexicalAnalyzer->state = DOUBLE_OPERATOR_FINAL;
            } else {
                setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_OPERATOR, false, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                lexicalAnalyzer->lineHasToken = true;
            }
            break;
        case DOUBLE_OPERATOR_FINAL:
            setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
            compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                          &tokenLexemeLength, TOKEN_OPERATOR, false, false);
            if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_INTERNAL_ERROR;
            }
            lexicalAnalyzer->lineHasToken = true;
            break;
        case RIGHT_PAREN_FINAL:
            setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
            compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                          &tokenLexemeLength, TOKEN_RIGHT_PAREN, false, false);
            if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_INTERNAL_ERROR;
            }
            lexicalAnalyzer->lineHasToken = true;
            break;
        case RIGHT_SHARP_PAREN_FINAL:
            setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
            compilerStatus =
                editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength,
                             TOKEN_RIGHT_SHARP_PAREN, false, false);
            if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_INTERNAL_ERROR;
            }
            lexicalAnalyzer->lineHasToken = true;
            break;
        case COMMA_FINAL:
            setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
            compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                          &tokenLexemeLength, TOKEN_COMMA, false, false);
            if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_INTERNAL_ERROR;
            }
            lexicalAnalyzer->lineHasToken = true;
            break;
        case COLON_FINAL:
            setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
            compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                          &tokenLexemeLength, TOKEN_COLON, false, false);
            if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_INTERNAL_ERROR;
            }
            lexicalAnalyzer->lineHasToken = true;
            break;
        case SEMICOLON_FINAL:
            setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
            compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                          &tokenLexemeLength, TOKEN_SEMICOLON, false, false);
            if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_INTERNAL_ERROR;
            }
            lexicalAnalyzer->lineHasToken = true;
            break;
        case AT_FINAL:
            setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
            compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                          &tokenLexemeLength, TOKEN_AT, false, false);
            if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_INTERNAL_ERROR;
            }
            lexicalAnalyzer->lineHasToken = true;
            break;
        case INVALID:
            break;
        default:
            compilerError = COMPILER_LEXICAL_ERROR;
            lexicalAnalyzer->state = startCases(currentChar, lexicalAnalyzer, outToken, tokenLexeme,
                                                &tokenLexemeLength, &compilerError);
            if (lexicalAnalyzer->state == INVALID) {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return compilerError;
            }
            break;
        }
    }

    if (commentEnded) {
        strcpy(tokenLexeme, "");
        tokenLexemeLength = 0;
        commentEnded = false;
        if (lexicalAnalyzer->prevCharRead) {
            setTokenLocation(outToken, lexicalAnalyzer->prevCharLocation);
            lexicalAnalyzer->prevCharRead = false;
            if (!stringAppend(&tokenLexeme, &tokenLexemeLength, lexicalAnalyzer->prevChar)) {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_INTERNAL_ERROR;
            }
            compilerStatus = diagnoseAfterComment(outToken, lexicalAnalyzer, tokenLexeme,
                                                  &tokenLexemeLength, &newlineInComment);
            if (compilerStatus != COMPILER_OK) {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return compilerStatus;
            }
        } else {
            setTokenLocation(outToken, lexicalAnalyzer->location);
        }
    }

    if (currentChar == EOF) {
        if (ferror(lexicalAnalyzer->input) != 0) {
            lexicalAnalyzer->state = INVALID;
            tokenAndLexemeDestroy(outToken, tokenLexeme);
            return COMPILER_INTERNAL_ERROR;
        }
        lexicalAnalyzer->reachedEof = true;
    }

    if (lexicalAnalyzer->reachedEof) {
        if (isFinal(lexicalAnalyzer->state)) {
            switch (lexicalAnalyzer->state) {
            case UNDERSCORE_FINAL:
                lexicalAnalyzer->state = START;
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_UNDERSCORE, true, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                break;
            case ID_FINAL1:
                lexicalAnalyzer->state = START;
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_IDENTIFIER, true, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                break;
            case NEWLINE_FINAL:
                lexicalAnalyzer->logicalIndentKnown = false;
                lexicalAnalyzer->continuationPending = false;
                lexicalAnalyzer->state = START;
                copyString(tokenLexeme, "\n", &tokenLexemeLength);
                setTokenLocation(outToken, lexicalAnalyzer->newlineLocation);
                compilerStatus = setToken(outToken, tokenLexeme, TOKEN_NEWLINE);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                lexicalAnalyzer->lineHasToken = false;
                break;
            case ONE_LINE_COMMENT_FINAL:
                if (lexicalAnalyzer->lineHasToken && lexicalAnalyzer->parenDepth == 0) {
                    lexicalAnalyzer->state = START;
                    setTokenLocation(outToken, lexicalAnalyzer->prevCharLocation);
                    compilerStatus = setToken(outToken, "\n", TOKEN_NEWLINE);
                    if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                        lexicalAnalyzer->state = INVALID;
                        tokenAndLexemeDestroy(outToken, tokenLexeme);
                        return COMPILER_INTERNAL_ERROR;
                    }
                    lexicalAnalyzer->lineHasToken = false;
                    lexicalAnalyzer->logicalIndentKnown = false;
                    lexicalAnalyzer->continuationPending = false;
                } else {
                    (*outToken)->type = TOKEN_EOF;
                    compilerStatus = COMPILER_OK;
                    lexicalAnalyzer->state = START;
                }
                break;
            case ID_FINAL2:
                lexicalAnalyzer->state = START;
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_IDENTIFIER, true, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                break;
            case ZERO_FINAL:
                lexicalAnalyzer->state = START;
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_INT_LITERAL, true, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                break;
            case INT_FINAL:
                lexicalAnalyzer->state = START;
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_INT_LITERAL, true, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                break;
            case DOUBLE_NO_EXP_FINAL:
                lexicalAnalyzer->state = START;
                compilerStatus =
                    editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength,
                                 TOKEN_DOUBLE_LITERAL, true, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                break;
            case DOUBLE_NO_EXP_D_FINAL:
                lexicalAnalyzer->state = START;
                compilerStatus =
                    editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength,
                                 TOKEN_DOUBLE_LITERAL, true, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                break;
            case EXPONENTIAL_FINAL:
                lexicalAnalyzer->state = START;
                compilerStatus =
                    editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength,
                                 TOKEN_DOUBLE_LITERAL, true, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                break;
            case EXPONENTIAL_FINAL_D:
                lexicalAnalyzer->state = START;
                compilerStatus =
                    editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength,
                                 TOKEN_DOUBLE_LITERAL, true, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                break;
            case ONE_LINE_STRING_FINAL:
                lexicalAnalyzer->state = START;
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_STRING_LITERAL, true, true);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                break;
            case ONE_LINE_STRING_NO_EMPTY_FINAL:
                lexicalAnalyzer->state = START;
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_STRING_LITERAL, true, true);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                break;
            case MULTIPLE_LINE_STRING_FINAL:
                lexicalAnalyzer->state = START;
                compilerStatus =
                    editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength,
                                 TOKEN_STRING_LITERAL, true, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                break;
            case OPERATOR_FINAL:
                lexicalAnalyzer->state = START;
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_OPERATOR, true, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                break;
            case SINGLE_OR_DOUBLE_OP_FINAL:
                lexicalAnalyzer->state = START;
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_OPERATOR, true, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                break;
            case EQUAL_OPERATOR_FINAL:
                lexicalAnalyzer->state = START;
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_OPERATOR, true, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                break;
            case PLUS_OPERATOR_FINAL:
                lexicalAnalyzer->state = START;
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_OPERATOR, true, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                break;
            case LESS_OP_FINAL:
                lexicalAnalyzer->state = START;
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_OPERATOR, true, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                break;
            case UNIFICATION_OPERATOR_FINAL:
                lexicalAnalyzer->state = START;
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_OPERATOR, true, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                break;
            case DOUBLE_OPERATOR_FINAL:
                lexicalAnalyzer->state = START;
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_OPERATOR, true, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                break;
            case LEFT_PAREN_FINAL:
                lexicalAnalyzer->state = START;
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_LEFT_PAREN, true, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                break;
            case LEFT_SHARP_PAREN_FINAL:
                lexicalAnalyzer->state = START;
                compilerStatus =
                    editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength,
                                 TOKEN_LEFT_SHARP_PAREN, true, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                break;
            case RIGHT_PAREN_FINAL:
                lexicalAnalyzer->state = START;
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_RIGHT_PAREN, true, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                break;
            case RIGHT_SHARP_PAREN_FINAL:
                lexicalAnalyzer->state = START;
                compilerStatus =
                    editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength,
                                 TOKEN_RIGHT_SHARP_PAREN, true, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                break;
            case COMMA_FINAL:
                lexicalAnalyzer->state = START;
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_COMMA, true, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                break;
            case COLON_FINAL:
                lexicalAnalyzer->state = START;
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_COLON, true, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                break;
            case SEMICOLON_FINAL:
                lexicalAnalyzer->state = START;
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_SEMICOLON, true, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                break;
            case DOT_FINAL:
                lexicalAnalyzer->state = START;
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_DOT, true, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                break;
            case AT_FINAL:
                lexicalAnalyzer->state = START;
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_AT, true, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                break;
            case MULTIPLE_LINE_COMMENT_FINAL:
                if (newlineInComment && lexicalAnalyzer->lineHasToken &&
                    lexicalAnalyzer->parenDepth == 0) {
                    lexicalAnalyzer->logicalIndentKnown = false;
                    lexicalAnalyzer->continuationPending = false;
                    lexicalAnalyzer->state = START;
                    setTokenLocation(outToken, lexicalAnalyzer->newlineLocation);
                    copyString(tokenLexeme, "\n", &tokenLexemeLength);
                    compilerStatus = setToken(outToken, tokenLexeme, TOKEN_NEWLINE);
                    if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                        lexicalAnalyzer->state = INVALID;
                        tokenAndLexemeDestroy(outToken, tokenLexeme);
                        return COMPILER_INTERNAL_ERROR;
                    }
                    lexicalAnalyzer->lineHasToken = false;
                } else {
                    lexicalAnalyzer->state = START;
                    (*outToken)->type = TOKEN_EOF;
                    compilerStatus = COMPILER_OK;
                }
                break;
            default:
                lexicalAnalyzer->state = START;
                (*outToken)->type = TOKEN_EOF;
                compilerStatus = COMPILER_OK;
                break;
            }
        } else {
            if (lexicalAnalyzer->state == COMMENT) // '/' operator
            {
                if (!beginRealToken(lexicalAnalyzer, lexicalAnalyzer->slashIndent,
                                    lexicalAnalyzer->slashWasPending)) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_LEXICAL_ERROR;
                }
                lexicalAnalyzer->state = START;
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme,
                                              &tokenLexemeLength, TOKEN_OPERATOR, true, false);
                if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                    lexicalAnalyzer->state = INVALID;
                    tokenAndLexemeDestroy(outToken, tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
            } else if (lexicalAnalyzer->state == AFTER_COMMENT_CHECK) {
                if (lexicalAnalyzer->lineHasToken && lexicalAnalyzer->parenDepth == 0) {
                    lexicalAnalyzer->logicalIndentKnown = false;
                    lexicalAnalyzer->continuationPending = false;
                    lexicalAnalyzer->state = START;
                    copyString(tokenLexeme, "\n", &tokenLexemeLength);
                    setTokenLocation(outToken, lexicalAnalyzer->newlineLocation);
                    compilerStatus = setToken(outToken, tokenLexeme, TOKEN_NEWLINE);
                    if (compilerStatus == COMPILER_INTERNAL_ERROR) {
                        lexicalAnalyzer->state = INVALID;
                        tokenAndLexemeDestroy(outToken, tokenLexeme);
                        return COMPILER_INTERNAL_ERROR;
                    }
                    lexicalAnalyzer->lineHasToken = false;
                } else {
                    lexicalAnalyzer->state = START;
                    (*outToken)->type = TOKEN_EOF;
                    compilerStatus = COMPILER_OK;
                }
            } else if (lexicalAnalyzer->state == START ||
                       lexicalAnalyzer->state == AFTER_COMMENT_CHECK ||
                       lexicalAnalyzer->state == ONE_LINE_COMMENT ||
                       lexicalAnalyzer->state == INDENT_COUNT ||
                       lexicalAnalyzer->state == TAB_CHECK) {
                lexicalAnalyzer->state = START;
                (*outToken)->type = TOKEN_EOF;
                compilerStatus = COMPILER_OK;
            } else {
                lexicalAnalyzer->state = INVALID;
                tokenAndLexemeDestroy(outToken, tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
        }
    }

    free(tokenLexeme);

    if ((*outToken)->type == TOKEN_EOF) {
        return finishAtEof(lexicalAnalyzer, outToken);
    }

    return compilerStatus;
}
