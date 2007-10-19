#include <fitz.h>
#include <mupdf.h>

/* this mess is jeong's */

#define BIGNUM 32000

#define NSEGS 32
#define MAX_RAD_SEGS 36

fz_error *
pdf_loadtype1shade(fz_shade *shade, pdf_xref *xref, fz_obj *dict, fz_obj *ref)
{
	fz_error *error;
	fz_obj *obj;
	fz_matrix matrix;
	pdf_function *func;

	int xx, yy;
	float x, y;
	float xn, yn;
	float x0, y0, x1, y1;
	int n;

	pdf_logshade("load type1 shade {\n");

	obj = fz_dictgets(dict, "Domain");
	if (obj) {
		x0 = fz_toreal(fz_arrayget(obj, 0));
		x1 = fz_toreal(fz_arrayget(obj, 1));
		y0 = fz_toreal(fz_arrayget(obj, 2));
		y1 = fz_toreal(fz_arrayget(obj, 3));
	}
	else {
		x0 = 0;
		x1 = 1.0;
		y0 = 0;
		y1 = 1.0;
	}

	pdf_logshade("domain %g %g %g %g\n", x0, x1, y0, y1);

	obj = fz_dictgets(dict, "Matrix");
	if (obj)
	{
		matrix = pdf_tomatrix(obj);
		pdf_logshade("matrix [%g %g %g %g %g %g]\n",
			matrix.a, matrix.b, matrix.c, matrix.d, matrix.e, matrix.f);
	}
	else
		matrix = fz_identity();

	obj = fz_dictgets(dict, "Function");
	error = pdf_loadfunction(&func, xref, obj);
	if (error)
		return error;

	shade->usefunction = 0;

	if (error)
		return error;

	shade->meshlen = NSEGS * NSEGS * 2;
	shade->mesh = fz_malloc(sizeof(float) * (2 + shade->cs->n) * 3 * shade->meshlen);
	if (!shade->mesh)
		return fz_outofmem;

	n = 0;
	for (yy = 0; yy < NSEGS; ++yy)
	{
		y = y0 + (y1 - y0) * yy / (float)NSEGS;
		yn = y0 + (y1 - y0) * (yy + 1) / (float)NSEGS;
		for (xx = 0; xx < NSEGS; ++xx)
		{
			x = x0 + (x1 - x0) * (xx / (float)NSEGS);
			xn = x0 + (x1 - x0) * (xx + 1) / (float)NSEGS;

#define ADD_VERTEX(xx, yy) \
			{\
				fz_point p;\
				float cp[2], cv[FZ_MAXCOLORS];\
				int c;\
				p.x = xx;\
				p.y = yy;\
				p = fz_transformpoint(matrix, p);\
				shade->mesh[n++] = p.x;\
				shade->mesh[n++] = p.y;\
				\
				cp[0] = xx;\
				cp[1] = yy;\
				error = pdf_evalfunction(func, cp, 2, cv, shade->cs->n);\
				\
				for (c = 0; c < shade->cs->n; ++c) {\
					shade->mesh[n++] = cv[c];\
				}\
			}\

			ADD_VERTEX(x, y);
			ADD_VERTEX(xn, y);
			ADD_VERTEX(xn, yn);

			ADD_VERTEX(x, y);
			ADD_VERTEX(xn, yn);
			ADD_VERTEX(x, yn);
		}
	}

	pdf_logshade("}\n");

	return nil;
}

