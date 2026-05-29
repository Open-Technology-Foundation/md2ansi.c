/* unicode.c - UTF-8 + wcwidth */
#include "unicode.h"
#include "md_common.h"

#include <locale.h>
#include <wchar.h>

void md_unicode_init(void) {
    static int done = 0;
    if (done) return;
    setlocale(LC_CTYPE, "");
    done = 1;
}

size_t md_utf8_decode(const char *src, size_t len, uint32_t *cp) {
    if (len == 0) { *cp = 0; return 0; }
    unsigned char b0 = (unsigned char)src[0];

    if (b0 < 0x80) { *cp = b0; return 1; }

    size_t  need;
    uint32_t v;
    unsigned char mask;

    if      ((b0 & 0xe0) == 0xc0) { need = 2; mask = 0x1f; }
    else if ((b0 & 0xf0) == 0xe0) { need = 3; mask = 0x0f; }
    else if ((b0 & 0xf8) == 0xf0) { need = 4; mask = 0x07; }
    else { *cp = 0xfffd; return 1; }

    if (len < need) { *cp = 0xfffd; return 1; }

    v = (uint32_t)(b0 & mask);
    for (size_t i = 1; i < need; i++) {
        unsigned char b = (unsigned char)src[i];
        if ((b & 0xc0) != 0x80) { *cp = 0xfffd; return 1; }
        v = (v << 6) | (uint32_t)(b & 0x3f);
    }
    /* Reject surrogates, overlong encodings, and out-of-range (> U+10FFFF):
     * emit U+FFFD. Malformed input never crashes the renderer and the decoder
     * always advances >= 1 byte, so no scanner can loop on bad bytes. */
    static const uint32_t mincp[5] = { 0, 0, 0x80, 0x800, 0x10000 };
    if (v < mincp[need])            { *cp = 0xfffd; return need; }
    if (v >= 0xd800 && v <= 0xdfff) { *cp = 0xfffd; return need; }
    if (v > 0x10ffff)               { *cp = 0xfffd; return need; }
    *cp = v;
    return need;
}

int md_codepoint_width(uint32_t cp) {
    if (cp == 0) return 0;
    int w = wcwidth((wchar_t)cp);
    if (w < 0) return 1;
    return w;
}

size_t md_visible_width(const char *src, size_t len) {
    size_t i = 0, total = 0;
    while (i < len) {
        unsigned char c = (unsigned char)src[i];
        if (c == 0x1b) { i = md_ansi_skip(src, i, len); continue; }
        uint32_t cp;
        size_t adv = md_utf8_decode(src + i, len - i, &cp);
        if (adv == 0) break;
        total += (size_t)md_codepoint_width(cp);
        i += adv;
    }
    return total;
}

size_t md_token_width(const char *src, size_t len, size_t *advance) {
    uint32_t cp;
    size_t adv = md_utf8_decode(src, len, &cp);
    if (advance) *advance = adv;
    return (size_t)md_codepoint_width(cp);
}

/*fin*/
