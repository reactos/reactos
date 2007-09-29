#include "fitz-base.h"
#include "fitz-world.h"
#include "fitz-draw.h"

/*
 * polygon clipping
 */

enum { IN, OUT, ENTER, LEAVE };
enum { MAXV = 3 + 4 };
enum { MAXN = 2 + FZ_MAXCOLORS };

static int clipx(float val, int ismax, float *v1, float *v2, int n)
{
	float t;
	int i;
	int v1o = ismax ? v1[0] > val : v1[0] < val;
	int v2o = ismax ? v2[0] > val : v2[0] < val;
	if (v1o + v2o == 0)
		return IN;
	if (v1o + v2o == 2)
		return OUT;
	if (v2o)
	{
		t = (val - v1[0]) / (v2[0] - v1[0]);
		v2[0] = val;
		v2[1] = v1[1] + t * (v2[1] - v1[1]);
		for (i = 2; i < n; i++)
			v2[i] = v1[i] + t * (v2[i] - v1[i]);
		return LEAVE;
	}
	else
	{
		t = (val - v2[0]) / (v1[0] - v2[0]);
		v1[0] = val;
		v1[1] = v2[1] + t * (v1[1] - v2[1]);
		for (i = 2; i < n; i++)
			v1[i] = v2[i] + t * (v1[i] - v2[i]);
		return ENTER;
	}
}

static int clipy(float val, int ismax, float *v1, float *v2, int n)
{
	float t;
	int i;
	int v1o = ismax ? v1[1] > val : v1[1] < val;
	int v2o = ismax ? v2[1] > val : v2[1] < val;
	if (v1o + v2o == 0)
		return IN;
	if (v1o + v2o == 2)
		return OUT;
	if (v2o)
	{
		t = (val - v1[1]) / (v2[1] - v1[1]);
		v2[0] = v1[0] + t * (v2[0] - v1[0]);
		v2[1] = val;
		for (i = 2; i < n; i++)
			v2[i] = v1[i] + t * (v2[i] - v1[i]);
		return LEAVE;
	}
	else
	{
		t = (val - v2[1]) / (v1[1] - v2[1]);
		v1[0] = v2[0] + t * (v1[0] - v2[0]);
		v1[1] = val;
		for (i = 2; i < n; i++)
			v1[i] = v2[i] + t * (v1[i] - v2[i]);
		return ENTER;
	}
}

static inline void copyvert(float *dst, float *src, int n)
{
	while (n--)
		*dst++ = *src++;
}

static int clippoly(float src[MAXV][MAXN],
					float dst[MAXV][MAXN], int len, int n,
					float val, int isy, int ismax)
{
	float cv1[MAXN];
	float cv2[MAXN];
	int v1, v2, cp;
	int r;

	v1 = len - 1;
	cp = 0;

	for (v2 = 0; v2 < len; v2++)
	{
		copyvert(cv1, src[v1], n);
		copyvert(cv2, src[v2], n);

		if (isy)
			r = clipy(val, ismax, cv1, cv2, n);
		else
			r = clipx(val, ismax, cv1, cv2, n);

		switch (r)
		{
			case IN:
				copyvert(dst[cp++], cv2, n);
				break;
			case OUT:
				break;
			case LEAVE:
				copyvert(dst[cp++], cv2, n);
				break;
			case ENTER:
				copyvert(dst[cp++], cv1, n);
				copyvert(dst[cp++], cv2, n);
				break;
		}
		v1 = v2;
	}

	return cp;
}

/*
 * gouraud shaded polygon scan conversion
 */

static inline void
drawscan(fz_pixmap *pix, int y, int x1, int x2, int *v1, int *v2, int n)
{
	unsigned char *p = pix->samples + ((y - pix->y) * pix->w + (x1 - pix->x)) * pix->n;
	int v[FZ_MAXCOLORS];
	int dv[FZ_MAXCOLORS];
	int w = x2 - x1;
	int k;

	assert(w >= 0);
	assert(y >= pix->y);
	assert(y < pix->y + pix->h);
	assert(x1 >= pix->x);
	assert(x2 <= pix->x + pix->w);

	if (w == 0)
		return;

	for (k = 0; k < n; k++)
	{
		v[k] = v1[k];
		dv[k] = (v2[k] - v1[k]) / w;
	}

	while (w--)
	{
		*p++ = 255;
		for (k = 0; k < n; k++)
		{
			*p++ = v[k] >> 16;
			v[k] += dv[k];
		}
	}
}

static inline int
findnext(int gel[MAXV][MAXN], int len, int a, int *s, int *e, int d)
{
	int b;

	while (1)
	{
		b = a + d;
		if (b == len)
			b = 0;
		if (b == -1)
			b = len - 1;

		if (gel[b][1] == gel[a][1])
		{
			a = b;
			continue;
		}

		if (gel[b][1] > gel[a][1])
		{
			*s = a;
			*e = b;
			return 0;
		}

		return 1;
	}
}

static inline void
loadedge(int gel[MAXV][MAXN], int s, int e, int *ael, int *del, int n)
{
	int swp, k, dy;

	if (gel[s][1] > gel[s][1])
	{
		swp = s; s = e; e = swp;
	}

	dy = gel[e][1] - gel[s][1];

	ael[0] = gel[s][0];
	del[0] = (gel[e][0] - gel[s][0]) / dy;
	for (k = 2; k < n; k++)
	{
		ael[k] = gel[s][k];
		del[k] = (gel[e][k] - gel[s][k]) / dy;
	}
}

