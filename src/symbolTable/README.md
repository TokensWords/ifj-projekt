# Symbol table

The symbol table is an abstract data type. Semantic analysis uses only `symtable.h`.

## Assignment constraint

Variant `vv-BVS` requires a manually implemented height-balanced binary search tree. The course
rules prohibit AI-generated search-structure code. Therefore, this scaffold defines the public
contract only. The team must choose, design, implement, and explain the internal tree.

## Input

- Scope-entry and scope-exit events.
- Borrowed `Symbol` values for insertion.
- Borrowed identifier strings for lookup.

## Output

- `CompilerStatus` for mutating operations.
- Borrowed `const Symbol *` for successful lookup.
- `NULL` when a symbol is absent.

## `FunctionSignature`

| Field | Type | Meaning |
|---|---|---|
| `parameterTypes` | `DataType *` | Owned copied parameter-type array |
| `parameterCount` | `size_t` | Parameter count |
| `returnType` | `DataType` | Declared or inferred result |

## `Symbol`

| Field | Type | Meaning |
|---|---|---|
| `name` | `const char *` | Identifier key supplied for copying |
| `kind` | `SymbolKind` | Variable, function, or type |
| `dataType` | `DataType` | Variable or scalar type |
| `mutable` | `bool` | Assignment permission |
| `defined` | `bool` | Completed-definition marker |
| `scopeDepth` | `size_t` | Lexical depth |
| `function` | `FunctionSignature` | Function-only type data |
| `variableId` | `size_t` | Unique id for every variable |

## `SymbolTable`

`SymbolTable` stays opaque. The team-owned definition must represent:

- the current lexical scope;
- links toward enclosing scopes;
- a balanced search-tree root per chosen design;
- complete ownership of copied symbol data;
- any balancing metadata required by the chosen algorithm.

Exact node fields and balancing metadata remain intentionally unspecified. Add their final names,
types, ownership, and invariants here before implementation review.

## Public operations

| Function | Meaning |
|---|---|
| `symbolTableCreate()` | Create the global table |
| `symbolTableDestroy()` | Release all scopes and symbols |
| `symbolTableEnterScope()` | Push a new lexical scope |
| `symbolTableLeaveScope()` | Destroy the current scope |
| `symbolTableInsert()` | Copy one symbol into current scope |
| `symbolTableFind()` | Search current scope outward |
| `symbolTableFindCurrentScope()` | Search only current scope |

## Ownership

Insertions copy the name and function parameter types. Lookup results stay valid until their
scope disappears or the complete table is destroyed.

