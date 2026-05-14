# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.0.2] - 2026-05-14

### Changed

- Documentation: simplified README title (dropped "(C Implementation)" suffix).
- Documentation: added pointer to the pure-Bash sibling at
  [md2ansi.bash](https://github.com/Open-Technology-Foundation/md2ansi.bash).
- Documentation: Quick Start now includes a one-liner install
  (`git clone … && cd … && make && sudo make install`) plus an
  unprivileged variant using `PREFIX=$HOME/.local`.

### Added

- `CHANGELOG.md` following Keep a Changelog 1.1.0 format.

## [1.0.1] - 2026-05-14

### Added

- Initial public release of `md2ansi` C11 implementation.
- Modular architecture across 9 translation units (`md_common`, `ansi`, `unicode`,
  `parser`, `inline`, `render`, `table`, `syntax`, `md2ansi`).
- Block-level rendering: headers H1–H6, paragraphs, blockquotes, horizontal rules,
  ordered/unordered/task lists with nesting, fenced and indented code blocks,
  GFM pipe tables with alignment, footnote definitions, YAML frontmatter strip.
- Inline rendering: bold, italic, bold-italic (`***`, `**_..._**`, `_**...**_`),
  strikethrough, inline code, inline links, images as `[IMG: alt]`,
  reference links, shortcut reference links, angle autolinks, bare URL autolinks,
  footnote references, 17-char CommonMark backslash-escape set.
- Syntax highlighting for Python, JavaScript, and Bash, including comment
  detection and Python docstring colouring.
- Unicode handling via `wcwidth`: CJK chars counted as 2 columns, combining
  marks as 0, with visible-width measurement that skips ANSI escape sequences.
- Terminal-aware behaviour: width auto-detection via `TIOCGWINSZ` → `$COLUMNS`
  → fallback `80`, clamped `[20, 500]`. Honours `NO_COLOR` env and
  `--color=auto|always|never`.
- CLI flags: `-h`/`--help`, `-V`/`--version`, `-D`/`--debug`, `-w`/`--width`,
  `-t`/`--plain`, `--color`, `--no-footnotes`, `--no-syntax-highlight`,
  `--no-tables`, `--no-task-lists`, `--no-images`, `--no-links`, `--no-bare-urls`.
- Deterministic exit codes: `0`, `3`, `4`, `8`, `9`, `13`, `22`.
- Security guards: 10MB file and stdin caps, in-place ANSI strip on all input,
  directory rejection, `access(R_OK)` pre-check, no `system()` shellouts,
  hand-rolled scanners (no `<regex.h>`, no ReDoS surface).
- Bundled companion tools: `md` (pager wrapper), `mdview` (browser preview via
  pandoc + themes), `md-link-extract`, `ansi-info`.
- Distribution: Makefile with auto-tier `PREFIX` selection (root → `/usr/local`,
  user → `$HOME/.local`), granular `install-*` subtargets, man page
  (`md2ansi.1`), bash completion, `mdview` data files (themes, config, Lua filter).
- Test suite: 11 shell-based suites covering basic formatting, blockquotes, code,
  lists, tables, footnotes, options, security, wrapping, unicode, links, escapes,
  miscellaneous behaviour, and companion smoke tests.

[Unreleased]: https://github.com/Open-Technology-Foundation/md2ansi.c/compare/v1.0.2...HEAD
[1.0.2]: https://github.com/Open-Technology-Foundation/md2ansi.c/compare/v1.0.1...v1.0.2
[1.0.1]: https://github.com/Open-Technology-Foundation/md2ansi.c/releases/tag/v1.0.1

#fin
