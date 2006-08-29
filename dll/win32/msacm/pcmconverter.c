/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 *      MSACM32 library
 *
 *      Copyright 2000		Eric Pouech
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *	FIXME / TODO list
 *	+ most of the computation should be done in fixed point arithmetic
 *	  instead of floating point (16 bits for integral part, and 16 bits
 *	  for fractional part for example)
 *	+ implement PCM_FormatSuggest function
 *	+ get rid of hack for PCM_DriverProc (msacm32.dll shouldn't export
 *	  a DriverProc, but this would require implementing a generic
 *	  embedded driver handling scheme in msacm32.dll which isn't done yet
 */

#include "config.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "mmsystem.h"
#include "mmreg.h"
#include "msacm.h"
#include "wingdi.h"
#include "winnls.h"
#include "winuser.h"

#include "msacmdrv.h"
#include "wineacm.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msacm);

/***********************************************************************
 *           PCM_drvOpen
 */
static	DWORD	PCM_drvOpen(LPCSTR str, PACMDRVOPENDESCW adod)
{
    TRACE("(%p, %p)\n", str, adod);

    return (adod == NULL) ||
	(adod->fccType == ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC &&
	 adod->fccComp == ACMDRIVERDETAILS_FCCCOMP_UNDEFINED);
}

/***********************************************************************
 *           PCM_drvClose
 */
static	DWORD	PCM_drvClose(DWORD dwDevID)
{
    TRACE("(%ld)\n", dwDevID);

    return 1;
}

#define	NUM_PCM_FORMATS	(sizeof(PCM_Formats) / sizeof(PCM_Formats[0]))
#define	NUM_OF(a,b)	(((a)+(b)-1)/(b))

/* flags for fdwDriver */
#define PCM_RESAMPLE	1

/* data used while converting */
typedef struct tagAcmPcmData {
    /* conversion routine, depending if rate conversion is required */
    union {
	void (*cvtKeepRate)(const unsigned char*, int, unsigned char*);
	void (*cvtChangeRate)(struct tagAcmPcmData*, const unsigned char*,
			      LPDWORD, unsigned char*, LPDWORD);
    } cvt;
    /* the following fields are used only with rate conversion) */
    DWORD	srcPos;		/* position in source stream */
    double	dstPos;		/* position in destination stream */
    double	dstIncr;	/* value to increment dst stream when src stream
				   is incremented by 1 */
    /* last source stream value read */
    union {
	unsigned char	b;	/*  8 bit value */
	short		s;	/* 16 bit value */
    } last[2]; /* two channels max (stereo) */
} AcmPcmData;

/* table to list all supported formats... those are the basic ones. this
 * also helps given a unique index to each of the supported formats
 */
static	struct {
    int		nChannels;
    int		nBits;
    int		rate;
} PCM_Formats[] = {
    {1,  8,  8000}, {2,  8,  8000}, {1, 16,  8000}, {2, 16,  8000},
    {1,  8, 11025}, {2,  8, 11025}, {1, 16, 11025}, {2, 16, 11025},
    {1,  8, 22050}, {2,  8, 22050}, {1, 16, 22050}, {2, 16, 22050},
    {1,  8, 44100}, {2,  8, 44100}, {1, 16, 44100}, {2, 16, 44100},
    {1,  8, 48000}, {2,  8, 48000}, {1, 16, 48000}, {2, 16, 48000},
    {1,  8, 96000}, {2,  8, 96000}, {1, 16, 96000}, {2, 16, 96000}
};

/***********************************************************************
 *           PCM_GetFormatIndex
 */
static DWORD PCM_GetFormatIndex(LPWAVEFORMATEX wfx)
{
    int i;
    TRACE("(%p)\n", wfx);

    for (i = 0; i < NUM_PCM_FORMATS; i++) {
	if (wfx->nChannels == PCM_Formats[i].nChannels &&
	    wfx->nSamplesPerSec == PCM_Formats[i].rate &&
	    wfx->wBitsPerSample == PCM_Formats[i].nBits)
	    return i;
    }
    return 0xFFFFFFFF;
}

/* PCM Conversions:
 *
 * parameters:
 *	+ 8 bit unsigned vs 16 bit signed
 *	+ mono vs stereo (1 or 2 channels)
 *	+ sampling rate (8.0, 11.025, 22.05, 44.1 kHz are defined, but algo
 *	  shall work in all cases)
 *
 * mono => stereo: copy the same sample on Left & Right channels
 * stereo =) mono: use the average value of samples from Left & Right channels
 * resampling; we lookup for each destination sample the two source adjacent
 *      samples were src <= dst < src+1 (dst is increased by a fractional
 *      value which is equivalent to the increment by one on src); then we
 *      use a linear interpolation between src and src+1
 */

/***********************************************************************
 *           C816
 *
 * Converts a 8 bit sample to a 16 bit one
 */
static inline short C816(unsigned char b)
{
    return (short)((b+(b << 8))-32768);
}

/***********************************************************************
 *           C168
 *
 * Converts a 16 bit sample to a 8 bit one (data loss !!)
 */
static inline unsigned char C168(short s)
{
    return HIBYTE(s) ^ (unsigned char)0x80;
}

/***********************************************************************
 *           R16
 *
 * Read a 16 bit sample (correctly handles endianess)
 */
static inline short  R16(const unsigned char* src)
{
    return (short)((unsigned short)src[0] | ((unsigned short)src[1] << 8));
}

/***********************************************************************
 *           W16
 *
 * Write a 16 bit sample (correctly handles endianess)
 */
static inline void  W16(unsigned char* dst, short s)
{
    dst[0] = LOBYTE(s);
    dst[1] = HIBYTE(s);
}

/***********************************************************************
 *           M16
 *
 * Convert the (l,r) 16 bit stereo sample into a 16 bit mono
 * (takes the mid-point of the two values)
 */
static inline short M16(short l, short r)
{
    return (l + r) / 2;
}

/***********************************************************************
 *           M8
 *
 * Convert the (l,r) 8 bit stereo sample into a 8 bit mono
 * (takes the mid-point of the two values)
 */
static inline unsigned char M8(unsigned char a, unsigned char b)
{
    return (unsigned char)((a + b) / 2);
}

/* the conversion routines without rate conversion are labelled cvt<X><Y><N><M>K
 * where :
 * <X> is the (M)ono/(S)tereo configuration of  input channel
 * <Y> is the (M)ono/(S)tereo configuration of output channel
 * <N> is the number of bits of  input channel (8 or 16)
 * <M> is the number of bits of output channel (8 or 16)
 *
 * in the parameters, ns is always the number of samples, so the size of input
 * buffer (resp output buffer) is ns * (<X> == 'Mono' ? 1:2) * (<N> == 8 ? 1:2)
 */

static	void cvtMM88K(const unsigned char* src, int ns, unsigned char* dst)
{
    TRACE("(%p, %d, %p)\n", src, ns, dst);
    memcpy(dst, src, ns);
}

static	void cvtSS88K(const unsigned char* src, int ns, unsigned char* dst)
{
    TRACE("(%p, %d, %p)\n", src, ns, dst);
    memcpy(dst, src, ns * 2);
}

static	void cvtMM1616K(const unsigned char* src, int ns, unsigned char* dst)
{
    TRACE("(%p, %d, %p)\n", src, ns, dst);
    memcpy(dst, src, ns * 2);
}

static	void cvtSS1616K(const unsigned char* src, int ns, unsigned char* dst)
{
    TRACE("(%p, %d, %p)\n", src, ns, dst);
    memcpy(dst, src, ns * 4);
}

static	void cvtMS88K(const unsigned char* src, int ns, unsigned char* dst)
{
    TRACE("(%p, %d, %p)\n", src, ns, dst);

    while (ns--) {
	*dst++ = *src;
	*dst++ = *src++;
    }
}

static	void cvtMS816K(const unsigned char* src, int ns, unsigned char* dst)
{
    short	v;
    TRACE("(%p, %d, %p)\n", src, ns, dst);

    while (ns--) {
	v = C816(*src++);
	W16(dst, v);		dst += 2;
	W16(dst, v);		dst += 2;
    }
}

static	void cvtMS168K(const unsigned char* src, int ns, unsigned char* dst)
{
    unsigned char v;
    TRACE("(%p, %d, %p)\n", src, ns, dst);

    while (ns--) {
	v = C168(R16(src));		src += 2;
	*dst++ = v;
	*dst++ = v;
    }
}

static	void cvtMS1616K(const unsigned char* src, int ns, unsigned char* dst)
{
    short	v;
    TRACE("(%p, %d, %p)\n", src, ns, dst);

    while (ns--) {
	v = R16(src);		src += 2;
	W16(dst, v);		dst += 2;
	W16(dst, v);		dst += 2;
    }
}

static	void cvtSM88K(const unsigned char* src, int ns, unsigned char* dst)
{
    TRACE("(%p, %d, %p)\n", src, ns, dst);

    while (ns--) {
	*dst++ = M8(src[0], src[1]);
	src += 2;
    }
}

static	void cvtSM816K(const unsigned char* src, int ns, unsigned char* dst)
{
    short	v;
    TRACE("(%p, %d, %p)\n", src, ns, dst);

    while (ns--) {
	v = M16(C816(src[0]), C816(src[1]));
	src += 2;
	W16(dst, v);		dst += 2;
    }
}

static	void cvtSM168K(const unsigned char* src, int ns, unsigned char* dst)
{
    TRACE("(%p, %d, %p)\n", src, ns, dst);

    while (ns--) {
	*dst++ = C168(M16(R16(src), R16(src + 2)));
	src += 4;
    }
}

static	void cvtSM1616K(const unsigned char* src, int ns, unsigned char* dst)
{
    TRACE("(%p, %d, %p)\n", src, ns, dst);

    while (ns--) {
	W16(dst, M16(R16(src),R16(src+2)));	dst += 2;
	src += 4;
    }
}

static	void cvtMM816K(const unsigned char* src, int ns, unsigned char* dst)
{
    TRACE("(%p, %d, %p)\n", src, ns, dst);

    while (ns--) {
	W16(dst, C816(*src++));		dst += 2;
    }
}

static	void cvtSS816K(const unsigned char* src, int ns, unsigned char* dst)
{
    TRACE("(%p, %d, %p)\n", src, ns, dst);

    while (ns--) {
	W16(dst, C816(*src++));	dst += 2;
	W16(dst, C816(*src++));	dst += 2;
    }
}

static	void cvtMM168K(const unsigned char* src, int ns, unsigned char* dst)
{
    TRACE("(%p, %d, %p)\n", src, ns, dst);

    while (ns--) {
	*dst++ = C168(R16(src));	src += 2;
    }
}

static	void cvtSS168K(const unsigned char* src, int ns, unsigned char* dst)
{
    TRACE("(%p, %d, %p)\n", src, ns, dst);

    while (ns--) {
	*dst++ = C168(R16(src));	src += 2;
	*dst++ = C168(R16(src));	src += 2;
    }
}

static	void (*PCM_ConvertKeepRate[16])(const unsigned char*, int, unsigned char*) = {
    cvtSS88K,	cvtSM88K,   cvtMS88K,   cvtMM88K,
    cvtSS816K,	cvtSM816K,  cvtMS816K,  cvtMM816K,
    cvtSS168K,	cvtSM168K,  cvtMS168K,  cvtMM168K,
    cvtSS1616K, cvtSM1616K, cvtMS1616K, cvtMM1616K,
};

/***********************************************************************
 *           I
 *
 * Interpolate the value at r (r in ]0, 1]) between the two points v1 and v2
 * Linear interpolation is used
 */
static	inline double	I(double v1, double v2, double r)
{
    if (0.0 >= r || r > 1.0) FIXME("r!! %f\n", r);
    return (1.0 - r) * v1 + r * v2;
}

static	void cvtSS88C(AcmPcmData* apd, const unsigned char* src, LPDWORD nsrc,
		      unsigned char* dst, LPDWORD ndst)
{
    double     		r;
    TRACE("(%p, %p, %p, %p, %p)\n", apd, src, nsrc, dst, ndst);

    while (*nsrc != 0 && *ndst != 0) {
	while ((r = (double)apd->srcPos - apd->dstPos) <= 0) {
	    if (*nsrc == 0) return;
	    apd->last[0].b = *src++;
	    apd->last[1].b = *src++;
	    apd->srcPos++;
	    (*nsrc)--;
	}
	/* now do the interpolation */
	*dst++ = I(apd->last[0].b, src[0], r);
	*dst++ = I(apd->last[1].b, src[1], r);
	apd->dstPos += apd->dstIncr;
	(*ndst)--;
    }
}

/* the conversion routines with rate conversion are labelled cvt<X><Y><N><M>C
 * where :
 * <X> is the (M)ono/(S)tereo configuration of  input channel
 * <Y> is the (M)ono/(S)tereo configuration of output channel
 * <N> is the number of bits of  input channel (8 or 16)
 * <M> is the number of bits of output channel (8 or 16)
 *
 */
static	void cvtSM88C(AcmPcmData* apd, const unsigned char* src, LPDWORD nsrc,
		      unsigned char* dst, LPDWORD ndst)
{
    double   	r;
    TRACE("(%p, %p, %p, %p, %p)\n", apd, src, nsrc, dst, ndst);

    while (*nsrc != 0 && *ndst != 0) {
	while ((r = (double)apd->srcPos - apd->dstPos) <= 0) {
	    if (*nsrc == 0) return;
	    apd->last[0].b = *src++;
	    apd->last[1].b = *src++;
	    apd->srcPos++;
	    (*nsrc)--;
	}
	/* now do the interpolation */
        if (*nsrc)	/* don't go off end of data */
            *dst++ = I(M8(apd->last[0].b, apd->last[1].b), M8(src[0], src[1]), r);
        else
            *dst++ = M8(apd->last[0].b, apd->last[1].b);
	apd->dstPos += apd->dstIncr;
	(*ndst)--;
    }
}

static	void cvtMS88C(AcmPcmData* apd, const unsigned char* src, LPDWORD nsrc,
		      unsigned char* dst, LPDWORD ndst)
{
    double	r;
    TRACE("(%p, %p, %p, %p, %p)\n", apd, src, nsrc, dst, ndst);

    while (*nsrc != 0 && *ndst != 0) {
	while ((r = (double)apd->srcPos - apd->dstPos) <= 0) {
	    if (*nsrc == 0) return;
	    apd->last[0].b = *src++;
	    apd->srcPos++;
	    (*nsrc)--;
	}
	/* now do the interpolation */
        if (*nsrc)	/* don't go off end of data */
            dst[0] = dst[1] = I(apd->last[0].b, src[0], r);
        else
            dst[0] = dst[1] = apd->last[0].b;
	dst += 2;
	apd->dstPos += apd->dstIncr;
	(*ndst)--;
    }
}

static	void cvtMM88C(AcmPcmData* apd, const unsigned char* src, LPDWORD nsrc,
		      unsigned char* dst, LPDWORD ndst)
{
    double     	r;
    TRACE("(%p, %p, %p, %p, %p)\n", apd, src, nsrc, dst, ndst);

    while (*nsrc != 0 && *ndst != 0) {
	while ((r = (double)apd->srcPos - apd->dstPos) <= 0) {
	    if (*nsrc == 0) return;
	    apd->last[0].b = *src++;
	    apd->srcPos++;
	    (*nsrc)--;
	}
	/* now do the interpolation */
        if (*nsrc)	/* don't go off end of data */
            *dst++ = I(apd->last[0].b, src[0], r);
        else
            *dst++ = apd->last[0].b;
	apd->dstPos += apd->dstIncr;
	(*ndst)--;
    }
}

static	void cvtSS816C(AcmPcmData* apd, const unsigned char* src, LPDWORD nsrc,
		       unsigned char* dst, LPDWORD ndst)
{
    double	r;
    TRACE("(%p, %p, %p, %p, %p)\n", apd, src, nsrc, dst, ndst);

    while (*nsrc != 0 && *ndst != 0) {
	while ((r = (double)apd->srcPos - apd->dstPos) <= 0) {
	    if (*nsrc == 0) return;
	    apd->last[0].b = *src++;
	    apd->last[1].b = *src++;
	    apd->srcPos++;
	    (*nsrc)--;
	}
	/* now do the interpolation */
        if (*nsrc)	/* don't go off end of data */
            W16(dst, I(C816(apd->last[0].b), C816(src[0]), r));
        else
            W16(dst, C816(apd->last[0].b));
        dst += 2;
        if (*nsrc)	/* don't go off end of data */
	    W16(dst, I(C816(apd->last[1].b), C816(src[1]), r));
        else
	    W16(dst, C816(apd->last[1].b));
        dst += 2;
	apd->dstPos += apd->dstIncr;
	(*ndst)--;
    }
}

static	void cvtSM816C(AcmPcmData* apd, const unsigned char* src, LPDWORD nsrc,
			unsigned char* dst, LPDWORD ndst)
{
    double     	r;
    TRACE("(%p, %p, %p, %p, %p)\n", apd, src, nsrc, dst, ndst);

    while (*nsrc != 0 && *ndst != 0) {
	while ((r = (double)apd->srcPos - apd->dstPos) <= 0) {
	    if (*nsrc == 0) return;
	    apd->last[0].b = *src++;
	    apd->last[1].b = *src++;
	    apd->srcPos++;
	    (*nsrc)--;
	}
	/* now do the interpolation */
        if (*nsrc)	/* don't go off end of data */
            W16(dst, I(M16(C816(apd->last[0].b), C816(apd->last[1].b)),
                       M16(C816(src[0]), C816(src[1])), r));
        else
            W16(dst, M16(C816(apd->last[0].b), C816(apd->last[1].b)));
	dst += 2;
	apd->dstPos += apd->dstIncr;
	(*ndst)--;
    }
}

static	void cvtMS816C(AcmPcmData* apd, const unsigned char* src, LPDWORD nsrc,
			unsigned char* dst, LPDWORD ndst)
{
    double     	r;
    short	v;
    TRACE("(%p, %p, %p->(%ld), %p, %p->(%ld))\n", apd, src, nsrc, *nsrc, dst, ndst, *ndst);

    while (*nsrc != 0 && *ndst != 0) {
	while ((r = (double)apd->srcPos - apd->dstPos) <= 0) {
	    if (*nsrc == 0) return;
	    apd->last[0].b = *src++;
	    apd->srcPos++;
	    (*nsrc)--;
	}
	/* now do the interpolation */
        if (*nsrc)	/* don't go off end of data */
	    v = I(C816(apd->last[0].b), C816(src[0]), r);
        else
            v = C816(apd->last[0].b);
	W16(dst, v);		dst += 2;
	W16(dst, v);		dst += 2;
	apd->dstPos += apd->dstIncr;
	(*ndst)--;
    }
}

static	void cvtMM816C(AcmPcmData* apd, const unsigned char* src, LPDWORD nsrc,
			unsigned char* dst, LPDWORD ndst)
{
    double     	r;
    TRACE("(%p, %p, %p, %p, %p)\n", apd, src, nsrc, dst, ndst);

    while (*nsrc != 0 && *ndst != 0) {
	while ((r = (double)apd->srcPos - apd->dstPos) <= 0) {
	    if (*nsrc == 0) return;
	    apd->last[0].b = *src++;
	    apd->srcPos++;
	    (*nsrc)--;
	}
	/* now do the interpolation */
        if (*nsrc)	/* don't go off end of data */
	    W16(dst, I(C816(apd->last[0].b), C816(src[0]), r));
        else
            W16(dst, C816(apd->last[0].b));
	dst += 2;
	apd->dstPos += apd->dstIncr;
	(*ndst)--;
    }
}

static	void cvtSS168C(AcmPcmData* apd, const unsigned char* src, LPDWORD nsrc,
			unsigned char* dst, LPDWORD ndst)
{
    double     	r;
    TRACE("(%p, %p, %p, %p, %p)\n", apd, src, nsrc, dst, ndst);

    while (*nsrc != 0 && *ndst != 0) {
	while ((r = (double)apd->srcPos - apd->dstPos) <= 0) {
	    if (*nsrc == 0) return;
	    apd->last[0].s = R16(src);	src += 2;
	    apd->last[1].s = R16(src);	src += 2;
	    apd->srcPos++;
	    (*nsrc)--;
	}
	/* now do the interpolation */
        if (*nsrc) {	/* don't go off end of data */
	    *dst++ = C168(I(apd->last[0].s, R16(src)  , r));
	    *dst++ = C168(I(apd->last[1].s, R16(src+2), r));
        } else {
	    *dst++ = C168(apd->last[0].s);
	    *dst++ = C168(apd->last[1].s);
        }
	apd->dstPos += apd->dstIncr;
	(*ndst)--;
    }
}

static	void cvtSM168C(AcmPcmData* apd, const unsigned char* src, LPDWORD nsrc,
			unsigned char* dst, LPDWORD ndst)
{
    double     	r;
    TRACE("(%p, %p, %p, %p, %p)\n", apd, src, nsrc, dst, ndst);

    while (*nsrc != 0 && *ndst != 0) {
	while ((r = (double)apd->srcPos - apd->dstPos) <= 0) {
	    if (*nsrc == 0) return;
	    apd->last[0].s = R16(src);	src += 2;
	    apd->last[1].s = R16(src);	src += 2;
	    apd->srcPos++;
	    (*nsrc)--;
	}
	/* now do the interpolation */
        if (*nsrc)	/* don't go off end of data */
	    *dst++ = C168(I(M16(apd->last[0].s, apd->last[1].s),
			    M16(R16(src), R16(src + 2)), r));
        else
	    *dst++ = C168(M16(apd->last[0].s, apd->last[1].s));
	apd->dstPos += apd->dstIncr;
	(*ndst)--;
    }
}


static	void cvtMS168C(AcmPcmData* apd, const unsigned char* src, LPDWORD nsrc,
			unsigned char* dst, LPDWORD ndst)
{
    double     	r;
    TRACE("(%p, %p, %p, %p, %p)\n", apd, src, nsrc, dst, ndst);

    while (*nsrc != 0 && *ndst != 0) {
	while ((r = (double)apd->srcPos - apd->dstPos) <= 0) {
	    if (*nsrc == 0) return;
	    apd->last[0].s = R16(src);	src += 2;
	    apd->srcPos++;
	    (*nsrc)--;
	}
	/* now do the interpolation */
        if (*nsrc)	/* don't go off end of data */
	    dst[0] = dst[1] = C168(I(apd->last[0].s, R16(src), r));
        else
	    dst[0] = dst[1] = C168(apd->last[0].s);
        dst += 2;
	apd->dstPos += apd->dstIncr;
	(*ndst)--;
    }
}


static	void cvtMM168C(AcmPcmData* apd, const unsigned char* src, LPDWORD nsrc,
			unsigned char* dst, LPDWORD ndst)
{
    double     	r;
    TRACE("(%p, %p, %p, %p, %p)\n", apd, src, nsrc, dst, ndst);

    while (*nsrc != 0 && *ndst != 0) {
	while ((r = (double)apd->srcPos - apd->dstPos) <= 0) {
	    if (*nsrc == 0) return;
	    apd->last[0].s = R16(src);	src += 2;
	    apd->srcPos++;
	    (*nsrc)--;
	}
	/* now do the interpolation */
        if (*nsrc)	/* don't go off end of data */
	    *dst++ = C168(I(apd->last[0].s, R16(src), r));
        else
	    *dst++ = C168(apd->last[0].s);
	apd->dstPos += apd->dstIncr;
	(*ndst)--;
    }
}

static	void cvtSS1616C(AcmPcmData* apd, const unsigned char* src, LPDWORD nsrc,
			unsigned char* dst, LPDWORD ndst)
{
    double     	r;
    TRACE("(%p, %p, %p, %p, %p)\n", apd, src, nsrc, dst, ndst);

    while (*nsrc != 0 && *ndst != 0) {
	while ((r = (double)apd->srcPos - apd->dstPos) <= 0) {
	    if (*nsrc == 0) return;
	    apd->last[0].s = R16(src);	src += 2;
	    apd->last[1].s = R16(src);	src += 2;
	    apd->srcPos++;
	    (*nsrc)--;
	}
	/* now do the interpolation */
        if (*nsrc)	/* don't go off end of data */
	    W16(dst, I(apd->last[0].s, R16(src), r));
        else
	    W16(dst, apd->last[0].s);
        dst += 2;
        if (*nsrc)	/* don't go off end of data */
	    W16(dst, I(apd->last[1].s, R16(src+2), r));
        else
	    W16(dst, apd->last[1].s);
        dst += 2;
	apd->dstPos += apd->dstIncr;
	(*ndst)--;
    }
}

static	void cvtSM1616C(AcmPcmData* apd, const unsigned char* src, LPDWORD nsrc,
			unsigned char* dst, LPDWORD ndst)
{
    double     	r;
    TRACE("(%p, %p, %p, %p, %p)\n", apd, src, nsrc, dst, ndst);

    while (*nsrc != 0 && *ndst != 0) {
	while ((r = (double)apd->srcPos - apd->dstPos) <= 0) {
	    if (*nsrc == 0) return;
	    apd->last[0].s = R16(src);	src += 2;
	    apd->last[1].s = R16(src);	src += 2;
	    apd->srcPos++;
	    (*nsrc)--;
	}
	/* now do the interpolation */
        if (*nsrc)	/* don't go off end of data */
            W16(dst, I(M16(apd->last[0].s, apd->last[1].s),
 		       M16(R16(src), R16(src+2)), r));
        else
            W16(dst, M16(apd->last[0].s, apd->last[1].s));
 	dst += 2;
	apd->dstPos += apd->dstIncr;
	(*ndst)--;
    }
}

static	void cvtMS1616C(AcmPcmData* apd, const unsigned char* src, LPDWORD nsrc,
			unsigned char* dst, LPDWORD ndst)
{
    double     	r;
    short	v;
    TRACE("(%p, %p, %p, %p, %p)\n", apd, src, nsrc, dst, ndst);

    while (*nsrc != 0 && *ndst != 0) {
	while ((r = (double)apd->srcPos - apd->dstPos) <= 0) {
	    if (*nsrc == 0) return;
	    apd->last[0].s = R16(src);	src += 2;
	    apd->srcPos++;
	    (*nsrc)--;
	}
	/* now do the interpolation */
        if (*nsrc)	/* don't go off end of data */
	    v = I(apd->last[0].s, R16(src), r);
        else
	    v = apd->last[0].s;
	W16(dst, v);		dst += 2;
	W16(dst, v);		dst += 2;
	apd->dstPos += apd->dstIncr;
	(*ndst)--;
    }
}

static	void cvtMM1616C(AcmPcmData* apd, const unsigned char* src, LPDWORD nsrc,
			unsigned char* dst, LPDWORD ndst)
{
    double     	r;
    TRACE("(%p, %p, %p, %p, %p)\n", apd, src, nsrc, dst, ndst);

    while (*nsrc != 0 && *ndst != 0) {
	while ((r = (double)apd->srcPos - apd->dstPos) <= 0) {
	    if (*nsrc == 0) return;
	    apd->last[0].s = R16(src);	src += 2;
	    apd->srcPos++;
	    (*nsrc)--;
	}
	/* now do the interpolation */
        if (*nsrc)	/* don't go off end of data */
	    W16(dst, I(apd->last[0].s, R16(src), r));
        else
	    W16(dst, apd->last[0].s);
        dst += 2;
	apd->dstPos += apd->dstIncr;
	(*ndst)--;
    }
}

static	void (*PCM_ConvertChangeRate[16])(AcmPcmData* apd,
					  const unsigned char* src, LPDWORD nsrc,
					  unsigned char* dst, LPDWORD ndst) = {
    cvtSS88C,   cvtSM88C,   cvtMS88C,   cvtMM88C,
    cvtSS816C,	cvtSM816C,  cvtMS816C,  cvtMM816C,
    cvtSS168C,	cvtSM168C,  cvtMS168C,  cvtMM168C,
    cvtSS1616C, cvtSM1616C, cvtMS1616C, cvtMM1616C,
};

/***********************************************************************
 *           PCM_DriverDetails
 *
 */
static	LRESULT PCM_DriverDetails(PACMDRIVERDETAILSW add)
{
    TRACE("(%p)\n", add);

    add->fccType = ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC;
    add->fccComp = ACMDRIVERDETAILS_FCCCOMP_UNDEFINED;
    add->wMid = 0xFF;
    add->wPid = 0x00;
    add->vdwACM = 0x01000000;
    add->vdwDriver = 0x01000000;
    add->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CONVERTER;
    add->cFormatTags = 1;
    add->cFilterTags = 0;
    add->hicon = NULL;
    MultiByteToWideChar( CP_ACP, 0, "WINE-PCM", -1,
                         add->szShortName, sizeof(add->szShortName)/sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, "Wine PCM converter", -1,
                         add->szLongName, sizeof(add->szLongName)/sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, "Brought to you by the Wine team...", -1,
                         add->szCopyright, sizeof(add->szCopyright)/sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, "Refer to LICENSE file", -1,
                         add->szLicensing, sizeof(add->szLicensing)/sizeof(WCHAR) );
    add->szFeatures[0] = 0;

    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           PCM_FormatTagDetails
 *
 */
static	LRESULT	PCM_FormatTagDetails(PACMFORMATTAGDETAILSW aftd, DWORD dwQuery)
{
    TRACE("(%p, %08lx)\n", aftd, dwQuery);

    switch (dwQuery) {
    case ACM_FORMATTAGDETAILSF_INDEX:
	if (aftd->dwFormatTagIndex != 0) {
            WARN("not possible\n");
            return ACMERR_NOTPOSSIBLE;
        }
	break;
    case ACM_FORMATTAGDETAILSF_FORMATTAG:
	if (aftd->dwFormatTag != WAVE_FORMAT_PCM) {
            WARN("not possible\n");
            return ACMERR_NOTPOSSIBLE;
        }
	break;
    case ACM_FORMATTAGDETAILSF_LARGESTSIZE:
	if (aftd->dwFormatTag != WAVE_FORMAT_UNKNOWN &&
	    aftd->dwFormatTag != WAVE_FORMAT_PCM) {
            WARN("not possible\n");
	    return ACMERR_NOTPOSSIBLE;
        }
	break;
    default:
	WARN("Unsupported query %08lx\n", dwQuery);
	return MMSYSERR_NOTSUPPORTED;
    }

    aftd->dwFormatTagIndex = 0;
    aftd->dwFormatTag = WAVE_FORMAT_PCM;
    aftd->cbFormatSize = sizeof(PCMWAVEFORMAT);
    aftd->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CONVERTER;
    aftd->cStandardFormats = NUM_PCM_FORMATS;
    aftd->szFormatTag[0] = 0;

    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           PCM_FormatDetails
 *
 */
static	LRESULT	PCM_FormatDetails(PACMFORMATDETAILSW afd, DWORD dwQuery)
{
    TRACE("(%p, %08lx)\n", afd, dwQuery);

    switch (dwQuery) {
    case ACM_FORMATDETAILSF_FORMAT:
	if (PCM_GetFormatIndex(afd->pwfx) == 0xFFFFFFFF) {
            return ACMERR_NOTPOSSIBLE;
            WARN("not possible\n");
        }
	break;
    case ACM_FORMATDETAILSF_INDEX:
	assert(afd->dwFormatIndex < NUM_PCM_FORMATS);
	afd->pwfx->wFormatTag = WAVE_FORMAT_PCM;
	afd->pwfx->nChannels = PCM_Formats[afd->dwFormatIndex].nChannels;
	afd->pwfx->nSamplesPerSec = PCM_Formats[afd->dwFormatIndex].rate;
	afd->pwfx->wBitsPerSample = PCM_Formats[afd->dwFormatIndex].nBits;
	/* native MSACM uses a PCMWAVEFORMAT structure, so cbSize is not
	 * accessible afd->pwfx->cbSize = 0;
	 */
	afd->pwfx->nBlockAlign =
	    (afd->pwfx->nChannels * afd->pwfx->wBitsPerSample) / 8;
	afd->pwfx->nAvgBytesPerSec =
	    afd->pwfx->nSamplesPerSec * afd->pwfx->nBlockAlign;
	break;
    default:
	WARN("Unsupported query %08lx\n", dwQuery);
	return MMSYSERR_NOTSUPPORTED;
    }

    afd->dwFormatTag = WAVE_FORMAT_PCM;
    afd->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CONVERTER;
    afd->szFormat[0] = 0; /* let MSACM format this for us... */
    afd->cbwfx = sizeof(PCMWAVEFORMAT);

    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           PCM_FormatSuggest
 *
 */
static	LRESULT	PCM_FormatSuggest(PACMDRVFORMATSUGGEST adfs)
{
    TRACE("(%p)\n", adfs);

    /* some tests ... */
    if (adfs->cbwfxSrc < sizeof(PCMWAVEFORMAT) ||
	adfs->cbwfxDst < sizeof(PCMWAVEFORMAT) ||
	PCM_GetFormatIndex(adfs->pwfxSrc) == 0xFFFFFFFF) {
            WARN("not possible\n");
            return ACMERR_NOTPOSSIBLE;
       }

    /* is no suggestion for destination, then copy source value */
    if (!(adfs->fdwSuggest & ACM_FORMATSUGGESTF_NCHANNELS)) {
	adfs->pwfxDst->nChannels = adfs->pwfxSrc->nChannels;
    }
    if (!(adfs->fdwSuggest & ACM_FORMATSUGGESTF_NSAMPLESPERSEC)) {
	adfs->pwfxDst->nSamplesPerSec = adfs->pwfxSrc->nSamplesPerSec;
    }
    if (!(adfs->fdwSuggest & ACM_FORMATSUGGESTF_WBITSPERSAMPLE)) {
	adfs->pwfxDst->wBitsPerSample = adfs->pwfxSrc->wBitsPerSample;
    }
    if (!(adfs->fdwSuggest & ACM_FORMATSUGGESTF_WFORMATTAG)) {
	if (adfs->pwfxSrc->wFormatTag != WAVE_FORMAT_PCM) {
            WARN("not possible\n");
            return ACMERR_NOTPOSSIBLE;
        }
	adfs->pwfxDst->wFormatTag = adfs->pwfxSrc->wFormatTag;
    }
    /* check if result is ok */
    if (PCM_GetFormatIndex(adfs->pwfxDst) == 0xFFFFFFFF) {
        WARN("not possible\n");
        return ACMERR_NOTPOSSIBLE;
    }

    /* recompute other values */
    adfs->pwfxDst->nBlockAlign = (adfs->pwfxDst->nChannels * adfs->pwfxDst->wBitsPerSample) / 8;
    adfs->pwfxDst->nAvgBytesPerSec = adfs->pwfxDst->nSamplesPerSec * adfs->pwfxDst->nBlockAlign;

    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           PCM_Reset
 *
 */
static	void	PCM_Reset(AcmPcmData* apd, int srcNumBits)
{
    TRACE("(%p, %d)\n", apd, srcNumBits);

    apd->srcPos = 0;
    apd->dstPos = 0;
    /* initialize with neutral value */
    if (srcNumBits == 16) {
	apd->last[0].s = 0;
	apd->last[1].s = 0;
    } else {
	apd->last[0].b = (BYTE)0x80;
	apd->last[1].b = (BYTE)0x80;
    }
}

/***********************************************************************
 *           PCM_StreamOpen
 *
 */
static	LRESULT	PCM_StreamOpen(PACMDRVSTREAMINSTANCE adsi)
{
    AcmPcmData*	apd;
    int		idx = 0;

    TRACE("(%p)\n", adsi);

    assert(!(adsi->fdwOpen & ACM_STREAMOPENF_ASYNC));

    if (PCM_GetFormatIndex(adsi->pwfxSrc) == 0xFFFFFFFF ||
	PCM_GetFormatIndex(adsi->pwfxDst) == 0xFFFFFFFF) {
        WARN("not possible\n");
	return ACMERR_NOTPOSSIBLE;
    }

    apd = HeapAlloc(GetProcessHeap(), 0, sizeof(AcmPcmData));
    if (apd == 0) {
        WARN("no memory\n");
        return MMSYSERR_NOMEM;
    }

    adsi->dwDriver = (DWORD)apd;
    adsi->fdwDriver = 0;

    if (adsi->pwfxSrc->wBitsPerSample == 16) idx += 8;
    if (adsi->pwfxDst->wBitsPerSample == 16) idx += 4;
    if (adsi->pwfxSrc->nChannels      == 1)  idx += 2;
    if (adsi->pwfxDst->nChannels      == 1)  idx += 1;

    if (adsi->pwfxSrc->nSamplesPerSec == adsi->pwfxDst->nSamplesPerSec) {
	apd->cvt.cvtKeepRate = PCM_ConvertKeepRate[idx];
    } else {
	adsi->fdwDriver |= PCM_RESAMPLE;
	apd->dstIncr = (double)(adsi->pwfxSrc->nSamplesPerSec) /
	    (double)(adsi->pwfxDst->nSamplesPerSec);
	PCM_Reset(apd, adsi->pwfxSrc->wBitsPerSample);
	apd->cvt.cvtChangeRate = PCM_ConvertChangeRate[idx];
    }

    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           PCM_StreamClose
 *
 */
static	LRESULT	PCM_StreamClose(PACMDRVSTREAMINSTANCE adsi)
{
    TRACE("(%p)\n", adsi);

    HeapFree(GetProcessHeap(), 0, (void*)adsi->dwDriver);
    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           PCM_round
 *
 */
static	inline DWORD	PCM_round(DWORD a, DWORD b, DWORD c)
{
    assert(c);
    /* to be sure, always return an entire number of c... */
    return ((double)a * (double)b + (double)c - 1) / (double)c;
}

/***********************************************************************
 *           PCM_StreamSize
 *
 */
static	LRESULT PCM_StreamSize(PACMDRVSTREAMINSTANCE adsi, PACMDRVSTREAMSIZE adss)
{
    DWORD	srcMask = ~(adsi->pwfxSrc->nBlockAlign - 1);
    DWORD	dstMask = ~(adsi->pwfxDst->nBlockAlign - 1);

    TRACE("(%p, %p)\n", adsi, adss);

    switch (adss->fdwSize) {
    case ACM_STREAMSIZEF_DESTINATION:
	/* cbDstLength => cbSrcLength */
	adss->cbSrcLength = PCM_round(adss->cbDstLength & dstMask,
				      adsi->pwfxSrc->nAvgBytesPerSec,
				      adsi->pwfxDst->nAvgBytesPerSec) & srcMask;
	break;
    case ACM_STREAMSIZEF_SOURCE:
	/* cbSrcLength => cbDstLength */
	adss->cbDstLength =  PCM_round(adss->cbSrcLength & srcMask,
				       adsi->pwfxDst->nAvgBytesPerSec,
				       adsi->pwfxSrc->nAvgBytesPerSec) & dstMask;
	break;
    default:
	WARN("Unsupported query %08lx\n", adss->fdwSize);
	return MMSYSERR_NOTSUPPORTED;
    }
    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           PCM_StreamConvert
 *
 */
static LRESULT PCM_StreamConvert(PACMDRVSTREAMINSTANCE adsi, PACMDRVSTREAMHEADER adsh)
{
    AcmPcmData*	apd = (AcmPcmData*)adsi->dwDriver;
    DWORD	nsrc = NUM_OF(adsh->cbSrcLength, adsi->pwfxSrc->nBlockAlign);
    DWORD	ndst = NUM_OF(adsh->cbDstLength, adsi->pwfxDst->nBlockAlign);

    TRACE("(%p, %p)\n", adsi, adsh);

    TRACE("nsrc=%ld,adsh->cbSrcLength=%ld\n", nsrc, adsh->cbSrcLength);
    TRACE("ndst=%ld,adsh->cbDstLength=%ld\n", ndst, adsh->cbDstLength);
    TRACE("src [wFormatTag=%u, nChannels=%u, nSamplesPerSec=%lu, nAvgBytesPerSec=%lu, nBlockAlign=%u, wBitsPerSample=%u, cbSize=%u]\n",
          adsi->pwfxSrc->wFormatTag, adsi->pwfxSrc->nChannels, adsi->pwfxSrc->nSamplesPerSec, adsi->pwfxSrc->nAvgBytesPerSec,
          adsi->pwfxSrc->nBlockAlign, adsi->pwfxSrc->wBitsPerSample, adsi->pwfxSrc->cbSize);
    TRACE("dst [wFormatTag=%u, nChannels=%u, nSamplesPerSec=%lu, nAvgBytesPerSec=%lu, nBlockAlign=%u, wBitsPerSample=%u, cbSize=%u]\n",
          adsi->pwfxDst->wFormatTag, adsi->pwfxDst->nChannels, adsi->pwfxDst->nSamplesPerSec, adsi->pwfxDst->nAvgBytesPerSec,
          adsi->pwfxDst->nBlockAlign, adsi->pwfxDst->wBitsPerSample, adsi->pwfxDst->cbSize);

    if (adsh->fdwConvert &
	~(ACM_STREAMCONVERTF_BLOCKALIGN|
	  ACM_STREAMCONVERTF_END|
	  ACM_STREAMCONVERTF_START)) {
	FIXME("Unsupported fdwConvert (%08lx), ignoring it\n", adsh->fdwConvert);
    }
    /* ACM_STREAMCONVERTF_BLOCKALIGN
     *	currently all conversions are block aligned, so do nothing for this flag
     * ACM_STREAMCONVERTF_END
     *	no pending data, so do nothing for this flag
     */
    if ((adsh->fdwConvert & ACM_STREAMCONVERTF_START) &&
	(adsi->fdwDriver & PCM_RESAMPLE)) {
	PCM_Reset(apd, adsi->pwfxSrc->wBitsPerSample);
    }

    /* do the job */
    if (adsi->fdwDriver & PCM_RESAMPLE) {
	DWORD	nsrc2 = nsrc;
	DWORD	ndst2 = ndst;

	apd->cvt.cvtChangeRate(apd, adsh->pbSrc, &nsrc2, adsh->pbDst, &ndst2);
	nsrc -= nsrc2;
	ndst -= ndst2;
    } else {
	if (nsrc < ndst) ndst = nsrc; else nsrc = ndst;

	/* nsrc is now equal to ndst */
	apd->cvt.cvtKeepRate(adsh->pbSrc, nsrc, adsh->pbDst);
    }

    adsh->cbSrcLengthUsed = nsrc * adsi->pwfxSrc->nBlockAlign;
    adsh->cbDstLengthUsed = ndst * adsi->pwfxDst->nBlockAlign;

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			DriverProc (MSACM32.@)
 */
LRESULT CALLBACK	PCM_DriverProc(DWORD dwDevID, HDRVR hDriv, UINT wMsg,
				       LPARAM dwParam1, LPARAM dwParam2)
{
    TRACE("(%08lx %08lx %u %08lx %08lx);\n",
	  dwDevID, (DWORD)hDriv, wMsg, dwParam1, dwParam2);

    switch (wMsg) {
    case DRV_LOAD:		return 1;
    case DRV_FREE:		return 1;
    case DRV_OPEN:		return PCM_drvOpen((LPSTR)dwParam1, (PACMDRVOPENDESCW)dwParam2);
    case DRV_CLOSE:		return PCM_drvClose(dwDevID);
    case DRV_ENABLE:		return 1;
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBoxA(0, "MSACM PCM filter !", "Wine Driver", MB_OK); return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;

    case ACMDM_DRIVER_NOTIFY:
	/* no caching from other ACM drivers is done so far */
	return MMSYSERR_NOERROR;

    case ACMDM_DRIVER_DETAILS:
	return PCM_DriverDetails((PACMDRIVERDETAILSW)dwParam1);

    case ACMDM_FORMATTAG_DETAILS:
	return PCM_FormatTagDetails((PACMFORMATTAGDETAILSW)dwParam1, dwParam2);

    case ACMDM_FORMAT_DETAILS:
	return PCM_FormatDetails((PACMFORMATDETAILSW)dwParam1, dwParam2);

    case ACMDM_FORMAT_SUGGEST:
	return PCM_FormatSuggest((PACMDRVFORMATSUGGEST)dwParam1);

    case ACMDM_STREAM_OPEN:
	return PCM_StreamOpen((PACMDRVSTREAMINSTANCE)dwParam1);

    case ACMDM_STREAM_CLOSE:
	return PCM_StreamClose((PACMDRVSTREAMINSTANCE)dwParam1);

    case ACMDM_STREAM_SIZE:
	return PCM_StreamSize((PACMDRVSTREAMINSTANCE)dwParam1, (PACMDRVSTREAMSIZE)dwParam2);

    case ACMDM_STREAM_CONVERT:
	return PCM_StreamConvert((PACMDRVSTREAMINSTANCE)dwParam1, (PACMDRVSTREAMHEADER)dwParam2);

    case ACMDM_HARDWARE_WAVE_CAPS_INPUT:
    case ACMDM_HARDWARE_WAVE_CAPS_OUTPUT:
	/* this converter is not a hardware driver */
    case ACMDM_FILTERTAG_DETAILS:
    case ACMDM_FILTER_DETAILS:
	/* this converter is not a filter */
    case ACMDM_STREAM_RESET:
	/* only needed for asynchronous driver... we aren't, so just say it */
    case ACMDM_STREAM_PREPARE:
    case ACMDM_STREAM_UNPREPARE:
	/* nothing special to do here... so don't do anything */
	return MMSYSERR_NOTSUPPORTED;

    default:
	return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
    return 0;
}
