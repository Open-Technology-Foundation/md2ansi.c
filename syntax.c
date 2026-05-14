/* syntax.c - per-language single-line syntax highlighters.
 * Same simplifications as Bash version: no multi-line strings, no nested
 * formatting — just comments, optional docstrings, and keyword replacement. */
#include "syntax.h"
#include "ansi.h"

#include <ctype.h>
#include <string.h>
#include <strings.h>

static const char *PY_KEYWORDS[] = {
    "def","class","if","elif","else","for","while","return","import","from","print",
    NULL
};
static const char *JS_KEYWORDS[] = {
    "function","const","let","var","if","else","for","while","return","class","console",
    NULL
};
static const char *BASH_KEYWORDS[] = {
    "if","then","else","elif","fi","for","while","do","done",
    "echo","printf","local","declare",
    NULL
};

static int is_word_char(unsigned char c) {
    return isalnum(c) || c == '_';
}

/* Normalize lang: py→python, js→javascript, sh|shell→bash */
static const char *normalize_lang(const char *lang) {
    if (!lang) return NULL;
    if (strcasecmp(lang, "py")    == 0) return "python";
    if (strcasecmp(lang, "js")    == 0) return "javascript";
    if (strcasecmp(lang, "sh")    == 0) return "bash";
    if (strcasecmp(lang, "shell") == 0) return "bash";
    return lang;
}

/* Find first non-whitespace char index. */
static size_t first_nonws(const char *s, size_t len) {
    size_t i = 0;
    while (i < len && (s[i] == ' ' || s[i] == '\t')) i++;
    return i;
}

/* Is this line a comment for the given lang? */
static int is_comment_line(const char *s, size_t len, const char *lang) {
    size_t i = first_nonws(s, len);
    if (i >= len) return 0;
    if (strcmp(lang, "python") == 0 || strcmp(lang, "bash") == 0)
        return s[i] == '#';
    if (strcmp(lang, "javascript") == 0)
        return i + 1 < len && s[i] == '/' && s[i + 1] == '/';
    return 0;
}

/* Python triple-quote heuristic: line contains ''' or """ */
static int is_python_docstring(const char *s, size_t len) {
    for (size_t i = 0; i + 2 < len; i++) {
        if ((s[i] == '\'' && s[i+1] == '\'' && s[i+2] == '\'')
            || (s[i] == '"' && s[i+1] == '"' && s[i+2] == '"'))
            return 1;
    }
    return 0;
}

static int is_keyword(const char *word, size_t wlen, const char **list) {
    for (size_t i = 0; list[i]; i++) {
        size_t klen = strlen(list[i]);
        if (klen == wlen && memcmp(word, list[i], wlen) == 0) return 1;
    }
    return 0;
}

static void highlight_with_keywords(const char *s, size_t len,
                                    const char **kw, md_buf_t *out) {
    size_t i = 0;
    while (i < len) {
        unsigned char c = (unsigned char)s[i];
        if (is_word_char(c) && !isdigit(c)) {
            size_t start = i;
            while (i < len && is_word_char((unsigned char)s[i])) i++;
            size_t wlen = i - start;
            if (is_keyword(s + start, wlen, kw)) {
                md_buf_append_str(out, md_pal.keyword);
                md_buf_append(out, s + start, wlen);
                md_buf_append_str(out, md_pal.codeblock);
            } else {
                md_buf_append(out, s + start, wlen);
            }
        } else {
            md_buf_append_byte(out, s[i]);
            i++;
        }
    }
}

void md_syntax_highlight(const char *src, size_t len,
                         const char *lang, md_buf_t *out) {
    md_buf_reset(out);
    const char *l = normalize_lang(lang);
    if (!l) { md_buf_append(out, src, len); return; }

    /* Comment line — whole line in comment color */
    if (is_comment_line(src, len, l)) {
        md_buf_append_str(out, md_pal.comment);
        md_buf_append(out, src, len);
        md_buf_append_str(out, md_pal.codeblock);
        return;
    }

    /* Python docstring — whole line in string color */
    if (strcmp(l, "python") == 0 && is_python_docstring(src, len)) {
        md_buf_append_str(out, md_pal.string);
        md_buf_append(out, src, len);
        md_buf_append_str(out, md_pal.codeblock);
        return;
    }

    if (strcmp(l, "python") == 0) {
        highlight_with_keywords(src, len, PY_KEYWORDS, out);
    } else if (strcmp(l, "javascript") == 0) {
        highlight_with_keywords(src, len, JS_KEYWORDS, out);
    } else if (strcmp(l, "bash") == 0) {
        highlight_with_keywords(src, len, BASH_KEYWORDS, out);
    } else {
        md_buf_append(out, src, len);
    }
}

/*fin*/
