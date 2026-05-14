#!/usr/bin/env bash
# tests/test-basic.sh - headers, inline formatting, HR
set -uo pipefail
TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$TEST_DIR/utils.sh"
MD="${BIN_DIR:-$TEST_DIR/..}/md2ansi"
[[ -x $MD ]] || { echo "md2ansi not found at $MD"; exit 1; }

fails=0

# Headers H1-H6
for n in 1 2 3 4 5 6; do
  hashes=$(printf '#%.0s' $(seq 1 "$n"))
  output=$(printf '%s header text\n' "$hashes" | "$MD" --color=always)
  assert_contains "$output" "header text" "H$n contains text" || ((fails++))
done

# Bold
output=$(echo '**bold text**' | "$MD" --color=always)
assert_contains "$output" "bold text" "bold contains text" || ((fails++))
assert_contains "$output" $'\033[1m'   "bold uses BOLD escape" || ((fails++))

# Italic
output=$(echo 'a *italic* b' | "$MD" --color=always)
assert_contains "$output" "italic" "italic contains text" || ((fails++))
assert_contains "$output" $'\033[3m'   "italic uses ITALIC escape" || ((fails++))

# Strike
output=$(echo 'a ~~strike~~ b' | "$MD" --color=always)
assert_contains "$output" "strike" "strike contains text" || ((fails++))
assert_contains "$output" $'\033[9m'   "strike uses STRIKE escape" || ((fails++))

# Inline code
output=$(echo 'use `printf` here' | "$MD" --color=always)
assert_contains "$output" "printf" "inline code contains text" || ((fails++))

# HR (--- ___ ===)
for hr in '---' '___' '===' '----' '======'; do
  output=$(printf '%s\n' "$hr" | "$MD" --color=always)
  assert_contains "$output" $'\xe2\x94\x80' "HR ($hr) uses box-drawing" || ((fails++))
done

# Triple star
output=$(echo '***bold italic***' | "$MD" --color=always)
assert_contains "$output" "bold italic" "triple-star contains text" || ((fails++))
assert_contains "$output" $'\033[1m\033[3m' "triple-star bold+italic" || ((fails++))

# Inline formatting nested inside header
output=$(echo '# title with **bold** word' | "$MD" --color=always)
assert_contains "$output" "title with"        "header text emitted" || ((fails++))
assert_contains "$output" $'\033[1m'          "header retains BOLD inside" || ((fails++))

# Inline code preserved literally (no double-asterisk processing inside)
output=$(echo 'use `**not bold**` here' | "$MD" --color=always)
assert_contains    "$output" '**not bold**' "code span keeps asterisks literal" || ((fails++))
assert_not_contains "$output" $'\033[1mnot bold'   "no bold rendering inside backticks" || ((fails++))

# H7 (7 hashes) is NOT a header — degrades to paragraph or H6 cap
output=$(echo '####### too many' | "$MD" --color=always)
assert_contains "$output" "too many" "7-hash line still emits content" || ((fails++))

((fails == 0)) && { echo "All basic tests passed."; exit 0; }
echo "$fails basic tests failed."
exit 1
#fin
