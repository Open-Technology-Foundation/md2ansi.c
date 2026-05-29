/* md_common.c - shared helpers */
#include "md_common.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

const char *md_prog  = "md2ansi";
int         md_debug = 0;

/* ----- messaging ----- */

static int stderr_isatty(void) {
    return isatty(STDERR_FILENO);
}

static void msg_v(const char *icon_open, const char *icon, const char *icon_close,
                  const char *fmt, va_list ap) {
    fprintf(stderr, "%s: ", md_prog);
    if (stderr_isatty()) fputs(icon_open, stderr);
    fputs(icon, stderr);
    if (stderr_isatty()) fputs(icon_close, stderr);
    fputc(' ', stderr);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
}

void md_warn(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    msg_v("\033[0;33m", "\xe2\x96\xb2", "\033[0m", fmt, ap);
    va_end(ap);
}

void md_error(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    msg_v("\033[0;31m", "\xe2\x9c\x97", "\033[0m", fmt, ap);
    va_end(ap);
}

void md_die(int code, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    msg_v("\033[0;31m", "\xe2\x9c\x97", "\033[0m", fmt, ap);
    va_end(ap);
    exit(code);
}

void md_debugf(const char *fmt, ...) {
    if (!md_debug) return;

    static unsigned long seq = 0;
    seq++;

    time_t now = time(NULL);
    struct tm tm; localtime_r(&now, &tm);
    char ts[16];
    strftime(ts, sizeof ts, "%H:%M:%S", &tm);

    int tty = stderr_isatty();
    fprintf(stderr, "[%s.%lu] %s\xe2\xa6\xbf%s ",
            ts, seq,
            tty ? "\033[0;33m" : "",
            tty ? "\033[0m"    : "");
    va_list ap; va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}

/* ----- md_buf_t ----- */

void md_buf_init(md_buf_t *b) {
    b->data = NULL;
    b->len  = 0;
    b->cap  = 0;
}

void md_buf_free(md_buf_t *b) {
    free(b->data);
    b->data = NULL;
    b->len  = 0;
    b->cap  = 0;
}

void md_buf_reset(md_buf_t *b) {
    b->len = 0;
    if (b->data && b->cap > 0) b->data[0] = '\0';
}

void md_buf_reserve(md_buf_t *b, size_t additional) {
    size_t need = b->len + additional + 1;
    if (need <= b->cap) return;
    size_t newcap = b->cap ? b->cap : 64;
    while (newcap < need) {
        if (newcap > SIZE_MAX / 2) { newcap = need; break; } /* clamp: no overflow */
        newcap *= 2;
    }
    char *p = realloc(b->data, newcap);
    if (!p) md_die(MD_EX_INVAL, "out of memory");
    b->data = p;
    b->cap  = newcap;
}

void md_buf_append_byte(md_buf_t *b, char c) {
    md_buf_reserve(b, 1);
    b->data[b->len++] = c;
    b->data[b->len]   = '\0';
}

void md_buf_append(md_buf_t *b, const char *data, size_t len) {
    if (!len) return;
    md_buf_reserve(b, len);
    memcpy(b->data + b->len, data, len);
    b->len += len;
    b->data[b->len] = '\0';
}

void md_buf_append_str(md_buf_t *b, const char *s) {
    if (s) md_buf_append(b, s, strlen(s));
}

void md_buf_append_fmt(md_buf_t *b, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    va_list ap2; va_copy(ap2, ap);
    int n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (n <= 0) { va_end(ap2); return; }
    md_buf_reserve(b, (size_t)n);
    vsnprintf(b->data + b->len, (size_t)n + 1, fmt, ap2);
    va_end(ap2);
    b->len += (size_t)n;
}

/* ----- md_lines_t ----- */

void md_lines_init(md_lines_t *ls) {
    ls->items = NULL;
    ls->count = 0;
    ls->cap   = 0;
}

void md_lines_free(md_lines_t *ls) {
    free(ls->items);
    ls->items = NULL;
    ls->count = 0;
    ls->cap   = 0;
}

void md_lines_push(md_lines_t *ls, const char *start, size_t len) {
    if (ls->count == ls->cap) {
        size_t newcap = ls->cap ? ls->cap * 2 : 64;
        md_line_t *p = realloc(ls->items, newcap * sizeof *p);
        if (!p) md_die(MD_EX_INVAL, "out of memory");
        ls->items = p;
        ls->cap   = newcap;
    }
    ls->items[ls->count].start = start;
    ls->items[ls->count].len   = len;
    ls->count++;
}

void md_split_lines(const char *src, size_t src_len, md_lines_t *out) {
    size_t i = 0;
    while (i < src_len) {
        size_t start = i;
        while (i < src_len && src[i] != '\n') i++;
        size_t end = i;
        if (end > start && src[end - 1] == '\r') end--;
        md_lines_push(out, src + start, end - start);
        if (i < src_len) i++; /* skip '\n' */
    }
    /* If source ended with '\n', preserve a trailing empty line only if the
     * source was completely empty. Bash's `read` loop drops the trailing
     * newline; we match that. */
}

/* ----- slurpers ----- */

