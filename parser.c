/* parser.c - line classifier and block builder */
#include "parser.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* ----- small helpers ----- */

static char *xstrdup(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s);
    char *r = malloc(n + 1);
    if (!r) md_die(MD_EX_INVAL, "out of memory");
    memcpy(r, s, n + 1);
    return r;
}

static char *xstrndup(const char *s, size_t n) {
    char *r = malloc(n + 1);
    if (!r) md_die(MD_EX_INVAL, "out of memory");
    if (n) memcpy(r, s, n);
    r[n] = '\0';
    return r;
}

static int is_ws(char c) { return c == ' ' || c == '\t'; }

/* Trim trailing whitespace, returning new length. */
static size_t rtrim_len(const char *s, size_t len) {
    while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t'
                       || s[len - 1] == '\r')) len--;
    return len;
}

/* Count leading spaces (tabs counted as one each, matching Bash regex). */
static size_t leading_ws(const char *s, size_t len) {
    size_t i = 0;
    while (i < len && is_ws(s[i])) i++;
    return i;
}

/* ----- doc lifecycle ----- */

void md_doc_init(md_doc_t *doc) {
    doc->head = NULL;
    doc->tail = NULL;
    doc->link_refs = NULL;
    doc->footnote_defs = NULL;
    doc->footnote_refs = NULL;
    doc->footnote_refs_tail = NULL;
}

void md_doc_free(md_doc_t *doc) {
    md_block_t *b = doc->head;
    while (b) {
        md_block_t *next = b->next;
        free(b->lang);
        md_buf_free(&b->content);
        md_lines_free(&b->lines);
        free(b);
        b = next;
    }
    md_link_ref_t *r = doc->link_refs;
    while (r) {
        md_link_ref_t *n = r->next;
        free(r->id); free(r->url); free(r->title); free(r);
        r = n;
    }
    md_footnote_def_t *f = doc->footnote_defs;
    while (f) {
        md_footnote_def_t *n = f->next;
        free(f->id); free(f->text); free(f);
        f = n;
    }
    md_str_list_node_t *s = doc->footnote_refs;
    while (s) {
        md_str_list_node_t *n = s->next;
        free(s->s); free(s);
        s = n;
    }
    md_doc_init(doc);
}

void md_doc_append(md_doc_t *doc, md_block_t *b) {
    b->next = NULL;
    if (!doc->head) {
        doc->head = doc->tail = b;
    } else {
        doc->tail->next = b;
        doc->tail = b;
    }
}

void md_doc_add_footnote_ref(md_doc_t *doc, const char *id) {
    for (md_str_list_node_t *n = doc->footnote_refs; n; n = n->next) {
        if (strcmp(n->s, id) == 0) return;
    }
    md_str_list_node_t *node = malloc(sizeof *node);
    if (!node) md_die(MD_EX_INVAL, "out of memory");
    node->s = xstrdup(id);
    node->next = NULL;
    if (!doc->footnote_refs) {
        doc->footnote_refs = doc->footnote_refs_tail = node;
    } else {
        doc->footnote_refs_tail->next = node;
        doc->footnote_refs_tail = node;
    }
}

void md_doc_add_link_ref(md_doc_t *doc,
                         const char *id, const char *url, const char *title) {
    md_link_ref_t *r = malloc(sizeof *r);
    if (!r) md_die(MD_EX_INVAL, "out of memory");
    r->id = xstrdup(id);
    r->url = xstrdup(url);
    r->title = title ? xstrdup(title) : NULL;
    r->next = doc->link_refs;
    doc->link_refs = r;
}

const md_link_ref_t *md_doc_find_link_ref(const md_doc_t *doc, const char *id) {
    for (const md_link_ref_t *r = doc->link_refs; r; r = r->next) {
        if (strcasecmp(r->id, id) == 0) return r;
    }
    return NULL;
}

