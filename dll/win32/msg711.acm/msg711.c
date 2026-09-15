/*
 * G711 handling (includes A-Law & MU-Law)
 *
 *      Copyright (C) 2002		Eric Pouech
 *
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
 */

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "mmsystem.h"
#include "mmreg.h"
#include "msacm.h"
#include "msacmdrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(g711);

/***********************************************************************
 *           G711_drvOpen
 */
static LRESULT G711_drvOpen(LPCSTR str)
{
    return 1;
}

/***********************************************************************
 *           G711_drvClose
 */
static LRESULT G711_drvClose(DWORD_PTR dwDevID)
{
    return 1;
}

typedef struct tagAcmG711Data
{
    void (*convert)(PACMDRVSTREAMINSTANCE adsi,
		    const unsigned char*, LPDWORD, unsigned char*, LPDWORD);
} AcmG711Data;

/* table to list all supported formats... those are the basic ones. this
 * also helps given a unique index to each of the supported formats
 */
typedef	struct
{
    int		nChannels;
    int		nBits;
    int		rate;
} Format;

static const Format PCM_Formats[] =
{
    /*{1,  8,  8000}, {2,  8,  8000}, */{1, 16,  8000}, {2, 16,  8000},
    /*{1,  8, 11025}, {2,  8, 11025}, */{1, 16, 11025}, {2, 16, 11025},
    /*{1,  8, 22050}, {2,  8, 22050}, */{1, 16, 22050}, {2, 16, 22050},
    /*{1,  8, 44100}, {2,  8, 44100}, */{1, 16, 44100}, {2, 16, 44100},
};

static const Format ALaw_Formats[] =
{
    {1,  8,  8000}, {2,	8,  8000},  {1,  8, 11025}, {2,	 8, 11025},
    {1,  8, 22050}, {2,	8, 22050},  {1,  8, 44100}, {2,	 8, 44100},
};

static const Format ULaw_Formats[] =
{
    {1,  8,  8000}, {2,	8,  8000},  {1,  8, 11025}, {2,	 8, 11025},
    {1,  8, 22050}, {2,	8, 22050},  {1,  8, 44100}, {2,	 8, 44100},
};

/***********************************************************************
 *           G711_GetFormatIndex
 */
