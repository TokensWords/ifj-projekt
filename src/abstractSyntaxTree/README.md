# Abstract syntax tree

This block stores source-program meaning without concrete punctuation or indentation.

## Input

The syntax analyzer supplies node kinds, source locations, names, literals, and owned children.

## Output

Semantic analysis receives a mutable `SourceProgram *`. Intermediate-code generation later
receives the same program through a borrowed `const SourceProgram *`.

## `SourceProgram`

| Field | Type | Meaning |
|---|---|---|
| `root` | `AbstractSyntaxNode *` | Owned program root |

## `AbstractSyntaxNode`

| Field | Type | Meaning |
|---|---|---|
| `kind` | `AbstractSyntaxNodeKind` | Node meaning |
| `location` | `SourceLocation` | Diagnostic origin |
| `inferredType` | `DataType` | Semantic result |
| `text` | `char *` | Owned name, operator, or string |
| `value` | `AbstractSyntaxValue` | Scalar literal value |
| `children` | `AbstractSyntaxNode **` | Owned child array |
| `childCount` | `size_t` | Initialized children |
| `childCapacity` | `size_t` | Allocated slots |

`AbstractSyntaxValue` contains `int64_t intValue`, `double doubleValue`, or `bool boolValue`.

## Child conventions

| Node kind | `text` | Children |
|---|---|---|
| `ABSTRACT_SYNTAX_PROGRAM` | Null | Top-level functions |
| `ABSTRACT_SYNTAX_FUNCTION` | Function name | Parameters, then body |
| `ABSTRACT_SYNTAX_PARAMETER` | Parameter name | None |
| `ABSTRACT_SYNTAX_BLOCK` | Null | Ordered statements |
| `ABSTRACT_SYNTAX_VARIABLE_DECLARATION` | Variable name | Initializer |
| `ABSTRACT_SYNTAX_ASSIGNMENT` | Variable name | Assigned value |
| `ABSTRACT_SYNTAX_FUNCTION_CALL` | Function name | Ordered arguments |
| `ABSTRACT_SYNTAX_RETURN` | Null | Zero or one returned value |
| `ABSTRACT_SYNTAX_IF` | Null | Condition, then branch, else branch |
| `ABSTRACT_SYNTAX_WHILE` | Null | Condition, then body |
| `ABSTRACT_SYNTAX_BINARY_EXPRESSION` | Operator spelling | Left, then right operand |
| `ABSTRACT_SYNTAX_UNARY_EXPRESSION` | Operator spelling | Single operand |
| `ABSTRACT_SYNTAX_IDENTIFIER` | Identifier name | None |
| Literal kinds | String spelling when needed | None |

If future extensions require extra metadata, prefer a documented dedicated structure over
overloading unrelated fields.

## Ownership

- `sourceProgramCreate()` returns an owned program.
- `abstractSyntaxNodeCreate()` returns an owned node.
- `abstractSyntaxNodeAddChild()` transfers child ownership to the parent after success.
- `sourceProgramDestroy()` recursively releases everything.
- A child never belongs to multiple parents.

## Independent work boundary

This module owns representation and memory only. It never parses or validates language rules.

