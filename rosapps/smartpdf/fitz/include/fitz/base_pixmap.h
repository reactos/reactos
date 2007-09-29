/* TODO: move this into draw module */
/*
pixmaps have n components per pixel. the first is always alpha.
premultiplied alpha when rendering, but non-premultiplied for colorspace
conversions and rescaling.
*/

typedef struct fz_pixmap_s fz_pixmap;
typedef unsigned char fz_sample;

struct fz_pixmap_s
{
	int x, y, w, h, n;
	fz_sample *samples;
};

fz_error *fz_newpixmapwithrect(fz_pixmap **mapp, fz_irect bbox, int n);
fz_error *fz_newpixmap(fz_pixmap **mapp, int x, int y, int w, int h, int n);
fz_error *fz_newpixmapcopy(fz_pixmap **pixp, fz_pixmap *old);

void fz_debugpixmap(fz_pixmap *map);
void fz_clearpixmap(fz_pixmap *map);
void fz_droppixmap(fz_pixmap *map);

fz_error *fz_scalepixmap(fz_pixmap **dstp, fz_pixmap *src, int xdenom, int ydenom);