static	DWORD	G711_GetFormatIndex(const WAVEFORMATEX *wfx)
{
    int             i, hi;
    const Format*   fmts;

    switch (wfx->wFormatTag)
    {
    case WAVE_FORMAT_PCM:
	hi = ARRAY_SIZE(PCM_Formats);
	fmts = PCM_Formats;
	break;
    case WAVE_FORMAT_ALAW:
	hi = ARRAY_SIZE(ALaw_Formats);
	fmts = ALaw_Formats;
	break;
    case WAVE_FORMAT_MULAW:
	hi = ARRAY_SIZE(ULaw_Formats);
	fmts = ULaw_Formats;
	break;
    default:
	return 0xFFFFFFFF;
    }

    for (i = 0; i < hi; i++)
    {
	if (wfx->nChannels == fmts[i].nChannels &&
	    wfx->nSamplesPerSec == fmts[i].rate &&
	    wfx->wBitsPerSample == fmts[i].nBits)
	    return i;
    }

    return 0xFFFFFFFF;
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
 *           W16
 *
 * Write a 16 bit sample (correctly handles endianness)
 */
static inline void  W16(unsigned char* dst, short s)
{
    dst[0] = LOBYTE(s);
    dst[1] = HIBYTE(s);
}

/* You can uncomment this if you don't want the statically generated conversion
 * table, but rather recompute the Xlaw => PCM conversion for each sample
#define NO_FASTDECODE
 * Since the conversion tables are rather small (2k), I don't think it's really
 * interesting not to use them, but keeping the actual conversion code around
 * is helpful to regenerate the tables when needed.
 */

/* -------------------------------------------------------------------------------*/

/*
 * This source code is a product of Sun Microsystems, Inc. and is provided
 * for unrestricted use.  Users may copy or modify this source code without
 * charge.
 *
 * SUN SOURCE CODE IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING
 * THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun source code is provided with no support and without any obligation on
 * the part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY THIS SOFTWARE
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

/*
 * g711.c
 *
 * u-law, A-law and linear PCM conversions.
 */

/*
 * December 30, 1994:
 * Functions linear2alaw, linear2ulaw have been updated to correctly
 * convert unquantized 16 bit values.
 * Tables for direct u- to A-law and A- to u-law conversions have been
 * corrected.
 * Borge Lindberg, Center for PersonKommunikation, Aalborg University.
 * bli@cpk.auc.dk
 *
 */

#define	SIGN_BIT	(0x80)		/* Sign bit for an A-law byte. */
#define	QUANT_MASK	(0xf)		/* Quantization field mask. */
#define	NSEGS		(8)		/* Number of A-law segments. */
#define	SEG_SHIFT	(4)		/* Left shift for segment number. */
#define	SEG_MASK	(0x70)		/* Segment field mask. */

static const short seg_aend[8] = {0x1F, 0x3F, 0x7F, 0x0FF, 0x1FF, 0x3FF, 0x7FF, 0x0FFF};
static const short seg_uend[8] = {0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF};

/* copy from CCITT G.711 specifications */
static const unsigned char _u2a[128] = {		/* u- to A-law conversions */
	1,	1,	2,	2,	3,	3,	4,	4,
	5,	5,	6,	6,	7,	7,	8,	8,
	9,	10,	11,	12,	13,	14,	15,	16,
	17,	18,	19,	20,	21,	22,	23,	24,
	25,	27,	29,	31,	33,	34,	35,	36,
	37,	38,	39,	40,	41,	42,	43,	44,
	46,	48,	49,	50,	51,	52,	53,	54,
	55,	56,	57,	58,	59,	60,	61,	62,
	64,	65,	66,	67,	68,	69,	70,	71,
	72,	73,	74,	75,	76,	77,	78,	79,
/* corrected:
	81,	82,	83,	84,	85,	86,	87,	88,
   should be: */
	80,	82,	83,	84,	85,	86,	87,	88,
	89,	90,	91,	92,	93,	94,	95,	96,
	97,	98,	99,	100,	101,	102,	103,	104,
	105,	106,	107,	108,	109,	110,	111,	112,
	113,	114,	115,	116,	117,	118,	119,	120,
	121,	122,	123,	124,	125,	126,	127,	128};

static const unsigned char _a2u[128] = {		/* A- to u-law conversions */
	1,	3,	5,	7,	9,	11,	13,	15,
	16,	17,	18,	19,	20,	21,	22,	23,
	24,	25,	26,	27,	28,	29,	30,	31,
	32,	32,	33,	33,	34,	34,	35,	35,
	36,	37,	38,	39,	40,	41,	42,	43,
	44,	45,	46,	47,	48,	48,	49,	49,
	50,	51,	52,	53,	54,	55,	56,	57,
	58,	59,	60,	61,	62,	63,	64,	64,
	65,	66,	67,	68,	69,	70,	71,	72,
/* corrected:
	73,	74,	75,	76,	77,	78,	79,	79,
   should be: */
	73,	74,	75,	76,	77,	78,	79,	80,

	80,	81,	82,	83,	84,	85,	86,	87,
	88,	89,	90,	91,	92,	93,	94,	95,
	96,	97,	98,	99,	100,	101,	102,	103,
	104,	105,	106,	107,	108,	109,	110,	111,
	112,	113,	114,	115,	116,	117,	118,	119,
	120,	121,	122,	123,	124,	125,	126,	127};

static short
search(
    int		val,	        /* changed from "short" *drago* */
    const short	*table,
    int		size)	        /* changed from "short" *drago* */
{
    int		i;	/* changed from "short" *drago* */

    for (i = 0; i < size; i++) {
        if (val <= *table++)
            return (i);
    }
    return (size);
}

/*
 * linear2alaw() - Convert a 16-bit linear PCM value to 8-bit A-law
 *
 * linear2alaw() accepts an 16-bit integer and encodes it as A-law data.
 *
 *		Linear Input Code	Compressed Code
 *	------------------------	---------------
 *	0000000wxyza			000wxyz
 *	0000001wxyza			001wxyz
 *	000001wxyzab			010wxyz
 *	00001wxyzabc			011wxyz
 *	0001wxyzabcd			100wxyz
 *	001wxyzabcde			101wxyz
 *	01wxyzabcdef			110wxyz
 *	1wxyzabcdefg			111wxyz
 *
 * For further information see John C. Bellamy's Digital Telephony, 1982,
 * John Wiley & Sons, pps 98-111 and 472-476.
 */
static inline unsigned char
linear2alaw(int pcm_val)	/* 2's complement (16-bit range) */
    /* changed from "short" *drago* */
{
    int		mask;	/* changed from "short" *drago* */
    int		seg;	/* changed from "short" *drago* */
    unsigned char	aval;

    pcm_val = pcm_val >> 3;

    if (pcm_val >= 0) {
        mask = 0xD5;		/* sign (7th) bit = 1 */
    } else {
        mask = 0x55;		/* sign bit = 0 */
        pcm_val = -pcm_val - 1;
    }

    /* Convert the scaled magnitude to segment number. */
    seg = search(pcm_val, seg_aend, 8);

    /* Combine the sign, segment, and quantization bits. */

    if (seg >= 8)		/* out of range, return maximum value. */
        return (unsigned char) (0x7F ^ mask);
    else {
        aval = (unsigned char) seg << SEG_SHIFT;
        if (seg < 2)
            aval |= (pcm_val >> 1) & QUANT_MASK;
        else
            aval |= (pcm_val >> seg) & QUANT_MASK;
        return (aval ^ mask);
    }
}

#ifdef NO_FASTDECODE
/*
 * alaw2linear() - Convert an A-law value to 16-bit linear PCM
 *
 */
static inline int
alaw2linear(unsigned char a_val)
{
    int		t;	/* changed from "short" *drago* */
    int		seg;	/* changed from "short" *drago* */

    a_val ^= 0x55;

    t = (a_val & QUANT_MASK) << 4;
    seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;
    switch (seg) {
    case 0:
        t += 8;
        break;
    case 1:
        t += 0x108;
        break;
    default:
        t += 0x108;
        t <<= seg - 1;
    }
    return ((a_val & SIGN_BIT) ? t : -t);
}
#else
/* EPP (for Wine):
 * this array has been statically generated from the above routine
 */
static const unsigned short _a2l[] = {
0xEA80, 0xEB80, 0xE880, 0xE980, 0xEE80, 0xEF80, 0xEC80, 0xED80,
0xE280, 0xE380, 0xE080, 0xE180, 0xE680, 0xE780, 0xE480, 0xE580,
0xF540, 0xF5C0, 0xF440, 0xF4C0, 0xF740, 0xF7C0, 0xF640, 0xF6C0,
0xF140, 0xF1C0, 0xF040, 0xF0C0, 0xF340, 0xF3C0, 0xF240, 0xF2C0,
0xAA00, 0xAE00, 0xA200, 0xA600, 0xBA00, 0xBE00, 0xB200, 0xB600,
0x8A00, 0x8E00, 0x8200, 0x8600, 0x9A00, 0x9E00, 0x9200, 0x9600,
0xD500, 0xD700, 0xD100, 0xD300, 0xDD00, 0xDF00, 0xD900, 0xDB00,
0xC500, 0xC700, 0xC100, 0xC300, 0xCD00, 0xCF00, 0xC900, 0xCB00,
0xFEA8, 0xFEB8, 0xFE88, 0xFE98, 0xFEE8, 0xFEF8, 0xFEC8, 0xFED8,
0xFE28, 0xFE38, 0xFE08, 0xFE18, 0xFE68, 0xFE78, 0xFE48, 0xFE58,
0xFFA8, 0xFFB8, 0xFF88, 0xFF98, 0xFFE8, 0xFFF8, 0xFFC8, 0xFFD8,
0xFF28, 0xFF38, 0xFF08, 0xFF18, 0xFF68, 0xFF78, 0xFF48, 0xFF58,
0xFAA0, 0xFAE0, 0xFA20, 0xFA60, 0xFBA0, 0xFBE0, 0xFB20, 0xFB60,
0xF8A0, 0xF8E0, 0xF820, 0xF860, 0xF9A0, 0xF9E0, 0xF920, 0xF960,
0xFD50, 0xFD70, 0xFD10, 0xFD30, 0xFDD0, 0xFDF0, 0xFD90, 0xFDB0,
0xFC50, 0xFC70, 0xFC10, 0xFC30, 0xFCD0, 0xFCF0, 0xFC90, 0xFCB0,
0x1580, 0x1480, 0x1780, 0x1680, 0x1180, 0x1080, 0x1380, 0x1280,
0x1D80, 0x1C80, 0x1F80, 0x1E80, 0x1980, 0x1880, 0x1B80, 0x1A80,
0x0AC0, 0x0A40, 0x0BC0, 0x0B40, 0x08C0, 0x0840, 0x09C0, 0x0940,
0x0EC0, 0x0E40, 0x0FC0, 0x0F40, 0x0CC0, 0x0C40, 0x0DC0, 0x0D40,
0x5600, 0x5200, 0x5E00, 0x5A00, 0x4600, 0x4200, 0x4E00, 0x4A00,
0x7600, 0x7200, 0x7E00, 0x7A00, 0x6600, 0x6200, 0x6E00, 0x6A00,
0x2B00, 0x2900, 0x2F00, 0x2D00, 0x2300, 0x2100, 0x2700, 0x2500,
0x3B00, 0x3900, 0x3F00, 0x3D00, 0x3300, 0x3100, 0x3700, 0x3500,
0x0158, 0x0148, 0x0178, 0x0168, 0x0118, 0x0108, 0x0138, 0x0128,
0x01D8, 0x01C8, 0x01F8, 0x01E8, 0x0198, 0x0188, 0x01B8, 0x01A8,
0x0058, 0x0048, 0x0078, 0x0068, 0x0018, 0x0008, 0x0038, 0x0028,
0x00D8, 0x00C8, 0x00F8, 0x00E8, 0x0098, 0x0088, 0x00B8, 0x00A8,
0x0560, 0x0520, 0x05E0, 0x05A0, 0x0460, 0x0420, 0x04E0, 0x04A0,
0x0760, 0x0720, 0x07E0, 0x07A0, 0x0660, 0x0620, 0x06E0, 0x06A0,
0x02B0, 0x0290, 0x02F0, 0x02D0, 0x0230, 0x0210, 0x0270, 0x0250,
0x03B0, 0x0390, 0x03F0, 0x03D0, 0x0330, 0x0310, 0x0370, 0x0350,
};
static inline int
alaw2linear(unsigned char a_val)
{
    return (short)_a2l[a_val];
}
#endif

#define	BIAS		(0x84)		/* Bias for linear code. */
#define CLIP            8159

/*
 * linear2ulaw() - Convert a linear PCM value to u-law
 *
 * In order to simplify the encoding process, the original linear magnitude
 * is biased by adding 33 which shifts the encoding range from (0 - 8158) to
 * (33 - 8191). The result can be seen in the following encoding table:
 *
 *	Biased Linear Input Code	Compressed Code
 *	------------------------	---------------
 *	00000001wxyza			000wxyz
 *	0000001wxyzab			001wxyz
 *	000001wxyzabc			010wxyz
 *	00001wxyzabcd			011wxyz
 *	0001wxyzabcde			100wxyz
 *	001wxyzabcdef			101wxyz
 *	01wxyzabcdefg			110wxyz
 *	1wxyzabcdefgh			111wxyz
 *
 * Each biased linear code has a leading 1 which identifies the segment
 * number. The value of the segment number is equal to 7 minus the number
 * of leading 0's. The quantization interval is directly available as the
 * four bits wxyz.  * The trailing bits (a - h) are ignored.
 *
 * Ordinarily the complement of the resulting code word is used for
 * transmission, and so the code word is complemented before it is returned.
 *
 * For further information see John C. Bellamy's Digital Telephony, 1982,
 * John Wiley & Sons, pps 98-111 and 472-476.
 */
static inline unsigned char
linear2ulaw(short pcm_val)	/* 2's complement (16-bit range) */
{
    short		mask;
    short		seg;
    unsigned char	uval;

    /* Get the sign and the magnitude of the value. */
    pcm_val = pcm_val >> 2;
    if (pcm_val < 0) {
        pcm_val = -pcm_val;
        mask = 0x7F;
    } else {
        mask = 0xFF;
    }
    if ( pcm_val > CLIP ) pcm_val = CLIP;		/* clip the magnitude */
    pcm_val += (BIAS >> 2);

    /* Convert the scaled magnitude to segment number. */
    seg = search(pcm_val, seg_uend, 8);

    /*
     * Combine the sign, segment, quantization bits;
     * and complement the code word.
     */
    if (seg >= 8)		/* out of range, return maximum value. */
        return (unsigned char) (0x7F ^ mask);
    else {
        uval = (unsigned char) (seg << 4) | ((pcm_val >> (seg + 1)) & 0xF);
        return (uval ^ mask);
    }
}

#ifdef NO_FASTDECODE
/*
 * ulaw2linear() - Convert a u-law value to 16-bit linear PCM
 *
 * First, a biased linear code is derived from the code word. An unbiased
 * output can then be obtained by subtracting 33 from the biased code.
 *
 * Note that this function expects to be passed the complement of the
 * original code word. This is in keeping with ISDN conventions.
 */
static inline short
ulaw2linear(unsigned char u_val)
{
    short		t;

    /* Complement to obtain normal u-law value. */
    u_val = ~u_val;

    /*
     * Extract and bias the quantization bits. Then
     * shift up by the segment number and subtract out the bias.
     */
    t = ((u_val & QUANT_MASK) << 3) + BIAS;
    t <<= ((unsigned)u_val & SEG_MASK) >> SEG_SHIFT;

    return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}
#else
/* EPP (for Wine):
 * this array has been statically generated from the above routine
 */
static const unsigned short _u2l[] = {
0x8284, 0x8684, 0x8A84, 0x8E84, 0x9284, 0x9684, 0x9A84, 0x9E84,
0xA284, 0xA684, 0xAA84, 0xAE84, 0xB284, 0xB684, 0xBA84, 0xBE84,
0xC184, 0xC384, 0xC584, 0xC784, 0xC984, 0xCB84, 0xCD84, 0xCF84,
0xD184, 0xD384, 0xD584, 0xD784, 0xD984, 0xDB84, 0xDD84, 0xDF84,
0xE104, 0xE204, 0xE304, 0xE404, 0xE504, 0xE604, 0xE704, 0xE804,
0xE904, 0xEA04, 0xEB04, 0xEC04, 0xED04, 0xEE04, 0xEF04, 0xF004,
0xF0C4, 0xF144, 0xF1C4, 0xF244, 0xF2C4, 0xF344, 0xF3C4, 0xF444,
0xF4C4, 0xF544, 0xF5C4, 0xF644, 0xF6C4, 0xF744, 0xF7C4, 0xF844,
0xF8A4, 0xF8E4, 0xF924, 0xF964, 0xF9A4, 0xF9E4, 0xFA24, 0xFA64,
0xFAA4, 0xFAE4, 0xFB24, 0xFB64, 0xFBA4, 0xFBE4, 0xFC24, 0xFC64,
0xFC94, 0xFCB4, 0xFCD4, 0xFCF4, 0xFD14, 0xFD34, 0xFD54, 0xFD74,
0xFD94, 0xFDB4, 0xFDD4, 0xFDF4, 0xFE14, 0xFE34, 0xFE54, 0xFE74,
0xFE8C, 0xFE9C, 0xFEAC, 0xFEBC, 0xFECC, 0xFEDC, 0xFEEC, 0xFEFC,
0xFF0C, 0xFF1C, 0xFF2C, 0xFF3C, 0xFF4C, 0xFF5C, 0xFF6C, 0xFF7C,
0xFF88, 0xFF90, 0xFF98, 0xFFA0, 0xFFA8, 0xFFB0, 0xFFB8, 0xFFC0,
0xFFC8, 0xFFD0, 0xFFD8, 0xFFE0, 0xFFE8, 0xFFF0, 0xFFF8, 0x0000,
0x7D7C, 0x797C, 0x757C, 0x717C, 0x6D7C, 0x697C, 0x657C, 0x617C,
0x5D7C, 0x597C, 0x557C, 0x517C, 0x4D7C, 0x497C, 0x457C, 0x417C,
0x3E7C, 0x3C7C, 0x3A7C, 0x387C, 0x367C, 0x347C, 0x327C, 0x307C,
0x2E7C, 0x2C7C, 0x2A7C, 0x287C, 0x267C, 0x247C, 0x227C, 0x207C,
0x1EFC, 0x1DFC, 0x1CFC, 0x1BFC, 0x1AFC, 0x19FC, 0x18FC, 0x17FC,
0x16FC, 0x15FC, 0x14FC, 0x13FC, 0x12FC, 0x11FC, 0x10FC, 0x0FFC,
0x0F3C, 0x0EBC, 0x0E3C, 0x0DBC, 0x0D3C, 0x0CBC, 0x0C3C, 0x0BBC,
0x0B3C, 0x0ABC, 0x0A3C, 0x09BC, 0x093C, 0x08BC, 0x083C, 0x07BC,
0x075C, 0x071C, 0x06DC, 0x069C, 0x065C, 0x061C, 0x05DC, 0x059C,
0x055C, 0x051C, 0x04DC, 0x049C, 0x045C, 0x041C, 0x03DC, 0x039C,
0x036C, 0x034C, 0x032C, 0x030C, 0x02EC, 0x02CC, 0x02AC, 0x028C,
0x026C, 0x024C, 0x022C, 0x020C, 0x01EC, 0x01CC, 0x01AC, 0x018C,
0x0174, 0x0164, 0x0154, 0x0144, 0x0134, 0x0124, 0x0114, 0x0104,
0x00F4, 0x00E4, 0x00D4, 0x00C4, 0x00B4, 0x00A4, 0x0094, 0x0084,
0x0078, 0x0070, 0x0068, 0x0060, 0x0058, 0x0050, 0x0048, 0x0040,
0x0038, 0x0030, 0x0028, 0x0020, 0x0018, 0x0010, 0x0008, 0x0000,
};
static inline short ulaw2linear(unsigned char u_val)
{
    return (short)_u2l[u_val];
}
#endif

/* A-law to u-law conversion */
static inline unsigned char
alaw2ulaw(unsigned char aval)
{
    aval &= 0xff;
    return (unsigned char) ((aval & 0x80) ? (0xFF ^ _a2u[aval ^ 0xD5]) :
                            (0x7F ^ _a2u[aval ^ 0x55]));
}

/* u-law to A-law conversion */
static inline unsigned char
ulaw2alaw(unsigned char uval)
{
    uval &= 0xff;
    return (uval & 0x80) ? (0xD5 ^ (_u2a[0xFF ^ uval] - 1)) : (0x55 ^ (_u2a[0x7F ^ uval] - 1));
}

/* -------------------------------------------------------------------------------*/

static void cvtXXalaw16K(PACMDRVSTREAMINSTANCE adsi,
                         const unsigned char* src, LPDWORD srcsize,
                         unsigned char* dst, LPDWORD dstsize)
{
    DWORD       len = min(*srcsize, *dstsize / 2);
    DWORD       i;
    short       w;

    *srcsize = len;
    *dstsize = len * 2;
    for (i = 0; i < len; i++)
    {
        w = alaw2linear(*src++);
        W16(dst, w);    dst += 2;
    }
}

static void cvtXX16alawK(PACMDRVSTREAMINSTANCE adsi,
                         const unsigned char* src, LPDWORD srcsize,
                         unsigned char* dst, LPDWORD dstsize)
{
    DWORD       len = min(*srcsize / 2, *dstsize);
    DWORD       i;

    *srcsize = len * 2;
    *dstsize = len;
    for (i = 0; i < len; i++)
    {
        *dst++ = linear2alaw(R16(src)); src += 2;
    }
}

static void cvtXXulaw16K(PACMDRVSTREAMINSTANCE adsi,
                         const unsigned char* src, LPDWORD srcsize,
                         unsigned char* dst, LPDWORD dstsize)
{
    DWORD       len = min(*srcsize, *dstsize / 2);
    DWORD       i;
    short       w;

    *srcsize = len;
    *dstsize = len * 2;
    for (i = 0; i < len; i++)
    {
        w = ulaw2linear(*src++);
        W16(dst, w);    dst += 2;
    }
}

static void cvtXX16ulawK(PACMDRVSTREAMINSTANCE adsi,
                         const unsigned char* src, LPDWORD srcsize,
                         unsigned char* dst, LPDWORD dstsize)
{
    DWORD       len = min(*srcsize / 2, *dstsize);
    DWORD       i;

    *srcsize = len * 2;
    *dstsize = len;
    for (i = 0; i < len; i++)
    {
        *dst++ = linear2ulaw(R16(src)); src += 2;
    }
}

static void cvtXXalawulawK(PACMDRVSTREAMINSTANCE adsi,
                           const unsigned char* src, LPDWORD srcsize,
                           unsigned char* dst, LPDWORD dstsize)
{
    DWORD       len = min(*srcsize, *dstsize);
    DWORD       i;

    *srcsize = len;
    *dstsize = len;

    for (i = 0; i < len; i++)
        *dst++ = alaw2ulaw(*src++);
}


static void cvtXXulawalawK(PACMDRVSTREAMINSTANCE adsi,
                           const unsigned char* src, LPDWORD srcsize,
                           unsigned char* dst, LPDWORD dstsize)
{
    DWORD       len = min(*srcsize, *dstsize);
    DWORD       i;

    *srcsize = len;
    *dstsize = len;

    for (i = 0; i < len; i++)
        *dst++ = ulaw2alaw(*src++);
}

/***********************************************************************
 *           G711_DriverDetails
 *
 */
static	LRESULT G711_DriverDetails(PACMDRIVERDETAILSW add)
{
    add->fccType = ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC;
    add->fccComp = ACMDRIVERDETAILS_FCCCOMP_UNDEFINED;
    add->wMid = MM_MICROSOFT;
    add->wPid = MM_MSFT_ACM_G711;
    add->vdwACM = 0x01000000;
    add->vdwDriver = 0x01000000;
    add->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CODEC;
    add->cFormatTags = 3; /* PCM, G711 A-LAW & MU-LAW */
    add->cFilterTags = 0;
    add->hicon = NULL;
    MultiByteToWideChar( CP_ACP, 0, "Microsoft CCITT G.711", -1,
                         add->szShortName, ARRAY_SIZE( add->szShortName ));
    MultiByteToWideChar( CP_ACP, 0, "Wine G711 converter", -1,
                         add->szLongName, ARRAY_SIZE( add->szLongName ));
    MultiByteToWideChar( CP_ACP, 0, "Brought to you by the Wine team...", -1,
                         add->szCopyright, ARRAY_SIZE( add->szCopyright ));
    MultiByteToWideChar( CP_ACP, 0, "Refer to LICENSE file", -1,
                         add->szLicensing, ARRAY_SIZE( add->szLicensing ));
    add->szFeatures[0] = 0;

    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           G711_FormatTagDetails
 *
 */
static	LRESULT	G711_FormatTagDetails(PACMFORMATTAGDETAILSW aftd, DWORD dwQuery)
{
    switch (dwQuery)
    {
    case ACM_FORMATTAGDETAILSF_INDEX:
	if (aftd->dwFormatTagIndex >= 3) return ACMERR_NOTPOSSIBLE;
	break;
    case ACM_FORMATTAGDETAILSF_LARGESTSIZE:
	if (aftd->dwFormatTag == WAVE_FORMAT_UNKNOWN)
        {
            aftd->dwFormatTagIndex = 1;
	    break;
	}
	/* fall through */
    case ACM_FORMATTAGDETAILSF_FORMATTAG:
	switch (aftd->dwFormatTag)
        {
	case WAVE_FORMAT_PCM:	aftd->dwFormatTagIndex = 0; break;
	case WAVE_FORMAT_ALAW:  aftd->dwFormatTagIndex = 1; break;
	case WAVE_FORMAT_MULAW: aftd->dwFormatTagIndex = 2; break;
	default:		return ACMERR_NOTPOSSIBLE;
	}
	break;
    default:
	WARN("Unsupported query %08lx\n", dwQuery);
	return MMSYSERR_NOTSUPPORTED;
    }

    aftd->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CODEC;
    switch (aftd->dwFormatTagIndex)
    {
    case 0:
	aftd->dwFormatTag = WAVE_FORMAT_PCM;
	aftd->cbFormatSize = sizeof(PCMWAVEFORMAT);
	aftd->cStandardFormats = ARRAY_SIZE(PCM_Formats);
        lstrcpyW(aftd->szFormatTag, L"PCM");
        break;
    case 1:
	aftd->dwFormatTag = WAVE_FORMAT_ALAW;
	aftd->cbFormatSize = sizeof(WAVEFORMATEX);
	aftd->cStandardFormats = ARRAY_SIZE(ALaw_Formats);
        lstrcpyW(aftd->szFormatTag, L"A-Law");
	break;
    case 2:
	aftd->dwFormatTag = WAVE_FORMAT_MULAW;
	aftd->cbFormatSize = sizeof(WAVEFORMATEX);
	aftd->cStandardFormats = ARRAY_SIZE(ULaw_Formats);
        lstrcpyW(aftd->szFormatTag, L"U-Law");
	break;
    }
    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           G711_FormatDetails
 *
 */
static	LRESULT	G711_FormatDetails(PACMFORMATDETAILSW afd, DWORD dwQuery)
{
    switch (dwQuery)
    {
    case ACM_FORMATDETAILSF_FORMAT:
	if (G711_GetFormatIndex(afd->pwfx) == 0xFFFFFFFF) return ACMERR_NOTPOSSIBLE;
	break;
    case ACM_FORMATDETAILSF_INDEX:
	afd->pwfx->wFormatTag = afd->dwFormatTag;
	switch (afd->dwFormatTag)
        {
	case WAVE_FORMAT_PCM:
	    if (afd->dwFormatIndex >= ARRAY_SIZE(PCM_Formats)) return ACMERR_NOTPOSSIBLE;
	    afd->pwfx->nChannels = PCM_Formats[afd->dwFormatIndex].nChannels;
	    afd->pwfx->nSamplesPerSec = PCM_Formats[afd->dwFormatIndex].rate;
	    afd->pwfx->wBitsPerSample = PCM_Formats[afd->dwFormatIndex].nBits;
	    afd->pwfx->nBlockAlign = afd->pwfx->nChannels * 2;
	    afd->pwfx->nAvgBytesPerSec = afd->pwfx->nSamplesPerSec * afd->pwfx->nBlockAlign;
	    break;
	case WAVE_FORMAT_ALAW:
	    if (afd->dwFormatIndex >= ARRAY_SIZE(ALaw_Formats)) return ACMERR_NOTPOSSIBLE;
	    afd->pwfx->nChannels = ALaw_Formats[afd->dwFormatIndex].nChannels;
	    afd->pwfx->nSamplesPerSec = ALaw_Formats[afd->dwFormatIndex].rate;
	    afd->pwfx->wBitsPerSample = ALaw_Formats[afd->dwFormatIndex].nBits;
	    afd->pwfx->nBlockAlign = ALaw_Formats[afd->dwFormatIndex].nChannels;
	    afd->pwfx->nAvgBytesPerSec = afd->pwfx->nSamplesPerSec * afd->pwfx->nChannels;
            afd->pwfx->cbSize = 0;
	    break;
	case WAVE_FORMAT_MULAW:
	    if (afd->dwFormatIndex >= ARRAY_SIZE(ULaw_Formats)) return ACMERR_NOTPOSSIBLE;
	    afd->pwfx->nChannels = ULaw_Formats[afd->dwFormatIndex].nChannels;
	    afd->pwfx->nSamplesPerSec = ULaw_Formats[afd->dwFormatIndex].rate;
	    afd->pwfx->wBitsPerSample = ULaw_Formats[afd->dwFormatIndex].nBits;
	    afd->pwfx->nBlockAlign = ULaw_Formats[afd->dwFormatIndex].nChannels;
	    afd->pwfx->nAvgBytesPerSec = afd->pwfx->nSamplesPerSec * afd->pwfx->nChannels;
            afd->pwfx->cbSize = 0;
	    break;
	default:
            WARN("Unsupported tag %08lx\n", afd->dwFormatTag);
	    return MMSYSERR_INVALPARAM;
	}
	break;
    default:
	WARN("Unsupported query %08lx\n", dwQuery);
	return MMSYSERR_NOTSUPPORTED;
    }
    afd->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CODEC;
    afd->szFormat[0] = 0; /* let MSACM format this for us... */

    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           G711_FormatSuggest
 *
 */
static	LRESULT	G711_FormatSuggest(PACMDRVFORMATSUGGEST adfs)
{
    /* some tests ... */
    if (adfs->cbwfxSrc < sizeof(PCMWAVEFORMAT) ||
	adfs->cbwfxDst < sizeof(PCMWAVEFORMAT) ||
	G711_GetFormatIndex(adfs->pwfxSrc) == 0xFFFFFFFF) return ACMERR_NOTPOSSIBLE;
    /* FIXME: should do those tests against the real size (according to format tag */

    /* If no suggestion for destination, then copy source value */
    if (!(adfs->fdwSuggest & ACM_FORMATSUGGESTF_NCHANNELS))
	adfs->pwfxDst->nChannels = adfs->pwfxSrc->nChannels;
    if (!(adfs->fdwSuggest & ACM_FORMATSUGGESTF_NSAMPLESPERSEC))
        adfs->pwfxDst->nSamplesPerSec = adfs->pwfxSrc->nSamplesPerSec;

    if (!(adfs->fdwSuggest & ACM_FORMATSUGGESTF_WBITSPERSAMPLE))
    {
	if (adfs->pwfxSrc->wFormatTag == WAVE_FORMAT_PCM)
            adfs->pwfxDst->wBitsPerSample = 8;
        else
            adfs->pwfxDst->wBitsPerSample = 16;
    }
    if (!(adfs->fdwSuggest & ACM_FORMATSUGGESTF_WFORMATTAG))
    {
	switch (adfs->pwfxSrc->wFormatTag)
        {
        case WAVE_FORMAT_PCM:   adfs->pwfxDst->wFormatTag = WAVE_FORMAT_ALAW; break;
        case WAVE_FORMAT_ALAW:  adfs->pwfxDst->wFormatTag = WAVE_FORMAT_PCM; break;
        case WAVE_FORMAT_MULAW: adfs->pwfxDst->wFormatTag = WAVE_FORMAT_PCM; break;
        }
    }
    /* check if result is ok */
    if (G711_GetFormatIndex(adfs->pwfxDst) == 0xFFFFFFFF) return ACMERR_NOTPOSSIBLE;

    /* recompute other values */
    switch (adfs->pwfxDst->wFormatTag)
    {
    case WAVE_FORMAT_PCM:
        adfs->pwfxDst->nBlockAlign = adfs->pwfxDst->nChannels * 2;
        adfs->pwfxDst->nAvgBytesPerSec = adfs->pwfxDst->nSamplesPerSec * adfs->pwfxDst->nBlockAlign;
        break;
    case WAVE_FORMAT_ALAW:
        adfs->pwfxDst->nBlockAlign = adfs->pwfxDst->nChannels;
        adfs->pwfxDst->nAvgBytesPerSec = adfs->pwfxDst->nSamplesPerSec * adfs->pwfxSrc->nChannels;
        break;
    case WAVE_FORMAT_MULAW:
        adfs->pwfxDst->nBlockAlign = adfs->pwfxDst->nChannels;
        adfs->pwfxDst->nAvgBytesPerSec = adfs->pwfxDst->nSamplesPerSec * adfs->pwfxSrc->nChannels;
        break;
    }

    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           G711_Reset
 *
 */
static	void	G711_Reset(PACMDRVSTREAMINSTANCE adsi, AcmG711Data* aad)
{
}

/***********************************************************************
 *           G711_StreamOpen
 *
 */
static	LRESULT	G711_StreamOpen(PACMDRVSTREAMINSTANCE adsi)
{
    AcmG711Data*	aad;

    assert(!(adsi->fdwOpen & ACM_STREAMOPENF_ASYNC));

    if (G711_GetFormatIndex(adsi->pwfxSrc) == 0xFFFFFFFF ||
	G711_GetFormatIndex(adsi->pwfxDst) == 0xFFFFFFFF)
	return ACMERR_NOTPOSSIBLE;

    aad = HeapAlloc(GetProcessHeap(), 0, sizeof(AcmG711Data));
    if (aad == 0) return MMSYSERR_NOMEM;

    adsi->dwDriver = (DWORD_PTR)aad;

    if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_PCM &&
	adsi->pwfxDst->wFormatTag == WAVE_FORMAT_PCM)
    {
	goto theEnd;
    }
    else if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_ALAW &&
             adsi->pwfxDst->wFormatTag == WAVE_FORMAT_PCM)
    {
	/* resampling or mono <=> stereo not available
         * G711 algo only define 16 bit per sample output
         */
	if (adsi->pwfxSrc->nSamplesPerSec != adsi->pwfxDst->nSamplesPerSec ||
	    adsi->pwfxSrc->nChannels != adsi->pwfxDst->nChannels ||
            adsi->pwfxDst->wBitsPerSample != 16)
	    goto theEnd;

	/* g711 A-Law decoding... */
	if (adsi->pwfxDst->wBitsPerSample == 16)
	    aad->convert = cvtXXalaw16K;
    }
    else if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_PCM &&
             adsi->pwfxDst->wFormatTag == WAVE_FORMAT_ALAW)
    {
	if (adsi->pwfxSrc->nSamplesPerSec != adsi->pwfxDst->nSamplesPerSec ||
	    adsi->pwfxSrc->nChannels != adsi->pwfxDst->nChannels ||
            adsi->pwfxSrc->wBitsPerSample != 16)
	    goto theEnd;

	/* g711 coding... */
	if (adsi->pwfxSrc->wBitsPerSample == 16)
	    aad->convert = cvtXX16alawK;
    }
    else if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_MULAW &&
             adsi->pwfxDst->wFormatTag == WAVE_FORMAT_PCM)
    {
	/* resampling or mono <=> stereo not available
         * G711 algo only define 16 bit per sample output
         */
	if (adsi->pwfxSrc->nSamplesPerSec != adsi->pwfxDst->nSamplesPerSec ||
	    adsi->pwfxSrc->nChannels != adsi->pwfxDst->nChannels ||
            adsi->pwfxDst->wBitsPerSample != 16)
	    goto theEnd;

	/* g711 MU-Law decoding... */
	if (adsi->pwfxDst->wBitsPerSample == 16)
	    aad->convert = cvtXXulaw16K;
    }
    else if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_PCM &&
             adsi->pwfxDst->wFormatTag == WAVE_FORMAT_MULAW)
    {
	if (adsi->pwfxSrc->nSamplesPerSec != adsi->pwfxDst->nSamplesPerSec ||
	    adsi->pwfxSrc->nChannels != adsi->pwfxDst->nChannels ||
            adsi->pwfxSrc->wBitsPerSample != 16)
	    goto theEnd;

	/* g711 coding... */
	if (adsi->pwfxSrc->wBitsPerSample == 16)
	    aad->convert = cvtXX16ulawK;
    }
    else if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_MULAW &&
             adsi->pwfxDst->wFormatTag == WAVE_FORMAT_ALAW)
    {
	if (adsi->pwfxSrc->nSamplesPerSec != adsi->pwfxDst->nSamplesPerSec ||
	    adsi->pwfxSrc->nChannels != adsi->pwfxDst->nChannels)
	    goto theEnd;

	/* MU-Law => A-Law... */
        aad->convert = cvtXXulawalawK;
    }
    else if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_ALAW &&
             adsi->pwfxDst->wFormatTag == WAVE_FORMAT_MULAW)
    {
	if (adsi->pwfxSrc->nSamplesPerSec != adsi->pwfxDst->nSamplesPerSec ||
	    adsi->pwfxSrc->nChannels != adsi->pwfxDst->nChannels)
	    goto theEnd;

	/* A-Law => MU-Law... */
        aad->convert = cvtXXalawulawK;
    }
    else goto theEnd;

    G711_Reset(adsi, aad);

    return MMSYSERR_NOERROR;

 theEnd:
    HeapFree(GetProcessHeap(), 0, aad);
    adsi->dwDriver = 0L;
    return MMSYSERR_NOTSUPPORTED;
}

