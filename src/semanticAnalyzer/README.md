# Semantic analyzer

Semantic analysis validates meaning and annotates the abstract syntax tree.

## Input

| Input | Type | Ownership |
|---|---|---|
| Parsed program | `SourceProgram *` | Borrowed, mutable |
| Scoped symbols | `SymbolTable *` | Borrowed, mutable |

## Output

The same program remains owned by the caller. Each AST node stores its type descriptor.
Errors return the exact required `CompilerStatus`.

## Function

```c
CompilerStatus semanticAnalyzerAnalyze(SourceProgram *program, SymbolTable *symbols);
```

## Responsibilities

- Register built-in functions.
- Collect user-function signatures before body analysis.
- Validate exactly one parameterless Unit entrypoint.
- Enter and leave lexical scopes.
- Enforce same-scope redefinition rules.
- Allow nested-scope shadowing.
- Resolve variables, functions, and types.
- Enforce `val`, `var`, and parameter mutability.
- Infer omitted variable and function types.
- Permit only implicit Int-to-Double widening.
- Validate function arguments and return behavior.
- Validate condition and expression operand types.
- Assign specific semantic exit codes.

## Pass boundary

A two-pass approach avoids source-order restrictions:

1. Collect all top-level function signatures.
2. Analyze each function body and nested scope.

This is an architectural recommendation, not generated grammar.

## Exclusions

- No tree balancing logic.
- No target instructions.
- No ownership transfer of the program.

