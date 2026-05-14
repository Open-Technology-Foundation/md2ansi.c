#!/usr/bin/env bash
# md: Pager wrapper for md2ansi
# Forwards all arguments to md2ansi and pipes the rendered ANSI output
# through `less` so colour escapes survive and short docs auto-exit.
set -euo pipefail

declare -r VERSION=1.0.0
#shellcheck disable=SC2155
declare -r SCRIPT_PATH=$(realpath -- "$0")
declare -r SCRIPT_DIR=${SCRIPT_PATH%/*} SCRIPT_NAME=${SCRIPT_PATH##*/}

# Help / version - intercepted before delegating to md2ansi so the wrapper
# can identify itself; any other args (including md2ansi's own -h/-V) are
# passed through.
if (($#)); then
    if [[ $1 == '-h' || $1 == '--help' ]]; then
        cat <<HELP
$SCRIPT_NAME $VERSION - Wrapper for md2ansi that pipes output through less

Provides paginated viewing of markdown files via md2ansi.

Usage: $SCRIPT_NAME [OPTIONS] file.md
       cat file.md | $SCRIPT_NAME [OPTIONS]
       $SCRIPT_NAME --help
       $SCRIPT_NAME --version

OPTIONS are md2ansi options: see md2ansi --help

Examples:
    $SCRIPT_NAME README.md
    cat README.md | $SCRIPT_NAME --no-syntax-highlight --width 76

HELP
        exit 0
    fi

    if [[ $1 == '-V' || $1 == '--version' ]]; then
        printf '%s %s\n' "$SCRIPT_NAME" "$VERSION"
        exit 0
    fi
fi

# less flags for ANSI markdown viewing:
#   -F  quit if entire output fits on one screen
#   -X  do not send terminal init/deinit (preserves output after exit)
#   -R  pass raw ANSI colour escapes through to the terminal
#   -S  chop long lines instead of wrapping (preserves table layout)
export LESS='-FXRS'

# Delegate to the sibling md2ansi binary so dev-mode (running from the
# checkout) and installed-mode (both files in /usr/local/bin) both work.
"$SCRIPT_DIR/md2ansi" "$@" | less
#fin
