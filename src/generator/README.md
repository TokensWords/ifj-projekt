# Generator

The generator converts validated intermediate instructions into IFJcode26 text.

## Input

| Input | Type | Ownership |
|---|---|---|
| Intermediate program | `const IntermediateProgram *` | Borrowed |
| Destination stream | `FILE *` | Borrowed |

Production passes `stdout`. The generator never closes the stream.

## Output

`generatorWrite()` writes one instruction per line. Successful output begins with
`.IFJcode26`. It returns `COMPILER_OK` only after complete emission.

## Responsibilities

- Emit the mandatory header.
- Map every `IntermediateOpcode` deterministically.
- Format integer and floating constants correctly.
- Escape string characters using three-digit decimal escapes.
- Emit frame-qualified variable names.
- Emit stable, collision-free labels.
- End every instruction with one newline.

## Error channel

Generated code goes only to the provided output stream. Diagnostics belong exclusively on
`stderr` and should be produced by the orchestrating layer.

## Exclusions

- No source-language validation.
- No semantic decisions.
- No ownership transfer.
