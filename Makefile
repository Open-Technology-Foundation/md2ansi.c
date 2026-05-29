# Makefile - C reimplementation of md2ansi
CC      ?= cc
# Optimization level is user-tunable (e.g. `make OPT=-O3`). The remaining flags
# are mandatory and force-appended via `override`, so `make CFLAGS=...` can add
# flags without dropping -D_XOPEN_SOURCE=700 (which would break -Werror on
# wcwidth/strcasecmp via implicit-declaration warnings).
OPT     ?= -O2
override CFLAGS += $(OPT) -std=c11 -Wall -Wextra -Wpedantic -Werror -D_XOPEN_SOURCE=700

# Install tier: root -> system-wide; non-root -> user-local.
# Override either explicitly:  make PREFIX=/opt/local install
ifeq ($(shell id -u),0)
  PREFIX ?= /usr/local
else
  PREFIX ?= $(HOME)/.local
endif

BINDIR  ?= $(PREFIX)/bin
MANDIR  ?= $(PREFIX)/share/man/man1
COMPDIR ?= $(PREFIX)/share/bash-completion/completions
DATADIR ?= $(PREFIX)/share/mdview
DESTDIR ?=

# Version used by `release-notes` (defaults to the current MD_VERSION marker).
RELEASE_VERSION ?= $(shell grep -oE '[0-9]+\.[0-9]+\.[0-9]+' $(srcdir)md_common.h | head -1)

# Directory of this Makefile (trailing slash). Anchors install-recipe sources
# so they cannot resolve against the invoking CWD (or a like-named file in a
# parent directory). Does NOT make off-CWD `make -f ... install` fully work
# on its own -- the build-rule prerequisites are still bare names.
srcdir := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

