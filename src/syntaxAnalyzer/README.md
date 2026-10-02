# Syntax analyzer

The syntax analyzer consumes tokens and builds one owned `SourceProgram`.

## Input

`syntaxAnalyzerCreate()` receives one borrowed `LexicalAnalyzer *`. The lexical analyzer must
outlive the syntax analyzer.

## Output

`syntaxAnalyzerParseProgram()` writes an owned `SourceProgram *` into `outProgram`.

| Status | `outProgram` | Meaning |
|---|---|---|
| `COMPILER_OK` | Non-null | Complete syntax tree |
| `COMPILER_SYNTAX_ERROR` | Null | Invalid program structure |
| `COMPILER_LEXICAL_ERROR` | Null | Propagated lexical failure |
| `COMPILER_INTERNAL_ERROR` | Null | Allocation or invariant failure |

## `SyntaxAnalyzer`

| Private field | Type | Meaning |
|---|---|---|
| `lexicalAnalyzer` | `LexicalAnalyzer *` | Borrowed token source |
| `currentToken` | `Token *` | Owned current token |
| `nextToken` | `Token *` | Owned one-token lookahead |

Additional syntax-analyzer state remains private. Document new fields here.

## Responsibilities

- Require exactly one `import ifj26.*` prologue.
- Parse top-level function definitions and `@main`.
- Parse indentation-delimited non-empty blocks.
- Parse declarations, assignments, calls, returns, conditions, and loops.
- Use recursive descent for the LL portion.
- Use precedence analysis for every expression.
- Preserve `SourceLocation` on created nodes.
- Release partial trees after failure.

## Abstract-syntax output

The syntax analyzer sets structural fields only. It leaves every `inferredType` as
`DATA_TYPE_UNKNOWN`. Name resolution and type decisions belong to semantic analysis.

## Exclusions

- No undefined-name errors.
- No assignment mutability checks.
- No type inference.
- No target-code generation.

## Team-owned artifacts

The grammar, LL table, precedence table, and derived parsing logic remain team-created. The
scaffold intentionally contains no generated productions or tables.
