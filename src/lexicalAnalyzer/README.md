# Lexical analyzer

<!-- AI-assisted README. Prompt: Document the current lexer, token contract and integer
stack, including empty-stack behavior and extension boundaries; change only this README. -->

This module reads IFJ26 source characters with a finite-state scanner and produces tokens
for the syntax analyzer. It also handles comments, logical line endings and indentation.

## Files

| Files | Purpose |
|---|---|
| `lexicalAnalyzer.c`, `lexicalAnalyzer.h` | Scanner implementation and opaque public interface |
| `token.c`, `token.h` | Token categories, values, allocation and lexeme ownership |
| `stack.c`, `stack.h` | Opaque linked-list stack of integers used for indentation levels |

## Public interface and ownership

- `lexicalAnalyzerCreate(FILE *input)` creates an analyzer or returns `NULL` on an invalid
  input pointer or allocation failure. The input stream is borrowed and is never closed by
  the analyzer. The compiler normally supplies `stdin`.
- `lexicalAnalyzerNextToken(LexicalAnalyzer *lexicalAnalyzer, Token **outToken)` returns a
  `CompilerStatus` and transfers one token to the caller on success, including layout and EOF
  tokens. The caller must destroy previously returned tokens; the function does not do that.
- `lexicalAnalyzerDestroy()` frees the analyzer, its indentation stack and any token still
  waiting inside it. Passing `NULL` is allowed. Tokens already returned belong to the caller.

| Status | Meaning |
|---|---|
| `COMPILER_OK` | A non-null token is returned through `outToken` |
| `COMPILER_LEXICAL_ERROR` | Invalid lexical input or indentation |
| `COMPILER_INTERNAL_ERROR` | Invalid API arguments, allocation failure or input read failure |

On failure, stop scanning that input and destroy the analyzer. Successful EOF calls may be
repeated and return new EOF tokens without reading the stream again. Use `tokenDestroy()`
to release each returned token and its lexeme.

## Token contract

| Field | Type | Meaning |
|---|---|---|
| `type` | `TokenType` | Token category |
| `lexeme` | `char *` | Owned, null-terminated text; literals may be normalized or decoded |
| `lexemeLength` | `size_t` | Number of bytes in `lexeme`, excluding the terminator |
| `location` | `SourceLocation` | One-based source line and column for source tokens |
| `value` | `TokenValue` | Numeric payload for integer and double literals |

`TokenValue` contains `int intValue` and `double doubleValue` as union alternatives. Read
only the member appropriate for the token category. Boolean literals use the lexemes `true`
and `false`; there is no separate Boolean value field. Layout tokens carry no indentation
value. Synthetic `INDENT`/`DEDENT` tokens retain the default location from `tokenCreate()`;
their location is not an indentation level or a source-span guarantee. Layout and EOF tokens
may have a `NULL` lexeme, so consumers must inspect the category before using the text.

Literal text is not always the original source spelling:

- Numeric separators are removed: `1_2` produces lexeme `12` and integer value `12`.
  Double suffixes `d`/`D` are also removed, for example `1_2.5D` produces lexeme `12.5`.
- String delimiters are removed. Ordinary strings decode `\\`, `\"`, `\n`, `\r` and `\t`.
  Triple-quoted strings preserve their contents, including physical newlines and backslashes.
- The current implementation rejects actual NUL bytes in string content. The escape `\0`
  is not supported. Token text uses C strings and cannot represent embedded NUL bytes.

## Recognized words, operators and punctuation

Identifiers are checked against this case-sensitive keyword table:

```text
def do Double else if import Int return String then Unit val var while
Boolean case class extension for List match type
```

Keywords share `TOKEN_KEYWORD` and are distinguished by their lexeme. Exact `true` and
`false` produce `TOKEN_BOOL_LITERAL`, with the truth value stored in `TokenValue.boolValue`.
A single `_` produces `TOKEN_UNDERSCORE`; longer identifiers such as `__`, `_1` and
`trueValue` remain identifiers.

Operators share `TOKEN_OPERATOR` and retain their spelling:

```text
+ - * / = == != < <= > >= ! && || | <- => ++
```

The scanner prefers a longer recognized operator when possible. A single `&` is invalid.
Minus is a separate operator, rather than part of a numeric literal.

Round brackets have `TOKEN_LEFT_PAREN`/`TOKEN_RIGHT_PAREN`; square brackets have
`TOKEN_LEFT_SHARP_PAREN`/`TOKEN_RIGHT_SHARP_PAREN`. Other punctuation has its own category:
comma, colon, semicolon, dot and `@`. Empty round brackets are returned as two tokens;
recognizing the Unit expression `()` belongs to the parser. `TOKEN_UNIT_LITERAL` is declared
in the shared enum but is not emitted by this scanner.

Built-in function and method names, including `readBoolean`, `readIntMaybe` and
`readDoubleMaybe`, remain identifiers. The contextual words `to`, `until` and `by` also
remain identifiers.

