/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 *      MSACM32 library
 *
 *      Copyright 2000		Eric Pouech
 *      Copyright 2004		Robert Reif
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 *	FIXME / TODO list
 *	+ get rid of hack for PCM_DriverProc (msacm32.dll shouldn't export
 *	  a DriverProc, but this would require implementing a generic
 *	  embedded driver handling scheme in msacm32.dll which isn't done yet
 */

#include "wineacm.h"

#include <assert.h>

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
    TRACE("(%d)\n", dwDevID);

    return 1;
}

#define	NUM_PCM_FORMATS	(sizeof(PCM_Formats) / sizeof(PCM_Formats[0]))
#define	NUM_OF(a,b)	((a)/(b))

/* flags for fdwDriver */
#define PCM_RESAMPLE	1

/* data used while converting */
typedef struct tagAcmPcmData {
    /* conversion routine, depending if rate conversion is required */
    union {
	void (*cvtKeepRate)(const unsigned char*, int, unsigned char*);
	void (*cvtChangeRate)(DWORD, const unsigned char*, LPDWORD,
			      DWORD, unsigned char*, LPDWORD);
    } cvt;
} AcmPcmData;

/* table to list all supported formats... those are the basic ones. this
 * also helps given a unique index to each of the supported formats
 */
static const struct {
    int		nChannels;
    int		nBits;
    int		rate;
} PCM_Formats[] = {
    {1,  8,  8000}, {2,  8,  8000}, {1, 16,  8000}, {2, 16,  8000}, {1, 24,  8000}, {2, 24,  8000},
    {1,  8, 11025}, {2,  8, 11025}, {1, 16, 11025}, {2, 16, 11025}, {1, 24, 11025}, {2, 24, 11025},
    {1,  8, 22050}, {2,  8, 22050}, {1, 16, 22050}, {2, 16, 22050}, {1, 24, 22050}, {2, 24, 22050},
    {1,  8, 44100}, {2,  8, 44100}, {1, 16, 44100}, {2, 16, 44100}, {1, 24, 44100}, {2, 24, 44100},
    {1,  8, 48000}, {2,  8, 48000}, {1, 16, 48000}, {2, 16, 48000}, {1, 24, 48000}, {2, 24, 48000},
    {1,  8, 96000}, {2,  8, 96000}, {1, 16, 96000}, {2, 16, 96000}, {1, 24, 96000}, {2, 24, 96000},
};

/***********************************************************************
 *           PCM_GetFormatIndex
 */
