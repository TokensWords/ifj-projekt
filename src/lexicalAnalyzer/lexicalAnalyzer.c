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
  MULTIPLE_LINE_STRING_INSIDE_FINAL,
  ONE_LINE_STRING_NO_EMPTY_FINAL
};

struct LexicalAnalyzer {
    FILE *input;
    size_t line;
    size_t column;
    size_t currentIndent;
    enum State state;
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
    lexicalAnalyzer->reachedEof = false;
    return lexicalAnalyzer;
}

void lexicalAnalyzerDestroy(LexicalAnalyzer *lexicalAnalyzer)
{
    free(lexicalAnalyzer);
}

CompilerStatus lexicalAnalyzerNextToken(LexicalAnalyzer *lexicalAnalyzer, Token **outToken)
{
    if (lexicalAnalyzer == NULL || outToken == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    *outToken = NULL;

    /* Team TODO: implement the required finite-state scanner manually. */
    int currentChar;
    while((currentChar = fgetc(lexicalAnalyzer->input)) != EOF)
    {
        switch (lexicalAnalyzer->state) {
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
                *outToken = TOKEN_IDENTIFIER;
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
                *outToken = TOKEN_IDENTIFIER;
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
            else
            {
                lexicalAnalyzer->state = INVALID;
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
            //todo
            break;
        case DOUBLE:
            if(currentChar >= '0' && currentChar <= '9')
            {
                lexicalAnalyzer->state = DOUBLE_NO_EXP_FINAL;
            }
            else
            {
                lexicalAnalyzer->state = INVALID;
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
            }
            break;
        case ID_FINAL2:
            if((currentChar >= 'a' && currentChar <= 'z') || (currentChar >= 'A' && currentChar <= 'Z') || (currentChar >= '0' && currentChar <= '9') || currentChar == '_')
            {
                lexicalAnalyzer->state = ID_FINAL2;
            }
            //todo
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
            else
            {
                lexicalAnalyzer->state = INVALID;
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
                lexicalAnalyzer->state = INVALID;
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
            //todo
            break;
        case ONE_LINE_COMMENT:
            if((currentChar >= 0 && currentChar < '\n') || (currentChar > '\n' && currentChar <= 127))
            {
                lexicalAnalyzer->state = ONE_LINE_COMMENT;
            }
            else if(currentChar == 10)
            {
                lexicalAnalyzer->state = ONE_LINE_COMMENT_FINAL;
            }
            // maybe else
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
            }
            break;
        case ONE_LINE_STRING_NO_EMPTY_FINAL:
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
            }
            break;
        case ONE_LINE_COMMENT_FINAL:
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
            break;
        case ONE_LINE_STRING_FINAL:
            if(currentChar == '\"')
            {
                lexicalAnalyzer->state = MULTIPLE_LINE_STRING_INSIDE_FINAL;
            }
            else
            {
                lexicalAnalyzer->state = INVALID;
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
        case MULTIPLE_LINE_STRING_INSIDE_FINAL:
            if(currentChar != '\"')
            {
                lexicalAnalyzer->state = MULTIPLE_LINE_STRING_INSIDE;
            }
            else if(currentChar == '\"')
            {
                lexicalAnalyzer->state = MULTIPLE_LINE_STRING_ONE;
            }
            else
            {
                lexicalAnalyzer->state = INVALID;
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
                lexicalAnalyzer->state = INVALID;
            }
            break;
        case MULTIPLE_LINE_COMMENT_FINAL:
            lexicalAnalyzer->state = INVALID;
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
            //todo
            break;
        case EXPONENTIAL_FINAL_D:
            lexicalAnalyzer->state = INVALID;
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
            lexicalAnalyzer->state = INVALID;
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
    lexicalAnalyzer->reachedEof = true;
    return COMPILER_INTERNAL_ERROR;
}