char *md_slurp_file(const char *path, size_t max_bytes, size_t *out_len) {
    /* Open first, then fstat the SAME descriptor: one inode observed through
     * the size/type checks and the read, so a hostile symlink swap between
     * checks (TOCTOU) cannot bypass the directory rejection or size cap. */
    int fd = open(path, O_RDONLY | O_NOCTTY);
    if (fd < 0) {
        if (errno == ENOENT || errno == ENOTDIR) {
            md_die(MD_EX_NOTFOUND, "File not found '%s'", path);
        }
        if (errno == EACCES) {
            md_die(MD_EX_NOREAD, "Cannot read file '%s'", path);
        }
        md_die(MD_EX_NOREAD, "Cannot open '%s': %s", path, strerror(errno));
    }

    struct stat st;
    if (fstat(fd, &st) != 0) {
        int e = errno; close(fd);
        md_die(MD_EX_NOREAD, "Cannot stat '%s': %s", path, strerror(e));
    }
    if (S_ISDIR(st.st_mode)) {
        close(fd);
        md_die(MD_EX_ISDIR, "'%s' is a directory, not a file", path);
    }
    if ((size_t)st.st_size > max_bytes) {
        close(fd);
        md_die(MD_EX_TOOBIG,
               "File too large: %lld bytes (maximum: %zu bytes / 10MB)",
               (long long)st.st_size, max_bytes);
    }

    FILE *fp = fdopen(fd, "rb");
    if (!fp) {
        int e = errno; close(fd);
        md_die(MD_EX_NOREAD, "Cannot open '%s': %s", path, strerror(e));
    }

    size_t n = (size_t)st.st_size;
    char  *buf = malloc(n + 1);
    if (!buf) { fclose(fp); md_die(MD_EX_INVAL, "out of memory"); }

    size_t got = fread(buf, 1, n, fp);
    fclose(fp);
    buf[got] = '\0';
    if (out_len) *out_len = got;
    return buf;
}

size_t md_ansi_skip(const char *s, size_t i, size_t len) {
    /* Caller guarantees s[i] == 0x1b (ESC). */
    if (i + 1 >= len) return i + 1; /* lone trailing ESC */
    unsigned char n = (unsigned char)s[i + 1];
    if (n == '[') {
        /* CSI: skip until final byte 0x40-0x7e */
        i += 2;
        while (i < len) {
            unsigned char x = (unsigned char)s[i++];
            if (x >= 0x40 && x <= 0x7e) break;
        }
        return i;
    }
    if (n == ']' || n == 'P' || n == '_' || n == '^' || n == 'X') {
        /* String controls: OSC(]) DCS(P) APC(_) PM(^) SOS(X).
         * Terminated by BEL (0x07) or ST (ESC \). */
        i += 2;
        while (i < len) {
            unsigned char x = (unsigned char)s[i];
            if (x == 0x07) return i + 1;
            if (x == 0x1b && i + 1 < len && (unsigned char)s[i + 1] == '\\') {
                return i + 2;
            }
            i++;
        }
        return i;
    }
    /* Two-byte escape */
    return i + 2;
}

size_t md_strip_ansi_inplace(char *buf, size_t len) {
    size_t out = 0;
    size_t i = 0;
    while (i < len) {
        unsigned char c = (unsigned char)buf[i];
        if (c != 0x1b) { buf[out++] = (char)c; i++; continue; }
        i = md_ansi_skip(buf, i, len);
    }
    buf[out] = '\0'; /* re-terminate: safe, out <= len < cap */
    return out;
}

char *md_slurp_stdin(size_t max_bytes, size_t *out_len) {
    size_t cap  = 65536;
    size_t len  = 0;
    char  *buf  = malloc(cap);
    if (!buf) md_die(MD_EX_INVAL, "out of memory");

    while (1) {
        if (len == cap) {
            size_t newcap = cap * 2;
            if (newcap > max_bytes + 1) newcap = max_bytes + 1;
            if (newcap == cap) {
                /* Try one more byte to detect overflow */
                int c = fgetc(stdin);
                if (c == EOF) break;
                free(buf);
                md_die(MD_EX_TOOBIG,
                       "Input from stdin exceeds maximum size: %zu bytes (10MB)",
                       max_bytes);
            }
            char *p = realloc(buf, newcap);
            if (!p) { free(buf); md_die(MD_EX_INVAL, "out of memory"); }
            buf = p;
            cap = newcap;
        }
        size_t want = cap - len;
        size_t got  = fread(buf + len, 1, want, stdin);
        len += got;
        if (got < want) {
            if (feof(stdin)) break;
            if (ferror(stdin)) { free(buf); md_die(MD_EX_NOREAD, "stdin read error"); }
        }
        if (len > max_bytes) {
            free(buf);
            md_die(MD_EX_TOOBIG,
                   "Input from stdin exceeds maximum size: %zu bytes (10MB)",
                   max_bytes);
        }
    }

    char *p = realloc(buf, len + 1);
    if (p) buf = p;
    buf[len] = '\0';
    if (out_len) *out_len = len;
    return buf;
}

/*fin*/
