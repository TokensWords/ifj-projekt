#!/bin/sh

# AI-authored integration runner.
# Prompt: Execute IFJ26 fixtures without encoding language rules.

set -u

scriptDirectory=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
projectDirectory=$(CDPATH= cd -- "$scriptDirectory/.." && pwd)
compiler=${IFJ_COMPILER:-"$projectDirectory/ifj26compiler"}
suiteName=${1:-currentScaffold}
fixtureDirectory="$scriptDirectory/integration"

case "$suiteName" in
    currentScaffold)
        suiteDirectory="$fixtureDirectory/currentScaffold"
        ;;
    future)
        suiteDirectory="$fixtureDirectory/future"
        ;;
    all)
        suiteDirectory="$fixtureDirectory"
        ;;
    *)
        echo "Usage: $0 [currentScaffold|future|all]" >&2
        exit 2
        ;;
esac

if [ ! -x "$compiler" ]; then
    echo "Compiler unavailable: $compiler" >&2
    echo "Build it using: make build" >&2
    exit 2
fi

if [ ! -d "$suiteDirectory" ]; then
    echo "Fixture directory unavailable: $suiteDirectory" >&2
    exit 2
fi

temporaryDirectory="${TMPDIR:-/tmp}/ifj-integration.$$"
if ! (umask 077 && mkdir "$temporaryDirectory"); then
    echo "Temporary directory unavailable: $temporaryDirectory" >&2
    exit 2
fi
trap 'rm -rf "$temporaryDirectory"' EXIT HUP INT TERM

fixtureList="$temporaryDirectory/fixtures"
find "$suiteDirectory" -type f -name '*.ifj' -print | LC_ALL=C sort > "$fixtureList"

passedCount=0
failedCount=0
skippedCount=0
totalCount=0

while IFS= read -r sourceFile; do
    totalCount=$((totalCount + 1))
    testBase=${sourceFile%.ifj}
    relativeName=${sourceFile#"$fixtureDirectory/"}
    statusFile="$testBase.expected.status"

    if [ -f "$testBase.pending" ]; then
        echo "SKIP $relativeName"
        skippedCount=$((skippedCount + 1))
        continue
    fi

    if [ ! -f "$statusFile" ]; then
        echo "FAIL $relativeName: missing expected status"
        failedCount=$((failedCount + 1))
        continue
    fi

    IFS= read -r expectedStatus < "$statusFile"
    case "$expectedStatus" in
        ''|*[!0-9]*)
            echo "FAIL $relativeName: invalid expected status"
            failedCount=$((failedCount + 1))
            continue
            ;;
    esac

    actualStdout="$temporaryDirectory/$totalCount.stdout"
    actualStderr="$temporaryDirectory/$totalCount.stderr"

    if "$compiler" < "$sourceFile" > "$actualStdout" 2> "$actualStderr"; then
        actualStatus=0
    else
        actualStatus=$?
    fi

    failureReason=''
    if [ "$actualStatus" -ne "$expectedStatus" ]; then
        failureReason="status $actualStatus; expected $expectedStatus"
    elif [ -f "$testBase.expected.stdout" ] && ! cmp -s "$testBase.expected.stdout" "$actualStdout"; then
        failureReason='standard output differs'
    elif [ -f "$testBase.assertEmptyStdout" ] && [ -s "$actualStdout" ]; then
        failureReason='standard output is not empty'
    elif [ -f "$testBase.expected.stderr" ] && ! cmp -s "$testBase.expected.stderr" "$actualStderr"; then
        failureReason='standard error differs'
    elif [ -f "$testBase.assertEmptyStderr" ] && [ -s "$actualStderr" ]; then
        failureReason='standard error is not empty'
    fi

    if [ -n "$failureReason" ]; then
        echo "FAIL $relativeName: $failureReason"
        failedCount=$((failedCount + 1))
    else
        echo "PASS $relativeName"
        passedCount=$((passedCount + 1))
    fi
done < "$fixtureList"

echo "Passed: $passedCount; failed: $failedCount; skipped: $skippedCount; total: $totalCount"

if [ "$failedCount" -ne 0 ]; then
    exit 1
fi
