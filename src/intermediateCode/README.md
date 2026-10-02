# Intermediate code

This block lowers a typed source program into owned three-address instructions.

## Input

`intermediateCodeGenerate()` receives a borrowed, semantically valid `const SourceProgram *`.

## Output

The caller allocates an `IntermediateProgram`. The generator appends owned instructions. Errors
return `COMPILER_INTERNAL_ERROR`; source-language errors should already be impossible.

## `IntermediateOperand`

| Field | Type | Meaning |
|---|---|---|
| `kind` | `IntermediateOperandKind` | Variable, label, literal, or type |
| `text` | `char *` | Owned variable, label, or string text |
| `value` | `IntermediateOperandValue` | Numeric, Boolean, or type value |

## `IntermediateInstruction`

| Field | Type | Meaning |
|---|---|---|
| `opcode` | `IntermediateOpcode` | Operation identity |
| `operands` | `IntermediateOperand[3]` | Ordered operands |
| `operandCount` | `size_t` | Valid operand count, zero through three |
| `origin` | `SourceLocation` | Original source position |

## `IntermediateProgram`

| Field | Type | Meaning |
|---|---|---|
| `instructions` | `IntermediateInstruction *` | Owned instruction array |
| `count` | `size_t` | Initialized instructions |
| `capacity` | `size_t` | Allocated instruction slots |

## Responsibilities

- Create deterministic temporary names and labels.
- Preserve left-to-right expression evaluation.
- Lower numeric conversions explicitly.
- Lower branches and loops into labels and jumps.
- Define function frames and calling conventions.
- Lower built-ins into suitable IFJcode26-level operations.
- Emit runtime exits where required.
- Preserve source locations for debugging.

## Ownership

`intermediateProgramAppend()` copies the instruction and every textual operand. The caller may
therefore release temporary input text after success.

## Exclusions

- No source syntax validation.
- No symbol lookup policy.
- No final text escaping.

