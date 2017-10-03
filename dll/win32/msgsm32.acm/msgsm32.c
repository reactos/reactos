/*
 * GSM 06.10 codec handling
 * Copyright (C) 2009 Maarten Lankhorst
 *
 * Based on msg711.acm
 * Copyright (C) 2002 Eric Pouech
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

#define WIN32_NO_STATUS

#include <config.h>
//#include <wine/port.h>

//#include <assert.h>
#include <stdarg.h>
//#include <string.h>

#ifdef HAVE_GSM_GSM_H
#include <gsm/gsm.h>
#elif defined(HAVE_GSM_H)
#include <gsm.h>
#endif

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <winnls.h>
//#include "mmsystem.h"
//#include "mmreg.h"
//#include "msacm.h"
#include <msacmdrv.h>
//#include "wine/library.h"
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(gsm);

#ifdef SONAME_LIBGSM

static void *libgsm_handle;
#define FUNCPTR(f) static typeof(f) * p##f
FUNCPTR(gsm_create);
FUNCPTR(gsm_destroy);
FUNCPTR(gsm_option);
FUNCPTR(gsm_encode);
FUNCPTR(gsm_decode);

#define LOAD_FUNCPTR(f) \
    if((p##f = wine_dlsym(libgsm_handle, #f, NULL, 0)) == NULL) { \
        wine_dlclose(libgsm_handle, NULL, 0); \
        libgsm_handle = NULL; \
        return FALSE; \
    }

/***********************************************************************
 *           GSM_drvLoad
 */
static BOOL GSM_drvLoad(void)
{
    char error[128];

    libgsm_handle = wine_dlopen(SONAME_LIBGSM, RTLD_NOW, error, sizeof(error));
    if (libgsm_handle)
    {
        LOAD_FUNCPTR(gsm_create);
        LOAD_FUNCPTR(gsm_destroy);
        LOAD_FUNCPTR(gsm_option);
        LOAD_FUNCPTR(gsm_encode);
        LOAD_FUNCPTR(gsm_decode);
        return TRUE;
    }
    else
    {
        ERR("Couldn't load " SONAME_LIBGSM ": %s\n", error);
        return FALSE;
    }
}

/***********************************************************************
 *           GSM_drvFree
 */
static LRESULT GSM_drvFree(void)
{
    if (libgsm_handle)
        wine_dlclose(libgsm_handle, NULL, 0);
    return 1;
}

#else

static LRESULT GSM_drvFree(void)
{
    return 1;
}

#endif

/***********************************************************************
 *           GSM_DriverDetails
 *
 */
static	LRESULT GSM_DriverDetails(PACMDRIVERDETAILSW add)
{
    add->fccType = ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC;
    add->fccComp = ACMDRIVERDETAILS_FCCCOMP_UNDEFINED;
    /* Details found from probing native msgsm32.acm */
    add->wMid = MM_MICROSOFT;
    add->wPid = MM_MSFT_ACM_GSM610;
    add->vdwACM = 0x3320000;
    add->vdwDriver = 0x4000000;
    add->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CODEC;
    add->cFormatTags = 2;
    add->cFilterTags = 0;
    add->hicon = NULL;
    MultiByteToWideChar( CP_ACP, 0, "Microsoft GSM 6.10", -1,
                         add->szShortName, sizeof(add->szShortName)/sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, "Wine GSM 6.10 libgsm codec", -1,
                         add->szLongName, sizeof(add->szLongName)/sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, "Brought to you by the Wine team...", -1,
                         add->szCopyright, sizeof(add->szCopyright)/sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, "Refer to LICENSE file", -1,
                         add->szLicensing, sizeof(add->szLicensing)/sizeof(WCHAR) );
    add->szFeatures[0] = 0;
    return MMSYSERR_NOERROR;
}

