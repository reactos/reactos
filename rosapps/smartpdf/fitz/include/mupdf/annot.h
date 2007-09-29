/*
 * Interactive features
 */

typedef struct pdf_link_s pdf_link;
typedef struct pdf_comment_s pdf_comment;
typedef struct pdf_widget_s pdf_widget;
typedef struct pdf_outline_s pdf_outline;

typedef enum pdf_linkkind_e
{
	PDF_LGOTO,
	PDF_LURI,
	PDF_LUNKNOWN
} pdf_linkkind;

struct pdf_link_s
{
	pdf_linkkind kind;
	fz_rect rect;
	fz_obj *dest;
	pdf_link *next;
};

typedef enum pdf_commentkind_e
{
	PDF_CTEXT,
	PDF_CFREETEXT,
	PDF_CLINE,
	PDF_CSQUARE,
	PDF_CCIRCLE,
	PDF_CPOLYGON,
	PDF_CPOLYLINE,
	PDF_CMARKUP,
	PDF_CCARET,
	PDF_CSTAMP,
	PDF_CINK
} pdf_commentkind;

struct pdf_comment_s
{
	pdf_commentkind kind;
	fz_rect rect;
	fz_rect popup;
	fz_obj *contents;
	pdf_comment *next;
};

struct pdf_outline_s
{
	char *title;
	pdf_link *link;
	pdf_outline *child;
	pdf_outline *next;
};

fz_error *pdf_loadnametree(fz_obj **dictp, pdf_xref *xref, fz_obj *root);
fz_error *pdf_loadnametrees(pdf_xref *xref);

fz_error *pdf_newlink(pdf_link**, fz_rect rect, fz_obj *dest, pdf_linkkind kind);
fz_error *pdf_loadlink(pdf_link **linkp, pdf_xref *xref, fz_obj *dict);
void pdf_droplink(pdf_link *link);

fz_error *pdf_loadoutline(pdf_outline **outlinep, pdf_xref *xref);
void pdf_debugoutline(pdf_outline *outline, int level);
void pdf_dropoutline(pdf_outline *outline);

fz_error *pdf_loadannots(pdf_comment **, pdf_link **, pdf_xref *, fz_obj *annots);

