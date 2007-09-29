typedef struct fz_matrix_s fz_matrix;
typedef struct fz_point_s fz_point;
typedef struct fz_rect_s fz_rect;
typedef struct fz_ipoint_s fz_ipoint;
typedef struct fz_irect_s fz_irect;

extern fz_rect fz_emptyrect;
extern fz_rect fz_infiniterect;

#define fz_isemptyrect(r) ((r).x0 == (r).x1)
#define fz_isinfiniterect(r) ((r).x0 > (r).x1)

/*
	/ a b 0 \
	| c d 0 |
	\ e f 1 /
*/
struct fz_matrix_s
{
	float a, b, c, d, e, f;
};

struct fz_point_s
{
	float x, y;
};

struct fz_rect_s
{
	float x0, y0;
	float x1, y1;
};

struct fz_ipoint_s
{
	int x, y;
};

struct fz_irect_s
{
	int x0, y0;
	int x1, y1;
};

void fz_invert3x3(float *dst, float *m);

fz_matrix fz_concat(fz_matrix one, fz_matrix two);
fz_matrix fz_identity(void);
fz_matrix fz_scale(float sx, float sy);
fz_matrix fz_rotate(float theta);
fz_matrix fz_translate(float tx, float ty);
fz_matrix fz_invertmatrix(fz_matrix m);
int fz_isrectilinear(fz_matrix m);
float fz_matrixexpansion(fz_matrix m);

fz_rect fz_intersectrects(fz_rect a, fz_rect b);
fz_rect fz_mergerects(fz_rect a, fz_rect b);

fz_irect fz_roundrect(fz_rect r);
fz_irect fz_intersectirects(fz_irect a, fz_irect b);
fz_irect fz_mergeirects(fz_irect a, fz_irect b);

fz_point fz_transformpoint(fz_matrix m, fz_point p);
fz_rect fz_transformaabb(fz_matrix m, fz_rect r);

