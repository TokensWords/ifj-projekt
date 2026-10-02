# Optimizer

The optimizer receives mutable intermediate code and preserves observable behavior.

## Input and output

```c
CompilerStatus optimizerRun(IntermediateProgram *program);
```

The pointer remains caller-owned. Instructions may change in place. The current implementation
is a valid no-op returning `COMPILER_OK`.

## Possible passes

- Constant propagation.
- Copy propagation.
- Dead-code removal.
- Redundant temporary removal.

## Required invariants

- Preserve left-to-right evaluation.
- Preserve side effects and runtime failures.
- Preserve function-call order.
- Preserve exact printed output.
- Keep every operand owned and releasable.

Each optimization requires focused before-and-after tests.

