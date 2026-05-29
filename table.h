/* table.h - GFM pipe table parser + renderer */
#ifndef MD_TABLE_H
#define MD_TABLE_H

#include <stdio.h>
#include "parser.h"

struct md_inline_ctx;  /* forward decl */

typedef enum {
    MD_ALIGN_LEFT = 0,
    MD_ALIGN_CENTER,
    MD_ALIGN_RIGHT
} md_align_t;

/* Cell: an arena of bytes per cell. Each row is a contiguous array of
 * pointers into a flat cell buffer. */
typedef struct {
    char       **cells;        /* row r, col c at cells[r * col_count + c] */
    size_t       row_count;
    size_t       col_count;
    md_align_t  *aligns;       /* one per column */
    int          has_alignment;
} md_table_t;

void md_table_init(md_table_t *t);
void md_table_free(md_table_t *t);

/* Parse the raw lines of a table block into table struct. */
void md_table_parse(const md_lines_t *lines, md_table_t *out);

/* Render with box-drawing borders. Cells get inline-expanded via ictx.
 * max_width is the terminal-width budget: when > 0 and the table's natural
 * width exceeds it, overlong columns are word-wrapped to fit. max_width <= 0
 * disables wrapping (byte-identical to the unbudgeted layout). */
void md_table_render(const md_table_t *t, FILE *fp,
                     const struct md_inline_ctx *ictx, int max_width);

#endif /* MD_TABLE_H */
/*fin*/
