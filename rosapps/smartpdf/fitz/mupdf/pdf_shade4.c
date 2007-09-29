#include <fitz.h>
#include <mupdf.h>

/* this mess is jeong's */

typedef struct pdf_tensorpatch_s pdf_tensorpatch;
struct pdf_tensorpatch_s {
    fz_point pole[4][4];
    float color[4][FZ_MAXCOLORS];
};

fz_error *
pdf_loadtype4shade(fz_shade *shade, pdf_xref *xref, fz_obj *shading, fz_obj *ref)
{
	fz_error *error;
	fz_obj *obj;
	int bpcoord;
	int bpcomp;
	int bpflag;
	int ncomp;
	float x0, x1, y0, y1;
	float c0[FZ_MAXCOLORS];
	float c1[FZ_MAXCOLORS];
	int i, z;
	int bitspervertex;
	int bytepervertex;
	fz_buffer *buf;
	int n;
	int j;
	float cval[16];

	int flag;
	unsigned int t;
	float x, y;

	error = nil;

	ncomp = shade->cs->n;
	bpcoord = fz_toint(fz_dictgets(shading, "BitsPerCoordinate"));
	bpcomp = fz_toint(fz_dictgets(shading, "BitsPerComponent"));
	bpflag = fz_toint(fz_dictgets(shading, "BitsPerFlag"));

	obj = fz_dictgets(shading, "Decode");
	if (fz_isarray(obj))
	{
		pdf_logshade("decode array\n");
		x0 = fz_toreal(fz_arrayget(obj, 0));
		x1 = fz_toreal(fz_arrayget(obj, 1));
		y0 = fz_toreal(fz_arrayget(obj, 2));
		y1 = fz_toreal(fz_arrayget(obj, 3));
		for (i=0; i < fz_arraylen(obj) / 2; ++i) {
			c0[i] = fz_toreal(fz_arrayget(obj, i*2+4));
			c1[i] = fz_toreal(fz_arrayget(obj, i*2+5));
		}
	}
	else {
		error = fz_throw("syntaxerror: No Decode key in Type 4 Shade");
		goto cleanup;
	}

	obj = fz_dictgets(shading, "Function");
	if (obj) {
		ncomp = 1;
		pdf_loadshadefunction(shade, xref, shading, c0[0], c1[0]);
	}

	bitspervertex = bpflag + bpcoord * 2 + bpcomp * ncomp;	
	bytepervertex = (bitspervertex+7) / 8;

	error = pdf_loadstream(&buf, xref, fz_tonum(ref), fz_togen(ref));
	if (error) goto cleanup;

	shade->usefunction = 0;


	n = 2 + shade->cs->n;
	j = 0;
	for (z = 0; z < (buf->ep - buf->bp) / bytepervertex; ++z)
	{
		flag = *buf->rp++;

		t = *buf->rp++;
		t = (t << 8) + *buf->rp++;
		t = (t << 8) + *buf->rp++;
		x = x0 + (t * (x1 - x0) / (pow(2, 24) - 1));

		t = *buf->rp++;
		t = (t << 8) + *buf->rp++;
		t = (t << 8) + *buf->rp++;
		y = y0 + (t * (y1 - y0) / (pow(2, 24) - 1));

		for (i=0; i < ncomp; ++i) {
			t = *buf->rp++;
			t = (t << 8) + *buf->rp++;
		}

		if (flag == 0) {
			j += n;
		}
		if (flag == 1 || flag == 2) {
			j += 3 * n;
		}
	}
	buf->rp = buf->bp;

	shade->mesh = (float*) malloc(sizeof(float) * j);
	/* 8, 24, 16 only */
	j = 0;
	for (z = 0; z < (buf->ep - buf->bp) / bytepervertex; ++z)
	{
		flag = *buf->rp++;

		t = *buf->rp++;
		t = (t << 8) + *buf->rp++;
		t = (t << 8) + *buf->rp++;
		x = x0 + (t * (x1 - x0) / (pow(2, 24) - 1));

		t = *buf->rp++;
		t = (t << 8) + *buf->rp++;
		t = (t << 8) + *buf->rp++;
		y = y0 + (t * (y1 - y0) / (pow(2, 24) - 1));

		for (i=0; i < ncomp; ++i) {
			t = *buf->rp++;
			t = (t << 8) + *buf->rp++;
			cval[i] = t / (double)(pow(2, 16) - 1);
		}

		if (flag == 0) {
			shade->mesh[j++] = x;
			shade->mesh[j++] = y;
			for (i=0; i < ncomp; ++i) {
				shade->mesh[j++] = cval[i];
			}
		}
		if (flag == 1) {
			memcpy(&(shade->mesh[j]), &(shade->mesh[j - 2 * n]), n * sizeof(float));
			memcpy(&(shade->mesh[j + 1 * n]), &(shade->mesh[j - 1 * n]), n * sizeof(float));
			j+= 2 * n;
			shade->mesh[j++] = x;
			shade->mesh[j++] = y;
			for (i=0; i < ncomp; ++i) {
				shade->mesh[j++] = cval[i];
			}
		}
		if (flag == 2) {
			memcpy(&(shade->mesh[j]), &(shade->mesh[j - 3 * n]), n * sizeof(float));
			memcpy(&(shade->mesh[j + 1 * n]), &(shade->mesh[j - 1 * n]), n * sizeof(float));
			j+= 2 * n;
			shade->mesh[j++] = x;
			shade->mesh[j++] = y;
			for (i=0; i < ncomp; ++i) {
				shade->mesh[j++] = cval[i];
			}
		}
	}
	shade->meshlen = j / n / 3;

	fz_dropbuffer(buf);

cleanup:

	return nil;
}

