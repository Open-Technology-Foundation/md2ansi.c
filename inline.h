/* inline.h - inline span expansion (code, bold, italic, links, etc.) */
#ifndef MD_INLINE_H
#define MD_INLINE_H

#include "md_common.h"
#include "parser.h"

typedef struct md_inline_ctx {
    int no_images;
    int no_links;
    int no_footnotes;
    int bare_urls;          /* if 1, auto-link bare http(s):// URLs */
    md_doc_t *doc;          /* for ref-link/footnote lookups */
} md_inline_ctx_t;

/* Expand inline markdown in `src` into `out`. `out` is reset first.
 * Output contains ANSI codes per current palette. */
void md_inline_expand(const char *src, size_t len,
                      const md_inline_ctx_t *ctx, md_buf_t *out);

#endif /* MD_INLINE_H */
/*fin*/
