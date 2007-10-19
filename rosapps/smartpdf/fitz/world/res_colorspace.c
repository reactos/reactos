#include "fitz-base.h"
#include "fitz-world.h"

void
fz_convertpixmap(fz_colorspace *srcs, fz_pixmap *src, fz_colorspace *dsts, fz_pixmap *dst)
{
	srcs->convpixmap(srcs, src, dsts, dst);
}

void
fz_convertcolor(fz_colorspace *srcs, float *srcv, fz_colorspace *dsts, float *dstv)
{
	srcs->convcolor(srcs, srcv, dsts, dstv);
}

fz_colorspace *
fz_keepcolorspace(fz_colorspace *cs)
{
	if (cs->refs < 0)
		return cs;
	cs->refs ++;
	return cs;
}

void
fz_dropcolorspace(fz_colorspace *cs)
{
	if (cs->refs < 0)
		return;
	if (--cs->refs == 0)
	{
		if (cs->drop)
			cs->drop(cs);
		fz_free(cs);
	}
}

void
fz_stdconvcolor(fz_colorspace *srcs, float *srcv, fz_colorspace *dsts, float *dstv)
{
	float xyz[3];
	int i;

	if (srcs != dsts)
	{
		srcs->toxyz(srcs, srcv, xyz);
		dsts->fromxyz(dsts, xyz, dstv);
		for (i = 0; i < dsts->n; i++)
			dstv[i] = CLAMP(dstv[i], 0.0, 1.0);
	}
	else
	{
		for (i = 0; i < srcs->n; i++)
			dstv[i] = srcv[i];
	}
}

void
fz_stdconvpixmap(fz_colorspace *srcs, fz_pixmap *src, fz_colorspace *dsts, fz_pixmap *dst)
{
	float srcv[FZ_MAXCOLORS];
	float dstv[FZ_MAXCOLORS];
	int y, x, k;

	unsigned char *s = src->samples;
	unsigned char *d = dst->samples;

#ifndef NDEBUG
	printf("convert pixmap from %s to %s\n", srcs->name, dsts->name);
#endif

	assert(src->w == dst->w && src->h == dst->h);
	assert(src->n == srcs->n + 1);
	assert(dst->n == dsts->n + 1);

	for (y = 0; y < src->h; y++)
	{
		for (x = 0; x < src->w; x++)
		{
			*d++ = *s++;

			for (k = 0; k < src->n - 1; k++)
				srcv[k] = *s++ / 255.0;

			fz_convertcolor(srcs, srcv, dsts, dstv);

			for (k = 0; k < dst->n - 1; k++)
				*d++ = dstv[k] * 255;
		}
	}
}

