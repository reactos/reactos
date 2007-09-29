#include "fitz-base.h"

fz_rect fz_infiniterect = { 1, 1, -1, -1 };
fz_rect fz_emptyrect = { 0, 0, 0, 0 };

static fz_irect infinite = { 1, 1, -1, -1 };
static fz_irect empty = { 0, 0, 0, 0 };

fz_irect
fz_roundrect(fz_rect f)
{
	fz_irect i;
	i.x0 = fz_floor(f.x0);
	i.y0 = fz_floor(f.y0);
	i.x1 = fz_ceil(f.x1);
	i.y1 = fz_ceil(f.y1);
	return i;
}

fz_rect
fz_intersectrects(fz_rect a, fz_rect b)
{
	fz_rect r;
	if (fz_isinfiniterect(a)) return b;
	if (fz_isinfiniterect(b)) return a;
	r.x0 = MAX(a.x0, b.x0);
	r.y0 = MAX(a.y0, b.y0);
	r.x1 = MIN(a.x1, b.x1);
	r.y1 = MIN(a.y1, b.y1);
	return (r.x1 < r.x0 || r.y1 < r.y0) ? fz_emptyrect : r;
}

fz_rect
fz_mergerects(fz_rect a, fz_rect b)
{
	fz_rect r;
	if (fz_isinfiniterect(a) || fz_isinfiniterect(b))
		return fz_infiniterect;
	if (fz_isemptyrect(a)) return b;
	if (fz_isemptyrect(b)) return a;
	r.x0 = MIN(a.x0, b.x0);
	r.y0 = MIN(a.y0, b.y0);
	r.x1 = MAX(a.x1, b.x1);
	r.y1 = MAX(a.y1, b.y1);
	return r;
}

fz_irect
fz_intersectirects(fz_irect a, fz_irect b)
{
	fz_irect r;
	if (fz_isinfiniterect(a)) return b;
	if (fz_isinfiniterect(b)) return a;
	r.x0 = MAX(a.x0, b.x0);
	r.y0 = MAX(a.y0, b.y0);
	r.x1 = MIN(a.x1, b.x1);
	r.y1 = MIN(a.y1, b.y1);
	return (r.x1 < r.x0 || r.y1 < r.y0) ? empty : r;
}

fz_irect
fz_mergeirects(fz_irect a, fz_irect b)
{
	fz_irect r;
	if (fz_isinfiniterect(a) || fz_isinfiniterect(b))
		return infinite;
	if (fz_isemptyrect(a)) return b;
	if (fz_isemptyrect(b)) return a;
	r.x0 = MIN(a.x0, b.x0);
	r.y0 = MIN(a.y0, b.y0);
	r.x1 = MAX(a.x1, b.x1);
	r.y1 = MAX(a.y1, b.y1);
	return r;
}

