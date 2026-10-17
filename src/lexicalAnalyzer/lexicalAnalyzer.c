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
#include <math.h>

#define NUM_OF_KEYWORDS 22
#define LEN_OF_KEYWORDS 10

static int tokenLexemeCapacity = 100;
static char keywords[NUM_OF_KEYWORDS][LEN_OF_KEYWORDS] = {"def", "do", "Double", "else", "if", "import",
    "Int", "return", "String", "then", "Unit", "val", "var", "while", "Boolean", "case", "class",
    "extension", "for", "List", "match", "type"};

enum State {
  INVALID,
  START,
  ID_,
  LEFT_PAREN_FINAL,
  NEWLINE_FINAL,
  AFTER_COMMENT_CHECK,
  ID_FINAL1,
  ZERO_FINAL,
  //NUM_PLUS,
  INT_FINAL,
  DOUBLE,
  COMMENT,
  STRING,
  ID_FINAL2,
  //UNIT_FINAL,
  //ZERO_PLUS_FINAL,
  //INT_PLUS_FINAL,
  INT_,
  DOUBLE_NO_EXP_FINAL,
  ONE_LINE_COMMENT,
  MULTIPLE_LINE_COMMENT,
  ONE_LINE_STRING,
  ESCAPE_STRING,
  EXPONENTIAL,
  //INT_PLUS_,
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
  DOUBLE_OPERATOR,
  DOUBLE_OPERATOR_FINAL,
  RIGHT_PAREN_FINAL,
  COMMA_FINAL,
  COLON_FINAL,
  SEMICOLON_FINAL,
  DOT_FINAL,
  AT_FINAL,
  INDENT_COUNT,
  TAB_CHECK
};

struct LexicalAnalyzer {
    FILE *input;
    size_t line;
    size_t column;
    Token * prevToken;
    int prevIndent;
    Stack * indentStack;
    int indentCount;
    int currentIndent;
    int parenIndent;
    int parenDepth;
    bool newlineEnded;
    bool logicalIndentKnown;
    bool continuationPending;
    int slashIndent;
    bool slashWasPending;
    enum State state;
    int prevChar;
    bool prevCharRead;
    size_t prevCharLine;
    size_t prevCharColumn;
    bool reachedEof;
    bool lineHasToken;
    size_t newlineLine;
    size_t newlineColumn;
};

LexicalAnalyzer *lexicalAnalyzerCreate(FILE *input)
{
    if (input == NULL) {
        return NULL;
    }

    LexicalAnalyzer *lexicalAnalyzer = memoryAllocate(sizeof(LexicalAnalyzer));
    if(lexicalAnalyzer == NULL)
    {
        return NULL;
    }

    lexicalAnalyzer->indentStack = stackCreate();
    if(lexicalAnalyzer->indentStack == NULL)
    {
        free(lexicalAnalyzer);
        return NULL;
    }
    if(!stackPush(lexicalAnalyzer->indentStack, 0))
    {
        stackDestroy(lexicalAnalyzer->indentStack);
        free(lexicalAnalyzer);
        return NULL;
    }
    

    lexicalAnalyzer->input = input;
    lexicalAnalyzer->line = 1;
    lexicalAnalyzer->column = 1;
    lexicalAnalyzer->prevToken = NULL;
    lexicalAnalyzer->prevIndent = 0;
    lexicalAnalyzer->indentCount = 0;
    lexicalAnalyzer->currentIndent = 0;
    lexicalAnalyzer->parenIndent = 0;
    lexicalAnalyzer->parenDepth = 0;
    lexicalAnalyzer->newlineEnded = true;
    lexicalAnalyzer->logicalIndentKnown = false;
    lexicalAnalyzer->continuationPending = false;
    lexicalAnalyzer->slashIndent = 0;
    lexicalAnalyzer->slashWasPending = false;
    lexicalAnalyzer->state = START;
    lexicalAnalyzer->prevCharRead = false;
    lexicalAnalyzer->prevCharLine = 1;
    lexicalAnalyzer->prevCharColumn = 1;
    lexicalAnalyzer->reachedEof = false;
    lexicalAnalyzer->lineHasToken = false;
    lexicalAnalyzer->newlineLine = 1;
    lexicalAnalyzer->newlineColumn = 1;
    return lexicalAnalyzer;
}

static bool isFinal(enum State state)
{
    return (state == ID_FINAL1 || state == ID_FINAL2 || state == ZERO_FINAL /*|| state == ZERO_PLUS_FINAL*/ || state == INT_FINAL
        /*|| state == INT_PLUS_FINAL*/ || state == DOUBLE_NO_EXP_FINAL || state == DOUBLE_NO_EXP_D_FINAL || state == ONE_LINE_COMMENT_FINAL
        || state == MULTIPLE_LINE_COMMENT_FINAL || state == ONE_LINE_STRING_FINAL || state == ONE_LINE_STRING_NO_EMPTY_FINAL
        || state == EXPONENTIAL_FINAL || state == EXPONENTIAL_FINAL_D || state == MULTIPLE_LINE_STRING_FINAL || state == NEWLINE_FINAL
        || state == OPERATOR_FINAL || state == COMMENT || state == SINGLE_OR_DOUBLE_OP_FINAL || state == DOUBLE_OPERATOR_FINAL
        /*|| state == UNIT_FINAL*/ || state == LEFT_PAREN_FINAL || state == RIGHT_PAREN_FINAL || state == COMMA_FINAL || state == COLON_FINAL
        || state == SEMICOLON_FINAL || state == DOT_FINAL || state == AT_FINAL);
}

