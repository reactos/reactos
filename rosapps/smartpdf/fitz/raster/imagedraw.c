#include "fitz-base.h"
#include "fitz-world.h"
#include "fitz-draw.h"

typedef unsigned char byte;

#define lerp(a,b,t) (a + (((b - a) * t) >> 16))

static inline byte getcomp(byte *s, int w, int h, int u, int v, int n, int k)
{
	if (u < 0 || u >= w) return 0;
	if (v < 0 || v >= h) return 0;
	return s[(w * v + u) * n + k];
}

static inline int samplecomp(byte *s, int w, int h, int u, int v, int n, int k)
{
	int ui = u >> 16;
	int vi = v >> 16;
	int ud = u & 0xFFFF;
	int vd = v & 0xFFFF;
	int a = getcomp(s, w, h, ui, vi, n, k);
	int b = getcomp(s, w, h, ui+1, vi, n, k);
	int c = getcomp(s, w, h, ui, vi+1, n, k);
	int d = getcomp(s, w, h, ui+1, vi+1, n, k);
	int ab = lerp(a, b, ud);
	int cd = lerp(c, d, ud);
	return lerp(ab, cd, vd);
}

static inline byte getmask(byte *s, int w, int h, int u, int v)
{
	if (u < 0 || u >= w) return 0;
	if (v < 0 || v >= h) return 0;
	return s[w * v + u];
}

static inline int samplemask(byte *s, int w, int h, int u, int v)
{
	int ui = u >> 16;
	int vi = v >> 16;
	int ud = u & 0xFFFF;
	int vd = v & 0xFFFF;
	int a = getmask(s, w, h, ui, vi);
	int b = getmask(s, w, h, ui+1, vi);
	int c = getmask(s, w, h, ui, vi+1);
	int d = getmask(s, w, h, ui+1, vi+1);
	int ab = lerp(a, b, ud);
	int cd = lerp(c, d, ud);
	return lerp(ab, cd, vd);
}

static FORCEINLINE void lerpargb(byte *dst, byte *a, byte *b, int t)
{
	dst[0] = lerp(a[0], b[0], t);
	dst[1] = lerp(a[1], b[1], t);
	dst[2] = lerp(a[2], b[2], t);
	dst[3] = lerp(a[3], b[3], t);
}

static FORCEINLINE byte *getargb(byte *s, int w, int h, int u, int v)
{
	static byte zero[4] = { 0, 0, 0, 0 };
	if (u < 0 || u >= w) return zero;
	if (v < 0 || v >= h) return zero;
	return s + ((w * v + u) << 2);
}

static FORCEINLINE void sampleargb(byte *s, int w, int h, int u, int v, byte *abcd)
{
	byte ab[4];
	byte cd[4];
	int ui = u >> 16;
	int vi = v >> 16;
	int ud = u & 0xFFFF;
	int vd = v & 0xFFFF;
	byte *a = getargb(s, w, h, ui, vi);
	byte *b = getargb(s, w, h, ui+1, vi);
	byte *c = getargb(s, w, h, ui, vi+1);
	byte *d = getargb(s, w, h, ui+1, vi+1);
	lerpargb(ab, a, b, ud);
	lerpargb(cd, c, d, ud);
	lerpargb(abcd, ab, cd, vd);
}

static void img_ncn(FZ_PSRC, int srcn, FZ_PDST, FZ_PCTM)
{
	int k;
	while (h--)
	{
		byte *dstp = dst0;
		int u = u0;
		int v = v0;
		int w = w0;
		while (w--)
		{
			for (k = 0; k < srcn; k++)
			{
				dstp[k] = samplecomp(src, srcw, srch, u, v, srcn, k);
				dstp += srcn;
				u += fa;
				v += fb;
			}
		}
		dst0 += dstw;
		u0 += fc;
		v0 += fd;
	}
}

