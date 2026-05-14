#!/usr/bin/env bash
# tests/test-security.sh - ANSI injection, size limits, fuzz inputs
set -uo pipefail
TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$TEST_DIR/utils.sh"
MD="${BIN_DIR:-$TEST_DIR/..}/md2ansi"
fails=0

# ANSI in input is stripped
out=$(printf 'before\033[31mRED\033[0mafter\n' | "$MD" --color=always)
assert_not_contains "$out" $'before\033[31m' "raw ANSI in text stripped" || ((fails++))
assert_contains    "$out" "RED"             "RED literal preserved" || ((fails++))

# 10MB+1 byte stdin → exit 9
out=$(head -c $((10*1024*1024 + 1)) /dev/zero | "$MD" 2>&1); rc=$?
assert_exit_code 9 "$rc" "10MB+1 stdin → 9" || ((fails++))

# 10MB+1 byte file → exit 9
big_file=$TMP_DIR/big.md
head -c $((10*1024*1024 + 1)) /dev/zero > "$big_file"
out=$("$MD" "$big_file" 2>&1); rc=$?
assert_exit_code 9 "$rc" "10MB+1 file → 9" || ((fails++))

# Deeply nested lists do not crash
nested=''
for n in $(seq 1 20); do
  indent=$(printf '%*s' $((n*2 - 2)) '')
  nested+="${indent}- level $n"$'\n'
done
out=$(printf '%s' "$nested" | "$MD" --color=always); rc=$?
assert_exit_code 0 "$rc" "deeply nested lists ok" || ((fails++))
assert_contains "$out" "level 20" "deepest level rendered" || ((fails++))

# Malformed fence (unterminated) doesn't crash
out=$(printf '```python\ndef foo():\n    pass\n' | "$MD"); rc=$?
assert_exit_code 0 "$rc" "unterminated fence → 0" || ((fails++))

# Empty input ok
out=$(printf '' | "$MD"); rc=$?
assert_exit_code 0 "$rc" "empty stdin → 0" || ((fails++))

# Very long single line
long=$(printf '%.s_' {1..5000})
out=$(printf '%s\n' "$long" | "$MD"); rc=$?
assert_exit_code 0 "$rc" "very long line → 0" || ((fails++))

((fails == 0)) && { echo "All security tests passed."; exit 0; }
echo "$fails security tests failed."
exit 1
#fin
