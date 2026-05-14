#!/usr/bin/env bash
# tests/test-code.sh - fenced/indented code blocks + syntax highlighting
set -uo pipefail
TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$TEST_DIR/utils.sh"
MD="${BIN_DIR:-$TEST_DIR/..}/md2ansi"
fails=0

# Fenced code block (no lang)
output=$(printf '```\nplain code\n```\n' | "$MD" --color=always)
assert_contains "$output" "plain code" "fenced code (no lang) preserves text" || ((fails++))
assert_contains "$output" $'\033[90m'  "fenced code uses CODEBLOCK color" || ((fails++))

# Tilde fence
output=$(printf '~~~\ntilde fence\n~~~\n' | "$MD" --color=always)
assert_contains "$output" "tilde fence" "tilde-fenced code preserves text" || ((fails++))

# Python syntax highlight
output=$(printf '```python\ndef foo():\n    return 42\n```\n' | "$MD" --color=always)
assert_contains "$output" $'\033[38;5;204m' "python keyword colored" || ((fails++))
assert_contains "$output" "def" "python def emitted" || ((fails++))
assert_contains "$output" "return" "python return emitted" || ((fails++))

# Bash syntax highlight
output=$(printf '```bash\nfor i in 1 2; do\n  echo "$i"\ndone\n```\n' | "$MD" --color=always)
assert_contains "$output" $'\033[38;5;204m' "bash keyword colored" || ((fails++))
assert_contains "$output" "for"  "bash for emitted" || ((fails++))
assert_contains "$output" "done" "bash done emitted" || ((fails++))

# JS syntax highlight
output=$(printf '```javascript\nfunction f() { return 1; }\n```\n' | "$MD" --color=always)
assert_contains "$output" $'\033[38;5;204m' "js keyword colored" || ((fails++))

# Python comment
output=$(printf '```python\n# this is a comment\n```\n' | "$MD" --color=always)
assert_contains "$output" $'\033[38;5;245m' "python comment colored" || ((fails++))

# --no-syntax-highlight disables it
output=$(printf '```python\ndef foo():\n    pass\n```\n' | "$MD" --color=always --no-syntax-highlight)
assert_not_contains "$output" $'\033[38;5;204m' "no-syntax-highlight disables keyword color" || ((fails++))

# Indented code (4 spaces)
output=$(printf 'normal text\n\n    indented code\n\nmore text\n' | "$MD" --color=always)
assert_contains "$output" "indented code" "indented code block preserved" || ((fails++))

# Tab-indented code block
output=$(printf 'para\n\n\ttab-indented code\n\nend\n' | "$MD" --color=always)
assert_contains "$output" "tab-indented code" "tab-indented code preserved" || ((fails++))
assert_contains "$output" $'\033[90m'        "tab-indented uses CODEBLOCK color" || ((fails++))

# Bash comment colored
output=$(printf '```bash\n# bash comment line\necho hi\n```\n' | "$MD" --color=always)
assert_contains "$output" $'\033[38;5;245m' "bash comment colored" || ((fails++))

# JS comment colored
output=$(printf '```javascript\n// js comment line\nfoo();\n```\n' | "$MD" --color=always)
assert_contains "$output" $'\033[38;5;245m' "js comment colored" || ((fails++))

# Python alias 'py' triggers highlighting
output=$(printf '```py\ndef f(): pass\n```\n' | "$MD" --color=always)
assert_contains "$output" $'\033[38;5;204m' "py alias triggers python highlight" || ((fails++))

# Bash alias 'sh' triggers highlighting
output=$(printf '```sh\nfor x in 1; do :; done\n```\n' | "$MD" --color=always)
assert_contains "$output" $'\033[38;5;204m' "sh alias triggers bash highlight" || ((fails++))

# ANSI in code block is sanitized
output=$(printf '```\nbad\033[31mRED\033[0m\n```\n' | "$MD" --color=always)
assert_not_contains "$output" $'bad\033[31m' "raw ANSI in code stripped" || ((fails++))

# Mismatched fence inside code block treated as content
output=$(printf '```\nstart\n~~~\nend\n```\n' | "$MD" --color=always)
assert_contains "$output" "~~~" "mismatched fence rendered as content" || ((fails++))

# Multi-backtick inline code: `` content with ` inside ``
output=$(printf '%s\n' '``foo`bar``' | "$MD" --color=always)
assert_contains "$output" 'foo`bar' "double-backtick span keeps single backtick" || ((fails++))

# Triple-backtick wrap around double-backtick content
output=$(printf '%s\n' '```foo``bar```' | "$MD" --color=always)
assert_contains "$output" 'foo``bar' "triple-backtick span keeps double backticks" || ((fails++))

# Multi-backtick inside table cell (the README pattern)
output=$(printf '| col |\n|-----|\n| Fenced (%s + %s) |\n' '`` ``` ``' '`~~~`' | "$MD" --color=always)
assert_contains    "$output" '```' "table cell: triple-backtick literal inside double-backtick span" || ((fails++))
assert_contains    "$output" '~~~' "table cell: single-backtick span around tildes" || ((fails++))
assert_not_contains "$output" 'Fenced ([90m[0m' "table cell: no spurious empty code spans" || ((fails++))

# CommonMark: leading + trailing single space stripped when content not all spaces
output=$(printf '%s\n' '`` foo ``' | "$MD" --color=always)
assert_contains "$output" $'\033[90mfoo\033[0m' "single space pad stripped from code content" || ((fails++))

((fails == 0)) && { echo "All code tests passed."; exit 0; }
echo "$fails code tests failed."
exit 1
#fin
