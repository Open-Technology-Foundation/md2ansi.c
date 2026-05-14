/* render.c - block dispatch + wrap + per-kind renderers */
#include "render.h"
#include "inline.h"
#include "ansi.h"
#include "unicode.h"
#include "table.h"
#include "syntax.h"

#include <stdlib.h>
#include <string.h>

/* ----- wrap ----- */

void md_wrap_text(const char *src, size_t len, int width, md_buf_t *out) {
    md_buf_reset(out);
    if (width <= 0) {
        md_buf_append(out, src, len);
        md_buf_append_byte(out, '\n');
        return;
    }
    size_t visible = md_visible_width(src, len);
    if ((int)visible <= width) {
        md_buf_append(out, src, len);
        md_buf_append_byte(out, '\n');
        return;
    }

    md_buf_t cur; md_buf_init(&cur);
    size_t cur_w = 0;
    int first = 1;

    size_t i = 0;
    while (i < len) {
        while (i < len && src[i] == ' ') i++;
        if (i >= len) break;
        size_t w_start = i;
        while (i < len && src[i] != ' ') i++;
        size_t w_len = i - w_start;
        size_t w_w = md_visible_width(src + w_start, w_len);

        if (first) {
            md_buf_append(&cur, src + w_start, w_len);
            cur_w = w_w;
            first = 0;
        } else if (cur_w + 1 + w_w <= (size_t)width) {
            md_buf_append_byte(&cur, ' ');
            md_buf_append(&cur, src + w_start, w_len);
            cur_w += 1 + w_w;
        } else {
            md_buf_append(out, cur.data, cur.len);
            md_buf_append_byte(out, '\n');
            md_buf_reset(&cur);
            md_buf_append(&cur, src + w_start, w_len);
            cur_w = w_w;
        }
    }
    if (cur.len > 0) {
        md_buf_append(out, cur.data, cur.len);
        md_buf_append_byte(out, '\n');
    }
    md_buf_free(&cur);
}

/* Emit a wrap buffer to fp (already contains newlines). */
static void emit_wrap(FILE *fp, const md_buf_t *wrap) {
    if (wrap->len > 0) fwrite(wrap->data, 1, wrap->len, fp);
}

/* ----- block renderers ----- */

static void render_para(const md_block_t *b, const md_render_opts_t *opts,
                        const md_inline_ctx_t *ictx,
                        md_buf_t *inl, md_buf_t *wrap, FILE *fp) {
    md_inline_expand(b->content.data, b->content.len, ictx, inl);
    /* Prepend COLOR_TEXT (Bash does ${COLOR_TEXT}${formatted_line} before wrap). */
    md_buf_t tmp; md_buf_init(&tmp);
    md_buf_append_str(&tmp, md_pal.text);
    md_buf_append(&tmp, inl->data, inl->len);
    md_wrap_text(tmp.data, tmp.len, opts->width, wrap);
    emit_wrap(fp, wrap);
    md_buf_free(&tmp);
}

static void render_header(const md_block_t *b,
                          const md_inline_ctx_t *ictx, md_buf_t *inl, FILE *fp) {
    const char *col;
    switch (b->kind) {
    case MD_B_H1: col = md_pal.h1; break;
    case MD_B_H2: col = md_pal.h2; break;
    case MD_B_H3: col = md_pal.h3; break;
    case MD_B_H4: col = md_pal.h4; break;
    case MD_B_H5: col = md_pal.h5; break;
    default:      col = md_pal.h6; break;
    }
    md_inline_expand(b->content.data, b->content.len, ictx, inl);
    fputs(col, fp);
    if (inl->len) fwrite(inl->data, 1, inl->len, fp);
    fputs(md_pal.reset, fp);
    fputc('\n', fp);
}

static void render_hr(int width, FILE *fp) {
    fputs(md_pal.hr, fp);
    /* width-1 box-drawing horizontal chars (U+2500 = E2 94 80) */
    for (int n = 0; n < width - 1; n++) fputs("\xe2\x94\x80", fp);
    fputs(md_pal.reset, fp);
    fputc('\n', fp);
}