fz_error *
pdf_loadtype2shade(fz_shade *shade, pdf_xref *xref, fz_obj *dict, fz_obj *ref)
{
	fz_point p1, p2, p3, p4;
	fz_point ep1, ep2, ep3, ep4;
	float x0, y0, x1, y1;
	float t0, t1;
	int e0, e1;
	fz_obj *obj;
	float theta;
	float dist;
	int n;

	pdf_logshade("load type2 shade {\n");

	obj = fz_dictgets(dict, "Coords");
	x0 = fz_toreal(fz_arrayget(obj, 0));
	y0 = fz_toreal(fz_arrayget(obj, 1));
	x1 = fz_toreal(fz_arrayget(obj, 2));
	y1 = fz_toreal(fz_arrayget(obj, 3));

	pdf_logshade("coords %g %g %g %g\n", x0, y0, x1, y1);

	obj = fz_dictgets(dict, "Domain");
	if (obj) {
		t0 = fz_toreal(fz_arrayget(obj, 0));
		t1 = fz_toreal(fz_arrayget(obj, 1));
	} else {
		t0 = 0.;
		t1 = 1.;
	}

	obj = fz_dictgets(dict, "Extend");
	if (obj) {
		e0 = fz_tobool(fz_arrayget(obj, 0));
		e1 = fz_tobool(fz_arrayget(obj, 1));
	} else {
		e0 = 0;
		e1 = 0;
	}

	pdf_logshade("domain %g %g\n", t0, t1);
	pdf_logshade("extend %d %d\n", e0, e1);

	pdf_loadshadefunction(shade, xref, dict, t0, t1);

	shade->meshlen = 2 + e0 * 2 + e1 * 2;
	shade->mesh = fz_malloc(sizeof(float) * 3*3 * shade->meshlen);
	if (!shade->mesh)
		return fz_outofmem;

	theta = atan2(y1 - y0, x1 - x0);
	theta += M_PI / 2.0;

	pdf_logshade("theta=%g\n", theta);

	dist = hypot(x1 - x0, y1 - y0);

	p1.x = x0 + BIGNUM * cos(theta);
	p1.y = y0 + BIGNUM * sin(theta);
	p2.x = x1 + BIGNUM * cos(theta);
	p2.y = y1 + BIGNUM * sin(theta);
	p3.x = x0 - BIGNUM * cos(theta);
	p3.y = y0 - BIGNUM * sin(theta);
	p4.x = x1 - BIGNUM * cos(theta);
	p4.y = y1 - BIGNUM * sin(theta);

	ep1.x = p1.x - (x1 - x0) / dist * BIGNUM;
	ep1.y = p1.y - (y1 - y0) / dist * BIGNUM;
	ep2.x = p2.x + (x1 - x0) / dist * BIGNUM;
	ep2.y = p2.y + (y1 - y0) / dist * BIGNUM;
	ep3.x = p3.x - (x1 - x0) / dist * BIGNUM;
	ep3.y = p3.y - (y1 - y0) / dist * BIGNUM;
	ep4.x = p4.x + (x1 - x0) / dist * BIGNUM;
	ep4.y = p4.y + (y1 - y0) / dist * BIGNUM;

	pdf_logshade("p1 %g %g\n", p1.x, p1.y);
	pdf_logshade("p2 %g %g\n", p2.x, p2.y);
	pdf_logshade("p3 %g %g\n", p3.x, p3.y);
	pdf_logshade("p4 %g %g\n", p4.x, p4.y);

	n = 0;

	pdf_setmeshvalue(shade->mesh, n++, p1.x, p1.y, 0);
	pdf_setmeshvalue(shade->mesh, n++, p2.x, p2.y, 1);
	pdf_setmeshvalue(shade->mesh, n++, p4.x, p4.y, 1);
	pdf_setmeshvalue(shade->mesh, n++, p1.x, p1.y, 0);
	pdf_setmeshvalue(shade->mesh, n++, p4.x, p4.y, 1);
	pdf_setmeshvalue(shade->mesh, n++, p3.x, p3.y, 0);

	if (e0) {
		pdf_setmeshvalue(shade->mesh, n++, ep1.x, ep1.y, 0);
		pdf_setmeshvalue(shade->mesh, n++, p1.x, p1.y, 0);
		pdf_setmeshvalue(shade->mesh, n++, p3.x, p3.y, 0);
		pdf_setmeshvalue(shade->mesh, n++, ep1.x, ep1.y, 0);
		pdf_setmeshvalue(shade->mesh, n++, p3.x, p3.y, 0);
		pdf_setmeshvalue(shade->mesh, n++, ep3.x, ep3.y, 0);
	}

	if (e1) {
		pdf_setmeshvalue(shade->mesh, n++, p2.x, p2.y, 1);
		pdf_setmeshvalue(shade->mesh, n++, ep2.x, ep2.y, 1);
		pdf_setmeshvalue(shade->mesh, n++, ep4.x, ep4.y, 1);
		pdf_setmeshvalue(shade->mesh, n++, p2.x, p2.y, 1);
		pdf_setmeshvalue(shade->mesh, n++, ep4.x, ep4.y, 1);
		pdf_setmeshvalue(shade->mesh, n++, p4.x, p4.y, 1);
	}

	pdf_logshade("}\n");

	return nil;
}