void md_doc_add_footnote_def(md_doc_t *doc, const char *id, const char *text) {
    md_footnote_def_t *f = malloc(sizeof *f);
    if (!f) md_die(MD_EX_INVAL, "out of memory");
    f->id = xstrdup(id);
    f->text = xstrdup(text);
    f->next = doc->footnote_defs;
    doc->footnote_defs = f;
}

const md_footnote_def_t *md_doc_find_footnote_def(const md_doc_t *doc,
                                                  const char *id) {
    for (const md_footnote_def_t *f = doc->footnote_defs; f; f = f->next) {
        if (strcmp(f->id, id) == 0) return f;
    }
    return NULL;
}

/* ----- block builder helpers ----- */

static md_block_t *new_block(md_block_kind_t kind) {
    md_block_t *b = calloc(1, sizeof *b);
    if (!b) md_die(MD_EX_INVAL, "out of memory");
    b->kind = kind;
    md_buf_init(&b->content);
    md_lines_init(&b->lines);
    return b;
}

/* ----- pattern matchers (hand-rolled, no regex) ----- */

/* Fenced code open/close. line is rtrimmed.
 * Matches: ^(``` | ~~~)(.*)$
 * Returns: fence type via *fence_char (one of '`' or '~'), info string via *info. */
static int match_fence(const char *s, size_t len, char *fence_char,
                       const char **info, size_t *info_len) {
    if (len < 3) return 0;
    char f = s[0];
    if (f != '`' && f != '~') return 0;
    if (s[1] != f || s[2] != f) return 0;
    *fence_char = f;
    size_t i = 3;
    /* Allow more fence chars: ```` etc. */
    while (i < len && s[i] == f) i++;
    /* trim leading spaces in info string */
    while (i < len && is_ws(s[i])) i++;
    size_t end = len;
    while (end > i && is_ws(s[end - 1])) end--;
    *info = s + i;
    *info_len = end - i;
    return 1;
}

/* HR: ^[[:space:]]*([-_=])\1{2,}[[:space:]]*$ */
static int match_hr(const char *s, size_t len) {
    size_t i = leading_ws(s, len);
    if (i >= len) return 0;
    char c = s[i];
    if (c != '-' && c != '_' && c != '=') return 0;
    size_t run = 0;
    while (i < len && s[i] == c) { i++; run++; }
    if (run < 3) return 0;
    while (i < len && is_ws(s[i])) i++;
    return i == len;
}

/* Header: ^(#{1,6})[[:space:]]+(.+)$ */
static int match_header(const char *s, size_t len,
                        int *level, const char **text, size_t *text_len) {
    size_t i = 0;
    while (i < len && s[i] == '#') i++;
    if (i == 0 || i > 6) return 0;
    if (i >= len || !is_ws(s[i])) return 0;
    *level = (int)i;
    /* skip spaces */
    while (i < len && is_ws(s[i])) i++;
    if (i >= len) return 0;
    *text = s + i;
    *text_len = len - i;
    return 1;
}

/* Blockquote: ^[[:space:]]*\>[[:space:]]?(.*)$ */
static int match_blockquote(const char *s, size_t len,
                            const char **text, size_t *text_len) {
    size_t i = leading_ws(s, len);
    if (i >= len || s[i] != '>') return 0;
    i++;
    if (i < len && is_ws(s[i])) i++;
    *text = s + i;
    *text_len = len - i;
    return 1;
}

/* Task: ^([[:space:]]*)[-*][[:space:]]+\[([[:space:]x])\][[:space:]]+(.+)$ */
static int match_task(const char *s, size_t len,
                      size_t *indent, char *status,
                      const char **text, size_t *text_len) {
    size_t i = 0;
    while (i < len && is_ws(s[i])) i++;
    *indent = i;
    if (i >= len || (s[i] != '-' && s[i] != '*')) return 0;
    i++;
    if (i >= len || !is_ws(s[i])) return 0;
    while (i < len && is_ws(s[i])) i++;
    if (i + 2 >= len || s[i] != '[') return 0;
    char st = s[i + 1];
    if (st != ' ' && st != 'x' && st != 'X' && st != '\t') return 0;
    if (s[i + 2] != ']') return 0;
    i += 3;
    if (i >= len || !is_ws(s[i])) return 0;
    while (i < len && is_ws(s[i])) i++;
    if (i >= len) return 0;
    *status = (st == 'X') ? 'x' : st;
    *text = s + i;
    *text_len = len - i;
    return 1;
}