static void render_blockquote(const md_block_t *b,
                              const md_render_opts_t *opts,
                              const md_inline_ctx_t *ictx,
                              md_buf_t *inl, md_buf_t *wrap, FILE *fp) {
    md_inline_expand(b->content.data, b->content.len, ictx, inl);
    md_wrap_text(inl->data, inl->len, opts->width - 4, wrap);
    /* Walk wrap output line by line, emit "  > {BLOCKQUOTE}line{RESET}" each. */
    size_t i = 0;
    while (i < wrap->len) {
        size_t start = i;
        while (i < wrap->len && wrap->data[i] != '\n') i++;
        fputs(md_pal.text, fp);
        fputs("  > ", fp);
        fputs(md_pal.blockquote, fp);
        fwrite(wrap->data + start, 1, i - start, fp);
        fputs(md_pal.reset, fp);
        fputc('\n', fp);
        if (i < wrap->len) i++;
    }
}

static void render_list_item_common(const md_block_t *b,
                                    const md_render_opts_t *opts,
                                    const md_inline_ctx_t *ictx,
                                    md_buf_t *inl, md_buf_t *wrap, FILE *fp,
                                    const char *bullet, int bullet_visible_w,
                                    int extra_indent) {
    int indent_lvl   = b->level / 2;
    int bullet_ind   = indent_lvl * 2;
    int text_indent  = (indent_lvl + 1) * 2 + extra_indent;
    int wrap_width   = opts->width - text_indent - 2;
    if (wrap_width < 1) wrap_width = 1;

    md_inline_expand(b->content.data, b->content.len, ictx, inl);
    md_wrap_text(inl->data, inl->len, wrap_width, wrap);

    /* First line: bullet_indent spaces + bullet + first wrap line */
    size_t i = 0;
    int line_no = 0;
    while (i < wrap->len) {
        size_t start = i;
        while (i < wrap->len && wrap->data[i] != '\n') i++;
        if (line_no == 0) {
            for (int s = 0; s < bullet_ind; s++) fputc(' ', fp);
            fputs(md_pal.list, fp);
            fputs(bullet, fp);
            fputs(md_pal.text, fp);
        } else {
            for (int s = 0; s < text_indent; s++) fputc(' ', fp);
        }
        fwrite(wrap->data + start, 1, i - start, fp);
        fputs(md_pal.reset, fp);
        fputc('\n', fp);
        line_no++;
        if (i < wrap->len) i++;
    }
    (void)bullet_visible_w;
}

static void render_ul(const md_block_t *b, const md_render_opts_t *opts,
                      const md_inline_ctx_t *ictx,
                      md_buf_t *inl, md_buf_t *wrap, FILE *fp) {
    render_list_item_common(b, opts, ictx, inl, wrap, fp, "* ", 2, 0);
}

static void render_ol(const md_block_t *b, const md_render_opts_t *opts,
                      const md_inline_ctx_t *ictx,
                      md_buf_t *inl, md_buf_t *wrap, FILE *fp) {
    char bullet[16];
    int n = snprintf(bullet, sizeof bullet, "%d. ", b->ordinal);
    if (n < 0 || n >= (int)sizeof bullet) { strcpy(bullet, "?. "); n = 3; }
    int indent_lvl   = b->level / 2;
    int bullet_ind   = indent_lvl * 2;
    int text_indent  = bullet_ind + n;
    int wrap_width   = opts->width - text_indent - 2;
    if (wrap_width < 1) wrap_width = 1;

    md_inline_expand(b->content.data, b->content.len, ictx, inl);
    md_wrap_text(inl->data, inl->len, wrap_width, wrap);

    size_t i = 0;
    int line_no = 0;
    while (i < wrap->len) {
        size_t start = i;
        while (i < wrap->len && wrap->data[i] != '\n') i++;
        if (line_no == 0) {
            for (int s = 0; s < bullet_ind; s++) fputc(' ', fp);
            fputs(md_pal.list, fp);
            fputs(bullet, fp);
            fputs(md_pal.text, fp);
        } else {
            for (int s = 0; s < text_indent; s++) fputc(' ', fp);
        }
        fwrite(wrap->data + start, 1, i - start, fp);
        fputs(md_pal.reset, fp);
        fputc('\n', fp);
        line_no++;
        if (i < wrap->len) i++;
    }
}

