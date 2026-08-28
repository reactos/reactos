/*
 * MS ADPCM handling
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

/* see http://www.pcisys.net/~melanson/codecs/adpcm.txt for the details */

WINE_DEFAULT_DEBUG_CHANNEL(adpcm);

/***********************************************************************
 *           ADPCM_drvOpen
 */
static LRESULT ADPCM_drvOpen(LPCSTR str)
{
    return 1;
}

/***********************************************************************
 *           ADPCM_drvClose
 */
static LRESULT ADPCM_drvClose(DWORD_PTR dwDevID)
{
    return 1;
}

typedef struct tagAcmAdpcmData
{
    void (*convert)(const ACMDRVSTREAMINSTANCE *adsi,
		    const unsigned char*, LPDWORD, unsigned char*, LPDWORD);
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

static int MS_Delta[] =
{
    230, 230, 230, 230, 307, 409, 512, 614,
    768, 614, 512, 409, 307, 230, 230, 230
};


static ADPCMCOEFSET MSADPCM_CoeffSet[] =
{
    {256, 0}, {512, -256}, {0, 0}, {192, 64}, {240, 0}, {460, -208}, {392, -232}
};

/***********************************************************************
 *           ADPCM_GetFormatIndex
 */
static	DWORD	ADPCM_GetFormatIndex(const WAVEFORMATEX* wfx)
{
    int             i, hi;
    const Format*   fmts;

    switch (wfx->wFormatTag)
    {
    case WAVE_FORMAT_PCM:
	hi = ARRAY_SIZE(PCM_Formats);
	fmts = PCM_Formats;
	break;
    case WAVE_FORMAT_ADPCM:
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
    case WAVE_FORMAT_ADPCM:
	if(3 > wfx->nChannels &&
	   wfx->nChannels > 0 &&
	   wfx->wBitsPerSample == 4 &&
	   wfx->cbSize == 32)
	   return hi;
	break;
    }

    return 0xFFFFFFFF;
}

static void     init_wfx_adpcm(ADPCMWAVEFORMAT* awfx)
{
    register WAVEFORMATEX*      pwfx = &awfx->wfx;

    /* we assume wFormatTag, nChannels, nSamplesPerSec and wBitsPerSample
     * have been initialized... */

    if (pwfx->wFormatTag != WAVE_FORMAT_ADPCM) {FIXME("wrong FT\n"); return;}
    if (ADPCM_GetFormatIndex(pwfx) == 0xFFFFFFFF) {FIXME("wrong fmt\n"); return;}

    switch (pwfx->nSamplesPerSec)
    {
    case  8000: pwfx->nBlockAlign = 256 * pwfx->nChannels;   break;
    case 11025: pwfx->nBlockAlign = 256 * pwfx->nChannels;   break;
    case 22050: pwfx->nBlockAlign = 512 * pwfx->nChannels;   break;
    case 44100: pwfx->nBlockAlign = 1024 * pwfx->nChannels;  break;
    default:                               break;
    }
    pwfx->cbSize = 2 * sizeof(WORD) + 7 * sizeof(ADPCMCOEFSET);
    /* 7 is the size of the block head (which contains two samples) */

    awfx->wSamplesPerBlock = pwfx->nBlockAlign * 2 / pwfx->nChannels - 12;
    pwfx->nAvgBytesPerSec = (pwfx->nSamplesPerSec * pwfx->nBlockAlign) / awfx->wSamplesPerBlock;
    awfx->wNumCoef = 7;
    memcpy(awfx->aCoef, MSADPCM_CoeffSet, 7 * sizeof(ADPCMCOEFSET));
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

static inline void clamp_sample(int* sample)
{
    if (*sample < -32768) *sample = -32768;
    if (*sample >  32767) *sample =  32767;
}

static inline void process_nibble(unsigned nibble, int* idelta,
                                  int* sample1, int* sample2,
                                  const ADPCMCOEFSET* coeff)
{
    int sample;
    int snibble;

    /* nibble is in fact a signed 4 bit integer => propagate sign if needed */
    snibble = (nibble & 0x08) ? (nibble - 16) : nibble;
    sample = ((*sample1 * coeff->iCoef1) + (*sample2 * coeff->iCoef2)) / 256 +
        snibble * *idelta;
    clamp_sample(&sample);

    *sample2 = *sample1;
    *sample1 = sample;
    *idelta = ((MS_Delta[nibble] * *idelta) / 256);
    if (*idelta < 16) *idelta = 16;
}

static inline unsigned char C168(short s)
{
    return HIBYTE(s) ^ (unsigned char)0x80;
}

static	void cvtSSms16K(const ACMDRVSTREAMINSTANCE *adsi,
                        const unsigned char* src, LPDWORD nsrc,
                        unsigned char* dst, LPDWORD ndst)
{
    int                 ideltaL, ideltaR;
    int                 sample1L, sample2L;
    int                 sample1R, sample2R;
    ADPCMCOEFSET        coeffL, coeffR;
    int                 nsamp;
    int		        nsamp_blk = ((ADPCMWAVEFORMAT*)adsi->pwfxSrc)->wSamplesPerBlock;
    DWORD	        nblock = min(*nsrc / adsi->pwfxSrc->nBlockAlign,
                                     *ndst / (nsamp_blk * adsi->pwfxDst->nBlockAlign));

    *nsrc = nblock * adsi->pwfxSrc->nBlockAlign;
    *ndst = nblock * nsamp_blk * adsi->pwfxDst->nBlockAlign;

    nsamp_blk -= 2; /* see below for samples from block head */
    for (; nblock > 0; nblock--)
    {
        const unsigned char*    in_src = src;

        /* Catch a problem from Tomb Raider III (bug 21000) where it passes
         * invalid data after a valid sequence of blocks */
        if (*src > 6 || *(src + 1) > 6)
        {
            /* Recalculate the amount of used output buffer. We are not changing
             * nsrc, let's assume the bad data was parsed */
            *ndst -= nblock * nsamp_blk * adsi->pwfxDst->nBlockAlign;
            WARN("Invalid ADPCM data, stopping conversion\n");
            break;
        }
        coeffL = MSADPCM_CoeffSet[*src++];
        coeffR = MSADPCM_CoeffSet[*src++];

        ideltaL  = R16(src);    src += 2;
        ideltaR  = R16(src);    src += 2;
        sample1L = R16(src);    src += 2;
        sample1R = R16(src);    src += 2;
        sample2L = R16(src);    src += 2;
        sample2R = R16(src);    src += 2;

        if(adsi->pwfxDst->wBitsPerSample == 8){
            /* store samples from block head */
            *dst = C168(sample2L);      ++dst;
            *dst = C168(sample2R);      ++dst;
            *dst = C168(sample1L);      ++dst;
            *dst = C168(sample1R);      ++dst;

            for (nsamp = nsamp_blk; nsamp > 0; nsamp--)
            {
                process_nibble(*src >> 4, &ideltaL, &sample1L, &sample2L, &coeffL);
                *dst = C168(sample1L); ++dst;
                process_nibble(*src++ & 0x0F, &ideltaR, &sample1R, &sample2R, &coeffR);
                *dst = C168(sample1R); ++dst;
            }
        }else if(adsi->pwfxDst->wBitsPerSample == 16){
            /* store samples from block head */
            W16(dst, sample2L);      dst += 2;
            W16(dst, sample2R);      dst += 2;
            W16(dst, sample1L);      dst += 2;
            W16(dst, sample1R);      dst += 2;

            for (nsamp = nsamp_blk; nsamp > 0; nsamp--)
            {
                process_nibble(*src >> 4, &ideltaL, &sample1L, &sample2L, &coeffL);
                W16(dst, sample1L); dst += 2;
                process_nibble(*src++ & 0x0F, &ideltaR, &sample1R, &sample2R, &coeffR);
                W16(dst, sample1R); dst += 2;
            }
        }
        src = in_src + adsi->pwfxSrc->nBlockAlign;
    }
}

static	void cvtMMms16K(const ACMDRVSTREAMINSTANCE *adsi,
                        const unsigned char* src, LPDWORD nsrc,
                        unsigned char* dst, LPDWORD ndst)
{
    int                 idelta;
    int                 sample1, sample2;
    ADPCMCOEFSET        coeff;
    int                 nsamp;
    int		        nsamp_blk = ((ADPCMWAVEFORMAT*)adsi->pwfxSrc)->wSamplesPerBlock;
    DWORD	        nblock = min(*nsrc / adsi->pwfxSrc->nBlockAlign,
                                     *ndst / (nsamp_blk * adsi->pwfxDst->nBlockAlign));

    *nsrc = nblock * adsi->pwfxSrc->nBlockAlign;
    *ndst = nblock * nsamp_blk * adsi->pwfxDst->nBlockAlign;

    nsamp_blk -= 2; /* see below for samples from block head */
    for (; nblock > 0; nblock--)
    {
        const unsigned char*    in_src = src;

        /* Catch a problem from Lord of the Rings War of the Ring where it
         * passes invalid data. */
        if (*src > 6)
        {
                *ndst -= nblock * nsamp_blk * adsi->pwfxDst->nBlockAlign;
                WARN("Invalid ADPCM data, stopping conversion\n");
                break;
        }
        coeff = MSADPCM_CoeffSet[*src++];

        idelta =  R16(src);     src += 2;
        sample1 = R16(src);     src += 2;
        sample2 = R16(src);     src += 2;

        /* store samples from block head */
        if(adsi->pwfxDst->wBitsPerSample == 8){
            *dst = C168(sample2);    ++dst;
            *dst = C168(sample1);    ++dst;

            for (nsamp = nsamp_blk; nsamp > 0; nsamp -= 2)
            {
                process_nibble(*src >> 4, &idelta, &sample1, &sample2, &coeff);
                *dst = C168(sample1); ++dst;
                process_nibble(*src++ & 0x0F, &idelta, &sample1, &sample2, &coeff);
                *dst = C168(sample1); ++dst;
            }
        }else if(adsi->pwfxDst->wBitsPerSample == 16){
            W16(dst, sample2);      dst += 2;
            W16(dst, sample1);      dst += 2;

            for (nsamp = nsamp_blk; nsamp > 0; nsamp -= 2)
            {
                process_nibble(*src >> 4, &idelta, &sample1, &sample2, &coeff);
                W16(dst, sample1); dst += 2;
                process_nibble(*src++ & 0x0F, &idelta, &sample1, &sample2, &coeff);
                W16(dst, sample1); dst += 2;
            }
        }

        src = in_src + adsi->pwfxSrc->nBlockAlign;
    }
}

#if 0
static	void cvtSS16msK(PACMDRVSTREAMINSTANCE adsi,
                        const unsigned char* src, LPDWORD nsrc,
                        unsigned char* dst, LPDWORD ndst)
{
}

static	void cvtMM16msK(PACMDRVSTREAMINSTANCE adsi,
                        const unsigned char* src, LPDWORD nsrc,
                        unsigned char* dst, LPDWORD ndst)
{
}
#endif

/***********************************************************************
 *           ADPCM_DriverDetails
 *
 */
static	LRESULT ADPCM_DriverDetails(PACMDRIVERDETAILSW add)
{
    add->fccType = ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC;
    add->fccComp = ACMDRIVERDETAILS_FCCCOMP_UNDEFINED;
    add->wMid = MM_MICROSOFT;
    add->wPid = MM_MSFT_ACM_MSADPCM;
    add->vdwACM = 0x01000000;
    add->vdwDriver = 0x01000000;
    add->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CODEC;
    add->cFormatTags = 2; /* PCM, MS ADPCM */
    add->cFilterTags = 0;
    add->hicon = NULL;
    MultiByteToWideChar( CP_ACP, 0, "MS-ADPCM", -1,
                         add->szShortName, ARRAY_SIZE( add->szShortName ));
    MultiByteToWideChar( CP_ACP, 0, "Wine MS ADPCM converter", -1,
                         add->szLongName, ARRAY_SIZE( add->szLongName ));
    MultiByteToWideChar( CP_ACP, 0, "Brought to you by the Wine team...", -1,
                         add->szCopyright, ARRAY_SIZE( add->szCopyright ));
    MultiByteToWideChar( CP_ACP, 0, "Refer to LICENSE file", -1,
                         add->szLicensing, ARRAY_SIZE( add->szLicensing ));
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
            aftd->dwFormatTagIndex = 1; /* WAVE_FORMAT_ADPCM is bigger than PCM */
	    break;
	}
	/* fall through */
    case ACM_FORMATTAGDETAILSF_FORMATTAG:
	switch (aftd->dwFormatTag)
        {
	case WAVE_FORMAT_PCM:	aftd->dwFormatTagIndex = 0; break;
	case WAVE_FORMAT_ADPCM: aftd->dwFormatTagIndex = 1; break;
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
	aftd->dwFormatTag = WAVE_FORMAT_ADPCM;
	aftd->cbFormatSize = sizeof(ADPCMWAVEFORMAT) + (7 - 1) * sizeof(ADPCMCOEFSET);
	aftd->cStandardFormats = ARRAY_SIZE(ADPCM_Formats);
        lstrcpyW(aftd->szFormatTag, L"Microsoft ADPCM");
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
	case WAVE_FORMAT_ADPCM:
	    if (afd->dwFormatIndex >= ARRAY_SIZE(ADPCM_Formats)) return ACMERR_NOTPOSSIBLE;
            if (afd->cbwfx < sizeof(ADPCMWAVEFORMAT) + (7 - 1) * sizeof(ADPCMCOEFSET))
                return ACMERR_NOTPOSSIBLE;
	    afd->pwfx->nChannels = ADPCM_Formats[afd->dwFormatIndex].nChannels;
	    afd->pwfx->nSamplesPerSec = ADPCM_Formats[afd->dwFormatIndex].rate;
	    afd->pwfx->wBitsPerSample = ADPCM_Formats[afd->dwFormatIndex].nBits;
            init_wfx_adpcm((ADPCMWAVEFORMAT*)afd->pwfx);
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
    /* FIXME: should do those tests against the real size (according to format tag */

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
            adfs->pwfxDst->wFormatTag = WAVE_FORMAT_ADPCM;
        else
            adfs->pwfxDst->wFormatTag = WAVE_FORMAT_PCM;
    }

    /* recompute other values */
    switch (adfs->pwfxDst->wFormatTag)
    {
    case WAVE_FORMAT_PCM:
        adfs->pwfxDst->nBlockAlign = (adfs->pwfxDst->nChannels * adfs->pwfxDst->wBitsPerSample) / 8;
        adfs->pwfxDst->nAvgBytesPerSec = adfs->pwfxDst->nSamplesPerSec * adfs->pwfxDst->nBlockAlign;
        /* check if result is ok */
        if (ADPCM_GetFormatIndex(adfs->pwfxDst) == 0xFFFFFFFF) return ACMERR_NOTPOSSIBLE;
        break;
    case WAVE_FORMAT_ADPCM:
        if (adfs->cbwfxDst < sizeof(ADPCMWAVEFORMAT) + (7 - 1) * sizeof(ADPCMCOEFSET))
            return ACMERR_NOTPOSSIBLE;
        init_wfx_adpcm((ADPCMWAVEFORMAT*)adfs->pwfxDst);
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
}

/***********************************************************************
 *           ADPCM_StreamOpen
 *
 */
static	LRESULT	ADPCM_StreamOpen(PACMDRVSTREAMINSTANCE adsi)
{
    AcmAdpcmData*	aad;

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
    else if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_ADPCM &&
             adsi->pwfxDst->wFormatTag == WAVE_FORMAT_PCM)
    {
	/* resampling or mono <=> stereo not available */
	if (adsi->pwfxSrc->nSamplesPerSec != adsi->pwfxDst->nSamplesPerSec ||
	    adsi->pwfxSrc->nChannels != adsi->pwfxDst->nChannels)
	    goto theEnd;

#if 0
        {
            unsigned int nspb = ((IMAADPCMWAVEFORMAT*)adsi->pwfxSrc)->wSamplesPerBlock;
            FIXME("spb=%u\n", nspb);

            /* we check that in a block, after the header, samples are present on
             * 4-sample packet pattern
             * we also check that the block alignment is bigger than the expected size
             */
            if (((nspb - 1) & 3) != 0) goto theEnd;
            if ((((nspb - 1) / 2) + 4) * adsi->pwfxSrc->nChannels < adsi->pwfxSrc->nBlockAlign)
                goto theEnd;
        }
#endif

	/* adpcm decoding... */
	if (adsi->pwfxDst->nChannels == 2)
	    aad->convert = cvtSSms16K;
	else if (adsi->pwfxDst->nChannels == 1)
	    aad->convert = cvtMMms16K;
    }
    else if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_PCM &&
             adsi->pwfxDst->wFormatTag == WAVE_FORMAT_ADPCM)
    {
	if (adsi->pwfxSrc->nSamplesPerSec != adsi->pwfxDst->nSamplesPerSec ||
	    adsi->pwfxSrc->nChannels != adsi->pwfxDst->nChannels ||
            adsi->pwfxSrc->wBitsPerSample != 16)
	    goto theEnd;
#if 0
        nspb = ((IMAADPCMWAVEFORMAT*)adsi->pwfxDst)->wSamplesPerBlock;
        FIXME("spb=%u\n", nspb);

        /* we check that in a block, after the header, samples are present on
         * 4-sample packet pattern
         * we also check that the block alignment is bigger than the expected size
         */
        if (((nspb - 1) & 3) != 0) goto theEnd;
        if ((((nspb - 1) / 2) + 4) * adsi->pwfxDst->nChannels < adsi->pwfxDst->nBlockAlign)
            goto theEnd;
#endif
#if 0
	/* adpcm coding... */
	if (adsi->pwfxSrc->wBitsPerSample == 16 && adsi->pwfxSrc->nChannels == 2)
	    aad->convert = cvtSS16msK;
	if (adsi->pwfxSrc->wBitsPerSample == 16 && adsi->pwfxSrc->nChannels == 1)
	    aad->convert = cvtMM16msK;
#endif
        FIXME("We don't support encoding yet\n");
        goto theEnd;
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
    WORD wSamplesPerBlock;
    /* wSamplesPerBlock formula comes from MSDN ADPCMWAVEFORMAT page.*/
    switch (adss->fdwSize)
    {
    case ACM_STREAMSIZEF_DESTINATION:
	/* cbDstLength => cbSrcLength */
	if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_PCM &&
	    adsi->pwfxDst->wFormatTag == WAVE_FORMAT_ADPCM)
        {
	    wSamplesPerBlock = adsi->pwfxDst->nBlockAlign * 2 / adsi->pwfxDst->nChannels - 12;
	    nblocks = adss->cbDstLength / adsi->pwfxDst->nBlockAlign;
	    if (nblocks == 0)
		return ACMERR_NOTPOSSIBLE;
	    adss->cbSrcLength = nblocks * adsi->pwfxSrc->nBlockAlign * wSamplesPerBlock;
	}
        else if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_ADPCM &&
                 adsi->pwfxDst->wFormatTag == WAVE_FORMAT_PCM)
        {
	    wSamplesPerBlock = adsi->pwfxSrc->nBlockAlign * 2 / adsi->pwfxSrc->nChannels - 12;
	    nblocks = adss->cbDstLength / (adsi->pwfxDst->nBlockAlign * wSamplesPerBlock);
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
	    adsi->pwfxDst->wFormatTag == WAVE_FORMAT_ADPCM)
        {
	    wSamplesPerBlock = adsi->pwfxDst->nBlockAlign * 2 / adsi->pwfxDst->nChannels - 12;
	    nblocks = adss->cbSrcLength / (adsi->pwfxSrc->nBlockAlign * wSamplesPerBlock);
	    if (nblocks == 0)
		return ACMERR_NOTPOSSIBLE;
	    if (adss->cbSrcLength % (adsi->pwfxSrc->nBlockAlign * wSamplesPerBlock))
		/* Round block count up. */
		nblocks++;
	    adss->cbDstLength = nblocks * adsi->pwfxDst->nBlockAlign;
	}
        else if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_ADPCM &&
                 adsi->pwfxDst->wFormatTag == WAVE_FORMAT_PCM)
        {
	    wSamplesPerBlock = adsi->pwfxSrc->nBlockAlign * 2 / adsi->pwfxSrc->nChannels - 12;
	    nblocks = adss->cbSrcLength / adsi->pwfxSrc->nBlockAlign;
	    if (nblocks == 0)
		return ACMERR_NOTPOSSIBLE;
	    if (adss->cbSrcLength % adsi->pwfxSrc->nBlockAlign)
		/* Round block count up. */
		nblocks++;
	    adss->cbDstLength = nblocks * adsi->pwfxDst->nBlockAlign * wSamplesPerBlock;
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
    case DRV_OPEN:		return ADPCM_drvOpen((LPSTR)dwParam1);
    case DRV_CLOSE:		return ADPCM_drvClose(dwDevID);
    case DRV_ENABLE:		return 1;
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBoxA(0, "MSACM MS ADPCM filter !", "Wine Driver", MB_OK); return 1;
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
