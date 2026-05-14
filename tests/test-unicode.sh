#!/usr/bin/env bash
# tests/test-unicode.sh - UTF-8 + wcwidth correctness
set -uo pipefail
TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$TEST_DIR/utils.sh"
MD="${BIN_DIR:-$TEST_DIR/..}/md2ansi"
fails=0

# CJK content preserved
out=$(echo '日本語のテスト' | "$MD")
assert_contains "$out" '日本語のテスト' "CJK passthrough" || ((fails++))

# Emoji preserved
out=$(echo 'Hello 🎉 world' | "$MD")
assert_contains "$out" '🎉' "emoji passthrough" || ((fails++))

# Combining marks
out=$(echo 'café' | "$MD")
assert_contains "$out" 'café' "accented char" || ((fails++))

# CJK wraps correctly when words are space-separated (each char = 2 cols)
# 10 "日本" pairs = 4 cols + 1 space = 5 cols/word; 10 words = ~50 cols at width 20
text='日本 日本 日本 日本 日本 日本 日本 日本 日本 日本'
out=$(echo "$text" | "$MD" --width 20 --color=never)
line_count=$(printf '%s' "$out" | grep -c .)
(( line_count >= 2 )) && printf '%s✓%s CJK wrap line count (%d lines)\n' "$GREEN" "$NC" "$line_count" \
                       || { printf '%s✗%s CJK wrap line count: got %d\n' "$RED" "$NC" "$line_count"; ((fails++)); }

# CJK in table aligned correctly
out=$(printf '| a | b |\n|---|---|\n| 日本 | x |\n' | "$MD" --color=always)
assert_contains "$out" '日本' "CJK in table" || ((fails++))

((fails == 0)) && { echo "All unicode tests passed."; exit 0; }
echo "$fails unicode tests failed."
exit 1
#fin
