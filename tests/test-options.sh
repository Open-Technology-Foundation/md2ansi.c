#!/usr/bin/env bash
# tests/test-options.sh - CLI flags + exit codes
set -uo pipefail
TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$TEST_DIR/utils.sh"
MD="${BIN_DIR:-$TEST_DIR/..}/md2ansi"
fails=0

# --help works and contains "Usage"
out=$("$MD" --help 2>&1); rc=$?
assert_exit_code 0 "$rc" "--help exits 0" || ((fails++))
assert_contains "$out" "Usage:" "--help shows Usage" || ((fails++))

# -V version
out=$("$MD" -V 2>&1); rc=$?
assert_exit_code 0 "$rc" "-V exits 0" || ((fails++))
assert_contains "$out" "1.0.4" "version string present" || ((fails++))

# --bogus → 22
out=$("$MD" --bogus 2>&1); rc=$?
assert_exit_code 22 "$rc" "invalid long opt → 22" || ((fails++))

# Bundled bad short -X → 22
out=$("$MD" -X 2>&1); rc=$?
assert_exit_code 22 "$rc" "invalid short opt → 22" || ((fails++))

# File not found → 3
out=$("$MD" /nonexistent.md 2>&1); rc=$?
assert_exit_code 3 "$rc" "missing file → 3" || ((fails++))

# Directory → 4
out=$("$MD" /tmp 2>&1); rc=$?
assert_exit_code 4 "$rc" "directory → 4" || ((fails++))

# -w with invalid value → 22
out=$("$MD" -w abc 2>&1); rc=$?
assert_exit_code 22 "$rc" "-w abc → 22" || ((fails++))

# -w 1 (too narrow) → 22
out=$("$MD" -w 1 2>&1); rc=$?
assert_exit_code 22 "$rc" "-w 1 → 22" || ((fails++))

# -w 1000 (too wide) → 22
out=$("$MD" -w 1000 2>&1); rc=$?
assert_exit_code 22 "$rc" "-w 1000 → 22" || ((fails++))

# --width 100 ok
out=$(echo '# hi' | "$MD" --width 100 2>&1); rc=$?
assert_exit_code 0 "$rc" "--width 100 → 0" || ((fails++))

# --color=auto ok
out=$(echo hi | "$MD" --color=auto); rc=$?
assert_exit_code 0 "$rc" "--color=auto → 0" || ((fails++))

# --color=bogus → 22
out=$("$MD" --color=bogus 2>&1); rc=$?
assert_exit_code 22 "$rc" "--color=bogus → 22" || ((fails++))

# NO_COLOR env strips colors
out=$(echo '# hi' | NO_COLOR=1 "$MD" --color=auto)
assert_not_contains "$out" $'\033[' "NO_COLOR suppresses ANSI" || ((fails++))

# --color=never strips colors
out=$(echo '# hi' | "$MD" --color=never)
assert_not_contains "$out" $'\033[' "--color=never suppresses ANSI" || ((fails++))

((fails == 0)) && { echo "All option tests passed."; exit 0; }
echo "$fails option tests failed."
exit 1
#fin
