#!/usr/bin/env bash
# tests/test-escapes.sh - backslash escapes + frontmatter
set -uo pipefail
TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$TEST_DIR/utils.sh"
MD="${BIN_DIR:-$TEST_DIR/..}/md2ansi"
fails=0

# Escape \*
out=$(echo 'literal \*star\* not italic' | "$MD" --color=always)
assert_contains    "$out" "*star*" "escaped *star* renders literal" || ((fails++))
assert_not_contains "$out" $'\033[3m' "escaped *not* italicized" || ((fails++))

# Escape \\
out=$(echo 'a \\ b' | "$MD" --color=always)
assert_contains "$out" "a \\ b" "double backslash renders single" || ((fails++))

# Escape \# (would otherwise be header at start)
out=$(printf '\\# not a header\n' | "$MD" --color=always)
assert_contains "$out" "# not a header" "escaped # at start" || ((fails++))

# Escape \[ \]
out=$(echo 'see \[brackets\] please' | "$MD" --color=always)
assert_contains "$out" "[brackets]" "escaped brackets literal" || ((fails++))

# Escape \`
out=$(echo 'use \`backtick\` here' | "$MD" --color=always)
assert_contains "$out" '`backtick`' "escaped backticks literal" || ((fails++))

# Frontmatter --- ... --- stripped
out=$(printf -- '---\ntitle: hi\n---\n\n# Real header\n' | "$MD" --color=always)
assert_contains    "$out" "Real header" "post-frontmatter content shown" || ((fails++))
assert_not_contains "$out" "title: hi"  "frontmatter body stripped" || ((fails++))

# Frontmatter ... terminator also works
out=$(printf -- '---\ntitle: hi\n...\n\n# Real\n' | "$MD" --color=always)
assert_contains "$out" "Real" "... terminator works" || ((fails++))

((fails == 0)) && { echo "All escape tests passed."; exit 0; }
echo "$fails escape tests failed."
exit 1
#fin