/***********************************************************************
 *           G711_StreamClose
 *
 */
static	LRESULT	G711_StreamClose(PACMDRVSTREAMINSTANCE adsi)
{
    HeapFree(GetProcessHeap(), 0, (void*)adsi->dwDriver);
    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           G711_StreamSize
 *
 */
static	LRESULT G711_StreamSize(const ACMDRVSTREAMINSTANCE *adsi, PACMDRVSTREAMSIZE adss)
{
    switch (adss->fdwSize)
    {
    case ACM_STREAMSIZEF_DESTINATION:
	/* cbDstLength => cbSrcLength */
	if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_PCM &&
	    (adsi->pwfxDst->wFormatTag == WAVE_FORMAT_ALAW ||
             adsi->pwfxDst->wFormatTag == WAVE_FORMAT_MULAW))
        {
	    adss->cbSrcLength = adss->cbDstLength * 2;
	}
        else if ((adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_ALAW ||
                  adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_MULAW) &&
                 adsi->pwfxDst->wFormatTag == WAVE_FORMAT_PCM)
        {
	    adss->cbSrcLength = adss->cbDstLength / 2;
	}
        else if ((adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_ALAW ||
                  adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_MULAW) &&
                 (adsi->pwfxDst->wFormatTag == WAVE_FORMAT_ALAW ||
                  adsi->pwfxDst->wFormatTag == WAVE_FORMAT_MULAW))
        {
	    adss->cbSrcLength = adss->cbDstLength;
        }
        else
        {
	    return MMSYSERR_NOTSUPPORTED;
	}
	break;
    case ACM_STREAMSIZEF_SOURCE:
	/* cbSrcLength => cbDstLength */
	if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_PCM &&
	    (adsi->pwfxDst->wFormatTag == WAVE_FORMAT_ALAW ||
             adsi->pwfxDst->wFormatTag == WAVE_FORMAT_MULAW))
        {
	    adss->cbDstLength = adss->cbSrcLength / 2;
	}
        else if ((adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_ALAW ||
                  adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_MULAW) &&
                 adsi->pwfxDst->wFormatTag == WAVE_FORMAT_PCM)
        {
	    adss->cbDstLength = adss->cbSrcLength * 2;
	}
        else if ((adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_ALAW ||
                  adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_MULAW) &&
                 (adsi->pwfxDst->wFormatTag == WAVE_FORMAT_ALAW ||
                  adsi->pwfxDst->wFormatTag == WAVE_FORMAT_MULAW))
        {
	    adss->cbDstLength = adss->cbSrcLength;
        }
        else
        {
	    return MMSYSERR_NOTSUPPORTED;
	}
	break;
    default:
	WARN("Unsupported query %08lx\n", adss->fdwSize);
	return MMSYSERR_NOTSUPPORTED;
    }
    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           G711_StreamConvert
 *
 */