static bool removeWhiteChars(char * string, int * length, size_t * position)
{
    char * clearString = malloc(tokenLexemeCapacity * sizeof(char));
    if(clearString == NULL)
    {
        return false;
    }
    int idx1 = 0, idx2 = 0;
    while(string[idx1] == '\t' || string[idx1] == '\v' || string[idx1] == '\f' || string[idx1] == ' ')
    {
        idx1++;
    }
    *position += idx1;
    while(idx1 < *length)
    {
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

static bool decodeEscapeSeq(char * string, int * length)
{
    char * clearString = malloc(tokenLexemeCapacity * sizeof(char));
    if(clearString == NULL)
    {
        return false;
    }
    int idx2 = 0;
    for(int idx1 = 0; idx1 < *length; idx1++)
    {
        if(string[idx1] == '\\' && (string[idx1+1] == '\\' || string[idx1+1] == '"' || string[idx1+1] == 'n' || string[idx1+1] == 'r' || string[idx1+1] == 't'))
        {
            if(string[idx1+1] == '\\')
            {
                clearString[idx2] = '\\';
            }
            else if(string[idx1+1] == '"')
            {
                clearString[idx2] = '\"';
            }
            else if(string[idx1+1] == 'n')
            {
                clearString[idx2] = '\n';
            }
            else if(string[idx1+1] == 'r')
            {
                clearString[idx2] = '\r';
            }
            else if(string[idx1+1] == 't')
            {
                clearString[idx2] = '\t';
            }
            idx1++;
            idx2++;
        }
        else
        {
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

static bool removeUnnecessaryFromNumber(char * string)
{
    char * clearString = malloc(tokenLexemeCapacity * sizeof(char));
    if(clearString == NULL)
    {
        return false;
    }
    int idx2 = 0;
    for(int idx1 = 0; string[idx1] != '\0'; idx1++)
    {
        if(string[idx1] != '_' && string[idx1] != 'd' && string[idx1] != 'D' && string[idx1] != '\t' && string[idx1] != '\v' && string[idx1] != '\f' && string[idx1] != ' ')
        {
            clearString[idx2] = string[idx1];
            idx2++;
        }
    }
    clearString[idx2] = '\0';
    strcpy(string, clearString);
    free(clearString);
    return true;
}

static int64_t stringToInt(char * string, char ** endptr)
{
    long longResult = strtol(string, endptr, 10);
    
    return (int64_t)longResult; 
}

static double stringToDouble(char * string, char ** endptr)
{
    return strtod(string, endptr); 
}

void lexicalAnalyzerDestroy(LexicalAnalyzer *lexicalAnalyzer)
{
    if(lexicalAnalyzer == NULL)
    {
        return;
    }
    stackDestroy(lexicalAnalyzer->indentStack);
    tokenDestroy(lexicalAnalyzer->prevToken);
    free(lexicalAnalyzer);
}

static bool stringAppend(char ** string, int * length, char character)
{
    if((*length)+1 >= tokenLexemeCapacity)
    {
        int tempCapacity = tokenLexemeCapacity * 2;
        char * tmpString = realloc(*string, tempCapacity * sizeof(char));
        if(tmpString == NULL)
        {
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

static bool isKeyword(char * lexeme, char keywords[][LEN_OF_KEYWORDS])
{
    for(int idx = 0; idx < NUM_OF_KEYWORDS; idx++)
    {
        if(strcmp(lexeme, keywords[idx]) == 0)
        {
            return true;
        }
    }
    return false;
}

static void stringShortenByOne(char * string, int * length)
{
    string[(*length)-1] = '\0';
    (*length)--;
}

static void setTokenLocation(Token ** token,  size_t line, size_t column)
{
    (*token)->location.line = line;
    (*token)->location.column = column;
}

static bool beginRealToken(LexicalAnalyzer * lexicalAnalyzer, int indent, bool needsCheck)
{
    if(needsCheck && indent <= lexicalAnalyzer->parenIndent)
    {
        return false;
    }

    if(!lexicalAnalyzer->logicalIndentKnown)
    {
        lexicalAnalyzer->parenIndent = indent;
        lexicalAnalyzer->logicalIndentKnown = true;
    }

    return true;
}

static bool calculateIndentCount(LexicalAnalyzer * lexicalAnalyzer, int currentIndent, CompilerStatus * error)
{
    if(currentIndent > lexicalAnalyzer->prevIndent)
    {
        lexicalAnalyzer->indentCount++;
        if(!stackPush(lexicalAnalyzer->indentStack, currentIndent))
        {
            *error = COMPILER_INTERNAL_ERROR;
            return false;
        }
        return true;
    }
    else if(currentIndent < lexicalAnalyzer->prevIndent)
    {
        while(currentIndent < stackTop(lexicalAnalyzer->indentStack))
        {
            lexicalAnalyzer->indentCount--;
            stackPop(lexicalAnalyzer->indentStack);
        }
        if(currentIndent != stackTop(lexicalAnalyzer->indentStack))
        {
            *error = COMPILER_LEXICAL_ERROR;
            return false;
        }
        return true;
    }
    return true;
}

static enum State startCases(char character, LexicalAnalyzer * lexicalAnalyzer, Token ** token, char * tokenLexeme, int * tokenLexemeLength, CompilerStatus * error)
{
    if(lexicalAnalyzer->newlineEnded)
    {
        if(character == ' ')
        {
            lexicalAnalyzer->newlineEnded = false;
            lexicalAnalyzer->currentIndent++;
            return INDENT_COUNT;
        }
        else if(character == '\t') // tab
        {
            lexicalAnalyzer->newlineEnded = false;
            return TAB_CHECK;
        }
    }
    lexicalAnalyzer->newlineEnded = false;

    if(character != ' ' && character != '\t' && character != '\n' && character != '/')
    {
        if(lexicalAnalyzer->parenDepth == 0 && !lexicalAnalyzer->logicalIndentKnown)
        {
            if(!calculateIndentCount(lexicalAnalyzer, lexicalAnalyzer->currentIndent, error))
            {
                return INVALID;
            }
            lexicalAnalyzer->prevIndent = lexicalAnalyzer->currentIndent;
        }
        if(!beginRealToken(lexicalAnalyzer, lexicalAnalyzer->currentIndent, lexicalAnalyzer->continuationPending))
        {
            return INVALID;
        }
        lexicalAnalyzer->continuationPending = false;
    }

    if(character == '_')
    {
        return ID_; 
    }
    else if((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z'))
    {
        return ID_FINAL1;
    }
    else if(character == '(')
    {
        lexicalAnalyzer->parenDepth++;
        return LEFT_PAREN_FINAL;
    }
    else if(character == '0')
    {
        return ZERO_FINAL;
    }
    /*else if(character == '+')
    {
        return NUM_PLUS;
    }*/
    else if(character >= '1' && character <= '9')
    {
        return INT_FINAL;
    }
    else if(character == '.')
    {
        return DOT_FINAL;
    }
    else if(character == '/')
    {
        lexicalAnalyzer->slashIndent = lexicalAnalyzer->currentIndent;
        lexicalAnalyzer->slashWasPending = lexicalAnalyzer->continuationPending;
        return COMMENT;
    }
    else if(character == '\"')
    {
        return STRING;
    }
    else if(character == '\n') // LF = newline
    {
        lexicalAnalyzer->newlineEnded = true;
        if(lexicalAnalyzer->lineHasToken && lexicalAnalyzer->parenDepth == 0)
        {
            return NEWLINE_FINAL;
        }
        else
        {
            strcpy(tokenLexeme, "");
            *tokenLexemeLength = 0;
            setTokenLocation(token, lexicalAnalyzer->line, lexicalAnalyzer->column);
            return START;
        }
    }
    else if(character == '+' || character == '-' || character == '*')
    {
        return OPERATOR_FINAL;
    }
    else if(character == '=' || character == '<' || character == '>')
    {
        return SINGLE_OR_DOUBLE_OP_FINAL;
    }
    else if(character == '!')
    {
        return DOUBLE_OPERATOR;
    }
    else if(character == ')')
    {
        if(lexicalAnalyzer->parenDepth > 0)
        {
            lexicalAnalyzer->parenDepth--;
        }
        return RIGHT_PAREN_FINAL;
    }
    else if(character == ',')
    {
        return COMMA_FINAL;
    }
    else if(character == ':')
    {
        return COLON_FINAL;
    }
    else if(character == ';')
    {
        return SEMICOLON_FINAL;
    }
    else if(character == '@')
    {
        return AT_FINAL;
    }
    return INVALID;
}

static void setLexicalAnalyzerToStart(LexicalAnalyzer * lexicalAnalyzer, int currentChar)
{
    lexicalAnalyzer->state = START;
    lexicalAnalyzer->prevChar = currentChar;
    lexicalAnalyzer->prevCharRead = true;
}

static CompilerStatus setToken(Token ** token, char * tokenLexeme, TokenType type)
{
    CompilerStatus compilerStatus = tokenSetLexeme(*token, tokenLexeme);
    if(compilerStatus == COMPILER_INTERNAL_ERROR)
    {
        tokenDestroy(*token);
        *token = NULL;
        return COMPILER_INTERNAL_ERROR;
    }

    char *endptr;
    switch(type)
    {
    case TOKEN_IDENTIFIER:
        if(isKeyword((*token)->lexeme, keywords))
        {
            (*token)->type = TOKEN_KEYWORD;
        }
        else
        {
            (*token)->type = TOKEN_IDENTIFIER;
        }
        break;
    case TOKEN_INT_LITERAL:
        if(!removeUnnecessaryFromNumber(tokenLexeme))
        {
            tokenDestroy(*token);
            *token = NULL;
            return COMPILER_INTERNAL_ERROR;
        }
        (*token)->value.intValue = stringToInt(tokenLexeme, &endptr);
        if(*endptr != '\0')
        {
            tokenDestroy(*token);
            *token = NULL;
            return COMPILER_INTERNAL_ERROR;
        }
        (*token)->type = TOKEN_INT_LITERAL;
        break;
    case TOKEN_DOUBLE_LITERAL:
        if(!removeUnnecessaryFromNumber(tokenLexeme))
        {
            tokenDestroy(*token);
            *token = NULL;
            return COMPILER_INTERNAL_ERROR;
        }
        (*token)->value.doubleValue = stringToDouble(tokenLexeme, &endptr);
        if(*endptr != '\0')
        {
            tokenDestroy(*token);
            *token = NULL;
            return COMPILER_INTERNAL_ERROR;
        }
        (*token)->type = TOKEN_DOUBLE_LITERAL;
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
    case TOKEN_UNIT_LITERAL:
        (*token)->type = TOKEN_UNIT_LITERAL;
        break;
    case TOKEN_LEFT_PAREN:
        (*token)->type = TOKEN_LEFT_PAREN;
        break;
    case TOKEN_RIGHT_PAREN:
        (*token)->type = TOKEN_RIGHT_PAREN;
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

static CompilerStatus indentDedent(LexicalAnalyzer * lexicalAnalyzer, Token ** token)
{
    if(lexicalAnalyzer->indentCount > 0)
    {
        lexicalAnalyzer->indentCount--;
        (*token)->type = TOKEN_INDENT;
    }
    else if(lexicalAnalyzer->indentCount < 0)
    {
        lexicalAnalyzer->indentCount++;
        (*token)->type = TOKEN_DEDENT;
    }
    return COMPILER_OK;
}

static CompilerStatus finishAtEof(LexicalAnalyzer *lexicalAnalyzer, Token **outToken)
{
    if (lexicalAnalyzer->prevIndent > 0)
    {
        CompilerStatus compilerError;
        if(!calculateIndentCount(lexicalAnalyzer, 0, &compilerError))
        {
            return compilerError;
        }
        lexicalAnalyzer->prevIndent = 0;
        return indentDedent(lexicalAnalyzer, outToken);
    }

    (*outToken)->type = TOKEN_EOF;
    return COMPILER_OK;
}

static CompilerStatus editSetToken(Token ** token, LexicalAnalyzer * lexicalAnalyzer, char * tokenLexeme, int * tokenLexemeLength, TokenType type, bool reachedEof, bool oneLineString)
{
    if(!reachedEof)
    {
        stringShortenByOne(tokenLexeme, tokenLexemeLength);
    }
    
    if(!removeWhiteChars(tokenLexeme, tokenLexemeLength, &((*token)->location.column)))
    {
        return COMPILER_INTERNAL_ERROR;
    }
    if(oneLineString && !decodeEscapeSeq(tokenLexeme, tokenLexemeLength))
    {
        return COMPILER_INTERNAL_ERROR;
    }
    CompilerStatus compilerStatus = setToken(token, tokenLexeme, type);
    if(compilerStatus == COMPILER_INTERNAL_ERROR)
    {
        return COMPILER_INTERNAL_ERROR;
    }

    if(lexicalAnalyzer->indentCount != 0)
    {
        if(lexicalAnalyzer->prevToken != NULL)
        {
            return COMPILER_INTERNAL_ERROR;
        }
        Token *tempToken = tokenCreate();
        if(tempToken == NULL)
        {
            return COMPILER_INTERNAL_ERROR;
        }
        lexicalAnalyzer->prevToken = *token;
        *token = tempToken;
        return indentDedent(lexicalAnalyzer, token);
    }
    return compilerStatus;
}

CompilerStatus lexicalAnalyzerNextToken(LexicalAnalyzer *lexicalAnalyzer, Token **outToken)
{
    if(lexicalAnalyzer == NULL || outToken == NULL)
    {
        if(outToken != NULL)
        {
            *outToken = NULL;
        }
        return COMPILER_INTERNAL_ERROR;
    }
    
    /* Team TODO: implement the required finite-state scanner manually. */
    if(lexicalAnalyzer->indentCount != 0)
    {
        *outToken = tokenCreate();
        if(*outToken == NULL)
        {
            return COMPILER_INTERNAL_ERROR;
        }
        return indentDedent(lexicalAnalyzer, outToken);
    }
    else if(lexicalAnalyzer->prevToken != NULL)
    {
        *outToken = lexicalAnalyzer->prevToken;
        lexicalAnalyzer->prevToken = NULL;
        return COMPILER_OK;
    }
    *outToken = tokenCreate();
    if(*outToken == NULL)
    {
        return COMPILER_INTERNAL_ERROR;
    }
    if (lexicalAnalyzer->reachedEof)
    {
        return finishAtEof(lexicalAnalyzer, outToken);
    }

    int currentChar = EOF;
    char * tokenLexeme = malloc(tokenLexemeCapacity * sizeof(char));
    if(tokenLexeme == NULL)
    {
        tokenDestroy(*outToken);
        *outToken = NULL;
        return COMPILER_INTERNAL_ERROR;
    }
    strcpy(tokenLexeme, "");
    int tokenLexemeLength = 0, nestedComments = 0;
    bool commentEnded = false, newlineInComment = false;
    CompilerStatus compilerError = COMPILER_LEXICAL_ERROR;
    if(lexicalAnalyzer->prevCharRead)
    {
        setTokenLocation(outToken, lexicalAnalyzer->prevCharLine, lexicalAnalyzer->prevCharColumn);
        lexicalAnalyzer->prevCharRead = false;
        currentChar = lexicalAnalyzer->prevChar;
        if(!stringAppend(&tokenLexeme, &tokenLexemeLength, lexicalAnalyzer->prevChar))
        {
            tokenDestroy(*outToken);
            *outToken = NULL;
            free(tokenLexeme);
            return COMPILER_INTERNAL_ERROR;
        }

        compilerError = COMPILER_LEXICAL_ERROR;
        lexicalAnalyzer->state = startCases(currentChar, lexicalAnalyzer, outToken, tokenLexeme, &tokenLexemeLength, &compilerError);
        if(lexicalAnalyzer->state == INVALID)
        {
            lexicalAnalyzer->state = INVALID;
            tokenDestroy(*outToken);
            *outToken = NULL;
            free(tokenLexeme);
            return compilerError;
        }
    }
    else
    {
        setTokenLocation(outToken, lexicalAnalyzer->line, lexicalAnalyzer->column);
    }

    CompilerStatus compilerStatus = COMPILER_INTERNAL_ERROR;
    while((*outToken)->type == TOKEN_INVALID && (currentChar = fgetc(lexicalAnalyzer->input)) != EOF)
    {
        if(commentEnded)
        {
            strcpy(tokenLexeme, "");
            tokenLexemeLength = 0;
            commentEnded = false;
            if(lexicalAnalyzer->prevCharRead)
            {
                setTokenLocation(outToken, lexicalAnalyzer->prevCharLine, lexicalAnalyzer->prevCharColumn);
                lexicalAnalyzer->prevCharRead = false;
                if(!stringAppend(&tokenLexeme, &tokenLexemeLength, lexicalAnalyzer->prevChar))
                {
                    lexicalAnalyzer->state = INVALID;
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    free(tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                if(newlineInComment)
                {
                    newlineInComment = false;
                    if(lexicalAnalyzer->prevChar == '\n')
                    {
                        if(lexicalAnalyzer->lineHasToken && lexicalAnalyzer->parenDepth <= 0)
                        {
                            lexicalAnalyzer->state = NEWLINE_FINAL;
                        }
                        else
                        {
                            strcpy(tokenLexeme, "");
                            tokenLexemeLength = 0;
                            setTokenLocation(outToken, lexicalAnalyzer->line, lexicalAnalyzer->column);
                            lexicalAnalyzer->state = START;
                        }
                    }
                    else if(lexicalAnalyzer->prevChar == ' ' || lexicalAnalyzer->prevChar == '\t' || lexicalAnalyzer->prevChar == '\v' || lexicalAnalyzer->prevChar == '\f')
                    {
                        lexicalAnalyzer->state = AFTER_COMMENT_CHECK;
                    }
                    else
                    {
                        lexicalAnalyzer->state = INVALID;
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        free(tokenLexeme);
                        return COMPILER_LEXICAL_ERROR;
                    }
                }
                else
                {
                    compilerError = COMPILER_LEXICAL_ERROR;
                    lexicalAnalyzer->state = startCases(lexicalAnalyzer->prevChar, lexicalAnalyzer, outToken, tokenLexeme, &tokenLexemeLength, &compilerError);
                    if(lexicalAnalyzer->state == INVALID)
                    {
                        lexicalAnalyzer->state = INVALID;
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        free(tokenLexeme);
                        return compilerError;
                    }
                }
            }
            else
            {
                setTokenLocation(outToken, lexicalAnalyzer->line, lexicalAnalyzer->column);
            }
        }
        // Append string 
        if(!stringAppend(&tokenLexeme, &tokenLexemeLength, currentChar))
        {
            tokenDestroy(*outToken);
            *outToken = NULL;
            free(tokenLexeme);
            return COMPILER_INTERNAL_ERROR;
        }

        lexicalAnalyzer->prevCharLine = lexicalAnalyzer->line;
        lexicalAnalyzer->prevCharColumn = lexicalAnalyzer->column;
        if(currentChar == '\n')
        {
            if(lexicalAnalyzer->state != NEWLINE_FINAL && lexicalAnalyzer->state != ONE_LINE_COMMENT_FINAL)
            {
                lexicalAnalyzer->newlineLine = lexicalAnalyzer->line;
                lexicalAnalyzer->newlineColumn = lexicalAnalyzer->column;
            }
            if(lexicalAnalyzer->parenDepth > 0 && lexicalAnalyzer->state != MULTIPLE_LINE_STRING_INSIDE && lexicalAnalyzer->state != MULTIPLE_LINE_STRING_ONE && lexicalAnalyzer->state != MULTIPLE_LINE_STRING_TWO)
            {
                lexicalAnalyzer->continuationPending = true;
            }
            lexicalAnalyzer->column = 1;
            lexicalAnalyzer->line++;
            lexicalAnalyzer->newlineEnded = true;
            lexicalAnalyzer->currentIndent = 0;
        }
        else
        {
            lexicalAnalyzer->column++;
        }

        switch(lexicalAnalyzer->state)
        {
        case START:
            compilerError = COMPILER_LEXICAL_ERROR;
            lexicalAnalyzer->state = startCases(currentChar, lexicalAnalyzer, outToken, tokenLexeme, &tokenLexemeLength, &compilerError);
            if(lexicalAnalyzer->state == INVALID)
            {
                lexicalAnalyzer->state = INVALID;
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return compilerError;
            }
            break;
        case ID_:
            if((currentChar >= 'a' && currentChar <= 'z') || (currentChar >= 'A' && currentChar <= 'Z') || (currentChar >= '0' && currentChar <= '9') || currentChar == '_')
            {
                lexicalAnalyzer->state = ID_FINAL2;
            }
            else
            {
                lexicalAnalyzer->state = INVALID;
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case INDENT_COUNT:
            if(currentChar == ' ')
            {
                lexicalAnalyzer->currentIndent++;
                lexicalAnalyzer->state = INDENT_COUNT;
            }
            else if(currentChar == '\t')
            {
                lexicalAnalyzer->state = TAB_CHECK;
            }
            else if(currentChar == '\n')
            {
                compilerError = COMPILER_LEXICAL_ERROR;
                lexicalAnalyzer->state = startCases(currentChar, lexicalAnalyzer, outToken, tokenLexeme, &tokenLexemeLength, &compilerError);
                if(lexicalAnalyzer->state == INVALID)
                {
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    free(tokenLexeme);
                    return compilerError;
                }
            }
            else
            {
                compilerError = COMPILER_LEXICAL_ERROR;
                lexicalAnalyzer->state = startCases(currentChar, lexicalAnalyzer, outToken, tokenLexeme, &tokenLexemeLength, &compilerError);
                if(lexicalAnalyzer->state == INVALID)
                {
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    free(tokenLexeme);
                    return compilerError;
                }
            }
            break;
        case TAB_CHECK:
            if(currentChar == '\t' || currentChar == ' ')
            {
                lexicalAnalyzer->state = TAB_CHECK;
            }
            else if(currentChar == '\n')
            {
                lexicalAnalyzer->state = START;
            }
            else
            {
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case LEFT_PAREN_FINAL:
            setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
            compilerStatus = editSetToken(outToken, lexicalAnalyzer,tokenLexeme, &tokenLexemeLength, TOKEN_LEFT_PAREN, false, false);
            if(compilerStatus == COMPILER_INTERNAL_ERROR)
            {
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_INTERNAL_ERROR;
            }
            lexicalAnalyzer->lineHasToken = true;
            break;
        case ID_FINAL1:
            if((currentChar >= 'a' && currentChar <= 'z') || (currentChar >= 'A' && currentChar <= 'Z') || (currentChar >= '0' && currentChar <= '9') || currentChar == '_')
            {
                lexicalAnalyzer->state = ID_FINAL1;
            }
            else
            {
                setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_IDENTIFIER, false, false);
                if(compilerStatus == COMPILER_INTERNAL_ERROR)
                {
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    free(tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                lexicalAnalyzer->lineHasToken = true;
            }
            break;
        case NEWLINE_FINAL:
            lexicalAnalyzer->logicalIndentKnown = false;
            lexicalAnalyzer->continuationPending = false;
            setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
            stringShortenByOne(tokenLexeme, &tokenLexemeLength);
            setTokenLocation(outToken, lexicalAnalyzer->newlineLine, lexicalAnalyzer->newlineColumn);
            compilerStatus = setToken(outToken, tokenLexeme, TOKEN_NEWLINE);
            if(compilerStatus == COMPILER_INTERNAL_ERROR)
            {
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_INTERNAL_ERROR;
            }
            lexicalAnalyzer->lineHasToken = false;
            break;
        case AFTER_COMMENT_CHECK:
            if(currentChar == ' ' || currentChar == '\t' || currentChar == '\v' || currentChar == '\f')
            {
                lexicalAnalyzer->state = AFTER_COMMENT_CHECK;
            }
            else if(currentChar == '\n')
            {
                if(lexicalAnalyzer->lineHasToken && lexicalAnalyzer->parenDepth <= 0)
                {
                    lexicalAnalyzer->state = NEWLINE_FINAL;
                }
                else
                {
                    strcpy(tokenLexeme, "");
                    tokenLexemeLength = 0;
                    setTokenLocation(outToken, lexicalAnalyzer->line, lexicalAnalyzer->column);
                    lexicalAnalyzer->state = START;
                }
            }
            else
            {
                lexicalAnalyzer->state = INVALID;
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case ZERO_FINAL:
            if(currentChar == 'e' || currentChar == 'E')
            {
                lexicalAnalyzer->state = EXPONENTIAL;
            }
            else if(currentChar == '.')
            {
                lexicalAnalyzer->state = DOUBLE;
            }
            else if(currentChar >= '0' && currentChar <= '9')
            {
                lexicalAnalyzer->state = INVALID;
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            else
            {
                setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_INT_LITERAL, false, false);
                if(compilerStatus == COMPILER_INTERNAL_ERROR)
                {
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    free(tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                lexicalAnalyzer->lineHasToken = true;
            }
            break;
        /*case NUM_PLUS:
            if(currentChar == '0')
            {
                lexicalAnalyzer->state = ZERO_PLUS_FINAL;
            }
            else if(currentChar >= '1' && currentChar <= '9')
            {
                lexicalAnalyzer->state = INT_PLUS_FINAL;
            }
            else if(currentChar == '.')
            {
                lexicalAnalyzer->state = DOUBLE;
            }
            else
            {
                lexicalAnalyzer->state = INVALID;
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            break;*/
        case INT_FINAL:
            if(currentChar >= '0' && currentChar <= '9')
            {
                lexicalAnalyzer->state = INT_FINAL;
            }
            else if(currentChar == '_')
            {
                lexicalAnalyzer->state = INT_;
            }
            else if(currentChar == 'e' || currentChar == 'E')
            {
                lexicalAnalyzer->state = EXPONENTIAL;
            }
            else if(currentChar == '.')
            {
                lexicalAnalyzer->state = DOUBLE;
            }
            else
            {
                setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_INT_LITERAL, false, false);
                if(compilerStatus == COMPILER_INTERNAL_ERROR)
                {
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    free(tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                lexicalAnalyzer->lineHasToken = true;
            }
            break;
        case DOUBLE:
            if(currentChar >= '0' && currentChar <= '9')
            {
                lexicalAnalyzer->state = DOUBLE_NO_EXP_FINAL;
            }
            else
            {
                lexicalAnalyzer->state = INVALID;
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case DOT_FINAL:
            if(currentChar >= '0' && currentChar <= '9')
            {
                lexicalAnalyzer->state = DOUBLE_NO_EXP_FINAL;
            }
            else
            {
                setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_DOT, false, false);
                if(compilerStatus == COMPILER_INTERNAL_ERROR)
                {
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    free(tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                lexicalAnalyzer->lineHasToken = true;
            }
            break;
        case COMMENT:
            if(currentChar == '/')
            {
                lexicalAnalyzer->state = ONE_LINE_COMMENT;
            }
            else if(currentChar == '*')
            {
                lexicalAnalyzer->state = MULTIPLE_LINE_COMMENT;
            }
            else
            {
                if(lexicalAnalyzer->parenDepth == 0 && !lexicalAnalyzer->logicalIndentKnown)
                {
                    if(!calculateIndentCount(lexicalAnalyzer, lexicalAnalyzer->currentIndent, &compilerError))
                    {
                        lexicalAnalyzer->state = INVALID;
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        free(tokenLexeme);
                        return compilerError;
                    }
                    lexicalAnalyzer->prevIndent = lexicalAnalyzer->currentIndent;
                }
                if(!beginRealToken(lexicalAnalyzer, lexicalAnalyzer->slashIndent, lexicalAnalyzer->slashWasPending))
                {
                    lexicalAnalyzer->state = INVALID;
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    free(tokenLexeme);
                    return COMPILER_LEXICAL_ERROR;
                }

                if(currentChar != '\n')
                {
                    lexicalAnalyzer->continuationPending = false;
                }

                setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_OPERATOR, false, false);
                if(compilerStatus == COMPILER_INTERNAL_ERROR)
                {
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    free(tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                lexicalAnalyzer->lineHasToken = true;
                /*lexicalAnalyzer->state = INVALID;
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_LEXICAL_ERROR;*/
            }
            break;
        case STRING:
            if(currentChar != '\"' && currentChar != '\\' && currentChar != '\n')
            {
                lexicalAnalyzer->state = ONE_LINE_STRING;
            }
            else if(currentChar == '\"')
            {
                lexicalAnalyzer->state = ONE_LINE_STRING_FINAL;
            }
            else if(currentChar == '\\')
            {
                lexicalAnalyzer->state = ESCAPE_STRING;
            }
            else
            {
                lexicalAnalyzer->state = INVALID;
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case ID_FINAL2:
            if((currentChar >= 'a' && currentChar <= 'z') || (currentChar >= 'A' && currentChar <= 'Z') || (currentChar >= '0' && currentChar <= '9') || currentChar == '_')
            {
                lexicalAnalyzer->state = ID_FINAL2;
            }
            else
            {
                setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_IDENTIFIER, false, false);
                if(compilerStatus == COMPILER_INTERNAL_ERROR)
                {
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    free(tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                lexicalAnalyzer->lineHasToken = true;
            }
            break;
        /*case UNIT_FINAL:
            setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
            compilerStatus = editSetToken(outToken, tokenLexeme, &tokenLexemeLength, TOKEN_UNIT_LITERAL, false, false);
            if(compilerStatus == COMPILER_INTERNAL_ERROR)
            {
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_INTERNAL_ERROR;
            }
            lexicalAnalyzer->lineHasToken = true;
            break;
        case ZERO_PLUS_FINAL:
            if(currentChar == '.')
            {
                lexicalAnalyzer->state = DOUBLE;
            }
            else if(currentChar == 'e' || currentChar == 'E')
            {
                lexicalAnalyzer->state = EXPONENTIAL;
            }
            else if(currentChar >= '0' && currentChar <= '9')
            {
                lexicalAnalyzer->state = INVALID;
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            else
            {
                setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
                compilerStatus = editSetToken(outToken, tokenLexeme, &tokenLexemeLength, TOKEN_INT_LITERAL, false, false);
                if(compilerStatus == COMPILER_INTERNAL_ERROR)
                {
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    free(tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                lexicalAnalyzer->lineHasToken = true;
            }
            break;
        case INT_PLUS_FINAL:
            if(currentChar >= '0' && currentChar <= '9')
            {
                lexicalAnalyzer->state = INT_PLUS_FINAL;
            }
            else if(currentChar == '_')
            {
                lexicalAnalyzer->state = INT_PLUS_;
            }
            else if(currentChar == 'e' || currentChar == 'E')
            {
                lexicalAnalyzer->state = EXPONENTIAL;
            }
            else if(currentChar == '.')
            {
                lexicalAnalyzer->state = DOUBLE;
            }
            else
            {
                setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
                compilerStatus = editSetToken(outToken, tokenLexeme, &tokenLexemeLength, TOKEN_INT_LITERAL, false, false);
                if(compilerStatus == COMPILER_INTERNAL_ERROR)
                {
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    free(tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                lexicalAnalyzer->lineHasToken = true;
            }
            break;*/
        case INT_:
            if(currentChar >= '0' && currentChar <= '9')
            {
                lexicalAnalyzer->state = INT_FINAL;
            }
            else
            {
                lexicalAnalyzer->state = INVALID;
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case DOUBLE_NO_EXP_FINAL:
            if(currentChar >= '0' && currentChar <= '9')
            {
                lexicalAnalyzer->state = DOUBLE_NO_EXP_FINAL;
            }
            else if(currentChar == '_')
            {
                lexicalAnalyzer->state = DOUBLE_NO_EXP_;
            }
            else if(currentChar == 'e' || currentChar == 'E')
            {
                lexicalAnalyzer->state = EXPONENTIAL;
            }
            else if(currentChar == 'd' || currentChar == 'D')
            {
                lexicalAnalyzer->state = DOUBLE_NO_EXP_D_FINAL;
            }
            else
            {
                setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_DOUBLE_LITERAL, false, false);
                if(compilerStatus == COMPILER_INTERNAL_ERROR)
                {
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    free(tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                lexicalAnalyzer->lineHasToken = true;
            }
            break;
        case ONE_LINE_COMMENT:
            if((currentChar >= 0 && currentChar < '\n') || (currentChar > '\n' && currentChar <= 127))
            {
                lexicalAnalyzer->state = ONE_LINE_COMMENT;
            }
            else if(currentChar == '\n')
            {
                lexicalAnalyzer->state = ONE_LINE_COMMENT_FINAL;
            }
            else
            {
                lexicalAnalyzer->state = INVALID;
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case MULTIPLE_LINE_COMMENT:
            if(currentChar == '/')
            {
                lexicalAnalyzer->state = MULTIPLE_LINE_POSSIBLE_NESTED_COMMENT;
            }
            else if(currentChar == '*')
            {
                lexicalAnalyzer->state = MULTIPLE_LINE_COMMENT2;
            }
            else
            {
                if(currentChar == '\n')
                {
                    newlineInComment = true;
                }
                lexicalAnalyzer->state = MULTIPLE_LINE_COMMENT;
            }
            break;
        case ONE_LINE_STRING:
            if(currentChar != '\"' && currentChar != '\\' && currentChar != '\n')
            {
                lexicalAnalyzer->state = ONE_LINE_STRING;
            }
            else if(currentChar == '\\')
            {
                lexicalAnalyzer->state = ESCAPE_STRING;
            }
            else if(currentChar == '\"')
            {
                lexicalAnalyzer->state = ONE_LINE_STRING_NO_EMPTY_FINAL;
            }
            else
            {
                lexicalAnalyzer->state = INVALID;
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case ONE_LINE_STRING_NO_EMPTY_FINAL:
            setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
            compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_STRING_LITERAL, false, true);
            if(compilerStatus == COMPILER_INTERNAL_ERROR)
            {
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_INTERNAL_ERROR;
            }
            lexicalAnalyzer->lineHasToken = true;
            break;
        case ESCAPE_STRING:
            if(currentChar == '\"' || currentChar == '\\' || currentChar == 'n' || currentChar == 'r' || currentChar == 't')
            {
                lexicalAnalyzer->state = ONE_LINE_STRING;
            }
            else
            {
                lexicalAnalyzer->state = INVALID;
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case EXPONENTIAL:
            if(currentChar == '+' || currentChar == '-')
            {
                lexicalAnalyzer->state = EXPONENTIAL_PLUS_MIN;
            }
            else if(currentChar >= '0' && currentChar <= '9')
            {
                lexicalAnalyzer->state = EXPONENTIAL_FINAL;
            }
            else
            {
                lexicalAnalyzer->state = INVALID;
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        /*case INT_PLUS_:
            if(currentChar >= '0' && currentChar <= '9')
            {
                lexicalAnalyzer->state = INT_PLUS_FINAL;
            }
            else
            {
                lexicalAnalyzer->state = INVALID;
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            break;*/
        case DOUBLE_NO_EXP_:
            if(currentChar >= '0' && currentChar <= '9')
            {
                lexicalAnalyzer->state = DOUBLE_NO_EXP_FINAL;
            }
            else
            {
                lexicalAnalyzer->state = INVALID;
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case ONE_LINE_COMMENT_FINAL:
            setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
            if(lexicalAnalyzer->lineHasToken && lexicalAnalyzer->parenDepth <= 0)
            {
                stringShortenByOne(tokenLexeme, &tokenLexemeLength);
                setTokenLocation(outToken, lexicalAnalyzer->newlineLine, lexicalAnalyzer->newlineColumn);
                compilerStatus = setToken(outToken, tokenLexeme, TOKEN_NEWLINE);
                if(compilerStatus == COMPILER_INTERNAL_ERROR)
                {
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    free(tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                lexicalAnalyzer->lineHasToken = false;
                lexicalAnalyzer->logicalIndentKnown = false;
                lexicalAnalyzer->continuationPending = false;
                strcpy(tokenLexeme, "");
                free(tokenLexeme);
                tokenLexemeLength = 0;
                lexicalAnalyzer->state = START;
                return compilerStatus;
            }
            commentEnded = true;
            break;
        case MULTIPLE_LINE_POSSIBLE_NESTED_COMMENT:
            if(currentChar == '*')
            {
                lexicalAnalyzer->state = MULTIPLE_LINE_COMMENT;
                nestedComments++;
            }
            else if(currentChar == '/')
            {
                lexicalAnalyzer->state = MULTIPLE_LINE_POSSIBLE_NESTED_COMMENT;
            }
            else
            {
                if(currentChar == '\n')
                {
                    newlineInComment = true;
                }
                lexicalAnalyzer->state = MULTIPLE_LINE_COMMENT;
            }
            break;
        case MULTIPLE_LINE_COMMENT2:
            if(currentChar != '/' && currentChar != '*')
            {
                if(currentChar == '\n')
                {
                    newlineInComment = true;
                }
                lexicalAnalyzer->state = MULTIPLE_LINE_COMMENT;
            }
            else if(currentChar == '/')
            {
                if(nestedComments > 0)
                {
                    nestedComments--;
                    lexicalAnalyzer->state = MULTIPLE_LINE_COMMENT;
                }
                else
                {
                    lexicalAnalyzer->state = MULTIPLE_LINE_COMMENT_FINAL;
                }
            }
            else if(currentChar == '*')
            {
                lexicalAnalyzer->state = MULTIPLE_LINE_COMMENT2;
            }
            break;
        case DOUBLE_NO_EXP_D_FINAL:
            setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
            compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_DOUBLE_LITERAL, false, false);
            if(compilerStatus == COMPILER_INTERNAL_ERROR)
            {
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_INTERNAL_ERROR;
            }
            lexicalAnalyzer->lineHasToken = true;
            break;
        case ONE_LINE_STRING_FINAL:
            if(currentChar == '\"')
            {
                lexicalAnalyzer->state = MULTIPLE_LINE_STRING_INSIDE;
            }
            else
            {
                setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_STRING_LITERAL, false, true);
                if(compilerStatus == COMPILER_INTERNAL_ERROR)
                {
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    free(tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                lexicalAnalyzer->lineHasToken = true;
            }
            break;
        case MULTIPLE_LINE_STRING_INSIDE:
            if(currentChar == '\"')
            {
                lexicalAnalyzer->state = MULTIPLE_LINE_STRING_ONE;
            }
            else if(currentChar != '\"')
            {
                if(currentChar == '\n')
                {
                    lexicalAnalyzer->newlineEnded = false;
                }
                lexicalAnalyzer->state = MULTIPLE_LINE_STRING_INSIDE;
            }
            break;
        case EXPONENTIAL_PLUS_MIN:
            if(currentChar >= '0' && currentChar <= '9')
            {
                lexicalAnalyzer->state = EXPONENTIAL_FINAL;
            }
            else
            {
                lexicalAnalyzer->state = INVALID;
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case EXPONENTIAL_FINAL:
            if(currentChar >= '0' && currentChar <= '9')
            {
                lexicalAnalyzer->state = EXPONENTIAL_FINAL;
            }
            else if(currentChar == '_')
            {
                lexicalAnalyzer->state = EXPONENTIAL_;
            }
            else if(currentChar == 'd' || currentChar == 'D')
            {
                lexicalAnalyzer->state = EXPONENTIAL_FINAL_D;
            }
            else
            {
                setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_DOUBLE_LITERAL, false, false);
                if(compilerStatus == COMPILER_INTERNAL_ERROR)
                {
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    free(tokenLexeme);
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
            if(currentChar == '\"')
            {
                lexicalAnalyzer->state = MULTIPLE_LINE_STRING_TWO;
            }
            else if(currentChar != '\"')
            {
                if(currentChar == '\n')
                {
                    lexicalAnalyzer->newlineEnded = false;
                }
                lexicalAnalyzer->state = MULTIPLE_LINE_STRING_INSIDE;
            }
            break;
        case EXPONENTIAL_:
            if(currentChar >= '0' && currentChar <= '9')
            {
                lexicalAnalyzer->state = EXPONENTIAL_FINAL;
            }
            else
            {
                lexicalAnalyzer->state = INVALID;
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case EXPONENTIAL_FINAL_D:
            setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
            compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_DOUBLE_LITERAL, false, false);
            if(compilerStatus == COMPILER_INTERNAL_ERROR)
            {
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_INTERNAL_ERROR;
            }
            lexicalAnalyzer->lineHasToken = true;
            break;
        case MULTIPLE_LINE_STRING_TWO:
            if(currentChar == '\"')
            {
                lexicalAnalyzer->state = MULTIPLE_LINE_STRING_FINAL;
            }
            else if(currentChar != '\"')
            {
                if(currentChar == '\n')
                {
                    lexicalAnalyzer->newlineEnded = false;
                }
                lexicalAnalyzer->state = MULTIPLE_LINE_STRING_INSIDE;
            }
            break;
        case MULTIPLE_LINE_STRING_FINAL:
            setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
            compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_STRING_LITERAL, false, false);
            if(compilerStatus == COMPILER_INTERNAL_ERROR)
            {
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_INTERNAL_ERROR;
            }
            lexicalAnalyzer->lineHasToken = true;
            break;
        case OPERATOR_FINAL:
            setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
            compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_OPERATOR, false, false);
            if(compilerStatus == COMPILER_INTERNAL_ERROR)
            {
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_INTERNAL_ERROR;
            }
            lexicalAnalyzer->lineHasToken = true;
            break;
        case SINGLE_OR_DOUBLE_OP_FINAL:
            if(currentChar == '=')
            {
                lexicalAnalyzer->state = DOUBLE_OPERATOR_FINAL;
            }
            else
            {
                setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
                compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_OPERATOR, false, false);
                if(compilerStatus == COMPILER_INTERNAL_ERROR)
                {
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    free(tokenLexeme);
                    return COMPILER_INTERNAL_ERROR;
                }
                lexicalAnalyzer->lineHasToken = true;
            }
            break;
        case DOUBLE_OPERATOR:
            if(currentChar == '=')
            {
                lexicalAnalyzer->state = DOUBLE_OPERATOR_FINAL;
            }
            else
            {
                lexicalAnalyzer->state = INVALID;
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case DOUBLE_OPERATOR_FINAL:
            setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
            compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_OPERATOR, false, false);
            if(compilerStatus == COMPILER_INTERNAL_ERROR)
            {
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_INTERNAL_ERROR;
            }
            lexicalAnalyzer->lineHasToken = true;
            break;
        case RIGHT_PAREN_FINAL:
            setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
            compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_RIGHT_PAREN, false, false);
            if(compilerStatus == COMPILER_INTERNAL_ERROR)
            {
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_INTERNAL_ERROR;
            }
            lexicalAnalyzer->lineHasToken = true;
            break;
        case COMMA_FINAL:
            setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
            compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_COMMA, false, false);
            if(compilerStatus == COMPILER_INTERNAL_ERROR)
            {
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_INTERNAL_ERROR;
            }
            lexicalAnalyzer->lineHasToken = true;
            break;
        case COLON_FINAL:
            setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
            compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_COLON, false, false);
            if(compilerStatus == COMPILER_INTERNAL_ERROR)
            {
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_INTERNAL_ERROR;
            }
            lexicalAnalyzer->lineHasToken = true;
            break;
        case SEMICOLON_FINAL:
            setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
            compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_SEMICOLON, false, false);
            if(compilerStatus == COMPILER_INTERNAL_ERROR)
            {
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_INTERNAL_ERROR;
            }
            lexicalAnalyzer->lineHasToken = true;
            break;
        case AT_FINAL:
            setLexicalAnalyzerToStart(lexicalAnalyzer, currentChar);
            compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_AT, false, false);
            if(compilerStatus == COMPILER_INTERNAL_ERROR)
            {
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_INTERNAL_ERROR;
            }
            lexicalAnalyzer->lineHasToken = true;
            break;
        case INVALID:
            break;
        default:
            compilerError = COMPILER_LEXICAL_ERROR;
            lexicalAnalyzer->state = startCases(currentChar, lexicalAnalyzer, outToken, tokenLexeme, &tokenLexemeLength, &compilerError);
            if(lexicalAnalyzer->state == INVALID)
            {
                lexicalAnalyzer->state = INVALID;
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return compilerError;
            }
            break;
        }
    }

    if(commentEnded)
    {
        strcpy(tokenLexeme, "");
        tokenLexemeLength = 0;
        commentEnded = false;
        if(lexicalAnalyzer->prevCharRead)
        {
            setTokenLocation(outToken, lexicalAnalyzer->prevCharLine, lexicalAnalyzer->prevCharColumn);
            lexicalAnalyzer->prevCharRead = false;
            if(!stringAppend(&tokenLexeme, &tokenLexemeLength, lexicalAnalyzer->prevChar))
            {
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_INTERNAL_ERROR;
            }
            if(newlineInComment)
            {
                newlineInComment = false;
                if(lexicalAnalyzer->prevChar == '\n')
                {
                    if(lexicalAnalyzer->lineHasToken && lexicalAnalyzer->parenDepth <= 0)
                    {
                        lexicalAnalyzer->state = NEWLINE_FINAL;
                    }
                    else
                    {
                        strcpy(tokenLexeme, "");
                        tokenLexemeLength = 0;
                        setTokenLocation(outToken, lexicalAnalyzer->line, lexicalAnalyzer->column);
                        lexicalAnalyzer->state = START;
                    }
                }
                else if(lexicalAnalyzer->prevChar == ' ' || lexicalAnalyzer->prevChar == '\t' || lexicalAnalyzer->prevChar == '\v' || lexicalAnalyzer->prevChar == '\f')
                {
                    lexicalAnalyzer->state = AFTER_COMMENT_CHECK;
                }
                else
                {
                    lexicalAnalyzer->state = INVALID;
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    free(tokenLexeme);
                    return COMPILER_LEXICAL_ERROR;
                }
            }
            else
            {
                compilerError = COMPILER_LEXICAL_ERROR;
                lexicalAnalyzer->state = startCases(lexicalAnalyzer->prevChar, lexicalAnalyzer, outToken, tokenLexeme, &tokenLexemeLength, &compilerError);
                if(lexicalAnalyzer->state == INVALID)
                {
                    lexicalAnalyzer->state = INVALID;
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    free(tokenLexeme);
                    return compilerError;
                }
            }
        }
        else
        {
            setTokenLocation(outToken, lexicalAnalyzer->line, lexicalAnalyzer->column);
        }
    }


    if(currentChar == EOF)
    {
        if(ferror(lexicalAnalyzer->input) != 0)
        {
            tokenDestroy(*outToken);
            *outToken = NULL;
            free(tokenLexeme);
            return COMPILER_INTERNAL_ERROR;
        }
        lexicalAnalyzer->reachedEof = true;
    }

    if(lexicalAnalyzer->reachedEof)
    {
        if(isFinal(lexicalAnalyzer->state))
        {
            switch(lexicalAnalyzer->state)
            {
                case ID_FINAL1:
                    lexicalAnalyzer->state = START;
                    compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_IDENTIFIER, true, false);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        free(tokenLexeme);
                        return COMPILER_INTERNAL_ERROR;
                    }
                    break;
                case NEWLINE_FINAL:
                    lexicalAnalyzer->logicalIndentKnown = false;
                    lexicalAnalyzer->continuationPending = false;
                    lexicalAnalyzer->state = START;
                    setTokenLocation(outToken, lexicalAnalyzer->newlineLine, lexicalAnalyzer->newlineColumn);
                    compilerStatus = setToken(outToken, tokenLexeme, TOKEN_NEWLINE);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        free(tokenLexeme);
                        return COMPILER_INTERNAL_ERROR;
                    }
                    lexicalAnalyzer->lineHasToken = false;
                    break;
                case ONE_LINE_COMMENT_FINAL:
                    if(lexicalAnalyzer->lineHasToken && lexicalAnalyzer->parenDepth <= 0)
                    {
                        lexicalAnalyzer->state = START;
                        setTokenLocation(outToken, lexicalAnalyzer->prevCharLine, lexicalAnalyzer->prevCharColumn);
                        compilerStatus = setToken(outToken, tokenLexeme, TOKEN_NEWLINE);
                        if(compilerStatus == COMPILER_INTERNAL_ERROR)
                        {
                            tokenDestroy(*outToken);
                            *outToken = NULL;
                            free(tokenLexeme);
                            return COMPILER_INTERNAL_ERROR;
                        }
                        lexicalAnalyzer->lineHasToken = false;
                        lexicalAnalyzer->logicalIndentKnown = false;
                        lexicalAnalyzer->continuationPending = false;
                    }
                    else
                    {
                        (*outToken)->type = TOKEN_EOF;
                        compilerStatus = COMPILER_OK;
                        lexicalAnalyzer->state = START;
                    }
                    break;
                case ID_FINAL2:
                    lexicalAnalyzer->state = START;
                    compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_IDENTIFIER, true, false);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        free(tokenLexeme);
                        return COMPILER_INTERNAL_ERROR;
                    }
                    break;
                case ZERO_FINAL:
                    lexicalAnalyzer->state = START;
                    compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_INT_LITERAL, true, false);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        free(tokenLexeme);
                        return COMPILER_INTERNAL_ERROR;
                    }
                    break;
                /*case ZERO_PLUS_FINAL:
                    lexicalAnalyzer->state = START;
                    compilerStatus = editSetToken(outToken, tokenLexeme, &tokenLexemeLength, TOKEN_INT_LITERAL, true, false);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        free(tokenLexeme);
                        return COMPILER_INTERNAL_ERROR;
                    }
                    break;*/
                case INT_FINAL:
                    lexicalAnalyzer->state = START;
                    compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_INT_LITERAL, true, false);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        free(tokenLexeme);
                        return COMPILER_INTERNAL_ERROR;
                    }
                    break;
                /*case INT_PLUS_FINAL:
                    lexicalAnalyzer->state = START;
                    compilerStatus = editSetToken(outToken, tokenLexeme, &tokenLexemeLength, TOKEN_INT_LITERAL, true, false);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        free(tokenLexeme);
                        return COMPILER_INTERNAL_ERROR;
                    }
                    break;*/
                case DOUBLE_NO_EXP_FINAL:
                    lexicalAnalyzer->state = START;
                    compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_DOUBLE_LITERAL, true, false);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        free(tokenLexeme);
                        return COMPILER_INTERNAL_ERROR;
                    }
                    break;
                case DOUBLE_NO_EXP_D_FINAL:
                    lexicalAnalyzer->state = START;
                    compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_DOUBLE_LITERAL, true, false);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        free(tokenLexeme);
                        return COMPILER_INTERNAL_ERROR;
                    }
                    break;
                case EXPONENTIAL_FINAL:
                    lexicalAnalyzer->state = START;
                    compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_DOUBLE_LITERAL, true, false);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        free(tokenLexeme);
                        return COMPILER_INTERNAL_ERROR;
                    }
                    break;
                case EXPONENTIAL_FINAL_D:
                    lexicalAnalyzer->state = START;
                    compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_DOUBLE_LITERAL, true, false);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        free(tokenLexeme);
                        return COMPILER_INTERNAL_ERROR;
                    }
                    break;
                case ONE_LINE_STRING_FINAL:
                    lexicalAnalyzer->state = START;
                    compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_STRING_LITERAL, true, true);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        free(tokenLexeme);
                        return COMPILER_INTERNAL_ERROR;
                    }
                    break;
                case ONE_LINE_STRING_NO_EMPTY_FINAL:
                    lexicalAnalyzer->state = START;
                    compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_STRING_LITERAL, true, true);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        free(tokenLexeme);
                        return COMPILER_INTERNAL_ERROR;
                    }
                    break;
                case MULTIPLE_LINE_STRING_FINAL:
                    lexicalAnalyzer->state = START;
                    compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_STRING_LITERAL, true, false);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        free(tokenLexeme);
                        return COMPILER_INTERNAL_ERROR;
                    }
                    break;
                case OPERATOR_FINAL:
                    lexicalAnalyzer->state = START;
                    compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_OPERATOR, true, false);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        free(tokenLexeme);
                        return COMPILER_INTERNAL_ERROR;
                    }
                    break;
                case COMMENT:
                    if(!beginRealToken(lexicalAnalyzer, lexicalAnalyzer->slashIndent, lexicalAnalyzer->slashWasPending))
                    {
                        lexicalAnalyzer->state = INVALID;
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        free(tokenLexeme);
                        return COMPILER_LEXICAL_ERROR;
                    }
                    lexicalAnalyzer->continuationPending = false;
                    lexicalAnalyzer->state = START;
                    compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_OPERATOR, true, false);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        free(tokenLexeme);
                        return COMPILER_INTERNAL_ERROR;
                    }
                    break;
                case SINGLE_OR_DOUBLE_OP_FINAL:
                    lexicalAnalyzer->state = START;
                    compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_OPERATOR, true, false);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        free(tokenLexeme);
                        return COMPILER_INTERNAL_ERROR;
                    }
                    break;
                case DOUBLE_OPERATOR_FINAL:
                    lexicalAnalyzer->state = START;
                    compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_OPERATOR, true, false);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        free(tokenLexeme);
                        return COMPILER_INTERNAL_ERROR;
                    }
                    break;
                /*case UNIT_FINAL:
                    lexicalAnalyzer->state = START;
                    compilerStatus = editSetToken(outToken, tokenLexeme, &tokenLexemeLength, TOKEN_UNIT_LITERAL, true, false);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        free(tokenLexeme);
                        return COMPILER_INTERNAL_ERROR;
                    }
                    break;*/
                case LEFT_PAREN_FINAL:
                    lexicalAnalyzer->state = START;
                    compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_LEFT_PAREN, true, false);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        free(tokenLexeme);
                        return COMPILER_INTERNAL_ERROR;
                    }
                    break;
                case RIGHT_PAREN_FINAL:
                    lexicalAnalyzer->state = START;
                    compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_RIGHT_PAREN, true, false);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        free(tokenLexeme);
                        return COMPILER_INTERNAL_ERROR;
                    }
                    break;
                case COMMA_FINAL:
                    lexicalAnalyzer->state = START;
                    compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_COMMA, true, false);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        free(tokenLexeme);
                        return COMPILER_INTERNAL_ERROR;
                    }
                    break;
                case COLON_FINAL:
                    lexicalAnalyzer->state = START;
                    compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_COLON, true, false);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        free(tokenLexeme);
                        return COMPILER_INTERNAL_ERROR;
                    }
                    break;
                case SEMICOLON_FINAL:
                    lexicalAnalyzer->state = START;
                    compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_SEMICOLON, true, false);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        free(tokenLexeme);
                        return COMPILER_INTERNAL_ERROR;
                    }
                    break;
                case DOT_FINAL:
                    lexicalAnalyzer->state = START;
                    compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_DOT, true, false);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        free(tokenLexeme);
                        return COMPILER_INTERNAL_ERROR;
                    }
                    break;
                case AT_FINAL:
                    lexicalAnalyzer->state = START;
                    compilerStatus = editSetToken(outToken, lexicalAnalyzer, tokenLexeme, &tokenLexemeLength, TOKEN_AT, true, false);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        free(tokenLexeme);
                        return COMPILER_INTERNAL_ERROR;
                    }
                    break;
                default:
                    lexicalAnalyzer->state = START;
                    (*outToken)->type = TOKEN_EOF;
                    compilerStatus = COMPILER_OK;
                    break;
            }
        }
        else
        {
            if(lexicalAnalyzer->state == START || lexicalAnalyzer->state == AFTER_COMMENT_CHECK || lexicalAnalyzer->state == ONE_LINE_COMMENT || lexicalAnalyzer->state == INDENT_COUNT || lexicalAnalyzer->state == TAB_CHECK)
            {
                lexicalAnalyzer->state = START;
                (*outToken)->type = TOKEN_EOF;
                compilerStatus = COMPILER_OK;
            }
            else
            {
                tokenDestroy(*outToken);
                *outToken = NULL;
                free(tokenLexeme);
                return COMPILER_LEXICAL_ERROR;
            }
        }
    }

    free(tokenLexeme);

    if ((*outToken)->type == TOKEN_EOF)
    {
        return finishAtEof(lexicalAnalyzer, outToken);
    }

    return compilerStatus;
}
