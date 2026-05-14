#!/usr/bin/env bash
# tests/test-wrapping.sh - width-based word wrapping
set -uo pipefail
TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$TEST_DIR/utils.sh"
MD="${BIN_DIR:-$TEST_DIR/..}/md2ansi"
fails=0

# Long text wraps at requested width
long='one two three four five six seven eight nine ten eleven twelve'
out=$(printf '%s\n' "$long" | "$MD" --color=never --width 30)
line_count=$(printf '%s' "$out" | wc -l)
# Should wrap into 2+ lines
(( line_count >= 2 )) && pass=1 || pass=0
[[ $pass -eq 1 ]] && printf '%s✓%s long text wraps at width 30 (%d lines)\n' "$GREEN" "$NC" "$line_count" || { printf '%s✗%s expected ≥2 lines, got %d\n' "$RED" "$NC" "$line_count"; ((fails++)); }

# Short text does NOT wrap
out=$(echo 'short' | "$MD" --color=never --width 80)
assert_equals "$out" "short" "short text emits single line" || ((fails++))

# Width is preserved in wrap output
out=$(printf 'aaa bbb ccc ddd\n' | "$MD" --color=never --width 5)
# Each wrapped line should be ≤ 5 visible chars
while IFS= read -r line; do
  vlen=${#line}
  (( vlen <= 5 )) || { printf '%s✗%s wrap line too long: |%s|=%d\n' "$RED" "$NC" "$line" "$vlen"; ((fails++)); }
done <<< "$out"

((fails == 0)) && { echo "All wrapping tests passed."; exit 0; }
echo "$fails wrapping tests failed."
exit 1
#fin
