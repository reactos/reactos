/*
 * PowerPC specific render optims live here
 */

#include "fitz-base.h"
#include "fitz-world.h"
#include "fitz-draw.h"

typedef unsigned char byte;

#ifdef HAVE_ALTIVEC

static void srow1ppc(byte *src, byte *dst, int w, int denom)
{
	int x, left;
	int sum;

	left = 0;
	sum = 0;

	for (x = 0; x < w; x++)
	{
		sum += *src++;
		if (++left == denom)
		{
			left = 0;
			*dst++ = sum / denom;
			sum = 0;
		}
	}

	if (left)
		*dst++ = sum / left;
}

static void scol1ppc(byte *src, byte *dst, int w, int denom)
{
	int x, y;
	unsigned char *s;
	int sum;

	for (x = 0; x < w; x++)
	{
		s = src + x;
		sum = 0;
		for (y = 0; y < denom; y++)
			sum += s[y * w];
		*dst++ = sum / denom;
	}
}

#endif /* HAVE_ALTIVEC */

#if defined (ARCH_PPC)
void
fz_accelerate(void)
{
#  ifdef HAVE_ALTIVEC
	if (fz_cpuflags & HAVE_ALTIVEC)
	{
		fz_srow1 = srow1ppc;
		fz_scol1 = scol1ppc;
	}
#  endif
}
#endif