static void img_1c1(FZ_PSRC, FZ_PDST, FZ_PCTM)
{
	while (h--)
	{
		byte *dstp = dst0;
		int u = u0;
		int v = v0;
		int w = w0;
		while (w--)
		{
			dstp[0] = samplemask(src, srcw, srch, u, v);
			dstp ++;
			u += fa;
			v += fb;
		}
		dst0 += dstw;
		u0 += fc;
		v0 += fd;
	}
}

static void img_4c4(FZ_PSRC, FZ_PDST, FZ_PCTM)
{
	while (h--)
	{
		byte *dstp = dst0;
		int u = u0;
		int v = v0;
		int w = w0;
		while (w--)
		{
			sampleargb(src, srcw, srch, u, v, dstp);
			dstp += 4;
			u += fa;
			v += fb;
		}
		dst0 += dstw;
		u0 += fc;
		v0 += fd;
	}
}

static void img_1o1(FZ_PSRC, FZ_PDST, FZ_PCTM)
{
	byte srca;
	while (h--)
	{
		byte *dstp = dst0;
		int u = u0;
		int v = v0;
		int w = w0;
		while (w--)
		{
			srca = samplemask(src, srcw, srch, u, v);
			dstp[0] = srca + fz_mul255(dstp[0], 255 - srca);
			dstp ++;
			u += fa;
			v += fb;
		}
		dst0 += dstw;
		u0 += fc;
		v0 += fd;
	}
}

static void img_4o4(FZ_PSRC, FZ_PDST, FZ_PCTM)
{
	byte argb[4];
	byte ssa;
	while (h--)
	{
		byte *dstp = dst0;
		int u = u0;
		int v = v0;
		int w = w0;
		while (w--)
		{
			sampleargb(src, srcw, srch, u, v, argb);
			ssa = 255 - argb[0];
			dstp[0] = argb[0] + fz_mul255(dstp[0], ssa);
			dstp[1] = argb[1] + fz_mul255(dstp[1], ssa);
			dstp[2] = argb[2] + fz_mul255(dstp[2], ssa);
			dstp[3] = argb[3] + fz_mul255(dstp[3], ssa);
			dstp += 4;
			u += fa;
			v += fb;
		}
		dst0 += dstw;
		u0 += fc;
		v0 += fd;
	}
}

static void img_w3i1o4(byte *rgb, FZ_PSRC, FZ_PDST, FZ_PCTM)
{
	byte rgb0 = rgb[0];
	byte rgb1 = rgb[1];
	byte rgb2 = rgb[2];
	byte sa, ssa;
	while (h--)
	{
		byte *dstp = dst0;
		int u = u0;
		int v = v0;
		int w = w0;
		while (w--)
		{
			sa = samplemask(src, srcw, srch, u, v);
			ssa = 255 - sa;
			dstp[0] = sa + fz_mul255(dstp[0], ssa);
			dstp[1] = rgb0 + fz_mul255((short)dstp[1] - rgb0, ssa);
			dstp[2] = rgb1 + fz_mul255((short)dstp[2] - rgb1, ssa);
			dstp[3] = rgb2 + fz_mul255((short)dstp[3] - rgb2, ssa);
			dstp += 4;
			u += fa;
			v += fb;
		}
		dst0 += dstw;
		u0 += fc;
		v0 += fd;
	}
}

void (*fz_img_ncn)(FZ_PSRC, int sn, FZ_PDST, FZ_PCTM) = img_ncn;
void (*fz_img_1c1)(FZ_PSRC, FZ_PDST, FZ_PCTM) = img_1c1;
void (*fz_img_4c4)(FZ_PSRC, FZ_PDST, FZ_PCTM) = img_4c4;
void (*fz_img_1o1)(FZ_PSRC, FZ_PDST, FZ_PCTM) = img_1o1;
void (*fz_img_4o4)(FZ_PSRC, FZ_PDST, FZ_PCTM) = img_4o4;
void (*fz_img_w3i1o4)(byte*,FZ_PSRC,FZ_PDST,FZ_PCTM) = img_w3i1o4;