static void render_task(const md_block_t *b, const md_render_opts_t *opts,
                        const md_inline_ctx_t *ictx,
                        md_buf_t *inl, md_buf_t *wrap, FILE *fp) {
    int indent_lvl   = b->level / 2;
    int bullet_ind   = indent_lvl * 2;
    /* Bash adds 5 extra for "[ ] " (it's actually 4 chars, but Bash uses 5
     * for parity with the visible space; we mirror exactly). */
    int text_indent  = (indent_lvl + 1) * 2 + 5;
    int wrap_width   = opts->width - text_indent - 2;
    if (wrap_width < 1) wrap_width = 1;

    md_inline_expand(b->content.data, b->content.len, ictx, inl);
    md_wrap_text(inl->data, inl->len, wrap_width, wrap);

    /* checkbox string */
    char box[32];
    if (b->checked) {
        snprintf(box, sizeof box, "%s[%sx%s%s]",
                 md_pal.list, md_pal.bold, md_pal.reset, md_pal.list);
    } else {
        snprintf(box, sizeof box, "%s[ ]", md_pal.list);
    }

    size_t i = 0;
    int line_no = 0;
    while (i < wrap->len) {
        size_t start = i;
        while (i < wrap->len && wrap->data[i] != '\n') i++;
        if (line_no == 0) {
            for (int s = 0; s < bullet_ind; s++) fputc(' ', fp);
            fputs(md_pal.list, fp);
            fputs("* ", fp);
            fputs(box, fp);
            fputc(' ', fp);
            fputs(md_pal.text, fp);
        } else {
            for (int s = 0; s < text_indent; s++) fputc(' ', fp);
        }
        fwrite(wrap->data + start, 1, i - start, fp);
        fputs(md_pal.reset, fp);
        fputc('\n', fp);
        line_no++;
        if (i < wrap->len) i++;
    }
}

static void render_code_fenced(const md_block_t *b,
                               const md_render_opts_t *opts, FILE *fp) {
    /* Determine fence string and char */
    const char *fence_str = "```";
    if (b->ordinal == '~') fence_str = "~~~";
    /* Open line: CODEBLOCK + fence + " lang" + RESET */
    fputs(md_pal.codeblock, fp);
    fputs(fence_str, fp);
    if (b->lang && *b->lang) {
        fputc(' ', fp);
        fputs(b->lang, fp);
    }
    fputs(md_pal.reset, fp);
    fputc('\n', fp);

    /* Content lines */
    md_buf_t hl; md_buf_init(&hl);
    md_buf_t clean; md_buf_init(&clean);
    for (size_t j = 0; j < b->lines.count; j++) {
        const md_line_t *L = &b->lines.items[j];
        /* Sanitize input: strip any ANSI from raw input */
        md_buf_reset(&clean);
        md_strip_ansi(L->start, L->len, &clean);

        if (!opts->no_syntax && b->lang && *b->lang) {
            md_syntax_highlight(clean.data, clean.len, b->lang, &hl);
            fputs(md_pal.codeblock, fp);
            if (hl.len) fwrite(hl.data, 1, hl.len, fp);
            fputs(md_pal.reset, fp);
            fputc('\n', fp);
        } else {
            fputs(md_pal.codeblock, fp);
            if (clean.len) fwrite(clean.data, 1, clean.len, fp);
            fputs(md_pal.reset, fp);
            fputc('\n', fp);
        }
    }
    md_buf_free(&hl);
    md_buf_free(&clean);

    /* Close fence (only if parser saw one — ordinal != 0) */
    if (b->ordinal != 0) {
        fputs(md_pal.codeblock, fp);
        fputs(fence_str, fp);
        fputs(md_pal.reset, fp);
        fputc('\n', fp);
    }
}

static void render_code_indented(const md_block_t *b, FILE *fp) {
    fputs(md_pal.codeblock, fp);
    if (b->content.len) fwrite(b->content.data, 1, b->content.len, fp);
    fputs(md_pal.reset, fp);
    fputc('\n', fp);
}

/* ----- footnotes ----- */