/* Validate a WAVEFORMATEX structure */
static BOOL GSM_FormatValidate(const WAVEFORMATEX *wfx)
{
    if (wfx->nChannels != 1)
        return FALSE;

    switch (wfx->wFormatTag)
    {
    case WAVE_FORMAT_PCM:
        if (wfx->wBitsPerSample != 16)
        {
            WARN("PCM wBitsPerSample %u\n", wfx->wBitsPerSample);
            return FALSE;
        }
        if (wfx->nBlockAlign != 2)
        {
            WARN("PCM nBlockAlign %u\n", wfx->nBlockAlign);
            return FALSE;
        }
        if (wfx->nAvgBytesPerSec != wfx->nBlockAlign * wfx->nSamplesPerSec)
        {
            WARN("PCM nAvgBytesPerSec %u/%u\n",
                 wfx->nAvgBytesPerSec,
                 wfx->nBlockAlign * wfx->nSamplesPerSec);
            return FALSE;
        }
        return TRUE;
    case WAVE_FORMAT_GSM610:
        if (wfx->cbSize < sizeof(WORD))
        {
            WARN("GSM cbSize %u\n", wfx->cbSize);
            return FALSE;
        }
        if (wfx->wBitsPerSample != 0)
        {
            WARN("GSM wBitsPerSample %u\n", wfx->wBitsPerSample);
            return FALSE;
        }
        if (wfx->nBlockAlign != 65)
        {
            WARN("GSM nBlockAlign %u\n", wfx->nBlockAlign);
            return FALSE;
        }
        if (((const GSM610WAVEFORMAT*)wfx)->wSamplesPerBlock != 320)
        {
            WARN("GSM wSamplesPerBlock %u\n",
                 ((const GSM610WAVEFORMAT*)wfx)->wSamplesPerBlock);
            return FALSE;
        }
        if (wfx->nAvgBytesPerSec != wfx->nSamplesPerSec * 65 / 320)
        {
            WARN("GSM nAvgBytesPerSec %d / %d\n",
                 wfx->nAvgBytesPerSec, wfx->nSamplesPerSec * 65 / 320);
            return FALSE;
        }
        return TRUE;
    default:
        return FALSE;
    }
    return FALSE;
}

static const DWORD gsm_rates[] = { 8000, 11025, 22050, 44100, 48000, 96000 };
#define NUM_RATES (sizeof(gsm_rates)/sizeof(*gsm_rates))

/***********************************************************************
 *           GSM_FormatTagDetails
 *
 */
static	LRESULT	GSM_FormatTagDetails(PACMFORMATTAGDETAILSW aftd, DWORD dwQuery)
{
    static const WCHAR szPcm[]={'P','C','M',0};
    static const WCHAR szGsm[]={'G','S','M',' ','6','.','1','0',0};

    switch (dwQuery)
    {
    case ACM_FORMATTAGDETAILSF_INDEX:
	if (aftd->dwFormatTagIndex > 1) return ACMERR_NOTPOSSIBLE;
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
	case WAVE_FORMAT_PCM: aftd->dwFormatTagIndex = 0; break;
	case WAVE_FORMAT_GSM610: aftd->dwFormatTagIndex = 1; break;
	default: return ACMERR_NOTPOSSIBLE;
	}
	break;
    default:
	WARN("Unsupported query %08x\n", dwQuery);
	return MMSYSERR_NOTSUPPORTED;
    }

    aftd->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CODEC;
    switch (aftd->dwFormatTagIndex)
    {
    case 0:
	aftd->dwFormatTag = WAVE_FORMAT_PCM;
	aftd->cbFormatSize = sizeof(PCMWAVEFORMAT);
	aftd->cStandardFormats = NUM_RATES;
        lstrcpyW(aftd->szFormatTag, szPcm);
        break;
    case 1:
	aftd->dwFormatTag = WAVE_FORMAT_GSM610;
	aftd->cbFormatSize = sizeof(GSM610WAVEFORMAT);
	aftd->cStandardFormats = NUM_RATES;
        lstrcpyW(aftd->szFormatTag, szGsm);
	break;
    }
    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           GSM_FormatDetails
 *
 */