static LRESULT G711_StreamConvert(PACMDRVSTREAMINSTANCE adsi, PACMDRVSTREAMHEADER adsh)
{
    AcmG711Data*	aad = (AcmG711Data*)adsi->dwDriver;
    DWORD		nsrc = adsh->cbSrcLength;
    DWORD		ndst = adsh->cbDstLength;

    if (adsh->fdwConvert &
	~(ACM_STREAMCONVERTF_BLOCKALIGN|
	  ACM_STREAMCONVERTF_END|
	  ACM_STREAMCONVERTF_START))
    {
	FIXME("Unsupported fdwConvert (%08lx), ignoring it\n", adsh->fdwConvert);
    }
    /* ACM_STREAMCONVERTF_BLOCKALIGN
     *	currently all conversions are block aligned, so do nothing for this flag
     * ACM_STREAMCONVERTF_END
     *	no pending data, so do nothing for this flag
     */
    if ((adsh->fdwConvert & ACM_STREAMCONVERTF_START))
    {
	G711_Reset(adsi, aad);
    }

    aad->convert(adsi, adsh->pbSrc, &nsrc, adsh->pbDst, &ndst);
    adsh->cbSrcLengthUsed = nsrc;
    adsh->cbDstLengthUsed = ndst;

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			G711_DriverProc			[exported]
 */
LRESULT CALLBACK G711_DriverProc(DWORD_PTR dwDevID, HDRVR hDriv, UINT wMsg,
					 LPARAM dwParam1, LPARAM dwParam2)
{
    TRACE("(%08Ix %p %04x %08Ix %08Ix);\n",
          dwDevID, hDriv, wMsg, dwParam1, dwParam2);

    switch (wMsg)
    {
    case DRV_LOAD:		return 1;
    case DRV_FREE:		return 1;
    case DRV_OPEN:		return G711_drvOpen((LPSTR)dwParam1);
    case DRV_CLOSE:		return G711_drvClose(dwDevID);
    case DRV_ENABLE:		return 1;
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBoxA(0, "MS G711 (a-Law & mu-Law) filter !", "Wine Driver", MB_OK); return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;

    case ACMDM_DRIVER_NOTIFY:
	/* no caching from other ACM drivers is done so far */
	return MMSYSERR_NOERROR;

    case ACMDM_DRIVER_DETAILS:
	return G711_DriverDetails((PACMDRIVERDETAILSW)dwParam1);

    case ACMDM_FORMATTAG_DETAILS:
	return G711_FormatTagDetails((PACMFORMATTAGDETAILSW)dwParam1, dwParam2);

    case ACMDM_FORMAT_DETAILS:
	return G711_FormatDetails((PACMFORMATDETAILSW)dwParam1, dwParam2);

    case ACMDM_FORMAT_SUGGEST:
	return G711_FormatSuggest((PACMDRVFORMATSUGGEST)dwParam1);

    case ACMDM_STREAM_OPEN:
	return G711_StreamOpen((PACMDRVSTREAMINSTANCE)dwParam1);

    case ACMDM_STREAM_CLOSE:
	return G711_StreamClose((PACMDRVSTREAMINSTANCE)dwParam1);

    case ACMDM_STREAM_SIZE:
	return G711_StreamSize((PACMDRVSTREAMINSTANCE)dwParam1, (PACMDRVSTREAMSIZE)dwParam2);

    case ACMDM_STREAM_CONVERT:
	return G711_StreamConvert((PACMDRVSTREAMINSTANCE)dwParam1, (PACMDRVSTREAMHEADER)dwParam2);

    case ACMDM_HARDWARE_WAVE_CAPS_INPUT:
    case ACMDM_HARDWARE_WAVE_CAPS_OUTPUT:
	/* this converter is not a hardware driver */
    case ACMDM_FILTERTAG_DETAILS:
    case ACMDM_FILTER_DETAILS:
	/* this converter is not a filter */
    case ACMDM_STREAM_RESET:
	/* only needed for asynchronous driver... we aren't, so just say it */
	return MMSYSERR_NOTSUPPORTED;
    case ACMDM_STREAM_PREPARE:
    case ACMDM_STREAM_UNPREPARE:
	/* nothing special to do here... so don't do anything */
	return MMSYSERR_NOERROR;

    default:
	return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
}
