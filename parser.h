/* parser.h - markdown line classifier and block builder */
#ifndef MD_PARSER_H
#define MD_PARSER_H

#include "md_common.h"

typedef enum {
    MD_B_EMPTY = 0,
    MD_B_PARA,
    MD_B_H1, MD_B_H2, MD_B_H3, MD_B_H4, MD_B_H5, MD_B_H6,
    MD_B_HR,
    MD_B_BLOCKQUOTE,
    MD_B_UL_ITEM,
    MD_B_OL_ITEM,
    MD_B_TASK_ITEM,
    MD_B_CODE_FENCED,
    MD_B_CODE_INDENTED,
    MD_B_TABLE,
    MD_B_FOOTNOTE_DEF,
    MD_B_FRONTMATTER
} md_block_kind_t;

typedef struct md_block {
    md_block_kind_t  kind;
    int              level;
    int              ordinal;
    int              checked;
    char            *lang;
    md_buf_t         content;        /* joined content (one buf per block) */
    md_lines_t       lines;          /* raw lines (for code/table) */
    void            *extra;
    struct md_block *next;
} md_block_t;

typedef struct md_link_ref {
    char *id;
    char *url;
    char *title;
    struct md_link_ref *next;
} md_link_ref_t;

typedef struct md_footnote_def {
    char *id;
    char *text;
    struct md_footnote_def *next;
} md_footnote_def_t;

typedef struct md_str_list_node {
    char *s;
    struct md_str_list_node *next;
} md_str_list_node_t;

typedef struct {
    md_block_t          *head;
    md_block_t          *tail;
    md_link_ref_t       *link_refs;
    md_footnote_def_t   *footnote_defs;
    md_str_list_node_t  *footnote_refs;   /* appearance order */
    md_str_list_node_t  *footnote_refs_tail;
} md_doc_t;

void md_doc_init(md_doc_t *doc);
void md_doc_free(md_doc_t *doc);

/* Append a block to the document; the doc takes ownership of `b`. */
void md_doc_append(md_doc_t *doc, md_block_t *b);

/* Append unique footnote ref (in appearance order). */
void md_doc_add_footnote_ref(md_doc_t *doc, const char *id);

/* Add a link reference definition. */
void md_doc_add_link_ref(md_doc_t *doc,
                         const char *id, const char *url, const char *title);
const md_link_ref_t *md_doc_find_link_ref(const md_doc_t *doc, const char *id);

/* Add a footnote definition. */
void md_doc_add_footnote_def(md_doc_t *doc, const char *id, const char *text);
const md_footnote_def_t *md_doc_find_footnote_def(const md_doc_t *doc,
                                                  const char *id);

/* Parse the line array into the document. */
void md_parse(const md_lines_t *lines, md_doc_t *doc);

#endif /* MD_PARSER_H */
/*fin*/