static void render_footnotes(const md_doc_t *doc,
                             const md_inline_ctx_t *ictx,
                             md_buf_t *inl, FILE *fp) {
    if (!doc->footnote_refs) return;
    fputc('\n', fp);
    fputs(md_pal.h2, fp);
    fputs("Footnotes:", fp);
    fputs(md_pal.reset, fp);
    fputc('\n', fp);
    fputc('\n', fp);

    for (const md_str_list_node_t *n = doc->footnote_refs; n; n = n->next) {
        const md_footnote_def_t *def = md_doc_find_footnote_def(doc, n->s);
        fputs(md_pal.text, fp);
        fputc('[', fp);
        fputs(md_pal.bold, fp);
        fputs(md_pal.dim, fp);
        fputc('^', fp);
        fputs(n->s, fp);
        fputs(md_pal.reset, fp);
        fputs(md_pal.text, fp);
        fputs("]: ", fp);
        if (def) {
            md_inline_expand(def->text, strlen(def->text), ictx, inl);
            if (inl->len) fwrite(inl->data, 1, inl->len, fp);
        } else {
            fputs(md_pal.italic, fp);
            fputs("Missing footnote definition", fp);
        }
        fputs(md_pal.reset, fp);
        fputc('\n', fp);
    }
}

/* ----- top-level dispatch ----- */

void md_render(const md_doc_t *doc, const md_render_opts_t *opts, FILE *fp) {
    md_inline_ctx_t ictx = {
        .no_images    = opts->no_images,
        .no_links     = opts->no_links,
        .no_footnotes = opts->no_footnotes,
        .bare_urls    = opts->bare_urls,
        .doc          = (md_doc_t *)doc
    };
    md_buf_t inl, wrap;
    md_buf_init(&inl);
    md_buf_init(&wrap);

    for (const md_block_t *b = doc->head; b; b = b->next) {
        switch (b->kind) {
        case MD_B_EMPTY:
            fputc('\n', fp);
            break;
        case MD_B_PARA:
            render_para(b, opts, &ictx, &inl, &wrap, fp);
            break;
        case MD_B_H1: case MD_B_H2: case MD_B_H3:
        case MD_B_H4: case MD_B_H5: case MD_B_H6:
            render_header(b, &ictx, &inl, fp);
            break;
        case MD_B_HR:
            render_hr(opts->width, fp);
            break;
        case MD_B_BLOCKQUOTE:
            render_blockquote(b, opts, &ictx, &inl, &wrap, fp);
            break;
        case MD_B_UL_ITEM:
            render_ul(b, opts, &ictx, &inl, &wrap, fp);
            break;
        case MD_B_OL_ITEM:
            render_ol(b, opts, &ictx, &inl, &wrap, fp);
            break;
        case MD_B_TASK_ITEM:
            if (opts->no_tasks) {
                /* Treat as plain UL item with checkbox in content */
                md_block_t synth = *b;
                /* Rewrite content to "[x] text" */
                md_buf_t new_c; md_buf_init(&new_c);
                md_buf_append_fmt(&new_c, "[%c] %.*s",
                                  b->checked ? 'x' : ' ',
                                  (int)b->content.len, b->content.data);
                synth.content = new_c;
                render_ul(&synth, opts, &ictx, &inl, &wrap, fp);
                md_buf_free(&new_c);
            } else {
                render_task(b, opts, &ictx, &inl, &wrap, fp);
            }
            break;
        case MD_B_CODE_FENCED:
            render_code_fenced(b, opts, fp);
            break;
        case MD_B_CODE_INDENTED:
            render_code_indented(b, fp);
            break;
        case MD_B_TABLE:
            if (opts->no_tables) {
                /* Treat each line as paragraph */
                for (size_t j = 0; j < b->lines.count; j++) {
                    const md_line_t *L = &b->lines.items[j];
                    md_inline_expand(L->start, L->len, &ictx, &inl);
                    md_buf_t tmp; md_buf_init(&tmp);
                    md_buf_append_str(&tmp, md_pal.text);
                    md_buf_append(&tmp, inl.data, inl.len);
                    md_wrap_text(tmp.data, tmp.len, opts->width, &wrap);
                    emit_wrap(fp, &wrap);
                    md_buf_free(&tmp);
                }
            } else {
                md_table_t tbl;
                md_table_parse(&b->lines, &tbl);
                md_table_render(&tbl, fp, &ictx);
                md_table_free(&tbl);
            }
            break;
        case MD_B_FOOTNOTE_DEF:
        case MD_B_FRONTMATTER:
            /* no output */
            break;
        }
    }

    if (!opts->no_footnotes) render_footnotes(doc, &ictx, &inl, fp);

    md_buf_free(&inl);
    md_buf_free(&wrap);
}

/*fin*/