static int
buildannulusmesh(float* mesh, int pos,
					float x0, float y0, float r0, float x1, float y1, float r1,
					float c0, float c1, int nomesh)
{
	int n = pos * 3;
	float dist = hypot(x1 - x0, y1 - y0);
	float step;
	float theta;
	int i;

	if (dist != 0)
		theta = asin((r1 - r0) / dist) + M_PI/2.0 + atan2(y1 - y0, x1 - x0);
	else
		theta = 0;

	if (!(theta >= 0 && theta <= M_PI))
		theta = 0;

	step = M_PI * 2.f / (float)MAX_RAD_SEGS;

	for (i = 0; i < MAX_RAD_SEGS; theta -= step, ++i)
	{
		fz_point pt1, pt2, pt3, pt4;

		pt1.x = cos (theta) * r1 + x1;
		pt1.y = sin (theta) * r1 + y1;
		pt2.x = cos (theta) * r0 + x0;
		pt2.y = sin (theta) * r0 + y0;
		pt3.x = cos (theta+step) * r1 + x1;
		pt3.y = sin (theta+step) * r1 + y1;
		pt4.x = cos (theta+step) * r0 + x0;
		pt4.y = sin (theta+step) * r0 + y0;

		if (r0 > 0) {
			if (!nomesh) {
				pdf_setmeshvalue(mesh, n++, pt1.x, pt1.y, c1);
				pdf_setmeshvalue(mesh, n++, pt2.x, pt2.y, c0);
				pdf_setmeshvalue(mesh, n++, pt4.x, pt4.y, c0);
			}
			pos++;
		}

		if (r1 > 0) {
			if (!nomesh) {
				pdf_setmeshvalue(mesh, n++, pt1.x, pt1.y, c1);
				pdf_setmeshvalue(mesh, n++, pt3.x, pt3.y, c1);
				pdf_setmeshvalue(mesh, n++, pt4.x, pt4.y, c0);
			}
			pos++;
		}
	}

	return pos;
}

fz_error *
pdf_loadtype3shade(fz_shade *shade, pdf_xref *xref, fz_obj *shading, fz_obj *ref)
{
	fz_obj *obj;
	float x0, y0, r0, x1, y1, r1;
	float t0, t1;
	int e0, e1;
	float ex0, ey0, er0;
	float ex1, ey1, er1;
	float rs;
	int i;

	pdf_logshade("load type3 shade {\n");

	obj = fz_dictgets(shading, "Coords");
	x0 = fz_toreal(fz_arrayget(obj, 0));
	y0 = fz_toreal(fz_arrayget(obj, 1));
	r0 = fz_toreal(fz_arrayget(obj, 2));
	x1 = fz_toreal(fz_arrayget(obj, 3));
	y1 = fz_toreal(fz_arrayget(obj, 4));
	r1 = fz_toreal(fz_arrayget(obj, 5));

	pdf_logshade("coords %g %g %g  %g %g %g\n", x0, y0, r0, x1, y1, r1);

	obj = fz_dictgets(shading, "Domain");
	if (obj) {
		t0 = fz_toreal(fz_arrayget(obj, 0));
		t1 = fz_toreal(fz_arrayget(obj, 1));
	} else {
		t0 = 0.;
		t1 = 1.;
	}

	obj = fz_dictgets(shading, "Extend");
	if (obj) {
		e0 = fz_tobool(fz_arrayget(obj, 0));
		e1 = fz_tobool(fz_arrayget(obj, 1));
	} else {
		e0 = 0;
		e1 = 0;
	}

	pdf_logshade("domain %g %g\n", t0, t1);
	pdf_logshade("extend %d %d\n", e0, e1);

	pdf_loadshadefunction(shade, xref, shading, t0, t1);

	if (r0 < r1)
		rs = r0 / (r0 - r1);
	else
		rs = -BIGNUM;

	ex0 = x0 + (x1 - x0) * rs;
	ey0 = y0 + (y1 - y0) * rs;
	er0 = r0 + (r1 - r0) * rs;

	if (r0 > r1)
		rs = r1 / (r1 - r0);
	else
		rs = -BIGNUM;

	ex1 = x1 + (x0 - x1) * rs;
	ey1 = y1 + (y0 - y1) * rs;
	er1 = r1 + (r0 - r1) * rs;

	for (i=0; i<2; ++i)
	{
		int pos = 0;
		if (e0)
			pos = buildannulusmesh(shade->mesh, pos, ex0, ey0, er0, x0, y0, r0, 0, 0, 1-i);
		pos = buildannulusmesh(shade->mesh, pos, x0, y0, r0, x1, y1, r1, 0, 1., 1-i);
		if (e1)
			pos = buildannulusmesh(shade->mesh, pos, x1, y1, r1, ex1, ey1, er1, 1., 1., 1-i);

		if (i == 0)
		{
			shade->meshlen = pos;
			shade->mesh = fz_malloc(sizeof(float) * 9 * shade->meshlen);
			if (!shade->mesh)
				return fz_outofmem;
		}
	}

	pdf_logshade("}\n");

	return nil;
}