static	LRESULT	GSM_FormatDetails(PACMFORMATDETAILSW afd, DWORD dwQuery)
{
    switch (dwQuery)
    {
    case ACM_FORMATDETAILSF_FORMAT:
	if (!GSM_FormatValidate(afd->pwfx)) return ACMERR_NOTPOSSIBLE;
	break;
    case ACM_FORMATDETAILSF_INDEX:
	afd->pwfx->wFormatTag = afd->dwFormatTag;
	switch (afd->dwFormatTag)
        {
	case WAVE_FORMAT_PCM:
	    if (afd->dwFormatIndex >= NUM_RATES) return ACMERR_NOTPOSSIBLE;
	    afd->pwfx->nChannels = 1;
	    afd->pwfx->nSamplesPerSec = gsm_rates[afd->dwFormatIndex];
	    afd->pwfx->wBitsPerSample = 16;
	    afd->pwfx->nBlockAlign = 2;
	    afd->pwfx->nAvgBytesPerSec = afd->pwfx->nSamplesPerSec * afd->pwfx->nBlockAlign;
	    break;
	case WAVE_FORMAT_GSM610:
            if (afd->dwFormatIndex >= NUM_RATES) return ACMERR_NOTPOSSIBLE;
	    afd->pwfx->nChannels = 1;
	    afd->pwfx->nSamplesPerSec = gsm_rates[afd->dwFormatIndex];
	    afd->pwfx->wBitsPerSample = 0;
	    afd->pwfx->nBlockAlign = 65;
            afd->pwfx->nAvgBytesPerSec = afd->pwfx->nSamplesPerSec * 65 / 320;
            afd->pwfx->cbSize = sizeof(WORD);
            ((GSM610WAVEFORMAT*)afd->pwfx)->wSamplesPerBlock = 320;
	    break;
	default:
            WARN("Unsupported tag %08x\n", afd->dwFormatTag);
	    return MMSYSERR_INVALPARAM;
	}
	break;
    default:
	WARN("Unsupported query %08x\n", dwQuery);
	return MMSYSERR_NOTSUPPORTED;
    }
    afd->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CODEC;
    afd->szFormat[0] = 0; /* let MSACM format this for us... */

    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           GSM_FormatSuggest
 *
 */
static	LRESULT	GSM_FormatSuggest(PACMDRVFORMATSUGGEST adfs)
{
    /* some tests ... */
    if (adfs->cbwfxSrc < sizeof(PCMWAVEFORMAT) ||
	adfs->cbwfxDst < sizeof(PCMWAVEFORMAT) ||
	!GSM_FormatValidate(adfs->pwfxSrc)) return ACMERR_NOTPOSSIBLE;
    /* FIXME: should do those tests against the real size (according to format tag */

    /* If no suggestion for destination, then copy source value */
    if (!(adfs->fdwSuggest & ACM_FORMATSUGGESTF_NCHANNELS))
	adfs->pwfxDst->nChannels = adfs->pwfxSrc->nChannels;
    if (!(adfs->fdwSuggest & ACM_FORMATSUGGESTF_NSAMPLESPERSEC))
        adfs->pwfxDst->nSamplesPerSec = adfs->pwfxSrc->nSamplesPerSec;

    if (!(adfs->fdwSuggest & ACM_FORMATSUGGESTF_WBITSPERSAMPLE))
    {
	if (adfs->pwfxSrc->wFormatTag == WAVE_FORMAT_PCM)
            adfs->pwfxDst->wBitsPerSample = 0;
        else
            adfs->pwfxDst->wBitsPerSample = 16;
    }
    if (!(adfs->fdwSuggest & ACM_FORMATSUGGESTF_WFORMATTAG))
    {
	switch (adfs->pwfxSrc->wFormatTag)
        {
        case WAVE_FORMAT_PCM: adfs->pwfxDst->wFormatTag = WAVE_FORMAT_GSM610; break;
        case WAVE_FORMAT_GSM610: adfs->pwfxDst->wFormatTag = WAVE_FORMAT_PCM; break;
        }
    }

    /* recompute other values */
    switch (adfs->pwfxDst->wFormatTag)
    {
    case WAVE_FORMAT_PCM:
        adfs->pwfxDst->nBlockAlign = 2;
        adfs->pwfxDst->nAvgBytesPerSec = adfs->pwfxDst->nSamplesPerSec * 2;
        break;
    case WAVE_FORMAT_GSM610:
        if (adfs->pwfxDst->cbSize < sizeof(WORD))
            return ACMERR_NOTPOSSIBLE;
        adfs->pwfxDst->nBlockAlign = 65;
        adfs->pwfxDst->nAvgBytesPerSec = adfs->pwfxDst->nSamplesPerSec * 65 / 320;
        ((GSM610WAVEFORMAT*)adfs->pwfxDst)->wSamplesPerBlock = 320;
        break;
    default:
        return ACMERR_NOTPOSSIBLE;
    }

    /* check if result is ok */
    if (!GSM_FormatValidate(adfs->pwfxDst)) return ACMERR_NOTPOSSIBLE;
    return MMSYSERR_NOERROR;
}

#ifdef SONAME_LIBGSM
/***********************************************************************
 *           GSM_StreamOpen
 *
 */
static	LRESULT	GSM_StreamOpen(PACMDRVSTREAMINSTANCE adsi)
{
    int used = 1;
    gsm r;
    if (!GSM_FormatValidate(adsi->pwfxSrc) || !GSM_FormatValidate(adsi->pwfxDst))
        return MMSYSERR_NOTSUPPORTED;

    if (adsi->pwfxSrc->nSamplesPerSec != adsi->pwfxDst->nSamplesPerSec)
        return MMSYSERR_NOTSUPPORTED;

    if (!GSM_drvLoad()) return MMSYSERR_NOTSUPPORTED;

    r = pgsm_create();
    if (!r)
        return MMSYSERR_NOMEM;
    if (pgsm_option(r, GSM_OPT_WAV49, &used) < 0)
    {
        FIXME("Your libgsm library doesn't support GSM_OPT_WAV49\n");
        FIXME("Please recompile libgsm with WAV49 support\n");
        pgsm_destroy(r);
        return MMSYSERR_NOTSUPPORTED;
    }
    adsi->dwDriver = (DWORD_PTR)r;
    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           GSM_StreamClose
 *
 */
static	LRESULT	GSM_StreamClose(PACMDRVSTREAMINSTANCE adsi)
{
    pgsm_destroy((gsm)adsi->dwDriver);
    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           GSM_StreamSize
 *
 */
static	LRESULT GSM_StreamSize(const ACMDRVSTREAMINSTANCE *adsi, PACMDRVSTREAMSIZE adss)
{
    switch (adss->fdwSize)
    {
    case ACM_STREAMSIZEF_DESTINATION:
	/* cbDstLength => cbSrcLength */
	if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_PCM &&
             adsi->pwfxDst->wFormatTag == WAVE_FORMAT_GSM610)
        {
	    adss->cbSrcLength = adss->cbDstLength / 65 * 640;
	}
        else if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_GSM610 &&
                 adsi->pwfxDst->wFormatTag == WAVE_FORMAT_PCM)
        {
	    adss->cbSrcLength = adss->cbDstLength / 640 * 65;
	}
        else
        {
	    return MMSYSERR_NOTSUPPORTED;
	}
	return MMSYSERR_NOERROR;
    case ACM_STREAMSIZEF_SOURCE:
	/* cbSrcLength => cbDstLength */
	if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_PCM &&
             adsi->pwfxDst->wFormatTag == WAVE_FORMAT_GSM610)
        {
	    adss->cbDstLength = (adss->cbSrcLength + 639) / 640 * 65;
	}
        else if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_GSM610 &&
                 adsi->pwfxDst->wFormatTag == WAVE_FORMAT_PCM)
        {
	    adss->cbDstLength = adss->cbSrcLength / 65 * 640;
	}
        else
        {
	    return MMSYSERR_NOTSUPPORTED;
	}
	return MMSYSERR_NOERROR;
    default:
	WARN("Unsupported query %08x\n", adss->fdwSize);
	return MMSYSERR_NOTSUPPORTED;
    }
}

