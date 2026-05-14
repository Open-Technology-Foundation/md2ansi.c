/* table.c - GFM pipe table parser + renderer with box-drawing borders */
#include "table.h"
#include "inline.h"
#include "ansi.h"
#include "unicode.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* Box-drawing UTF-8 sequences */
#define BD_H   "\xe2\x94\x80"  /* ─ */
#define BD_V   "\xe2\x94\x82"  /* │ */
#define BD_TL  "\xe2\x94\x8c"  /* ┌ */
#define BD_TR  "\xe2\x94\x90"  /* ┐ */
#define BD_BL  "\xe2\x94\x94"  /* └ */
#define BD_BR  "\xe2\x94\x98"  /* ┘ */
#define BD_LT  "\xe2\x94\x9c"  /* ├ */
#define BD_RT  "\xe2\x94\xa4"  /* ┤ */
#define BD_TT  "\xe2\x94\xac"  /* ┬ */
#define BD_BT  "\xe2\x94\xb4"  /* ┴ */
#define BD_X   "\xe2\x94\xbc"  /* ┼ */

void md_table_init(md_table_t *t) {
    t->cells = NULL;
    t->row_count = 0;
    t->col_count = 0;
    t->aligns = NULL;
    t->has_alignment = 0;
}

void md_table_free(md_table_t *t) {
    if (!t) return;
    if (t->cells) {
        for (size_t k = 0; k < t->row_count * t->col_count; k++) free(t->cells[k]);
        free(t->cells);
    }
    free(t->aligns);
    md_table_init(t);
}

/* Detect alignment cell: ^:?-+:?$ */
static int parse_align_cell(const char *s, size_t len, md_align_t *out) {
    if (len == 0) return 0;
    size_t i = 0;
    int left_colon = 0, right_colon = 0;
    if (s[i] == ':') { left_colon = 1; i++; }
    size_t dashes = 0;
    while (i < len && s[i] == '-') { i++; dashes++; }
    if (dashes == 0) return 0;
    if (i < len && s[i] == ':') { right_colon = 1; i++; }
    if (i != len) return 0;
    if (left_colon && right_colon) *out = MD_ALIGN_CENTER;
    else if (right_colon)          *out = MD_ALIGN_RIGHT;
    else                           *out = MD_ALIGN_LEFT;
    return 1;
}

/* True if position `i` in `line` is a cell separator (unescaped `|`,
 * not inside a backtick span). */
static int is_cell_sep(const char *line, size_t i) {
    if (line[i] != '|') return 0;
    if (i > 0 && line[i - 1] == '\\') return 0;
    /* count backticks before position i: odd = inside code span */
    int in_code = 0;
    for (size_t k = 0; k < i; k++) {
        if (line[k] == '`' && (k == 0 || line[k - 1] != '\\')) in_code = !in_code;
    }
    return !in_code;
}

/* Copy s[a..b) into a malloc'd NUL-terminated string, with '\|' unescaped
 * to literal '|' ONLY when outside a backtick span (CommonMark: backtick
 * content is literal and not subject to escape processing). */
static char *cell_dup_unescape(const char *s, size_t a, size_t b) {
    char *out = malloc(b - a + 1);
    if (!out) md_die(MD_EX_INVAL, "out of memory");
    size_t o = 0;
    int in_code = 0;
    for (size_t i = a; i < b; i++) {
        if (s[i] == '`' && (i == a || s[i - 1] != '\\')) in_code = !in_code;
        if (!in_code && s[i] == '\\' && i + 1 < b && s[i + 1] == '|') {
            out[o++] = '|';
            i++;
            continue;
        }
        out[o++] = s[i];
    }
    out[o] = '\0';
    return out;
}

/* Split one row by unescaped '|' (after trimming leading/trailing ws and
 * outer pipes). Pipes inside `code` and `\|` escapes are preserved literally. */
