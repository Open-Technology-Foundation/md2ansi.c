#!/usr/bin/env bash
# tests/test-blockquotes.sh - > blockquote rendering
set -uo pipefail
TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$TEST_DIR/utils.sh"
MD="${BIN_DIR:-$TEST_DIR/..}/md2ansi"
fails=0

# Single-line quote
out=$(echo '> quoted text' | "$MD" --color=always)
assert_contains "$out" "quoted text"        "single-line quote text" || ((fails++))
assert_contains "$out" $'\033[48;5;236m'    "quote uses dark-bg" || ((fails++))
assert_contains "$out" "> "                 "quote marker preserved" || ((fails++))

# Multi-line quote
out=$(printf '> line one\n> line two\n' | "$MD" --color=always)
assert_contains "$out" "line one"           "multi-line quote 1" || ((fails++))
assert_contains "$out" "line two"           "multi-line quote 2" || ((fails++))

# Plain mode keeps text, no background
out=$(echo '> quoted' | "$MD" --color=never)
assert_contains    "$out" "quoted"                  "plain-mode quote text" || ((fails++))
assert_not_contains "$out" $'\033[48;5;236m'        "plain-mode no dark-bg" || ((fails++))

# Inline formatting inside blockquote
out=$(echo '> with **bold** in it' | "$MD" --color=always)
assert_contains "$out" "bold"               "blockquote contains bolded text" || ((fails++))
assert_contains "$out" $'\033[1m'           "blockquote applies BOLD inside" || ((fails++))

# Quote followed by paragraph — both render
out=$(printf '> a quote\n\nplain para\n' | "$MD" --color=never)
assert_contains "$out" "a quote"            "quote line emitted" || ((fails++))
assert_contains "$out" "plain para"         "trailing paragraph emitted" || ((fails++))

((fails == 0)) && { echo "All blockquote tests passed."; exit 0; }
echo "$fails blockquote tests failed."
exit 1
#fin
