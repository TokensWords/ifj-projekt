/*
 * Project: IFJ26 compiler
 * Authors: TODO(team)
 * AI-assisted scaffold.
 * Prompt: Define shared compiler statuses, types, and source locations.
 */

#ifndef COMPILER_TYPES_H
#define COMPILER_TYPES_H

#include <stddef.h>
#include <stdint.h>

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
    DATA_TYPE_UNKNOWN = 0,
    DATA_TYPE_INT = 1u << 0,
    DATA_TYPE_DOUBLE = 1u << 1,
    DATA_TYPE_STRING = 1u << 2,
    DATA_TYPE_UNIT = 1u << 3,
    DATA_TYPE_BOOLEAN = 1u << 4,
    DATA_TYPE_LIST = 1u << 5,
    DATA_TYPE_ALIAS = 1u << 6,
    DATA_TYPE_CASE_CLASS = 1u << 7
} DataType;

typedef struct {
    size_t line;
    size_t column;
} SourceLocation;

#endif
