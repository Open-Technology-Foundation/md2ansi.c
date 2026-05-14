#!/usr/bin/env bash
# tests/test-lists.sh - ul, ol, task, nested
set -uo pipefail
TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$TEST_DIR/utils.sh"
MD="${BIN_DIR:-$TEST_DIR/..}/md2ansi"
fails=0

# Unordered list
output=$(printf -- '- item one\n- item two\n' | "$MD" --color=always)
assert_contains "$output" "item one" "ul item 1" || ((fails++))
assert_contains "$output" "item two" "ul item 2" || ((fails++))
assert_contains "$output" '* '       "ul uses bullet *" || ((fails++))

# Asterisk bullet
output=$(printf '* alpha\n* beta\n' | "$MD" --color=always)
assert_contains "$output" "alpha" "asterisk-bullet ul works" || ((fails++))

# Ordered list
output=$(printf '1. first\n2. second\n3. third\n' | "$MD" --color=always)
assert_contains "$output" "1. " "ol numbered 1" || ((fails++))
assert_contains "$output" "2. " "ol numbered 2" || ((fails++))
assert_contains "$output" "third" "ol item 3" || ((fails++))

# Nested unordered
output=$(printf -- '- outer\n  - inner\n    - deepest\n' | "$MD" --color=always)
assert_contains "$output" "outer"   "nested ul outer" || ((fails++))
assert_contains "$output" "inner"   "nested ul inner" || ((fails++))
assert_contains "$output" "deepest" "nested ul deepest" || ((fails++))

# Task list
output=$(printf -- '- [ ] todo\n- [x] done\n' | "$MD" --color=always)
assert_contains "$output" "todo" "task incomplete text" || ((fails++))
assert_contains "$output" "done" "task complete text" || ((fails++))
assert_contains "$output" "[ ]"  "task open checkbox" || ((fails++))

# --no-task-lists falls back to UL
output=$(printf -- '- [x] something\n' | "$MD" --color=always --no-task-lists)
assert_contains "$output" "[x]"       "fallback task as text" || ((fails++))
assert_contains "$output" "something" "fallback task content" || ((fails++))

# Mixed nested ul + ol (marker + content are separated by an SGR code)
output=$(printf -- '- alpha\n  1. one\n  2. two\n- beta\n' | "$MD" --color=always)
assert_contains "$output" "alpha"   "mixed nest: outer alpha" || ((fails++))
assert_contains "$output" "1. "     "mixed nest: nested ol marker 1" || ((fails++))
assert_contains "$output" "one"     "mixed nest: nested ol content 1" || ((fails++))
assert_contains "$output" "2. "     "mixed nest: nested ol marker 2" || ((fails++))
assert_contains "$output" "two"     "mixed nest: nested ol content 2" || ((fails++))
assert_contains "$output" "beta"    "mixed nest: outer beta"  || ((fails++))

# Mixed task states inside same list
output=$(printf -- '- [x] done one\n- [ ] still pending\n- [x] done two\n' | "$MD" --color=always)
assert_contains "$output" "done one"      "mixed-task: completed 1" || ((fails++))
assert_contains "$output" "still pending" "mixed-task: incomplete"  || ((fails++))
assert_contains "$output" "done two"      "mixed-task: completed 2" || ((fails++))

((fails == 0)) && { echo "All list tests passed."; exit 0; }
echo "$fails list tests failed."
exit 1
#fin
