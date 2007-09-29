#include "fitz-base.h"
#include "fitz-world.h"
#include "fitz-draw.h"

typedef unsigned char byte;

/*
 * Blend pixmap regions
 */

/* dst = src over dst */
static void
duff_non(byte * restrict sp0, int sw, int sn, byte * restrict dp0, int dw, int w0, int h)
{
	int k;
	while (h--)
	{
		byte *sp = sp0;
		byte *dp = dp0;
		int w = w0;
		while (w--)
		{
			byte sa = sp[0];
			byte ssa = 255 - sa;
			for (k = 0; k < sn; k++)
			{
				dp[k] = sp[k] + fz_mul255(dp[k], ssa);
			}
			sp += sn;
			dp += sn;
		}
		sp0 += sw;
		dp0 += dw;
	}
}

/* dst = src in msk */
static void
duff_nimcn(byte * restrict sp0, int sw, int sn, byte * restrict mp0, int mw, int mn, byte * restrict dp0, int dw, int w0, int h)
{
	int k;
	while (h--)
	{
		byte *sp = sp0;
		byte *mp = mp0;
		byte *dp = dp0;
		int w = w0;
		while (w--)
		{
			byte ma = mp[0];
			for (k = 0; k < sn; k++)
				dp[k] = fz_mul255(sp[k], ma);
			sp += sn;
			mp += mn;
			dp += sn;
		}
		sp0 += sw;
		mp0 += mw;
		dp0 += dw;
	}
}

/* dst = src in msk over dst */
static void
duff_nimon(byte * restrict sp0, int sw, int sn, byte * restrict mp0, int mw, int mn, byte * restrict dp0, int dw, int w0, int h)
{
	int k;
	while (h--)
	{
		byte *sp = sp0;
		byte *mp = mp0;
		byte *dp = dp0;
		int w = w0;
		while (w--)
		{
			/* TODO: validate this */
			byte ma = mp[0];
			byte sa = fz_mul255(sp[0], ma);
			byte ssa = 255 - sa;
			for (k = 0; k < sn; k++)
			{
				dp[k] = fz_mul255(sp[k], ma) + fz_mul255(dp[k], ssa);
			}
			sp += sn;
			mp += mn;
			dp += sn;
		}
		sp0 += sw;
		mp0 += mw;
		dp0 += dw;
	}
}

static void duff_1o1(byte * restrict sp0, int sw, byte * restrict dp0, int dw, int w0, int h)
{
	/* duff_non(sp0, sw, 1, dp0, dw, w0, h); */
	while (h--)
	{
		byte *sp = sp0;
		byte *dp = dp0;
		int w = w0;
		while (w--)
		{
			dp[0] = sp[0] + fz_mul255(dp[0], 255 - sp[0]);
			sp ++;
			dp ++;
		}
		sp0 += sw;
		dp0 += dw;
	}
}

static void duff_4o4(byte *sp0, int sw, byte *dp0, int dw, int w0, int h)
{
	/* duff_non(sp0, sw, 4, dp0, dw, w0, h); */
	while (h--)
	{
		byte *sp = sp0;
		byte *dp = dp0;
		int w = w0;
		while (w--)
		{
			byte ssa = 255 - sp[0];
			dp[0] = sp[0] + fz_mul255(dp[0], ssa);
			dp[1] = sp[1] + fz_mul255(dp[1], ssa);
			dp[2] = sp[2] + fz_mul255(dp[2], ssa);
			dp[3] = sp[3] + fz_mul255(dp[3], ssa);
			sp += 4;
			dp += 4;
		}
		sp0 += sw;
		dp0 += dw;
	}
}

