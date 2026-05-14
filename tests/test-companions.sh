#!/usr/bin/env bash
# tests/test-companions.sh - smoke tests for md, mdview, md-link-extract, ansi-info
set -uo pipefail
TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$TEST_DIR/utils.sh"
BIN_DIR="${BIN_DIR:-$TEST_DIR/..}"
MD="$BIN_DIR/md2ansi"
fails=0

# Locate sibling scripts (dev mode: alongside md2ansi)
MD_WRAP="$BIN_DIR/md"
MDLE="$BIN_DIR/md-link-extract"
AINFO="$BIN_DIR/ansi-info"
MDVIEW="$BIN_DIR/mdview"

# All four companions must be present and executable
for f in "$MD_WRAP" "$MDLE" "$AINFO" "$MDVIEW"; do
  if [[ -x $f ]]; then
    printf '%s✓%s %s present + executable\n' "$GREEN" "$NC" "${f##*/}"
  else
    printf '%s✗%s %s missing or not executable\n' "$RED" "$NC" "${f##*/}"
    ((fails++))
  fi
done

# Syntax-check all bash companions
for f in "$MD_WRAP" "$MDLE" "$AINFO" "$MDVIEW"; do
  if bash -n "$f" 2>/dev/null; then
    printf '%s✓%s %s bash -n clean\n' "$GREEN" "$NC" "${f##*/}"
  else
    printf '%s✗%s %s bash -n failed\n' "$RED" "$NC" "${f##*/}"
    ((fails++))
  fi
done

# ----- md (pager wrapper) -----
# --help works without launching less
out=$("$MD_WRAP" --help 2>&1); rc=$?
assert_exit_code 0 "$rc"           "md --help → 0" || ((fails++))
assert_contains "$out" "md2ansi"   "md --help mentions md2ansi delegation" || ((fails++))
out=$("$MD_WRAP" --version 2>&1); rc=$?
assert_exit_code 0 "$rc"           "md --version → 0" || ((fails++))

# md sets LESS=-FXRS (grep its source)
assert_contains "$(<"$MD_WRAP")" "FXRS" "md sets LESS=-FXRS" || ((fails++))
# md delegates via SCRIPT_DIR/md2ansi
assert_contains "$(<"$MD_WRAP")" 'SCRIPT_DIR' "md uses SCRIPT_DIR for sibling resolution" || ((fails++))

# ----- md-link-extract -----
out=$("$MDLE" --help 2>&1); rc=$?
assert_exit_code 0 "$rc" "md-link-extract --help → 0" || ((fails++))

# Round-trip: write a file with three links, expect three URLs back, UTM stripped
tmp_md=$TMP_DIR/links.md
cat > "$tmp_md" <<'EOF'
See [first](https://example.com/a) for info.
And [second](https://example.com/b?utm_source=foo&page=2) too.
Plus an autolink: <https://example.com/c>
EOF
out=$("$MDLE" "$tmp_md")
assert_contains    "$out" "https://example.com/a"        "extract: first URL" || ((fails++))
assert_contains    "$out" "https://example.com/b?page=2" "extract: UTM stripped" || ((fails++))
assert_not_contains "$out" "utm_source"                   "extract: utm_source removed" || ((fails++))

# Missing file → non-zero
out=$("$MDLE" /nonexistent.md 2>&1); rc=$?
[[ $rc -ne 0 ]] && printf '%s✓%s md-link-extract missing-file → non-zero\n' "$GREEN" "$NC" \
                || { printf '%s✗%s md-link-extract missing-file rc=%d\n' "$RED" "$NC" "$rc"; ((fails++)); }

# ----- ansi-info -----
out=$("$AINFO" --version 2>&1); rc=$?
assert_exit_code 0 "$rc"          "ansi-info --version → 0" || ((fails++))
assert_contains "$out" "ansi-info" "ansi-info version banner" || ((fails++))
out=$("$AINFO" --help 2>&1); rc=$?
assert_exit_code 0 "$rc"          "ansi-info --help → 0" || ((fails++))

# ----- mdview -----
out=$("$MDVIEW" --version 2>&1); rc=$?
assert_exit_code 0 "$rc"          "mdview --version → 0" || ((fails++))
assert_contains "$out" "mdview"   "mdview version banner" || ((fails++))
out=$("$MDVIEW" --help 2>&1); rc=$?
assert_exit_code 0 "$rc"          "mdview --help → 0" || ((fails++))
assert_contains "$out" "pandoc"   "mdview help mentions pandoc" || ((fails++))
assert_contains "$out" "theme"    "mdview help documents theme support" || ((fails++))

# mdview data files present alongside script (dev mode)
for f in "$BIN_DIR/mdview.conf" "$BIN_DIR/rewrite-md-links.lua"; do
  [[ -f $f ]] \
    && printf '%s✓%s %s present\n' "$GREEN" "$NC" "${f##*/}" \
    || { printf '%s✗%s %s missing\n' "$RED" "$NC" "${f##*/}"; ((fails++)); }
done

# At least one default theme present (CSS + .theme pair)
theme_dir="$BIN_DIR/themes"
if [[ -d $theme_dir ]]; then
  shopt -s nullglob
  css_files=("$theme_dir"/*.css)
  theme_files=("$theme_dir"/*.theme)
  shopt -u nullglob
  if (( ${#css_files[@]} >= 1 && ${#theme_files[@]} >= 1 )); then
    printf '%s✓%s themes/ has CSS + .theme pairs (%d themes)\n' "$GREEN" "$NC" "${#css_files[@]}"
  else
    printf '%s✗%s themes/ missing CSS or .theme files\n' "$RED" "$NC"
    ((fails++))
  fi
  # github-dark is the documented default
  if [[ -f $theme_dir/github-dark.css && -f $theme_dir/github-dark.theme ]]; then
    printf '%s✓%s github-dark theme pair present (default)\n' "$GREEN" "$NC"
  else
    printf '%s✗%s github-dark theme pair missing (default fails to load)\n' "$RED" "$NC"
    ((fails++))
  fi
else
  printf '%s✗%s themes/ directory missing\n' "$RED" "$NC"
  ((fails++))
fi

# ----- bash completion file syntax -----
comp="$BIN_DIR/md2ansi.bash_completion"
if [[ -f $comp ]]; then
  if bash -n "$comp" 2>/dev/null; then
    printf '%s✓%s bash completion file bash -n clean\n' "$GREEN" "$NC"
  else
    printf '%s✗%s bash completion file bash -n failed\n' "$RED" "$NC"
    ((fails++))
  fi
  # Confirm it registers all five commands
  for cmd in md2ansi md mdview md-link-extract ansi-info; do
    if grep -qE "complete -F [_a-zA-Z0-9]+[[:space:]]+${cmd}\$" "$comp"; then
      printf '%s✓%s completion registers %s\n' "$GREEN" "$NC" "$cmd"
    else
      printf '%s✗%s completion missing %s\n' "$RED" "$NC" "$cmd"
      ((fails++))
    fi
  done
else
  printf '%s✗%s md2ansi.bash_completion missing\n' "$RED" "$NC"
  ((fails++))
fi

((fails == 0)) && { echo "All companion tests passed."; exit 0; }
echo "$fails companion tests failed."
exit 1
#fin
