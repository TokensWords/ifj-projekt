# IFJ26 compiler

Development scaffold for an IFJ26-to-IFJcode26 compiler.

This repository currently defines module boundaries, shared contracts, memory ownership,
portable build rules, and test entrypoints. It deliberately does not implement the scanner
automaton, grammar, precedence table, syntax analysis, semantic rules, balanced tree, or final
code generation. Those artifacts must be designed and understood by the team.

The repository targets assignment variant `vv-BVS`. Its symbol table must therefore use a
height-balanced binary search tree implemented manually by the team.

## Required behavior

The final executable acts as a Unix filter:

```text
stdin:  IFJ26 source
stdout: IFJcode26 target program
stderr: diagnostics only
exit:   0, 1, 2, 3, 4, 5, 6, 7, 10, or 99
```

The compiler must never print diagnostics into generated output. The final IFJcode26 output
starts with `.IFJcode26`. The production compiler must not open network connections, spawn
processes, modify files, or depend on third-party libraries.

## Architecture

```text
stdin
  |
  v
Lexical analyzer --Token--> Syntax analyzer --SourceProgram--> Semantic analyzer
                                                                      |
                                                                      | typed SourceProgram
                                                                      v
                                                                Intermediate-code generator
                                                                      |
                                                                      | IntermediateProgram
                                                                      v
                                                                 Optimizer
                                                                      |
                                                                      | IntermediateProgram
                                                                      v
                                                               IFJcode26 generator
                                                                      |
                                                                      v
                                                                    stdout

Semantic analyzer <--> Symbol table
All blocks ---------> Common types and allocation helpers
```

The optimizer is optional. Its initial valid implementation may be a semantics-preserving
no-op. Every other block participates in one integrated executable.

## Repository layout

| Directory | Responsibility | Primary input | Primary output |
|---|---|---|---|
| `src/shared` | Shared statuses, types, locations, allocation | Primitive C values | Shared contracts |
| `src/lexicalAnalyzer` | Finite-state lexical analysis and indentation | `FILE *` source stream | Owned `Token *` values |
| `src/syntaxAnalyzer` | Recursive descent and precedence parsing | Borrowed `LexicalAnalyzer *` | Owned `SourceProgram *` |
| `src/abstractSyntaxTree` | Abstract syntax tree representation | Parsed node data | Owned abstract-syntax nodes |
| `src/semanticAnalyzer` | Names, scopes, types, returns, calls | Mutable `Program *` | Typed tree or error |
| `src/symbolTable` | Scoped symbols using vv-BVS | `Symbol` records | Borrowed symbol lookups |
| `src/intermediateCode` | Three-address representation and lowering | Typed `Program *` | Owned `IntermediateProgram *` |
| `src/optimizer` | Optional intermediate-code transformations | Mutable `IntermediateProgram *` | Equivalent `IntermediateProgram *` |
| `src/generator` | IFJcode26 text emission | Borrowed `IntermediateProgram *` | Text written into `FILE *` |
| `tests` | AI-authored verification | Modules or executable | Pass/fail process status |

Every directory contains its own `README.md`. Those files define detailed boundaries,
ownership, structures, and independent work expectations.

## Entrypoint rule

Only `src/main.c` contains `main()`.

Module directories never contain another `main.c`. Unit-test files may contain isolated test
entrypoints because they compile into separate test executables. Public declarations belong in
headers. Function bodies belong in matching `.c` files. No ad-hoc function declarations belong
inside `main.c`.

## Shared structures

### `SourceLocation`

```c
typedef struct {
    size_t line;
    size_t column;
} SourceLocation;
```

One-based source location. The lexical analyzer creates it. Every later stage preserves it.

### `CompilerStatus`

| Name | Value | Meaning |
|---|---:|---|
| `COMPILER_OK` | 0 | Successful translation |
| `COMPILER_LEXICAL_ERROR` | 1 | Invalid token, comment, escape, or indentation |
| `COMPILER_SYNTAX_ERROR` | 2 | Invalid syntax, prologue, or block structure |
| `COMPILER_UNDEFINED_SYMBOL_ERROR` | 3 | Missing variable, function, type, or entrypoint |
| `COMPILER_REDEFINITION_ERROR` | 4 | Forbidden duplicate definition |
| `COMPILER_ARGUMENT_ERROR` | 5 | Wrong call arguments or main parameters |
| `COMPILER_TYPE_ERROR` | 6 | Type mismatch or failed inference |
| `COMPILER_IMMUTABLE_WRITE_ERROR` | 7 | Write into `val` or formal parameter |
| `COMPILER_SEMANTIC_ERROR` | 10 | Other semantic failure |
| `COMPILER_INTERNAL_ERROR` | 99 | Allocation or compiler-internal failure |

