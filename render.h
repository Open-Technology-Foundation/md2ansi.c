/* render.h - block dispatch + ANSI output */
#ifndef MD_RENDER_H
#define MD_RENDER_H

#include <stdio.h>
#include "parser.h"
#include "ansi.h"

typedef struct {
    int width;
    int no_footnotes;
    int no_syntax;
    int no_tables;
    int no_tasks;
    int no_images;
    int no_links;
    int bare_urls;
    int no_table_wrap;
} md_render_opts_t;

/* Render document to stdout. */
void md_render(const md_doc_t *doc, const md_render_opts_t *opts, FILE *fp);

/* Wrap text to width (ANSI- and UTF-8-aware), emitting lines (with newlines)
 * into `out`. */
void md_wrap_text(const char *src, size_t len, int width, md_buf_t *out);

#endif /* MD_RENDER_H */
/*fin*/