static void duff_1i1c1(byte * restrict sp0, int sw, byte * restrict mp0, int mw, byte * restrict dp0, int dw, int w0, int h)
{
	/* duff_nimcn(sp0, sw, 1, mp0, mw, 1, dp0, dw, w0, h); */
	while (h--)
	{
		byte *sp = sp0;
		byte *mp = mp0;
		byte *dp = dp0;
		int w = w0;
		while (w--)
		{
			dp[0] = fz_mul255(sp[0], mp[0]);
			sp ++;
			mp ++;
			dp ++;
		}
		sp0 += sw;
		mp0 += mw;
		dp0 += dw;
	}
}

static void duff_4i1c4(byte * restrict sp0, int sw, byte * restrict mp0, int mw, byte * restrict dp0, int dw, int w0, int h)
{
	/* duff_nimcn(sp0, sw, 4, mp0, mw, 1, dp0, dw, w0, h); */
	while (h--)
	{
		byte *sp = sp0;
		byte *mp = mp0;
		byte *dp = dp0;
		int w = w0;
		while (w--)
		{
			byte ma = mp[0];
			dp[0] = fz_mul255(sp[0], ma);
			dp[1] = fz_mul255(sp[1], ma);
			dp[2] = fz_mul255(sp[2], ma);
			dp[3] = fz_mul255(sp[3], ma);
			sp += 4;
			mp += 1;
			dp += 4;
		}
		sp0 += sw;
		mp0 += mw;
		dp0 += dw;
	}
}

static void duff_1i1o1(byte * restrict sp0, int sw, byte * restrict mp0, int mw, byte * restrict dp0, int dw, int w0, int h)
{
	/* duff_nimon(sp0, sw, 1, mp0, mw, 1, dp0, dw, w0, h); */
	while (h--)
	{
		byte *sp = sp0;
		byte *mp = mp0;
		byte *dp = dp0;
		int w = w0;
		while (w--)
		{
			byte ma = mp[0];
			byte sa = fz_mul255(sp[0], ma);
			byte ssa = 255 - sa;
			dp[0] = fz_mul255(sp[0], ma) + fz_mul255(dp[0], ssa);
			sp ++;
			mp ++;
			dp ++;
		}
		sp0 += sw;
		mp0 += mw;
		dp0 += dw;
	}
}

static void duff_4i1o4(byte * restrict sp0, int sw, byte * restrict mp0, int mw, byte * restrict dp0, int dw, int w0, int h)
{
	/* duff_nimon(sp0, sw, 4, mp0, mw, 1, dp0, dw, w0, h); */
	while (h--)
	{
		byte *sp = sp0;
		byte *mp = mp0;
		byte *dp = dp0;
		int w = w0;
		while (w--)
		{
			byte ma = mp[0];
			byte sa = fz_mul255(sp[0], ma);
			byte ssa = 255 - sa;
			dp[0] = fz_mul255(sp[0], ma) + fz_mul255(dp[0], ssa);
			dp[1] = fz_mul255(sp[1], ma) + fz_mul255(dp[1], ssa);
			dp[2] = fz_mul255(sp[2], ma) + fz_mul255(dp[2], ssa);
			dp[3] = fz_mul255(sp[3], ma) + fz_mul255(dp[3], ssa);
			sp += 4;
			mp += 1;
			dp += 4;
		}
		sp0 += sw;
		mp0 += mw;
		dp0 += dw;
	}
}

/*
 * Path and text masks
 */

static void path_1c1(byte * restrict src, int cov, int len, byte * restrict dst)
{
	while (len--)
	{
		cov += *src; *src = 0; src++;
		*dst++ = cov;
	}
}

static void path_1o1(byte * restrict src, int cov, int len, byte * restrict dst)
{
	while (len--)
	{
		cov += *src; *src = 0; src++;
		dst[0] = cov + fz_mul255(dst[0], 255 - cov);
		dst++;
	}
}

