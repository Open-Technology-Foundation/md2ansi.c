#!/usr/bin/env bash
# tests/test-links.sh - inline / ref / autolink / image
set -uo pipefail
TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$TEST_DIR/utils.sh"
MD="${BIN_DIR:-$TEST_DIR/..}/md2ansi"
fails=0

# Inline link
out=$(echo '[click](https://example.com)' | "$MD" --color=always)
assert_contains "$out" "click" "inline link text" || ((fails++))
assert_contains "$out" $'\033[38;5;45m' "link color" || ((fails++))
assert_contains "$out" $'\033[4m'       "link underlined" || ((fails++))

# Image
out=$(echo '![alt](logo.png)' | "$MD" --color=always)
assert_contains "$out" "IMG: alt" "image placeholder" || ((fails++))

# Ref-style link
out=$(printf '[text][ref] then more.\n\n[ref]: https://example.com\n' | "$MD" --color=always)
assert_contains "$out" "text" "ref link text" || ((fails++))
assert_contains "$out" $'\033[38;5;45m' "ref link color" || ((fails++))

# Shortcut ref
out=$(printf 'see [ref] doc.\n\n[ref]: https://example.com\n' | "$MD" --color=always)
assert_contains "$out" "ref" "shortcut ref text" || ((fails++))

# Autolink <http://...>
out=$(echo '<https://example.com>' | "$MD" --color=always)
assert_contains "$out" "https://example.com" "autolink URL kept" || ((fails++))
assert_contains "$out" $'\033[38;5;45m' "autolink color" || ((fails++))

# Bare URL autolink (default ON)
out=$(echo 'Visit https://example.com today.' | "$MD" --color=always)
assert_contains "$out" $'\033[38;5;45m\033[4mhttps://example.com' "bare URL autolinked" || ((fails++))

# --no-bare-urls disables it
out=$(echo 'Visit https://example.com today.' | "$MD" --color=always --no-bare-urls)
assert_not_contains "$out" $'\033[38;5;45m' "no-bare-urls suppresses autolink" || ((fails++))

# --no-links suppresses link colour
out=$(echo '[click](https://example.com)' | "$MD" --color=always --no-links)
assert_not_contains "$out" $'\033[38;5;45m' "--no-links suppresses link color" || ((fails++))

# --no-images shows literal markdown
out=$(echo '![alt](logo.png)' | "$MD" --color=always --no-images)
assert_contains "$out" "![alt](logo.png)" "--no-images keeps literal" || ((fails++))

((fails == 0)) && { echo "All link tests passed."; exit 0; }
echo "$fails link tests failed."
exit 1
#fin