static int
getdata(fz_stream *stream, int bps)
{
	unsigned int bitmask = (1 << bps) - 1;
	unsigned int buf = 0;
	int bits = 0;
	int s;

	while (bits < bps)
	{
		buf = (buf << 8) | (fz_readbyte(stream) & 0xff);
		bits += 8;
	}
	s = buf >> (bits - bps);
	if (bps < 32)
		s = s & bitmask;
	bits -= bps;

	return s;
}

fz_error *
pdf_loadtype5shade(fz_shade *shade, pdf_xref *xref, fz_obj *shading, fz_obj *ref)
{
	fz_error *error;
	fz_stream *stream;
	fz_obj *obj;

	int bpcoord;
	int bpcomp;
	int vpr, vpc;
	int ncomp;

	float x0, x1, y0, y1;

	float c0[FZ_MAXCOLORS];
	float c1[FZ_MAXCOLORS];

	int i, n, j;
	int p, q;
	unsigned int t;

	float *x, *y, *c[FZ_MAXCOLORS];

	error = nil;

	ncomp = shade->cs->n;
	bpcoord = fz_toint(fz_dictgets(shading, "BitsPerCoordinate"));
	bpcomp = fz_toint(fz_dictgets(shading, "BitsPerComponent"));
	vpr = fz_toint(fz_dictgets(shading, "VerticesPerRow"));
	if (vpr < 2) {
		error = fz_throw("VerticesPerRow must be greater than or equal to 2");
		goto cleanup;
	}

	obj = fz_dictgets(shading, "Decode");
	if (fz_isarray(obj))
	{
		pdf_logshade("decode array\n");
		x0 = fz_toreal(fz_arrayget(obj, 0));
		x1 = fz_toreal(fz_arrayget(obj, 1));
		y0 = fz_toreal(fz_arrayget(obj, 2));
		y1 = fz_toreal(fz_arrayget(obj, 3));
		for (i=0; i < fz_arraylen(obj) / 2; ++i) {
			c0[i] = fz_toreal(fz_arrayget(obj, i*2+4));
			c1[i] = fz_toreal(fz_arrayget(obj, i*2+5));
		}
	}
	else {
		error = fz_throw("syntaxerror: No Decode key in Type 4 Shade");
		goto cleanup;
	}

	obj = fz_dictgets(shading, "Function");
	if (obj) {
		ncomp = 1;
		pdf_loadshadefunction(shade, xref, shading, c0[0], c1[0]);
		shade->usefunction = 1;
	} 
	else
		shade->usefunction = 0;

	n = 2 + shade->cs->n;
	j = 0;

#define BIGNUM 1024

	x = fz_malloc(sizeof(float) * vpr * BIGNUM);
	y = fz_malloc(sizeof(float) * vpr * BIGNUM);
	for (i = 0; i < ncomp; ++i) {
		c[i] = fz_malloc(sizeof(float) * vpr * BIGNUM);
	}
	q = 0;

	error = pdf_openstream(&stream, xref, fz_tonum(ref), fz_togen(ref));
	if (error) goto cleanup;

	while (fz_peekbyte(stream) != EOF)
	{
		for (p = 0; p < vpr; ++p) {
			int idx;
			idx = q * vpr + p;

			t = getdata(stream, bpcoord);
			x[idx] = x0 + (t * (x1 - x0) / ((float)pow(2, bpcoord) - 1));
			t = getdata(stream, bpcoord);
			y[idx] = y0 + (t * (y1 - y0) / ((float)pow(2, bpcoord) - 1));

			for (i=0; i < ncomp; ++i) {
				t = getdata(stream, bpcomp);
				c[i][idx] = c0[i] + (t * (c1[i] - c0[i]) / (float)(pow(2, bpcomp) - 1));
			}
		}
		q++;
	}

	fz_dropstream(stream);

#define ADD_VERTEX(idx) \
			{\
				int z;\
				shade->mesh[j++] = x[idx];\
				shade->mesh[j++] = y[idx];\
				for (z = 0; z < shade->cs->n; ++z) {\
					shade->mesh[j++] = c[z][idx];\
				}\
			}\

	vpc = q;

	shade->meshcap = 0;
	shade->mesh = fz_malloc(sizeof(float) * 1024);
	if (!shade) {
		error = fz_outofmem;
		goto cleanup;
	}

	j = 0;
	for (p = 0; p < vpr-1; ++p) {
		for (q = 0; q < vpc-1; ++q) {
			ADD_VERTEX(q * vpr + p);
			ADD_VERTEX(q * vpr + p + 1);
			ADD_VERTEX((q + 1) * vpr + p + 1);
			
			ADD_VERTEX(q * vpr + p);
			ADD_VERTEX((q + 1) * vpr + p + 1);
			ADD_VERTEX((q + 1) * vpr + p);
		}
	}

	shade->meshlen = j / n / 3;

	fz_free(x);
	fz_free(y);
	for (i = 0; i < ncomp; ++i) {
		fz_free(c[i]);
	}


cleanup:

	return nil;
}

