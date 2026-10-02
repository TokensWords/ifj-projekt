# Lexical analyzer

The lexical analyzer reads IFJ26 source characters. It produces a syntax-independent token
stream. The finite-state automaton remains team-designed.

## Input

`lexicalAnalyzerCreate(FILE *input)` receives one borrowed stream. Production passes `stdin`.
The analyzer never closes that stream.

## Output

`lexicalAnalyzerNextToken()` writes one owned `Token *` into `outToken`.

| Status | `outToken` | Meaning |
|---|---|---|
| `COMPILER_OK` | Non-null | One valid token produced |
| `COMPILER_LEXICAL_ERROR` | Null | Invalid source spelling or indentation |
| `COMPILER_INTERNAL_ERROR` | Null | Allocation or invariant failure |

The syntax analyzer destroys each returned token with `tokenDestroy()`.

## `LexicalAnalyzer`

The public header keeps this structure opaque.

| Private field | Type | Meaning |
|---|---|---|
| `input` | `FILE *` | Borrowed source stream |
| `line` | `size_t` | Current one-based line |
| `column` | `size_t` | Current one-based column |
| `currentIndent` | `size_t` | Current block indentation |
| `reachedEof` | `bool` | Prevents reading beyond EOF |

The team may add character lookahead, parenthesis depth, indentation stacks, and pending-token
queues. Document every added field here.

## `Token`

| Field | Type | Meaning |
|---|---|---|
| `type` | `TokenType` | Token category |
| `lexeme` | `char *` | Owned textual spelling |
| `lexemeLength` | `size_t` | Bytes before the terminator |
| `location` | `SourceLocation` | First token character |
| `value` | `TokenValue` | Decoded scalar or indentation |

`TokenValue` contains `int64_t intValue`, `double doubleValue`, or `size_t indentLevel`.

## Responsibilities

- Recognize identifiers, reserved words, literals, punctuation, and operators.
- Decode permitted string escapes.
- Support nested block comments.
- Distinguish physical and logical lines.
- Track parentheses and indentation.
- Produce `NEWLINE`, `INDENT`, and `DEDENT` events.
- Reject leading tabs and malformed lexical units.

## Exclusions

- No grammar decisions.
- No symbol lookups.
- No type checking.
- No generated output.

## Team-owned artifacts

The assignment requires a manually designed finite-state automaton. Do not replace the marked
stub with AI-generated transition logic.
