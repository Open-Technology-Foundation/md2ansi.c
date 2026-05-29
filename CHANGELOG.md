# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.0.4] - 2026-05-29

Documentation and release-tooling only; the rendered binary is byte-identical
to 1.0.3 apart from the embedded version string.

### Added

- `make release-notes` target: prints a version's `CHANGELOG.md` body for
  `gh release create --notes-file`, bounded on the next version heading, the
  link-reference footer, or `#fin` (correct even for the last section).

### Changed

- Documentation: added a Releasing section describing the CHANGELOG-driven
  release flow; refreshed project-size figures (now approximate, with pointers
  to `make stats`) and corrected stale test-count references.

## [1.0.3] - 2026-05-29

Robustness and maintainability hardening from a full code review. No CLI-surface
change; rendered output is byte-identical to 1.0.2.

### Changed

- `md_slurp_file`: read via `open` + `fstat` + `fdopen` on a single descriptor,
  closing the `stat`/`access`/`fopen` TOCTOU window (exit codes 3/4/13 preserved).
- De-duplicated the CSI/OSC escape walker into a single shared `md_ansi_skip()`
  used by all four strip/width routines (net source reduction).
- `NO_COLOR`: now follows the [no-color.org](https://no-color.org/) convention —
  any presence, including an empty value, disables colour (matches the man page).
- `COLUMNS`: parsed with `strtol` + range/tail validation instead of `atoi`.
- Makefile: optimization level is now the user-tunable `OPT` (default `-O2`);
  mandatory flags are force-appended via `override CFLAGS +=`, so
  `make CFLAGS=...` can no longer drop `-D_XOPEN_SOURCE=700`.
- README: project-size figures are now approximate and reference `make stats`.
- `mdview`: shebang normalised to `#!/usr/bin/env bash`.

### Added

- `md_ansi_skip()` additionally handles DCS/APC/PM/SOS string controls.
- `make stats` target reporting source/test LOC and binary size.

### Fixed

- `md_buf_reserve`: clamp capacity doubling at `SIZE_MAX/2` to avoid a
  theoretical overflow; fall back to the exact required size.
- `md_strip_ansi_inplace`: re-terminate `buf[out]` after stripping.
- UTF-8 decoder: reject overlong encodings and codepoints above `U+10FFFF`
  (emit `U+FFFD`).

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

[Unreleased]: https://github.com/Open-Technology-Foundation/md2ansi.c/compare/v1.0.4...HEAD
[1.0.4]: https://github.com/Open-Technology-Foundation/md2ansi.c/compare/v1.0.3...v1.0.4
[1.0.3]: https://github.com/Open-Technology-Foundation/md2ansi.c/compare/v1.0.2...v1.0.3
[1.0.2]: https://github.com/Open-Technology-Foundation/md2ansi.c/compare/v1.0.1...v1.0.2
[1.0.1]: https://github.com/Open-Technology-Foundation/md2ansi.c/releases/tag/v1.0.1

#fin
