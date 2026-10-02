# Integration tests

These integration assets were written by an AI agent. Team members must review every fixture and expectation before merging changes.

The tests treat the compiler as a command-line program:

```text
fixture.ifj -> standard input -> ifj26compiler -> standard output, standard error, exit status
```

The runner never implements, reproduces, or validates the language grammar. It only supplies source text and checks externally visible behavior.

## Running tests

Build the compiler first, then run the active suite:

```sh
make build
tests/runIntegrationTests.sh
```

Override the executable when required:

```sh
IFJ_COMPILER=/path/to/ifj26compiler tests/runIntegrationTests.sh
```

The script is portable POSIX shell. It requires only standard command-line utilities.

## Fixture contract

Every test starts with one `name.ifj` source file. Companion files use the same `name` prefix:

| File | Required | Meaning |
| --- | --- | --- |
| `name.expected.status` | Yes | Decimal compiler exit status |
| `name.expected.stdout` | No | Exact expected standard output |
| `name.expected.stderr` | No | Exact expected standard error |
| `name.assertEmptyStdout` | No | Require empty standard output |
| `name.assertEmptyStderr` | No | Require empty standard error |
| `name.pending` | No | Skip unfinished future behavior |

Do not combine `expected.stdout` with `assertEmptyStdout`. Apply the same rule to standard error.

Expected output files are byte-for-byte golden files. Missing output expectations intentionally leave that stream unchecked. Exit status always remains checked.

## Suites

`integration/currentScaffold` records the executable scaffold behavior. These tests currently expect internal-error status `99`. Replace or remove them once the real pipeline exists.

`integration/future/valid` demonstrates successful-source conventions. `integration/future/errors` demonstrates lexical, syntactic, and semantic failure conventions. Future cases carry `pending` markers, preventing false failures before their compiler stages exist.

Run any suite explicitly:

```sh
tests/runIntegrationTests.sh currentScaffold
tests/runIntegrationTests.sh future
tests/runIntegrationTests.sh all
```

Removing one `pending` marker activates that case. Add exact generated IFJcode26 output as `expected.stdout` when generator output stabilizes.

## Adding tests

Use descriptive camelCase names. Keep each fixture minimal. Prefer one observable failure per fixture. Record the specification reason during human review. Never encode grammar tables, automata, parser logic, or symbol-table algorithms inside the runner.

All new tests and directory READMEs should remain AI-authored, then human-reviewed. Production logic remains governed by the assignment's AI restrictions.