static size_t split_row(const char *line, size_t len, char ***cells_out) {
    size_t s = 0, e = len;
    while (s < e && (line[s] == ' ' || line[s] == '\t')) s++;
    while (e > s && (line[e - 1] == ' ' || line[e - 1] == '\t')) e--;
    if (s < e && line[s] == '|') s++;
    if (e > s && line[e - 1] == '|' && (e - 1 == s || line[e - 2] != '\\')) e--;

    /* Count cells (positions of unescaped pipes outside code spans) */
    size_t n = 1;
    for (size_t i = s; i < e; i++) if (is_cell_sep(line, i)) n++;
    char **out = malloc(n * sizeof(char *));
    if (!out) md_die(MD_EX_INVAL, "out of memory");
    size_t idx = 0;
    size_t cs = s;
    for (size_t i = s; i <= e; i++) {
        if (i == e || is_cell_sep(line, i)) {
            size_t ce = i;
            size_t a = cs, b = ce;
            while (a < b && (line[a] == ' ' || line[a] == '\t')) a++;
            while (b > a && (line[b - 1] == ' ' || line[b - 1] == '\t')) b--;
            out[idx++] = cell_dup_unescape(line, a, b);
            cs = i + 1;
        }
    }
    *cells_out = out;
    return n;
}

void md_table_parse(const md_lines_t *lines, md_table_t *out) {
    md_table_init(out);
    if (lines->count == 0) return;

    /* Pass 1: split each row, determine max col_count */
    char ***rows = calloc(lines->count, sizeof *rows);
    size_t *row_lens = calloc(lines->count, sizeof *row_lens);
    if (!rows || !row_lens) md_die(MD_EX_INVAL, "out of memory");
    size_t max_cols = 0;
    for (size_t i = 0; i < lines->count; i++) {
        row_lens[i] = split_row(lines->items[i].start, lines->items[i].len,
                                &rows[i]);
        if (row_lens[i] > max_cols) max_cols = row_lens[i];
    }

    out->col_count = max_cols;
    out->aligns = calloc(max_cols, sizeof *out->aligns);
    if (!out->aligns) md_die(MD_EX_INVAL, "out of memory");

    /* Detect alignment row (row 1) */
    int is_align_row = 0;
    if (lines->count >= 2 && row_lens[1] > 0) {
        is_align_row = 1;
        for (size_t c = 0; c < row_lens[1]; c++) {
            md_align_t a;
            if (!parse_align_cell(rows[1][c], strlen(rows[1][c]), &a)) {
                is_align_row = 0;
                break;
            }
        }
    }
    if (is_align_row) {
        out->has_alignment = 1;
        for (size_t c = 0; c < row_lens[1] && c < max_cols; c++) {
            parse_align_cell(rows[1][c], strlen(rows[1][c]), &out->aligns[c]);
        }
    }

    /* Build data row list: skip alignment row if present */
    size_t out_rows = lines->count - (is_align_row ? 1 : 0);
    out->row_count = out_rows;
    out->cells = calloc(out_rows * max_cols, sizeof *out->cells);
    if (!out->cells) md_die(MD_EX_INVAL, "out of memory");

    size_t dr = 0;
    for (size_t r = 0; r < lines->count; r++) {
        if (is_align_row && r == 1) {
            for (size_t c = 0; c < row_lens[r]; c++) free(rows[r][c]);
            free(rows[r]);
            continue;
        }
        for (size_t c = 0; c < max_cols; c++) {
            if (c < row_lens[r]) {
                out->cells[dr * max_cols + c] = rows[r][c]; /* take ownership */
            } else {
                char *empty = malloc(1);
                if (!empty) md_die(MD_EX_INVAL, "out of memory");
                empty[0] = '\0';
                out->cells[dr * max_cols + c] = empty;
            }
        }
        free(rows[r]);
        dr++;
    }
    free(rows);
    free(row_lens);
}

