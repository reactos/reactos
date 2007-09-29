typedef struct fz_shade_s fz_shade;

struct fz_shade_s
{
	int refs;

	fz_rect bbox;		/* can be fz_infiniterect */
	fz_colorspace *cs;

	/* used by build.c -- not used in drawshade.c */
	fz_matrix matrix;	/* matrix from pattern dict */
	int usebackground;	/* background color for fills but not 'sh' */
	float background[FZ_MAXCOLORS];

	int usefunction;
	float function[256][FZ_MAXCOLORS];

	int meshlen;
	int meshcap;
	float *mesh; /* [x y t] or [x y c1 ... cn] * 3 * meshlen */
};


fz_shade *fz_keepshade(fz_shade *shade);
void fz_dropshade(fz_shade *shade);

fz_rect fz_boundshade(fz_shade *shade, fz_matrix ctm);
fz_error *fz_rendershade(fz_shade *shade, fz_matrix ctm, fz_colorspace *dsts, fz_pixmap *dstp);