/* UL: ^([[:space:]]*)[-*][[:space:]]+(.+)$ (NOT a task) */
static int match_ul(const char *s, size_t len,
                    size_t *indent, const char **text, size_t *text_len) {
    size_t i = 0;
    while (i < len && is_ws(s[i])) i++;
    *indent = i;
    if (i >= len || (s[i] != '-' && s[i] != '*')) return 0;
    i++;
    if (i >= len || !is_ws(s[i])) return 0;
    while (i < len && is_ws(s[i])) i++;
    if (i >= len) return 0;
    *text = s + i;
    *text_len = len - i;
    return 1;
}

/* OL: ^([[:space:]]*)([0-9]+)\.[[:space:]]+(.+)$ */
static int match_ol(const char *s, size_t len,
                    size_t *indent, int *number,
                    const char **text, size_t *text_len) {
    size_t i = 0;
    while (i < len && is_ws(s[i])) i++;
    *indent = i;
    if (i >= len || !isdigit((unsigned char)s[i])) return 0;
    int n = 0;
    while (i < len && isdigit((unsigned char)s[i])) {
        n = n * 10 + (s[i] - '0');
        i++;
    }
    if (i >= len || s[i] != '.') return 0;
    i++;
    if (i >= len || !is_ws(s[i])) return 0;
    while (i < len && is_ws(s[i])) i++;
    if (i >= len) return 0;
    *number = n;
    *text = s + i;
    *text_len = len - i;
    return 1;
}

/* Footnote def: ^\[\^([^]]+)\]:[[:space:]]+(.+)$ */
static int match_footnote_def(const char *s, size_t len,
                              const char **id, size_t *id_len,
                              const char **text, size_t *text_len) {
    if (len < 6 || s[0] != '[' || s[1] != '^') return 0;
    size_t i = 2;
    size_t id_start = i;
    while (i < len && s[i] != ']') i++;
    if (i >= len || i == id_start) return 0;
    *id = s + id_start;
    *id_len = i - id_start;
    i++;  /* ] */
    if (i >= len || s[i] != ':') return 0;
    i++;
    if (i >= len || !is_ws(s[i])) return 0;
    while (i < len && is_ws(s[i])) i++;
    if (i >= len) return 0;
    *text = s + i;
    *text_len = len - i;
    return 1;
}

/* Link ref def: ^\[([^]]+)\]:[[:space:]]+(\S+)([[:space:]]+"(.+)")?$
 * (NOT footnote def; ref ID does not start with ^) */
static int match_link_ref(const char *s, size_t len,
                          const char **id, size_t *id_len,
                          const char **url, size_t *url_len,
                          const char **title, size_t *title_len) {
    if (len < 4 || s[0] != '[' || s[1] == '^') return 0;
    size_t i = 1;
    size_t id_start = i;
    while (i < len && s[i] != ']') i++;
    if (i >= len || i == id_start) return 0;
    *id = s + id_start;
    *id_len = i - id_start;
    i++;
    if (i >= len || s[i] != ':') return 0;
    i++;
    if (i >= len || !is_ws(s[i])) return 0;
    while (i < len && is_ws(s[i])) i++;
    if (i >= len) return 0;
    size_t u0 = i;
    while (i < len && !is_ws(s[i])) i++;
    *url = s + u0;
    *url_len = i - u0;
    /* optional " title " */
    *title = NULL; *title_len = 0;
    while (i < len && is_ws(s[i])) i++;
    if (i < len && s[i] == '"') {
        size_t t0 = ++i;
        while (i < len && s[i] != '"') i++;
        if (i < len) { *title = s + t0; *title_len = i - t0; }
    }
    return 1;
}

