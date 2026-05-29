/* md2ansi.c - C implementation of md2ansi. Bash CLI parity + gap fixes. */
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "md_common.h"
#include "ansi.h"
#include "unicode.h"
#include "parser.h"
#include "render.h"

/* CLI parse state — populated from argv, then consumed by main loop. */
typedef struct {
    int  width_force;            /* 0 = auto */
    int  plain;
    int  no_footnotes;
    int  no_syntax;
    int  no_tables;
    int  no_tasks;
    int  no_images;
    int  no_links;
    int  bare_urls;              /* default 1; --no-bare-urls → 0 */
    int  no_table_wrap;          /* default 0; --no-table-wrap → 1 */
    md_color_mode_t color_mode;
    /* file list */
    char **files;
    int    file_count;
} md_cli_opts_t;

static void show_version(void) {
    printf("%s %s\n", md_prog, MD_VERSION);
}

static void show_help(void) {
    printf(
"%s %s - Convert Markdown to ANSI-colored terminal output\n"
"\n"
"A zero-dependency C implementation that renders markdown files with color\n"
"and style directly in your terminal.\n"
"\n"
"Usage: %s [OPTIONS] [file1.md [file2.md ...]]\n"
"       cat README.md | %s\n"
"       curl -s https://example.com/file.md | %s\n"
"\n"
"Options:\n"
"  -h, --help              Show this help message and exit\n"
"  -V, --version           Show version information and exit\n"
"  -D, --debug             Enable debug mode with detailed traces (to stderr)\n"
"  -w, --width WIDTH       Force specific terminal width (default: auto-detect)\n"
"  -t, --plain             Plain text mode (disable all feature toggles)\n"
"      --color=MODE        auto (default), always, or never\n"
"\n"
"Feature Toggles:\n"
"  --no-footnotes          Disable footnotes processing\n"
"  --no-syntax-highlight   Disable syntax highlighting in code blocks\n"
"  --no-tables             Disable table formatting\n"
"  --no-task-lists         Disable task list (checkbox) formatting\n"
"  --no-images             Disable image placeholders\n"
"  --no-links              Disable link formatting\n"
"  --no-bare-urls          Disable bare URL autolinking (http://, https://)\n"
"  --no-table-wrap         Disable wrapping overlong table cells to terminal width\n"
"\n"
"Examples:\n"
"  %s README.md | less -R          # view with pager\n"
"  %s *.md                          # multiple files\n"
"  cat doc.md | %s                  # stdin\n"
"  %s --width 100 README.md         # forced width\n"
"  %s --no-tables doc.md            # selective disable\n"
"  %s --plain README.md             # all features off\n"
"\n"
"Security:\n"
"  - Files larger than 10MB are rejected for safety\n"
"  - Input from stdin is also limited to 10MB\n"
"  - ANSI escape sequences in input are sanitized\n"
"  - NO_COLOR environment variable is honored\n",
        md_prog, MD_VERSION, md_prog, md_prog, md_prog,
        md_prog, md_prog, md_prog, md_prog, md_prog, md_prog);
}

static void apply_plain(md_cli_opts_t *o) {
    o->no_footnotes = 1;
    o->no_syntax    = 1;
    o->no_tables    = 1;
    o->no_tasks     = 1;
    o->no_images    = 1;
    o->no_links     = 1;
    o->bare_urls    = 0;
    o->no_table_wrap = 1;
}

static const char *prog_basename(const char *argv0) {
    const char *p = strrchr(argv0, '/');
    return p ? p + 1 : argv0;
}

/* getopt_long indices for --no-* options (>255 to avoid clash with chars). */
enum {
    OPT_NO_FOOTNOTES = 1000,
    OPT_NO_SYNTAX,
    OPT_NO_TABLES,
    OPT_NO_TASKLISTS,
    OPT_NO_IMAGES,
    OPT_NO_LINKS,
    OPT_NO_BARE_URLS,
    OPT_NO_TABLE_WRAP,
    OPT_COLOR
};

