/*
 * Fitz display tree text node.
 *
 * The text node is an optimization to reference glyphs in a font resource
 * and specifying an individual transform matrix for each one.
 *
 * The trm field contains the a, b, c and d coefficients.
 * The e and f coefficients come from the individual elements,
 * together they form the transform matrix for the glyph.
 *
 * Glyphs are referenced by glyph ID.
 * The Unicode text equivalent is kept in a separate array
 * with indexes into the glyph array.
 *

TODO the unicode textels

struct fz_textgid_s { float e, f; int gid; };
struct fz_textucs_s { int idx; int ucs; };

 */

typedef struct fz_textel_s fz_textel;

struct fz_textel_s
{
	float x, y;
	int cid;
};

struct fz_textnode_s
{
	fz_node super;
	fz_font *font;
	fz_matrix trm;
	int len, cap;
	fz_textel *els;
};

fz_error *fz_newtextnode(fz_textnode **textp, fz_font *face);
fz_error *fz_clonetextnode(fz_textnode **textp, fz_textnode *oldtext);
fz_error *fz_addtext(fz_textnode *text, int g, float x, float y);
fz_error *fz_endtext(fz_textnode *text);

