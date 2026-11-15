#!/usr/bin/env python3
"""AI-generated regression runner; see README.md for prompt and test boundaries.

The runner invokes the real compiler and the official interpreter. It contains
no implementation of IFJ26 syntax, semantics, or symbol-table algorithms.
"""

from __future__ import annotations

import argparse
import collections
import difflib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time


SUITE = Path(__file__).resolve().parent
ROOT = SUITE.parent.parent
DEFAULT_INTERPRETER = ROOT.parent / "ic26int-windows-x86_64.exe"


def executable(value: str | Path) -> str:
    value = str(value)
    path = Path(value).expanduser()
    if path.is_file():
        return str(path.resolve())
    found = shutil.which(value)
    if found:
        return found
    raise ValueError(f"Executable not found: {value}")


def inside_suite(path: Path) -> Path:
    resolved = path.resolve()
    if not resolved.is_relative_to(SUITE):
        raise ValueError(f"Fixture path escapes the suite: {path}")
    return resolved


def load_cases() -> list[dict]:
    document = json.loads((SUITE / "manifest.json").read_text(encoding="utf-8"))
    cases = document["cases"]
    seen: set[str] = set()
    for case in cases:
        case_id = case["id"]
        if case_id in seen or any(c not in "abcdefghijklmnopqrstuvwxyz0123456789_-" for c in case_id):
            raise ValueError(f"Invalid or duplicate case id: {case_id}")
        seen.add(case_id)
        for key in ("source", "stdin", "stdout", "compiler_status_file"):
            if not inside_suite(SUITE / case[key]).is_file():
                raise ValueError(f"Missing {key} for {case_id}")
        expected_compiler = int((SUITE / case["compiler_status_file"]).read_text().strip())
        if expected_compiler != case["compiler_status"]:
            raise ValueError(f"Compiler status mismatch in manifest for {case_id}")
        if expected_compiler not in (0, 1, 2, 3, 4, 5, 6, 7, 10):
            raise ValueError(f"Unexpected language-test compiler status: {case_id}")
        if expected_compiler == 0:
            status_file = inside_suite(SUITE / case["runtime_status_file"])
            if int(status_file.read_text().strip()) != case["runtime_status"]:
                raise ValueError(f"Runtime status mismatch for {case_id}")
        elif case["runtime_status"] is not None:
            raise ValueError(f"A rejected program cannot have a runtime status: {case_id}")
    return cases


def build_compiler(cc: str, target: Path, timeout: float) -> str:
    includes = [ROOT / "src"] + sorted(p for p in (ROOT / "src").iterdir() if p.is_dir())
    sources = sorted((ROOT / "src").glob("*.c")) + sorted((ROOT / "src").glob("*/*.c"))
    if not sources:
        raise ValueError("No compiler source files found")
    command = [executable(cc), "-std=c17", "-O2", "-Wall", "-Wextra", "-Wpedantic"]
    command += [f"-I{p}" for p in includes]
    command += [str(p) for p in sources] + ["-o", str(target)]
    target.parent.mkdir(parents=True, exist_ok=True)
    completed = subprocess.run(command, cwd=ROOT, capture_output=True, timeout=timeout)
    (target.parent / "build.stdout").write_bytes(completed.stdout)
    (target.parent / "build.stderr").write_bytes(completed.stderr)
    if completed.returncode:
        sys.stderr.buffer.write(completed.stderr)
        raise ValueError(f"Compiler build failed with status {completed.returncode}; see {target.parent}")
    print(f"Built: {target}", flush=True)
    return str(target.resolve())


def invoke(command: list[str], input_bytes: bytes, stem: Path, timeout: float) -> tuple[int | None, bytes]:
    try:
        completed = subprocess.run(command, input=input_bytes, capture_output=True,
                                   timeout=timeout, cwd=ROOT)
        status, stdout, stderr = completed.returncode, completed.stdout, completed.stderr
    except subprocess.TimeoutExpired as error:
        status, stdout, stderr = None, error.stdout or b"", error.stderr or b""
    stem.with_suffix(".stdout").write_bytes(stdout)
    stem.with_suffix(".stderr").write_bytes(stderr)
    stem.with_suffix(".status").write_text("TIMEOUT\n" if status is None else f"{status}\n", encoding="ascii")
    return status, stdout


