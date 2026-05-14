#!/usr/bin/env bash
# tests/run-tests.sh - aggregate all md2ansi C tests
set -uo pipefail

TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export BIN_DIR="$(cd "$TEST_DIR/.." && pwd)"

if [[ -t 1 ]]; then
  GREEN=$'\033[0;32m'; RED=$'\033[0;31m'; YELLOW=$'\033[0;33m'; NC=$'\033[0m'
else
  GREEN=''; RED=''; YELLOW=''; NC=''
fi

tests=(
  test-basic.sh
  test-code.sh
  test-lists.sh
  test-tables.sh
  test-footnotes.sh
  test-options.sh
  test-security.sh
  test-wrapping.sh
  test-unicode.sh
  test-links.sh
  test-escapes.sh
  test-blockquotes.sh
  test-misc.sh
  test-companions.sh
)

pass=0; fail=0
failed=()

for t in "${tests[@]}"; do
  printf '%s=== %s ===%s\n' "$YELLOW" "$t" "$NC"
  if bash "$TEST_DIR/$t"; then
    ((pass++))
  else
    ((fail++))
    failed+=("$t")
  fi
  echo
done

printf '====================\n'
printf 'Passed: %s%d%s\n' "$GREEN" "$pass" "$NC"
printf 'Failed: %s%d%s\n' "$RED"   "$fail" "$NC"
if ((fail)); then
  printf 'Failing tests:\n'
  for f in "${failed[@]}"; do printf '  - %s\n' "$f"; done
  exit 1
fi
printf '%sAll test suites passed.%s\n' "$GREEN" "$NC"
exit 0
#fin