static inline void
stepedge(int *ael, int *del, int n)
{
	int k;
	ael[0] += del[0];
	for (k = 2; k < n; k++)
		ael[k] += del[k];
}

void
fz_drawtriangle(fz_pixmap *pix, float *av, float *bv, float *cv, int n)
{
	float poly[MAXV][MAXN];
	float temp[MAXV][MAXN];
	float cx0 = pix->x;
	float cy0 = pix->y;
	float cx1 = pix->x + pix->w;
	float cy1 = pix->y + pix->h;

	int gel[MAXV][MAXN];
	int ael[2][MAXN];
	int del[2][MAXN];
	int y, s0, s1, e0, e1;
	int top, bot, len;

	int i, k;

	copyvert(poly[0], av, n);
	copyvert(poly[1], bv, n);
	copyvert(poly[2], cv, n);

	len = clippoly(poly, temp,   3, n, cx0, 0, 0);
	len = clippoly(temp, poly, len, n, cx1, 0, 1);
	len = clippoly(poly, temp, len, n, cy0, 1, 0);
	len = clippoly(temp, poly, len, n, cy1, 1, 1);

	if (len < 3)
		return;

	for (i = 0; i < len; i++)
	{
		gel[i][0] = fz_floor(poly[i][0] + 0.5) * 65536; /* trunc and fix */
		gel[i][1] = fz_floor(poly[i][1] + 0.5);	/* y is not fixpoint */
		for (k = 2; k < n; k++)
			gel[i][k] = poly[i][k] * 65536;	/* fix with precision */
	}

	top = bot = 0;
	for (i = 0; i < len; i++)
	{
		if (gel[i][1] < gel[top][1])
			top = i;
		if (gel[i][1] > gel[bot][1])
			bot = i;
	}

	if (gel[bot][1] - gel[top][1] == 0)
		return;

	y = gel[top][1];

	if (findnext(gel, len, top, &s0, &e0,  1))
		return;
	if (findnext(gel, len, top, &s1, &e1, -1))
		return;

	loadedge(gel, s0, e0, ael[0], del[0], n);
	loadedge(gel, s1, e1, ael[1], del[1], n);

	while (1)
	{
		int x0 = ael[0][0] >> 16;
		int x1 = ael[1][0] >> 16;

		if (ael[0][0] < ael[1][0])
			drawscan(pix, y, x0, x1, ael[0]+2, ael[1]+2, n-2);
		else
			drawscan(pix, y, x1, x0, ael[1]+2, ael[0]+2, n-2);

		stepedge(ael[0], del[0], n);
		stepedge(ael[1], del[1], n);
		y ++;

		if (y >= gel[e0][1])
		{
			if (findnext(gel, len, e0, &s0, &e0, 1))
				return;
			loadedge(gel, s0, e0, ael[0], del[0], n);
		}

		if (y >= gel[e1][1])
		{
			if (findnext(gel, len, e1, &s1, &e1, -1))
				return;
			loadedge(gel, s1, e1, ael[1], del[1], n);
		}
	}
}

/*
 * mesh drawing
 */

fz_error *
fz_rendershade(fz_shade *shade, fz_matrix ctm, fz_colorspace *destcs, fz_pixmap *dest)
{
	unsigned char clut[256][3];
	unsigned char *s, *d;
	fz_error *error;
	fz_pixmap *temp;
	float rgb[3];
	float tri[3][MAXN];
	fz_point p;
	int i, j, k, n;

	assert(dest->n == 4);

	ctm = fz_concat(shade->matrix, ctm);

	if (shade->usefunction)
	{
		n = 3;
		error = fz_newpixmap(&temp, dest->x, dest->y, dest->w, dest->h, 2);
		if (error)
			return error;
	}
	else if (shade->cs != destcs)
	{
		n = 2 + shade->cs->n;
		error = fz_newpixmap(&temp, dest->x, dest->y, dest->w, dest->h,
					shade->cs->n + 1);
		if (error)
			return error;
	}
	else
	{
		n = 2 + shade->cs->n;
		temp = dest;
	}

	fz_clearpixmap(temp);

	for (i = 0; i < shade->meshlen; i++)
	{
		for (k = 0; k < 3; k++)
		{
			p.x = shade->mesh[(i * 3 + k) * n + 0];
			p.y = shade->mesh[(i * 3 + k) * n + 1];
			p = fz_transformpoint(ctm, p);
			tri[k][0] = p.x;
			tri[k][1] = p.y;
			for (j = 2; j < n; j++)
				tri[k][j] = shade->mesh[( i * 3 + k) * n + j] * 255;
		}
		fz_drawtriangle(temp, tri[0], tri[1], tri[2], n);
	}

	if (shade->usefunction)
	{
		for (i = 0; i < 256; i++)
		{
			fz_convertcolor(shade->cs, shade->function[i], destcs, rgb);
			clut[i][0] = rgb[0] * 255;
			clut[i][1] = rgb[1] * 255;
			clut[i][2] = rgb[2] * 255;
		}

		n = temp->w * temp->h;
		s = temp->samples;
		d = dest->samples;

		while (n--)
		{
			d[0] = s[0];
			d[1] = fz_mul255(s[0], clut[s[1]][0]);
			d[2] = fz_mul255(s[0], clut[s[1]][1]);
			d[3] = fz_mul255(s[0], clut[s[1]][2]);
			s += 2;
			d += 4;
		}

		fz_droppixmap(temp);
	}

	else if (shade->cs != destcs)
	{
		fz_convertpixmap(shade->cs, temp, destcs, dest);
		fz_droppixmap(temp);
	}

	return nil;
}

