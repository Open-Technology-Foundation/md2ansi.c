/* ansi.h - terminal detection, color palette, ANSI stripping */
#ifndef MD_ANSI_H
#define MD_ANSI_H

#include <stddef.h>
#include "md_common.h"

typedef enum {
    MD_COLOR_AUTO = 0,
    MD_COLOR_ALWAYS,
    MD_COLOR_NEVER
} md_color_mode_t;

typedef struct {
    /* basic attributes */
    const char *reset, *bold, *dim, *italic, *underline, *strike;
    /* header colors h1..h6 */
    const char *h1, *h2, *h3, *h4, *h5, *h6;
    /* element colors */
    const char *text, *blockquote, *codeblock, *list, *hr, *table, *link;
    /* syntax colors */
    const char *keyword, *string, *number, *comment, *function, *class, *builtin;
} md_palette_t;

extern md_palette_t md_pal;
extern int          md_color_on;

/* Initialize palette: if `on` is non-zero, fields point to ANSI codes;
 * otherwise fields point to empty strings. */
void md_palette_init(int on);

/* Decide whether color should be on based on mode + environment.
 * Mode: AUTO → NO_COLOR / TTY / TERM heuristic. ALWAYS → on. NEVER → off. */
int  md_color_decide(md_color_mode_t mode);

/* Detect terminal width. Priority: TIOCGWINSZ → $COLUMNS → 80.
 * Clamped to [20, 500]. */
int  md_term_width(void);

/* Strip ANSI SGR / CSI escape sequences from `src` (len bytes) into `out`.
 * Appends to out; does not pre-reset it. */
void md_strip_ansi(const char *src, size_t len, md_buf_t *out);

/* Return the visible byte length of `src` (len bytes) after ANSI stripping.
 * NOT the same as visible width — use md_visible_width for that. */
size_t md_visible_bytes(const char *src, size_t len);

#endif /* MD_ANSI_H */
/*fin*/
