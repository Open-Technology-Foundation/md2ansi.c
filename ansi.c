/* ansi.c - terminal detection, color palette, ANSI stripping */
#include "ansi.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

int          md_color_on = 0;
md_palette_t md_pal;

/* All codes are bytewise identical to the Bash md2ansi 1.0.1 palette.
 * Keep these strings in sync; the differential test depends on it. */

#define C_RESET     "\033[0m"
#define C_BOLD      "\033[1m"
#define C_DIM       "\033[2m"
#define C_ITALIC    "\033[3m"
#define C_UNDERLINE "\033[4m"
#define C_STRIKE    "\033[9m"

#define C_H1 "\033[38;5;226m"
#define C_H2 "\033[38;5;214m"
#define C_H3 "\033[38;5;118m"
#define C_H4 "\033[38;5;21m"
#define C_H5 "\033[38;5;93m"
#define C_H6 "\033[38;5;239m"

#define C_TEXT       "\033[38;5;7m"
#define C_BLOCKQUOTE "\033[48;5;236m"
#define C_CODEBLOCK  "\033[90m"
#define C_LIST       "\033[36m"
#define C_HR         "\033[36m"
#define C_TABLE      "\033[38;5;238m"
#define C_LINK       "\033[38;5;45m"

#define C_KEYWORD  "\033[38;5;204m"
#define C_STRING   "\033[38;5;114m"
#define C_NUMBER   "\033[38;5;220m"
#define C_COMMENT  "\033[38;5;245m"
#define C_FUNCTION "\033[38;5;81m"
#define C_CLASS    "\033[38;5;214m"
#define C_BUILTIN  "\033[38;5;147m"

static const md_palette_t PAL_ON = {
    .reset = C_RESET, .bold = C_BOLD, .dim = C_DIM,
    .italic = C_ITALIC, .underline = C_UNDERLINE, .strike = C_STRIKE,
    .h1 = C_H1, .h2 = C_H2, .h3 = C_H3,
    .h4 = C_H4, .h5 = C_H5, .h6 = C_H6,
    .text = C_TEXT, .blockquote = C_BLOCKQUOTE, .codeblock = C_CODEBLOCK,
    .list = C_LIST, .hr = C_HR, .table = C_TABLE, .link = C_LINK,
    .keyword = C_KEYWORD, .string = C_STRING, .number = C_NUMBER,
    .comment = C_COMMENT, .function = C_FUNCTION, .class = C_CLASS,
    .builtin = C_BUILTIN
};

static const md_palette_t PAL_OFF = {
    .reset = "", .bold = "", .dim = "",
    .italic = "", .underline = "", .strike = "",
    .h1 = "", .h2 = "", .h3 = "",
    .h4 = "", .h5 = "", .h6 = "",
    .text = "", .blockquote = "", .codeblock = "",
    .list = "", .hr = "", .table = "", .link = "",
    .keyword = "", .string = "", .number = "",
    .comment = "", .function = "", .class = "",
    .builtin = ""
};

void md_palette_init(int on) {
    md_color_on = on ? 1 : 0;
    md_pal = on ? PAL_ON : PAL_OFF;
}

int md_color_decide(md_color_mode_t mode) {
    if (mode == MD_COLOR_NEVER)  return 0;
    if (mode == MD_COLOR_ALWAYS) return 1;

    /* AUTO */
    /* NO_COLOR spec (no-color.org): ANY presence — including an empty value —
     * disables color. The Bash sibling has no NO_COLOR handling, so there is
     * no parity constraint here; follow the spec. */
    if (getenv("NO_COLOR") != NULL) return 0;

    /* Match Bash: (stdout TTY AND stderr TTY) OR (TERM set and != "dumb").
     * The TERM fallback enables color when output is piped to a color-aware
     * pager (e.g. `less -R`). */
    int both_tty = isatty(STDOUT_FILENO) && isatty(STDERR_FILENO);
    if (both_tty) return 1;

    const char *term = getenv("TERM");
    if (term && *term && strcmp(term, "dumb") != 0) return 1;

    return 0;
}

int md_term_width(void) {
    struct winsize ws;
    int w = 0;

    if (isatty(STDOUT_FILENO) && ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0
        && ws.ws_col > 0) {
        w = ws.ws_col;
    } else {
        const char *cols = getenv("COLUMNS");
        if (cols && *cols) {
            char *end;
            errno = 0;
            long v = strtol(cols, &end, 10);
            if (errno == 0 && end != cols && *end == '\0' && v > 0 && v <= 500) {
                w = (int)v;
            }
        }
    }
    if (w == 0) w = 80;
    if (w < 20)  w = 20;
    if (w > 500) w = 500;
    return w;
}

/* Strip CSI / OSC / DCS / APC / PM / SOS / two-byte ESC sequences. The escape
 * walker itself lives in md_ansi_skip() (md_common.c) so every strip + width
 * routine shares one implementation. Here we just copy non-ESC bytes through. */
void md_strip_ansi(const char *src, size_t len, md_buf_t *out) {
    size_t i = 0;
    while (i < len) {
        unsigned char c = (unsigned char)src[i];
        if (c != 0x1b) {
            md_buf_append_byte(out, (char)c);
            i++;
            continue;
        }
        i = md_ansi_skip(src, i, len);
    }
}

size_t md_visible_bytes(const char *src, size_t len) {
    size_t n = 0;
    size_t i = 0;
    while (i < len) {
        unsigned char c = (unsigned char)src[i];
        if (c != 0x1b) { n++; i++; continue; }
        i = md_ansi_skip(src, i, len);
    }
    return n;
}

/*fin*/
