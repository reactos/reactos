/*
 * Digital video MCI Wine Driver
 *
 * Copyright 1999, 2000 Eric POUECH
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

#include <string.h>
#include "private_mciavi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mciavi);

/**************************************************************************
 * 				MCIAVI_ConvertFrameToTimeFormat	[internal]
 */
static DWORD MCIAVI_ConvertFrameToTimeFormat(WINE_MCIAVI* wma, DWORD val, LPDWORD lpRet)
{
    DWORD	   ret = 0;

    switch (wma->dwMciTimeFormat) {
    case MCI_FORMAT_MILLISECONDS:
        ret = (val * wma->mah.dwMicroSecPerFrame) / 1000;
	break;
    case MCI_FORMAT_FRAMES:
	ret = val;
	break;
    default:
	WARN("Bad time format %lu!\n", wma->dwMciTimeFormat);
    }
    TRACE("val=%lu=0x%08lx [tf=%lu] => ret=%lu\n", val, val, wma->dwMciTimeFormat, ret);
    *lpRet = 0;
    return ret;
}

/**************************************************************************
 * 				MCIAVI_ConvertTimeFormatToFrame	[internal]
 */
DWORD 	MCIAVI_ConvertTimeFormatToFrame(WINE_MCIAVI* wma, DWORD val)
{
    DWORD	ret = 0;

    switch (wma->dwMciTimeFormat) {
    case MCI_FORMAT_MILLISECONDS:
	ret = (val * 1000) / wma->mah.dwMicroSecPerFrame;
	break;
    case MCI_FORMAT_FRAMES:
	ret = val;
	break;
    default:
	WARN("Bad time format %lu!\n", wma->dwMciTimeFormat);
    }
    TRACE("val=%lu=0x%08lx [tf=%lu] => ret=%lu\n", val, val, wma->dwMciTimeFormat, ret);
    return ret;
}

/***************************************************************************
 * 				MCIAVI_mciGetDevCaps		[internal]
 */