#define SEGMENTATION_DEPTH 2

static inline void copyvert(float *dst, float *src, int n)
{
	while (n--)
		*dst++ = *src++;
}


static inline void copycolor(float *c, const float *s)
{
	int i;
	for (i = 0; i<FZ_MAXCOLORS; ++i)
		c[i] = s[i];
}

static inline void midcolor(float *c, const float *c1, const float *c2)
{
	int i;
	for (i = 0; i<FZ_MAXCOLORS; ++i)
		c[i] = (float)((c1[i] + c2[i]) / 2.0);
}


static void
filltensorinterior(pdf_tensorpatch *p)
{
#define lcp1(p0, p3)\
	((p0 + p0 + p3) / 3.0f)

#define lcp2(p0, p3)\
	((p0 + p3 + p3) / 3.0f)

	p->pole[1][1].x = lcp1(p->pole[0][1].x, p->pole[3][1].x) +
		lcp1(p->pole[1][0].x, p->pole[1][3].x) -
		lcp1(lcp1(p->pole[0][0].x, p->pole[0][3].x),
		lcp1(p->pole[3][0].x, p->pole[3][3].x));
	p->pole[1][2].x = lcp1(p->pole[0][2].x, p->pole[3][2].x) +
		lcp2(p->pole[1][0].x, p->pole[1][3].x) -
		lcp1(lcp2(p->pole[0][0].x, p->pole[0][3].x),
		lcp2(p->pole[3][0].x, p->pole[3][3].x));
	p->pole[2][1].x = lcp2(p->pole[0][1].x, p->pole[3][1].x) +
		lcp1(p->pole[2][0].x, p->pole[2][3].x) -
		lcp2(lcp1(p->pole[0][0].x, p->pole[0][3].x),
		lcp1(p->pole[3][0].x, p->pole[3][3].x));
	p->pole[2][2].x = lcp2(p->pole[0][2].x, p->pole[3][2].x) +
		lcp2(p->pole[2][0].x, p->pole[2][3].x) -
		lcp2(lcp2(p->pole[0][0].x, p->pole[0][3].x),
		lcp2(p->pole[3][0].x, p->pole[3][3].x));
	
	p->pole[1][1].y = lcp1(p->pole[0][1].y, p->pole[3][1].y) +
		lcp1(p->pole[1][0].y, p->pole[1][3].y) -
		lcp1(lcp1(p->pole[0][0].y, p->pole[0][3].y),
		lcp1(p->pole[3][0].y, p->pole[3][3].y));
	p->pole[1][2].y = lcp1(p->pole[0][2].y, p->pole[3][2].y) +
		lcp2(p->pole[1][0].y, p->pole[1][3].y) -
		lcp1(lcp2(p->pole[0][0].y, p->pole[0][3].y),
		lcp2(p->pole[3][0].y, p->pole[3][3].y));
	p->pole[2][1].y = lcp2(p->pole[0][1].y, p->pole[3][1].y) +
		lcp1(p->pole[2][0].y, p->pole[2][3].y) -
		lcp2(lcp1(p->pole[0][0].y, p->pole[0][3].y),
		lcp1(p->pole[3][0].y, p->pole[3][3].y));
	p->pole[2][2].y = lcp2(p->pole[0][2].y, p->pole[3][2].y) +
		lcp2(p->pole[2][0].y, p->pole[2][3].y) -
		lcp2(lcp2(p->pole[0][0].y, p->pole[0][3].y),
		lcp2(p->pole[3][0].y, p->pole[3][3].y));

#undef lcp1
#undef lcp2
}

