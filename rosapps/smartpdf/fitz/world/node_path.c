#include "fitz-base.h"
#include "fitz-world.h"

fz_error *
fz_newpathnode(fz_pathnode **pathp)
{
	fz_pathnode *path;

	path = *pathp = fz_malloc(sizeof(fz_pathnode));
	if (!path)
		return fz_outofmem;

	fz_initnode((fz_node*)path, FZ_NPATH);

	path->paint = FZ_FILL;
	path->linecap = 0;
	path->linejoin = 0;
	path->linewidth = 1.0;
	path->miterlimit = 10.0;
	path->dash = nil;
	path->len = 0;
	path->cap = 0;
	path->els = nil;

	return nil;
}

fz_error *
fz_clonepathnode(fz_pathnode **pathp, fz_pathnode *oldpath)
{
	fz_pathnode *path;

	path = *pathp = fz_malloc(sizeof(fz_pathnode));
	if (!path)
		return fz_outofmem;

	fz_initnode((fz_node*)path, FZ_NPATH);

	path->paint = FZ_FILL;
	path->linecap = 0;
	path->linejoin = 0;
	path->linewidth = 1.0;
	path->miterlimit = 10.0;
	path->dash = nil;
	path->len = oldpath->len;
	path->cap = oldpath->len;

	path->els = fz_malloc(sizeof (fz_pathel) * path->len);
	if (!path->els) {
		fz_free(path);
		return fz_outofmem;
	}
	memcpy(path->els, oldpath->els, sizeof(fz_pathel) * path->len);

	return nil;
}

void
fz_droppathnode(fz_pathnode *node)
{
	fz_free(node->dash);
	fz_free(node->els);
}

static fz_error *
growpath(fz_pathnode *path, int n)
{
	int newcap;
	fz_pathel *newels;

	while (path->len + n > path->cap)
	{
		newcap = path->cap + 36;
		newels = fz_realloc(path->els, sizeof (fz_pathel) * newcap);
		if (!newels)
			return fz_outofmem;
		path->cap = newcap;
		path->els = newels;
	}

	return nil;
}

fz_error *
fz_moveto(fz_pathnode *path, float x, float y)
{
	if (growpath(path, 3) != nil)
		return fz_outofmem;
	path->els[path->len++].k = FZ_MOVETO;
	path->els[path->len++].v = x;
	path->els[path->len++].v = y;
	return nil;
}

fz_error *
fz_lineto(fz_pathnode *path, float x, float y)
{
	if (growpath(path, 3) != nil)
		return fz_outofmem;
	path->els[path->len++].k = FZ_LINETO;
	path->els[path->len++].v = x;
	path->els[path->len++].v = y;
	return nil;
}

fz_error *
fz_curveto(fz_pathnode *path,
		float x1, float y1,
		float x2, float y2,
		float x3, float y3)
{
	if (growpath(path, 7) != nil)
		return fz_outofmem;
	path->els[path->len++].k = FZ_CURVETO;
	path->els[path->len++].v = x1;
	path->els[path->len++].v = y1;
	path->els[path->len++].v = x2;
	path->els[path->len++].v = y2;
	path->els[path->len++].v = x3;
	path->els[path->len++].v = y3;
	return nil;
}

fz_error *
fz_curvetov(fz_pathnode *path, float x2, float y2, float x3, float y3)
{
	float x1 = path->els[path->len-2].v;
	float y1 = path->els[path->len-1].v;
	return fz_curveto(path, x1, y1, x2, y2, x3, y3);
}

fz_error *
fz_curvetoy(fz_pathnode *path, float x1, float y1, float x3, float y3)
{
	return fz_curveto(path, x1, y1, x3, y3, x3, y3);
}

fz_error *
fz_closepath(fz_pathnode *path)
{
	if (growpath(path, 1) != nil)
		return fz_outofmem;
	path->els[path->len++].k = FZ_CLOSEPATH;
	return nil;
}

