# MD2ANSI (C Implementation)

![Version](https://img.shields.io/badge/version-1.0.1-blue.svg)
![License](https://img.shields.io/badge/license-GPL--3.0-green.svg)
![Language](https://img.shields.io/badge/C-C11-orange.svg)
![Status](https://img.shields.io/badge/status-stable-brightgreen.svg)

A **zero-dependency C11 implementation** that converts Markdown to ANSI-colored terminal output. Renders headers, lists, tables, code blocks with syntax highlighting, links, footnotes, and inline formatting directly to a TTY with correct Unicode width measurement and predictable performance.

## Overview

A pure C11 implementation of a Markdown → ANSI terminal renderer. The binary is statically self-contained (libc only), ~54KB, and renders a 39KB README in milliseconds.

**Key Features:**

- ✓ Zero runtime dependencies (libc only — no ncurses, no regex)
- ✓ Hand-rolled scanners — no `<regex.h>`, no ReDoS surface
- ✓ Correct Unicode width via `wcwidth` (CJK = 2 cols, combining marks = 0)
- ✓ Stable CLI surface: short + long flags, deterministic exit codes
- ✓ Indented code, reference links, autolinks, backslash escapes, YAML frontmatter strip
- ✓ `NO_COLOR` env honoured, `--color=auto|always|never` flag
- ✓ Syntax highlighting for Python, JavaScript, Bash
- ✓ Unicode box-drawing borders for tables (`─ │ ┌ ┐ └ ┘ ├ ┤ ┬ ┴ ┼`)
- ✓ Pipe-escape support in tables (`\|` and `` `code with | inside` ``)
- ✓ Bundled companion tools (`md` pager, `mdview` browser preview, `md-link-extract`, `ansi-info`)

## Quick Start

### One-Liner Install

```bash
git clone https://github.com/Open-Technology-Foundation/md2ansi.c.git && cd md2ansi.c && make && sudo make install
```

Unprivileged install to `~/.local` (no `sudo`):

```bash
git clone https://github.com/Open-Technology-Foundation/md2ansi.c.git && cd md2ansi.c && make && make install PREFIX=$HOME/.local
```

### Build & Install (step-by-step)

```bash
git clone https://github.com/Open-Technology-Foundation/md2ansi.c.git
cd md2ansi.c
make
sudo make install         # installs binary + companions + man + completion + mdview data
```

`make install` puts the C binary, the bundled bash companions (`md`, `mdview`, `md-link-extract`, `ansi-info`), the manual page, the bash completion file, and the `mdview` data files (themes + `mdview.conf` + `rewrite-md-links.lua`) under `$(PREFIX)`:

| Component | Destination |
|:----------|:------------|
| Binaries | `$(BINDIR)` (default `$(PREFIX)/bin`) |
| Manual page | `$(MANDIR)` (default `$(PREFIX)/share/man/man1`) |
| Bash completion | `$(COMPDIR)` (default `$(PREFIX)/share/bash-completion/completions`) |
| mdview data | `$(DATADIR)` (default `$(PREFIX)/share/mdview`) |

The Makefile auto-selects `PREFIX=/usr/local` for root installs and `PREFIX=$HOME/.local` for unprivileged installs. Override explicitly with `make PREFIX=/opt/local install`. Granular subtargets (`install-bin`, `install-companions`, `install-man`, `install-comp`, `install-data`, `install-themes`) are available if you want to install only one slice.

Requires GCC or Clang with libc 2.17+ (`wcwidth` support). Builds clean under `-O2 -std=c11 -Wall -Wextra -Wpedantic -Werror`. The companion scripts additionally require **bash 5.2+**; `mdview` also requires **pandoc** and a browser (auto-detected: `google-chrome`, `chromium-browser`, `chromium`, then `xdg-open`).

```bash
md2ansi -V             # md2ansi 1.0.1
md2ansi README.md
md README.md           # paged
mdview README.md       # browser preview
```

### Uninstall

```bash
sudo make uninstall
```

### Basic Usage

```bash
# View a markdown file
md2ansi README.md

# Paginated viewing
md2ansi README.md | less -R

# Process from stdin
cat README.md | md2ansi
echo "# Hello **World**" | md2ansi

# Multiple files
md2ansi file1.md file2.md file3.md

# Force terminal width (auto-detected by default)
md2ansi --width 100 README.md

# Plain mode (disable all feature toggles)
md2ansi --plain README.md

# Force colors regardless of TTY / NO_COLOR
md2ansi --color=always README.md > /tmp/colored.txt
```

### Common Use Cases

```bash
# Read documentation in color, paginated (md is the bundled wrapper)
md README.md

# Read non-markdown docs in colour
md2ansi /usr/share/doc/bash/README | less -R

# Preview your markdown before committing
md2ansi CHANGELOG.md | less -R

# Process while honouring NO_COLOR
NO_COLOR=1 md2ansi README.md > plain.txt

# Browser-based preview with theming
mdview README.md

# Extract every link from a doc set
md-link-extract docs/*.md
```

## Companion Tools

Bundled alongside the C `md2ansi` binary:

| Tool | Language | Purpose |
|:-----|:---------|:--------|
| `md` | bash | Pager wrapper. Delegates to `md2ansi` and pipes through `less -FXRS`. |
| `mdview` | bash + pandoc | Browser-based preview. CSS-themed HTML rendering; rewrites local `.md` links to `.html` via a Lua filter. Default theme `github-dark`; built-in `github-light`. |
| `md-link-extract` | bash | Extract every link from one or more markdown files. Strips UTM tracking params. |
| `ansi-info` | bash | Display 256-colour palette, SGR attributes, truecolor ramps, and terminal capability info. |

Installed by `make install`. Data files for `mdview` (theme CSS + `.theme` pandoc syntax styles, `mdview.conf`, `rewrite-md-links.lua`) live under `$(DATADIR)` (default `$(PREFIX)/share/mdview`). Users can override system themes by dropping files into `$XDG_DATA_HOME/mdview/themes/`.

## Features

### Fully Implemented

**Headers** (H1–H6) with a 256-colour gradient (yellow → orange → green → blue → purple → dark gray). Inline formatting works inside header text.

**Inline Formatting:**

- ✓ Bold (`**text**`)
- ✓ Italic (`*text*`) — neighbour-char guard to avoid `snake_case` clashes
- ✓ Combined bold+italic (`***text***`, `**_text_**`, `_**text**_`)
- ✓ Strikethrough (`~~text~~`)
- ✓ Inline code (`` `code` ``) — suppresses other formatting inside
- ✓ Inline links (`[text](url)`) with underline
- ✓ Images (`![alt](url)`) shown as `[IMG: alt]` placeholders
- ✓ Reference links (`[text][ref]` + `[ref]: url`)
- ✓ Shortcut reference links (`[ref]` matches def)
- ✓ Angle autolinks (`<http://...>`, `<mailto:...>`, `<ftp://...>`)
- ✓ Bare URL autolinks (`https://...` in text) — default ON, disable with `--no-bare-urls`
- ✓ Backslash escapes (`\*`, `\_`, `\\`, `\#`, `\[`, `\]`, `\(`, `\)`, `\{`, `\}`, `\!`, `\.`, `\+`, `\-`, `\>`, `\~`, `` \` ``)
- ✓ Footnote references (`[^id]`)

**Lists:**

- ✓ Unordered lists (`-`, `*`)
- ✓ Ordered lists (`1.`, `2.`, …)
- ✓ Task lists (`- [ ]`, `- [x]`)
- ✓ Nested lists (every 2 spaces = one level)
- ✓ Inline formatting within list items, with width-aware text wrapping

**Tables:**

- ✓ GFM pipe-delimited tables
- ✓ Left, center, right alignment (`:---`, `:---:`, `---:`)
- ✓ Inline formatting in cells (bold, italic, code, links)
- ✓ Unicode-aware column width (CJK chars = 2 cols)
- ✓ Box-drawing borders (`─ │ ┌ ┐ └ ┘ ├ ┤ ┬ ┴ ┼`)
- ✓ Backslash-escaped pipes (`\|`) preserved as literal content
- ✓ Backtick-wrapped pipes (`` `--mode=a|b|c` ``) preserved as literal content

**Code Blocks:**

- ✓ Fenced code blocks (` ``` ` and `~~~`)
- ✓ Indented code blocks (4-space or 1-tab prefix)
- ✓ Syntax highlighting for `python`, `javascript`, `bash` (with aliases `py`, `js`, `sh`, `shell`)
- ✓ Comment detection (whole-line gray) for all three languages
- ✓ Python docstring detection (`'''` and `"""` → green)
- ✓ Mismatched fence inside code block rendered as content

**Additional Elements:**

- ✓ Blockquotes (`>`)
- ✓ Horizontal rules (`---`, `===`, `___`) with Unicode `─` chars
- ✓ Footnote definitions (`[^id]: text`)
- ✓ Footnote section auto-rendered at end of document
- ✓ Missing footnote definition shown in italic
- ✓ YAML frontmatter (`---\n…\n---` or `---\n…\n...`) stripped
- ✓ Shebang line stripped on file ingest

**Advanced Features:**

- ✓ ANSI-aware text wrapping
- ✓ Terminal width auto-detection via `TIOCGWINSZ` → `$COLUMNS` → `80`, clamped `[20, 500]`
- ✓ Per-feature toggles (`--no-tables`, `--no-syntax-highlight`, `--no-tasks`, `--no-images`, `--no-links`, `--no-footnotes`, `--no-bare-urls`)
- ✓ Plain mode (`--plain` / `-t`) disables all feature toggles
- ✓ Debug mode (`--debug` / `-D`) with timestamped sequential traces to stderr

**Security:**

- ✓ File size limit (10MB)
- ✓ Stdin size limit (10MB)
- ✓ ANSI escape sequence stripping on all input
- ✓ Hand-rolled scanners — no regex DoS surface
- ✓ Directory rejection with explicit exit code 4

## Command-Line Options

### Core Options

```bash
md2ansi -h, --help              # show help
md2ansi -V, --version           # show version
md2ansi -D, --debug             # enable debug traces (to stderr)
md2ansi -w, --width WIDTH       # force width (20–500)
md2ansi -t, --plain             # disable all feature toggles
md2ansi --color=MODE            # auto (default) | always | never
```

### Feature Toggles

```bash
md2ansi --no-footnotes          # don't render footnotes section
md2ansi --no-syntax-highlight   # disable keyword colouring in code
md2ansi --no-tables             # render tables as plain text
md2ansi --no-task-lists         # render task items as regular bullets
md2ansi --no-images             # show literal ![alt](url)
md2ansi --no-links              # show literal [text](url)
md2ansi --no-bare-urls          # don't auto-link bare http(s):// URLs
```

### Exit Codes

| Code | Meaning |
|:----:|:--------|
| `0` | Success |
| `3` | File not found |
| `4` | Path is a directory |
| `8` | Missing argument for option |
| `9` | Input exceeds 10MB |
| `13` | Cannot read file (permission denied) |
| `22` | Invalid option / argument |

## Expanded Examples

### Headers and Formatting

**Input:**

```markdown
# Main Title
## Subtitle
### Section Header

This is **bold**, *italic*, and ***bold italic*** text.
This is ~~strikethrough~~ and `inline code`.

Visit [GitHub](https://github.com) or just https://github.com directly.

Footnote with citation[^1].

[^1]: Source: Documentation, 2026
```

**Command:**

```bash
md2ansi sample.md
```

**Result:** Headers colour-graded H1→H6; inline spans formatted; both `[GitHub](url)` and the bare URL render as underlined cyan links; footnote `[^1]` shown with dim bold marker and definition rendered in a "Footnotes:" section at end.

### Reference Links + Autolinks

```markdown
See the [project home][home] for details. Also <https://example.com>.

Shortcut form: [home] matches the def too.

[home]: https://example.com/project "Project home"
```

All three forms render as cyan-underlined links.

### Tables with Alignment + CJK

```markdown
| Feature | Status | 言語 |
|:--------|:------:|----:|
| Headers | ✓      | 英語 |
| Tables  | ✓      | 日本語 |
| Code    | ✓      | 中文 |
```

Column widths are computed with `wcwidth`, so `日本語` (6 visible cols) aligns properly against `Headers` (7 cols).

### Pipe Escapes in Tables

```markdown
| CLI option | Notes |
|:-----------|:------|
| `--color=auto\|always\|never` | backticked pipes preserved |
| `\|literal\|` | escaped pipes outside backticks |
```

Both backtick-wrapped pipes and `\|` escapes are preserved as literal content within a single cell.

### Code Blocks with Syntax Highlighting

````markdown
```python
def fibonacci(n):
    """Compute the n-th Fibonacci number."""
    if n <= 1:
        return n
    return fibonacci(n-1) + fibonacci(n-2)
```
````

Keywords (`def`, `return`, `if`) coloured pink. Docstring line coloured green. Comments (whole-line `#…`) coloured gray.

### Indented Code Block

```markdown
normal paragraph text

    this is a code block
    no fence needed
    4 spaces of indent

more text
```

The indented section is recognised as a `MD_B_CODE_INDENTED` block.

### Backslash Escapes

```markdown
Escape literally: \*not italic\* and \[not a link\] and \\backslash.
```

The 17-char CommonMark backslash-escape set is honoured.

### YAML Frontmatter Strip

```markdown
---
title: My Document
author: Gary Dean
date: 2026-05-13
---

# Real Content Starts Here
```

The frontmatter block (between `---` lines, terminated by `---` or `...`) is silently dropped.

## Rendering Behaviour

| Item | Behaviour |
|:-----|:----------|
| Table borders | Unicode box-drawing `─ │ ┌ ┐ └ ┘ ├ ┤ ┬ ┴ ┼` |
| Table chrome color | Three explicit levels: cell content `38;5;7` > inline `code` `90` > chrome `38;5;238` (borders/padding) |
| Horizontal rules | Rendered with `─` chars |
| Bold/italic inside `` `code` `` | Suppressed (literal) inside code spans |
| Trailing `\033[0m` | One reset at end of stream |
| Indented code blocks | Recognised as `MD_B_CODE_INDENTED` |
| Reference links / autolinks / bare URLs | Rendered as cyan-underlined links |
| Backslash escapes | Honoured (17-char CommonMark set) |
| YAML frontmatter | Silently stripped |
| CJK column width | 2 cols per char via `wcwidth` |
| ANSI in input | Stripped everywhere before parsing |

## Architecture

### Design Philosophy

Modular, not monolithic. Each markdown subsystem lives in its own translation unit, with public headers that declare types and function signatures. This shape favours hand-modification and per-module testing across ~3K lines of C.

**Design Benefits:**

| Benefit | Description |
|:--------|:------------|
| **Modular split** | Each feature isolated in its own `.c`/`.h` pair — easy to extend |
| **Single binary deploy** | Statically structured, libc-only — copy one 54KB file to PATH |
| **Hand-rolled scanners** | No `<regex.h>` — eliminates ReDoS surface, predictable perf |
| **`-Werror` clean** | Zero warnings under `-Wall -Wextra -Wpedantic -Werror` |
| **Type-safe core** | `md_buf_t`, `md_lines_t`, `md_block_t`, `md_doc_t` give consistent ownership rules |

### Project Structure

```
md2ansi.c/
├── md2ansi.c                # main entry, argv parsing, file loop
├── md_common.h/c            # die/warn/debug, buffers, line splitter, slurp, ANSI strip
├── ansi.h/c                 # palette, terminal detect, color decision, --color mode
├── unicode.h/c              # UTF-8 decode, wcwidth wrapper, visible-width measurement
├── parser.h/c               # line classifier → block list, link-ref + footnote-def tables
├── inline.h/c               # inline span scanner (escape/code/link/bold/italic/strike/...)
├── render.h/c               # block dispatch, per-kind renderers, word-wrap, footnotes
├── table.h/c                # GFM pipe-table parser + box-drawing renderer
├── syntax.h/c               # python/javascript/bash keyword + comment highlighters
├── md                       # bash pager wrapper (pipes md2ansi through less)
├── mdview                   # bash browser-preview wrapper (pandoc + themes)
├── md-link-extract          # bash link extractor
├── ansi-info                # bash 256-colour palette + terminal capability report
├── mdview.conf              # default config for mdview
├── rewrite-md-links.lua     # pandoc Lua filter — .md → .html for inter-file links
├── themes/                  # mdview themes (github-dark, github-light: .css + .theme)
├── md2ansi.1                # roff manual page
├── md2ansi.bash_completion  # completion for md2ansi + md + mdview + md-link-extract + ansi-info
├── .symlink                 # dev-machine PATH-exposure manifest (used by `symlink -S`)
├── Makefile                 # build/install/uninstall/test/clean targets
├── README.md                # this file
├── LICENSE                  # GPL-3.0 full text
├── .gitignore
└── tests/
    ├── run-tests.sh         # aggregator
    ├── utils.sh             # assertion helpers
    ├── test-basic.sh        # headers, inline formatting, HR
    ├── test-blockquotes.sh  # > quote rendering
    ├── test-code.sh         # fenced + indented + syntax highlight
    ├── test-lists.sh        # ul, ol, task, nested, mixed
    ├── test-tables.sh       # pipes, alignment, CJK, escape
    ├── test-footnotes.sh    # def + ref + missing
    ├── test-options.sh      # CLI flags + exit codes
    ├── test-security.sh     # ANSI injection, size limits, fuzz
    ├── test-wrapping.sh     # width-based word wrap
    ├── test-unicode.sh      # UTF-8 + CJK width
    ├── test-links.sh        # inline / ref / autolink / image
    ├── test-escapes.sh      # backslash escapes + frontmatter
    ├── test-misc.sh         # shebang strip, multi-file, --plain, --debug, perm denied
    ├── test-companions.sh   # smoke tests for md / mdview / md-link-extract / ansi-info
    └── fixtures/            # input + expected golden files
```

**Total project size:**

- Source code: 2,952 lines (9 modules + headers + main)
- Test suite: 649 lines (11 test files + runner + utils)
- Binary: ~54KB (dynamically linked to libc only)

### Module Responsibilities

| Module | Responsibility |
|:-------|:---------------|
| `md_common` | Messaging (`md_die`/`md_warn`/`md_debugf`), growable `md_buf_t`, line splitter `md_lines_t`, file/stdin slurpers with 10MB cap, in-place ANSI strip |
| `ansi` | 256-colour palette as `md_palette_t`, terminal width via `TIOCGWINSZ`, color decision honouring `NO_COLOR` + `TERM` + isatty, ANSI sequence parser |
| `unicode` | UTF-8 codepoint decode, `wcwidth` wrapper, visible-width measurement that skips ANSI + counts CJK correctly |
| `parser` | Line-by-line classifier (header / HR / list / blockquote / code fence / table / footnote def / link ref def / paragraph), produces `md_doc_t` block list |
| `inline` | Single-pass token scanner for inline spans: escape, code, image, footnote ref, inline link, ref-link, shortcut ref, autolink, bare URL, 3-/2-/1-star, strike |
| `render` | Block dispatch via switch on `md_block_kind_t`, per-kind renderers, Unicode-aware word-wrap, footnotes section emit |
| `table` | Pipe-row splitter honouring `\|` and backtick context, alignment row detector, per-column visible-width calc, box-drawing border emit |
| `syntax` | Per-language keyword tables, comment detection, Python docstring heuristic, single-line tokenizer (no multi-line strings) |

### Data Flow

```
argv → md_cli_opts_t + file list
        ↓
file/stdin → md_slurp_* (10MB cap) → md_strip_ansi_inplace → md_lines_t
        ↓
parser → md_doc_t (block list + link refs + footnote defs)
        ↓
render → stdout
   ├── inline (per-line inline expansion)
   ├── table  (table block branch with inline expansion in cells)
   └── syntax (fenced code branch when --no-syntax-highlight not set and lang known)
```

### Key Data Structures

```c
typedef struct { char *data; size_t len, cap; } md_buf_t;

typedef struct { const char *start; size_t len; } md_line_t;
typedef struct { md_line_t *items; size_t count, cap; } md_lines_t;

typedef enum {
    MD_B_EMPTY, MD_B_PARA,
    MD_B_H1, MD_B_H2, MD_B_H3, MD_B_H4, MD_B_H5, MD_B_H6,
    MD_B_HR, MD_B_BLOCKQUOTE,
    MD_B_UL_ITEM, MD_B_OL_ITEM, MD_B_TASK_ITEM,
    MD_B_CODE_FENCED, MD_B_CODE_INDENTED, MD_B_TABLE,
    MD_B_FOOTNOTE_DEF, MD_B_FRONTMATTER
} md_block_kind_t;

typedef struct md_block {
    md_block_kind_t  kind;
    int              level;   /* header level OR list indent */
    int              ordinal; /* OL number OR fence char */
    int              checked; /* task checkbox state */
    char            *lang;    /* fenced code lang */
    md_buf_t         content; /* joined inline content */
    md_lines_t       lines;   /* raw lines (for code/table) */
    void            *extra;
    struct md_block *next;
} md_block_t;

typedef struct {
    md_block_t        *head, *tail;
    md_link_ref_t     *link_refs;
    md_footnote_def_t *footnote_defs;
    md_str_list_node_t *footnote_refs;   /* appearance order */
} md_doc_t;
```

## Testing

### Test Suite

11 per-feature test files plus a runner and shared assertion helpers:

| Test File | Coverage |
|:----------|:---------|
| **run-tests.sh** | Aggregator: runs every `test-*.sh`, summarises pass/fail |
| **utils.sh** | `assert_equals`, `assert_contains`, `assert_not_contains`, `assert_exit_code`, `assert_file_equals` |
| **test-basic.sh** | Headers H1–H6, bold, italic, strike, inline code, HR (all variants), triple-star |
| **test-code.sh** | Fenced (`` ``` `` + `~~~`), indented, python/js/bash syntax highlight, comment colouring, mismatched-fence handling, ANSI sanitisation in code |
| **test-lists.sh** | UL (`-`, `*`), OL, task lists, nested lists, `--no-task-lists` fallback |
| **test-tables.sh** | Pipe tables, alignment markers, CJK width, `--no-tables` fallback, **escaped-pipe + backticked-pipe handling** |
| **test-footnotes.sh** | Ref + def, missing def warning, appearance ordering, `--no-footnotes` |
| **test-options.sh** | `--help`/`-V`/`--bogus` exit codes, file-not-found / dir / invalid-width handling, `--color` modes, `NO_COLOR` env |
| **test-security.sh** | ANSI injection stripped from text, 10MB+1 stdin → exit 9, 10MB+1 file → exit 9, deeply nested lists, malformed fence, empty/very-long input |
| **test-wrapping.sh** | Width-based word wrap, short text emits single line, wrap respects requested width |
| **test-unicode.sh** | CJK passthrough, emoji, accented chars, CJK column-width wrap, CJK in table cells |
| **test-links.sh** | Inline `[t](url)`, image `![a](u)`, ref `[t][r]`, shortcut `[r]`, autolink `<url>`, bare URL autolink, `--no-links` / `--no-images` / `--no-bare-urls` |
| **test-escapes.sh** | `\*`, `\\`, `\#`, `\[`/`\]`, `` \` ``, YAML frontmatter strip (both `---` and `...` terminators) |

**Total test coverage:** 649 lines across 13 files.

### Running Tests

```bash
# Build the binary first
make

# Run the full suite (uses ./md2ansi from project root)
make test

# Or invoke directly
tests/run-tests.sh

# Run a single suite
bash tests/test-tables.sh
```

### Test Framework

Assertions emit colour-coded `✓ pass` / `✗ fail` lines, count failures, and exit non-zero on any failure:

```bash
assert_equals       "$actual" "$expected"       "message"
assert_contains     "$haystack" "$needle"       "message"
assert_not_contains "$haystack" "$forbidden"    "message"
assert_exit_code    $expected_code $actual_rc   "message"
assert_file_equals  "$actual_file" "$expected_file" "message"
```

## Development

### Code Standards

| Standard | Implementation |
|:---------|:---------------|
| **Language** | ISO C11 (`-std=c11`) |
| **Warnings** | `-Wall -Wextra -Wpedantic -Werror` — zero tolerance |
| **Optimisation** | `-O2` for release |
| **Feature macros** | `-D_XOPEN_SOURCE=700` (implies POSIX 2008, enables `wcwidth`, `strcasecmp`) |
| **Indentation** | 4 spaces throughout (no tabs) |
| **Naming** | `md_` prefix on all public symbols |
| **Memory** | Owned-pointer convention: `md_doc_t` owns blocks, blocks own content/lines/lang |
| **Error handling** | `md_die(exit_code, fmt, …)` — never returns, prefixed `✗` to stderr |
| **EOF marker** | Every `.c` and `.h` ends with `/*fin*/` |
| **No regex** | All scanners hand-rolled to eliminate ReDoS surface |

### Building

```bash
make            # build md2ansi binary
make test       # run full test suite
make clean      # remove binary + object files
make install    # install to /usr/local/bin (requires sudo for system dirs)
make uninstall  # remove from /usr/local/bin
make help       # show targets + active variables
```

### Adding New Features

**To add inline formatting** (e.g. underscore italics):

Edit `inline.c` and add a new branch in `md_inline_expand`'s scanner loop. Use `find_*_close` helpers as a pattern. Update `is_escape_char` if the new token uses a special character.

**To add block-level elements** (e.g. definition lists):

1. Add new enum value to `md_block_kind_t` in `parser.h`.
2. Write a `match_*` helper in `parser.c` and add to the main `md_parse` classifier loop.
3. Add a `render_*` function in `render.c` and a case in the `switch` inside `md_render`.

**To add syntax highlighting for a new language:**

1. Define a keyword table (`static const char *MYLANG_KEYWORDS[] = { ... };`) at the top of `syntax.c`.
2. Add the language name to `normalize_lang` if aliases are needed.
3. Add an `else if (strcmp(l, "mylang") == 0)` branch in `md_syntax_highlight` that calls `highlight_with_keywords` (or a custom highlighter for special tokens).
4. Add `is_comment_line` handling for the language's comment syntax.

### Debug Mode

```bash
md2ansi -D file.md 2>debug.log
```

Each debug message gets a timestamped sequence number:

```
[14:23:01.1] ⦿ width=80 color=1 files=1
```

The counter increments per message, so trace order is unambiguous even when piped.

### Code Review Checklist

Before submitting changes:

- [ ] Compiles clean under `-Wall -Wextra -Wpedantic -Werror`
- [ ] No new heap allocations without a corresponding free path
- [ ] All `.c`/`.h` files end with `/*fin*/`
- [ ] New scanners are hand-rolled (no `<regex.h>`)
- [ ] New CLI flags documented in `show_help` AND README
- [ ] Tests cover new features (assertion + edge cases)
- [ ] `make test` passes 11/11

## Performance

### Throughput

A 39KB / ~1217-line README renders in ~2ms on a modern desktop CPU. The pipeline is a single in-process pass: zero subprocess spawning, hand-rolled scanners, in-process buffer reuse.

### Memory Usage

- **Static**: ~54KB binary, ~2MB resident on libc startup
- **Dynamic**: O(file size) — one slurp buffer per file, plus parsed `md_doc_t` block list. A 10MB markdown file uses roughly 25–30MB peak RSS

### Throughput Characteristics

- **Startup**: <1ms (one libc relocation + initial palette setup)
- **Throughput**: ~5MB/sec on a typical desktop CPU
- **File size limit**: 10MB (configurable in `md_common.h` via `MD_MAX_FILE_SIZE`)
- **Terminal width**: 20–500 columns

## Security Features

| Feature | Implementation | Limit/Behavior |
|:--------|:---------------|:---------------|
| **File size limit** | `stat()` + buffered slurp | 10MB max |
| **Stdin size limit** | Doubling buffer with cap check | 10MB max, fails fast with exit 9 |
| **Input sanitisation** | `md_strip_ansi_inplace` on slurped buffer | All input cleaned (not just code blocks) |
| **No regex DoS** | Hand-rolled scanners only | Linear time, bounded memory |
| **Bounds checking** | Width clamped to `[20, 500]` | Invalid → exit 22 |
| **Directory protection** | Explicit `S_ISDIR` check | Rejects with exit 4 |
| **Permission checks** | `access(R_OK)` before open | Exit 13 with clear message |
| **No shell-out** | Pure libc, no `system()` calls | No shell injection surface |

### Security Audit Notes

- **No external regex engine** — hand-rolled scanners are linear-time, no catastrophic backtracking
- **No `printf` format-string injection** — all user content passes through `fputs` / `fwrite`, never `printf`
- **No `eval`/dynamic execution** — pure parsing pipeline
- **No tempfile creation** — entire pipeline is in-memory

## FAQ

### Compatibility

**Q: Does it handle GitHub Flavored Markdown?**

A: All commonly-used GFM features: tables (with alignment + pipe escapes), task lists, strikethrough, fenced code blocks, autolinks. Emoji shortcodes and HTML passthrough are not implemented.

### Build / Install

**Q: Does it work on macOS / BSD?**

A: It should — only requires libc with `wcwidth`. Tested on Linux (glibc 2.39); macOS (libSystem) and BSD (libc) should work but haven't been verified.

**Q: Can I install without root?**

A: Yes — `make install PREFIX=$HOME/.local` puts the binary in `~/.local/bin/md2ansi`. Add `~/.local/bin` to your PATH.

**Q: Why does the binary need libc but nothing else?**

A: All terminal detection (`ioctl(TIOCGWINSZ)`), Unicode width (`wcwidth`), and string handling come from libc. No ncurses, no terminfo lookup, no regex.

### Features

**Q: Why are bare URL autolinks default ON?**

A: Most users expect `https://example.com` in markdown text to render as a link. Disable with `--no-bare-urls` for stricter CommonMark semantics.

**Q: Can I add colours?**

A: Yes — edit `ansi.c` and modify the `PAL_ON` palette constants. Recompile with `make`.

**Q: What about HTML passthrough?**

A: Not implemented. Raw `<tag>` content renders as literal text.

### Performance

**Q: Can it be faster?**

A: Possibly. Current bottleneck is per-line `md_buf_t` allocation in inline expansion. SIMD-accelerated scanning or arena allocators could help, but ~5MB/s throughput already exceeds the practical use case.

## Known Limitations

| Limitation | Impact | Workaround |
|:-----------|:-------|:-----------|
| **Underscore italics** (`_text_`) | Not honoured (clashes with `snake_case`) | Use `*text*` instead |
| **HTML passthrough** | Raw `<tag>` rendered as literal text | None |
| **Math** (`$...$`, `$$...$$`) | Not supported | Use a dedicated math renderer for math docs |
| **Definition lists** (`term : def`) | Not implemented | Use bullet lists instead |
| **Multi-line table cells** | Not supported | Each row must fit on one source line |
| **OSC 8 terminal hyperlinks** | Not emitted | Use a richer terminal (kitty, wezterm) with built-in URL handling |
| **Syntax highlighter multi-line strings** | Single-line tokenizer only | None |
| **Setext headers** (`====` underline) | Not implemented | Use ATX `#` headers |

## Contributing

### Workflow

```bash
# 1. Make changes to source
$EDITOR parser.c inline.c

# 2. Build (must be clean under -Werror)
make

# 3. Run the test suite (must be 11/11)
make test

# 4. Smoke test manually
echo '# hi' | ./md2ansi --color=always
./md2ansi README.md | head -30

# 5. Commit
git add <files>
git commit -m "Add feature: description"
```

### Pre-merge Requirements

- ✓ Compiles clean under `-O2 -std=c11 -Wall -Wextra -Wpedantic -Werror`
- ✓ Passes `make test` (11/11)
- ✓ New features have test coverage
- ✓ CLI changes documented in `show_help` AND this README
- ✓ No new external dependencies

## External Tools Used

| Tool / Library | Purpose |
|:---------------|:--------|
| `libc` (glibc / musl / libSystem / BSD libc) | `wcwidth`, `ioctl`, `getopt_long`, `fopen`, all string handling |

**That's it.** No ncurses, no terminfo, no regex library, no JSON parser, no UTF-8 library. Pure libc.

## License

**GPL-3.0**. See [LICENSE](LICENSE) for full text.

## Project Statistics

| Metric | Value |
|:-------|:------|
| **Source code lines** | 2,952 (9 modules + headers + main) |
| **Largest module** | `parser.c` (565 lines) |
| **Test code lines** | 649 (11 test files + runner + utils) |
| **Test suites** | 11 (all passing) |
| **Binary size** | ~54KB (dynamically linked) |
| **Runtime dependencies** | 1 (libc only) |
| **External libraries** | 0 |
| **Build time** | <2 seconds on a modern machine |
| **Languages highlighted** | 3 (python, javascript, bash) |
| **CLI flags** | 13 |
| **Exit codes** | 7 (`0`, `3`, `4`, `8`, `9`, `13`, `22`) |

---

**Status**: ✓ Production-ready C11 implementation

**Version**: 1.0.1

#fin
