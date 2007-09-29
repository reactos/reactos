/*
 * content stream parsing
 */

typedef struct pdf_material_s pdf_material;
typedef struct pdf_gstate_s pdf_gstate;
typedef struct pdf_csi_s pdf_csi;

enum
{
	PDF_MFILL,
	PDF_MSTROKE
};

enum
{
	PDF_MNONE,
	PDF_MCOLOR,
	PDF_MLAB,
	PDF_MINDEXED,
	PDF_MPATTERN,
	PDF_MSHADE
};

struct pdf_material_s
{
	int kind;
	fz_colorspace *cs;
	float v[32];
	pdf_indexed *indexed;
	pdf_pattern *pattern;
	fz_shade *shade;
};

struct pdf_gstate_s
{
	/* path stroking */
	float linewidth;
	int linecap;
	int linejoin;
	float miterlimit;
	float dashphase;
	int dashlen;
	float dashlist[32];

	/* materials */
	pdf_material stroke;
	pdf_material fill;

	/* text state */
	float charspace;
	float wordspace;
	float scale;
	float leading;
	pdf_font *font;
	float size;
	int render;
	float rise;

	/* tree construction state */
	fz_node *head;
};

struct pdf_csi_s
{
	pdf_gstate gstate[32];
	int gtop;
	fz_obj *stack[32];
	int top;
	int xbalance;
	fz_obj *array;

	/* path object state */
	fz_pathnode *path;
	int clip;

	/* text object state */
	fz_node *textclip;
	fz_textnode *text;
	fz_matrix tlm;
	fz_matrix tm;
	int textmode;

	fz_tree *tree;
};

/* build.c */
void pdf_initgstate(pdf_gstate *gs);
fz_error *pdf_setcolorspace(pdf_csi *csi, int what, fz_colorspace *cs);
fz_error *pdf_setcolor(pdf_csi *csi, int what, float *v);
fz_error *pdf_setpattern(pdf_csi *csi, int what, pdf_pattern *pat, float *v);
fz_error *pdf_setshade(pdf_csi *csi, int what, fz_shade *shade);

fz_error *pdf_buildstrokepath(pdf_gstate *gs, fz_pathnode *path);
fz_error *pdf_buildfillpath(pdf_gstate *gs, fz_pathnode *path, int evenodd);
fz_error *pdf_addfillshape(pdf_gstate *gs, fz_node *shape);
fz_error *pdf_addstrokeshape(pdf_gstate *gs, fz_node *shape);
fz_error *pdf_addclipmask(pdf_gstate *gs, fz_node *shape);
fz_error *pdf_addtransform(pdf_gstate *gs, fz_node *transform);
fz_error *pdf_addshade(pdf_gstate *gs, fz_shade *shade);
fz_error *pdf_showpath(pdf_csi*, int close, int fill, int stroke, int evenodd);
fz_error *pdf_showtext(pdf_csi*, fz_obj *text);
fz_error *pdf_flushtext(pdf_csi*);
fz_error *pdf_showimage(pdf_csi*, pdf_image *img);

/* interpret.c */
fz_error *pdf_newcsi(pdf_csi **csip, int maskonly);
fz_error *pdf_runcsi(pdf_csi *, pdf_xref *xref, fz_obj *rdb, fz_stream *);
void pdf_dropcsi(pdf_csi *csi);