static void path_w3i1o4(byte * restrict rgb, byte * restrict src, int cov, int len, byte * restrict dst)
{
	byte rgb0 = rgb[0];
	byte rgb1 = rgb[1];
	byte rgb2 = rgb[2];
	byte ssa;
	while (len--)
	{
		cov += *src; *src = 0; src++;
		ssa = 255 - cov;
		dst[0] = cov + fz_mul255(dst[0], ssa);
		dst[1] = rgb0 + fz_mul255((short)dst[1] - rgb0, ssa);
		dst[2] = rgb1 + fz_mul255((short)dst[2] - rgb1, ssa);
		dst[3] = rgb2 + fz_mul255((short)dst[3] - rgb2, ssa);
		dst += 4;
	}
}

static void text_1c1(byte * restrict src0, int srcw, byte * restrict dst0, int dstw, int w0, int h)
{
	while (h--)
	{
		byte *src = src0;
		byte *dst = dst0;
		int w = w0;
		while (w--)
		{
			*dst++ = *src++;
		}
		src0 += srcw;
		dst0 += dstw;
	}
}

static void text_1o1(byte * restrict src0, int srcw, byte * restrict dst0, int dstw, int w0, int h)
{
	while (h--)
	{
		byte *src = src0;
		byte *dst = dst0;
		int w = w0;
		while (w--)
		{
			dst[0] = src[0] + fz_mul255(dst[0], 255 - src[0]);
			src++;
			dst++;
		}
		src0 += srcw;
		dst0 += dstw;
	}
}

static void text_w3i1o4(byte * restrict rgb, byte * restrict src0, int srcw, byte * restrict dst0, int dstw, int w0, int h)
{
	unsigned char rgb0 = rgb[0];
	unsigned char rgb1 = rgb[1];
	unsigned char rgb2 = rgb[2];
	while (h--)
	{
		byte *src = src0;
		byte *dst = dst0;
		int w = w0;
		while (w--)
		{
			byte sa = src[0];
			byte ssa = 255 - sa;
			dst[0] = sa + fz_mul255(dst[0], ssa);
			dst[1] = rgb0 + fz_mul255((short)dst[1] - rgb0, ssa);
			dst[2] = rgb1 + fz_mul255((short)dst[2] - rgb1, ssa);
			dst[3] = rgb2 + fz_mul255((short)dst[3] - rgb2, ssa);
			src ++;
			dst += 4;
		}
		src0 += srcw;
		dst0 += dstw;
	}
}

/*
 * ... and the function pointers
 */

void (*fz_duff_non)(byte*,int,int,byte*,int,int,int) = duff_non;
void (*fz_duff_nimcn)(byte*,int,int,byte*,int,int,byte*,int,int,int) = duff_nimcn;
void (*fz_duff_nimon)(byte*,int,int,byte*,int,int,byte*,int,int,int) = duff_nimon;
void (*fz_duff_1o1)(byte*,int,byte*,int,int,int) = duff_1o1;
void (*fz_duff_4o4)(byte*,int,byte*,int,int,int) = duff_4o4;
void (*fz_duff_1i1c1)(byte*,int,byte*,int,byte*,int,int,int) = duff_1i1c1;
void (*fz_duff_4i1c4)(byte*,int,byte*,int,byte*,int,int,int) = duff_4i1c4;
void (*fz_duff_1i1o1)(byte*,int,byte*,int,byte*,int,int,int) = duff_1i1o1;
void (*fz_duff_4i1o4)(byte*,int,byte*,int,byte*,int,int,int) = duff_4i1o4;

void (*fz_path_1c1)(byte*,int,int,byte*) = path_1c1;
void (*fz_path_1o1)(byte*,int,int,byte*) = path_1o1;
void (*fz_path_w3i1o4)(byte*,byte*,int,int,byte*) = path_w3i1o4;

void (*fz_text_1c1)(byte*,int,byte*,int,int,int) = text_1c1;
void (*fz_text_1o1)(byte*,int,byte*,int,int,int) = text_1o1;
void (*fz_text_w3i1o4)(byte*,byte*,int,byte*,int,int,int) = text_w3i1o4;

