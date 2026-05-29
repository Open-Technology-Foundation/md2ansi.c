#!/usr/bin/env bash
# tests/test-tables.sh - pipe tables, alignment, CJK width
set -uo pipefail
TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$TEST_DIR/utils.sh"
MD="${BIN_DIR:-$TEST_DIR/..}/md2ansi"
fails=0

# Basic table renders with box-drawing
output=$(printf '| h1 | h2 |\n|----|----|\n| a  | b  |\n' | "$MD" --color=always)
assert_contains "$output" $'\xe2\x94\x8c' "top-left corner" || ((fails++))
assert_contains "$output" $'\xe2\x94\x90' "top-right corner" || ((fails++))
assert_contains "$output" $'\xe2\x94\x82' "vertical bar" || ((fails++))
assert_contains "$output" $'\xe2\x94\xbc' "cross" || ((fails++))
assert_contains "$output" "h1"            "header h1" || ((fails++))
assert_contains "$output" "h2"            "header h2" || ((fails++))

# Right alignment
output=$(printf '| n |\n|---:|\n| 42 |\n' | "$MD" --color=always)
assert_contains "$output" "42" "right-align table renders content" || ((fails++))

# Center alignment
output=$(printf '| n |\n|:---:|\n| 1 |\n' | "$MD" --color=always)
assert_contains "$output" "1" "center-align table renders content" || ((fails++))

# CJK width: 日 counts as 2 cols
output=$(printf '| a | b |\n|---|---|\n| 日本人 | x |\n' | "$MD" --color=always)
assert_contains "$output" "日本人" "CJK content preserved" || ((fails++))

# --no-tables fallback: pipe lines as plain text
output=$(printf '| a | b |\n|---|---|\n| 1 | 2 |\n' | "$MD" --color=always --no-tables)
assert_not_contains "$output" $'\xe2\x94\x8c' "no-tables disables borders" || ((fails++))

# Backslash-escaped pipe in cell stays literal (not a column separator)
output=$(printf '| col |\n|-----|\n| a\\|b\\|c |\n' | "$MD" --color=always)
assert_contains     "$output" "a|b|c"        "escaped \\| renders as literal pipe" || ((fails++))
assert_not_contains "$output" $'\xe2\x94\xac' "escaped pipes do not create extra columns" || ((fails++))

# Pipe inside backtick code span stays literal
output=$(printf '| col |\n|-----|\n| `--mode=a|b|c` |\n' | "$MD" --color=always)
assert_contains     "$output" "a|b|c"        "pipe inside backticks renders as literal" || ((fails++))
assert_not_contains "$output" $'\xe2\x94\xac' "backticked pipes do not create extra columns" || ((fails++))

# Mixed: escaped pipe + backticked pipe in same row, multi-column
output=$(printf '| col1 | col2 |\n|------|------|\n| `x|y` | a\\|b |\n' | "$MD" --color=always)
assert_contains "$output" "x|y" "backticked pipe preserved in multi-col row" || ((fails++))
assert_contains "$output" "a|b" "escaped pipe preserved in multi-col row" || ((fails++))

# CommonMark: \| inside backticks stays literal (no escape processing in code spans)
output=$(printf '| col |\n|-----|\n| see `\\|` here |\n' | "$MD" --color=always)
assert_contains     "$output" '\|'  "backslash-pipe inside backticks preserved literally" || ((fails++))
assert_not_contains "$output" 'see `|` here' "backslash NOT stripped from inside-backtick \\|" || ((fails++))

# Overlong cell wraps to a forced narrow terminal width (--width 40).
# --color=never emits structural box-drawing only (no ANSI), so `wc -L` under
# a UTF-8 locale measures true display columns directly.
wide=$'| Module | Responsibility |\n|--------|----------------|\n| render | This is a very long responsibility description that must exceed forty columns and therefore wrap across multiple physical lines |\n'
wrapped=$(printf '%s' "$wide" | "$MD" --color=never --width 40)
nowrap=$(printf '%s' "$wide" | "$MD" --color=never --width 40 --no-table-wrap)

max_w=$(printf '%s\n' "$wrapped" | LC_ALL=C.UTF-8 wc -L)
if ((max_w <= 40)); then
  printf '%s✓ pass%s: wrapped table fits 40 cols (max=%s)\n' "$GREEN" "$NC" "$max_w"
else
  printf '%s✗ fail%s: wrapped table exceeds 40 cols (max=%s)\n' "$RED" "$NC" "$max_w"; ((fails+=1))
fi

assert_contains "$wrapped" "responsibility" "long cell content preserved when wrapped" || ((fails++))

lines_wrapped=$(printf '%s\n' "$wrapped" | wc -l)
lines_nowrap=$(printf '%s\n' "$nowrap" | wc -l)
if ((lines_wrapped > lines_nowrap)); then
  printf '%s✓ pass%s: wrapped cell spans multiple physical lines (%s > %s)\n' \
    "$GREEN" "$NC" "$lines_wrapped" "$lines_nowrap"
else
  printf '%s✗ fail%s: wrapped cell did not add physical lines (%s vs %s)\n' \
    "$RED" "$NC" "$lines_wrapped" "$lines_nowrap"; ((fails+=1))
fi

# --no-table-wrap preserves the overflow path: wide line not split.
max_nw=$(printf '%s\n' "$nowrap" | LC_ALL=C.UTF-8 wc -L)
if ((max_nw > 40)); then
  printf '%s✓ pass%s: --no-table-wrap preserves overflow (max=%s)\n' "$GREEN" "$NC" "$max_nw"
else
  printf '%s✗ fail%s: --no-table-wrap unexpectedly fit width (max=%s)\n' "$RED" "$NC" "$max_nw"; ((fails+=1))
fi
assert_contains "$nowrap" "responsibility" "content intact under --no-table-wrap" || ((fails++))

((fails == 0)) && { echo "All table tests passed."; exit 0; }
echo "$fails table tests failed."
exit 1
#fin