static void
split_curve_s(const fz_point *pole, fz_point *q0, fz_point *q1, int pole_step)
{
#define midpoint(a,b)\
	((a)/2.0f + (b)/2.0f) // to avoid overflow
    float x12 = midpoint(pole[1 * pole_step].x, pole[2 * pole_step].x);
    float y12 = midpoint(pole[1 * pole_step].y, pole[2 * pole_step].y);

    q0[1 * pole_step].x = midpoint(pole[0 * pole_step].x, pole[1 * pole_step].x);
    q0[1 * pole_step].y = midpoint(pole[0 * pole_step].y, pole[1 * pole_step].y);
    q1[2 * pole_step].x = midpoint(pole[2 * pole_step].x, pole[3 * pole_step].x);
    q1[2 * pole_step].y = midpoint(pole[2 * pole_step].y, pole[3 * pole_step].y);
    q0[2 * pole_step].x = midpoint(q0[1 * pole_step].x, x12);
    q0[2 * pole_step].y = midpoint(q0[1 * pole_step].y, y12);
    q1[1 * pole_step].x = midpoint(x12, q1[2 * pole_step].x);
    q1[1 * pole_step].y = midpoint(y12, q1[2 * pole_step].y);
    q0[0 * pole_step].x = pole[0 * pole_step].x;
    q0[0 * pole_step].y = pole[0 * pole_step].y;
    q0[3 * pole_step].x = q1[0 * pole_step].x = midpoint(q0[2 * pole_step].x, q1[1 * pole_step].x);
    q0[3 * pole_step].y = q1[0 * pole_step].y = midpoint(q0[2 * pole_step].y, q1[1 * pole_step].y);
    q1[3 * pole_step].x = pole[3 * pole_step].x;
    q1[3 * pole_step].y = pole[3 * pole_step].y;
#undef midpoint
}

