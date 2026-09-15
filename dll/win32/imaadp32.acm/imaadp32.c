/*
 * IMA ADPCM handling
 *
 *      Copyright (C) 2001,2002		Eric Pouech
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

/* see http://www.pcisys.net/~melanson/codecs/adpcm.txt for the details */

WINE_DEFAULT_DEBUG_CHANNEL(adpcm);

/***********************************************************************
 *           ADPCM_drvClose
 */
static LRESULT ADPCM_drvClose(DWORD_PTR dwDevID)
{
    return 1;
}

typedef struct tagAcmAdpcmData
{
    void (*convert)(PACMDRVSTREAMINSTANCE adsi,
		    const unsigned char*, LPDWORD, unsigned char*, LPDWORD);
    /* IMA encoding only */
    BYTE	stepIndexL;
    BYTE	stepIndexR;
    /* short	sample; */
} AcmAdpcmData;

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
    {1,  8,  8000}, {2,  8,  8000}, {1, 16,  8000}, {2, 16,  8000},
    {1,  8, 11025}, {2,  8, 11025}, {1, 16, 11025}, {2, 16, 11025},
    {1,  8, 22050}, {2,  8, 22050}, {1, 16, 22050}, {2, 16, 22050},
    {1,  8, 44100}, {2,  8, 44100}, {1, 16, 44100}, {2, 16, 44100},
};

static const Format ADPCM_Formats[] =
{
    {1,  4,  8000}, {2,	4,  8000},  {1,  4, 11025}, {2,	 4, 11025},
    {1,  4, 22050}, {2,	4, 22050},  {1,  4, 44100}, {2,	 4, 44100},
};

/***********************************************************************
 *           ADPCM_GetFormatIndex
 */