DWORD	MCIAVI_mciGetDevCaps(UINT wDevID, DWORD dwFlags,  LPMCI_GETDEVCAPS_PARMS lpParms)
{
    WINE_MCIAVI*	wma = MCIAVI_mciGetOpenDev(wDevID);
    DWORD		ret = MCIERR_UNSUPPORTED_FUNCTION;

    TRACE("(%04x, %08lX, %p)\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL) 	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    if (!(dwFlags & MCI_GETDEVCAPS_ITEM)) return MCIERR_MISSING_PARAMETER;
    if (dwFlags & MCI_TEST)	return 0;

    EnterCriticalSection(&wma->cs);

    if (dwFlags & MCI_GETDEVCAPS_ITEM) {
	switch (lpParms->dwItem) {
	case MCI_GETDEVCAPS_DEVICE_TYPE:
	    TRACE("MCI_GETDEVCAPS_DEVICE_TYPE !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(MCI_DEVTYPE_DIGITAL_VIDEO, MCI_DEVTYPE_DIGITAL_VIDEO);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_HAS_AUDIO:
	    TRACE("MCI_GETDEVCAPS_HAS_AUDIO !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_HAS_VIDEO:
	    TRACE("MCI_GETDEVCAPS_HAS_VIDEO !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_USES_FILES:
	    TRACE("MCI_GETDEVCAPS_USES_FILES !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_COMPOUND_DEVICE:
	    TRACE("MCI_GETDEVCAPS_COMPOUND_DEVICE !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_CAN_EJECT:
	    TRACE("MCI_GETDEVCAPS_CAN_EJECT !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_CAN_PLAY:
	    TRACE("MCI_GETDEVCAPS_CAN_PLAY !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_CAN_RECORD:
	    TRACE("MCI_GETDEVCAPS_CAN_RECORD !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_CAN_SAVE:
	    TRACE("MCI_GETDEVCAPS_CAN_SAVE !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_DGV_GETDEVCAPS_CAN_REVERSE:
	    TRACE("MCI_DGV_GETDEVCAPS_CAN_REVERSE !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE); /* FIXME */
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_DGV_GETDEVCAPS_CAN_STRETCH:
	    TRACE("MCI_DGV_GETDEVCAPS_CAN_STRETCH !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE); /* FIXME */
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_DGV_GETDEVCAPS_CAN_LOCK:
	    TRACE("MCI_DGV_GETDEVCAPS_CAN_LOCK !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_DGV_GETDEVCAPS_CAN_FREEZE:
	    TRACE("MCI_DGV_GETDEVCAPS_CAN_FREEZE !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_DGV_GETDEVCAPS_CAN_STR_IN:
	    TRACE("MCI_DGV_GETDEVCAPS_CAN_STRETCH_INPUT !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_DGV_GETDEVCAPS_HAS_STILL:
	    TRACE("MCI_DGV_GETDEVCAPS_HAS_STILL !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_DGV_GETDEVCAPS_CAN_TEST:
	    TRACE("MCI_DGV_GETDEVCAPS_CAN_TEST !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_DGV_GETDEVCAPS_PALETTES:
	    TRACE("MCI_DGV_GETDEVCAPS_PALETTES !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE); /* FIXME */
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	/* w2k does not know MAX_WINDOWS or MAX/MINIMUM_RATE */
	default:
            FIXME("Unknown capability (%08lx) !\n", lpParms->dwItem);
            break;
	}
    }

    LeaveCriticalSection(&wma->cs);
    return ret;
}

/***************************************************************************
 * 				MCIAVI_mciInfo			[internal]
 */
DWORD	MCIAVI_mciInfo(UINT wDevID, DWORD dwFlags, LPMCI_DGV_INFO_PARMSW lpParms)
{
    LPCWSTR		str = 0;
    WINE_MCIAVI*	wma = MCIAVI_mciGetOpenDev(wDevID);
    DWORD		ret = 0;

    if (lpParms == NULL || lpParms->lpstrReturn == NULL)
	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL) return MCIERR_INVALID_DEVICE_ID;
    if (dwFlags & MCI_TEST)	return 0;

    TRACE("buf=%p, len=%lu\n", lpParms->lpstrReturn, lpParms->dwRetSize);

    EnterCriticalSection(&wma->cs);

    if (dwFlags & MCI_INFO_PRODUCT)
	str = L"Wine's AVI player";
    else if (dwFlags & MCI_INFO_VERSION)
	str = L"1.1";
    else if (dwFlags & MCI_INFO_FILE)
	str = wma->lpFileName;
    else {
	WARN("Don't know this info command (%lu)\n", dwFlags);
	ret = MCIERR_UNRECOGNIZED_COMMAND;
    }
    if (!ret) {
	/* Only mciwave, mciseq and mcicda set dwRetSize (since NT). */
	lstrcpynW(lpParms->lpstrReturn, str ? str : L"", lpParms->dwRetSize);
    }
    LeaveCriticalSection(&wma->cs);
    return ret;
}

/***************************************************************************
 * 				MCIAVI_mciSet			[internal]
 */
DWORD	MCIAVI_mciSet(UINT wDevID, DWORD dwFlags, LPMCI_DGV_SET_PARMS lpParms)
{
    WINE_MCIAVI*	wma = MCIAVI_mciGetOpenDev(wDevID);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    if (dwFlags & MCI_TEST)	return 0;

    EnterCriticalSection(&wma->cs);

    if (dwFlags & MCI_SET_TIME_FORMAT) {
	switch (lpParms->dwTimeFormat) {
	case MCI_FORMAT_MILLISECONDS:
	    TRACE("MCI_FORMAT_MILLISECONDS !\n");
	    wma->dwMciTimeFormat = MCI_FORMAT_MILLISECONDS;
	    break;
	case MCI_FORMAT_FRAMES:
	    TRACE("MCI_FORMAT_FRAMES !\n");
	    wma->dwMciTimeFormat = MCI_FORMAT_FRAMES;
	    break;
	default:
            WARN("Bad time format %lu!\n", lpParms->dwTimeFormat);
            LeaveCriticalSection(&wma->cs);
	    return MCIERR_BAD_TIME_FORMAT;
	}
    }

    if (dwFlags & MCI_SET_DOOR_OPEN) {
	TRACE("No support for door open !\n");
        LeaveCriticalSection(&wma->cs);
	return MCIERR_UNSUPPORTED_FUNCTION;
    }
    if (dwFlags & MCI_SET_DOOR_CLOSED) {
	TRACE("No support for door close !\n");
        LeaveCriticalSection(&wma->cs);
	return MCIERR_UNSUPPORTED_FUNCTION;
    }

    if (dwFlags & MCI_SET_ON) {
	const char *szVideo="";
	const char *szAudio="";
	const char *szSeek="";

	if (dwFlags & MCI_SET_VIDEO) {
	    szVideo = " video";
	    wma->dwSet |= 4;
	}
	if (dwFlags & MCI_SET_AUDIO) {
	    switch (lpParms->dwAudio) {
	    case MCI_SET_AUDIO_ALL:
		szAudio = " audio all";
		wma->dwSet |= 3;
		break;
	    case MCI_SET_AUDIO_LEFT:
		szAudio = " audio left";
		wma->dwSet |= 1;
		break;
	    case MCI_SET_AUDIO_RIGHT:
		szAudio = " audio right";
		wma->dwSet |= 2;
		break;
	    default:
		szAudio = " audio unknown";
		WARN("Unknown audio channel %lu\n", lpParms->dwAudio);
		break;
	    }
	}
	if (dwFlags & MCI_DGV_SET_SEEK_EXACTLY) {
	    szSeek = " seek_exactly";
	}
	FIXME("MCI_SET_ON:%s%s%s\n", szVideo, szAudio, szSeek);
    }

    if (dwFlags & MCI_SET_OFF) {
	const char *szVideo="";
	const char *szAudio="";
	const char *szSeek="";

	if (dwFlags & MCI_SET_VIDEO) {
	    szVideo = " video";
	    wma->dwSet &= ~4;
	}
	if (dwFlags & MCI_SET_AUDIO) {
	    switch (lpParms->dwAudio) {
	    case MCI_SET_AUDIO_ALL:
		szAudio = " audio all";
		wma->dwSet &= ~3;
		break;
	    case MCI_SET_AUDIO_LEFT:
		szAudio = " audio left";
		wma->dwSet &= ~2;
		break;
	    case MCI_SET_AUDIO_RIGHT:
		szAudio = " audio right";
		wma->dwSet &= ~2;
		break;
	    default:
		szAudio = " audio unknown";
		WARN("Unknown audio channel %lu\n", lpParms->dwAudio);
		break;
	    }
	}
	if (dwFlags & MCI_DGV_SET_SEEK_EXACTLY) {
	    szSeek = " seek_exactly";
	}
	FIXME("MCI_SET_OFF:%s%s%s\n", szVideo, szAudio, szSeek);
    }
    if (dwFlags & MCI_DGV_SET_FILEFORMAT) {
	LPCSTR	str = "save";
	if (dwFlags & MCI_DGV_SET_STILL)
	    str = "capture";

	switch (lpParms->dwFileFormat) {
	case MCI_DGV_FF_AVI: 	FIXME("Setting file format (%s) to 'AVI'\n", str); 	break;
	case MCI_DGV_FF_AVSS: 	FIXME("Setting file format (%s) to 'AVSS'\n", str);	break;
	case MCI_DGV_FF_DIB: 	FIXME("Setting file format (%s) to 'DIB'\n", str);	break;
	case MCI_DGV_FF_JFIF: 	FIXME("Setting file format (%s) to 'JFIF'\n", str);	break;
	case MCI_DGV_FF_JPEG: 	FIXME("Setting file format (%s) to 'JPEG'\n", str);	break;
	case MCI_DGV_FF_MPEG: 	FIXME("Setting file format (%s) to 'MPEG'\n", str); 	break;
	case MCI_DGV_FF_RDIB:	FIXME("Setting file format (%s) to 'RLE DIB'\n", str);	break;
	case MCI_DGV_FF_RJPEG: 	FIXME("Setting file format (%s) to 'RJPEG'\n", str);	break;
	default:		FIXME("Setting unknown file format (%s): %ld\n", str, lpParms->dwFileFormat);
	}
    }

    if (dwFlags & MCI_DGV_SET_SPEED) {
	FIXME("Setting speed to %ld\n", lpParms->dwSpeed);
    }

    LeaveCriticalSection(&wma->cs);
    return 0;
}

/***************************************************************************
 * 				MCIAVI_mciStatus			[internal]
 */
DWORD	MCIAVI_mciStatus(UINT wDevID, DWORD dwFlags, LPMCI_DGV_STATUS_PARMSW lpParms)
{
    WINE_MCIAVI*	wma = MCIAVI_mciGetOpenDev(wDevID);
    DWORD		ret = 0;

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;
    if (!(dwFlags & MCI_STATUS_ITEM))	return MCIERR_MISSING_PARAMETER;
    if (dwFlags & MCI_TEST)	return 0;

    EnterCriticalSection(&wma->cs);

    if (dwFlags & MCI_STATUS_ITEM) {
	switch (lpParms->dwItem) {
	case MCI_STATUS_CURRENT_TRACK:
	    lpParms->dwReturn = 1;
	    TRACE("MCI_STATUS_CURRENT_TRACK => %Iu\n", lpParms->dwReturn);
	    break;
	case MCI_STATUS_LENGTH:
	    if (!wma->hFile) {
		lpParms->dwReturn = 0;
                LeaveCriticalSection(&wma->cs);
		return MCIERR_UNSUPPORTED_FUNCTION;
	    }
	    /* only one track in file is currently handled, so don't take care of MCI_TRACK flag */
	    lpParms->dwReturn = MCIAVI_ConvertFrameToTimeFormat(wma, wma->mah.dwTotalFrames, &ret);
	    TRACE("MCI_STATUS_LENGTH => %Iu\n", lpParms->dwReturn);
	    break;
	case MCI_STATUS_MODE:
 	    lpParms->dwReturn = MAKEMCIRESOURCE(wma->dwStatus, wma->dwStatus);
	    ret = MCI_RESOURCE_RETURNED;
           TRACE("MCI_STATUS_MODE => 0x%04x\n", LOWORD(lpParms->dwReturn));
	    break;
	case MCI_STATUS_MEDIA_PRESENT:
	    TRACE("MCI_STATUS_MEDIA_PRESENT => TRUE\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_STATUS_NUMBER_OF_TRACKS:
	    lpParms->dwReturn = 1;
	    TRACE("MCI_STATUS_NUMBER_OF_TRACKS => %Iu\n", lpParms->dwReturn);
	    break;
	case MCI_STATUS_POSITION:
	    if (!wma->hFile) {
		lpParms->dwReturn = 0;
                LeaveCriticalSection(&wma->cs);
		return MCIERR_UNSUPPORTED_FUNCTION;
	    }
	    /* only one track in file is currently handled, so don't take care of MCI_TRACK flag */
	    lpParms->dwReturn = MCIAVI_ConvertFrameToTimeFormat(wma,
							     (dwFlags & MCI_STATUS_START) ? 0 : wma->dwCurrVideoFrame,
							     &ret);
	    TRACE("MCI_STATUS_POSITION %s => %Iu\n",
		  (dwFlags & MCI_STATUS_START) ? "start" : "current", lpParms->dwReturn);
	    break;
	case MCI_STATUS_READY:
	    lpParms->dwReturn = (wma->dwStatus == MCI_MODE_NOT_READY) ?
		MAKEMCIRESOURCE(FALSE, MCI_FALSE) : MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    TRACE("MCI_STATUS_READY = %u\n", LOWORD(lpParms->dwReturn));
	    break;
	case MCI_STATUS_TIME_FORMAT:
	    lpParms->dwReturn = MAKEMCIRESOURCE(wma->dwMciTimeFormat,
                                wma->dwMciTimeFormat + MCI_FORMAT_RETURN_BASE);
	    TRACE("MCI_STATUS_TIME_FORMAT => %u\n", LOWORD(lpParms->dwReturn));
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_DGV_STATUS_AUDIO:
	    lpParms->dwReturn = (wma->dwSet & 3) ?
		MAKEMCIRESOURCE(MCI_ON, MCI_ON_S) : MAKEMCIRESOURCE(MCI_OFF, MCI_OFF_S);
	    ret = MCI_RESOURCE_RETURNED|MCI_RESOURCE_DRIVER;
	    TRACE("MCI_STATUS_AUDIO = %u\n", LOWORD(lpParms->dwReturn));
	    break;
	case MCI_DGV_STATUS_VIDEO:
	    lpParms->dwReturn = (wma->dwSet & 4) ?
		MAKEMCIRESOURCE(MCI_ON, MCI_ON_S) : MAKEMCIRESOURCE(MCI_OFF, MCI_OFF_S);
	    ret = MCI_RESOURCE_RETURNED|MCI_RESOURCE_DRIVER;
	    TRACE("MCI_STATUS_VIDEO = %u\n", LOWORD(lpParms->dwReturn));
	    break;

#if 0
	case MCI_AVI_STATUS_AUDIO_BREAKS:
	case MCI_AVI_STATUS_FRAMES_SKIPPED:
	case MCI_AVI_STATUS_LAST_PLAY_SPEED:
	case MCI_DGV_STATUS_AUDIO_INPUT:
	case MCI_DGV_STATUS_AUDIO_RECORD:
	case MCI_DGV_STATUS_AUDIO_SOURCE:
	case MCI_DGV_STATUS_AVGBYTESPERSEC:
	case MCI_DGV_STATUS_BASS:
	case MCI_DGV_STATUS_BITSPERSAMPLE:
	case MCI_DGV_STATUS_BLOCKALIGN:
	case MCI_DGV_STATUS_BRIGHTNESS:
	case MCI_DGV_STATUS_COLOR:
	case MCI_DGV_STATUS_CONTRAST:
	case MCI_DGV_STATUS_FILEFORMAT:
	case MCI_DGV_STATUS_FILE_MODE:
	case MCI_DGV_STATUS_FILE_COMPLETION:
	case MCI_DGV_STATUS_GAMMA:
#endif
	case MCI_DGV_STATUS_BITSPERPEL:
	    lpParms->dwReturn = wma->inbih->biBitCount;
	    TRACE("MCI_DGV_STATUS_BITSPERPEL => %Iu\n", lpParms->dwReturn);
	    break;
	case MCI_DGV_STATUS_HPAL:
	    lpParms->dwReturn = 0;
	    TRACE("MCI_DGV_STATUS_HPAL => %Ix\n", lpParms->dwReturn);
	    break;
	case MCI_DGV_STATUS_HWND:
           lpParms->dwReturn = (DWORD_PTR)wma->hWndPaint;
           TRACE("MCI_DGV_STATUS_HWND => %p\n", wma->hWndPaint);
	   break;
	case MCI_DGV_STATUS_WINDOW_VISIBLE:
	    lpParms->dwReturn = IsWindowVisible(wma->hWndPaint) ?
		MAKEMCIRESOURCE(TRUE, MCI_TRUE) : MAKEMCIRESOURCE(FALSE, MCI_FALSE);
	    ret = MCI_RESOURCE_RETURNED;
	    TRACE("MCI_STATUS_WINDOW_VISIBLE = %u\n", LOWORD(lpParms->dwReturn));
	    break;
	case MCI_DGV_STATUS_WINDOW_MINIMIZED:
	    lpParms->dwReturn = IsIconic(wma->hWndPaint) ?
		MAKEMCIRESOURCE(TRUE, MCI_TRUE) : MAKEMCIRESOURCE(FALSE, MCI_FALSE);
	    ret = MCI_RESOURCE_RETURNED;
	    TRACE("MCI_STATUS_WINDOW_MINIMIZED = %u\n", LOWORD(lpParms->dwReturn));
	    break;
	case MCI_DGV_STATUS_WINDOW_MAXIMIZED:
	    lpParms->dwReturn = IsZoomed(wma->hWndPaint) ?
		MAKEMCIRESOURCE(TRUE, MCI_TRUE) : MAKEMCIRESOURCE(FALSE, MCI_FALSE);
	    ret = MCI_RESOURCE_RETURNED;
	    TRACE("MCI_STATUS_WINDOW_MMAXIMIZED = %u\n", LOWORD(lpParms->dwReturn));
	    break;
	case MCI_DGV_STATUS_SPEED:
	    lpParms->dwReturn = 1000;
	    TRACE("MCI_DGV_STATUS_SPEED = %Iu\n", lpParms->dwReturn);
	    break;
	case MCI_DGV_STATUS_FRAME_RATE:
	    /* FIXME: 1000 is a settable speed multiplier */
	    lpParms->dwReturn = MulDiv(1000000,1000,wma->mah.dwMicroSecPerFrame);
	    TRACE("MCI_DGV_STATUS_FRAME_RATE = %Iu\n", lpParms->dwReturn);
	    break;
	case MCI_DGV_STATUS_FORWARD:
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    TRACE("MCI_DGV_STATUS_FORWARD = %u\n", LOWORD(lpParms->dwReturn));
	    break;
	case MCI_DGV_STATUS_PAUSE_MODE:
	    if (wma->dwStatus != MCI_MODE_PAUSE) {
		LeaveCriticalSection(&wma->cs);
		return MCIERR_NONAPPLICABLE_FUNCTION;
	    }
	    lpParms->dwReturn = MAKEMCIRESOURCE(MCI_MODE_PLAY, MCI_MODE_PLAY);
	    ret = MCI_RESOURCE_RETURNED;
	    TRACE("MCI_STATUS_MODE => 0x%04x\n", LOWORD(lpParms->dwReturn));
	    break;
	case MCI_DGV_STATUS_AUDIO_STREAM:
	   lpParms->dwReturn = wma->audio_stream_n;
	   TRACE("MCI_DGV_STATUS_AUDIO_STREAM => %Iu\n", lpParms->dwReturn);
	   break;
#if 0
	case MCI_DGV_STATUS_KEY_COLOR:
	case MCI_DGV_STATUS_KEY_INDEX:
	case MCI_DGV_STATUS_MONITOR:
	case MCI_DGV_MONITOR_FILE:
	case MCI_DGV_MONITOR_INPUT:
	case MCI_DGV_STATUS_MONITOR_METHOD:
	case MCI_DGV_STATUS_SAMPLESPERSECOND:
	case MCI_DGV_STATUS_SEEK_EXACTLY:
	case MCI_DGV_STATUS_SHARPNESS:
	case MCI_DGV_STATUS_SIZE:
	case MCI_DGV_STATUS_SMPTE:
	case MCI_DGV_STATUS_STILL_FILEFORMAT:
	case MCI_DGV_STATUS_TINT:
	case MCI_DGV_STATUS_TREBLE:
	case MCI_DGV_STATUS_UNSAVED:
	case MCI_DGV_STATUS_VIDEO_RECORD:
	case MCI_DGV_STATUS_VIDEO_SOURCE:
	case MCI_DGV_STATUS_VIDEO_SRC_NUM:
	case MCI_DGV_STATUS_VIDEO_STREAM:
	case MCI_DGV_STATUS_VOLUME:
#endif
	default:
            FIXME("Unknown command %08lX !\n", lpParms->dwItem);
            TRACE("(%04x, %08lX, %p)\n", wDevID, dwFlags, lpParms);
            LeaveCriticalSection(&wma->cs);
	    return MCIERR_UNSUPPORTED_FUNCTION;
	}
    }

    if (dwFlags & MCI_NOTIFY) {
	TRACE("MCI_NOTIFY_SUCCESSFUL %08IX !\n", lpParms->dwCallback);
	mciDriverNotify(HWND_32(LOWORD(lpParms->dwCallback)),
                       wDevID, MCI_NOTIFY_SUCCESSFUL);
    }
    LeaveCriticalSection(&wma->cs);
    return ret;
}