static inline void
split_patch(pdf_tensorpatch *s0, pdf_tensorpatch *s1, const pdf_tensorpatch *p)
{
    split_curve_s(&p->pole[0][0], &s0->pole[0][0], &s1->pole[0][0], 4);
    split_curve_s(&p->pole[0][1], &s0->pole[0][1], &s1->pole[0][1], 4);
    split_curve_s(&p->pole[0][2], &s0->pole[0][2], &s1->pole[0][2], 4);
    split_curve_s(&p->pole[0][3], &s0->pole[0][3], &s1->pole[0][3], 4);

	copycolor(s0->color[0], p->color[0]);
	midcolor(s0->color[1], p->color[0], p->color[1]);
	midcolor(s0->color[2], p->color[2], p->color[3]);
	copycolor(s0->color[3], p->color[3]);

	copycolor(s1->color[0], s0->color[1]);
	copycolor(s1->color[1], p->color[1]);
	copycolor(s1->color[2], p->color[2]);	
	copycolor(s1->color[3], s0->color[2]);
}

static inline void
split_stripe(pdf_tensorpatch *s0, pdf_tensorpatch *s1, const pdf_tensorpatch *p)

{
    split_curve_s(p->pole[0], s0->pole[0], s1->pole[0], 1);
    split_curve_s(p->pole[1], s0->pole[1], s1->pole[1], 1);
    split_curve_s(p->pole[2], s0->pole[2], s1->pole[2], 1);
    split_curve_s(p->pole[3], s0->pole[3], s1->pole[3], 1);

	copycolor(s0->color[0], p->color[0]);
	copycolor(s0->color[1], p->color[1]);
	midcolor(s0->color[2], p->color[1], p->color[2]);
	midcolor(s0->color[3], p->color[0], p->color[3]);

	copycolor(s1->color[0], s0->color[3]);
	copycolor(s1->color[1], s0->color[2]);
	copycolor(s1->color[2], p->color[2]);
	copycolor(s1->color[3], p->color[3]);
}

static fz_error *
growshademesh(fz_shade *shade, int amount)
{
	float *newmesh;
	int newcap;

	newcap = shade->meshcap + amount;
	newmesh = fz_realloc(shade->mesh, sizeof(float) * newcap);
	if (!newmesh)
		return fz_outofmem;

	shade->mesh = newmesh;
	shade->meshcap = newcap;

	return nil;
}

static inline int setvertex(float *mesh, fz_point pt, float *color, int ptr, int ncomp)
{
	int i;

	mesh[ptr++] = pt.x;
	mesh[ptr++] = pt.y;
	for (i=0; i < ncomp; ++i) {
		mesh[ptr++] = color[i];
	}

	return ptr;
}

static int
triangulatepatch(pdf_tensorpatch p, fz_shade *shade, int ptr, int ncomp)
{
	fz_error* error;

	ptr = setvertex(shade->mesh, p.pole[0][0], p.color[0], ptr, ncomp);
	ptr = setvertex(shade->mesh, p.pole[3][0], p.color[1], ptr, ncomp);
	ptr = setvertex(shade->mesh, p.pole[3][3], p.color[2], ptr, ncomp);
	ptr = setvertex(shade->mesh, p.pole[0][0], p.color[0], ptr, ncomp);
	ptr = setvertex(shade->mesh, p.pole[3][3], p.color[2], ptr, ncomp);
	ptr = setvertex(shade->mesh, p.pole[0][3], p.color[3], ptr, ncomp);

	if (shade->meshcap - 1024 < ptr) {
		error = growshademesh(shade, 1024);
		if (error) goto cleanup;
	}

	return ptr;

cleanup:
	// error handling
	return -1;
}

static int
drawstripe(pdf_tensorpatch patch, fz_shade *shade, int ptr, int ncomp, int depth)
{
	pdf_tensorpatch s0, s1;
	
	split_stripe(&s0, &s1, &patch);

	depth++;
	
	if (depth >= SEGMENTATION_DEPTH) 
	{
		ptr = triangulatepatch(s0, shade, ptr, ncomp);
		ptr = triangulatepatch(s1, shade, ptr, ncomp);
	}
	else {
		ptr = drawstripe(s0, shade, ptr, ncomp, depth);
		ptr = drawstripe(s1, shade, ptr, ncomp, depth);
	}

	return ptr;
}

