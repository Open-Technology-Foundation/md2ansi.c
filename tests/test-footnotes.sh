#!/usr/bin/env bash
# tests/test-footnotes.sh
set -uo pipefail
TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$TEST_DIR/utils.sh"
MD="${BIN_DIR:-$TEST_DIR/..}/md2ansi"
fails=0

# Footnote ref + def
output=$(printf 'Text[^1] more.\n\n[^1]: definition\n' | "$MD" --color=always)
assert_contains "$output" "^1"         "ref shows id" || ((fails++))
assert_contains "$output" "Footnotes:" "section header" || ((fails++))
assert_contains "$output" "definition" "def text emitted" || ((fails++))

# Missing definition warning
output=$(printf 'Text[^missing] ref only.\n' | "$MD" --color=always)
assert_contains "$output" "Missing footnote definition" "missing def text" || ((fails++))

# Multiple footnotes preserve appearance order
output=$(printf 'a[^b] b[^a] c[^c]\n\n[^a]: first\n[^b]: second\n[^c]: third\n' | "$MD" --color=always)
# All three should appear in footer
assert_contains "$output" "first"  "fn a def" || ((fails++))
assert_contains "$output" "second" "fn b def" || ((fails++))
assert_contains "$output" "third"  "fn c def" || ((fails++))

# --no-footnotes disables section
output=$(printf 'Text[^1] more.\n\n[^1]: definition\n' | "$MD" --color=always --no-footnotes)
assert_not_contains "$output" "Footnotes:" "section suppressed" || ((fails++))

((fails == 0)) && { echo "All footnote tests passed."; exit 0; }
echo "$fails footnote tests failed."
exit 1
#fin
