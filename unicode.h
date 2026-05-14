/* unicode.h - UTF-8 decode + visible-width measurement */
#ifndef MD_UNICODE_H
#define MD_UNICODE_H

#include <stddef.h>
#include <stdint.h>

/* One-time locale initialization needed for wcwidth(). Idempotent. */
void md_unicode_init(void);

/* Decode one UTF-8 codepoint starting at src[0..len-1].
 * On success: writes codepoint to *cp and returns bytes consumed (1..4).
 * On malformed sequence: writes 0xFFFD to *cp and returns 1 (skip one byte). */
size_t md_utf8_decode(const char *src, size_t len, uint32_t *cp);

/* Width of one codepoint: 0 (combining/zero-width), 1 (normal), 2 (wide).
 * Negative wcwidth() results are clamped to 1 (so unprintables still
 * count as something — they will display as a glyph in most terminals). */
int    md_codepoint_width(uint32_t cp);

/* Visible terminal width of UTF-8 text, skipping ANSI CSI/OSC sequences.
 * Combining marks contribute 0; CJK chars contribute 2. */
size_t md_visible_width(const char *src, size_t len);

/* Width of a single UTF-8 token (one codepoint). Returns bytes consumed
 * via *advance, width via return value. Caller pre-checks that src[0] is
 * not 0x1b (call site handles ANSI). */
size_t md_token_width(const char *src, size_t len, size_t *advance);

#endif /* MD_UNICODE_H */
/*fin*/