static int
drawpatch(pdf_tensorpatch patch, fz_shade *shade, int ptr, int ncomp, int depth)
{
	pdf_tensorpatch s0, s1;
	
	split_patch(&s0, &s1, &patch);
	depth++;
	
	if (depth > SEGMENTATION_DEPTH) 
	{
		ptr = drawstripe(s0, shade, ptr, ncomp, 0);
		ptr = drawstripe(s1, shade, ptr, ncomp, 0);
	}
	else {
		ptr = drawpatch(s0, shade, ptr, ncomp, depth);
		ptr = drawpatch(s1, shade, ptr, ncomp, depth);
	}

	return ptr;
}

fz_error *
pdf_loadtype6shade(fz_shade *shade, pdf_xref *xref, fz_obj *shading, fz_obj *ref)
{
	fz_error *error;
	fz_stream *stream;
	fz_obj *obj;

	int bpcoord;
	int bpcomp;
	int bpflag;
	int ncomp;

	fz_point p0, p1;

	float c0[FZ_MAXCOLORS];
	float c1[FZ_MAXCOLORS];

	int i, n, j;
	unsigned int t;

	int flag;
	fz_point p[12];

	pdf_tensorpatch patch;

	error = nil;

	ncomp = shade->cs->n;
	bpcoord = fz_toint(fz_dictgets(shading, "BitsPerCoordinate"));
	bpcomp = fz_toint(fz_dictgets(shading, "BitsPerComponent"));
	bpflag = fz_toint(fz_dictgets(shading, "BitsPerFlag"));

	obj = fz_dictgets(shading, "Decode");
	if (fz_isarray(obj))
	{
		pdf_logshade("decode array\n");
		p0.x = fz_toreal(fz_arrayget(obj, 0));
		p1.x = fz_toreal(fz_arrayget(obj, 1));
		p0.y = fz_toreal(fz_arrayget(obj, 2));
		p1.y = fz_toreal(fz_arrayget(obj, 3));
		for (i=0; i < fz_arraylen(obj) / 2; ++i) {
			c0[i] = fz_toreal(fz_arrayget(obj, i*2+4));
			c1[i] = fz_toreal(fz_arrayget(obj, i*2+5));
		}
	}
	else {
		error = fz_throw("syntaxerror: No Decode key in Type 6 Shade");
		goto cleanup;
	}

	obj = fz_dictgets(shading, "Function");
	if (obj) {
		ncomp = 1;
		pdf_loadshadefunction(shade, xref, shading, c0[0], c1[0]);
		shade->usefunction = 1;
	} 
	else
		shade->usefunction = 0;

	shade->meshcap = 0;
	shade->mesh = nil;
	error = growshademesh(shade, 1024);
	if (error) goto cleanup;

	n = 2 + shade->cs->n;
	j = 0;

	error = pdf_openstream(&stream, xref, fz_tonum(ref), fz_togen(ref));
	if (error) goto cleanup;

	while (fz_peekbyte(stream) != EOF)
	{
		flag = getdata(stream, bpflag);

		for (i = 0; i < 12; ++i) {
			t = getdata(stream, bpcoord);
			p[i].x = (float)(p0.x + (t * (p1.x - p0.x) / (pow(2, bpcoord) - 1.)));
			t = getdata(stream, bpcoord);
			p[i].y = (float)(p0.y + (t * (p1.y - p0.y) / (pow(2, bpcoord) - 1.)));
		}

		for (i = 0; i < 4; ++i) {
			int k;
			for (k=0; k < ncomp; ++k) {
				t = getdata(stream, bpcomp);
				patch.color[i][k] = 
					c0[k] + (t * (c1[k] - c0[k]) / (pow(2, bpcomp) - 1.0f));
			}
		}

		patch.pole[0][0] = p[0];
		patch.pole[1][0] = p[1];
		patch.pole[2][0] = p[2];
		patch.pole[3][0] = p[3];
		patch.pole[3][1] = p[4];
		patch.pole[3][2] = p[5];
		patch.pole[3][3] = p[6];
		patch.pole[2][3] = p[7];
		patch.pole[1][3] = p[8];
		patch.pole[0][3] = p[9];
		patch.pole[0][2] = p[10];
		patch.pole[0][1] = p[11];
		filltensorinterior(&patch);

		j = drawpatch(patch, shade, j, ncomp, 0);
	}

	fz_dropstream(stream);

	shade->meshlen = j / n / 3;

cleanup:

	return nil;
}

