/*
 * MPEG Layer 3 handling
 *
 * Copyright (C) 2002 Eric Pouech
 * Copyright (C) 2009 CodeWeavers, Aric Stewart
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
#include <mpg123.h>

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

WINE_DEFAULT_DEBUG_CHANNEL(mpeg3);

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
    {1,  8, 12000}, {2,  8, 12000}, {1, 16, 12000}, {2, 16, 12000},
    {1,  8, 16000}, {2,  8, 16000}, {1, 16, 16000}, {2, 16, 16000},
    {1,  8, 22050}, {2,  8, 22050}, {1, 16, 22050}, {2, 16, 22050},
    {1,  8, 24000}, {2,  8, 24000}, {1, 16, 24000}, {2, 16, 24000},
    {1,  8, 32000}, {2,  8, 32000}, {1, 16, 32000}, {2, 16, 32000},
    {1,  8, 44100}, {2,  8, 44100}, {1, 16, 44100}, {2, 16, 44100},
    {1,  8, 48000}, {2,  8, 48000}, {1, 16, 48000}, {2, 16, 48000}
};

static const Format MPEG3_Formats[] =
{
    {1,  0,  8000}, {2,  0,  8000},
    {1,  0, 11025}, {2,  0, 11025},
    {1,  0, 12000}, {2,  0, 12000},
    {1,  0, 16000}, {2,  0, 16000},
    {1,  0, 22050}, {2,  0, 22050},
    {1,  0, 24000}, {2,  0, 24000},
    {1,  0, 32000}, {2,  0, 32000},
    {1,  0, 44100}, {2,  0, 44100},
    {1,  0, 48000}, {2,  0, 48000}
};

/***********************************************************************
 *           MPEG3_GetFormatIndex
 */
static	DWORD	MPEG3_GetFormatIndex(LPWAVEFORMATEX wfx)
{
    int 	i, hi;
    const Format *fmts;

    switch (wfx->wFormatTag)
    {
    case WAVE_FORMAT_PCM:
	hi = ARRAY_SIZE(PCM_Formats);
	fmts = PCM_Formats;
	break;
    case WAVE_FORMAT_MPEG:
    case WAVE_FORMAT_MPEGLAYER3:
	hi = ARRAY_SIZE(MPEG3_Formats);
	fmts = MPEG3_Formats;
	break;
    default:
	return 0xFFFFFFFF;
    }

    for (i = 0; i < hi; i++)
    {
	if (wfx->nChannels == fmts[i].nChannels &&
	    wfx->nSamplesPerSec == fmts[i].rate &&
	    (wfx->wBitsPerSample == fmts[i].nBits || !fmts[i].nBits))
	    return i;
    }

    return 0xFFFFFFFF;
}

/***********************************************************************
 *           MPEG3_drvOpen
 */
static LRESULT MPEG3_drvOpen(LPCSTR str)
{
    mpg123_init();
    return 1;
}

/***********************************************************************
 *           MPEG3_drvClose
 */
static LRESULT MPEG3_drvClose(DWORD_PTR dwDevID)
{
    mpg123_exit();
    return 1;
}


static void mp3_horse(mpg123_handle *handle, const unsigned char *src,
        DWORD *nsrc, unsigned char *dst, DWORD *ndst)
{
    int                 ret;
    size_t              size;
    DWORD               dpos = 0;


    if (*nsrc > 0)
    {
        ret = mpg123_feed(handle, src, *nsrc);
        if (ret != MPG123_OK)
        {
            ERR("Error feeding data\n");
            *ndst = *nsrc = 0;
            return;
        }
    }

    do {
        size = 0;
        ret = mpg123_read(handle, dst + dpos, *ndst - dpos, &size);
        if (ret == MPG123_ERR)
        {
            FIXME("Error occurred during decoding!\n");
            *ndst = *nsrc = 0;
            return;
        }

        if (ret == MPG123_NEW_FORMAT)
        {
            long rate;
            int channels, enc;
            mpg123_getformat(handle, &rate, &channels, &enc);
            TRACE("New format: %li Hz, %i channels, encoding value %i\n", rate, channels, enc);
        }
        dpos += size;
        if (dpos >= *ndst) break;
    } while (ret != MPG123_ERR && ret != MPG123_NEED_MORE);
    *ndst = dpos;
}

