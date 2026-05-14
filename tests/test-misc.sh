#!/usr/bin/env bash
# tests/test-misc.sh - shebang, multi-file, permission denied, --plain, --debug, reset count
set -uo pipefail
TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$TEST_DIR/utils.sh"
MD="${BIN_DIR:-$TEST_DIR/..}/md2ansi"
fails=0

# Shebang line on file ingest is stripped
shebang_file=$TMP_DIR/shebang.md
printf '#!/usr/bin/env md2ansi\n\n# Real Header\nbody\n' > "$shebang_file"
out=$("$MD" --color=never "$shebang_file")
assert_not_contains "$out" "/usr/bin/env" "shebang line dropped from file" || ((fails++))
assert_contains    "$out" "Real Header"   "post-shebang header rendered"  || ((fails++))

# Multi-file: each file processed, separator newline between
file_a=$TMP_DIR/a.md
file_b=$TMP_DIR/b.md
printf 'alpha\n' > "$file_a"
printf 'beta\n'  > "$file_b"
out=$("$MD" --color=never "$file_a" "$file_b")
assert_contains "$out" "alpha" "multi-file: a content" || ((fails++))
assert_contains "$out" "beta"  "multi-file: b content" || ((fails++))
# alpha must appear before beta
[[ "$out" == *alpha*beta* ]] \
  && printf '%s✓%s multi-file order preserved\n' "$GREEN" "$NC" \
  || { printf '%s✗%s multi-file order wrong\n' "$RED" "$NC"; ((fails++)); }

# Unreadable file → exit 13
noread=$TMP_DIR/noread.md
: > "$noread"
chmod 000 "$noread"
out=$("$MD" "$noread" 2>&1); rc=$?
chmod 644 "$noread"
assert_exit_code 13 "$rc" "unreadable file → 13" || ((fails++))

# --plain keeps images literal (does not emit [IMG: ...])
out=$(echo '![alt](logo.png)' | "$MD" --plain --color=never)
assert_contains    "$out" '![alt](logo.png)' "--plain renders image literal" || ((fails++))
assert_not_contains "$out" 'IMG:'             "--plain suppresses [IMG: …] placeholder" || ((fails++))

# --plain keeps links literal
out=$(echo '[click](https://example.com)' | "$MD" --plain --color=never)
assert_contains "$out" '[click](https://example.com)' "--plain renders link literal" || ((fails++))

# --plain disables table borders
out=$(printf '| a | b |\n|---|---|\n| 1 | 2 |\n' | "$MD" --plain --color=never)
assert_not_contains "$out" $'\xe2\x94\x8c' "--plain disables table box-drawing" || ((fails++))

# --plain disables footnote section
out=$(printf 'see[^1]\n\n[^1]: note\n' | "$MD" --plain --color=never)
assert_not_contains "$out" "Footnotes:" "--plain disables footnote section" || ((fails++))

# -t short alias of --plain
out=$(echo '![alt](x.png)' | "$MD" -t --color=never)
assert_contains "$out" '![alt](x.png)' "-t short alias of --plain" || ((fails++))

# --debug emits trace lines to stderr (NOT stdout)
stderr_out=$(echo '# hi' | "$MD" --debug 2>&1 >/dev/null)
assert_contains "$stderr_out" "width=" "--debug trace contains width= on stderr" || ((fails++))
stdout_out=$(echo '# hi' | "$MD" --debug --color=never 2>/dev/null)
assert_not_contains "$stdout_out" "width=" "--debug trace absent from stdout" || ((fails++))

# Output ends with a reset (clean terminal state on exit)
out=$(echo '# hi' | "$MD" --color=always)
[[ "$out" == *$'\033[0m' ]] \
  && printf '%s✓%s output ends with reset\n' "$GREEN" "$NC" \
  || { printf '%s✗%s output does not end with reset\n' "$RED" "$NC"; ((fails++)); }

# --color=never produces no ANSI at all
out=$(printf '# H1\n**b** *i* `c`\n' | "$MD" --color=never)
assert_not_contains "$out" $'\033[' "--color=never strips all ANSI" || ((fails++))

# Stdin from /dev/null works (empty)
out=$("$MD" --color=never < /dev/null); rc=$?
assert_exit_code 0 "$rc" "stdin /dev/null → 0" || ((fails++))
assert_equals "$out" "" "stdin /dev/null produces empty output" || ((fails++))

# -w short opt with valid value
out=$(echo '# hi' | "$MD" -w 60 --color=never); rc=$?
assert_exit_code 0 "$rc" "-w 60 → 0" || ((fails++))

# -w without argument → 8
out=$("$MD" -w 2>&1); rc=$?
assert_exit_code 8 "$rc" "-w with no value → 8" || ((fails++))

((fails == 0)) && { echo "All misc tests passed."; exit 0; }
echo "$fails misc tests failed."
exit 1
#fin