fz_error *
pdf_loadtype7shade(fz_shade *shade, pdf_xref *xref, fz_obj *shading, fz_obj *ref)
{
	fz_error *error;
	fz_stream *stream;
	fz_obj *obj;

	int bpcoord;
	int bpcomp;
	int bpflag;
	int ncomp;

	float x0, x1, y0, y1;

	float c0[FZ_MAXCOLORS];
	float c1[FZ_MAXCOLORS];

	int i, n, j;
	unsigned int t;

	int flag;
	fz_point p[16];
	pdf_tensorpatch patch;

	error = nil;

	ncomp = shade->cs->n;
	bpcoord = fz_toint(fz_dictgets(shading, "BitsPerCoordinate"));
	bpcomp = fz_toint(fz_dictgets(shading, "BitsPerComponent"));
	bpflag = fz_toint(fz_dictgets(shading, "BitsPerFlag"));

	obj = fz_dictgets(shading, "Decode");
	if (fz_isarray(obj))
	{
		pdf_logshade("decode array\n");
		x0 = fz_toreal(fz_arrayget(obj, 0));
		x1 = fz_toreal(fz_arrayget(obj, 1));
		y0 = fz_toreal(fz_arrayget(obj, 2));
		y1 = fz_toreal(fz_arrayget(obj, 3));
		for (i=0; i < fz_arraylen(obj) / 2; ++i) {
			c0[i] = fz_toreal(fz_arrayget(obj, i*2+4));
			c1[i] = fz_toreal(fz_arrayget(obj, i*2+5));
		}
	}
	else {
		error = fz_throw("syntaxerror: No Decode key in Type 6 Shade");
		goto cleanup;
	}

	obj = fz_dictgets(shading, "Function");
	if (obj) {
		ncomp = 1;
		pdf_loadshadefunction(shade, xref, shading, c0[0], c1[0]);
		shade->usefunction = 1;
	} 
	else
		shade->usefunction = 0;

	shade->meshcap = 0;
	shade->mesh = nil;
	error = growshademesh(shade, 1024);
	if (error) goto cleanup;

	n = 2 + shade->cs->n;
	j = 0;

	error = pdf_openstream(&stream, xref, fz_tonum(ref), fz_togen(ref));
	if (error) goto cleanup;

	while (fz_peekbyte(stream) != EOF)
	{
		flag = getdata(stream, bpflag);

		for (i = 0; i < 16; ++i) {
			t = getdata(stream, bpcoord);
			p[i].x = x0 + (t * (x1 - x0) / (pow(2, bpcoord) - 1.));
			t = getdata(stream, bpcoord);
			p[i].y = y0 + (t * (y1 - y0) / (pow(2, bpcoord) - 1.));
		}

		for (i = 0; i < 4; ++i) {
			int k;
			for (k=0; k < ncomp; ++k) {
				t = getdata(stream, bpcomp);
				patch.color[i][k] = 
					c0[k] + (t * (c1[k] - c0[k]) / (pow(2, bpcomp) - 1.0f));
			}
		}

		patch.pole[0][0] = p[0];
		patch.pole[0][1] = p[1];
		patch.pole[0][2] = p[2];
		patch.pole[0][3] = p[3];
		patch.pole[1][3] = p[4];
		patch.pole[2][3] = p[5];
		patch.pole[3][3] = p[6];
		patch.pole[3][2] = p[7];
		patch.pole[3][1] = p[8];
		patch.pole[3][0] = p[9];
		patch.pole[2][0] = p[10];
		patch.pole[1][0] = p[11];
		patch.pole[1][1] = p[12];
		patch.pole[1][2] = p[13];
		patch.pole[2][2] = p[14];
		patch.pole[2][1] = p[15];

		j = drawpatch(patch, shade, j, ncomp, 0);
	}

	fz_dropstream(stream);

	shade->meshlen = j / n / 3;

cleanup:

	return nil;
}