/***********************************************************************
 *           GSM_StreamConvert
 *
 */
static LRESULT GSM_StreamConvert(PACMDRVSTREAMINSTANCE adsi, PACMDRVSTREAMHEADER adsh)
{
    gsm r = (gsm)adsi->dwDriver;
    DWORD nsrc = 0;
    DWORD ndst = 0;
    BYTE *src = adsh->pbSrc;
    BYTE *dst = adsh->pbDst;
    int odd = 0;

    if (adsh->fdwConvert &
	~(ACM_STREAMCONVERTF_BLOCKALIGN|
	  ACM_STREAMCONVERTF_END|
	  ACM_STREAMCONVERTF_START))
    {
	FIXME("Unsupported fdwConvert (%08x), ignoring it\n", adsh->fdwConvert);
    }

    /* Reset the index to 0, just to be sure */
    pgsm_option(r, GSM_OPT_FRAME_INDEX, &odd);

    /* The native ms codec writes 65 bytes, and this requires 2 libgsm calls.
     * First 32 bytes are written, or 33 bytes read
     * Second 33 bytes are written, or 32 bytes read
     */

    /* Decode */
    if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_GSM610)
    {
        if (adsh->cbSrcLength / 65 * 640 > adsh->cbDstLength)
        {
             return ACMERR_NOTPOSSIBLE;
        }

        while (nsrc + 65 <= adsh->cbSrcLength)
        {
            /* Decode data */
            if (pgsm_decode(r, src + nsrc, (gsm_signal*)(dst + ndst)) < 0)
                FIXME("Couldn't decode data\n");
            ndst += 320;
            nsrc += 33;

            if (pgsm_decode(r, src + nsrc, (gsm_signal*)(dst + ndst)) < 0)
                FIXME("Couldn't decode data\n");
            ndst += 320;
            nsrc += 32;
        }
    }
    else
    {
        /* Testing a little seems to reveal that despite being able to fit
         * inside the buffer if ACM_STREAMCONVERTF_BLOCKALIGN is set
         * it still rounds up
         */
        if ((adsh->cbSrcLength + 639) / 640 * 65 > adsh->cbDstLength)
        {
            return ACMERR_NOTPOSSIBLE;
        }

        /* The packing algorithm writes 32 bytes, then 33 bytes,
         * and it seems to pad to align to 65 bytes always
         * adding extra data where necessary
         */
        while (nsrc + 640 <= adsh->cbSrcLength)
        {
            /* Encode data */
            pgsm_encode(r, (gsm_signal*)(src+nsrc), dst+ndst);
            nsrc += 320;
            ndst += 32;
            pgsm_encode(r, (gsm_signal*)(src+nsrc), dst+ndst);
            nsrc += 320;
            ndst += 33;
        }

        /* If ACM_STREAMCONVERTF_BLOCKALIGN isn't set pad with zeros */
        if (!(adsh->fdwConvert & ACM_STREAMCONVERTF_BLOCKALIGN) &&
            nsrc < adsh->cbSrcLength)
        {
            char emptiness[320];
            int todo = adsh->cbSrcLength - nsrc;

            if (todo > 320)
            {
                pgsm_encode(r, (gsm_signal*)(src+nsrc), dst+ndst);
                ndst += 32;
                todo -= 320;
                nsrc += 320;

                memcpy(emptiness, src+nsrc, todo);
                memset(emptiness + todo, 0, 320 - todo);
                pgsm_encode(r, (gsm_signal*)emptiness, dst+ndst);
                ndst += 33;
            }
            else
            {
                memcpy(emptiness, src+nsrc, todo);
                memset(emptiness + todo, 0, 320 - todo);
                pgsm_encode(r, (gsm_signal*)emptiness, dst+ndst);
                ndst += 32;

                memset(emptiness, 0, todo);
                pgsm_encode(r, (gsm_signal*)emptiness, dst+ndst);
                ndst += 33;
            }
            nsrc = adsh->cbSrcLength;
        }
    }

    adsh->cbSrcLengthUsed = nsrc;
    adsh->cbDstLengthUsed = ndst;
    TRACE("%d(%d) -> %d(%d)\n", nsrc, adsh->cbSrcLength, ndst, adsh->cbDstLength);
    return MMSYSERR_NOERROR;
}

