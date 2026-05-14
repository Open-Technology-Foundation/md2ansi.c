/* inline.c - inline span scanner */
#include "inline.h"
#include "ansi.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

/* ----- helpers ----- */

static int is_escape_char(char c) {
    switch (c) {
    case '\\': case '*': case '_':  case '[': case ']':
    case '(':  case ')': case '{':  case '}': case '#':
    case '!':  case '.': case '+':  case '-': case '>':
    case '~':  case '`':
        return 1;
    default:
        return 0;
    }
}

/* Find next char `c`. Returns position or SIZE_MAX. */
static size_t find_char(const char *src, size_t start, size_t len, char c) {
    for (size_t j = start; j < len; j++) if (src[j] == c) return j;
    return (size_t)-1;
}

/* Scanners for ** / *** / ~~ closes (content must be non-empty, no inner *) */
static size_t find_3star_close(const char *src, size_t start, size_t len) {
    size_t j = start;
    while (j < len && src[j] != '*') j++;
    if (j == start || j + 2 >= len) return (size_t)-1;
    if (src[j] == '*' && src[j + 1] == '*' && src[j + 2] == '*') return j;
    return (size_t)-1;
}

static size_t find_2star_close(const char *src, size_t start, size_t len) {
    size_t j = start;
    while (j < len && src[j] != '*') j++;
    if (j == start || j + 1 >= len) return (size_t)-1;
    if (src[j] == '*' && src[j + 1] == '*') return j;
    return (size_t)-1;
}

static size_t find_1star_close(const char *src, size_t start, size_t len) {
    size_t j = start;
    while (j < len && src[j] != '*') j++;
    if (j == start || j >= len) return (size_t)-1;
    return j;
}

static size_t find_strike_close(const char *src, size_t start, size_t len) {
    size_t j = start;
    while (j + 1 < len) {
        if (src[j] == '~' && src[j + 1] == '~') {
            if (j == start) return (size_t)-1;
            return j;
        }
        j++;
    }
    return (size_t)-1;
}

/* Autolink URL schemes we recognize (CommonMark-ish subset). */
static int has_autolink_scheme(const char *s, size_t len) {
    if (len >= 7  && memcmp(s, "http://",  7) == 0) return 1;
    if (len >= 8  && memcmp(s, "https://", 8) == 0) return 1;
    if (len >= 6  && memcmp(s, "ftp://",   6) == 0) return 1;
    if (len >= 7  && memcmp(s, "mailto:",  7) == 0) return 1;
    return 0;
}

/* Emit a link span: LINK + UNDERLINE + text + RESET + TEXT */
static void emit_link(md_buf_t *out, const char *text, size_t tlen) {
    md_buf_append_str(out, md_pal.link);
    md_buf_append_str(out, md_pal.underline);
    md_buf_append(out, text, tlen);
    md_buf_append_str(out, md_pal.reset);
    md_buf_append_str(out, md_pal.text);
}

/* Emit an image placeholder: BOLD[IMG: alt]RESET + TEXT */
static void emit_image(md_buf_t *out, const char *alt, size_t alen) {
    md_buf_append_str(out, md_pal.bold);
    md_buf_append_byte(out, '[');
    md_buf_append_str(out, "IMG: ");
    md_buf_append(out, alt, alen);
    md_buf_append_byte(out, ']');
    md_buf_append_str(out, md_pal.reset);
    md_buf_append_str(out, md_pal.text);
}

/* Emit a footnote reference: TEXT[BOLD DIM ^id RESET TEXT] */
static void emit_footnote_ref(md_buf_t *out, const char *id, size_t idlen) {
    md_buf_append_str(out, md_pal.text);
    md_buf_append_byte(out, '[');
    md_buf_append_str(out, md_pal.bold);
    md_buf_append_str(out, md_pal.dim);
    md_buf_append_byte(out, '^');
    md_buf_append(out, id, idlen);
    md_buf_append_str(out, md_pal.reset);
    md_buf_append_str(out, md_pal.text);
    md_buf_append_byte(out, ']');
}

/* ----- main scanner ----- */

