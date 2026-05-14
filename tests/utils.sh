#!/usr/bin/env bash
# tests/utils.sh - assertion helpers.
# Note: NO 'set -e' here — tests intentionally run commands that exit non-zero
# (e.g. validating --bogus → 22) and use $? to capture the code.
set -uo pipefail

TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FIXTURES_DIR="$TEST_DIR/fixtures"
TMP_DIR="$FIXTURES_DIR/tmp"
mkdir -p "$TMP_DIR"

if [[ -t 1 ]]; then
  GREEN=$'\033[0;32m'; RED=$'\033[0;31m'; YELLOW=$'\033[0;33m'; NC=$'\033[0m'
else
  GREEN=''; RED=''; YELLOW=''; NC=''
fi

assert_equals() {
  local actual="$1" expected="$2" msg="${3:-}"
  if [[ "$actual" == "$expected" ]]; then
    printf '%s✓ pass%s%s\n' "$GREEN" "$NC" "${msg:+: $msg}"
    return 0
  fi
  printf '%s✗ fail%s%s\n' "$RED" "$NC" "${msg:+: $msg}"
  printf '  expected: %s\n' "$expected"
  printf '  actual:   %s\n' "$actual"
  return 1
}

assert_contains() {
  local haystack="$1" needle="$2" msg="${3:-}"
  if [[ "$haystack" == *"$needle"* ]]; then
    printf '%s✓ pass%s%s\n' "$GREEN" "$NC" "${msg:+: $msg}"
    return 0
  fi
  printf '%s✗ fail%s%s\n' "$RED" "$NC" "${msg:+: $msg}"
  printf '  needle: %s\n' "$needle"
  printf '  in:     %.200s\n' "$haystack"
  return 1
}

assert_not_contains() {
  local haystack="$1" needle="$2" msg="${3:-}"
  if [[ "$haystack" != *"$needle"* ]]; then
    printf '%s✓ pass%s%s\n' "$GREEN" "$NC" "${msg:+: $msg}"
    return 0
  fi
  printf '%s✗ fail%s%s\n' "$RED" "$NC" "${msg:+: $msg}"
  printf '  forbidden needle present: %s\n' "$needle"
  return 1
}

assert_exit_code() {
  local expected="$1" actual="$2" msg="${3:-}"
  if [[ "$actual" -eq "$expected" ]]; then
    printf '%s✓ pass%s%s\n' "$GREEN" "$NC" "${msg:+: $msg}"
    return 0
  fi
  printf '%s✗ fail%s%s\n' "$RED" "$NC" "${msg:+: $msg}"
  printf '  expected exit %d, got %d\n' "$expected" "$actual"
  return 1
}

assert_file_equals() {
  local actual_file="$1" expected_file="$2" msg="${3:-}"
  if diff -q "$actual_file" "$expected_file" > /dev/null; then
    printf '%s✓ pass%s%s\n' "$GREEN" "$NC" "${msg:+: $msg}"
    return 0
  fi
  printf '%s✗ fail%s%s\n' "$RED" "$NC" "${msg:+: $msg}"
  diff "$expected_file" "$actual_file" | head -20
  return 1
}

cleanup_tmp() { rm -f "$TMP_DIR"/*; }
trap cleanup_tmp EXIT
#fin
