/* md_common.h - shared helpers: messaging, buffers, line splitting, I/O */
#ifndef MD_COMMON_H
#define MD_COMMON_H

#include <stddef.h>
#include <stdio.h>

#define MD_VERSION       "1.0.2"
#define MD_MAX_FILE_SIZE (10UL * 1024UL * 1024UL)

/* Exit codes match Bash md2ansi 1.0.1 exactly. */
enum {
    MD_EX_OK        = 0,
    MD_EX_NOTFOUND  = 3,
    MD_EX_ISDIR     = 4,
    MD_EX_NOARG     = 8,
    MD_EX_TOOBIG    = 9,
    MD_EX_NOREAD    = 13,
    MD_EX_INVAL     = 22
};

/* Program-wide state. Set once from main(); read everywhere else. */
extern const char *md_prog;
extern int         md_debug;

/* Messaging. die() never returns. */
void md_warn(const char *fmt, ...);
void md_error(const char *fmt, ...);
void md_die(int code, const char *fmt, ...) __attribute__((noreturn));
void md_debugf(const char *fmt, ...);

/* Growable byte buffer. data is always NUL-terminated when len > 0. */
typedef struct {
    char  *data;
    size_t len;
    size_t cap;
} md_buf_t;

void md_buf_init(md_buf_t *b);
void md_buf_free(md_buf_t *b);
void md_buf_reset(md_buf_t *b);
void md_buf_reserve(md_buf_t *b, size_t additional);
void md_buf_append_byte(md_buf_t *b, char c);
void md_buf_append(md_buf_t *b, const char *data, size_t len);
void md_buf_append_str(md_buf_t *b, const char *s);
void md_buf_append_fmt(md_buf_t *b, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

/* A line in the slurped input. NOT NUL-terminated; use len. */
typedef struct {
    const char *start;
    size_t      len;
} md_line_t;

typedef struct {
    md_line_t *items;
    size_t     count;
    size_t     cap;
} md_lines_t;

void md_lines_init(md_lines_t *ls);
void md_lines_free(md_lines_t *ls);
void md_lines_push(md_lines_t *ls, const char *start, size_t len);

/* Split a NUL-terminated input buffer into newline-delimited lines.
 * Strips the trailing '\n' from each line (and '\r' if present).
 * A trailing unterminated chunk still becomes a line. */
void md_split_lines(const char *src, size_t src_len, md_lines_t *out);

/* Slurp a file or stdin into a malloc'd buffer. Returns NULL and calls
 * md_die() on hard errors (oversize, unreadable, etc.). */
char *md_slurp_file(const char *path, size_t max_bytes, size_t *out_len);
char *md_slurp_stdin(size_t max_bytes, size_t *out_len);

/* Strip ANSI escape sequences in-place from buf[0..len). Returns new length. */
size_t md_strip_ansi_inplace(char *buf, size_t len);

/* Given s[i] == 0x1b (ESC), return the index just past the escape sequence.
 * The single source of truth for the CSI / OSC / string-control / two-byte
 * escape walker shared by every ANSI strip + width routine. Handles:
 *   ESC [ <params> <final 0x40..0x7e>     (CSI)
 *   ESC ] | P | _ | ^ | X <data> BEL|ST   (OSC, DCS, APC, PM, SOS)
 *   ESC <single byte>                      (two-byte escape)
 * A lone trailing ESC advances by one. Never reads past `len`. */
size_t md_ansi_skip(const char *s, size_t i, size_t len);

#endif /* MD_COMMON_H */
/*fin*/