## Logical lines, comments and indentation

- Outside literals and comments, space, tab, vertical tab, form feed and carriage return
  are whitespace. Only spaces contribute to indentation. LF is the physical line ending;
  CRLF therefore produces one line ending.
- A tab in the leading whitespace of a line containing code is rejected. Blank and
  comment-only lines do not create indentation changes or standalone logical newlines.
- Outside brackets, a completed nonempty logical line produces `TOKEN_NEWLINE`. Its lexeme
  is normalized to `\n`. A final nonempty logical line is also terminated at EOF.
- Both `()` and `[]` contribute to one shared bracket-depth counter. Physical newlines
  inside them do not produce `TOKEN_NEWLINE`. Continuation lines containing tokens must be
  indented more than the first physical line of the logical line. Nested openings do not
  replace that original indentation baseline. Newlines inside triple-quoted string content
  are part of the string and do not trigger this continuation check.
- `//` comments end at LF or EOF. Block comments may nest. A block comment without LF acts
  as a separator. A block comment containing LF ends the current physical line; after its
  closing delimiter, only whitespace followed by LF or EOF is accepted.
- The indentation stack starts with `0`. Each increase pushes one level and produces one
  `TOKEN_INDENT`. A decrease must reach a previously stored level and produces one
  `TOKEN_DEDENT` for each removed level. Levels need not be multiples of four.
- For levels `0 -> 2 -> 6 -> 0`, the layout events are `INDENT`, `INDENT`, `DEDENT`,
  `DEDENT`. Pending layout tokens precede the first source token of the new logical line.
  At EOF, remaining levels are closed before `TOKEN_EOF` is returned.

The depth counter controls line handling; the parser is responsible for validating bracket
pairing and the grammatical use of these tokens.

## Private analyzer state

`LexicalAnalyzer` is opaque in the public header. Its fields are defined in
`lexicalAnalyzer.c`:

| Private field | Type | Meaning |
|---|---|---|
| `input` | `FILE *` | Borrowed input stream |
| `location` | `SourceLocation` | Current physical reading position |
| `prevToken` | `Token *` | Recognized token delayed until pending layout events are returned |
| `prevChar` | `int` | One character saved for the next token |
| `prevCharRead` | `bool` | Whether a saved character must be processed |
| `prevCharLocation` | `SourceLocation` | Source position of that saved character |
| `prevIndent` | `int` | Last accepted indentation level |
| `indentStack` | `Stack *` | Active indentation levels, including the base level `0` |
| `indentCount` | `int` | Pending layout events: positive for INDENT, negative for DEDENT |
| `currentIndent` | `int` | Number of indentation spaces on the current physical line |
| `parenIndent` | `int` | Indentation baseline of the first physical line of the logical line |
| `parenDepth` | `int` | Combined count of open round and square brackets |
| `newlineEnded` | `bool` | Physical line-start flag used when processing leading whitespace |
| `logicalIndentKnown` | `bool` | Whether the current logical line's baseline has been recorded |
| `continuationPending` | `bool` | Whether the next real token needs a continuation-indent check |
| `slashIndent` | `int` | Saved indentation when a slash may begin a comment or operator |
| `slashWasPending` | `bool` | Saved continuation-check flag for that slash |
| `state` | `enum State` | Current scanner state |
| `reachedEof` | `bool` | Whether EOF has already been read |
| `lineHasToken` | `bool` | Whether the current logical line contains a source token |
| `newlineLocation` | `SourceLocation` | Saved source position for the logical newline token |

## Integer stack

`Stack` is an opaque ADT. Its linked nodes and top pointer are private to `stack.c`.
The stack stores integers by value and owns its nodes.

| Operation | Behavior |
|---|---|
| `stackCreate()` | Creates an empty stack; returns `NULL` if allocation fails |
| `stackPush(stack, value)` | Adds a value; returns `false` on `NULL` or allocation failure |
| `stackPop(stack)` | Removes and frees the top node; an empty stack or `NULL` is a no-op |
| `stackTop(stack)` | Returns the top value without removing it; returns `0` on empty or `NULL` |
| `stackIsEmpty(stack)` | Returns `true` for an empty stack or `NULL` |
| `stackDestroy(stack)` | Frees all nodes and the stack; `NULL` is a no-op |

The stack does not print errors or terminate the process. A `0` from `stackTop()` does not
distinguish an empty stack from a stored zero; callers needing that distinction must use
`stackIsEmpty()`. The lexer explicitly pushes its base indentation `0` after creating the
stack and keeps that base level during normal indentation processing. After destroying a
stack, the caller must not use its old pointer.

## Extension boundaries

The token vocabulary supports the lexical requirements of FUNEXP, BOOLTHEN, CONTROL,
ALGEBDT and LISTS. This does not imply that the parser, semantic analyzer or generated code
implements those extensions. This module does not resolve symbols, assign data types,
validate function arguments or generate IFJcode26.
