/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 *      MSACM32 library
 *
 *      Copyright 1998  Patrik Stridvall
 *		  1999	Eric Pouech
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

/* TODO
 * 	+ asynchronous conversion is not implemented
 *	+ callback/notification
 *	* acmStreamMessage
 *	+ properly close ACM streams
 */

#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wine/debug.h"
#include "mmsystem.h"
#define NOBITMAP
#include "mmreg.h"
#include "msacm.h"
#include "msacmdrv.h"
#include "wineacm.h"

WINE_DEFAULT_DEBUG_CHANNEL(msacm);

static PWINE_ACMSTREAM	ACM_GetStream(HACMSTREAM has)
{
    TRACE("(%p)\n", has);

    return (PWINE_ACMSTREAM)has;
}

/***********************************************************************
 *           acmStreamClose (MSACM32.@)
 */
MMRESULT WINAPI acmStreamClose(HACMSTREAM has, DWORD fdwClose)
{
    PWINE_ACMSTREAM	was;
    MMRESULT		ret;

    TRACE("(%p, %d)\n", has, fdwClose);

    if ((was = ACM_GetStream(has)) == NULL) {
        WARN("invalid handle\n");
	return MMSYSERR_INVALHANDLE;
    }
    ret = MSACM_Message((HACMDRIVER)was->pDrv, ACMDM_STREAM_CLOSE, (LPARAM)&was->drvInst, 0);
    if (ret == MMSYSERR_NOERROR) {
	if (was->hAcmDriver)
	    acmDriverClose(was->hAcmDriver, 0L);
	HeapFree(MSACM_hHeap, 0, was);
    }
    TRACE("=> (%d)\n", ret);
    return ret;
}

/***********************************************************************
 *           acmStreamConvert (MSACM32.@)
 */
MMRESULT WINAPI acmStreamConvert(HACMSTREAM has, PACMSTREAMHEADER pash,
				 DWORD fdwConvert)
{
    PWINE_ACMSTREAM	was;
    MMRESULT		ret = MMSYSERR_NOERROR;
    PACMDRVSTREAMHEADER	padsh;

    TRACE("(%p, %p, %d)\n", has, pash, fdwConvert);

    if ((was = ACM_GetStream(has)) == NULL) {
        WARN("invalid handle\n");
	return MMSYSERR_INVALHANDLE;
    }
    if (!pash || pash->cbStruct < sizeof(ACMSTREAMHEADER)) {
        WARN("invalid parameter\n");
	return MMSYSERR_INVALPARAM;
    }
    if (!(pash->fdwStatus & ACMSTREAMHEADER_STATUSF_PREPARED)) {
        WARN("unprepared header\n");
	return ACMERR_UNPREPARED;
    }

    pash->cbSrcLengthUsed = 0;
    pash->cbDstLengthUsed = 0;

    /* Note: the ACMSTREAMHEADER and ACMDRVSTREAMHEADER structs are of same
     * size. some fields are private to msacm internals, and are exposed
     * in ACMSTREAMHEADER in the dwReservedDriver array
     */
    padsh = (PACMDRVSTREAMHEADER)pash;

    /* check that pointers have not been modified */
    if (padsh->pbPreparedSrc != padsh->pbSrc ||
	padsh->cbPreparedSrcLength < padsh->cbSrcLength ||
	padsh->pbPreparedDst != padsh->pbDst ||
	padsh->cbPreparedDstLength < padsh->cbDstLength) {
        WARN("invalid parameter\n");
	return MMSYSERR_INVALPARAM;
    }

    padsh->fdwConvert = fdwConvert;

    ret = MSACM_Message((HACMDRIVER)was->pDrv, ACMDM_STREAM_CONVERT, (LPARAM)&was->drvInst, (LPARAM)padsh);
    if (ret == MMSYSERR_NOERROR) {
	padsh->fdwStatus |= ACMSTREAMHEADER_STATUSF_DONE;
    }
    TRACE("=> (%d)\n", ret);
    return ret;
}

/***********************************************************************
 *           acmStreamMessage (MSACM32.@)
 */