BIN          := md2ansi
COMPANIONS   := md mdview md-link-extract ansi-info
DATAFILES    := mdview.conf rewrite-md-links.lua
THEMES_CSS   := $(wildcard $(srcdir)themes/*.css)
THEMES_THEME := $(wildcard $(srcdir)themes/*.theme)
OBJS         := md_common.o ansi.o unicode.o parser.o inline.o render.o table.o syntax.o
HDRS         := md_common.h ansi.h unicode.h parser.h inline.h render.h table.h syntax.h

.PHONY: all install install-bin install-companions install-man install-comp install-data install-themes \
        uninstall check test clean stats release-notes help

all: $(BIN)

$(OBJS): %.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN): md2ansi.c $(HDRS) $(OBJS)
	$(CC) $(CFLAGS) -o $@ md2ansi.c $(OBJS)

install: install-bin install-companions install-man install-comp install-data install-themes
	@if [ -z "$(DESTDIR)" ]; then $(MAKE) --no-print-directory check; fi

install-bin: $(BIN)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(srcdir)$(BIN) $(DESTDIR)$(BINDIR)/$(BIN)

install-companions:
	install -d $(DESTDIR)$(BINDIR)
	@for f in $(COMPANIONS); do \
	  echo "install -m 755 $(srcdir)$$f $(DESTDIR)$(BINDIR)/$$f"; \
	  install -m 755 $(srcdir)$$f $(DESTDIR)$(BINDIR)/$$f; \
	done

install-man:
	install -d $(DESTDIR)$(MANDIR)
	install -m 644 $(srcdir)$(BIN).1 $(DESTDIR)$(MANDIR)/$(BIN).1

install-comp:
	install -d $(DESTDIR)$(COMPDIR)
	install -m 644 $(srcdir)$(BIN).bash_completion $(DESTDIR)$(COMPDIR)/$(BIN)

install-data:
	install -d $(DESTDIR)$(DATADIR)
	install -m 644 $(srcdir)mdview.conf           $(DESTDIR)$(DATADIR)/mdview.conf
	install -m 644 $(srcdir)rewrite-md-links.lua  $(DESTDIR)$(DATADIR)/rewrite-md-links.lua

install-themes:
	install -d $(DESTDIR)$(DATADIR)/themes
	@for f in $(THEMES_CSS) $(THEMES_THEME); do \
	  echo "install -m 644 $$f $(DESTDIR)$(DATADIR)/themes/"; \
	  install -m 644 $$f $(DESTDIR)$(DATADIR)/themes/; \
	done

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(BIN)
	@for f in $(COMPANIONS); do rm -f $(DESTDIR)$(BINDIR)/$$f; done
	rm -f $(DESTDIR)$(MANDIR)/$(BIN).1
	rm -f $(DESTDIR)$(COMPDIR)/$(BIN)
	rm -f $(DESTDIR)$(DATADIR)/mdview.conf
	rm -f $(DESTDIR)$(DATADIR)/rewrite-md-links.lua
	rm -rf $(DESTDIR)$(DATADIR)/themes
	-rmdir $(DESTDIR)$(DATADIR) 2>/dev/null || true

check:
	@command -v $(BIN) >/dev/null 2>&1 \
	  && echo '$(BIN): OK' \
	  || echo '$(BIN): NOT FOUND (check PATH)'
	@for f in $(COMPANIONS); do \
	  command -v $$f >/dev/null 2>&1 \
	    && echo "$$f: OK" \
	    || echo "$$f: NOT FOUND (check PATH)"; \
	done

test: $(BIN)
	./tests/run-tests.sh

clean:
	rm -f $(BIN) $(OBJS)

stats:
	@printf 'Source code: %s lines (C + headers)\n' "$$(cat $(srcdir)*.c $(srcdir)*.h | wc -l)"
	@printf 'Test suite:  %s lines across %s test files (+ runner + utils)\n' \
	  "$$(cat $(srcdir)tests/*.sh | wc -l)" "$$(ls $(srcdir)tests/test-*.sh | wc -l)"
	@printf 'Binary:      %s bytes\n' \
	  "$$(wc -c < $(srcdir)$(BIN) 2>/dev/null || echo '? (run make first)')"

# Print the CHANGELOG body for RELEASE_VERSION, stripped of the link-reference
# footer and #fin marker, for `gh release create --notes-file`. Bounds the
# section on the next version heading, a link-ref line, OR #fin -- so it stays
# correct even when the requested version is the LAST section in the file.
#   make release-notes                       # current version
#   make release-notes RELEASE_VERSION=1.0.1 # a specific version
release-notes:
	@awk -v v="$(RELEASE_VERSION)" ' \
	  $$0 ~ "^## \\[" v "\\]" { f=1; next } \
	  /^## \[/ || /^\[[^]]+\]:/ || /^#fin/ { f=0 } \
	  f { buf = buf $$0 "\n" } \
	  END { \
	    sub(/^\n+/, "", buf); sub(/\n+$$/, "", buf); \
	    if (buf == "") { print "release-notes: no CHANGELOG section for " v > "/dev/stderr"; exit 1 } \
	    print buf \
	  }' $(srcdir)CHANGELOG.md

help:
	@echo 'Usage: make [target]'
	@echo ''
	@echo 'Targets:'
	@echo '  all                Build $(BIN) (default)'
	@echo '  install            Install everything (bin + companions + man + comp + data + themes)'
	@echo '  install-bin        Install $(BIN) only'
	@echo '  install-companions Install md, mdview, md-link-extract, ansi-info'
	@echo '  install-man        Install manual page'
	@echo '  install-comp       Install bash completion'
	@echo '  install-data       Install mdview.conf + rewrite-md-links.lua'
	@echo '  install-themes     Install mdview themes (CSS + .theme)'
	@echo '  uninstall          Remove all installed files'
	@echo '  check              Verify installed binaries are on PATH'
	@echo '  test               Run test suite'
	@echo '  clean              Remove build artifacts'
	@echo '  stats              Print source/test LOC + binary size'
	@echo '  release-notes      Print CHANGELOG body for RELEASE_VERSION'
	@echo '  help               Show this message'
	@echo ''
	@echo 'Variables:'
	@echo '  OPT      = $(OPT)'
	@echo '  PREFIX   = $(PREFIX)'
	@echo '  BINDIR   = $(BINDIR)'
	@echo '  MANDIR   = $(MANDIR)'
	@echo '  COMPDIR  = $(COMPDIR)'
	@echo '  DATADIR  = $(DATADIR)'
	@echo '  DESTDIR  = $(DESTDIR)'
	@echo '  CC       = $(CC)'
	@echo '  CFLAGS   = $(CFLAGS)'
	@echo '  RELEASE_VERSION = $(RELEASE_VERSION)'
