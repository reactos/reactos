typedef struct fz_colorspace_s fz_colorspace;
typedef struct fz_colorcube_s fz_colorcube;
typedef struct fz_colorcube1_s fz_colorcube1;
typedef struct fz_colorcube3_s fz_colorcube3;
typedef struct fz_colorcube4_s fz_colorcube4;

enum { FZ_MAXCOLORS = 32 };

struct fz_colorspace_s
{
	int refs;
	char name[16];
	int n;
	void (*convpixmap)(fz_colorspace *ss, fz_pixmap *sp, fz_colorspace *ds, fz_pixmap *dp);
	void (*convcolor)(fz_colorspace *ss, float *sv, fz_colorspace *ds, float *dv);
	void (*toxyz)(fz_colorspace *, float *src, float *xyz);
	void (*fromxyz)(fz_colorspace *, float *xyz, float *dst);
	void (*drop)(fz_colorspace *);
};

struct fz_colorcube1_s { unsigned char v[17]; };
struct fz_colorcube3_s { unsigned char v[17][17][17]; };
struct fz_colorcube4_s { unsigned char v[17][17][17][17]; };

struct fz_colorcube_s
{
	fz_colorspace *src;
	fz_colorspace *dst;
	void **subcube;			/* dst->n * colorcube(src->n) */
};

fz_colorspace *fz_keepcolorspace(fz_colorspace *cs);
void fz_dropcolorspace(fz_colorspace *cs);

void fz_convertcolor(fz_colorspace *srcs, float *srcv, fz_colorspace *dsts, float *dstv);
void fz_convertpixmap(fz_colorspace *srcs, fz_pixmap *srcv, fz_colorspace *dsts, fz_pixmap *dstv);

void fz_stdconvcolor(fz_colorspace *srcs, float *srcv, fz_colorspace *dsts, float *dstv);
void fz_stdconvpixmap(fz_colorspace *srcs, fz_pixmap *srcv, fz_colorspace *dsts, fz_pixmap *dstv);

