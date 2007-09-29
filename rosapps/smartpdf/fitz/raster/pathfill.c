#include "fitz-base.h"
#include "fitz-world.h"
#include "fitz-draw.h"

static fz_error *
line(fz_gel *gel, fz_matrix *ctm, float x0, float y0, float x1, float y1)
{
	float tx0 = ctm->a * x0 + ctm->c * y0 + ctm->e;
	float ty0 = ctm->b * x0 + ctm->d * y0 + ctm->f;
	float tx1 = ctm->a * x1 + ctm->c * y1 + ctm->e;
	float ty1 = ctm->b * x1 + ctm->d * y1 + ctm->f;
	return fz_insertgel(gel, tx0, ty0, tx1, ty1);
}

static fz_error *
bezier(fz_gel *gel, fz_matrix *ctm, float flatness,
	float xa, float ya,
	float xb, float yb,
	float xc, float yc,
	float xd, float yd)
{
	fz_error *error;
	float dmax;
	float xab, yab;
	float xbc, ybc;
	float xcd, ycd;
	float xabc, yabc;
	float xbcd, ybcd;
	float xabcd, yabcd;

	/* termination check */
	dmax = ABS(xa - xb);
	dmax = MAX(dmax, ABS(ya - yb));
	dmax = MAX(dmax, ABS(xd - xc));
	dmax = MAX(dmax, ABS(yd - yc));
	if (dmax < flatness)
		return line(gel, ctm, xa, ya, xd, yd);

	xab = xa + xb;
	yab = ya + yb;
	xbc = xb + xc;
	ybc = yb + yc;
	xcd = xc + xd;
	ycd = yc + yd;

	xabc = xab + xbc;
	yabc = yab + ybc;
	xbcd = xbc + xcd;
	ybcd = ybc + ycd;

	xabcd = xabc + xbcd;
	yabcd = yabc + ybcd;

	xab *= 0.5f; yab *= 0.5f;
	xbc *= 0.5f; ybc *= 0.5f;
	xcd *= 0.5f; ycd *= 0.5f;

	xabc *= 0.25f; yabc *= 0.25f;
	xbcd *= 0.25f; ybcd *= 0.25f;

	xabcd *= 0.125f; yabcd *= 0.125f;

	error = bezier(gel, ctm, flatness, xa, ya, xab, yab, xabc, yabc, xabcd, yabcd);
	if (error)
		return error;
	return bezier(gel, ctm, flatness, xabcd, yabcd, xbcd, ybcd, xcd, ycd, xd, yd);
}

fz_error *
fz_fillpath(fz_gel *gel, fz_pathnode *path, fz_matrix ctm, float flatness)
{
	fz_error *error;
	float x1, y1, x2, y2, x3, y3;
	float cx = 0;
	float cy = 0;
	float bx = 0;
	float by = 0;
	int i = 0;

	while (i < path->len)
	{
		switch (path->els[i++].k)
		{
		case FZ_MOVETO:
			x1 = path->els[i++].v;
			y1 = path->els[i++].v;
			cx = bx = x1;
			cy = by = y1;
			break;

		case FZ_LINETO:
			x1 = path->els[i++].v;
			y1 = path->els[i++].v;
			error = line(gel, &ctm, cx, cy, x1, y1);
			if (error)
				return error;
			cx = x1;
			cy = y1;
			break;

		case FZ_CURVETO:
			x1 = path->els[i++].v;
			y1 = path->els[i++].v;
			x2 = path->els[i++].v;
			y2 = path->els[i++].v;
			x3 = path->els[i++].v;
			y3 = path->els[i++].v;
			error = bezier(gel, &ctm, flatness, cx, cy, x1, y1, x2, y2, x3, y3);
			if (error)
				return error;
			cx = x3;
			cy = y3;
			break;

		case FZ_CLOSEPATH:
			error = line(gel, &ctm, cx, cy, bx, by);
			if (error)
				return error;
			cx = bx;
			cy = by;
			break;
		}
	}

	if (i && (cx != bx || cy != by))
	{
		error = line(gel, &ctm, cx, cy, bx, by);
		if (error)
			return error;
	}

	return nil;
}