static void parse_cli(int argc, char **argv, md_cli_opts_t *o) {
    static const struct option longopts[] = {
        { "help",                no_argument,       0, 'h' },
        { "version",             no_argument,       0, 'V' },
        { "debug",               no_argument,       0, 'D' },
        { "width",               required_argument, 0, 'w' },
        { "plain",               no_argument,       0, 't' },
        { "no-footnotes",        no_argument,       0, OPT_NO_FOOTNOTES },
        { "no-syntax-highlight", no_argument,       0, OPT_NO_SYNTAX },
        { "no-tables",           no_argument,       0, OPT_NO_TABLES },
        { "no-task-lists",       no_argument,       0, OPT_NO_TASKLISTS },
        { "no-images",           no_argument,       0, OPT_NO_IMAGES },
        { "no-links",            no_argument,       0, OPT_NO_LINKS },
        { "no-bare-urls",        no_argument,       0, OPT_NO_BARE_URLS },
        { "no-table-wrap",       no_argument,       0, OPT_NO_TABLE_WRAP },
        { "color",               required_argument, 0, OPT_COLOR },
        { 0, 0, 0, 0 }
    };

    opterr = 0;  /* suppress getopt's own messages */
    int c;
    while ((c = getopt_long(argc, argv, ":hVDw:t", longopts, NULL)) != -1) {
        switch (c) {
        case 'h': show_help(); exit(MD_EX_OK);
        case 'V': show_version(); exit(MD_EX_OK);
        case 'D': md_debug = 1; break;
        case 'w': {
            char *end = NULL;
            long v = strtol(optarg, &end, 10);
            if (!end || *end != '\0' || v < 1) {
                md_die(MD_EX_INVAL, "Invalid width '%s'", optarg);
            }
            if (v < 20 || v > 500) {
                md_die(MD_EX_INVAL, "Width must be between 20 and 500");
            }
            o->width_force = (int)v;
            break;
        }
        case 't': apply_plain(o); o->plain = 1; break;
        case OPT_NO_FOOTNOTES: o->no_footnotes = 1; break;
        case OPT_NO_SYNTAX:    o->no_syntax    = 1; break;
        case OPT_NO_TABLES:    o->no_tables    = 1; break;
        case OPT_NO_TASKLISTS: o->no_tasks     = 1; break;
        case OPT_NO_IMAGES:    o->no_images    = 1; break;
        case OPT_NO_LINKS:     o->no_links     = 1; break;
        case OPT_NO_BARE_URLS: o->bare_urls    = 0; break;
        case OPT_NO_TABLE_WRAP: o->no_table_wrap = 1; break;
        case OPT_COLOR:
            if      (strcmp(optarg, "auto")   == 0) o->color_mode = MD_COLOR_AUTO;
            else if (strcmp(optarg, "always") == 0) o->color_mode = MD_COLOR_ALWAYS;
            else if (strcmp(optarg, "never")  == 0) o->color_mode = MD_COLOR_NEVER;
            else md_die(MD_EX_INVAL, "Invalid --color value '%s' (auto|always|never)",
                        optarg);
            break;
        case ':':
            /* missing required argument */
            md_die(MD_EX_NOARG, "Missing argument for option '%s'", argv[optind - 1]);
        case '?':
        default:
            if (optopt && optopt != '?') {
                md_die(MD_EX_INVAL, "Invalid option '-%c'", optopt);
            } else if (optind > 0 && optind <= argc) {
                md_die(MD_EX_INVAL, "Invalid option '%s'", argv[optind - 1]);
            } else {
                md_die(MD_EX_INVAL, "Invalid option");
            }
        }
    }
    if (optind < argc) {
        o->files      = &argv[optind];
        o->file_count = argc - optind;
    }
}

/* Strip leading shebang line if present (matches Bash version). */
static void strip_shebang(md_lines_t *lines) {
    if (lines->count == 0) return;
    const md_line_t *first = &lines->items[0];
    if (first->len >= 2 && first->start[0] == '#' && first->start[1] == '!') {
        memmove(lines->items, lines->items + 1,
                (lines->count - 1) * sizeof(md_line_t));
        lines->count--;
    }
}

static void process_buffer(char *buf, size_t len,
                           const md_render_opts_t *r_opts) {
    /* Strip ANSI escapes from input buffer in-place (defense in depth) */
    len = md_strip_ansi_inplace(buf, len);
    buf[len] = '\0';

    md_lines_t lines;
    md_lines_init(&lines);
    md_split_lines(buf, len, &lines);
    strip_shebang(&lines);

    md_doc_t doc;
    md_doc_init(&doc);
    md_parse(&lines, &doc);
    md_render(&doc, r_opts, stdout);
    md_doc_free(&doc);
    md_lines_free(&lines);
}

int main(int argc, char **argv) {
    md_prog = prog_basename(argv[0]);
    md_unicode_init();

    md_cli_opts_t o = {0};
    o.color_mode = MD_COLOR_AUTO;
    o.bare_urls  = 1;
    parse_cli(argc, argv, &o);

    md_palette_init(md_color_decide(o.color_mode));

    md_render_opts_t r_opts = {
        .width        = o.width_force ? o.width_force : md_term_width(),
        .no_footnotes = o.no_footnotes,
        .no_syntax    = o.no_syntax,
        .no_tables    = o.no_tables,
        .no_tasks     = o.no_tasks,
        .no_images    = o.no_images,
        .no_links     = o.no_links,
        .bare_urls    = o.bare_urls,
        .no_table_wrap = o.no_table_wrap
    };

    md_debugf("width=%d color=%d files=%d",
              r_opts.width, md_color_on, o.file_count);

    /* Initial reset for clean terminal state */
    fputs(md_pal.reset, stdout);

    if (o.file_count > 0) {
        for (int i = 0; i < o.file_count; i++) {
            size_t blen;
            char  *buf = md_slurp_file(o.files[i], MD_MAX_FILE_SIZE, &blen);
            process_buffer(buf, blen, &r_opts);
            free(buf);
            if (o.file_count > 1) fputc('\n', stdout);
        }
    } else {
        size_t blen;
        char  *buf = md_slurp_stdin(MD_MAX_FILE_SIZE, &blen);
        process_buffer(buf, blen, &r_opts);
        free(buf);
    }

    fputs(md_pal.reset, stdout);
    return 0;
}

/*fin*/