static DWORD PCM_GetFormatIndex(LPWAVEFORMATEX wfx)
{
    unsigned int i;
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
 * stereo => mono: use the sum of Left & Right channels
 */

/***********************************************************************
 *           C816
 *
 * Converts a 8 bit sample to a 16 bit one
 */
static inline short C816(unsigned char b)
{
    return (b - 128) << 8;
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
 *           C248
 *
 * Converts a 24 bit sample to a 8 bit one (data loss !!)
 */
static inline unsigned char C248(int s)
{
    return HIBYTE(HIWORD(s)) ^ (unsigned char)0x80;
}

/***********************************************************************
 *           C2416
 *
 * Converts a 24 bit sample to a 16 bit one (data loss !!)
 */
static inline short C2416(int s)
{
    return HIWORD(s);
}

/***********************************************************************
 *           R16
 *
 * Read a 16 bit sample (correctly handles endianness)
 */
static inline short  R16(const unsigned char* src)
{
    return (short)((unsigned short)src[0] | ((unsigned short)src[1] << 8));
}

/***********************************************************************
 *           R24
 *
 * Read a 24 bit sample (correctly handles endianness)
 * Note, to support signed arithmetic, the values are shifted high in the int
 * and low 8 bytes are unused.
 */
static inline int R24(const unsigned char* src)
{
    return ((int)src[0] | (int)src[1] << 8 | (int)src[2] << 16) << 8;
}

/***********************************************************************
 *           W16
 *
 * Write a 16 bit sample (correctly handles endianness)
 */
static inline void  W16(unsigned char* dst, short s)
{
    dst[0] = LOBYTE(s);
    dst[1] = HIBYTE(s);
}

/***********************************************************************
 *           W24
 *
 * Write a 24 bit sample (correctly handles endianness)
 */
static inline void  W24(unsigned char* dst, int s)
{
    dst[0] = HIBYTE(LOWORD(s));
    dst[1] = LOBYTE(HIWORD(s));
    dst[2] = HIBYTE(HIWORD(s));
}

/***********************************************************************
 *           M24
 *
 * Convert the (l,r) 24 bit stereo sample into a 24 bit mono
 * (takes the sum of the two values)
 */
static inline int M24(int l, int r)
{
    LONGLONG sum = l + r;

    /* clip sum to saturation */
    if (sum > 0x7fffff00)
        sum = 0x7fffff00;
    else if (sum < -0x7fffff00)
        sum = -0x7fffff00;

    return sum;
}

/***********************************************************************
 *           M16
 *
 * Convert the (l,r) 16 bit stereo sample into a 16 bit mono
 * (takes the sum of the two values)
 */
static inline short M16(short l, short r)
{
    int	sum = l + r;

    /* clip sum to saturation */
    if (sum > 32767)
        sum = 32767;
    else if (sum < -32768)
        sum = -32768;

    return sum;
}

/***********************************************************************
 *           M8
 *
 * Convert the (l,r) 8 bit stereo sample into a 8 bit mono
 * (takes the sum of the two values)
 */
static inline unsigned char M8(unsigned char a, unsigned char b)
{
    int l = a - 128;
    int r = b - 128;
    int	sum = (l + r) + 128;

    /* clip sum to saturation */
    if (sum > 0xff)
        sum = 0xff;
    else if (sum < 0)
        sum = 0;

    return sum;
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
	v = C168(R16(src));	src += 2;
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

static	void cvtMS248K(const unsigned char* src, int ns, unsigned char* dst)
{
    unsigned char v;
    TRACE("(%p, %d, %p)\n", src, ns, dst);

    while (ns--) {
	v = C248(R24(src));	src += 3;
	*dst++ = v;
	*dst++ = v;
    }
}

static	void cvtSM248K(const unsigned char* src, int ns, unsigned char* dst)
{
    TRACE("(%p, %d, %p)\n", src, ns, dst);

    while (ns--) {
	*dst++ = C248(M24(R24(src), R24(src + 3)));
	src += 6;
    }
}

static	void cvtMM248K(const unsigned char* src, int ns, unsigned char* dst)
{
    TRACE("(%p, %d, %p)\n", src, ns, dst);

    while (ns--) {
	*dst++ = C248(R24(src));	src += 3;
    }
}

static	void cvtSS248K(const unsigned char* src, int ns, unsigned char* dst)
{
    TRACE("(%p, %d, %p)\n", src, ns, dst);

    while (ns--) {
	*dst++ = C248(R24(src));	src += 3;
	*dst++ = C248(R24(src));	src += 3;
    }
}

static	void cvtMS2416K(const unsigned char* src, int ns, unsigned char* dst)
{
    short v;
    TRACE("(%p, %d, %p)\n", src, ns, dst);

    while (ns--) {
	v = C2416(R24(src));	src += 3;
	W16(dst, v);	dst += 2;
	W16(dst, v);	dst += 2;
    }
}

static	void cvtSM2416K(const unsigned char* src, int ns, unsigned char* dst)
{
    TRACE("(%p, %d, %p)\n", src, ns, dst);

    while (ns--) {
	W16(dst, C2416(M24(R24(src), R24(src + 3))));
	dst += 2;
	src += 6;
    }
}

static	void cvtMM2416K(const unsigned char* src, int ns, unsigned char* dst)
{
    TRACE("(%p, %d, %p)\n", src, ns, dst);

    while (ns--) {
	W16(dst, C2416(R24(src)));	dst += 2; src += 3;
    }
}

static	void cvtSS2416K(const unsigned char* src, int ns, unsigned char* dst)
{
    TRACE("(%p, %d, %p)\n", src, ns, dst);

    while (ns--) {
	W16(dst, C2416(R24(src)));	dst += 2; src += 3;
	W16(dst, C2416(R24(src)));	dst += 2; src += 3;
    }
}


typedef void (*PCM_CONVERT_KEEP_RATE)(const unsigned char*, int, unsigned char*);

static const PCM_CONVERT_KEEP_RATE PCM_ConvertKeepRate[] = {
    cvtSS88K,	cvtSM88K,   cvtMS88K,   cvtMM88K,
    cvtSS816K,	cvtSM816K,  cvtMS816K,  cvtMM816K,
    NULL, NULL, NULL, NULL, /* TODO: 8->24 */
    cvtSS168K,	cvtSM168K,  cvtMS168K,  cvtMM168K,
    cvtSS1616K, cvtSM1616K, cvtMS1616K, cvtMM1616K,
    NULL, NULL, NULL, NULL, /* TODO: 16->24 */
    cvtSS248K, cvtSM248K, cvtMS248K, cvtMM248K,
    cvtSS2416K, cvtSM2416K, cvtMS2416K, cvtMM2416K,
    NULL, NULL, NULL, NULL, /* TODO: 24->24 */
};

/* the conversion routines with rate conversion are labelled cvt<X><Y><N><M>C
 * where :
 * <X> is the (M)ono/(S)tereo configuration of  input channel
 * <Y> is the (M)ono/(S)tereo configuration of output channel
 * <N> is the number of bits of  input channel (8 or 16)
 * <M> is the number of bits of output channel (8 or 16)
 *
 */
static	void cvtSS88C(DWORD srcRate, const unsigned char* src, LPDWORD nsrc,
		      DWORD dstRate, unsigned char* dst, LPDWORD ndst)
{
    DWORD error = dstRate / 2;
    TRACE("(%d, %p, %p, %d, %p, %p)\n", srcRate, src, nsrc, dstRate, dst, ndst);

    while ((*ndst)--) {
        *dst++ = *src;
        *dst++ = *src;
        error = error + srcRate;
        while (error > dstRate) {
            src += 2;
            (*nsrc)--;
            if (*nsrc == 0)
                return;
            error = error - dstRate;
        }
    }
}

static	void cvtSM88C(DWORD srcRate, const unsigned char* src, LPDWORD nsrc,
		      DWORD dstRate, unsigned char* dst, LPDWORD ndst)
{
    DWORD error = dstRate / 2;
    TRACE("(%d, %p, %p, %d, %p, %p)\n", srcRate, src, nsrc, dstRate, dst, ndst);

    while ((*ndst)--) {
        *dst++ = M8(src[0], src[1]);
        error = error + srcRate;
        while (error > dstRate) {
            src += 2;
            (*nsrc)--;
            if (*nsrc == 0)
                return;
            error = error - dstRate;
        }
    }
}

static	void cvtMS88C(DWORD srcRate, const unsigned char* src, LPDWORD nsrc,
		      DWORD dstRate, unsigned char* dst, LPDWORD ndst)
{
    DWORD error = dstRate / 2;
    TRACE("(%d, %p, %p, %d, %p, %p)\n", srcRate, src, nsrc, dstRate, dst, ndst);

    while ((*ndst)--) {
        *dst++ = *src;
        *dst++ = *src;
        error = error + srcRate;
        while (error > dstRate) {
            src++;
            (*nsrc)--;
            if (*nsrc == 0)
                return;
            error = error - dstRate;
        }
    }
}

static	void cvtMM88C(DWORD srcRate, const unsigned char* src, LPDWORD nsrc,
		      DWORD dstRate, unsigned char* dst, LPDWORD ndst)
{
    DWORD error = dstRate / 2;
    TRACE("(%d, %p, %p, %d, %p, %p)\n", srcRate, src, nsrc, dstRate, dst, ndst);

    while ((*ndst)--) {
        *dst++ = *src;
        error = error + srcRate;
        while (error > dstRate) {
            src++;
            (*nsrc)--;
            if (*nsrc==0)
                return;
            error = error - dstRate;
        }
    }
}

static	void cvtSS816C(DWORD srcRate, const unsigned char* src, LPDWORD nsrc,
		       DWORD dstRate, unsigned char* dst, LPDWORD ndst)
{
    DWORD error = dstRate / 2;
    TRACE("(%d, %p, %p, %d, %p, %p)\n", srcRate, src, nsrc, dstRate, dst, ndst);

    while ((*ndst)--) {
	W16(dst, C816(src[0]));	dst += 2;
	W16(dst, C816(src[1]));	dst += 2;
        error = error + srcRate;
        while (error > dstRate) {
            src += 2;
            (*nsrc)--;
            if (*nsrc==0)
                return;
            error = error - dstRate;
        }
    }
}

static	void cvtSM816C(DWORD srcRate, const unsigned char* src, LPDWORD nsrc,
		       DWORD dstRate, unsigned char* dst, LPDWORD ndst)
{
    DWORD error = dstRate / 2;
    TRACE("(%d, %p, %p, %d, %p, %p)\n", srcRate, src, nsrc, dstRate, dst, ndst);

    while ((*ndst)--) {
        W16(dst, M16(C816(src[0]), C816(src[1]))); dst += 2;
        error = error + srcRate;
        while (error > dstRate) {
            src += 2;
            (*nsrc)--;
            if (*nsrc==0)
                return;
            error = error - dstRate;
        }
    }
}

static	void cvtMS816C(DWORD srcRate, const unsigned char* src, LPDWORD nsrc,
		       DWORD dstRate, unsigned char* dst, LPDWORD ndst)
{
    DWORD error = dstRate / 2;
    TRACE("(%d, %p, %p, %d, %p, %p)\n", srcRate, src, nsrc, dstRate, dst, ndst);

    while ((*ndst)--) {
        W16(dst, C816(*src)); dst += 2;
        W16(dst, C816(*src)); dst += 2;
        error = error + srcRate;
        while (error > dstRate) {
            src++;
            (*nsrc)--;
            if (*nsrc==0)
                return;
            error = error - dstRate;
        }
    }
}

static	void cvtMM816C(DWORD srcRate, const unsigned char* src, LPDWORD nsrc,
		       DWORD dstRate, unsigned char* dst, LPDWORD ndst)
{
    DWORD error = dstRate / 2;
    TRACE("(%d, %p, %p, %d, %p, %p)\n", srcRate, src, nsrc, dstRate, dst, ndst);

    while ((*ndst)--) {
        W16(dst, C816(*src)); dst += 2;
        error = error + srcRate;
        while (error > dstRate) {
            src++;
            (*nsrc)--;
            if (*nsrc==0)
                return;
            error = error - dstRate;
        }
    }
}

static	void cvtSS168C(DWORD srcRate, const unsigned char* src, LPDWORD nsrc,
		       DWORD dstRate, unsigned char* dst, LPDWORD ndst)
{
    DWORD error = dstRate / 2;
    TRACE("(%d, %p, %p, %d, %p, %p)\n", srcRate, src, nsrc, dstRate, dst, ndst);

    while ((*ndst)--) {
        *dst++ = C168(R16(src));
        *dst++ = C168(R16(src + 2));
        error = error + srcRate;
        while (error > dstRate) {
            src += 4;
            (*nsrc)--;
            if (*nsrc==0)
                return;
            error = error - dstRate;
        }
    }
}

static	void cvtSM168C(DWORD srcRate, const unsigned char* src, LPDWORD nsrc,
		       DWORD dstRate, unsigned char* dst, LPDWORD ndst)
{
    DWORD error = dstRate / 2;
    TRACE("(%d, %p, %p, %d, %p, %p)\n", srcRate, src, nsrc, dstRate, dst, ndst);

    while ((*ndst)--) {
	*dst++ = C168(M16(R16(src), R16(src + 2)));
        error = error + srcRate;
        while (error > dstRate) {
            src += 4;
            (*nsrc)--;
            if (*nsrc==0)
                return;
            error = error - dstRate;
        }
    }
}

static	void cvtMS168C(DWORD srcRate, const unsigned char* src, LPDWORD nsrc,
		       DWORD dstRate, unsigned char* dst, LPDWORD ndst)
{
    DWORD error = dstRate / 2;
    TRACE("(%d, %p, %p, %d, %p, %p)\n", srcRate, src, nsrc, dstRate, dst, ndst);

    while ((*ndst)--) {
        *dst++ = C168(R16(src));
        *dst++ = C168(R16(src));
        error = error + srcRate;
        while (error > dstRate) {
            src += 2;
            (*nsrc)--;
            if (*nsrc==0)
                return;
            error = error - dstRate;
        }
    }
}

static	void cvtMM168C(DWORD srcRate, const unsigned char* src, LPDWORD nsrc,
		       DWORD dstRate, unsigned char* dst, LPDWORD ndst)
{
    DWORD error = dstRate / 2;
    TRACE("(%d, %p, %p, %d, %p, %p)\n", srcRate, src, nsrc, dstRate, dst, ndst);

    while ((*ndst)--) {
        *dst++ = C168(R16(src));
        error = error + srcRate;
        while (error > dstRate) {
            src += 2;
            (*nsrc)--;
            if (*nsrc == 0)
                return;
            error = error - dstRate;
        }
    }
}

static	void cvtSS1616C(DWORD srcRate, const unsigned char* src, LPDWORD nsrc,
			DWORD dstRate, unsigned char* dst, LPDWORD ndst)
{
    DWORD error = dstRate / 2;
    TRACE("(%d, %p, %p, %d, %p, %p)\n", srcRate, src, nsrc, dstRate, dst, ndst);

    while ((*ndst)--) {
        W16(dst, R16(src)); dst += 2;
        W16(dst, R16(src)); dst += 2;
        error = error + srcRate;
        while (error > dstRate) {
            src += 4;
            (*nsrc)--;
            if (*nsrc == 0)
                return;
            error = error - dstRate;
        }
    }
}

static	void cvtSM1616C(DWORD srcRate, const unsigned char* src, LPDWORD nsrc,
			DWORD dstRate, unsigned char* dst, LPDWORD ndst)
{
    DWORD error = dstRate / 2;
    TRACE("(%d, %p, %p, %d, %p, %p)\n", srcRate, src, nsrc, dstRate, dst, ndst);

    while ((*ndst)--) {
        W16(dst, M16(R16(src), R16(src + 2))); dst += 2;
        error = error + srcRate;
        while (error > dstRate) {
            src += 4;
            (*nsrc)--;
            if (*nsrc == 0)
                return;
            error = error - dstRate;
        }
    }
}

static	void cvtMS1616C(DWORD srcRate, const unsigned char* src, LPDWORD nsrc,
			DWORD dstRate, unsigned char* dst, LPDWORD ndst)
{
    DWORD error = dstRate / 2;
    TRACE("(%d, %p, %p, %d, %p, %p)\n", srcRate, src, nsrc, dstRate, dst, ndst);

    while((*ndst)--) {
        W16(dst, R16(src)); dst += 2;
        W16(dst, R16(src)); dst += 2;
        error = error + srcRate;
        while (error > dstRate) {
            src += 2;
            (*nsrc)--;
            if (*nsrc == 0)
                return;
            error = error - dstRate;
        }
    }
}

static	void cvtMM1616C(DWORD srcRate, const unsigned char* src, LPDWORD nsrc,
			DWORD dstRate, unsigned char* dst, LPDWORD ndst)
{
    DWORD error = dstRate / 2;
    TRACE("(%d, %p, %p, %d, %p, %p)\n", srcRate, src, nsrc, dstRate, dst, ndst);

    while ((*ndst)--) {
        W16(dst, R16(src)); dst += 2;
        error = error + srcRate;
        while (error > dstRate) {
            src += 2;
            (*nsrc)--;
            if (*nsrc == 0)
                return;
            error = error - dstRate;
        }
    }
}

static	void cvtSS2424C(DWORD srcRate, const unsigned char* src, LPDWORD nsrc,
			DWORD dstRate, unsigned char* dst, LPDWORD ndst)
{
    DWORD error = dstRate / 2;
    TRACE("(%d, %p, %p, %d, %p, %p)\n", srcRate, src, nsrc, dstRate, dst, ndst);

    while ((*ndst)--) {
        W24(dst, R24(src)); dst += 3;
        W24(dst, R24(src)); dst += 3;
        error = error + srcRate;
        while (error > dstRate) {
            src += 6;
            (*nsrc)--;
            if (*nsrc == 0)
                return;
            error = error - dstRate;
        }
    }
}

static	void cvtSM2424C(DWORD srcRate, const unsigned char* src, LPDWORD nsrc,
			DWORD dstRate, unsigned char* dst, LPDWORD ndst)
{
    DWORD error = dstRate / 2;
    TRACE("(%d, %p, %p, %d, %p, %p)\n", srcRate, src, nsrc, dstRate, dst, ndst);

    while ((*ndst)--) {
        W24(dst, M24(R24(src), R24(src + 3))); dst += 3;
        error = error + srcRate;
        while (error > dstRate) {
            src += 6;
            (*nsrc)--;
            if (*nsrc == 0)
                return;
            error = error - dstRate;
        }
    }
}

static	void cvtMS2424C(DWORD srcRate, const unsigned char* src, LPDWORD nsrc,
			DWORD dstRate, unsigned char* dst, LPDWORD ndst)
{
    DWORD error = dstRate / 2;
    TRACE("(%d, %p, %p, %d, %p, %p)\n", srcRate, src, nsrc, dstRate, dst, ndst);

    while((*ndst)--) {
        W24(dst, R24(src)); dst += 3;
        W24(dst, R24(src)); dst += 3;
        error = error + srcRate;
        while (error > dstRate) {
            src += 3;
            (*nsrc)--;
            if (*nsrc == 0)
                return;
            error = error - dstRate;
        }
    }
}

static	void cvtMM2424C(DWORD srcRate, const unsigned char* src, LPDWORD nsrc,
			DWORD dstRate, unsigned char* dst, LPDWORD ndst)
{
    DWORD error = dstRate / 2;
    TRACE("(%d, %p, %p, %d, %p, %p)\n", srcRate, src, nsrc, dstRate, dst, ndst);

    while ((*ndst)--) {
        W24(dst, R24(src)); dst += 3;
        error = error + srcRate;
        while (error > dstRate) {
            src += 3;
            (*nsrc)--;
            if (*nsrc == 0)
                return;
            error = error - dstRate;
        }
    }
}

typedef void (*PCM_CONVERT_CHANGE_RATE)(DWORD, const unsigned char*, LPDWORD, DWORD, unsigned char*, LPDWORD);

static const PCM_CONVERT_CHANGE_RATE PCM_ConvertChangeRate[] = {
    cvtSS88C,   cvtSM88C,   cvtMS88C,   cvtMM88C,
    cvtSS816C,	cvtSM816C,  cvtMS816C,  cvtMM816C,
    NULL, NULL, NULL, NULL, /* TODO: 8->24 */
    cvtSS168C,	cvtSM168C,  cvtMS168C,  cvtMM168C,
    cvtSS1616C, cvtSM1616C, cvtMS1616C, cvtMM1616C,
    NULL, NULL, NULL, NULL, /* TODO: 16->24 */
    NULL, NULL, NULL, NULL, /* TODO: 24->8 */
    NULL, NULL, NULL, NULL, /* TODO: 24->16 */
    cvtSS2424C, cvtSM2424C, cvtMS2424C, cvtMM2424C,
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
    add->wMid = MM_MICROSOFT;
    add->wPid = MM_MSFT_ACM_PCM;
    add->vdwACM = 0x01000000;
    add->vdwDriver = 0x01000000;
    add->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CONVERTER;
    add->cFormatTags = 1;
    add->cFilterTags = 0;
    add->hicon = NULL;
    MultiByteToWideChar( CP_ACP, 0, "MS-PCM", -1,
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
    TRACE("(%p, %08x)\n", aftd, dwQuery);

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
	WARN("Unsupported query %08x\n", dwQuery);
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
    TRACE("(%p, %08x)\n", afd, dwQuery);

    switch (dwQuery) {
    case ACM_FORMATDETAILSF_FORMAT:
	if (PCM_GetFormatIndex(afd->pwfx) == 0xFFFFFFFF) {
            WARN("not possible\n");
            return ACMERR_NOTPOSSIBLE;
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
	WARN("Unsupported query %08x\n", dwQuery);
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
            WARN("source format 0x%x not supported\n", adfs->pwfxSrc->wFormatTag);
            return ACMERR_NOTPOSSIBLE;
        }
        adfs->pwfxDst->wFormatTag = adfs->pwfxSrc->wFormatTag;
    } else {
        if (adfs->pwfxDst->wFormatTag != WAVE_FORMAT_PCM) {
            WARN("destination format 0x%x not supported\n", adfs->pwfxDst->wFormatTag);
            return ACMERR_NOTPOSSIBLE;
        }
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
 *           PCM_StreamOpen
 *
 */
static	LRESULT	PCM_StreamOpen(PACMDRVSTREAMINSTANCE adsi)
{
    AcmPcmData* apd;
    int idx;
    DWORD flags;

    TRACE("(%p)\n", adsi);

    assert(!(adsi->fdwOpen & ACM_STREAMOPENF_ASYNC));

    switch(adsi->pwfxSrc->wBitsPerSample){
    case 8:
        idx = 0;
        break;
    case 16:
        idx = 12;
        break;
    case 24:
        if (adsi->pwfxSrc->nBlockAlign != 3 * adsi->pwfxSrc->nChannels) {
            FIXME("Source: 24-bit samples must be packed\n");
            return MMSYSERR_NOTSUPPORTED;
        }
        idx = 24;
        break;
    default:
        FIXME("Unsupported source bit depth: %u\n", adsi->pwfxSrc->wBitsPerSample);
        return MMSYSERR_NOTSUPPORTED;
    }

    switch(adsi->pwfxDst->wBitsPerSample){
    case 8:
        break;
    case 16:
        idx += 4;
        break;
    case 24:
        if (adsi->pwfxDst->nBlockAlign != 3 * adsi->pwfxDst->nChannels) {
            FIXME("Destination: 24-bit samples must be packed\n");
            return MMSYSERR_NOTSUPPORTED;
        }
        idx += 8;
        break;
    default:
        FIXME("Unsupported destination bit depth: %u\n", adsi->pwfxDst->wBitsPerSample);
        return MMSYSERR_NOTSUPPORTED;
    }

    if (adsi->pwfxSrc->nChannels      == 1)  idx += 2;

    if (adsi->pwfxDst->nChannels      == 1)  idx += 1;

    apd = HeapAlloc(GetProcessHeap(), 0, sizeof(AcmPcmData));
    if (!apd)
        return MMSYSERR_NOMEM;

    if (adsi->pwfxSrc->nSamplesPerSec == adsi->pwfxDst->nSamplesPerSec) {
        flags = 0;
        apd->cvt.cvtKeepRate = PCM_ConvertKeepRate[idx];
    } else {
        flags = PCM_RESAMPLE;
        apd->cvt.cvtChangeRate = PCM_ConvertChangeRate[idx];
    }

    if(!apd->cvt.cvtChangeRate && !apd->cvt.cvtKeepRate){
        FIXME("Unimplemented conversion from %u -> %u bps\n",
            adsi->pwfxSrc->wBitsPerSample,
            adsi->pwfxDst->wBitsPerSample);
        HeapFree(GetProcessHeap(), 0, apd);
        return MMSYSERR_NOTSUPPORTED;
    }

    adsi->dwDriver = (DWORD_PTR)apd;
    adsi->fdwDriver = flags;

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
	WARN("Unsupported query %08x\n", adss->fdwSize);
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

    TRACE("nsrc=%d,adsh->cbSrcLength=%d\n", nsrc, adsh->cbSrcLength);
    TRACE("ndst=%d,adsh->cbDstLength=%d\n", ndst, adsh->cbDstLength);
    TRACE("src [wFormatTag=%u, nChannels=%u, nSamplesPerSec=%u, nAvgBytesPerSec=%u, nBlockAlign=%u, wBitsPerSample=%u, cbSize=%u]\n",
          adsi->pwfxSrc->wFormatTag, adsi->pwfxSrc->nChannels, adsi->pwfxSrc->nSamplesPerSec, adsi->pwfxSrc->nAvgBytesPerSec,
          adsi->pwfxSrc->nBlockAlign, adsi->pwfxSrc->wBitsPerSample, adsi->pwfxSrc->cbSize);
    TRACE("dst [wFormatTag=%u, nChannels=%u, nSamplesPerSec=%u, nAvgBytesPerSec=%u, nBlockAlign=%u, wBitsPerSample=%u, cbSize=%u]\n",
          adsi->pwfxDst->wFormatTag, adsi->pwfxDst->nChannels, adsi->pwfxDst->nSamplesPerSec, adsi->pwfxDst->nAvgBytesPerSec,
          adsi->pwfxDst->nBlockAlign, adsi->pwfxDst->wBitsPerSample, adsi->pwfxDst->cbSize);

    if (adsh->fdwConvert &
	~(ACM_STREAMCONVERTF_BLOCKALIGN|
	  ACM_STREAMCONVERTF_END|
	  ACM_STREAMCONVERTF_START)) {
	FIXME("Unsupported fdwConvert (%08x), ignoring it\n", adsh->fdwConvert);
    }
    /* ACM_STREAMCONVERTF_BLOCKALIGN
     *	currently all conversions are block aligned, so do nothing for this flag
     * ACM_STREAMCONVERTF_END
     *	no pending data, so do nothing for this flag
     */
    if ((adsh->fdwConvert & ACM_STREAMCONVERTF_START) &&
	(adsi->fdwDriver & PCM_RESAMPLE)) {
    }

    /* do the job */
    if (adsi->fdwDriver & PCM_RESAMPLE) {
	DWORD	nsrc2 = nsrc;
	DWORD	ndst2 = ndst;
	apd->cvt.cvtChangeRate(adsi->pwfxSrc->nSamplesPerSec, adsh->pbSrc, &nsrc2,
			       adsi->pwfxDst->nSamplesPerSec, adsh->pbDst, &ndst2);
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
LRESULT CALLBACK PCM_DriverProc(DWORD_PTR dwDevID, HDRVR hDriv, UINT wMsg,
				       LPARAM dwParam1, LPARAM dwParam2)
{
    TRACE("(%08lx %p %u %08lx %08lx);\n",
          dwDevID, hDriv, wMsg, dwParam1, dwParam2);

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
}
