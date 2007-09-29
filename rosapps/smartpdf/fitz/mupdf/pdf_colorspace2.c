#include <fitz.h>
#include <mupdf.h>

/*
 * Optimized color conversions for Device colorspaces
 */

static void fastgraytorgb(fz_pixmap *src, fz_pixmap *dst)
{
	unsigned char *s = src->samples;
	unsigned char *d = dst->samples;
	int n = src->w * src->h;
	while (n--)
	{
		d[0] = s[0];
		d[1] = s[1];
		d[2] = s[1];
		d[3] = s[1];
		s += 2;
		d += 4;
	}
}

static void fastgraytocmyk(fz_pixmap *src, fz_pixmap *dst)
{
	unsigned char *s = src->samples;
	unsigned char *d = dst->samples;
	int n = src->w * src->h;
	while (n--)
	{
		d[0] = s[0];
		d[1] = 0;
		d[2] = 0;
		d[3] = 0;
		d[4] = s[1];
		s += 2;
		d += 5;
	}
}

static void fastrgbtogray(fz_pixmap *src, fz_pixmap *dst)
{
	unsigned char *s = src->samples;
	unsigned char *d = dst->samples;
	int n = src->w * src->h;
	while (n--)
	{
		d[0] = s[0];
		d[1] = ((s[1]+1) * 77 + (s[2]+1) * 150 + (s[3]+1) * 28) >> 8;
		s += 4;
		d += 2;
	}
}

static void fastrgbtocmyk(fz_pixmap *src, fz_pixmap *dst)
{
	unsigned char *s = src->samples;
	unsigned char *d = dst->samples;
	int n = src->w * src->h;
	while (n--)
	{
		unsigned char c = 255 - s[1];
		unsigned char m = 255 - s[2];
		unsigned char y = 255 - s[3];
		unsigned char k = MIN(c, MIN(y, m));
		d[0] = s[0];
		d[1] = c - k;
		d[2] = m - k;
		d[3] = y - k;
		d[4] = k;
		s += 4;
		d += 5;
	}
}

static void fastcmyktogray(fz_pixmap *src, fz_pixmap *dst)
{
	unsigned char *s = src->samples;
	unsigned char *d = dst->samples;
	int n = src->w * src->h;
	while (n--)
	{
		unsigned char c = fz_mul255(s[1], 77);
		unsigned char m = fz_mul255(s[2], 150);
		unsigned char y = fz_mul255(s[3], 28);
		d[0] = s[0];
		d[1] = 255 - MIN(c + m + y + s[4], 255);
		s += 5;
		d += 2;
	}
}

static void fastcmyktorgb(fz_pixmap *src, fz_pixmap *dst)
{
	unsigned char *s = src->samples;
	unsigned char *d = dst->samples;
	int n = src->w * src->h;
	while (n--)
	{
		d[0] = s[0];
		d[1] = 255 - MIN(s[1] + s[4], 255);
		d[2] = 255 - MIN(s[2] + s[4], 255);
		d[3] = 255 - MIN(s[3] + s[4], 255);
		s += 5;
		d += 4;
	}
}

void pdf_convpixmap(fz_colorspace *ss, fz_pixmap *sp, fz_colorspace *ds, fz_pixmap *dp)
{
	if (ss == pdf_devicegray)
	{
		if (ds == pdf_devicergb) fastgraytorgb(sp, dp);
		else if (ds == pdf_devicecmyk) fastgraytocmyk(sp, dp);
		else fz_stdconvpixmap(ss, sp, ds, dp);
	}

	else if (ss == pdf_devicergb)
	{
		if (ds == pdf_devicegray) fastrgbtogray(sp, dp);
		else if (ds == pdf_devicecmyk) fastrgbtocmyk(sp, dp);
		else fz_stdconvpixmap(ss, sp, ds, dp);

	}

	else if (ss == pdf_devicecmyk)
	{
		if (ds == pdf_devicegray) fastcmyktogray(sp, dp);
		else if (ds == pdf_devicergb) fastcmyktorgb(sp, dp);
		else fz_stdconvpixmap(ss, sp, ds, dp);
	}

	else fz_stdconvpixmap(ss, sp, ds, dp);
}

void pdf_convcolor(fz_colorspace *ss, float *sv, fz_colorspace *ds, float *dv)
{

	if (ss == pdf_devicegray)
	{
		if (ds == pdf_devicergb)
		{
			dv[0] = sv[0];
			dv[1] = sv[0];
			dv[2] = sv[0];
		}
		else if (ds == pdf_devicecmyk)
		{
			dv[0] = 0;
			dv[1] = 0;
			dv[2] = 0;
			dv[3] = sv[0];
		}
		else
			fz_stdconvcolor(ss, sv, ds, dv);
	}

	else if (ss == pdf_devicergb)
	{
		if (ds == pdf_devicegray)
		{
			dv[0] = sv[0] * 0.3 + sv[1] * 0.59 + sv[2] * 0.11;
		}
		else if (ds == pdf_devicecmyk)
		{
			float c = 1.0 - sv[1];
			float m = 1.0 - sv[2];
			float y = 1.0 - sv[3];
			float k = MIN(c, MIN(y, m));
			dv[0] = c - k;
			dv[1] = m - k;
			dv[2] = y - k;
			dv[3] = k;
		}
		else
			fz_stdconvcolor(ss, sv, ds, dv);
	}

	else if (ss == pdf_devicecmyk)
	{
		if (ds == pdf_devicegray)
		{
			float c = sv[1] * 0.3;
			float m = sv[2] * 0.59;
			float y = sv[2] * 0.11;
			dv[0] = 1.0 - MIN(c + m + y + sv[3], 1.0);
		}
		else if (ds == pdf_devicergb)
		{
			dv[0] = 1.0 - MIN(sv[0] + sv[3], 1.0);
			dv[1] = 1.0 - MIN(sv[1] + sv[3], 1.0);
			dv[2] = 1.0 - MIN(sv[2] + sv[3], 1.0);
		}
		else
			fz_stdconvcolor(ss, sv, ds, dv);
	}

	else
		fz_stdconvcolor(ss, sv, ds, dv);
}