#endif

/**************************************************************************
 * 			GSM_DriverProc			[exported]
 */
LRESULT CALLBACK GSM_DriverProc(DWORD_PTR dwDevID, HDRVR hDriv, UINT wMsg,
					 LPARAM dwParam1, LPARAM dwParam2)
{
    TRACE("(%08lx %p %04x %08lx %08lx);\n",
          dwDevID, hDriv, wMsg, dwParam1, dwParam2);

    switch (wMsg)
    {
    case DRV_LOAD:		return 1;
    case DRV_FREE:		return GSM_drvFree();
    case DRV_OPEN:		return 1;
    case DRV_CLOSE:		return 1;
    case DRV_ENABLE:		return 1;
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBoxA(0, "GSM 06.10 codec", "Wine Driver", MB_OK); return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;

    case ACMDM_DRIVER_NOTIFY:
	/* no caching from other ACM drivers is done so far */
	return MMSYSERR_NOERROR;

    case ACMDM_DRIVER_DETAILS:
	return GSM_DriverDetails((PACMDRIVERDETAILSW)dwParam1);

    case ACMDM_FORMATTAG_DETAILS:
	return GSM_FormatTagDetails((PACMFORMATTAGDETAILSW)dwParam1, dwParam2);

    case ACMDM_FORMAT_DETAILS:
	return GSM_FormatDetails((PACMFORMATDETAILSW)dwParam1, dwParam2);

    case ACMDM_FORMAT_SUGGEST:
	return GSM_FormatSuggest((PACMDRVFORMATSUGGEST)dwParam1);

#ifdef SONAME_LIBGSM
    case ACMDM_STREAM_OPEN:
	return GSM_StreamOpen((PACMDRVSTREAMINSTANCE)dwParam1);

    case ACMDM_STREAM_CLOSE:
	return GSM_StreamClose((PACMDRVSTREAMINSTANCE)dwParam1);

    case ACMDM_STREAM_SIZE:
	return GSM_StreamSize((PACMDRVSTREAMINSTANCE)dwParam1, (PACMDRVSTREAMSIZE)dwParam2);

    case ACMDM_STREAM_CONVERT:
	return GSM_StreamConvert((PACMDRVSTREAMINSTANCE)dwParam1, (PACMDRVSTREAMHEADER)dwParam2);
#else
    case ACMDM_STREAM_OPEN: WARN("libgsm support not compiled in!\n");
    case ACMDM_STREAM_CLOSE:
    case ACMDM_STREAM_SIZE:
    case ACMDM_STREAM_CONVERT:
        return MMSYSERR_NOTSUPPORTED;
#endif

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