void md_inline_expand(const char *src, size_t len,
                      const md_inline_ctx_t *ctx, md_buf_t *out) {
    md_buf_reset(out);

    size_t i = 0;
    while (i < len) {
        char c = src[i];

        /* Escape: \X */
        if (c == '\\' && i + 1 < len && is_escape_char(src[i + 1])) {
            md_buf_append_byte(out, src[i + 1]);
            i += 2;
            continue;
        }

        /* Inline code: open with a run of N backticks, close with a run of
         * EXACTLY N backticks (CommonMark). N-1 backticks inside are literal. */
        if (c == '`') {
            size_t open_run = 0;
            while (i + open_run < len && src[i + open_run] == '`') open_run++;
            size_t j = i + open_run;
            size_t close = (size_t)-1;
            while (j < len) {
                if (src[j] != '`') { j++; continue; }
                size_t run = 0;
                while (j + run < len && src[j + run] == '`') run++;
                if (run == open_run) { close = j; break; }
                j += run;
            }
            if (close != (size_t)-1) {
                size_t cstart = i + open_run;
                size_t clen   = close - cstart;
                /* Strip a single leading/trailing space if both ends have one
                 * AND the content isn't all spaces — matches CommonMark. */
                if (clen >= 2
                    && src[cstart] == ' ' && src[cstart + clen - 1] == ' ') {
                    int all_space = 1;
                    for (size_t k = 0; k < clen; k++) {
                        if (src[cstart + k] != ' ') { all_space = 0; break; }
                    }
                    if (!all_space) { cstart++; clen -= 2; }
                }
                md_buf_append_str(out, md_pal.codeblock);
                md_buf_append(out, src + cstart, clen);
                md_buf_append_str(out, md_pal.reset);
                md_buf_append_str(out, md_pal.text);
                i = close + open_run;
                continue;
            }
        }

        /* Image: ![alt](url) */
        if (c == '!' && i + 1 < len && src[i + 1] == '[') {
            size_t bclose = find_char(src, i + 2, len, ']');
            if (bclose != (size_t)-1 && bclose + 1 < len
                && src[bclose + 1] == '(') {
                size_t pclose = find_char(src, bclose + 2, len, ')');
                if (pclose != (size_t)-1) {
                    if (ctx && ctx->no_images) {
                        /* literal: ![alt](url) */
                        md_buf_append(out, src + i, pclose - i + 1);
                    } else {
                        emit_image(out, src + i + 2, bclose - i - 2);
                    }
                    i = pclose + 1;
                    continue;
                }
            }
        }

        /* Footnote ref: [^id] */
        if (c == '[' && i + 1 < len && src[i + 1] == '^') {
            size_t close = find_char(src, i + 2, len, ']');
            if (close != (size_t)-1 && close > i + 2) {
                size_t idlen = close - i - 2;
                if (ctx && ctx->no_footnotes) {
                    md_buf_append(out, src + i, close - i + 1);
                } else {
                    /* Register the ref into the doc (dedupes internally) */
                    if (ctx && ctx->doc) {
                        char *id_s = malloc(idlen + 1);
                        if (id_s) {
                            memcpy(id_s, src + i + 2, idlen);
                            id_s[idlen] = '\0';
                            md_doc_add_footnote_ref(ctx->doc, id_s);
                            free(id_s);
                        }
                    }
                    emit_footnote_ref(out, src + i + 2, idlen);
                }
                i = close + 1;
                continue;
            }
        }

        /* Inline link: [text](url) */
        if (c == '[') {
            size_t bclose = find_char(src, i + 1, len, ']');
            if (bclose != (size_t)-1 && bclose + 1 < len
                && src[bclose + 1] == '(') {
                size_t pclose = find_char(src, bclose + 2, len, ')');
                if (pclose != (size_t)-1) {
                    size_t tlen = bclose - i - 1;
                    if (ctx && ctx->no_links) {
                        md_buf_append(out, src + i, pclose - i + 1);
                    } else {
                        emit_link(out, src + i + 1, tlen);
                    }
                    i = pclose + 1;
                    continue;
                }
            }
            /* Ref-link: [text][id] or shortcut [text] */
            if (bclose != (size_t)-1 && bclose + 1 < len
                && src[bclose + 1] == '[') {
                size_t ridx = bclose + 2;
                size_t rclose = find_char(src, ridx, len, ']');
                if (rclose != (size_t)-1) {
                    size_t tlen   = bclose - i - 1;
                    size_t ridlen = rclose - ridx;
                    /* If id empty, use text as id (collapsed reference) */
                    const char *id  = ridlen ? src + ridx : src + i + 1;
                    size_t      ilen = ridlen ? ridlen      : tlen;
                    char *id_s = malloc(ilen + 1);
                    if (id_s) {
                        memcpy(id_s, id, ilen);
                        id_s[ilen] = '\0';
                        const md_link_ref_t *r = (ctx && ctx->doc)
                            ? md_doc_find_link_ref(ctx->doc, id_s) : NULL;
                        free(id_s);
                        if (r) {
                            if (ctx && ctx->no_links) {
                                md_buf_append(out, src + i, rclose - i + 1);
                            } else {
                                emit_link(out, src + i + 1, tlen);
                            }
                            i = rclose + 1;
                            continue;
                        }
                    }
                }
            }
            /* Shortcut ref: [text] alone matches def */
            if (bclose != (size_t)-1) {
                size_t tlen = bclose - i - 1;
                if (tlen > 0) {
                    char *id_s = malloc(tlen + 1);
                    if (id_s) {
                        memcpy(id_s, src + i + 1, tlen);
                        id_s[tlen] = '\0';
                        const md_link_ref_t *r = (ctx && ctx->doc)
                            ? md_doc_find_link_ref(ctx->doc, id_s) : NULL;
                        free(id_s);
                        if (r) {
                            if (ctx && ctx->no_links) {
                                md_buf_append(out, src + i, bclose - i + 1);
                            } else {
                                emit_link(out, src + i + 1, tlen);
                            }
                            i = bclose + 1;
                            continue;
                        }
                    }
                }
            }
        }

        /* Bare URL autolink: http(s)://... (gated by ctx->bare_urls).
         * Position guard: previous char must be word-boundary (start, space,
         * or punctuation that isn't part of an URL). */
        if (ctx && ctx->bare_urls && !ctx->no_links
            && (c == 'h' || c == 'H')) {
            int boundary = (i == 0) || !isalnum((unsigned char)src[i - 1]);
            if (boundary) {
                size_t urllen = 0;
                if (i + 7 < len && memcmp(src + i, "http://", 7) == 0) urllen = 7;
                else if (i + 8 < len && memcmp(src + i, "https://", 8) == 0) urllen = 8;
                if (urllen) {
                    size_t end = i + urllen;
                    while (end < len) {
                        unsigned char x = (unsigned char)src[end];
                        if (x <= ' ' || x == '<' || x == '>' || x == '"' || x == ')')
                            break;
                        end++;
                    }
                    while (end > i + urllen
                           && (src[end - 1] == '.' || src[end - 1] == ','
                               || src[end - 1] == ';' || src[end - 1] == '!'
                               || src[end - 1] == '?' || src[end - 1] == ':'))
                        end--;
                    if (end > i + urllen) {
                        emit_link(out, src + i, end - i);
                        i = end;
                        continue;
                    }
                }
            }
        }

        /* Autolink: <http://...> | <mailto:...> */
        if (c == '<') {
            size_t close = find_char(src, i + 1, len, '>');
            if (close != (size_t)-1) {
                size_t inner_len = close - i - 1;
                if (has_autolink_scheme(src + i + 1, inner_len)) {
                    if (ctx && ctx->no_links) {
                        md_buf_append(out, src + i, close - i + 1);
                    } else {
                        emit_link(out, src + i + 1, inner_len);
                    }
                    i = close + 1;
                    continue;
                }
            }
        }

        /* Triple star: ***foo*** */
        if (c == '*' && i + 2 < len && src[i + 1] == '*' && src[i + 2] == '*') {
            size_t close = find_3star_close(src, i + 3, len);
            if (close != (size_t)-1) {
                size_t clen = close - i - 3;
                md_buf_append_str(out, md_pal.bold);
                md_buf_append_str(out, md_pal.italic);
                md_buf_append(out, src + i + 3, clen);
                md_buf_append_str(out, md_pal.reset);
                md_buf_append_str(out, md_pal.text);
                i = close + 3;
                continue;
            }
        }

        /* Double star: **foo** */
        if (c == '*' && i + 1 < len && src[i + 1] == '*') {
            size_t close = find_2star_close(src, i + 2, len);
            if (close != (size_t)-1) {
                size_t clen = close - i - 2;
                md_buf_append_str(out, md_pal.bold);
                md_buf_append(out, src + i + 2, clen);
                md_buf_append_str(out, md_pal.reset);
                md_buf_append_str(out, md_pal.text);
                i = close + 2;
                continue;
            }
        }

        /* Italic: *foo* with non-* neighbours */
        if (c == '*' && i > 0 && src[i - 1] != '*'
            && i + 1 < len && src[i + 1] != '*') {
            size_t close = find_1star_close(src, i + 1, len);
            if (close != (size_t)-1
                && close + 1 < len && src[close + 1] != '*') {
                size_t clen = close - i - 1;
                md_buf_append_str(out, md_pal.italic);
                md_buf_append(out, src + i + 1, clen);
                md_buf_append_str(out, md_pal.reset);
                md_buf_append_str(out, md_pal.text);
                i = close + 1;
                continue;
            }
        }

        /* Strike: ~~foo~~ */
        if (c == '~' && i + 1 < len && src[i + 1] == '~') {
            size_t close = find_strike_close(src, i + 2, len);
            if (close != (size_t)-1) {
                size_t clen = close - i - 2;
                md_buf_append_str(out, md_pal.strike);
                md_buf_append(out, src + i + 2, clen);
                md_buf_append_str(out, md_pal.reset);
                md_buf_append_str(out, md_pal.text);
                i = close + 2;
                continue;
            }
        }

        /* Literal */
        md_buf_append_byte(out, c);
        i++;
    }
}

/*fin*/
