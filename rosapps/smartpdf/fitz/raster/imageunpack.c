#include "fitz-base.h"
#include "fitz-world.h"
#include "fitz-draw.h"

typedef unsigned char byte;

/*
 * Apply decode parameters
 */

static void decodetile(fz_pixmap *pix, int skip, float *decode)
{
	int min[FZ_MAXCOLORS];
	int max[FZ_MAXCOLORS];
	int sub[FZ_MAXCOLORS];
	int needed = 0;
	byte *p = pix->samples;
	int n = pix->n;
	int wh = pix->w * pix->h;
	int i;

	min[0] = 0;
	max[0] = 255;
	sub[0] = 255;

	for (i = skip; i < n; i++)
	{
		min[i] = decode[(i - skip) * 2] * 255;
		max[i] = decode[(i - skip) * 2 + 1] * 255;
		sub[i] = max[i] - min[i];
                needed |= (min[i] != 0) |  (max[i] != 255);
	}

	if (!needed)
		return;

	while (wh--)
	{
		for (i = 0; i < n; i++)
			p[i] = min[i] + fz_mul255(sub[i], p[i]);
		p += n;
	}
}

/*
 * Unpack image samples and optionally pad pixels with opaque alpha
 */

#define tbit(buf,x) ((buf[x >> 3] >> ( 7 - (x & 7) ) ) & 1 ) * 255
#define ttwo(buf,x) ((buf[x >> 2] >> ( ( 3 - (x & 3) ) << 1 ) ) & 3 ) * 85
#define tnib(buf,x) ((buf[x >> 1] >> ( ( 1 - (x & 1) ) << 2 ) ) & 15 ) * 17
#define toct(buf,x) (buf[x])

static byte t1pad0[256][8];
static byte t1pad1[256][16];

static void init1()
{
	static int inited = 0;
	byte bits[1];
	int i, k, x;

	if (inited)
		return;

	for (i = 0; i < 256; i++)
	{
		bits[0] = i;
		for (k = 0; k < 8; k++)
		{
			x = tbit(bits, k);
			t1pad0[i][k] = x;
			t1pad1[i][k * 2 + 0] = 255;
			t1pad1[i][k * 2 + 1] = x;
		}
	}

	inited = 1;
}

static void loadtile1(byte *src, int sw, byte *dst, int dw, int w, int h, int pad)
{
	byte *sp;
	byte *dp;
	int x;

	init1();

	if (pad == 0)
	{
		int w3 = w >> 3;
		while (h--)
		{
			sp = src;
			dp = dst;
			for (x = 0; x < w3; x++)
			{
				memcpy(dp, t1pad0[*sp++], 8);
				dp += 8;
			}
			x = x << 3;
			if (x < w)
				memcpy(dp, t1pad0[*sp], w - x);
			src += sw;
			dst += dw;
		}
	}

	else if (pad == 1)
	{
		int w3 = w >> 3;
		while (h--)
		{
			sp = src;
			dp = dst;
			for (x = 0; x < w3; x++)
			{
				memcpy(dp, t1pad1[*sp++], 16);
				dp += 16;
			}
			x = x << 3;
			if (x < w)
				memcpy(dp, t1pad1[*sp], (w - x) << 1);
			src += sw;
			dst += dw;
		}
	}

	else
	{
		while (h--)
		{
			dp = dst;
			for (x = 0; x < w; x++)
			{
				if ((x % pad) == 0)
					*dp++ = 255;
				*dp++ = tbit(src, x);
			}
			src += sw;
			dst += dw;
		}
	}
}

#define TILE(getf) \
{ \
	int x; \
	if (!pad) \
		while (h--) \
		{ \
			for (x = 0; x < w; x++) \
				dst[x] = getf(src, x); \
			src += sw; \
			dst += dw; \
		} \
	else \
		while (h--) \
		{ \
			byte *dp = dst; \
			for (x = 0; x < w; x++) \
			{ \
				if ((x % pad) == 0) \
					*dp++ = 255; \
				*dp++ = getf(src, x); \
			} \
			src += sw; \
			dst += dw; \
		} \
}

static inline void loadtile8_fast_pad3(byte * restrict src, byte * restrict dst, int w, int h)
{
	int tocopy = (h * w) / 3;
	while (tocopy--)
	{
		*dst++ = 255;
		*dst++ = *src++;
		*dst++ = *src++;
		*dst++ = *src++;
	}
	/* TODO: if there was a reminder, copy it */
}

static void loadtile8_fast(byte * restrict src, int sw, byte * restrict dst, int dw, int w, int h, int pad)
{
	int x;

	if (!pad)
        {
                if (sw == dw)
                {
                    memmove(dst, src, h * w);
                }
                else
                {
		    while (h--)
		    {
			    memmove(dst, src, w);
			    src += sw;
			    dst += dw;
		    }
                }
        }
	else
        {
                int swdelta = sw - w;
	        int dwdelta = dw - w - (w / pad);
                if ( (0 == swdelta) && (0 == dwdelta) )
		{
			if (3 == pad)
				loadtile8_fast_pad3(src, dst, w, h);
			else
				while (h--)
				{
					for (x = 0; x < w; x++)
					{
						if ((x % pad) == 0)
							*dst++ = 255;
						*dst++ = *src++;
					}
				}
		}
		else
			while (h--)
			{
				for (x = 0; x < w; x++)
				{
					if ((x % pad) == 0)
						*dst++ = 255;
					*dst++ = *src++;
				}
				src += swdelta;
				dst += dwdelta;
			}
        }
}
static void loadtile2(byte * restrict src, int sw, byte * restrict dst, int dw, int w, int h, int pad)
	TILE(ttwo)
static void loadtile4(byte * restrict src, int sw, byte * restrict dst, int dw, int w, int h, int pad)
	TILE(tnib)
static void loadtile8_orig(byte * restrict src, int sw, byte * restrict dst, int dw, int w, int h, int pad)
	TILE(toct)

#define loadtile8 loadtile8_fast

void (*fz_decodetile)(fz_pixmap *pix, int skip, float *decode) = decodetile;
void (*fz_loadtile1)(byte*, int sw, byte*, int dw, int w, int h, int pad) = loadtile1;
void (*fz_loadtile2)(byte*, int sw, byte*, int dw, int w, int h, int pad) = loadtile2;
void (*fz_loadtile4)(byte*, int sw, byte*, int dw, int w, int h, int pad) = loadtile4;
void (*fz_loadtile8)(byte*, int sw, byte*, int dw, int w, int h, int pad) = loadtile8;