/***********************************************************************
 *           MPEG3_StreamOpen
 *
 */
static LRESULT MPEG3_StreamOpen(ACMDRVSTREAMINSTANCE *instance)
{
    mpg123_handle *handle;
    int err;

    assert(!(instance->fdwOpen & ACM_STREAMOPENF_ASYNC));

    if (MPEG3_GetFormatIndex(instance->pwfxSrc) == 0xFFFFFFFF
            || MPEG3_GetFormatIndex(instance->pwfxDst) == 0xFFFFFFFF)
        return ACMERR_NOTPOSSIBLE;

    if (instance->pwfxDst->wFormatTag != WAVE_FORMAT_PCM)
        return MMSYSERR_NOTSUPPORTED;

    if (instance->pwfxSrc->wFormatTag != WAVE_FORMAT_MPEGLAYER3
            && instance->pwfxSrc->wFormatTag != WAVE_FORMAT_MPEG)
        return MMSYSERR_NOTSUPPORTED;

    if (instance->pwfxSrc->wFormatTag == WAVE_FORMAT_MPEGLAYER3)
    {
        MPEGLAYER3WAVEFORMAT *mp3_format = (MPEGLAYER3WAVEFORMAT *)instance->pwfxSrc;

        if (instance->pwfxSrc->cbSize < MPEGLAYER3_WFX_EXTRA_BYTES
                || mp3_format->wID != MPEGLAYER3_ID_MPEG)
            return ACMERR_NOTPOSSIBLE;
    }

    if (instance->pwfxSrc->nSamplesPerSec != instance->pwfxDst->nSamplesPerSec
            || instance->pwfxSrc->nChannels != instance->pwfxDst->nChannels
            || instance->pwfxDst->wBitsPerSample != 16)
        return MMSYSERR_NOTSUPPORTED;

    handle = mpg123_new(NULL, &err);
    instance->dwDriver = (DWORD_PTR)handle;
    mpg123_open_feed(handle);

    /* mpg123 may find a XING header in the mp3 and use that information
     * to ask for seeks in order to read specific frames in the file.
     * We cannot allow that since the caller application is feeding us.
     * This fixes problems for mp3 files encoded with LAME (bug 42361)
     */
    mpg123_param(handle, MPG123_ADD_FLAGS, MPG123_IGNORE_INFOFRAME, 0);

    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           MPEG3_StreamClose
 *
 */
static LRESULT MPEG3_StreamClose(ACMDRVSTREAMINSTANCE *instance)
{
    mpg123_handle *handle = (mpg123_handle *)instance->dwDriver;

    mpg123_close(handle);
    mpg123_delete(handle);
    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           MPEG3_DriverDetails
 *
 */
static	LRESULT MPEG3_DriverDetails(PACMDRIVERDETAILSW add)
{
    add->fccType = ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC;
    add->fccComp = ACMDRIVERDETAILS_FCCCOMP_UNDEFINED;
    add->wMid = MM_FRAUNHOFER_IIS;
    add->wPid = MM_FHGIIS_MPEGLAYER3_DECODE;
    add->vdwACM = 0x01000000;
    add->vdwDriver = 0x01000000;
    add->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CODEC;
    add->cFormatTags = 3; /* PCM, MPEG3 */
    add->cFilterTags = 0;
    add->hicon = NULL;
    MultiByteToWideChar( CP_ACP, 0, "MPEG Layer-3 Codec", -1,
                         add->szShortName, ARRAY_SIZE( add->szShortName ));
    MultiByteToWideChar( CP_ACP, 0, "Wine MPEG3 decoder", -1,
                         add->szLongName, ARRAY_SIZE( add->szLongName ));
    MultiByteToWideChar( CP_ACP, 0, "Brought to you by the Wine team...", -1,
                         add->szCopyright, ARRAY_SIZE( add->szCopyright ));
    MultiByteToWideChar( CP_ACP, 0, "Refer to LICENSE file", -1,
                         add->szLicensing, ARRAY_SIZE( add->szLicensing ));
    add->szFeatures[0] = 0;

    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           MPEG3_FormatTagDetails
 *
 */
static	LRESULT	MPEG3_FormatTagDetails(PACMFORMATTAGDETAILSW aftd, DWORD dwQuery)
{
    static const WCHAR szPcm[]={'P','C','M',0};
    static const WCHAR szMpeg3[]={'M','P','e','g','3',0};
    static const WCHAR szMpeg[]={'M','P','e','g',0};

    switch (dwQuery)
    {
    case ACM_FORMATTAGDETAILSF_INDEX:
	if (aftd->dwFormatTagIndex > 2) return ACMERR_NOTPOSSIBLE;
	break;
    case ACM_FORMATTAGDETAILSF_LARGESTSIZE:
	if (aftd->dwFormatTag == WAVE_FORMAT_UNKNOWN)
        {
            aftd->dwFormatTagIndex = 2; /* WAVE_FORMAT_MPEG is biggest */
	    break;
	}
	/* fall through */
    case ACM_FORMATTAGDETAILSF_FORMATTAG:
	switch (aftd->dwFormatTag)
        {
	case WAVE_FORMAT_PCM:		aftd->dwFormatTagIndex = 0; break;
	case WAVE_FORMAT_MPEGLAYER3:    aftd->dwFormatTagIndex = 1; break;
	case WAVE_FORMAT_MPEG:          aftd->dwFormatTagIndex = 2; break;
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
        lstrcpyW(aftd->szFormatTag, szPcm);
        break;
    case 1:
	aftd->dwFormatTag = WAVE_FORMAT_MPEGLAYER3;
	aftd->cbFormatSize = sizeof(MPEGLAYER3WAVEFORMAT);
        aftd->cStandardFormats = 0;
        lstrcpyW(aftd->szFormatTag, szMpeg3);
	break;
    case 2:
	aftd->dwFormatTag = WAVE_FORMAT_MPEG;
	aftd->cbFormatSize = sizeof(MPEG1WAVEFORMAT);
        aftd->cStandardFormats = 0;
        lstrcpyW(aftd->szFormatTag, szMpeg);
	break;
    }
    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           MPEG3_FormatDetails
 *
 */
static	LRESULT	MPEG3_FormatDetails(PACMFORMATDETAILSW afd, DWORD dwQuery)
{
    switch (dwQuery)
    {
    case ACM_FORMATDETAILSF_FORMAT:
	if (MPEG3_GetFormatIndex(afd->pwfx) == 0xFFFFFFFF) return ACMERR_NOTPOSSIBLE;
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
	case WAVE_FORMAT_MPEGLAYER3:
	case WAVE_FORMAT_MPEG:
            WARN("Encoding to MPEG is not supported\n");
            return ACMERR_NOTPOSSIBLE;
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
 *           MPEG3_FormatSuggest
 *
 */
static	LRESULT	MPEG3_FormatSuggest(PACMDRVFORMATSUGGEST adfs)
{
    /* some tests ... */
    if (adfs->cbwfxSrc < sizeof(PCMWAVEFORMAT) ||
	adfs->cbwfxDst < sizeof(PCMWAVEFORMAT) ||
	MPEG3_GetFormatIndex(adfs->pwfxSrc) == 0xFFFFFFFF) return ACMERR_NOTPOSSIBLE;
    /* FIXME: should do those tests against the real size (according to format tag */

    /* If no suggestion for destination, then copy source value */
    if (!(adfs->fdwSuggest & ACM_FORMATSUGGESTF_NCHANNELS))
	adfs->pwfxDst->nChannels = adfs->pwfxSrc->nChannels;
    if (!(adfs->fdwSuggest & ACM_FORMATSUGGESTF_NSAMPLESPERSEC))
        adfs->pwfxDst->nSamplesPerSec = adfs->pwfxSrc->nSamplesPerSec;
    if (!(adfs->fdwSuggest & ACM_FORMATSUGGESTF_WBITSPERSAMPLE))
        adfs->pwfxDst->wBitsPerSample = 16;
    if (!(adfs->fdwSuggest & ACM_FORMATSUGGESTF_WFORMATTAG))
    {
	if (adfs->pwfxSrc->wFormatTag == WAVE_FORMAT_PCM)
        {
            WARN("Encoding to MPEG is not supported\n");
            return ACMERR_NOTPOSSIBLE;
        }
        else
            adfs->pwfxDst->wFormatTag = WAVE_FORMAT_PCM;
    }

    /* check if result is ok */
    if (MPEG3_GetFormatIndex(adfs->pwfxDst) == 0xFFFFFFFF) return ACMERR_NOTPOSSIBLE;

    /* recompute other values */
    switch (adfs->pwfxDst->wFormatTag)
    {
    case WAVE_FORMAT_PCM:
        adfs->pwfxDst->nBlockAlign = (adfs->pwfxDst->nChannels * adfs->pwfxDst->wBitsPerSample) / 8;
        adfs->pwfxDst->nAvgBytesPerSec = adfs->pwfxDst->nSamplesPerSec * adfs->pwfxDst->nBlockAlign;
        break;
    case WAVE_FORMAT_MPEG:
    case WAVE_FORMAT_MPEGLAYER3:
        WARN("Encoding to MPEG is not supported\n");
        return ACMERR_NOTPOSSIBLE;
        break;
    default:
        FIXME("\n");
        break;
    }

    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           MPEG3_StreamSize
 *
 */
static	LRESULT MPEG3_StreamSize(PACMDRVSTREAMINSTANCE adsi, PACMDRVSTREAMSIZE adss)
{
    DWORD nblocks;

    switch (adss->fdwSize)
    {
    case ACM_STREAMSIZEF_DESTINATION:
	/* cbDstLength => cbSrcLength */
	if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_PCM &&
            (adsi->pwfxDst->wFormatTag == WAVE_FORMAT_MPEGLAYER3 ||
             adsi->pwfxDst->wFormatTag == WAVE_FORMAT_MPEG))
        {
            nblocks = (adss->cbDstLength - 3000) / (DWORD)(adsi->pwfxDst->nAvgBytesPerSec * 1152 / adsi->pwfxDst->nSamplesPerSec + 0.5);
            if (nblocks == 0)
                return ACMERR_NOTPOSSIBLE;
            adss->cbSrcLength = nblocks * 1152 * adsi->pwfxSrc->nBlockAlign;
	}
        else if ((adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_MPEGLAYER3 ||
                 adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_MPEG) &&
                 adsi->pwfxDst->wFormatTag == WAVE_FORMAT_PCM)
        {
            nblocks = adss->cbDstLength / (adsi->pwfxDst->nBlockAlign * 1152);
            if (nblocks == 0)
                return ACMERR_NOTPOSSIBLE;
            adss->cbSrcLength = nblocks * (DWORD)(adsi->pwfxSrc->nAvgBytesPerSec * 1152 / adsi->pwfxSrc->nSamplesPerSec);
	}
        else
        {
	    return MMSYSERR_NOTSUPPORTED;
	}
	break;
    case ACM_STREAMSIZEF_SOURCE:
	/* cbSrcLength => cbDstLength */
	if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_PCM &&
            (adsi->pwfxDst->wFormatTag == WAVE_FORMAT_MPEGLAYER3 ||
             adsi->pwfxDst->wFormatTag == WAVE_FORMAT_MPEG))
        {
            nblocks = adss->cbSrcLength / (adsi->pwfxSrc->nBlockAlign * 1152);
            if (adss->cbSrcLength % (DWORD)(adsi->pwfxSrc->nBlockAlign * 1152))
                /* Round block count up. */
                nblocks++;
            if (nblocks == 0)
                return ACMERR_NOTPOSSIBLE;
            adss->cbDstLength = 3000 + nblocks * (DWORD)(adsi->pwfxDst->nAvgBytesPerSec * 1152 / adsi->pwfxDst->nSamplesPerSec + 0.5);
	}
        else if ((adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_MPEGLAYER3 ||
                 adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_MPEG) &&
                 adsi->pwfxDst->wFormatTag == WAVE_FORMAT_PCM)
        {
            nblocks = adss->cbSrcLength / (DWORD)(adsi->pwfxSrc->nAvgBytesPerSec * 1152 / adsi->pwfxSrc->nSamplesPerSec);
            if (adss->cbSrcLength % (DWORD)(adsi->pwfxSrc->nAvgBytesPerSec * 1152 / adsi->pwfxSrc->nSamplesPerSec))
                /* Round block count up. */
                nblocks++;
            if (nblocks == 0)
                return ACMERR_NOTPOSSIBLE;
            adss->cbDstLength = nblocks * 1152 * adsi->pwfxDst->nBlockAlign;
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
 *           MPEG3_StreamConvert
 *
 */
static LRESULT MPEG3_StreamConvert(ACMDRVSTREAMINSTANCE *instance, ACMDRVSTREAMHEADER *adsh)
{
    mpg123_handle *handle = (mpg123_handle *)instance->dwDriver;
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
        mpg123_feedseek(handle, 0, SEEK_SET, NULL);
        mpg123_close(handle);
        mpg123_open_feed(handle);
    }

    mp3_horse(handle, adsh->pbSrc, &nsrc, adsh->pbDst, &ndst);
    adsh->cbSrcLengthUsed = nsrc;
    adsh->cbDstLengthUsed = ndst;

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			MPEG3_DriverProc			[exported]
 */
LRESULT CALLBACK MPEG3_DriverProc(DWORD_PTR dwDevID, HDRVR hDriv, UINT wMsg,
					 LPARAM dwParam1, LPARAM dwParam2)
{
    TRACE("(%08Ix %p %04x %08Ix %08Ix);\n",
	  dwDevID, hDriv, wMsg, dwParam1, dwParam2);

    switch (wMsg)
    {
    case DRV_LOAD:		return 1;
    case DRV_FREE:		return 1;
    case DRV_OPEN:		return MPEG3_drvOpen((LPSTR)dwParam1);
    case DRV_CLOSE:		return MPEG3_drvClose(dwDevID);
    case DRV_ENABLE:		return 1;
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBoxA(0, "MPEG3 filter !", "Wine Driver", MB_OK); return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;

    case ACMDM_DRIVER_NOTIFY:
	/* no caching from other ACM drivers is done so far */
	return MMSYSERR_NOERROR;

    case ACMDM_DRIVER_DETAILS:
	return MPEG3_DriverDetails((PACMDRIVERDETAILSW)dwParam1);

    case ACMDM_FORMATTAG_DETAILS:
	return MPEG3_FormatTagDetails((PACMFORMATTAGDETAILSW)dwParam1, dwParam2);

    case ACMDM_FORMAT_DETAILS:
	return MPEG3_FormatDetails((PACMFORMATDETAILSW)dwParam1, dwParam2);

    case ACMDM_FORMAT_SUGGEST:
	return MPEG3_FormatSuggest((PACMDRVFORMATSUGGEST)dwParam1);

    case ACMDM_STREAM_OPEN:
	return MPEG3_StreamOpen((PACMDRVSTREAMINSTANCE)dwParam1);

    case ACMDM_STREAM_CLOSE:
	return MPEG3_StreamClose((PACMDRVSTREAMINSTANCE)dwParam1);

    case ACMDM_STREAM_SIZE:
	return MPEG3_StreamSize((PACMDRVSTREAMINSTANCE)dwParam1, (PACMDRVSTREAMSIZE)dwParam2);

    case ACMDM_STREAM_CONVERT:
	return MPEG3_StreamConvert((PACMDRVSTREAMINSTANCE)dwParam1, (PACMDRVSTREAMHEADER)dwParam2);

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
