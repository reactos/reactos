typedef struct fz_renderer_s fz_renderer;

#define FZ_BYTE unsigned char

#define FZ_PSRC \
	unsigned char *src, int srcw, int srch
#define FZ_PDST \
	unsigned char *dst0, int dstw
#define FZ_PCTM \
	int u0, int v0, int fa, int fb, int fc, int fd, int w0, int h

/*
 * Function pointers -- they can be replaced by cpu-optimized versions
 */

extern void (*fz_duff_non)(FZ_BYTE*,int,int,FZ_BYTE*,int,int,int);
extern void (*fz_duff_nimcn)(FZ_BYTE*,int,int,FZ_BYTE*,int,int,FZ_BYTE*,int,int,int);
extern void (*fz_duff_nimon)(FZ_BYTE*,int,int,FZ_BYTE*,int,int,FZ_BYTE*,int,int,int);
extern void (*fz_duff_1o1)(FZ_BYTE*,int,FZ_BYTE*,int,int,int);
extern void (*fz_duff_4o4)(FZ_BYTE*,int,FZ_BYTE*,int,int,int);
extern void (*fz_duff_1i1c1)(FZ_BYTE*,int,FZ_BYTE*,int,FZ_BYTE*,int,int,int);
extern void (*fz_duff_4i1c4)(FZ_BYTE*,int,FZ_BYTE*,int,FZ_BYTE*,int,int,int);
extern void (*fz_duff_1i1o1)(FZ_BYTE*,int,FZ_BYTE*,int,FZ_BYTE*,int,int,int);
extern void (*fz_duff_4i1o4)(FZ_BYTE*,int,FZ_BYTE*,int,FZ_BYTE*,int,int,int);

extern void (*fz_path_1c1)(FZ_BYTE*,int,int,FZ_BYTE*);
extern void (*fz_path_1o1)(FZ_BYTE*,int,int,FZ_BYTE*);
extern void (*fz_path_w3i1o4)(FZ_BYTE*,FZ_BYTE*,int,int,FZ_BYTE*);

extern void (*fz_text_1c1)(FZ_BYTE*,int,FZ_BYTE*,int,int,int);
extern void (*fz_text_1o1)(FZ_BYTE*,int,FZ_BYTE*,int,int,int);
extern void (*fz_text_w3i1o4)(FZ_BYTE*,FZ_BYTE*,int,FZ_BYTE*,int,int,int);

extern void (*fz_img_ncn)(FZ_PSRC, int sn, FZ_PDST, FZ_PCTM);
extern void (*fz_img_1c1)(FZ_PSRC, FZ_PDST, FZ_PCTM);
extern void (*fz_img_4c4)(FZ_PSRC, FZ_PDST, FZ_PCTM);
extern void (*fz_img_1o1)(FZ_PSRC, FZ_PDST, FZ_PCTM);
extern void (*fz_img_4o4)(FZ_PSRC, FZ_PDST, FZ_PCTM);
extern void (*fz_img_w3i1o4)(FZ_BYTE*,FZ_PSRC,FZ_PDST,FZ_PCTM);

extern void (*fz_decodetile)(fz_pixmap *pix, int skip, float *decode);
extern void (*fz_loadtile1)(FZ_BYTE*, int sw, FZ_BYTE*, int dw, int w, int h, int pad);
extern void (*fz_loadtile2)(FZ_BYTE*, int sw, FZ_BYTE*, int dw, int w, int h, int pad);
extern void (*fz_loadtile4)(FZ_BYTE*, int sw, FZ_BYTE*, int dw, int w, int h, int pad);
extern void (*fz_loadtile8)(FZ_BYTE*, int sw, FZ_BYTE*, int dw, int w, int h, int pad);

extern void (*fz_srown)(FZ_BYTE *src, FZ_BYTE *dst, int w, int denom, int n);
extern void (*fz_srow1)(FZ_BYTE *src, FZ_BYTE *dst, int w, int denom);
extern void (*fz_srow2)(FZ_BYTE *src, FZ_BYTE *dst, int w, int denom);
extern void (*fz_srow4)(FZ_BYTE *src, FZ_BYTE *dst, int w, int denom);
extern void (*fz_srow5)(FZ_BYTE *src, FZ_BYTE *dst, int w, int denom);

extern void (*fz_scoln)(FZ_BYTE *src, FZ_BYTE *dst, int w, int denom, int n);
extern void (*fz_scol1)(FZ_BYTE *src, FZ_BYTE *dst, int w, int denom);
extern void (*fz_scol2)(FZ_BYTE *src, FZ_BYTE *dst, int w, int denom);
extern void (*fz_scol4)(FZ_BYTE *src, FZ_BYTE *dst, int w, int denom);
extern void (*fz_scol5)(FZ_BYTE *src, FZ_BYTE *dst, int w, int denom);

#undef FZ_BYTE

struct fz_renderer_s
{
	int maskonly;
	fz_colorspace *model;
	fz_glyphcache *cache;
	fz_gel *gel;
	fz_ael *ael;

	fz_irect clip;
	fz_pixmap *dest;
	fz_pixmap *over;
	unsigned char rgb[3];
	int flag;
};

extern void fz_accelerate();

fz_error *fz_newrenderer(fz_renderer **gcp, fz_colorspace *pcm, int maskonly, int gcmem);
void fz_droprenderer(fz_renderer *gc);
fz_error *fz_rendertree(fz_pixmap **out, fz_renderer *gc, fz_tree *tree, fz_matrix ctm, fz_irect bbox, int white);
fz_error *fz_rendertreeover(fz_renderer *gc, fz_pixmap *dest, fz_tree *tree, fz_matrix ctm);


