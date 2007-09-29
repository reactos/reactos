/*
 * Vector path nodes in the display tree.
 * They can be stroked and dashed, or be filled.
 * They have a fill rule (nonzero or evenodd).
 *
 * When rendering, they are flattened, stroked and dashed straight
 * into the Global Edge List.
 *
 * TODO flatten, stroke and dash into another path
 * TODO set operations on flat paths (union, intersect, difference)
 * TODO decide whether dashing should be part of the tree and renderer,
 *      or if it is something the client has to do (with a util function).
 */

typedef struct fz_stroke_s fz_stroke;
typedef struct fz_dash_s fz_dash;
typedef union fz_pathel_s fz_pathel;

typedef enum fz_pathkind_e
{
	FZ_STROKE,
	FZ_FILL,
	FZ_EOFILL
} fz_pathkind;

typedef enum fz_pathelkind_e
{
	FZ_MOVETO,
	FZ_LINETO,
	FZ_CURVETO,
	FZ_CLOSEPATH
} fz_pathelkind;

struct fz_stroke_s
{
	int linecap;
	int linejoin;
	float linewidth;
	float miterlimit;
};

struct fz_dash_s
{
	int len;
	float phase;
	float array[FZ_FLEX];
};

union fz_pathel_s
{
	fz_pathelkind k;
	float v;
};

struct fz_pathnode_s
{
	fz_node super;
	fz_pathkind paint;
	fz_dash *dash;
	int linecap;
	int linejoin;
	float linewidth;
	float miterlimit;
	int len, cap;
	fz_pathel *els;
};

fz_error *fz_newpathnode(fz_pathnode **pathp);
fz_error *fz_clonepathnode(fz_pathnode **pathp, fz_pathnode *oldpath);
fz_error *fz_moveto(fz_pathnode*, float x, float y);
fz_error *fz_lineto(fz_pathnode*, float x, float y);
fz_error *fz_curveto(fz_pathnode*, float, float, float, float, float, float);
fz_error *fz_curvetov(fz_pathnode*, float, float, float, float);
fz_error *fz_curvetoy(fz_pathnode*, float, float, float, float);
fz_error *fz_closepath(fz_pathnode*);
fz_error *fz_endpath(fz_pathnode*, fz_pathkind paint, fz_stroke *stroke, fz_dash *dash);

fz_rect fz_boundpathnode(fz_pathnode *node, fz_matrix ctm);
void fz_debugpathnode(fz_pathnode *node);

fz_error *fz_newdash(fz_dash **dashp, float phase, int len, float *array);
void fz_dropdash(fz_dash *dash);

