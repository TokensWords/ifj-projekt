# Shared contracts

This directory contains definitions used by every compiler block. It contains no language
policy and never depends on higher-level modules.

## Inputs and outputs

| Input | Output | Meaning |
|---|---|---|
| Allocation sizes | Owned pointer or `NULL` | Safe dynamic storage |
| Compiler condition | `CompilerStatus` | Process exit classification |
| Type category | `DataType` | Bit mask shared by compiler modules |
| Lexical-analyzer position | `SourceLocation` | Diagnostic origin |

## Structures

### `SourceLocation`

| Field | Type | Meaning |
|---|---|---|
| `line` | `size_t` | One-based physical source line |
| `column` | `size_t` | One-based source column |

### `DataType`

`DataType` values are bit flags. `DATA_TYPE_UNKNOWN` is the empty mask used before type
information is available. Recursive descriptors are an AST-layer abstraction managed by
`abstractSyntaxTree/typeDescriptor.h`.

### `CompilerStatus`

Values exactly match required compiler exit codes. Modules return a specific language error when
possible. Allocation failures always return `COMPILER_INTERNAL_ERROR`.

## Functions

| Function | Input | Output | Ownership |
|---|---|---|---|
| `memoryAllocate()` | Byte count | Pointer or `NULL` | Caller owns result |
| `memoryAllocateArray()` | Element count and size | Pointer or `NULL` | Caller owns result |
| `memoryResizeArray()` | Existing pointer and new dimensions | Pointer or `NULL` | Caller owns result |
| `memoryDuplicateString()` | Borrowed C string | Copied C string | Caller owns result |

`memoryResizeArray()` follows normal `realloc()` behavior. Callers must preserve the old pointer
until a non-null result arrives.

## Independent work boundary

Changes here affect every block. Require team review and a full `make test` run.
