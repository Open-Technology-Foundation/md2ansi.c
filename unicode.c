/* unicode.c - UTF-8 + wcwidth */
#include "unicode.h"

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
    /* Reject surrogates & overlongs for the common cases */
    if (v >= 0xd800 && v <= 0xdfff) { *cp = 0xfffd; return need; }
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
        if (c == 0x1b) {
            /* Skip ANSI escape. */
            if (i + 1 >= len) { i++; continue; }
            unsigned char n = (unsigned char)src[i + 1];
            if (n == '[') {
                i += 2;
                while (i < len) {
                    unsigned char x = (unsigned char)src[i++];
                    if (x >= 0x40 && x <= 0x7e) break;
                }
            } else if (n == ']') {
                i += 2;
                while (i < len) {
                    unsigned char x = (unsigned char)src[i];
                    if (x == 0x07) { i++; break; }
                    if (x == 0x1b && i + 1 < len
                        && (unsigned char)src[i + 1] == '\\') { i += 2; break; }
                    i++;
                }
            } else {
                i += 2;
            }
            continue;
        }
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