### `Token`

```c
typedef struct {
    TokenType type;
    char *lexeme;
    size_t lexemeLength;
    SourceLocation location;
    TokenValue value;
} Token;
```

- `type`: syntax-analyzer-facing token category.
- `lexeme`: owned textual spelling, when needed.
- `lexemeLength`: byte length excluding the terminator.
- `location`: first source character.
- `value`: decoded integer, double, or indentation level.

The caller owns every token returned by `lexicalAnalyzerNextToken()` and releases it through
`tokenDestroy()`.

### `SourceProgram` and `AbstractSyntaxNode`

```c
typedef struct AbstractSyntaxNode {
    AbstractSyntaxNodeKind kind;
    SourceLocation location;
    DataType inferredType;
    char *text;
    AbstractSyntaxValue value;
    struct AbstractSyntaxNode **children;
    size_t childCount;
    size_t childCapacity;
} AbstractSyntaxNode;

typedef struct {
    AbstractSyntaxNode *root;
} SourceProgram;
```

- `kind`: node meaning.
- `location`: diagnostic origin.
- `inferredType`: semantic result.
- `text`: owned name, operator, or string spelling.
- `value`: decoded scalar literal.
- `children`: owned child-node array.
- `childCount`: initialized child count.
- `childCapacity`: allocated child slots.
- `root`: owned `ABSTRACT_SYNTAX_PROGRAM` node.

`sourceProgramDestroy()` recursively releases the complete tree.

### `Symbol`

```c
typedef struct {
    const char *name;
    SymbolKind kind;
    DataType dataType;
    bool mutable;
    bool defined;
    size_t scopeDepth;
    FunctionSignature function;
} Symbol;
```

- `name`: identifier key.
- `kind`: variable, function, or type.
- `dataType`: scalar variable or type information.
- `mutable`: permits assignment.
- `defined`: distinguishes declared and completed functions.
- `scopeDepth`: lexical-scope level.
- `function`: parameter and return types.

The symbol table copies inserted names and parameter arrays. Lookup returns borrowed, immutable
records valid until scope removal or table destruction.

### `IntermediateProgram`

```c
typedef struct {
    IntermediateInstruction *instructions;
    size_t count;
    size_t capacity;
} IntermediateProgram;
```

Each `IntermediateInstruction` contains one `IntermediateOpcode`, up to three `IntermediateOperand` records, their count, and
one `SourceLocation`. Textual operands own their `text` fields. `intermediateProgramAppend()` copies its
instruction and all textual operands.

## Block contracts

### Lexical analyzer

- Input: borrowed `FILE *input`, normally `stdin`.
- Output: one owned `Token *` per request.
- Success: `COMPILER_OK` and non-null token.
- Language error: `COMPILER_LEXICAL_ERROR`.
- Allocation failure: `COMPILER_INTERNAL_ERROR`.
- Responsibilities: literals, keywords, comments, physical lines, logical lines, parentheses,
  indentation, `INDENT`, `DEDENT`, and `NEWLINE` events.
- Exclusion: no syntax or type decisions.

### Syntax analyzer

- Input: borrowed `LexicalAnalyzer *` producing tokens.
- Output: owned `SourceProgram *` containing the abstract syntax tree.
- Success: `COMPILER_OK` and non-null program.
- Syntax failure: `COMPILER_SYNTAX_ERROR`.
- Responsibilities: prologue, functions, blocks, statements, recursive descent, and precedence
  expression parsing.
- Exclusion: no symbol existence or type decisions.

### Semantic analyzer

- Input: mutable `SourceProgram *`, mutable `SymbolTable *`.
- Output: the same tree with `inferredType` annotations.
- Responsibilities: scopes, declarations, calls, mutability, type inference, numeric widening,
  return rules, `@main`, and exact semantic exit codes.
- Exclusion: no generated target text.

### Symbol table

- Input: `Symbol` insertions and identifier lookups.
- Output: borrowed `const Symbol *` results.
- Required representation: manually implemented height-balanced binary search tree.
- Responsibilities: lexical scopes, shadowing, same-scope duplicates, functions, variables,
  built-ins, and cleanup.