/* Compute the visible width of a cell after inline expansion. */
static size_t cell_visible_width(const char *cell,
                                 const md_inline_ctx_t *ictx,
                                 md_buf_t *scratch) {
    md_inline_expand(cell, strlen(cell), ictx, scratch);
    return md_visible_width(scratch->data, scratch->len);
}

/* Pad cell text to `width` visible columns according to alignment.
 * `formatted` is the already-inline-expanded text. */
static void emit_aligned_cell(FILE *fp, const char *formatted, size_t flen,
                              size_t target_width, md_align_t a) {
    size_t vw = md_visible_width(formatted, flen);
    size_t pad = (target_width > vw) ? (target_width - vw) : 0;
    size_t left_pad = 0, right_pad = 0;
    if (a == MD_ALIGN_CENTER) {
        left_pad  = pad / 2;
        right_pad = pad - left_pad;
    } else if (a == MD_ALIGN_RIGHT) {
        left_pad = pad;
    } else {
        right_pad = pad;
    }
    for (size_t k = 0; k < left_pad; k++) fputc(' ', fp);
    /* switch to text color so cell content is distinct from border chrome */
    fputs(md_pal.text, fp);
    if (flen) fwrite(formatted, 1, flen, fp);
    /* re-establish table color in case inline reset/text-color overrode it */
    fputs(md_pal.table, fp);
    for (size_t k = 0; k < right_pad; k++) fputc(' ', fp);
}

/* Emit a border row: left + segments + right, each segment of dashes. */
static void emit_border(FILE *fp, const size_t *widths, size_t cols,
                        const char *left, const char *mid, const char *right) {
    fputs(md_pal.table, fp);
    fputs(left, fp);
    for (size_t c = 0; c < cols; c++) {
        for (size_t k = 0; k < widths[c] + 2; k++) fputs(BD_H, fp);
        fputs(c + 1 == cols ? right : mid, fp);
    }
    fputs(md_pal.reset, fp);
    fputc('\n', fp);
}

void md_table_render(const md_table_t *t, FILE *fp,
                     const md_inline_ctx_t *ictx) {
    if (t->row_count == 0 || t->col_count == 0) return;

    /* Compute per-column widths using inline-expanded visible width */
    size_t *widths = calloc(t->col_count, sizeof *widths);
    if (!widths) md_die(MD_EX_INVAL, "out of memory");

    md_buf_t scratch; md_buf_init(&scratch);
    for (size_t r = 0; r < t->row_count; r++) {
        for (size_t c = 0; c < t->col_count; c++) {
            const char *cell = t->cells[r * t->col_count + c];
            size_t w = cell_visible_width(cell, ictx, &scratch);
            if (w > widths[c]) widths[c] = w;
        }
    }

    emit_border(fp, widths, t->col_count, BD_TL, BD_TT, BD_TR);

    md_buf_t exp; md_buf_init(&exp);
    for (size_t r = 0; r < t->row_count; r++) {
        fputs(md_pal.table, fp);
        fputs(BD_V, fp);
        for (size_t c = 0; c < t->col_count; c++) {
            const char *cell = t->cells[r * t->col_count + c];
            md_inline_expand(cell, strlen(cell), ictx, &exp);
            fputc(' ', fp);
            emit_aligned_cell(fp, exp.data, exp.len, widths[c], t->aligns[c]);
            fputc(' ', fp);
            fputs(BD_V, fp);
        }
        fputs(md_pal.reset, fp);
        fputc('\n', fp);

        /* Header separator after row 0 if alignment was present */
        if (r == 0 && t->has_alignment) {
            emit_border(fp, widths, t->col_count, BD_LT, BD_X, BD_RT);
        }
    }
    emit_border(fp, widths, t->col_count, BD_BL, BD_BT, BD_BR);
    md_buf_free(&exp);
    md_buf_free(&scratch);
    free(widths);
}

/*fin*/
