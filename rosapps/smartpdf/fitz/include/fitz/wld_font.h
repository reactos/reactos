typedef struct fz_font_s fz_font;
typedef struct fz_hmtx_s fz_hmtx;
typedef struct fz_vmtx_s fz_vmtx;
typedef struct fz_glyph_s fz_glyph;
typedef struct fz_glyphcache_s fz_glyphcache;

struct fz_hmtx_s
{
	unsigned short lo;
	unsigned short hi;
	int w;	/* type3 fonts can be big! */
};

struct fz_vmtx_s
{
	unsigned short lo;
	unsigned short hi;
	short x;
	short y;
	short w;
};

struct fz_font_s
{
	int refs;
	char name[32];

	fz_error* (*render)(fz_glyph*, fz_font*, int, fz_matrix);
	void (*drop)(fz_font *);

	int wmode;
	fz_irect bbox;

	int nhmtx, hmtxcap;
	fz_hmtx dhmtx;
	fz_hmtx *hmtx;

	int nvmtx, vmtxcap;
	fz_vmtx dvmtx;
	fz_vmtx *vmtx;
};

struct fz_glyph_s
{
	int x, y, w, h;
	unsigned char *samples;
};

void fz_initfont(fz_font *font, char *name);
fz_font *fz_keepfont(fz_font *font);
void fz_dropfont(fz_font *font);
void fz_debugfont(fz_font *font);
void fz_setfontwmode(fz_font *font, int wmode);
void fz_setfontbbox(fz_font *font, int xmin, int ymin, int xmax, int ymax);
void fz_setdefaulthmtx(fz_font *font, int w);
void fz_setdefaultvmtx(fz_font *font, int y, int w);
fz_error *fz_addhmtx(fz_font *font, int lo, int hi, int w);
fz_error *fz_addvmtx(fz_font *font, int lo, int hi, int x, int y, int w);
fz_error *fz_endhmtx(fz_font *font);
fz_error *fz_endvmtx(fz_font *font);
fz_hmtx fz_gethmtx(fz_font *font, int cid);
fz_vmtx fz_getvmtx(fz_font *font, int cid);

fz_error *fz_newglyphcache(fz_glyphcache **arenap, int slots, int size);
fz_error *fz_renderglyph(fz_glyphcache*, fz_glyph*, fz_font*, int, fz_matrix);
void fz_debugglyphcache(fz_glyphcache *);
void fz_dropglyphcache(fz_glyphcache *);

