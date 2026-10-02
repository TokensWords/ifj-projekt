/*
 * Project: IFJ26 compiler
 * Authors: TODO(team)
 * AI-assisted scaffold.
 * Prompt: Define shared compiler statuses, types, and source locations.
 */

#ifndef COMPILER_TYPES_H
#define COMPILER_TYPES_H

#include <stddef.h>

typedef enum {
    COMPILER_OK = 0,
    COMPILER_LEXICAL_ERROR = 1,
    COMPILER_SYNTAX_ERROR = 2,
    COMPILER_UNDEFINED_SYMBOL_ERROR = 3,
    COMPILER_REDEFINITION_ERROR = 4,
    COMPILER_ARGUMENT_ERROR = 5,
    COMPILER_TYPE_ERROR = 6,
    COMPILER_IMMUTABLE_WRITE_ERROR = 7,
    COMPILER_SEMANTIC_ERROR = 10,
    COMPILER_INTERNAL_ERROR = 99
} CompilerStatus;

typedef enum {
    DATA_TYPE_UNKNOWN,
    DATA_TYPE_INT,
    DATA_TYPE_DOUBLE,
    DATA_TYPE_STRING,
    DATA_TYPE_UNIT,
    DATA_TYPE_BOOLEAN
} DataType;

typedef struct {
    size_t line;
    size_t column;
} SourceLocation;

#endif

