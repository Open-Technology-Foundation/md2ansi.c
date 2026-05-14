/* syntax.h - per-language syntax highlighters */
#ifndef MD_SYNTAX_H
#define MD_SYNTAX_H

#include "md_common.h"

/* Apply per-language highlighting to one line of code.
 * `lang` may be normalized: py→python, js→javascript, sh/shell→bash.
 * `out` is reset first. */
void md_syntax_highlight(const char *src, size_t len,
                         const char *lang, md_buf_t *out);

#endif /* MD_SYNTAX_H */
/*fin*/