def difference(expected: bytes, actual: bytes) -> str:
    if expected == actual:
        return ""
    left = expected.decode("ascii", errors="backslashreplace").splitlines(keepends=True)
    right = actual.decode("ascii", errors="backslashreplace").splitlines(keepends=True)
    diff = "".join(difflib.unified_diff(left, right, fromfile="expected", tofile="actual"))
    return (diff + f"\nexpected bytes: {expected!r}\nactual bytes:   {actual!r}\n")


def compared_stdout(output: bytes, exact_bytes: bool) -> bytes:
    return output if exact_bytes or os.name != "nt" else output.replace(b"\r\n", b"\n")


def run_case(case: dict, compiler: str, interpreter: str | None, args, results: Path) -> dict:
    case_dir = results / case["id"]
    case_dir.mkdir(parents=True, exist_ok=True)
    started = time.monotonic()
    record = {"id": case["id"], "group": case["group"], "description": case["description"]}
    status, generated = invoke([compiler], (SUITE / case["source"]).read_bytes(),
                               case_dir / "compiler", args.compiler_timeout)
    record["actual_compiler_status"] = status
    record["expected_compiler_status"] = case["compiler_status"]
    if status != case["compiler_status"]:
        record.update(result="FAIL", reason=f"compiler status {status}; expected {case['compiler_status']}")
    elif status != 0:
        # A failed compilation must not be interpreted; partial generated output
        # and the exact diagnostic wording are deliberately not an oracle.
        record.update(result="PASS", reason="expected compiler rejection")
    else:
        code = case_dir / "program.ifjcode26"
        code.write_bytes(generated)
        if args.compile_only:
            record.update(result="COMPILE_OK", reason="runtime intentionally not checked")
        else:
            runtime_status, output = invoke([interpreter, str(code)],
                                            (SUITE / case["stdin"]).read_bytes(),
                                            case_dir / "runtime", args.timeout)
            record["actual_runtime_status"] = runtime_status
            record["expected_runtime_status"] = case["runtime_status"]
            expected = (SUITE / case["stdout"]).read_bytes()
            # The supplied native Windows interpreter writes stdout in CRT text
            # mode: every emitted LF becomes CRLF. Undo one such conversion in
            # actual output only. Do not strip whitespace or normalize expected
            # data; an intentional CR followed by LF must remain distinguishable.
            compared_output = compared_stdout(output, args.exact_bytes)
            (case_dir / "runtime.normalized.stdout").write_bytes(compared_output)
            if runtime_status != case["runtime_status"]:
                record.update(result="FAIL", reason=f"runtime status {runtime_status}; expected {case['runtime_status']}")
            elif compared_output != expected:
                record.update(result="FAIL", reason="program stdout differs")
            else:
                record.update(result="PASS", reason="runtime status and stdout match")
            if compared_output != expected:
                (case_dir / "output.diff.txt").write_text(difference(expected, compared_output), encoding="utf-8")
            elif (case_dir / "output.diff.txt").exists():
                (case_dir / "output.diff.txt").unlink()
    record["seconds"] = round(time.monotonic() - started, 3)
    (case_dir / "result.json").write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    return record


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", help="Path to your compiled IFJ26 compiler")
    parser.add_argument("--interpreter", help="Path to the official ic26int executable")
    parser.add_argument("--build", action="store_true", help="Build the real compiler into results/")
    parser.add_argument("--build-only", action="store_true", help="Build the compiler and exit without running cases")
    parser.add_argument("--cc", default="gcc", help="C compiler executable used with --build")
    parser.add_argument("--compile-only", action="store_true", help="Check compiler statuses only, not program results")
    parser.add_argument("--exact-bytes", action="store_true", help="Disable the Windows interpreter's CRLF transport adjustment")
    parser.add_argument("--compare", nargs=2, type=Path, metavar=("EXPECTED", "ACTUAL"), help="Compare an expected stdout file with a manually captured program stdout")
    parser.add_argument("--group", action="append", help="Select a group; repeat to select several")
    parser.add_argument("--case", action="append", help="Select a case id or a substring; repeat as needed")
    parser.add_argument("--list", action="store_true", help="List matching cases without running anything")
    parser.add_argument("--check-fixtures", action="store_true", help="Check fixture files and manifest only")
    parser.add_argument("--timeout", type=float, default=10, help="Runtime timeout per program in seconds")
    parser.add_argument("--compiler-timeout", type=float, default=30)
    parser.add_argument("--build-timeout", type=float, default=180)
    parser.add_argument("--results", type=Path, default=SUITE / "results")
    parser.add_argument("--quiet", action="store_true", help="Print failures and final summary only")
    args = parser.parse_args()
    if min(args.timeout, args.compiler_timeout, args.build_timeout) <= 0:
        parser.error("Timeouts must be positive")
    try:
        if args.compare:
            expected = args.compare[0].read_bytes()
            actual = compared_stdout(args.compare[1].read_bytes(), args.exact_bytes)
            if expected != actual:
                print(difference(expected, actual))
                return 1
            print("Output matches.")
            return 0
        cases = load_cases()
        groups = {case["group"] for case in cases}
        if args.group and set(args.group) - groups:
            raise ValueError(f"Unknown group; choose from {', '.join(sorted(groups))}")
        cases = [case for case in cases
                 if (not args.group or case["group"] in args.group)
                 and (not args.case or any(pattern in case["id"] for pattern in args.case))]
        if not cases:
            raise ValueError("No matching test cases")
        if args.list or args.check_fixtures:
            if args.list:
                for case in cases:
                    print(f"{case['id']}: {case['description']}")
            print(f"Valid fixtures: {len(cases)}")
            return 0
        results = args.results.resolve()
        results.mkdir(parents=True, exist_ok=True)
        if args.build or args.build_only:
            compiler = build_compiler(args.cc, results / ("ifj26compiler.exe" if os.name == "nt" else "ifj26compiler"), args.build_timeout)
        else:
            default_compiler = ROOT / ("ifj26compiler.exe" if os.name == "nt" else "ifj26compiler")
            compiler = executable(args.compiler or default_compiler)
        if args.build_only:
            return 0
        interpreter = None
        if not args.compile_only:
            candidate = args.interpreter or (str(DEFAULT_INTERPRETER) if DEFAULT_INTERPRETER.is_file() else "ic26int")
            interpreter = executable(candidate)
        print(f"Compiler: {compiler}\nInterpreter: {interpreter or 'not used (--compile-only)'}\nCases: {len(cases)}", flush=True)
        records = []
        for case in cases:
            record = run_case(case, compiler, interpreter, args, results)
            records.append(record)
            if not args.quiet or record["result"] == "FAIL":
                print(f"{record['result']:10} {record['id']}: {record['reason']}", flush=True)
        counts = collections.Counter(record["result"] for record in records)
        report = {"compiler": compiler, "interpreter": interpreter, "compile_only": args.compile_only,
                  "windows_crlf_adjustment": os.name == "nt" and not args.exact_bytes,
                  "counts": dict(counts), "cases": records}
        (results / "summary.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print("\n" + "; ".join(f"{key}={value}" for key, value in sorted(counts.items())))
        print(f"Results: {results / 'summary.json'}")
        if args.compile_only:
            print("COMPILE_OK is not a runtime PASS; use ic26int to verify expected stdout.")
        return 1 if counts["FAIL"] else 0
    except (OSError, ValueError, subprocess.TimeoutExpired) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