MMRESULT WINAPI acmStreamMessage(HACMSTREAM has, UINT uMsg, LPARAM lParam1,
				 LPARAM lParam2)
{
    FIXME("(%p, %u, %ld, %ld): stub\n", has, uMsg, lParam1, lParam2);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmStreamOpen (MSACM32.@)
 */
MMRESULT WINAPI acmStreamOpen(PHACMSTREAM phas, HACMDRIVER had,
                              PWAVEFORMATEX pwfxSrc, PWAVEFORMATEX pwfxDst,
                              PWAVEFILTER pwfltr, DWORD_PTR dwCallback,
                              DWORD_PTR dwInstance, DWORD fdwOpen)
{
    PWINE_ACMSTREAM	was;
    PWINE_ACMDRIVER	wad;
    MMRESULT		ret;
    int			wfxSrcSize;
    int			wfxDstSize;
    WAVEFORMATEX	wfxSrc, wfxDst;

    TRACE("(%p, %p, %p, %p, %p, %ld, %ld, %d)\n",
	  phas, had, pwfxSrc, pwfxDst, pwfltr, dwCallback, dwInstance, fdwOpen);

    /* NOTE: pwfxSrc and/or pwfxDst can point to a structure smaller than
     * WAVEFORMATEX so don't use them directly when not sure */
    if (pwfxSrc->wFormatTag == WAVE_FORMAT_PCM) {
        memcpy(&wfxSrc, pwfxSrc, sizeof(PCMWAVEFORMAT));
        wfxSrc.wBitsPerSample = pwfxSrc->wBitsPerSample;
        wfxSrc.cbSize = 0;
        pwfxSrc = &wfxSrc;
    }

    if (pwfxDst->wFormatTag == WAVE_FORMAT_PCM) {
        memcpy(&wfxDst, pwfxDst, sizeof(PCMWAVEFORMAT));
        wfxDst.wBitsPerSample = pwfxDst->wBitsPerSample;
        wfxDst.cbSize = 0;
        pwfxDst = &wfxDst;
    }

    TRACE("src [wFormatTag=%u, nChannels=%u, nSamplesPerSec=%u, nAvgBytesPerSec=%u, nBlockAlign=%u, wBitsPerSample=%u, cbSize=%u]\n",
	  pwfxSrc->wFormatTag, pwfxSrc->nChannels, pwfxSrc->nSamplesPerSec, pwfxSrc->nAvgBytesPerSec,
	  pwfxSrc->nBlockAlign, pwfxSrc->wBitsPerSample, pwfxSrc->cbSize);

    TRACE("dst [wFormatTag=%u, nChannels=%u, nSamplesPerSec=%u, nAvgBytesPerSec=%u, nBlockAlign=%u, wBitsPerSample=%u, cbSize=%u]\n",
	  pwfxDst->wFormatTag, pwfxDst->nChannels, pwfxDst->nSamplesPerSec, pwfxDst->nAvgBytesPerSec,
	  pwfxDst->nBlockAlign, pwfxDst->wBitsPerSample, pwfxDst->cbSize);

    /* (WS) In query mode, phas should be NULL. If it is not, then instead
     * of returning an error we are making sure it is NULL, preventing some
     * applications that pass garbage for phas from crashing.
     */
    if (fdwOpen & ACM_STREAMOPENF_QUERY) phas = NULL;

    if (pwfltr && (pwfxSrc->wFormatTag != pwfxDst->wFormatTag)) {
        WARN("invalid parameter\n");
        return MMSYSERR_INVALPARAM;
    }

    wfxSrcSize = wfxDstSize = sizeof(WAVEFORMATEX);
    if (pwfxSrc->wFormatTag != WAVE_FORMAT_PCM) wfxSrcSize += pwfxSrc->cbSize;
    if (pwfxDst->wFormatTag != WAVE_FORMAT_PCM) wfxDstSize += pwfxDst->cbSize;

    was = HeapAlloc(MSACM_hHeap, 0, sizeof(*was) + wfxSrcSize + wfxDstSize +
		    ((pwfltr) ? sizeof(WAVEFILTER) : 0));
    if (was == NULL) {
        WARN("no memory\n");
	return MMSYSERR_NOMEM;
    }

    was->drvInst.cbStruct = sizeof(was->drvInst);
    was->drvInst.pwfxSrc = (PWAVEFORMATEX)((LPSTR)was + sizeof(*was));
    memcpy(was->drvInst.pwfxSrc, pwfxSrc, wfxSrcSize);
    was->drvInst.pwfxDst = (PWAVEFORMATEX)((LPSTR)was + sizeof(*was) + wfxSrcSize);
    memcpy(was->drvInst.pwfxDst, pwfxDst, wfxDstSize);
    if (pwfltr) {
	was->drvInst.pwfltr = (PWAVEFILTER)((LPSTR)was + sizeof(*was) + wfxSrcSize + wfxDstSize);
	memcpy(was->drvInst.pwfltr, pwfltr, sizeof(WAVEFILTER));
    } else {
	was->drvInst.pwfltr = NULL;
    }
    was->drvInst.dwCallback = dwCallback;
    was->drvInst.dwInstance = dwInstance;
    was->drvInst.fdwOpen = fdwOpen;
    was->drvInst.fdwDriver = 0L;
    was->drvInst.dwDriver = 0L;
    /* real value will be stored once ACMDM_STREAM_OPEN succeeds */
    was->drvInst.has = 0L;

    if (had) {
	if (!(wad = MSACM_GetDriver(had))) {
	    ret = MMSYSERR_INVALPARAM;
	    goto errCleanUp;
	}

	was->obj.dwType = WINE_ACMOBJ_STREAM;
	was->obj.pACMDriverID = wad->obj.pACMDriverID;
	was->pDrv = wad;
	was->hAcmDriver = 0; /* not to close it in acmStreamClose */

	ret = MSACM_Message((HACMDRIVER)wad, ACMDM_STREAM_OPEN, (LPARAM)&was->drvInst, 0L);
	if (ret != MMSYSERR_NOERROR)
	    goto errCleanUp;
    } else {
	PWINE_ACMDRIVERID wadi;

	ret = ACMERR_NOTPOSSIBLE;
	for (wadi = MSACM_pFirstACMDriverID; wadi; wadi = wadi->pNextACMDriverID) {
	    if ((wadi->fdwSupport & ACMDRIVERDETAILS_SUPPORTF_DISABLED) ||
		!MSACM_FindFormatTagInCache(wadi, pwfxSrc->wFormatTag, NULL) ||
		!MSACM_FindFormatTagInCache(wadi, pwfxDst->wFormatTag, NULL))
		continue;
	    ret = acmDriverOpen(&had, (HACMDRIVERID)wadi, 0L);
	    if (ret != MMSYSERR_NOERROR)
		continue;
	    if ((wad = MSACM_GetDriver(had)) != 0) {
		was->obj.dwType = WINE_ACMOBJ_STREAM;
		was->obj.pACMDriverID = wad->obj.pACMDriverID;
		was->pDrv = wad;
		was->hAcmDriver = had;

		ret = MSACM_Message((HACMDRIVER)wad, ACMDM_STREAM_OPEN, (LPARAM)&was->drvInst, 0L);
		TRACE("%s => %08x\n", debugstr_w(wadi->pszDriverAlias), ret);
		if (ret == MMSYSERR_NOERROR) {
		    if (fdwOpen & ACM_STREAMOPENF_QUERY) {
			acmDriverClose(had, 0L);
		    }
		    break;
		}
	    }
	    /* no match, close this acm driver and try next one */
	    acmDriverClose(had, 0L);
	}
	if (ret != MMSYSERR_NOERROR) {
	    ret = ACMERR_NOTPOSSIBLE;
	    goto errCleanUp;
	}
    }
    ret = MMSYSERR_NOERROR;
    was->drvInst.has = (HACMSTREAM)was;
    if (!(fdwOpen & ACM_STREAMOPENF_QUERY)) {
	if (phas)
	    *phas = (HACMSTREAM)was;
	TRACE("=> (%d)\n", ret);
	return ret;
    }
errCleanUp:
    if (phas)
	*phas = NULL;
    HeapFree(MSACM_hHeap, 0, was);
    TRACE("=> (%d)\n", ret);
    return ret;
}


/***********************************************************************
 *           acmStreamPrepareHeader (MSACM32.@)
 */
MMRESULT WINAPI acmStreamPrepareHeader(HACMSTREAM has, PACMSTREAMHEADER pash,
				       DWORD fdwPrepare)
{
    PWINE_ACMSTREAM	was;
    MMRESULT		ret = MMSYSERR_NOERROR;
    PACMDRVSTREAMHEADER	padsh;

    TRACE("(%p, %p, %d)\n", has, pash, fdwPrepare);

    if ((was = ACM_GetStream(has)) == NULL) {
        WARN("invalid handle\n");
	return MMSYSERR_INVALHANDLE;
    }
    if (!pash || pash->cbStruct < sizeof(ACMSTREAMHEADER)) {
        WARN("invalid parameter\n");
	return MMSYSERR_INVALPARAM;
    }
    if (fdwPrepare)
	ret = MMSYSERR_INVALFLAG;

    if (pash->fdwStatus & ACMSTREAMHEADER_STATUSF_DONE)
	return MMSYSERR_NOERROR;

    /* Note: the ACMSTREAMHEADER and ACMDRVSTREAMHEADER structs are of same
     * size. some fields are private to msacm internals, and are exposed
     * in ACMSTREAMHEADER in the dwReservedDriver array
     */
    padsh = (PACMDRVSTREAMHEADER)pash;

    padsh->fdwConvert = fdwPrepare;
    padsh->padshNext = NULL;
    padsh->fdwDriver = padsh->dwDriver = 0L;

    padsh->fdwPrepared = 0;
    padsh->dwPrepared = 0;
    padsh->pbPreparedSrc = 0;
    padsh->cbPreparedSrcLength = 0;
    padsh->pbPreparedDst = 0;
    padsh->cbPreparedDstLength = 0;

    ret = MSACM_Message((HACMDRIVER)was->pDrv, ACMDM_STREAM_PREPARE, (LPARAM)&was->drvInst, (LPARAM)padsh);
    if (ret == MMSYSERR_NOERROR || ret == MMSYSERR_NOTSUPPORTED) {
	ret = MMSYSERR_NOERROR;
	padsh->fdwStatus &= ~(ACMSTREAMHEADER_STATUSF_DONE|ACMSTREAMHEADER_STATUSF_INQUEUE);
	padsh->fdwStatus |= ACMSTREAMHEADER_STATUSF_PREPARED;
	padsh->fdwPrepared = padsh->fdwStatus;
	padsh->dwPrepared = 0;
	padsh->pbPreparedSrc = padsh->pbSrc;
	padsh->cbPreparedSrcLength = padsh->cbSrcLength;
	padsh->pbPreparedDst = padsh->pbDst;
	padsh->cbPreparedDstLength = padsh->cbDstLength;
    } else {
	padsh->fdwPrepared = 0;
	padsh->dwPrepared = 0;
	padsh->pbPreparedSrc = 0;
	padsh->cbPreparedSrcLength = 0;
	padsh->pbPreparedDst = 0;
	padsh->cbPreparedDstLength = 0;
    }
    TRACE("=> (%d)\n", ret);
    return ret;
}

/***********************************************************************
 *           acmStreamReset (MSACM32.@)
 */
MMRESULT WINAPI acmStreamReset(HACMSTREAM has, DWORD fdwReset)
{
    PWINE_ACMSTREAM	was;
    MMRESULT		ret = MMSYSERR_NOERROR;

    TRACE("(%p, %d)\n", has, fdwReset);

    if (fdwReset) {
        WARN("invalid flag\n");
	ret = MMSYSERR_INVALFLAG;
    } else if ((was = ACM_GetStream(has)) == NULL) {
        WARN("invalid handle\n");
	return MMSYSERR_INVALHANDLE;
    } else if (was->drvInst.fdwOpen & ACM_STREAMOPENF_ASYNC) {
	ret = MSACM_Message((HACMDRIVER)was->pDrv, ACMDM_STREAM_RESET, (LPARAM)&was->drvInst, 0);
    }
    TRACE("=> (%d)\n", ret);
    return ret;
}

/***********************************************************************
 *           acmStreamSize (MSACM32.@)
 */
MMRESULT WINAPI acmStreamSize(HACMSTREAM has, DWORD cbInput,
			      LPDWORD pdwOutputBytes, DWORD fdwSize)
{
    PWINE_ACMSTREAM	was;
    ACMDRVSTREAMSIZE	adss;
    MMRESULT		ret;

    TRACE("(%p, %d, %p, %d)\n", has, cbInput, pdwOutputBytes, fdwSize);

    if ((was = ACM_GetStream(has)) == NULL) {
        WARN("invalid handle\n");
	return MMSYSERR_INVALHANDLE;
    }
    if ((fdwSize & ~ACM_STREAMSIZEF_QUERYMASK) != 0) {
        WARN("invalid flag\n");
	return MMSYSERR_INVALFLAG;
    }

    *pdwOutputBytes = 0L;

    switch (fdwSize & ACM_STREAMSIZEF_QUERYMASK) {
    case ACM_STREAMSIZEF_DESTINATION:
	adss.cbDstLength = cbInput;
	adss.cbSrcLength = 0;
	break;
    case ACM_STREAMSIZEF_SOURCE:
	adss.cbSrcLength = cbInput;
	adss.cbDstLength = 0;
	break;
    default:
        WARN("invalid flag\n");
	return MMSYSERR_INVALFLAG;
    }

    adss.cbStruct = sizeof(adss);
    adss.fdwSize = fdwSize;
    ret = MSACM_Message((HACMDRIVER)was->pDrv, ACMDM_STREAM_SIZE,
                            (LPARAM)&was->drvInst, (LPARAM)&adss);
    if (ret == MMSYSERR_NOERROR) {
	switch (fdwSize & ACM_STREAMSIZEF_QUERYMASK) {
	case ACM_STREAMSIZEF_DESTINATION:
	    *pdwOutputBytes = adss.cbSrcLength;
	    break;
	case ACM_STREAMSIZEF_SOURCE:
	    *pdwOutputBytes = adss.cbDstLength;
	    break;
	}
    }
    TRACE("=> (%d) [%u]\n", ret, *pdwOutputBytes);
    return ret;
}

/***********************************************************************
 *           acmStreamUnprepareHeader (MSACM32.@)
 */
MMRESULT WINAPI acmStreamUnprepareHeader(HACMSTREAM has, PACMSTREAMHEADER pash,
					 DWORD fdwUnprepare)
{
    PWINE_ACMSTREAM	was;
    MMRESULT		ret = MMSYSERR_NOERROR;
    PACMDRVSTREAMHEADER	padsh;

    TRACE("(%p, %p, %d)\n", has, pash, fdwUnprepare);

    if ((was = ACM_GetStream(has)) == NULL) {
        WARN("invalid handle\n");
	return MMSYSERR_INVALHANDLE;
    }
    if (!pash || pash->cbStruct < sizeof(ACMSTREAMHEADER)) {
        WARN("invalid parameter\n");
	return MMSYSERR_INVALPARAM;
    }
    if (!(pash->fdwStatus & ACMSTREAMHEADER_STATUSF_PREPARED)) {
        WARN("unprepared header\n");
	return ACMERR_UNPREPARED;
    }

    /* Note: the ACMSTREAMHEADER and ACMDRVSTREAMHEADER structs are of same
     * size. some fields are private to msacm internals, and are exposed
     * in ACMSTREAMHEADER in the dwReservedDriver array
     */
    padsh = (PACMDRVSTREAMHEADER)pash;

    /* check that pointers have not been modified */
    if (padsh->pbPreparedSrc != padsh->pbSrc ||
	padsh->cbPreparedSrcLength < padsh->cbSrcLength ||
	padsh->pbPreparedDst != padsh->pbDst ||
	padsh->cbPreparedDstLength < padsh->cbDstLength) {
        WARN("invalid parameter\n");
	return MMSYSERR_INVALPARAM;
    }

    padsh->fdwConvert = fdwUnprepare;

    ret = MSACM_Message((HACMDRIVER)was->pDrv, ACMDM_STREAM_UNPREPARE, (LPARAM)&was->drvInst, (LPARAM)padsh);
    if (ret == MMSYSERR_NOERROR || ret == MMSYSERR_NOTSUPPORTED) {
	ret = MMSYSERR_NOERROR;
	padsh->fdwStatus &= ~(ACMSTREAMHEADER_STATUSF_DONE|ACMSTREAMHEADER_STATUSF_INQUEUE|ACMSTREAMHEADER_STATUSF_PREPARED);
    }
    TRACE("=> (%d)\n", ret);
    return ret;
}