fz_error *
fz_endpath(fz_pathnode *path, fz_pathkind paint, fz_stroke *stroke, fz_dash *dash)
{
	fz_pathel *newels;

	newels = fz_realloc(path->els, path->len * sizeof(fz_pathel));
	if (!newels)
		return fz_outofmem;
	path->els = newels;

	path->paint = paint;
	path->dash = dash;
	if (stroke)
	{
		path->linecap = stroke->linecap;
		path->linejoin = stroke->linejoin;
		path->linewidth = stroke->linewidth;
		path->miterlimit = stroke->miterlimit;
	}

	if (path->linewidth < 0.01)
		path->linewidth = 0.01f;

	return nil;
}

static inline fz_rect boundexpand(fz_rect r, fz_point p)
{
	if (p.x < r.x0) r.x0 = p.x;
	if (p.y < r.y0) r.y0 = p.y;
	if (p.x > r.x1) r.x1 = p.x;
	if (p.y > r.y1) r.y1 = p.y;
	return r;
}

fz_rect
fz_boundpathnode(fz_pathnode *path, fz_matrix ctm)
{
	fz_point p;
	fz_rect r = fz_emptyrect;
	int i = 0;

	if (path->len)
	{
		p.x = path->els[1].v;
		p.y = path->els[2].v;
		p = fz_transformpoint(ctm, p);
		r.x0 = r.x1 = p.x;
		r.y0 = r.y1 = p.y;
	}

	while (i < path->len)
	{
		switch (path->els[i++].k)
		{
		case FZ_CURVETO:
			p.x = path->els[i++].v;
			p.y = path->els[i++].v;
			r = boundexpand(r, fz_transformpoint(ctm, p));
			p.x = path->els[i++].v;
			p.y = path->els[i++].v;
			r = boundexpand(r, fz_transformpoint(ctm, p));
		case FZ_MOVETO:
		case FZ_LINETO:
			p.x = path->els[i++].v;
			p.y = path->els[i++].v;
			r = boundexpand(r, fz_transformpoint(ctm, p));
			break;
		case FZ_CLOSEPATH:
			break;
		}
	}

	if (path->paint == FZ_STROKE)
	{
		float miterlength = sin(path->miterlimit / 2.0);
		float linewidth = path->linewidth;
		float expand = MAX(miterlength, linewidth) / 2.0;
		r.x0 -= expand;
		r.y0 -= expand;
		r.x1 += expand;
		r.y1 += expand;
	}

	return r;
}

void
fz_debugpathnode(fz_pathnode *path)
{
	float x, y;
	int i = 0;
	while (i < path->len)
	{
		switch (path->els[i++].k)
		{
		case FZ_MOVETO:
			x = path->els[i++].v;
			y = path->els[i++].v;
			printf("%g %g m\n", x, y);
			break;
		case FZ_LINETO:
			x = path->els[i++].v;
			y = path->els[i++].v;
			printf("%g %g l\n", x, y);
			break;
		case FZ_CURVETO:
			x = path->els[i++].v;
			y = path->els[i++].v;
			printf("%g %g ", x, y);
			x = path->els[i++].v;
			y = path->els[i++].v;
			printf("%g %g ", x, y);
			x = path->els[i++].v;
			y = path->els[i++].v;
			printf("%g %g c\n", x, y);
			break;
		case FZ_CLOSEPATH:
			printf("h\n");
		}
	}

	switch (path->paint)
	{
	case FZ_STROKE:
		printf("S\n");
		break;
	case FZ_FILL:
		printf("f\n");
		break;
	case FZ_EOFILL:
		printf("f*\n");
		break;
	}
}

fz_error *
fz_newdash(fz_dash **dashp, float phase, int len, float *array)
{
	fz_dash *dash;
	int i;

	dash = *dashp = fz_malloc(sizeof(fz_dash) + sizeof(float) * len);
	if (!dash)
		return fz_outofmem;

	dash->len = len;
	dash->phase = phase;
	for (i = 0; i < len; i++)
		dash->array[i] = array[i];

	return nil;
}

void
fz_dropdash(fz_dash *dash)
{
	fz_free(dash);
}