/* Table sniff: ^[[:space:]]*\| */
static int looks_like_table(const char *s, size_t len) {
    size_t i = leading_ws(s, len);
    return i < len && s[i] == '|';
}

/* Indented code: starts with 4 spaces (or 1 tab), and is NOT inside a list. */
static int looks_like_indented_code(const char *s, size_t len) {
    if (len == 0) return 0;
    if (s[0] == '\t') return 1;
    if (len >= 4 && s[0] == ' ' && s[1] == ' ' && s[2] == ' ' && s[3] == ' ')
        return 1;
    return 0;
}

/* Frontmatter delimiter: line == "---" or "..." exactly. */
static int is_frontmatter_delim(const char *s, size_t len, char which) {
    if (len != 3) return 0;
    return s[0] == which && s[1] == which && s[2] == which;
}

/* ----- main parser ----- */

void md_parse(const md_lines_t *lines, md_doc_t *doc) {
    size_t i = 0;

    /* Frontmatter: line 0 is "---" AND a closing "---" or "..." exists later.
     * Otherwise leave line 0 alone — it'll be classified as HR or paragraph. */
    if (lines->count > 1
        && is_frontmatter_delim(lines->items[0].start,
                                lines->items[0].len, '-')) {
        size_t close_at = 0;
        for (size_t j = 1; j < lines->count; j++) {
            const md_line_t *L = &lines->items[j];
            if (is_frontmatter_delim(L->start, L->len, '-')
                || is_frontmatter_delim(L->start, L->len, '.')) {
                close_at = j;
                break;
            }
        }
        if (close_at > 0) {
            md_block_t *fm = new_block(MD_B_FRONTMATTER);
            md_doc_append(doc, fm);
            i = close_at + 1;
        }
    }

    char fence_char = 0;
    md_block_t *open_code = NULL;

    while (i < lines->count) {
        const md_line_t *L = &lines->items[i];
        const char *s = L->start;
        size_t len = rtrim_len(s, L->len);

        /* Fence open/close */
        char fc;
        const char *info; size_t info_len;
        if (match_fence(s, len, &fc, &info, &info_len)
            && (open_code == NULL || fc == fence_char)) {
            if (open_code) {
                /* close — mark fence char by storing as level (ASCII) */
                open_code->ordinal = (int)fence_char;
                open_code = NULL;
                fence_char = 0;
                i++;
                continue;
            }
            /* open */
            open_code = new_block(MD_B_CODE_FENCED);
            open_code->lang = info_len ? xstrndup(info, info_len) : NULL;
            fence_char = fc;
            md_doc_append(doc, open_code);
            i++;
            continue;
        }

        if (open_code) {
            md_lines_push(&open_code->lines, s, len);
            i++;
            continue;
        }

        /* Table sniff: collect consecutive | lines into one TABLE block */
        if (looks_like_table(s, len)) {
            md_block_t *b = new_block(MD_B_TABLE);
            while (i < lines->count) {
                const md_line_t *T = &lines->items[i];
                size_t tlen = rtrim_len(T->start, T->len);
                if (!looks_like_table(T->start, tlen)) break;
                md_lines_push(&b->lines, T->start, tlen);
                i++;
            }
            md_doc_append(doc, b);
            continue;
        }

        /* HR */
        if (match_hr(s, len)) {
            md_doc_append(doc, new_block(MD_B_HR));
            i++;
            continue;
        }

        /* Blockquote */
        const char *qtext; size_t qlen;
        if (match_blockquote(s, len, &qtext, &qlen)) {
            md_block_t *b = new_block(MD_B_BLOCKQUOTE);
            md_buf_append(&b->content, qtext, qlen);
            md_doc_append(doc, b);
            i++;
            continue;
        }

        /* Header */
        int hlvl;
        const char *htext; size_t hlen;
        if (match_header(s, len, &hlvl, &htext, &hlen)) {
            md_block_t *b = new_block((md_block_kind_t)(MD_B_H1 + hlvl - 1));
            b->level = hlvl;
            md_buf_append(&b->content, htext, hlen);
            md_doc_append(doc, b);
            i++;
            continue;
        }

        /* Task */
        size_t indent; char status;
        const char *ttext; size_t ttlen;
        if (match_task(s, len, &indent, &status, &ttext, &ttlen)) {
            md_block_t *b = new_block(MD_B_TASK_ITEM);
            b->level = (int)indent;
            b->checked = (status == 'x');
            md_buf_append(&b->content, ttext, ttlen);
            md_doc_append(doc, b);
            i++;
            continue;
        }

        /* UL */
        const char *ultext; size_t ullen;
        if (match_ul(s, len, &indent, &ultext, &ullen)) {
            md_block_t *b = new_block(MD_B_UL_ITEM);
            b->level = (int)indent;
            md_buf_append(&b->content, ultext, ullen);
            md_doc_append(doc, b);
            i++;
            continue;
        }

        /* OL */
        int ord;
        const char *oltext; size_t ollen;
        if (match_ol(s, len, &indent, &ord, &oltext, &ollen)) {
            md_block_t *b = new_block(MD_B_OL_ITEM);
            b->level = (int)indent;
            b->ordinal = ord;
            md_buf_append(&b->content, oltext, ollen);
            md_doc_append(doc, b);
            i++;
            continue;
        }

        /* Footnote def: ^\[\^id\]: text */
        const char *fid, *ftext; size_t fid_len, ftext_len;
        if (match_footnote_def(s, len, &fid, &fid_len, &ftext, &ftext_len)) {
            char *id_s = xstrndup(fid, fid_len);
            char *tx_s = xstrndup(ftext, ftext_len);
            md_doc_add_footnote_def(doc, id_s, tx_s);
            md_doc_add_footnote_ref(doc, id_s);   /* ensure included */
            free(id_s); free(tx_s);
            i++;
            continue;
        }

        /* Link ref def: ^\[id\]: url ["title"] */
        const char *lid, *lurl, *ltitle; size_t lid_len, lurl_len, ltitle_len;
        if (match_link_ref(s, len,
                           &lid, &lid_len, &lurl, &lurl_len,
                           &ltitle, &ltitle_len)) {
            char *id_s = xstrndup(lid, lid_len);
            char *url_s = xstrndup(lurl, lurl_len);
            char *t_s = ltitle ? xstrndup(ltitle, ltitle_len) : NULL;
            md_doc_add_link_ref(doc, id_s, url_s, t_s);
            free(id_s); free(url_s); free(t_s);
            i++;
            continue;
        }

        /* Indented code: 4 spaces or tab at start (only if not inside a list
         * context). Phase 2 marks but Phase 6 verifies. */
        if (looks_like_indented_code(s, len)) {
            md_block_t *b = new_block(MD_B_CODE_INDENTED);
            /* Strip the 4-space or tab prefix */
            size_t skip = (s[0] == '\t') ? 1 : 4;
            if (len >= skip) {
                md_buf_append(&b->content, s + skip, len - skip);
            }
            md_doc_append(doc, b);
            i++;
            continue;
        }

        /* Empty */
        if (len == 0) {
            md_doc_append(doc, new_block(MD_B_EMPTY));
            i++;
            continue;
        }

        /* Paragraph */
        md_block_t *b = new_block(MD_B_PARA);
        md_buf_append(&b->content, s, len);
        md_doc_append(doc, b);
        i++;
    }

    /* Unterminated code fence: block has ordinal=0 — renderer skips close fence. */
    (void)fence_char;
}

/*fin*/
