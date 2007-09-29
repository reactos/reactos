/*
 * Page tree, pages and related objects
 */

typedef struct pdf_pagetree_s pdf_pagetree;
typedef struct pdf_page_s pdf_page;
typedef struct pdf_textline_s pdf_textline;
typedef struct pdf_textchar_s pdf_textchar;

struct pdf_pagetree_s
{
	int count;
	int cursor;
	fz_obj **pref;
	fz_obj **pobj;
};

struct pdf_page_s
{
	fz_rect mediabox;
	int rotate;
	fz_obj *resources;
	fz_tree *tree;
	pdf_comment *comments;
	pdf_link *links;
};

struct pdf_textchar_s
{
	fz_irect bbox;
	int c;
};

struct pdf_textline_s
{
	int len, cap;
	pdf_textchar *text;
	pdf_textline *next;
};

/* pagetree.c */
fz_error *pdf_loadpagetree(pdf_pagetree **pp, pdf_xref *xref);
int pdf_getpagecount(pdf_pagetree *pages);
fz_obj *pdf_getpageobject(pdf_pagetree *pages, int p);
void pdf_debugpagetree(pdf_pagetree *pages);
void pdf_droppagetree(pdf_pagetree *pages);

/* page.c */
fz_error *pdf_getpageinfo(pdf_xref *xref, fz_obj *dict, fz_rect *bboxp, int *rotatep);
fz_error *pdf_loadpage(pdf_page **pagep, pdf_xref *xref, fz_obj *ref);
void pdf_droppage(pdf_page *page);

/* unicode.c */
fz_error *pdf_loadtextfromtree(pdf_textline **linep, fz_tree *tree, fz_matrix ctm);
void pdf_debugtextline(pdf_textline *line);
fz_error *pdf_newtextline(pdf_textline **linep);
void pdf_droptextline(pdf_textline *line);

