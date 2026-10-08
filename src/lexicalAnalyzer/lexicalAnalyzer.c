/*
 * Project: IFJ26 compiler
 * Authors: xkubanm00
 * AI-assisted scaffold.
 * Prompt: Create lexical-analyzer lifecycle scaffolding without automaton logic.
 */

#include "lexicalAnalyzer.h"

#include "memory.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>


enum State {
  INVALID,
  START,
  ID_,
  BRACKETS,
  ID_FINAL1,
  ZERO_FINAL,
  NUM_PLUS,
  INT_FINAL,
  DOUBLE,
  COMMENT,
  STRING,
  ID_FINAL2,
  BRACKETS_FINAL,
  ZERO_PLUS_FINAL,
  INT_PLUS_FINAL,
  INT_,
  DOUBLE_NO_EXP_FINAL,
  ONE_LINE_COMMENT,
  MULTIPLE_LINE_COMMENT,
  ONE_LINE_STRING,
  MULTIPLE_LINE_STRING,
  ESCAPE_STRING,
  EXPONENTIAL,
  INT_PLUS_,
  DOUBLE_NO_EXP_,
  ONE_LINE_COMMENT_FINAL,
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
  ONE_LINE_STRING_NO_EMPTY_FINAL
};

struct LexicalAnalyzer {
    FILE *input;
    size_t line;
    size_t column;
    size_t currentIndent;
    enum State state;
    int prevChar;
    bool prevCharRead;
    bool reachedEof;
};


LexicalAnalyzer *lexicalAnalyzerCreate(FILE *input)
{
    if (input == NULL) {
        return NULL;
    }

    LexicalAnalyzer *lexicalAnalyzer = memoryAllocate(sizeof(LexicalAnalyzer));
    if (lexicalAnalyzer == NULL) {
        return NULL;
    }

    lexicalAnalyzer->input = input;
    lexicalAnalyzer->line = 1;
    lexicalAnalyzer->column = 1;
    lexicalAnalyzer->currentIndent = 0;
    lexicalAnalyzer->state = START;
    lexicalAnalyzer->prevCharRead = false;
    lexicalAnalyzer->reachedEof = false;
    return lexicalAnalyzer;
}

bool isFinal(enum State state)
{
    return (state == ID_FINAL1 || state == ID_FINAL2 || state == ZERO_FINAL || state == ZERO_PLUS_FINAL || state == INT_FINAL
        || state == INT_PLUS_FINAL || state == DOUBLE_NO_EXP_FINAL || state == DOUBLE_NO_EXP_D_FINAL || state == ONE_LINE_COMMENT_FINAL
        || state == ONE_LINE_STRING_FINAL || state == ONE_LINE_STRING_NO_EMPTY_FINAL || state == EXPONENTIAL_FINAL || state == EXPONENTIAL_FINAL_D
        || state == MULTIPLE_LINE_STRING_FINAL);
}

void removeWhiteChars(char * string, int * length)
{
    char clear_string[100];
    int idx2 = 0;
    for(int idx1 = 0; string[idx1] != '\0'; idx1++)
    {
        if(string[idx1] != '\t' && string[idx1] != '\v' && string[idx1] != '\f' && string[idx1] != ' ')
        {
            clear_string[idx2] = string[idx1];
            idx2++;
        }
    }
    clear_string[idx2] = '\0';
    strcpy(string, clear_string);
    *length = idx2;
}

void decodeEscapeSeq(char * string, int * length)
{
    char clearString[100];
    int idx2 = 0;
    for(int idx1 = 0; string[idx1] != '\0'; idx1++)
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
}

void removeUnnecessaryFromNumber(char * string)
{
    char clearString[100];
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
}

int64_t stringToInt(char * string, char ** endptr)
{
    removeUnnecessaryFromNumber(string);

    long longResult = strtol(string, endptr, 10);
    
    return (int64_t)longResult; 
}

double stringToDouble(char * string, char ** endptr)
{
    removeUnnecessaryFromNumber(string);
    
    return strtod(string, endptr); 
}