static	DWORD	ADPCM_GetFormatIndex(const WAVEFORMATEX *wfx)
{
    int             i, hi;
    const Format*   fmts;

    switch (wfx->wFormatTag)
    {
    case WAVE_FORMAT_PCM:
	hi = ARRAY_SIZE(PCM_Formats);
	fmts = PCM_Formats;
	break;
    case WAVE_FORMAT_IMA_ADPCM:
	hi = ARRAY_SIZE(ADPCM_Formats);
	fmts = ADPCM_Formats;
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

    switch (wfx->wFormatTag)
    {
    case WAVE_FORMAT_PCM:
	if(3 > wfx->nChannels &&
	   wfx->nChannels > 0 &&
	   wfx->nAvgBytesPerSec == 2 * wfx->nSamplesPerSec * wfx->nChannels &&
	   wfx->nBlockAlign == 2 * wfx->nChannels &&
	   wfx->wBitsPerSample == 16)
	   return hi;
	break;
    case WAVE_FORMAT_IMA_ADPCM:
	if(3 > wfx->nChannels &&
	   wfx->nChannels > 0 &&
	   wfx->wBitsPerSample == 4 &&
	   wfx->cbSize == 2)
	   return hi;
	break;
    }

    return 0xFFFFFFFF;
}

static void     init_wfx_ima_adpcm(IMAADPCMWAVEFORMAT* awfx/*, DWORD nba*/)
{
    WAVEFORMATEX* pwfx = &awfx->wfx;

    /* we assume wFormatTag, nChannels, nSamplesPerSec and wBitsPerSample
     * have been initialized... */

    if (pwfx->wFormatTag != WAVE_FORMAT_IMA_ADPCM) {FIXME("wrong FT\n"); return;}
    if (ADPCM_GetFormatIndex(pwfx) == 0xFFFFFFFF) {FIXME("wrong fmt\n"); return;}

    switch (pwfx->nSamplesPerSec)
    {
    case  8000: pwfx->nBlockAlign = 256 * pwfx->nChannels;   break;
    case 11025: pwfx->nBlockAlign = 256 * pwfx->nChannels;   break;
    case 22050: pwfx->nBlockAlign = 512 * pwfx->nChannels;   break;
    case 44100: pwfx->nBlockAlign = 1024 * pwfx->nChannels;  break;
    default: /*pwfx->nBlockAlign = nba;*/  break;
    }
    pwfx->cbSize = sizeof(WORD);

    awfx->wSamplesPerBlock = (pwfx->nBlockAlign - (4 * pwfx->nChannels)) * (2 / pwfx->nChannels) + 1;
    pwfx->nAvgBytesPerSec = (pwfx->nSamplesPerSec * pwfx->nBlockAlign) / awfx->wSamplesPerBlock;
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

/***********************************************************************
 *           W8
 *
 * Write a 8 bit sample
 */
static inline void W8(unsigned char* dst, short s)
{
    dst[0] = (unsigned char)((s + 32768) >> 8);
}


static inline void W8_16(unsigned char* dst, short s, int bytes)
{
    if(bytes == 1)
        W8(dst, s);
    else
        W16(dst, s);
}

/* IMA (or DVI) APDCM codec routines */

static const unsigned IMA_StepTable[89] =
{
    7, 8, 9, 10, 11, 12, 13, 14,
    16, 17, 19, 21, 23, 25, 28, 31,
    34, 37, 41, 45, 50, 55, 60, 66,
    73, 80, 88, 97, 107, 118, 130, 143,
    157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658,
    724, 796, 876, 963, 1060, 1166, 1282, 1411,
    1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
    3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
    7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
    32767
};

static const int IMA_IndexTable[16] =
{
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};

static inline void clamp_step_index(int* stepIndex)
{
    if (*stepIndex < 0 ) *stepIndex = 0;
    if (*stepIndex > 88) *stepIndex = 88;
}

static inline void clamp_sample(int* sample)
{
    if (*sample < -32768) *sample = -32768;
    if (*sample >  32767) *sample =  32767;
}

static inline void process_nibble(unsigned char code, int* stepIndex, int* sample)
{
    unsigned step;
    int diff;

    code &= 0x0F;

    step = IMA_StepTable[*stepIndex];
    diff = step >> 3;
    if (code & 1) diff += step >> 2;
    if (code & 2) diff += step >> 1;
    if (code & 4) diff += step;
    if (code & 8)	*sample -= diff;
    else 		*sample += diff;
    clamp_sample(sample);
    *stepIndex += IMA_IndexTable[code];
    clamp_step_index(stepIndex);
}

static inline unsigned char generate_nibble(int in, int* stepIndex, int* sample)
{
    int effdiff, diff = in - *sample;
    unsigned step;
    unsigned char code;

    if (diff < 0)
    {
        diff = -diff;
        code = 8;
    }
    else
    {
        code = 0;
    }

    step = IMA_StepTable[*stepIndex];
    effdiff = (step >> 3);
    if (diff >= step)
    {
        code |= 4;
        diff -= step;
        effdiff += step;
    }
    step >>= 1;
    if (diff >= step)
    {
        code |= 2;
        diff -= step;
        effdiff += step;
    }
    step >>= 1;
    if (diff >= step)
    {
        code |= 1;
        effdiff += step;
    }
    if (code & 8)       *sample -= effdiff;
    else                *sample += effdiff;
    clamp_sample(sample);
    *stepIndex += IMA_IndexTable[code];
    clamp_step_index(stepIndex);
    return code;
}

static	void cvtSSima16K(PACMDRVSTREAMINSTANCE adsi,
                         const unsigned char* src, LPDWORD nsrc,
                         unsigned char* dst, LPDWORD ndst)
{
    int         i;
    int         sampleL, sampleR;
    int		stepIndexL, stepIndexR;
    int		nsamp_blk = ((LPIMAADPCMWAVEFORMAT)adsi->pwfxSrc)->wSamplesPerBlock;
    int		nsamp;
    /* compute the number of entire blocks we can decode...
     * it's the min of the number of entire blocks in source buffer and the number
     * of entire blocks in destination buffer
     */
    DWORD	nblock = min(*nsrc / adsi->pwfxSrc->nBlockAlign,
                             *ndst / (nsamp_blk * 2 * 2));

    *nsrc = nblock * adsi->pwfxSrc->nBlockAlign;
    *ndst = nblock * (nsamp_blk * 2 * 2);

    nsamp_blk--; /* remove the sample in block header */
    for (; nblock > 0; nblock--)
    {
        const unsigned char* in_src = src;

	/* handle headers first */
	sampleL = R16(src);
	stepIndexL = (unsigned)*(src + 2);
        clamp_step_index(&stepIndexL);
	src += 4;
	W16(dst, sampleL);	dst += 2;

	sampleR = R16(src);
	stepIndexR = (unsigned)*(src + 2);
        clamp_step_index(&stepIndexR);
	src += 4;
	W16(dst, sampleR);	dst += 2;

	for (nsamp = nsamp_blk; nsamp > 0; nsamp -= 8)
        {
            for (i = 0; i < 4; i++)
            {
                process_nibble(*src, &stepIndexL, &sampleL);
                W16(dst + (2 * i + 0) * 4 + 0, sampleL);
                process_nibble(*src++ >> 4, &stepIndexL, &sampleL);
                W16(dst + (2 * i + 1) * 4 + 0, sampleL);
            }
            for (i = 0; i < 4; i++)
            {
                process_nibble(*src , &stepIndexR, &sampleR);
                W16(dst + (2 * i + 0) * 4 + 2, sampleR);
                process_nibble(*src++ >>4, &stepIndexR, &sampleR);
                W16(dst + (2 * i + 1) * 4 + 2, sampleR);
            }
            dst += 32;
        }
        /* we have now to realign the source pointer on block */
        src = in_src + adsi->pwfxSrc->nBlockAlign;
    }
}

static void cvtMMimaK(PACMDRVSTREAMINSTANCE adsi,
                    const unsigned char* src, LPDWORD nsrc,
                    unsigned char* dst, LPDWORD ndst)
{
    int sample;
    int stepIndex;
    int nsamp_blk = ((LPIMAADPCMWAVEFORMAT)adsi->pwfxSrc)->wSamplesPerBlock;
    int nsamp;
    int bytesPerSample = adsi->pwfxDst->wBitsPerSample / 8;
    /* compute the number of entire blocks we can decode...
     * it's the min of the number of entire blocks in source buffer and the number
     * of entire blocks in destination buffer
     */
    DWORD nblock = min(*nsrc / adsi->pwfxSrc->nBlockAlign, *ndst / (nsamp_blk * bytesPerSample));

    *nsrc = nblock * adsi->pwfxSrc->nBlockAlign;
    *ndst = nblock * nsamp_blk * bytesPerSample;

    nsamp_blk--; /* remove the sample in block header */
    for (; nblock > 0; nblock--)
    {
        const unsigned char* in_src = src;

        /* handle header first */
        sample = R16(src);
        stepIndex = (unsigned)*(src + 2);
        clamp_step_index(&stepIndex);
        src += 4;
        W8_16(dst, sample, bytesPerSample); dst += bytesPerSample;

        for (nsamp = nsamp_blk; nsamp > 0; nsamp -= 2)
        {
            process_nibble(*src, &stepIndex, &sample);
            W8_16(dst, sample, bytesPerSample); dst += bytesPerSample;
            process_nibble(*src++ >> 4, &stepIndex, &sample);
            W8_16(dst, sample, bytesPerSample); dst += bytesPerSample;
        }
        /* we have now to realign the source pointer on block */
        src = in_src + adsi->pwfxSrc->nBlockAlign;
    }
}

static	void cvtSS16imaK(PACMDRVSTREAMINSTANCE adsi,
                         const unsigned char* src, LPDWORD nsrc,
                         unsigned char* dst, LPDWORD ndst)
{
    int		stepIndexL, stepIndexR;
    int		sampleL, sampleR;
    BYTE 	code1, code2;
    int		nsamp_blk = ((LPIMAADPCMWAVEFORMAT)adsi->pwfxDst)->wSamplesPerBlock;
    int		i, nsamp;
    /* compute the number of entire blocks we can decode...
     * it's the min of the number of entire blocks in source buffer and the number
     * of entire blocks in destination buffer
     */
    DWORD	nblock = min(*nsrc / (nsamp_blk * 2 * 2),
                             *ndst / adsi->pwfxDst->nBlockAlign);

    *nsrc = nblock * (nsamp_blk * 2 * 2);
    *ndst = nblock * adsi->pwfxDst->nBlockAlign;

    stepIndexL = ((AcmAdpcmData*)adsi->dwDriver)->stepIndexL;
    stepIndexR = ((AcmAdpcmData*)adsi->dwDriver)->stepIndexR;

    nsamp_blk--; /* so that we won't count the sample in header while filling the block */

    for (; nblock > 0; nblock--)
    {
        unsigned char*   in_dst = dst;

        /* generate header */
    	sampleL = R16(src);  src += 2;
	W16(dst, sampleL); dst += 2;
	W16(dst, stepIndexL); dst += 2;

    	sampleR = R16(src); src += 2;
	W16(dst, sampleR); dst += 2;
	W16(dst, stepIndexR); dst += 2;

	for (nsamp = nsamp_blk; nsamp > 0; nsamp -= 8)
        {
            for (i = 0; i < 4; i++)
            {
                code1 = generate_nibble(R16(src + (4 * i + 0) * 2),
                                        &stepIndexL, &sampleL);
                code2 = generate_nibble(R16(src + (4 * i + 2) * 2),
                                        &stepIndexL, &sampleL);
                *dst++ = (code2 << 4) | code1;
            }
            for (i = 0; i < 4; i++)
            {
                code1 = generate_nibble(R16(src + (4 * i + 1) * 2),
                                        &stepIndexR, &sampleR);
                code2 = generate_nibble(R16(src + (4 * i + 3) * 2),
                                        &stepIndexR, &sampleR);
                *dst++ = (code2 << 4) | code1;
            }
            src += 32;
	}
	dst = in_dst + adsi->pwfxDst->nBlockAlign;
    }
    ((AcmAdpcmData*)adsi->dwDriver)->stepIndexL = stepIndexL;
    ((AcmAdpcmData*)adsi->dwDriver)->stepIndexR = stepIndexR;
}

static	void cvtMM16imaK(PACMDRVSTREAMINSTANCE adsi,
                         const unsigned char* src, LPDWORD nsrc,
                         unsigned char* dst, LPDWORD ndst)
{
    int		stepIndex;
    int		sample;
    BYTE 	code1, code2;
    int		nsamp_blk = ((LPIMAADPCMWAVEFORMAT)adsi->pwfxDst)->wSamplesPerBlock;
    int		nsamp;
    /* compute the number of entire blocks we can decode...
     * it's the min of the number of entire blocks in source buffer and the number
     * of entire blocks in destination buffer
     */
    DWORD	nblock = min(*nsrc / (nsamp_blk * 2),
                             *ndst / adsi->pwfxDst->nBlockAlign);

    *nsrc = nblock * (nsamp_blk * 2);
    *ndst = nblock * adsi->pwfxDst->nBlockAlign;

    stepIndex = ((AcmAdpcmData*)adsi->dwDriver)->stepIndexL;
    nsamp_blk--; /* so that we won't count the sample in header while filling the block */

    for (; nblock > 0; nblock--)
    {
        unsigned char*   in_dst = dst;

        /* generate header */
        /* FIXME: what about the last effective sample from previous block ??? */
        /* perhaps something like:
         *      sample += R16(src);
         *      clamp_sample(sample);
         * and with :
         *      + saving the sample in adsi->dwDriver when all blocks are done
         +      + reset should set the field in adsi->dwDriver to 0 too
         */
    	sample = R16(src); src += 2;
	W16(dst, sample); dst += 2;
	*dst = (unsigned char)(unsigned)stepIndex;
	dst += 2;

	for (nsamp = nsamp_blk; nsamp > 0; nsamp -= 2)
        {
            code1 = generate_nibble(R16(src), &stepIndex, &sample);
            src += 2;
            code2 = generate_nibble(R16(src), &stepIndex, &sample);
            src += 2;
            *dst++ = (code2 << 4) | code1;
	}
	dst = in_dst + adsi->pwfxDst->nBlockAlign;
    }
    ((AcmAdpcmData*)adsi->dwDriver)->stepIndexL = stepIndex;
}

/***********************************************************************
 *           ADPCM_DriverDetails
 *
 */
static	LRESULT ADPCM_DriverDetails(PACMDRIVERDETAILSW add)
{
    add->fccType = ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC;
    add->fccComp = ACMDRIVERDETAILS_FCCCOMP_UNDEFINED;
    add->wMid = MM_MICROSOFT;
    add->wPid = MM_MSFT_ACM_IMAADPCM;
    add->vdwACM = 0x3320000;
    add->vdwDriver = 0x04000000;
    add->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CODEC;
    add->cFormatTags = 2; /* PCM, IMA ADPCM */
    add->cFilterTags = 0;
    add->hicon = NULL;
    MultiByteToWideChar( CP_ACP, 0, "Microsoft IMA ADPCM", -1,
                         add->szShortName, ARRAY_SIZE(add->szShortName) );
    MultiByteToWideChar( CP_ACP, 0, "Microsoft IMA ADPCM CODEC", -1,
                         add->szLongName, ARRAY_SIZE(add->szLongName) );
    MultiByteToWideChar( CP_ACP, 0, "Brought to you by the Wine team...", -1,
                         add->szCopyright, ARRAY_SIZE(add->szCopyright) );
    MultiByteToWideChar( CP_ACP, 0, "Refer to LICENSE file", -1,
                         add->szLicensing, ARRAY_SIZE(add->szLicensing) );
    add->szFeatures[0] = 0;

    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           ADPCM_FormatTagDetails
 *
 */
static	LRESULT	ADPCM_FormatTagDetails(PACMFORMATTAGDETAILSW aftd, DWORD dwQuery)
{
    switch (dwQuery)
    {
    case ACM_FORMATTAGDETAILSF_INDEX:
	if (aftd->dwFormatTagIndex >= 2) return ACMERR_NOTPOSSIBLE;
	break;
    case ACM_FORMATTAGDETAILSF_LARGESTSIZE:
	if (aftd->dwFormatTag == WAVE_FORMAT_UNKNOWN)
        {
            aftd->dwFormatTagIndex = 1; /* WAVE_FORMAT_IMA_ADPCM is bigger than PCM */
	    break;
	}
	/* fall through */
    case ACM_FORMATTAGDETAILSF_FORMATTAG:
	switch (aftd->dwFormatTag)
        {
	case WAVE_FORMAT_PCM:		aftd->dwFormatTagIndex = 0; break;
	case WAVE_FORMAT_IMA_ADPCM:     aftd->dwFormatTagIndex = 1; break;
	default:			return ACMERR_NOTPOSSIBLE;
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
	aftd->dwFormatTag = WAVE_FORMAT_IMA_ADPCM;
	aftd->cbFormatSize = sizeof(IMAADPCMWAVEFORMAT);
	aftd->cStandardFormats = ARRAY_SIZE(ADPCM_Formats);
        lstrcpyW(aftd->szFormatTag, L"IMA ADPCM");
	break;
    }
    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           ADPCM_FormatDetails
 *
 */
static	LRESULT	ADPCM_FormatDetails(PACMFORMATDETAILSW afd, DWORD dwQuery)
{
    switch (dwQuery)
    {
    case ACM_FORMATDETAILSF_FORMAT:
	if (ADPCM_GetFormatIndex(afd->pwfx) == 0xFFFFFFFF) return ACMERR_NOTPOSSIBLE;
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
	    /* native MSACM uses a PCMWAVEFORMAT structure, so cbSize is not accessible
	     * afd->pwfx->cbSize = 0;
	     */
	    afd->pwfx->nBlockAlign =
		(afd->pwfx->nChannels * afd->pwfx->wBitsPerSample) / 8;
	    afd->pwfx->nAvgBytesPerSec =
		afd->pwfx->nSamplesPerSec * afd->pwfx->nBlockAlign;
	    break;
	case WAVE_FORMAT_IMA_ADPCM:
	    if (afd->dwFormatIndex >= ARRAY_SIZE(ADPCM_Formats)) return ACMERR_NOTPOSSIBLE;
	    afd->pwfx->nChannels = ADPCM_Formats[afd->dwFormatIndex].nChannels;
	    afd->pwfx->nSamplesPerSec = ADPCM_Formats[afd->dwFormatIndex].rate;
	    afd->pwfx->wBitsPerSample = ADPCM_Formats[afd->dwFormatIndex].nBits;
	    init_wfx_ima_adpcm((IMAADPCMWAVEFORMAT *)afd->pwfx);
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
 *           ADPCM_FormatSuggest
 *
 */
static	LRESULT	ADPCM_FormatSuggest(PACMDRVFORMATSUGGEST adfs)
{
    /* some tests ... */
    if (adfs->cbwfxSrc < sizeof(PCMWAVEFORMAT) ||
	adfs->cbwfxDst < sizeof(PCMWAVEFORMAT) ||
	adfs->pwfxSrc->wFormatTag == adfs->pwfxDst->wFormatTag ||
	ADPCM_GetFormatIndex(adfs->pwfxSrc) == 0xFFFFFFFF) return ACMERR_NOTPOSSIBLE;

    /* If no suggestion for destination, then copy source value */
    if (!(adfs->fdwSuggest & ACM_FORMATSUGGESTF_NCHANNELS))
	adfs->pwfxDst->nChannels = adfs->pwfxSrc->nChannels;
    if (!(adfs->fdwSuggest & ACM_FORMATSUGGESTF_NSAMPLESPERSEC))
        adfs->pwfxDst->nSamplesPerSec = adfs->pwfxSrc->nSamplesPerSec;

    if (!(adfs->fdwSuggest & ACM_FORMATSUGGESTF_WBITSPERSAMPLE))
    {
	if (adfs->pwfxSrc->wFormatTag == WAVE_FORMAT_PCM)
            adfs->pwfxDst->wBitsPerSample = 4;
        else
            adfs->pwfxDst->wBitsPerSample = 16;
    }
    if (!(adfs->fdwSuggest & ACM_FORMATSUGGESTF_WFORMATTAG))
    {
	if (adfs->pwfxSrc->wFormatTag == WAVE_FORMAT_PCM)
            adfs->pwfxDst->wFormatTag = WAVE_FORMAT_IMA_ADPCM;
        else
            adfs->pwfxDst->wFormatTag = WAVE_FORMAT_PCM;
    }

    /* recompute other values */
    switch (adfs->pwfxDst->wFormatTag)
    {
    case WAVE_FORMAT_PCM:
        if (adfs->cbwfxSrc < sizeof(IMAADPCMWAVEFORMAT)) return ACMERR_NOTPOSSIBLE;
        adfs->pwfxDst->nBlockAlign = (adfs->pwfxDst->nChannels * adfs->pwfxDst->wBitsPerSample) / 8;
        adfs->pwfxDst->nAvgBytesPerSec = adfs->pwfxDst->nSamplesPerSec * adfs->pwfxDst->nBlockAlign;
        /* check if result is ok */
        if (ADPCM_GetFormatIndex(adfs->pwfxDst) == 0xFFFFFFFF) return ACMERR_NOTPOSSIBLE;
        break;
    case WAVE_FORMAT_IMA_ADPCM:
        if (adfs->cbwfxDst < sizeof(IMAADPCMWAVEFORMAT)) return ACMERR_NOTPOSSIBLE;
        init_wfx_ima_adpcm((IMAADPCMWAVEFORMAT*)adfs->pwfxDst);
        /* FIXME: not handling header overhead */
        TRACE("setting spb=%u\n", ((IMAADPCMWAVEFORMAT*)adfs->pwfxDst)->wSamplesPerBlock);
        /* check if result is ok */
        if (ADPCM_GetFormatIndex(adfs->pwfxDst) == 0xFFFFFFFF) return ACMERR_NOTPOSSIBLE;
        break;
    default:
        return ACMERR_NOTPOSSIBLE;
    }

    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           ADPCM_Reset
 *
 */
static	void	ADPCM_Reset(PACMDRVSTREAMINSTANCE adsi, AcmAdpcmData* aad)
{
    aad->stepIndexL = aad->stepIndexR = 0;
}

/***********************************************************************
 *           ADPCM_StreamOpen
 *
 */
static	LRESULT	ADPCM_StreamOpen(PACMDRVSTREAMINSTANCE adsi)
{
    AcmAdpcmData*	aad;
    unsigned            nspb;

    assert(!(adsi->fdwOpen & ACM_STREAMOPENF_ASYNC));

    if (ADPCM_GetFormatIndex(adsi->pwfxSrc) == 0xFFFFFFFF ||
	ADPCM_GetFormatIndex(adsi->pwfxDst) == 0xFFFFFFFF)
	return ACMERR_NOTPOSSIBLE;

    aad = HeapAlloc(GetProcessHeap(), 0, sizeof(AcmAdpcmData));
    if (aad == 0) return MMSYSERR_NOMEM;

    adsi->dwDriver = (DWORD_PTR)aad;

    if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_PCM &&
	adsi->pwfxDst->wFormatTag == WAVE_FORMAT_PCM)
    {
	goto theEnd;
    }
    else if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_IMA_ADPCM &&
             adsi->pwfxDst->wFormatTag == WAVE_FORMAT_PCM)
    {
        /* resampling or mono <=> stereo not available
         * ADPCM algo only define 16 bit per sample output
         * (The API seems to still allow 8 bit per sample output)
         */
        if (adsi->pwfxSrc->nSamplesPerSec != adsi->pwfxDst->nSamplesPerSec ||
            adsi->pwfxSrc->nChannels != adsi->pwfxDst->nChannels ||
            (adsi->pwfxDst->wBitsPerSample != 16 && adsi->pwfxDst->wBitsPerSample != 8))
            goto theEnd;

        nspb = ((LPIMAADPCMWAVEFORMAT)adsi->pwfxSrc)->wSamplesPerBlock;
        TRACE("spb=%u\n", nspb);

        /* we check that in a block, after the header, samples are present on
         * 4-sample packet pattern
         * we also check that the block alignment is bigger than the expected size
         */
        if (((nspb - 1) & 3) != 0) goto theEnd;
        if ((((nspb - 1) / 2) + 4) * adsi->pwfxSrc->nChannels < adsi->pwfxSrc->nBlockAlign)
            goto theEnd;

        /* adpcm decoding... */
        if (adsi->pwfxDst->wBitsPerSample == 16 && adsi->pwfxDst->nChannels == 2)
            aad->convert = cvtSSima16K;
        if (adsi->pwfxDst->wBitsPerSample == 16 && adsi->pwfxDst->nChannels == 1)
            aad->convert = cvtMMimaK;
        if (adsi->pwfxDst->wBitsPerSample == 8 && adsi->pwfxDst->nChannels == 1)
            aad->convert = cvtMMimaK;
        /* FIXME: Stereo support for 8bit samples*/
        if (adsi->pwfxDst->wBitsPerSample == 8 && adsi->pwfxDst->nChannels == 2)
            goto theEnd;
    }
    else if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_PCM &&
             adsi->pwfxDst->wFormatTag == WAVE_FORMAT_IMA_ADPCM)
    {
	if (adsi->pwfxSrc->nSamplesPerSec != adsi->pwfxDst->nSamplesPerSec ||
	    adsi->pwfxSrc->nChannels != adsi->pwfxDst->nChannels ||
            adsi->pwfxSrc->wBitsPerSample != 16)
	    goto theEnd;

        nspb = ((LPIMAADPCMWAVEFORMAT)adsi->pwfxDst)->wSamplesPerBlock;
        TRACE("spb=%u\n", nspb);

        /* we check that in a block, after the header, samples are present on
         * 4-sample packet pattern
         * we also check that the block alignment is bigger than the expected size
         */
        if (((nspb - 1) & 3) != 0) goto theEnd;
        if ((((nspb - 1) / 2) + 4) * adsi->pwfxDst->nChannels < adsi->pwfxDst->nBlockAlign)
            goto theEnd;

	/* adpcm coding... */
	if (adsi->pwfxSrc->wBitsPerSample == 16 && adsi->pwfxSrc->nChannels == 2)
	    aad->convert = cvtSS16imaK;
	if (adsi->pwfxSrc->wBitsPerSample == 16 && adsi->pwfxSrc->nChannels == 1)
	    aad->convert = cvtMM16imaK;
    }
    else goto theEnd;
    ADPCM_Reset(adsi, aad);

    return MMSYSERR_NOERROR;

 theEnd:
    HeapFree(GetProcessHeap(), 0, aad);
    adsi->dwDriver = 0L;
    return MMSYSERR_NOTSUPPORTED;
}

/***********************************************************************
 *           ADPCM_StreamClose
 *
 */
static	LRESULT	ADPCM_StreamClose(PACMDRVSTREAMINSTANCE adsi)
{
    HeapFree(GetProcessHeap(), 0, (void*)adsi->dwDriver);
    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           ADPCM_StreamSize
 *
 */
static	LRESULT ADPCM_StreamSize(const ACMDRVSTREAMINSTANCE *adsi, PACMDRVSTREAMSIZE adss)
{
    DWORD nblocks;

    switch (adss->fdwSize)
    {
    case ACM_STREAMSIZEF_DESTINATION:
	/* cbDstLength => cbSrcLength */
	if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_PCM &&
	    adsi->pwfxDst->wFormatTag == WAVE_FORMAT_IMA_ADPCM)
        {
            nblocks = adss->cbDstLength / adsi->pwfxDst->nBlockAlign;
            if (nblocks == 0)
                return ACMERR_NOTPOSSIBLE;
            adss->cbSrcLength = nblocks * adsi->pwfxSrc->nBlockAlign * ((IMAADPCMWAVEFORMAT*)adsi->pwfxDst)->wSamplesPerBlock;
	}
        else if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_IMA_ADPCM &&
                 adsi->pwfxDst->wFormatTag == WAVE_FORMAT_PCM)
        {
            nblocks = adss->cbDstLength / (adsi->pwfxDst->nBlockAlign * ((IMAADPCMWAVEFORMAT*)adsi->pwfxSrc)->wSamplesPerBlock);
            if (nblocks == 0)
                return ACMERR_NOTPOSSIBLE;
            adss->cbSrcLength = nblocks * adsi->pwfxSrc->nBlockAlign;
	}
        else
        {
	    return MMSYSERR_NOTSUPPORTED;
	}
	break;
    case ACM_STREAMSIZEF_SOURCE:
	/* cbSrcLength => cbDstLength */
	if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_PCM &&
	    adsi->pwfxDst->wFormatTag == WAVE_FORMAT_IMA_ADPCM)
        {
            nblocks = adss->cbSrcLength / (adsi->pwfxSrc->nBlockAlign * ((IMAADPCMWAVEFORMAT*)adsi->pwfxDst)->wSamplesPerBlock);
            if (nblocks == 0)
                return ACMERR_NOTPOSSIBLE;
            if (adss->cbSrcLength % (adsi->pwfxSrc->nBlockAlign * ((IMAADPCMWAVEFORMAT*)adsi->pwfxDst)->wSamplesPerBlock))
                /* Round block count up. */
                nblocks++;
            adss->cbDstLength = nblocks * adsi->pwfxDst->nBlockAlign;
	}
        else if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_IMA_ADPCM &&
                 adsi->pwfxDst->wFormatTag == WAVE_FORMAT_PCM)
        {
            nblocks = adss->cbSrcLength / adsi->pwfxSrc->nBlockAlign;
            if (nblocks == 0)
                return ACMERR_NOTPOSSIBLE;
            if (adss->cbSrcLength % adsi->pwfxSrc->nBlockAlign)
                /* Round block count up. */
                nblocks++;
            adss->cbDstLength = nblocks * adsi->pwfxDst->nBlockAlign * ((IMAADPCMWAVEFORMAT*)adsi->pwfxSrc)->wSamplesPerBlock;
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
 *           ADPCM_StreamConvert
 *
 */
static LRESULT ADPCM_StreamConvert(PACMDRVSTREAMINSTANCE adsi, PACMDRVSTREAMHEADER adsh)
{
    AcmAdpcmData*	aad = (AcmAdpcmData*)adsi->dwDriver;
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
	ADPCM_Reset(adsi, aad);
    }

    aad->convert(adsi, adsh->pbSrc, &nsrc, adsh->pbDst, &ndst);
    adsh->cbSrcLengthUsed = nsrc;
    adsh->cbDstLengthUsed = ndst;

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			ADPCM_DriverProc			[exported]
 */
LRESULT CALLBACK ADPCM_DriverProc(DWORD_PTR dwDevID, HDRVR hDriv, UINT wMsg,
					 LPARAM dwParam1, LPARAM dwParam2)
{
    TRACE("(%08Ix %p %04x %08Ix %08Ix);\n",
	  dwDevID, hDriv, wMsg, dwParam1, dwParam2);

    switch (wMsg)
    {
    case DRV_LOAD:		return 1;
    case DRV_FREE:		return 1;
    case DRV_OPEN:		return 1;
    case DRV_CLOSE:		return ADPCM_drvClose(dwDevID);
    case DRV_ENABLE:		return 1;
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBoxA(0, "MSACM IMA ADPCM filter !", "Wine Driver", MB_OK); return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;

    case ACMDM_DRIVER_NOTIFY:
	/* no caching from other ACM drivers is done so far */
	return MMSYSERR_NOERROR;

    case ACMDM_DRIVER_DETAILS:
	return ADPCM_DriverDetails((PACMDRIVERDETAILSW)dwParam1);

    case ACMDM_FORMATTAG_DETAILS:
	return ADPCM_FormatTagDetails((PACMFORMATTAGDETAILSW)dwParam1, dwParam2);

    case ACMDM_FORMAT_DETAILS:
	return ADPCM_FormatDetails((PACMFORMATDETAILSW)dwParam1, dwParam2);

    case ACMDM_FORMAT_SUGGEST:
	return ADPCM_FormatSuggest((PACMDRVFORMATSUGGEST)dwParam1);

    case ACMDM_STREAM_OPEN:
	return ADPCM_StreamOpen((PACMDRVSTREAMINSTANCE)dwParam1);

    case ACMDM_STREAM_CLOSE:
	return ADPCM_StreamClose((PACMDRVSTREAMINSTANCE)dwParam1);

    case ACMDM_STREAM_SIZE:
	return ADPCM_StreamSize((PACMDRVSTREAMINSTANCE)dwParam1, (PACMDRVSTREAMSIZE)dwParam2);

    case ACMDM_STREAM_CONVERT:
	return ADPCM_StreamConvert((PACMDRVSTREAMINSTANCE)dwParam1, (PACMDRVSTREAMHEADER)dwParam2);

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
