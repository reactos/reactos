typedef struct fz_edge_s fz_edge;
typedef struct fz_gel_s fz_gel;
typedef struct fz_ael_s fz_ael;

struct fz_edge_s
{
	int x, e, h, y;
	int adjup, adjdown;
	int xmove;
	int xdir, ydir;     /* -1 or +1 */
};

struct fz_gel_s
{
	int hs, vs;
	int xmin, xmax;
	int ymin, ymax;
	int cap;
	int len;
	fz_edge *edges;
};

struct fz_ael_s
{
	int cap;
	int len;
	fz_edge **edges;
};

fz_error *fz_newgel(fz_gel **gelp);
fz_error *fz_insertgel(fz_gel *gel, float x0, float y0, float x1, float y1);
fz_irect fz_boundgel(fz_gel *gel);
void fz_resetgel(fz_gel *gel, int hs, int vs);
void fz_sortgel(fz_gel *gel);
void fz_dropgel(fz_gel *gel);

fz_error *fz_newael(fz_ael **aelp);
void fz_dropael(fz_ael *ael);

fz_error *fz_scanconvert(fz_gel *gel, fz_ael *ael, int eofill,
	fz_irect clip, fz_pixmap *pix, unsigned char *rgb, int over);

fz_error *fz_fillpath(fz_gel *gel, fz_pathnode *path, fz_matrix ctm, float flatness);
fz_error *fz_strokepath(fz_gel *gel, fz_pathnode *path, fz_matrix ctm, float flatness);
fz_error *fz_dashpath(fz_gel *gel, fz_pathnode *path, fz_matrix ctm, float flatness);