void lexicalAnalyzerDestroy(LexicalAnalyzer *lexicalAnalyzer)
{
    free(lexicalAnalyzer);
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
    *outToken = tokenCreate();
    if(*outToken == NULL)
    {
        return COMPILER_INTERNAL_ERROR;
    }

    /* Team TODO: implement the required finite-state scanner manually. */
    int currentChar;
    char tokenLexeme[100];
    strcpy(tokenLexeme, "");
    int tokenLexemeLength = 0;
    bool commentEnded = false;
    (*outToken)->location.line = lexicalAnalyzer->line;
    (*outToken)->location.column = lexicalAnalyzer->column;
    if(lexicalAnalyzer->prevCharRead)
    {
        lexicalAnalyzer->prevCharRead = false;
        currentChar = lexicalAnalyzer->prevChar;
        tokenLexeme[tokenLexemeLength] = lexicalAnalyzer->prevChar;
        tokenLexemeLength++;
        tokenLexeme[tokenLexemeLength] = '\0';
        if(currentChar == '_')
        {
            lexicalAnalyzer->state = ID_; 
        }
        else if((currentChar >= 'a' && currentChar <= 'z') || (currentChar >= 'A' && currentChar <= 'Z'))
        {
            lexicalAnalyzer->state = ID_FINAL1;
        }
        else if(currentChar == '(')
        {
            lexicalAnalyzer->state = BRACKETS;
        }
        else if(currentChar == '0')
        {
            lexicalAnalyzer->state = ZERO_FINAL;
        }
        else if(currentChar == '+')
        {
            lexicalAnalyzer->state = NUM_PLUS;
        }
        else if(currentChar >= '1' && currentChar <= '9')
        {
            lexicalAnalyzer->state = INT_FINAL;
        }
        else if(currentChar == '.')
        {
            lexicalAnalyzer->state = DOUBLE;
        }
        else if(currentChar == '/')
        {
            lexicalAnalyzer->state = COMMENT;
        }
        else if(currentChar == '\"')
        {
            lexicalAnalyzer->state = STRING;
        }
    }
    CompilerStatus compilerStatus = COMPILER_INTERNAL_ERROR;
    while((*outToken)->type == TOKEN_INVALID && (currentChar = fgetc(lexicalAnalyzer->input)) != EOF)
    {
        if(commentEnded)
        {
            (*outToken)->location.line = lexicalAnalyzer->line;
            (*outToken)->location.column = lexicalAnalyzer->column;
            strcpy(tokenLexeme, "");
            tokenLexemeLength = 0;
            commentEnded = false;
            if(lexicalAnalyzer->prevCharRead)
            {
                lexicalAnalyzer->prevCharRead = false;
                tokenLexeme[tokenLexemeLength] = lexicalAnalyzer->prevChar;
                tokenLexemeLength++;
                tokenLexeme[tokenLexemeLength] = '\0';
                if(lexicalAnalyzer->prevChar == '_')
                {
                    lexicalAnalyzer->state = ID_; 
                }
                else if((lexicalAnalyzer->prevChar >= 'a' && lexicalAnalyzer->prevChar <= 'z') || (lexicalAnalyzer->prevChar >= 'A' && lexicalAnalyzer->prevChar <= 'Z'))
                {
                    lexicalAnalyzer->state = ID_FINAL1;
                }
                else if(lexicalAnalyzer->prevChar == '(')
                {
                    lexicalAnalyzer->state = BRACKETS;
                }
                else if(lexicalAnalyzer->prevChar == '0')
                {
                    lexicalAnalyzer->state = ZERO_FINAL;
                }
                else if(lexicalAnalyzer->prevChar == '+')
                {
                    lexicalAnalyzer->state = NUM_PLUS;
                }
                else if(lexicalAnalyzer->prevChar >= '1' && lexicalAnalyzer->prevChar <= '9')
                {
                    lexicalAnalyzer->state = INT_FINAL;
                }
                else if(lexicalAnalyzer->prevChar == '.')
                {
                    lexicalAnalyzer->state = DOUBLE;
                }
                else if(lexicalAnalyzer->prevChar == '/')
                {
                    lexicalAnalyzer->state = COMMENT;
                }
                else if(lexicalAnalyzer->prevChar == '\"')
                {
                    lexicalAnalyzer->state = STRING;
                }
            }
        }
        // Append string 
        tokenLexeme[tokenLexemeLength] = currentChar;
        tokenLexeme[tokenLexemeLength + 1] = '\0';
        tokenLexemeLength++;

        if(currentChar == '\n')
        {
            lexicalAnalyzer->column = 1;
            lexicalAnalyzer->line++;
        }
        else
        {
            lexicalAnalyzer->column++;
        }
        switch(lexicalAnalyzer->state)
        {
        case START:
            if(currentChar == '_')
            {
                lexicalAnalyzer->state = ID_; 
            }
            else if((currentChar >= 'a' && currentChar <= 'z') || (currentChar >= 'A' && currentChar <= 'Z'))
            {
                lexicalAnalyzer->state = ID_FINAL1;
            }
            else if(currentChar == '(')
            {
                lexicalAnalyzer->state = BRACKETS;
            }
            else if(currentChar == '0')
            {
                lexicalAnalyzer->state = ZERO_FINAL;
            }
            else if(currentChar == '+')
            {
                lexicalAnalyzer->state = NUM_PLUS;
            }
            else if(currentChar >= '1' && currentChar <= '9')
            {
                lexicalAnalyzer->state = INT_FINAL;
            }
            else if(currentChar == '.')
            {
                lexicalAnalyzer->state = DOUBLE;
            }
            else if(currentChar == '/')
            {
                lexicalAnalyzer->state = COMMENT;
            }
            else if(currentChar == '\"')
            {
                lexicalAnalyzer->state = STRING;
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
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case BRACKETS:
            if(currentChar == ')')
            {
                lexicalAnalyzer->state = BRACKETS_FINAL;
            }
            else if(currentChar != ')')
            {
                lexicalAnalyzer->state = BRACKETS;
            }
            //todo
            break;
        case ID_FINAL1:
            if((currentChar >= 'a' && currentChar <= 'z') || (currentChar >= 'A' && currentChar <= 'Z') || (currentChar >= '0' && currentChar <= '9') || currentChar == '_')
            {
                lexicalAnalyzer->state = ID_FINAL1;
            }
            else
            {
                lexicalAnalyzer->state = START;
                lexicalAnalyzer->prevChar = currentChar;
                lexicalAnalyzer->prevCharRead = true;
                tokenLexeme[tokenLexemeLength-1] = '\0';
                tokenLexemeLength--;
                removeWhiteChars(tokenLexeme, &tokenLexemeLength);
                compilerStatus = tokenSetLexeme((*outToken), tokenLexeme);
                if(compilerStatus == COMPILER_INTERNAL_ERROR)
                {
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    return COMPILER_INTERNAL_ERROR;
                }
                (*outToken)->type = TOKEN_IDENTIFIER;
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
                return COMPILER_LEXICAL_ERROR;
            }
            else
            {
                lexicalAnalyzer->state = START;
                lexicalAnalyzer->prevChar = currentChar;
                lexicalAnalyzer->prevCharRead = true;
                tokenLexeme[tokenLexemeLength-1] = '\0';
                tokenLexemeLength--;
                removeWhiteChars(tokenLexeme, &tokenLexemeLength);
                compilerStatus = tokenSetLexeme((*outToken), tokenLexeme);
                if(compilerStatus == COMPILER_INTERNAL_ERROR)
                {
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    return COMPILER_INTERNAL_ERROR;
                }
                char *endptr;
                (*outToken)->value.intValue = stringToInt(tokenLexeme, &endptr);
                if(*endptr != '\0')
                {
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    return COMPILER_INTERNAL_ERROR;
                }
                (*outToken)->type = TOKEN_INT_LITERAL;
            }
            break;
        case NUM_PLUS:
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
                return COMPILER_LEXICAL_ERROR;
            }
            break;
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
                lexicalAnalyzer->state = START;
                lexicalAnalyzer->prevChar = currentChar;
                lexicalAnalyzer->prevCharRead = true;
                tokenLexeme[tokenLexemeLength-1] = '\0';
                tokenLexemeLength--;
                removeWhiteChars(tokenLexeme, &tokenLexemeLength);
                compilerStatus = tokenSetLexeme((*outToken), tokenLexeme);
                if(compilerStatus == COMPILER_INTERNAL_ERROR)
                {
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    return COMPILER_INTERNAL_ERROR;
                }
                char *endptr;
                (*outToken)->value.intValue = stringToInt(tokenLexeme, &endptr);
                if(*endptr != '\0')
                {
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    return COMPILER_INTERNAL_ERROR;
                }
                (*outToken)->type = TOKEN_INT_LITERAL;
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
                return COMPILER_LEXICAL_ERROR;
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
                lexicalAnalyzer->state = INVALID;
                tokenDestroy(*outToken);
                *outToken = NULL;
                return COMPILER_LEXICAL_ERROR;
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
                lexicalAnalyzer->state = START;
                lexicalAnalyzer->prevChar = currentChar;
                lexicalAnalyzer->prevCharRead = true;
                tokenLexeme[tokenLexemeLength-1] = '\0';
                tokenLexemeLength--;
                removeWhiteChars(tokenLexeme, &tokenLexemeLength);
                compilerStatus = tokenSetLexeme((*outToken), tokenLexeme);
                if(compilerStatus == COMPILER_INTERNAL_ERROR)
                {
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    return COMPILER_INTERNAL_ERROR;
                }
                (*outToken)->type = TOKEN_IDENTIFIER;
            }
            break;
        case BRACKETS_FINAL:
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
                return COMPILER_LEXICAL_ERROR;
            }
            else
            {
                lexicalAnalyzer->state = START;
                lexicalAnalyzer->prevChar = currentChar;
                lexicalAnalyzer->prevCharRead = true;
                tokenLexeme[tokenLexemeLength-1] = '\0';
                tokenLexemeLength--;
                removeWhiteChars(tokenLexeme, &tokenLexemeLength);
                compilerStatus = tokenSetLexeme((*outToken), tokenLexeme);
                if(compilerStatus == COMPILER_INTERNAL_ERROR)
                {
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    return COMPILER_INTERNAL_ERROR;
                }
                char *endptr;
                (*outToken)->value.intValue = stringToInt(tokenLexeme, &endptr);
                if(*endptr != '\0')
                {
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    return COMPILER_INTERNAL_ERROR;
                }
                (*outToken)->type = TOKEN_INT_LITERAL;
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
                lexicalAnalyzer->state = START;
                lexicalAnalyzer->prevChar = currentChar;
                lexicalAnalyzer->prevCharRead = true;
                tokenLexeme[tokenLexemeLength-1] = '\0';
                tokenLexemeLength--;
                removeWhiteChars(tokenLexeme, &tokenLexemeLength);
                compilerStatus = tokenSetLexeme((*outToken), tokenLexeme);
                if(compilerStatus == COMPILER_INTERNAL_ERROR)
                {
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    return COMPILER_INTERNAL_ERROR;
                }
                char *endptr;
                (*outToken)->value.intValue = stringToInt(tokenLexeme, &endptr);
                if(*endptr != '\0')
                {
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    return COMPILER_INTERNAL_ERROR;
                }
                (*outToken)->type = TOKEN_INT_LITERAL;
            }
            break;
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
                lexicalAnalyzer->state = START;
                lexicalAnalyzer->prevChar = currentChar;
                lexicalAnalyzer->prevCharRead = true;
                tokenLexeme[tokenLexemeLength-1] = '\0';
                tokenLexemeLength--;
                removeWhiteChars(tokenLexeme, &tokenLexemeLength);
                compilerStatus = tokenSetLexeme((*outToken), tokenLexeme);
                if(compilerStatus == COMPILER_INTERNAL_ERROR)
                {
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    return COMPILER_INTERNAL_ERROR;
                }
                char *endptr;
                (*outToken)->value.doubleValue = stringToDouble(tokenLexeme, &endptr);
                if(*endptr != '\0')
                {
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    return COMPILER_INTERNAL_ERROR;
                }
                (*outToken)->type = TOKEN_DOUBLE_LITERAL;
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
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case MULTIPLE_LINE_COMMENT:
            if(currentChar != '*')
            {
                lexicalAnalyzer->state = MULTIPLE_LINE_COMMENT;
            }
            else if(currentChar == '*')
            {
                lexicalAnalyzer->state = MULTIPLE_LINE_COMMENT2;
            }
            //maybe else
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
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case ONE_LINE_STRING_NO_EMPTY_FINAL:
            lexicalAnalyzer->state = START;
            lexicalAnalyzer->prevChar = currentChar;
            lexicalAnalyzer->prevCharRead = true;
            tokenLexeme[tokenLexemeLength-1] = '\0';
            tokenLexemeLength--;
            decodeEscapeSeq(tokenLexeme, &tokenLexemeLength);
            compilerStatus = tokenSetLexeme((*outToken), tokenLexeme);
            if(compilerStatus == COMPILER_INTERNAL_ERROR)
            {
                tokenDestroy(*outToken);
                *outToken = NULL;
                return COMPILER_INTERNAL_ERROR;
            }
            (*outToken)->type = TOKEN_STRING_LITERAL;
            break;
        /*case MULTIPLE_LINE_STRING:
            if(currentChar == '\"')
            {
                lexicalAnalyzer->state = MULTIPLE_LINE_STRING_INSIDE;
            }
            else
            {
                lexicalAnalyzer->state = INVALID;
            }
            break;*/
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
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case INT_PLUS_:
            if(currentChar >= '0' && currentChar <= '9')
            {
                lexicalAnalyzer->state = INT_PLUS_FINAL;
            }
            else
            {
                lexicalAnalyzer->state = INVALID;
                tokenDestroy(*outToken);
                *outToken = NULL;
                return COMPILER_LEXICAL_ERROR;
            }
            break;
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
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case ONE_LINE_COMMENT_FINAL:
            lexicalAnalyzer->state = START;
            lexicalAnalyzer->prevChar = currentChar;
            lexicalAnalyzer->prevCharRead = true;
            commentEnded = true;
            break;
        case MULTIPLE_LINE_COMMENT2:
            if(currentChar != '/' && currentChar != '*')
            {
                lexicalAnalyzer->state = MULTIPLE_LINE_COMMENT;
            }
            else if(currentChar == '/')
            {
                lexicalAnalyzer->state = MULTIPLE_LINE_COMMENT_FINAL;
            }
            else if(currentChar == '*')
            {
                lexicalAnalyzer->state = MULTIPLE_LINE_COMMENT2;
            }
            break;
        case DOUBLE_NO_EXP_D_FINAL:
            lexicalAnalyzer->state = START;
            lexicalAnalyzer->prevChar = currentChar;
            lexicalAnalyzer->prevCharRead = true;
            tokenLexeme[tokenLexemeLength-1] = '\0';
            tokenLexemeLength--;
            removeWhiteChars(tokenLexeme, &tokenLexemeLength);
            compilerStatus = tokenSetLexeme((*outToken), tokenLexeme);
            if(compilerStatus == COMPILER_INTERNAL_ERROR)
            {
                tokenDestroy(*outToken);
                *outToken = NULL;
                return COMPILER_INTERNAL_ERROR;
            }
            char *endptr;
            (*outToken)->value.doubleValue = stringToDouble(tokenLexeme, &endptr);
            if(*endptr != '\0')
            {
                tokenDestroy(*outToken);
                *outToken = NULL;
                return COMPILER_INTERNAL_ERROR;
            }
            (*outToken)->type = TOKEN_DOUBLE_LITERAL;
            break;
        case ONE_LINE_STRING_FINAL:
            if(currentChar == '\"')
            {
                lexicalAnalyzer->state = MULTIPLE_LINE_STRING_INSIDE;
            }
            else
            {
                lexicalAnalyzer->state = START;
                lexicalAnalyzer->prevChar = currentChar;
                lexicalAnalyzer->prevCharRead = true;
                tokenLexeme[tokenLexemeLength-1] = '\0';
                tokenLexemeLength--;
                decodeEscapeSeq(tokenLexeme, &tokenLexemeLength);
                compilerStatus = tokenSetLexeme((*outToken), tokenLexeme);
                if(compilerStatus == COMPILER_INTERNAL_ERROR)
                {
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    return COMPILER_INTERNAL_ERROR;
                }
                (*outToken)->type = TOKEN_STRING_LITERAL;
            }
            break;
        case MULTIPLE_LINE_STRING_INSIDE:
            if(currentChar == '\"')
            {
                lexicalAnalyzer->state = MULTIPLE_LINE_STRING_ONE;
            }
            else if(currentChar != '\"')
            {
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
                lexicalAnalyzer->state = START;
                lexicalAnalyzer->prevChar = currentChar;
                lexicalAnalyzer->prevCharRead = true;
                tokenLexeme[tokenLexemeLength-1] = '\0';
                tokenLexemeLength--;
                removeWhiteChars(tokenLexeme, &tokenLexemeLength);
                compilerStatus = tokenSetLexeme((*outToken), tokenLexeme);
                if(compilerStatus == COMPILER_INTERNAL_ERROR)
                {
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    return COMPILER_INTERNAL_ERROR;
                }
                char *endptr;
                (*outToken)->value.doubleValue = stringToDouble(tokenLexeme, &endptr);
                if(*endptr != '\0')
                {
                    tokenDestroy(*outToken);
                    *outToken = NULL;
                    return COMPILER_INTERNAL_ERROR;
                }
                (*outToken)->type = TOKEN_DOUBLE_LITERAL;
            }
            break;
        case MULTIPLE_LINE_COMMENT_FINAL:
            lexicalAnalyzer->state = START;
            lexicalAnalyzer->prevChar = currentChar;
            lexicalAnalyzer->prevCharRead = true;
            commentEnded = true;
            break;
        case MULTIPLE_LINE_STRING_ONE:
            if(currentChar == '\"')
            {
                lexicalAnalyzer->state = MULTIPLE_LINE_STRING_TWO;
            }
            else if(currentChar != '\"')
            {
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
                return COMPILER_LEXICAL_ERROR;
            }
            break;
        case EXPONENTIAL_FINAL_D:
            lexicalAnalyzer->state = START;
            lexicalAnalyzer->prevChar = currentChar;
            lexicalAnalyzer->prevCharRead = true;
            tokenLexeme[tokenLexemeLength-1] = '\0';
            tokenLexemeLength--;
            removeWhiteChars(tokenLexeme, &tokenLexemeLength);
            compilerStatus = tokenSetLexeme((*outToken), tokenLexeme);
            if(compilerStatus == COMPILER_INTERNAL_ERROR)
            {
                tokenDestroy(*outToken);
                *outToken = NULL;
                return COMPILER_INTERNAL_ERROR;
            }
            (*outToken)->value.doubleValue = stringToDouble(tokenLexeme, &endptr);
            if(*endptr != '\0')
            {
                tokenDestroy(*outToken);
                *outToken = NULL;
                return COMPILER_INTERNAL_ERROR;
            }
            (*outToken)->type = TOKEN_DOUBLE_LITERAL;
            break;
        case MULTIPLE_LINE_STRING_TWO:
            if(currentChar == '\"')
            {
                lexicalAnalyzer->state = MULTIPLE_LINE_STRING_FINAL;
            }
            else if(currentChar != '\"')
            {
                lexicalAnalyzer->state = MULTIPLE_LINE_STRING_INSIDE;
            }
            break;
        case MULTIPLE_LINE_STRING_FINAL:
            lexicalAnalyzer->state = START;
            lexicalAnalyzer->prevChar = currentChar;
            lexicalAnalyzer->prevCharRead = true;
            tokenLexeme[tokenLexemeLength-1] = '\0';
            tokenLexemeLength--;
            compilerStatus = tokenSetLexeme((*outToken), tokenLexeme);
            if(compilerStatus == COMPILER_INTERNAL_ERROR)
            {
                tokenDestroy(*outToken);
                *outToken = NULL;
                return COMPILER_INTERNAL_ERROR;
            }
            (*outToken)->type = TOKEN_STRING_LITERAL;
            break;
        case INVALID:
            break;
        default:
            if(currentChar == '_')
            {
                lexicalAnalyzer->state = ID_; 
            }
            else if((currentChar >= 'a' && currentChar <= 'z') || (currentChar >= 'A' && currentChar <= 'Z'))
            {
                lexicalAnalyzer->state = ID_FINAL1;
            }
            else if(currentChar == '(')
            {
                lexicalAnalyzer->state = BRACKETS;
            }
            else if(currentChar == '0')
            {
                lexicalAnalyzer->state = ZERO_FINAL;
            }
            else if(currentChar == '+')
            {
                lexicalAnalyzer->state = NUM_PLUS;
            }
            else if(currentChar >= '1' && currentChar <= '9')
            {
                lexicalAnalyzer->state = INT_FINAL;
            }
            else if(currentChar == '.')
            {
                lexicalAnalyzer->state = DOUBLE;
            }
            else if(currentChar == '/')
            {
                lexicalAnalyzer->state = COMMENT;
            }
            else if(currentChar == '\"')
            {
                lexicalAnalyzer->state = STRING;
            }
            break;
        }
    }

    if(commentEnded)
    {
        (*outToken)->location.line = lexicalAnalyzer->line;
        (*outToken)->location.column = lexicalAnalyzer->column;
        strcpy(tokenLexeme, "");
        tokenLexemeLength = 0;
        commentEnded = false;
        if(lexicalAnalyzer->prevCharRead)
        {
            lexicalAnalyzer->prevCharRead = false;
            tokenLexeme[tokenLexemeLength] = lexicalAnalyzer->prevChar;
            tokenLexemeLength++;
            tokenLexeme[tokenLexemeLength] = '\0';
            if(lexicalAnalyzer->prevChar == '_')
            {
                lexicalAnalyzer->state = ID_; 
            }
            else if((lexicalAnalyzer->prevChar >= 'a' && lexicalAnalyzer->prevChar <= 'z') || (lexicalAnalyzer->prevChar >= 'A' && lexicalAnalyzer->prevChar <= 'Z'))
            {
                lexicalAnalyzer->state = ID_FINAL1;
            }
            else if(lexicalAnalyzer->prevChar == '(')
            {
                lexicalAnalyzer->state = BRACKETS;
            }
            else if(lexicalAnalyzer->prevChar == '0')
            {
                lexicalAnalyzer->state = ZERO_FINAL;
            }
            else if(lexicalAnalyzer->prevChar == '+')
            {
                lexicalAnalyzer->state = NUM_PLUS;
            }
            else if(lexicalAnalyzer->prevChar >= '1' && lexicalAnalyzer->prevChar <= '9')
            {
                lexicalAnalyzer->state = INT_FINAL;
            }
            else if(lexicalAnalyzer->prevChar == '.')
            {
                lexicalAnalyzer->state = DOUBLE;
            }
            else if(lexicalAnalyzer->prevChar == '/')
            {
                lexicalAnalyzer->state = COMMENT;
            }
            else if(lexicalAnalyzer->prevChar == '\"')
            {
                lexicalAnalyzer->state = STRING;
            }
        }
    }


    if(currentChar == EOF)
    {
        if(ferror(lexicalAnalyzer->input) != 0)
        {
            tokenDestroy(*outToken);
            *outToken = NULL;
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
                    removeWhiteChars(tokenLexeme, &tokenLexemeLength);
                    compilerStatus = tokenSetLexeme((*outToken), tokenLexeme);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        return COMPILER_INTERNAL_ERROR;
                    }
                    (*outToken)->type = TOKEN_IDENTIFIER;
                    break;
                case ID_FINAL2:
                    lexicalAnalyzer->state = START;
                    removeWhiteChars(tokenLexeme, &tokenLexemeLength);
                    compilerStatus = tokenSetLexeme((*outToken), tokenLexeme);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        return COMPILER_INTERNAL_ERROR;
                    }
                    (*outToken)->type = TOKEN_IDENTIFIER;
                    break;
                case ZERO_FINAL:
                    lexicalAnalyzer->state = START;
                    removeWhiteChars(tokenLexeme, &tokenLexemeLength);
                    compilerStatus = tokenSetLexeme((*outToken), tokenLexeme);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        return COMPILER_INTERNAL_ERROR;
                    }
                    char *endptr;
                    (*outToken)->value.intValue = stringToInt(tokenLexeme, &endptr);
                    if(*endptr != '\0')
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        return COMPILER_INTERNAL_ERROR;
                    }
                    (*outToken)->type = TOKEN_INT_LITERAL;
                    break;
                case ZERO_PLUS_FINAL:
                    lexicalAnalyzer->state = START;
                    removeWhiteChars(tokenLexeme, &tokenLexemeLength);
                    compilerStatus = tokenSetLexeme((*outToken), tokenLexeme);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        return COMPILER_INTERNAL_ERROR;
                    }
                    (*outToken)->value.intValue = stringToInt(tokenLexeme, &endptr);
                    if(*endptr != '\0')
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        return COMPILER_INTERNAL_ERROR;
                    }
                    (*outToken)->type = TOKEN_INT_LITERAL;
                    break;
                case INT_FINAL:
                    lexicalAnalyzer->state = START;
                    removeWhiteChars(tokenLexeme, &tokenLexemeLength);
                    compilerStatus = tokenSetLexeme((*outToken), tokenLexeme);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        return COMPILER_INTERNAL_ERROR;
                    }
                    (*outToken)->value.intValue = stringToInt(tokenLexeme, &endptr);
                    if(*endptr != '\0')
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        return COMPILER_INTERNAL_ERROR;
                    }
                    (*outToken)->type = TOKEN_INT_LITERAL;
                    break;
                case INT_PLUS_FINAL:
                    lexicalAnalyzer->state = START;
                    removeWhiteChars(tokenLexeme, &tokenLexemeLength);
                    compilerStatus = tokenSetLexeme((*outToken), tokenLexeme);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        return COMPILER_INTERNAL_ERROR;
                    }
                    (*outToken)->value.intValue = stringToInt(tokenLexeme, &endptr);
                    if(*endptr != '\0')
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        return COMPILER_INTERNAL_ERROR;
                    }
                    (*outToken)->type = TOKEN_INT_LITERAL;
                    break;
                case DOUBLE_NO_EXP_FINAL:
                    lexicalAnalyzer->state = START;
                    removeWhiteChars(tokenLexeme, &tokenLexemeLength);
                    compilerStatus = tokenSetLexeme((*outToken), tokenLexeme);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        return COMPILER_INTERNAL_ERROR;
                    }
                    (*outToken)->value.doubleValue = stringToDouble(tokenLexeme, &endptr);
                    if(*endptr != '\0')
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        return COMPILER_INTERNAL_ERROR;
                    }
                    (*outToken)->type = TOKEN_DOUBLE_LITERAL;
                    break;
                case DOUBLE_NO_EXP_D_FINAL:
                    lexicalAnalyzer->state = START;
                    removeWhiteChars(tokenLexeme, &tokenLexemeLength);
                    compilerStatus = tokenSetLexeme((*outToken), tokenLexeme);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        return COMPILER_INTERNAL_ERROR;
                    }
                    (*outToken)->value.doubleValue = stringToDouble(tokenLexeme, &endptr);
                    if(*endptr != '\0')
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        return COMPILER_INTERNAL_ERROR;
                    }
                    (*outToken)->type = TOKEN_DOUBLE_LITERAL;
                    break;
                case EXPONENTIAL_FINAL:
                    lexicalAnalyzer->state = START;
                    removeWhiteChars(tokenLexeme, &tokenLexemeLength);
                    compilerStatus = tokenSetLexeme((*outToken), tokenLexeme);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        return COMPILER_INTERNAL_ERROR;
                    }
                    (*outToken)->value.doubleValue = stringToDouble(tokenLexeme, &endptr);
                    if(*endptr != '\0')
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        return COMPILER_INTERNAL_ERROR;
                    }
                    (*outToken)->type = TOKEN_DOUBLE_LITERAL;
                    break;
                case EXPONENTIAL_FINAL_D:
                    lexicalAnalyzer->state = START;
                    removeWhiteChars(tokenLexeme, &tokenLexemeLength);
                    compilerStatus = tokenSetLexeme((*outToken), tokenLexeme);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        return COMPILER_INTERNAL_ERROR;
                    }
                    (*outToken)->value.doubleValue = stringToDouble(tokenLexeme, &endptr);
                    if(*endptr != '\0')
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        return COMPILER_INTERNAL_ERROR;
                    }
                    (*outToken)->type = TOKEN_DOUBLE_LITERAL;
                    break;
                case ONE_LINE_STRING_FINAL:
                    lexicalAnalyzer->state = START;
                    decodeEscapeSeq(tokenLexeme, &tokenLexemeLength);
                    compilerStatus = tokenSetLexeme((*outToken), tokenLexeme);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        return COMPILER_INTERNAL_ERROR;
                    }
                    (*outToken)->type = TOKEN_STRING_LITERAL;
                    break;
                case ONE_LINE_STRING_NO_EMPTY_FINAL:
                    lexicalAnalyzer->state = START;
                    (*outToken)->type = TOKEN_EOF;
                    decodeEscapeSeq(tokenLexeme, &tokenLexemeLength);
                    compilerStatus = tokenSetLexeme((*outToken), tokenLexeme);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        return COMPILER_INTERNAL_ERROR;
                    }
                    (*outToken)->type = TOKEN_STRING_LITERAL;
                    break;
                case MULTIPLE_LINE_STRING_FINAL:
                    lexicalAnalyzer->state = START;
                    compilerStatus = tokenSetLexeme((*outToken), tokenLexeme);
                    if(compilerStatus == COMPILER_INTERNAL_ERROR)
                    {
                        tokenDestroy(*outToken);
                        *outToken = NULL;
                        return COMPILER_INTERNAL_ERROR;
                    }
                    (*outToken)->type = TOKEN_STRING_LITERAL;
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
            if(lexicalAnalyzer->state == START)
            {
                (*outToken)->type = TOKEN_EOF;
                return COMPILER_OK;
            }
            tokenDestroy(*outToken);
            *outToken = NULL;
            return COMPILER_INTERNAL_ERROR;
        }
    }

    return compilerStatus;
}