- Exclusion: no syntax-analyzer or semantic policy.

### Intermediate-code generator

- Input: borrowed, semantically valid `const SourceProgram *`.
- Output: caller-owned `IntermediateProgram *`.
- Responsibilities: temporaries, labels, frames, calls, conversions, control flow, built-ins,
  expression evaluation order, and runtime-error exits.
- Exclusion: no source-language validation.

### Optimizer

- Input: mutable `IntermediateProgram *`.
- Output: same program, behavior preserved.
- Initial implementation: return `COMPILER_OK` unchanged.
- Later candidates: constant propagation, copy propagation, dead-code removal.
- Exclusion: never alter observable evaluation order.

### Code generator

- Input: borrowed `const IntermediateProgram *` and borrowed `FILE *output`.
- Output: IFJcode26 lines written exclusively to `output`.
- Responsibilities: `.IFJcode26`, operand escaping, frame names, labels, opcodes, and newlines.
- Exclusion: no diagnostics on `stdout`.

## Memory ownership

Allocation uses pointer-returning constructors:

```c
Structure *array = malloc(sizeof(Structure) * count);
if (array == NULL) {
    return NULL;
}
```

Project helpers add overflow checking:

```c
Structure *array = memoryAllocateArray(count, sizeof(Structure));
if (array == NULL) {
    return NULL;
}
```

Rules:

1. Every `Create` function returns owned memory.
2. Every owned type has one matching `Destroy` function.
3. Borrowed pointers never outlive their owner.
4. Output parameters become owned only after success.
5. Failed constructors release partial allocations.
6. Abstract-syntax parents own child nodes.
7. Intermediate programs own instructions and operand text.
8. Symbol tables own inserted symbol copies.

## Naming and formatting

- Types: simple PascalCase names, such as `Token`.
- Functions: camelCase, such as `lexicalAnalyzerNextToken()`.
- Variables: camelCase, such as `currentToken`.
- Constants and enum values: uppercase snake case.
- Function definitions: `dataType functionName(...)`.
- Control statements: `while (condition)`.
- One public header per module.
- Shared formatting: repository `.clang-format`.
- No `TStructure`-style prefixes.

Replace every `TODO(team)` author marker immediately. Each file must list only its actual
student contributors, including names and FIT logins.

## Building

Requirements: GNU Make and a C17 compiler.

```sh
make
```

The default target creates exactly one executable:

```text
./ifj26compiler
```

Compiler selection remains overridable:

```sh
make CC=gcc
make CC=clang
```

Additional targets:

```sh
make test
make check
make build-check
make clean
```

`check` skips gracefully when `clang-format` is unavailable. The normal `build` target requires
only the standard compiler toolchain.

## Testing strategy

Tests are AI-authored, human-reviewed, and versioned. Their comments disclose AI generation.

- Unit tests validate owned structures and bounded module behavior.
- Integration fixtures feed IFJ26 through `stdin`.
- Expected exit codes validate lexical, syntactic, and semantic failures.
- Golden files compare IFJcode26 output byte-for-byte.
- Interpreter tests eventually compare runtime output byte-for-byte.
- Every regression receives the smallest reproducing fixture.

## Team workflow

Suggested ownership:

1. Lexical analyzer and token stream.
2. Syntax analyzer and abstract-syntax tree construction.
3. Semantic analysis and symbol-table integration.
4. Intermediate code, optimizer, and IFJcode26 generation.

The vv-BVS implementation remains shared knowledge despite individual ownership. Use focused
branches, small commits, required reviews, and continuously buildable pull requests.

## Development layout versus submission

The nested source layout supports independent team ownership. The official submission archive
requires flat source placement at its root or directly inside `src/`. Before submission, create a
separate packaging step that flattens uniquely named `.c` and `.h` files, preserves includes,
adds `Makefile`, `documentation.pdf`, and `rozdeleni`, then runs the official checker.

Do not submit `build/`, `.git/`, binaries, macOS metadata, or nested development directories.

## AI-use boundary

This scaffold and its tests are AI-assisted and marked accordingly. Under the assignment rules,
the team must manually create and understand:

- the lexical automaton;
- the LL grammar and LL table;
- the precedence table;
- the syntax-analyzer logic derived from those formalisms;
- the height-balanced search-tree implementation;
- the final submitted project documentation.

The READMEs are engineering notes, not final submission documentation.
