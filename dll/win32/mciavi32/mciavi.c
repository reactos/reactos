/*
 * Digital video MCI Wine Driver
 *
 * Copyright 1999, 2000 Eric POUECH
 * Copyright 2003 Dmitry Timoshkov
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

/* TODO list :
 *	- handling of palettes
 *	- recording (which input devices ?), a cam recorder ?
 *	- lots of messages still need to be handled (cf FIXME)
 *	- synchronization between audio and video (especially for interleaved
 *	  files)
 *	- robustness when reading file can be enhanced
 *	- better move the AVI handling part to avifile DLL and make use of it
 *	- some files appear to have more than one audio stream (we only play the
 *	  first one)
 *	- some files contain an index of audio/video frame. Better use it,
 *	  instead of rebuilding it
 *	- stopping while playing a file with sound blocks until all buffered
 *        audio is played... still should be stopped ASAP
 */

#include <string.h>
#include "private_mciavi.h"
#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(mciavi);

static DWORD MCIAVI_mciStop(UINT, DWORD, LPMCI_GENERIC_PARMS);

/*======================================================================*
 *                  	    MCI AVI implementation			*
 *======================================================================*/

HINSTANCE MCIAVI_hInstance = 0;

/***********************************************************************
 *		DllMain (MCIAVI.0)
 */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstDLL);
	MCIAVI_hInstance = hInstDLL;
	break;
    }
    return TRUE;
}

/**************************************************************************
 * 				MCIAVI_drvOpen			[internal]
 */
static	DWORD	MCIAVI_drvOpen(LPCWSTR str, LPMCI_OPEN_DRIVER_PARMSW modp)
{
    WINE_MCIAVI*	wma;
    static const WCHAR mciAviWStr[] = {'M','C','I','A','V','I',0};

    TRACE("%s, %p\n", debugstr_w(str), modp);

    /* session instance */
    if (!modp) return 0xFFFFFFFF;

    if (!MCIAVI_RegisterClass()) return 0;

    wma = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WINE_MCIAVI));
    if (!wma)
	return 0;

    InitializeCriticalSection(&wma->cs);
    wma->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": WINE_MCIAVI.cs");
    wma->ack_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    wma->hStopEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    wma->wDevID = modp->wDeviceID;
    wma->wCommandTable = mciLoadCommandResource(MCIAVI_hInstance, mciAviWStr, 0);
    modp->wCustomCommandTable = wma->wCommandTable;
    modp->wType = MCI_DEVTYPE_DIGITAL_VIDEO;
    mciSetDriverData(wma->wDevID, (DWORD_PTR)wma);

    return modp->wDeviceID;
}

/**************************************************************************
 * 				MCIAVI_drvClose		[internal]
 */
static	DWORD	MCIAVI_drvClose(DWORD dwDevID)
{
    WINE_MCIAVI *wma;

    TRACE("%04x\n", dwDevID);

    /* finish all outstanding things */
    MCIAVI_mciClose(dwDevID, MCI_WAIT, NULL);

    wma = (WINE_MCIAVI*)mciGetDriverData(dwDevID);

    if (wma) {
        MCIAVI_UnregisterClass();

        EnterCriticalSection(&wma->cs);

	mciSetDriverData(dwDevID, 0);
	mciFreeCommandResource(wma->wCommandTable);

        CloseHandle(wma->ack_event);
        CloseHandle(wma->hStopEvent);

        LeaveCriticalSection(&wma->cs);
        wma->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&wma->cs);

	HeapFree(GetProcessHeap(), 0, wma);
	return 1;
    }
    return (dwDevID == 0xFFFFFFFF) ? 1 : 0;
}

/**************************************************************************
 * 				MCIAVI_drvConfigure		[internal]
 */
static	DWORD	MCIAVI_drvConfigure(DWORD dwDevID)
{
    WINE_MCIAVI *wma;

    TRACE("%04x\n", dwDevID);

    MCIAVI_mciStop(dwDevID, MCI_WAIT, NULL);

    wma = (WINE_MCIAVI*)mciGetDriverData(dwDevID);

    if (wma) {
	MessageBoxA(0, "Sample AVI Wine Driver !", "MM-Wine Driver", MB_OK);
	return 1;
    }
    return 0;
}

/**************************************************************************
 * 				MCIAVI_mciGetOpenDev		[internal]
 */
WINE_MCIAVI*  MCIAVI_mciGetOpenDev(UINT wDevID)
{
    WINE_MCIAVI*	wma = (WINE_MCIAVI*)mciGetDriverData(wDevID);

    if (wma == NULL || wma->nUseCount == 0) {
	WARN("Invalid wDevID=%u\n", wDevID);
	return 0;
    }
    return wma;
}

static void MCIAVI_CleanUp(WINE_MCIAVI* wma)
{
    /* to prevent handling in WindowProc */
    wma->dwStatus = MCI_MODE_NOT_READY;
    if (wma->hFile) {
	mmioClose(wma->hFile, 0);
	wma->hFile = 0;

        HeapFree(GetProcessHeap(), 0, wma->lpFileName);
        wma->lpFileName = NULL;

        HeapFree(GetProcessHeap(), 0, wma->lpVideoIndex);
	wma->lpVideoIndex = NULL;
        HeapFree(GetProcessHeap(), 0, wma->lpAudioIndex);
	wma->lpAudioIndex = NULL;
	if (wma->hic)		ICClose(wma->hic);
	wma->hic = 0;
        HeapFree(GetProcessHeap(), 0, wma->inbih);
	wma->inbih = NULL;
        HeapFree(GetProcessHeap(), 0, wma->outbih);
	wma->outbih = NULL;
        HeapFree(GetProcessHeap(), 0, wma->indata);
	wma->indata = NULL;
        HeapFree(GetProcessHeap(), 0, wma->outdata);
	wma->outdata = NULL;
    	if (wma->hbmFrame)	DeleteObject(wma->hbmFrame);
	wma->hbmFrame = 0;
	if (wma->hWnd)		DestroyWindow(wma->hWnd);
	wma->hWnd = 0;

        HeapFree(GetProcessHeap(), 0, wma->lpWaveFormat);
	wma->lpWaveFormat = 0;

	memset(&wma->mah, 0, sizeof(wma->mah));
	memset(&wma->ash_video, 0, sizeof(wma->ash_video));
	memset(&wma->ash_audio, 0, sizeof(wma->ash_audio));
	wma->dwCurrVideoFrame = wma->dwCurrAudioBlock = 0;
        wma->dwCachedFrame = -1;
    }
}

/***************************************************************************
 * 				MCIAVI_mciOpen			[internal]
 */
static	DWORD	MCIAVI_mciOpen(UINT wDevID, DWORD dwFlags,
                               LPMCI_DGV_OPEN_PARMSW lpOpenParms)
{
    WINE_MCIAVI *wma;
    LRESULT		dwRet = 0;

    TRACE("(%04x, %08X, %p)\n", wDevID, dwFlags, lpOpenParms);

    MCIAVI_mciStop(wDevID, MCI_WAIT, NULL);

    if (lpOpenParms == NULL) 		return MCIERR_NULL_PARAMETER_BLOCK;

    wma = (WINE_MCIAVI *)mciGetDriverData(wDevID);
    if (wma == NULL)			return MCIERR_INVALID_DEVICE_ID;

    EnterCriticalSection(&wma->cs);

    if (wma->nUseCount > 0) {
	/* The driver is already open on this channel */
	/* If the driver was opened shareable before and this open specifies */
	/* shareable then increment the use count */
	if (wma->fShareable && (dwFlags & MCI_OPEN_SHAREABLE))
	    ++wma->nUseCount;
	else
        {
            LeaveCriticalSection(&wma->cs);
	    return MCIERR_MUST_USE_SHAREABLE;
        }
    } else {
	wma->nUseCount = 1;
	wma->fShareable = dwFlags & MCI_OPEN_SHAREABLE;
    }

    wma->dwStatus = MCI_MODE_NOT_READY;

    if (dwFlags & MCI_OPEN_ELEMENT) {
	if (dwFlags & MCI_OPEN_ELEMENT_ID) {
	    /* could it be that (DWORD)lpOpenParms->lpstrElementName
	     * contains the hFile value ?
	     */
	    dwRet = MCIERR_UNRECOGNIZED_COMMAND;
	} else if (strlenW(lpOpenParms->lpstrElementName) > 0) {
	    /* FIXME : what should be done id wma->hFile is already != 0, or the driver is playin' */
	    TRACE("MCI_OPEN_ELEMENT %s!\n", debugstr_w(lpOpenParms->lpstrElementName));

            if (lpOpenParms->lpstrElementName && (strlenW(lpOpenParms->lpstrElementName) > 0))
            {
                wma->lpFileName = HeapAlloc(GetProcessHeap(), 0, (strlenW(lpOpenParms->lpstrElementName) + 1) * sizeof(WCHAR));
                strcpyW(wma->lpFileName, lpOpenParms->lpstrElementName);

		wma->hFile = mmioOpenW(lpOpenParms->lpstrElementName, NULL,
				       MMIO_ALLOCBUF | MMIO_DENYWRITE | MMIO_READ);

		if (wma->hFile == 0) {
		    WARN("can't find file=%s!\n", debugstr_w(lpOpenParms->lpstrElementName));
		    dwRet = MCIERR_FILE_NOT_FOUND;
		} else {
		    if (!MCIAVI_GetInfo(wma))
			dwRet = MCIERR_INVALID_FILE;
		    else if (!MCIAVI_OpenVideo(wma))
			dwRet = MCIERR_CANNOT_LOAD_DRIVER;
		    else if (!MCIAVI_CreateWindow(wma, dwFlags, lpOpenParms))
			dwRet = MCIERR_CREATEWINDOW;
		}
	    }
	} else {
	    FIXME("Don't record yet\n");
	    dwRet = MCIERR_UNSUPPORTED_FUNCTION;
	}
    }

    if (dwRet == 0) {
        TRACE("lpOpenParms->wDeviceID = %04x\n", lpOpenParms->wDeviceID);

	wma->dwStatus = MCI_MODE_STOP;
	wma->dwMciTimeFormat = MCI_FORMAT_FRAMES;
    } else {
	MCIAVI_CleanUp(wma);
    }

    LeaveCriticalSection(&wma->cs);
    return dwRet;
}

/***************************************************************************
 * 				MCIAVI_mciClose			[internal]
 */
DWORD MCIAVI_mciClose(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCIAVI *wma;
    DWORD		dwRet = 0;

    TRACE("(%04x, %08X, %p)\n", wDevID, dwFlags, lpParms);

    MCIAVI_mciStop(wDevID, MCI_WAIT, NULL);

    wma = MCIAVI_mciGetOpenDev(wDevID);
    if (wma == NULL) 	return MCIERR_INVALID_DEVICE_ID;

    EnterCriticalSection(&wma->cs);

    if (wma->nUseCount == 1) {
	if (wma->dwStatus != MCI_MODE_STOP)
           dwRet = MCIAVI_mciStop(wDevID, MCI_WAIT, NULL);
    	MCIAVI_CleanUp(wma);

	if ((dwFlags & MCI_NOTIFY) && lpParms) {
	    mciDriverNotify(HWND_32(LOWORD(lpParms->dwCallback)),
                           wDevID,
			    MCI_NOTIFY_SUCCESSFUL);
	}
        LeaveCriticalSection(&wma->cs);
	return dwRet;
    }
    wma->nUseCount--;

    LeaveCriticalSection(&wma->cs);
    return dwRet;
}

static DWORD MCIAVI_mciPlay(UINT wDevID, DWORD dwFlags, LPMCI_PLAY_PARMS lpParms);

struct MCIAVI_play_data
{
    MCIDEVICEID wDevID;
    DWORD flags;
    MCI_PLAY_PARMS params;
};

/*
 * MCIAVI_mciPlay_thread
 *
 * FIXME: probably should use a common worker thread created at the driver
 * load time and queue all async commands to it.
 */
static DWORD WINAPI MCIAVI_mciPlay_thread(LPVOID arg)
{
    struct MCIAVI_play_data *data = (struct MCIAVI_play_data *)arg;
    DWORD ret;

    TRACE("In thread before async play command (id %08x, flags %08x)\n", data->wDevID, data->flags);
    ret = MCIAVI_mciPlay(data->wDevID, data->flags | MCI_WAIT, &data->params);
    TRACE("In thread after async play command (id %08x, flags %08x)\n", data->wDevID, data->flags);

    HeapFree(GetProcessHeap(), 0, data);
    return ret;
}

/*
 * MCIAVI_mciPlay_async
 */
static DWORD MCIAVI_mciPlay_async(WINE_MCIAVI *wma, DWORD dwFlags, LPMCI_PLAY_PARMS lpParams)
{
    HANDLE handle, ack_event = wma->ack_event;
    struct MCIAVI_play_data *data = HeapAlloc(GetProcessHeap(), 0, sizeof(struct MCIAVI_play_data));

    if (!data) return MCIERR_OUT_OF_MEMORY;

    data->wDevID = wma->wDevID;
    data->flags = dwFlags;
    data->params = *lpParams;

    if (!(handle = CreateThread(NULL, 0, MCIAVI_mciPlay_thread, data, 0, NULL)))
    {
        WARN("Couldn't create thread for async play, playing synchronously\n");
        return MCIAVI_mciPlay_thread(data);
    }
    SetThreadPriority(handle, THREAD_PRIORITY_TIME_CRITICAL);
    CloseHandle(handle);
    /* wait until the thread starts up, so the app could see a changed status */
    WaitForSingleObject(ack_event, INFINITE);
    TRACE("Async play has started\n");
    return 0;
}

/***************************************************************************
 * 				MCIAVI_mciPlay			[internal]
 */
static	DWORD	MCIAVI_mciPlay(UINT wDevID, DWORD dwFlags, LPMCI_PLAY_PARMS lpParms)
{
    WINE_MCIAVI *wma;
    DWORD		tc;
    DWORD		frameTime;
    DWORD		delta;
    DWORD		dwRet;
    LPWAVEHDR		waveHdr = NULL;
    unsigned		i, nHdr = 0;
    DWORD		dwFromFrame, dwToFrame;

    TRACE("(%04x, %08X, %p)\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIAVI_mciGetOpenDev(wDevID);
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    EnterCriticalSection(&wma->cs);

    if (!wma->hFile)
    {
        LeaveCriticalSection(&wma->cs);
        return MCIERR_FILE_NOT_FOUND;
    }
    if (!wma->hWndPaint)
    {
        LeaveCriticalSection(&wma->cs);
        return MCIERR_NO_WINDOW;
    }

    LeaveCriticalSection(&wma->cs);

    if (!(dwFlags & MCI_WAIT))
        return MCIAVI_mciPlay_async(wma, dwFlags, lpParms);

    if (!(GetWindowLongW(wma->hWndPaint, GWL_STYLE) & WS_VISIBLE))
        ShowWindow(wma->hWndPaint, SW_SHOWNA);

    EnterCriticalSection(&wma->cs);

    dwFromFrame = wma->dwCurrVideoFrame;
    dwToFrame = wma->dwPlayableVideoFrames - 1;

    if (lpParms && (dwFlags & MCI_FROM)) {
	dwFromFrame = MCIAVI_ConvertTimeFormatToFrame(wma, lpParms->dwFrom);
    }
    if (lpParms && (dwFlags & MCI_TO)) {
	dwToFrame = MCIAVI_ConvertTimeFormatToFrame(wma, lpParms->dwTo);
    }
    if (dwToFrame >= wma->dwPlayableVideoFrames)
	dwToFrame = wma->dwPlayableVideoFrames - 1;

    TRACE("Playing from frame=%u to frame=%u\n", dwFromFrame, dwToFrame);

    wma->dwCurrVideoFrame = dwFromFrame;
    wma->dwToVideoFrame = dwToFrame;

    /* if already playing exit */
    if (wma->dwStatus == MCI_MODE_PLAY)
    {
        LeaveCriticalSection(&wma->cs);
        SetEvent(wma->ack_event);
        return 0;
    }

    if (wma->dwToVideoFrame <= wma->dwCurrVideoFrame)
    {
        dwRet = 0;
        SetEvent(wma->ack_event);
        goto mci_play_done;
    }

    wma->dwStatus = MCI_MODE_PLAY;
    /* signal the state change */
    SetEvent(wma->ack_event);

    if (dwFlags & (MCI_DGV_PLAY_REPEAT|MCI_DGV_PLAY_REVERSE|MCI_MCIAVI_PLAY_WINDOW|MCI_MCIAVI_PLAY_FULLSCREEN))
	FIXME("Unsupported flag %08x\n", dwFlags);

    /* time is in microseconds, we should convert it to milliseconds */
    frameTime = (wma->mah.dwMicroSecPerFrame + 500) / 1000;

    if (wma->lpWaveFormat) {
       if (MCIAVI_OpenAudio(wma, &nHdr, &waveHdr) != 0)
        {
            /* can't play audio */
            HeapFree(GetProcessHeap(), 0, wma->lpWaveFormat);
            wma->lpWaveFormat = NULL;
        }
        else
	/* fill the queue with as many wave headers as possible */
	MCIAVI_PlayAudioBlocks(wma, nHdr, waveHdr);
    }

    while (wma->dwStatus == MCI_MODE_PLAY)
    {
        HDC hDC;
        DWORD ret;

	tc = GetTickCount();

        hDC = wma->hWndPaint ? GetDC(wma->hWndPaint) : 0;
        if (hDC)
        {
            MCIAVI_PaintFrame(wma, hDC);
            ReleaseDC(wma->hWndPaint, hDC);
        }

	if (wma->lpWaveFormat) {
            HANDLE events[2];

            events[0] = wma->hStopEvent;
            events[1] = wma->hEvent;

	    MCIAVI_PlayAudioBlocks(wma, nHdr, waveHdr);
	    delta = GetTickCount() - tc;

            LeaveCriticalSection(&wma->cs);
            ret = WaitForMultipleObjects(2, events, FALSE, (delta >= frameTime) ? 0 : frameTime - delta);
            EnterCriticalSection(&wma->cs);

            if (ret == WAIT_OBJECT_0 || wma->dwStatus != MCI_MODE_PLAY) break;
	}

	delta = GetTickCount() - tc;
	if (delta < frameTime)
            delta = frameTime - delta;
        else
            delta = 0;

        LeaveCriticalSection(&wma->cs);
        ret = WaitForMultipleObjects(1, &wma->hStopEvent, FALSE, delta);
        EnterCriticalSection(&wma->cs);
        if (ret == WAIT_OBJECT_0) break;

       if (wma->dwCurrVideoFrame < dwToFrame)
           wma->dwCurrVideoFrame++;
        else
            break;
    }

    if (wma->lpWaveFormat) {
       while (wma->dwEventCount != nHdr - 1)
        {
            LeaveCriticalSection(&wma->cs);
            Sleep(100);
            EnterCriticalSection(&wma->cs);
        }

	/* just to get rid of some race conditions between play, stop and pause */
	LeaveCriticalSection(&wma->cs);
	waveOutReset(wma->hWave);
	EnterCriticalSection(&wma->cs);

	for (i = 0; i < nHdr; i++)
	    waveOutUnprepareHeader(wma->hWave, &waveHdr[i], sizeof(WAVEHDR));
    }

    dwRet = 0;

    if (wma->lpWaveFormat) {
	HeapFree(GetProcessHeap(), 0, waveHdr);

	if (wma->hWave) {
	    LeaveCriticalSection(&wma->cs);
	    waveOutClose(wma->hWave);
	    EnterCriticalSection(&wma->cs);
	    wma->hWave = 0;
	}
	CloseHandle(wma->hEvent);
    }

mci_play_done:
    wma->dwStatus = MCI_MODE_STOP;

    if (lpParms && (dwFlags & MCI_NOTIFY)) {
	TRACE("MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify(HWND_32(LOWORD(lpParms->dwCallback)),
                       wDevID, MCI_NOTIFY_SUCCESSFUL);
    }
    LeaveCriticalSection(&wma->cs);
    return dwRet;
}

/***************************************************************************
 * 				MCIAVI_mciRecord			[internal]
 */
static	DWORD	MCIAVI_mciRecord(UINT wDevID, DWORD dwFlags, LPMCI_DGV_RECORD_PARMS lpParms)
{
    WINE_MCIAVI *wma;

    FIXME("(%04x, %08X, %p) : stub\n", wDevID, dwFlags, lpParms);

    MCIAVI_mciStop(wDevID, MCI_WAIT, NULL);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIAVI_mciGetOpenDev(wDevID);
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    EnterCriticalSection(&wma->cs);
    wma->dwStatus = MCI_MODE_RECORD;
    LeaveCriticalSection(&wma->cs);
    return 0;
}

/***************************************************************************
 * 				MCIAVI_mciStop			[internal]
 */
static	DWORD	MCIAVI_mciStop(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCIAVI *wma;
    DWORD		dwRet = 0;

    TRACE("(%04x, %08X, %p)\n", wDevID, dwFlags, lpParms);

    wma = MCIAVI_mciGetOpenDev(wDevID);
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    EnterCriticalSection(&wma->cs);

    TRACE("current status %04x\n", wma->dwStatus);

    switch (wma->dwStatus) {
    case MCI_MODE_PLAY:
    case MCI_MODE_RECORD:
        LeaveCriticalSection(&wma->cs);
        SetEvent(wma->hStopEvent);
        EnterCriticalSection(&wma->cs);
        /* fall through */
    case MCI_MODE_PAUSE:
	/* Since our wave notification callback takes the lock,
	 * we must release it before resetting the device */
        LeaveCriticalSection(&wma->cs);
        dwRet = waveOutReset(wma->hWave);
        EnterCriticalSection(&wma->cs);
        /* fall through */
    default:
        do /* one more chance for an async thread to finish */
        {
            LeaveCriticalSection(&wma->cs);
            Sleep(10);
            EnterCriticalSection(&wma->cs);
        } while (wma->dwStatus != MCI_MODE_STOP);

	break;

    case MCI_MODE_NOT_READY:
        break;        
    }

    if ((dwFlags & MCI_NOTIFY) && lpParms) {
	mciDriverNotify(HWND_32(LOWORD(lpParms->dwCallback)),
                       wDevID, MCI_NOTIFY_SUCCESSFUL);
    }
    LeaveCriticalSection(&wma->cs);
    return dwRet;
}

/***************************************************************************
 * 				MCIAVI_mciPause			[internal]
 */
static	DWORD	MCIAVI_mciPause(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCIAVI *wma;

    TRACE("(%04x, %08X, %p)\n", wDevID, dwFlags, lpParms);

    wma = MCIAVI_mciGetOpenDev(wDevID);
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    EnterCriticalSection(&wma->cs);

    if (wma->dwStatus == MCI_MODE_PLAY)
	wma->dwStatus = MCI_MODE_PAUSE;

    if (wma->lpWaveFormat) {
    	LeaveCriticalSection(&wma->cs);
	return waveOutPause(wma->hWave);
    }

    LeaveCriticalSection(&wma->cs);
    return 0;
}

/***************************************************************************
 * 				MCIAVI_mciResume			[internal]
 */
static	DWORD	MCIAVI_mciResume(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCIAVI *wma;

    TRACE("(%04x, %08X, %p)\n", wDevID, dwFlags, lpParms);

    wma = MCIAVI_mciGetOpenDev(wDevID);
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    EnterCriticalSection(&wma->cs);

    if (wma->dwStatus == MCI_MODE_PAUSE)
	wma->dwStatus = MCI_MODE_PLAY;

    if (wma->lpWaveFormat) {
    	LeaveCriticalSection(&wma->cs);
	return waveOutRestart(wma->hWave);
    }

    LeaveCriticalSection(&wma->cs);
    return 0;
}

/***************************************************************************
 * 				MCIAVI_mciSeek			[internal]
 */
static	DWORD	MCIAVI_mciSeek(UINT wDevID, DWORD dwFlags, LPMCI_SEEK_PARMS lpParms)
{
    WINE_MCIAVI *wma;

    TRACE("(%04x, %08X, %p)\n", wDevID, dwFlags, lpParms);

    MCIAVI_mciStop(wDevID, MCI_WAIT, NULL);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIAVI_mciGetOpenDev(wDevID);
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    EnterCriticalSection(&wma->cs);

    if (dwFlags & MCI_SEEK_TO_START) {
	wma->dwCurrVideoFrame = 0;
    } else if (dwFlags & MCI_SEEK_TO_END) {
	wma->dwCurrVideoFrame = wma->dwPlayableVideoFrames - 1;
    } else if (dwFlags & MCI_TO) {
        if (lpParms->dwTo > wma->dwPlayableVideoFrames - 1)
            lpParms->dwTo = wma->dwPlayableVideoFrames - 1;
	wma->dwCurrVideoFrame = MCIAVI_ConvertTimeFormatToFrame(wma, lpParms->dwTo);
    } else {
	WARN("dwFlag doesn't tell where to seek to...\n");
	LeaveCriticalSection(&wma->cs);
	return MCIERR_MISSING_PARAMETER;
    }

    TRACE("Seeking to frame=%u bytes\n", wma->dwCurrVideoFrame);

    if (dwFlags & MCI_NOTIFY) {
	mciDriverNotify(HWND_32(LOWORD(lpParms->dwCallback)),
                       wDevID, MCI_NOTIFY_SUCCESSFUL);
    }
    LeaveCriticalSection(&wma->cs);
    return 0;
}

/*****************************************************************************
 * 				MCIAVI_mciLoad			[internal]
 */
static DWORD	MCIAVI_mciLoad(UINT wDevID, DWORD dwFlags, LPMCI_DGV_LOAD_PARMSW lpParms)
{
    WINE_MCIAVI *wma;

    FIXME("(%04x, %08x, %p) : stub\n", wDevID, dwFlags, lpParms);

    MCIAVI_mciStop(wDevID, MCI_WAIT, NULL);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIAVI_mciGetOpenDev(wDevID);
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    return 0;
}

/******************************************************************************
 * 				MCIAVI_mciSave			[internal]
 */
static	DWORD	MCIAVI_mciSave(UINT wDevID, DWORD dwFlags, LPMCI_DGV_SAVE_PARMSW lpParms)
{
    WINE_MCIAVI *wma;

    FIXME("(%04x, %08x, %p) : stub\n", wDevID, dwFlags, lpParms);

    MCIAVI_mciStop(wDevID, MCI_WAIT, NULL);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIAVI_mciGetOpenDev(wDevID);
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    return 0;
}

/******************************************************************************
 * 				MCIAVI_mciFreeze			[internal]
 */
static	DWORD	MCIAVI_mciFreeze(UINT wDevID, DWORD dwFlags, LPMCI_DGV_RECT_PARMS lpParms)
{
    WINE_MCIAVI *wma;

    FIXME("(%04x, %08x, %p) : stub\n", wDevID, dwFlags, lpParms);

    MCIAVI_mciStop(wDevID, MCI_WAIT, NULL);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIAVI_mciGetOpenDev(wDevID);
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    return 0;
}

/******************************************************************************
 * 				MCIAVI_mciRealize			[internal]
 */
static	DWORD	MCIAVI_mciRealize(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCIAVI *wma;

    FIXME("(%04x, %08x, %p) : stub\n", wDevID, dwFlags, lpParms);

    MCIAVI_mciStop(wDevID, MCI_WAIT, NULL);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIAVI_mciGetOpenDev(wDevID);
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    return 0;
}

/******************************************************************************
 * 				MCIAVI_mciUnFreeze			[internal]
 */
static	DWORD	MCIAVI_mciUnFreeze(UINT wDevID, DWORD dwFlags, LPMCI_DGV_RECT_PARMS lpParms)
{
    WINE_MCIAVI *wma;

    FIXME("(%04x, %08x, %p) : stub\n", wDevID, dwFlags, lpParms);

    MCIAVI_mciStop(wDevID, MCI_WAIT, NULL);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIAVI_mciGetOpenDev(wDevID);
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    return 0;
}

/******************************************************************************
 * 				MCIAVI_mciUpdate			[internal]
 */
static	DWORD	MCIAVI_mciUpdate(UINT wDevID, DWORD dwFlags, LPMCI_DGV_UPDATE_PARMS lpParms)
{
    WINE_MCIAVI *wma;

    TRACE("%04x, %08x, %p\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIAVI_mciGetOpenDev(wDevID);
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    EnterCriticalSection(&wma->cs);

    if (dwFlags & MCI_DGV_UPDATE_HDC)
        MCIAVI_PaintFrame(wma, lpParms->hDC);

    LeaveCriticalSection(&wma->cs);

    return 0;
}

/******************************************************************************
 * 				MCIAVI_mciStep			[internal]
 */
static	DWORD	MCIAVI_mciStep(UINT wDevID, DWORD dwFlags, LPMCI_DGV_STEP_PARMS lpParms)
{
    WINE_MCIAVI *wma;

    FIXME("(%04x, %08x, %p) : stub\n", wDevID, dwFlags, lpParms);

    MCIAVI_mciStop(wDevID, MCI_WAIT, NULL);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIAVI_mciGetOpenDev(wDevID);
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    return 0;
}

/******************************************************************************
 * 				MCIAVI_mciCopy			[internal]
 */
static	DWORD	MCIAVI_mciCopy(UINT wDevID, DWORD dwFlags, LPMCI_DGV_COPY_PARMS lpParms)
{
    WINE_MCIAVI *wma;

    FIXME("(%04x, %08x, %p) : stub\n", wDevID, dwFlags, lpParms);

    MCIAVI_mciStop(wDevID, MCI_WAIT, NULL);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIAVI_mciGetOpenDev(wDevID);
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    return 0;
}

/******************************************************************************
 * 				MCIAVI_mciCut			[internal]
 */
static	DWORD	MCIAVI_mciCut(UINT wDevID, DWORD dwFlags, LPMCI_DGV_CUT_PARMS lpParms)
{
    WINE_MCIAVI *wma;

    FIXME("(%04x, %08x, %p) : stub\n", wDevID, dwFlags, lpParms);

    MCIAVI_mciStop(wDevID, MCI_WAIT, NULL);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIAVI_mciGetOpenDev(wDevID);
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    return 0;
}

/******************************************************************************
 * 				MCIAVI_mciDelete			[internal]
 */
static	DWORD	MCIAVI_mciDelete(UINT wDevID, DWORD dwFlags, LPMCI_DGV_DELETE_PARMS lpParms)
{
    WINE_MCIAVI *wma;

    FIXME("(%04x, %08x, %p) : stub\n", wDevID, dwFlags, lpParms);

    MCIAVI_mciStop(wDevID, MCI_WAIT, NULL);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIAVI_mciGetOpenDev(wDevID);
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    return 0;
}

/******************************************************************************
 * 				MCIAVI_mciPaste			[internal]
 */
static	DWORD	MCIAVI_mciPaste(UINT wDevID, DWORD dwFlags, LPMCI_DGV_PASTE_PARMS lpParms)
{
    WINE_MCIAVI *wma;

    FIXME("(%04x, %08x, %p) : stub\n", wDevID, dwFlags, lpParms);

    MCIAVI_mciStop(wDevID, MCI_WAIT, NULL);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIAVI_mciGetOpenDev(wDevID);
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    return 0;
}

/******************************************************************************
 * 				MCIAVI_mciCue			[internal]
 */
static	DWORD	MCIAVI_mciCue(UINT wDevID, DWORD dwFlags, LPMCI_DGV_CUE_PARMS lpParms)
{
    WINE_MCIAVI *wma;

    FIXME("(%04x, %08x, %p) : stub\n", wDevID, dwFlags, lpParms);

    MCIAVI_mciStop(wDevID, MCI_WAIT, NULL);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIAVI_mciGetOpenDev(wDevID);
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    return 0;
}

/******************************************************************************
 * 				MCIAVI_mciCapture			[internal]
 */
static	DWORD	MCIAVI_mciCapture(UINT wDevID, DWORD dwFlags, LPMCI_DGV_CAPTURE_PARMSW lpParms)
{
    WINE_MCIAVI *wma;

    FIXME("(%04x, %08x, %p) : stub\n", wDevID, dwFlags, lpParms);

    MCIAVI_mciStop(wDevID, MCI_WAIT, NULL);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIAVI_mciGetOpenDev(wDevID);
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    return 0;
}

/******************************************************************************
 * 				MCIAVI_mciMonitor			[internal]
 */
static	DWORD	MCIAVI_mciMonitor(UINT wDevID, DWORD dwFlags, LPMCI_DGV_MONITOR_PARMS lpParms)
{
    WINE_MCIAVI *wma;

    FIXME("(%04x, %08x, %p) : stub\n", wDevID, dwFlags, lpParms);

    MCIAVI_mciStop(wDevID, MCI_WAIT, NULL);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIAVI_mciGetOpenDev(wDevID);
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    return 0;
}

/******************************************************************************
 * 				MCIAVI_mciReserve			[internal]
 */
static	DWORD	MCIAVI_mciReserve(UINT wDevID, DWORD dwFlags, LPMCI_DGV_RESERVE_PARMSW lpParms)
{
    WINE_MCIAVI *wma;

    FIXME("(%04x, %08x, %p) : stub\n", wDevID, dwFlags, lpParms);

    MCIAVI_mciStop(wDevID, MCI_WAIT, NULL);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIAVI_mciGetOpenDev(wDevID);
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    return 0;
}

/******************************************************************************
 * 				MCIAVI_mciSetAudio			[internal]
 */
static	DWORD	MCIAVI_mciSetAudio(UINT wDevID, DWORD dwFlags, LPMCI_DGV_SETAUDIO_PARMSW lpParms)
{
    WINE_MCIAVI *wma;

    FIXME("(%04x, %08x, %p) : stub\n", wDevID, dwFlags, lpParms);

    MCIAVI_mciStop(wDevID, MCI_WAIT, NULL);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIAVI_mciGetOpenDev(wDevID);
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    return 0;
}

/******************************************************************************
 * 				MCIAVI_mciSignal			[internal]
 */
static	DWORD	MCIAVI_mciSignal(UINT wDevID, DWORD dwFlags, LPMCI_DGV_SIGNAL_PARMS lpParms)
{
    WINE_MCIAVI *wma;

    FIXME("(%04x, %08x, %p) : stub\n", wDevID, dwFlags, lpParms);

    MCIAVI_mciStop(wDevID, MCI_WAIT, NULL);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIAVI_mciGetOpenDev(wDevID);
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    return 0;
}

/******************************************************************************
 * 				MCIAVI_mciSetVideo			[internal]
 */
static	DWORD	MCIAVI_mciSetVideo(UINT wDevID, DWORD dwFlags, LPMCI_DGV_SETVIDEO_PARMSW lpParms)
{
    WINE_MCIAVI *wma;

    FIXME("(%04x, %08x, %p) : stub\n", wDevID, dwFlags, lpParms);

    MCIAVI_mciStop(wDevID, MCI_WAIT, NULL);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIAVI_mciGetOpenDev(wDevID);
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    return 0;
}

/******************************************************************************
 * 				MCIAVI_mciQuality			[internal]
 */
static	DWORD	MCIAVI_mciQuality(UINT wDevID, DWORD dwFlags, LPMCI_DGV_QUALITY_PARMSW lpParms)
{
    WINE_MCIAVI *wma;

    FIXME("(%04x, %08x, %p) : stub\n", wDevID, dwFlags, lpParms);

    MCIAVI_mciStop(wDevID, MCI_WAIT, NULL);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIAVI_mciGetOpenDev(wDevID);
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    return 0;
}

/******************************************************************************
 * 				MCIAVI_mciList			[internal]
 */
static	DWORD	MCIAVI_mciList(UINT wDevID, DWORD dwFlags, LPMCI_DGV_LIST_PARMSW lpParms)
{
    WINE_MCIAVI *wma;

    FIXME("(%04x, %08x, %p) : stub\n", wDevID, dwFlags, lpParms);

    MCIAVI_mciStop(wDevID, MCI_WAIT, NULL);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIAVI_mciGetOpenDev(wDevID);
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    return 0;
}

/******************************************************************************
 * 				MCIAVI_mciUndo			[internal]
 */
static	DWORD	MCIAVI_mciUndo(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCIAVI *wma;

    FIXME("(%04x, %08x, %p) : stub\n", wDevID, dwFlags, lpParms);

    MCIAVI_mciStop(wDevID, MCI_WAIT, NULL);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIAVI_mciGetOpenDev(wDevID);
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    return 0;
}

/******************************************************************************
 * 				MCIAVI_mciConfigure			[internal]
 */
static	DWORD	MCIAVI_mciConfigure(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCIAVI *wma;

    FIXME("(%04x, %08x, %p) : stub\n", wDevID, dwFlags, lpParms);

    MCIAVI_mciStop(wDevID, MCI_WAIT, NULL);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIAVI_mciGetOpenDev(wDevID);
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    return 0;
}

/******************************************************************************
 * 				MCIAVI_mciRestore			[internal]
 */
static	DWORD	MCIAVI_mciRestore(UINT wDevID, DWORD dwFlags, LPMCI_DGV_RESTORE_PARMSW lpParms)
{
    WINE_MCIAVI *wma;

    FIXME("(%04x, %08x, %p) : stub\n", wDevID, dwFlags, lpParms);

    MCIAVI_mciStop(wDevID, MCI_WAIT, NULL);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIAVI_mciGetOpenDev(wDevID);
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    return 0;
}

/*======================================================================*
 *                  	    MCI AVI entry points			*
 *======================================================================*/

/**************************************************************************
 * 				DriverProc (MCIAVI.@)
 */
LRESULT CALLBACK MCIAVI_DriverProc(DWORD_PTR dwDevID, HDRVR hDriv, UINT wMsg,
                                   LPARAM dwParam1, LPARAM dwParam2)
{
    TRACE("(%08lX, %p, %08X, %08lX, %08lX)\n",
	  dwDevID, hDriv, wMsg, dwParam1, dwParam2);

    switch (wMsg) {
    case DRV_LOAD:		return 1;
    case DRV_FREE:		return 1;
    case DRV_OPEN:		return MCIAVI_drvOpen((LPCWSTR)dwParam1, (LPMCI_OPEN_DRIVER_PARMSW)dwParam2);
    case DRV_CLOSE:		return MCIAVI_drvClose(dwDevID);
    case DRV_ENABLE:		return 1;
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		return MCIAVI_drvConfigure(dwDevID);
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;
    }

    /* session instance */
    if (dwDevID == 0xFFFFFFFF) return 1;

    switch (wMsg) {
    case MCI_OPEN_DRIVER:	return MCIAVI_mciOpen      (dwDevID, dwParam1, (LPMCI_DGV_OPEN_PARMSW)     dwParam2);
    case MCI_CLOSE_DRIVER:	return MCIAVI_mciClose     (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)       dwParam2);
    case MCI_PLAY:		return MCIAVI_mciPlay      (dwDevID, dwParam1, (LPMCI_PLAY_PARMS)          dwParam2);
    case MCI_RECORD:		return MCIAVI_mciRecord    (dwDevID, dwParam1, (LPMCI_DGV_RECORD_PARMS)    dwParam2);
    case MCI_STOP:		return MCIAVI_mciStop      (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)       dwParam2);
    case MCI_SET:		return MCIAVI_mciSet       (dwDevID, dwParam1, (LPMCI_DGV_SET_PARMS)       dwParam2);
    case MCI_PAUSE:		return MCIAVI_mciPause     (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)       dwParam2);
    case MCI_RESUME:		return MCIAVI_mciResume    (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)       dwParam2);
    case MCI_STATUS:		return MCIAVI_mciStatus    (dwDevID, dwParam1, (LPMCI_DGV_STATUS_PARMSW)   dwParam2);
    case MCI_GETDEVCAPS:	return MCIAVI_mciGetDevCaps(dwDevID, dwParam1, (LPMCI_GETDEVCAPS_PARMS)    dwParam2);
    case MCI_INFO:		return MCIAVI_mciInfo      (dwDevID, dwParam1, (LPMCI_DGV_INFO_PARMSW)     dwParam2);
    case MCI_SEEK:		return MCIAVI_mciSeek      (dwDevID, dwParam1, (LPMCI_SEEK_PARMS)          dwParam2);
    case MCI_PUT:		return MCIAVI_mciPut	   (dwDevID, dwParam1, (LPMCI_DGV_PUT_PARMS)       dwParam2);
    case MCI_WINDOW:		return MCIAVI_mciWindow	   (dwDevID, dwParam1, (LPMCI_DGV_WINDOW_PARMSW)   dwParam2);
    case MCI_LOAD:		return MCIAVI_mciLoad      (dwDevID, dwParam1, (LPMCI_DGV_LOAD_PARMSW)     dwParam2);
    case MCI_SAVE:		return MCIAVI_mciSave      (dwDevID, dwParam1, (LPMCI_DGV_SAVE_PARMSW)     dwParam2);
    case MCI_FREEZE:		return MCIAVI_mciFreeze	   (dwDevID, dwParam1, (LPMCI_DGV_RECT_PARMS)      dwParam2);
    case MCI_REALIZE:		return MCIAVI_mciRealize   (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)       dwParam2);
    case MCI_UNFREEZE:		return MCIAVI_mciUnFreeze  (dwDevID, dwParam1, (LPMCI_DGV_RECT_PARMS)      dwParam2);
    case MCI_UPDATE:		return MCIAVI_mciUpdate    (dwDevID, dwParam1, (LPMCI_DGV_UPDATE_PARMS)    dwParam2);
    case MCI_WHERE:		return MCIAVI_mciWhere	   (dwDevID, dwParam1, (LPMCI_DGV_RECT_PARMS)      dwParam2);
    case MCI_STEP:		return MCIAVI_mciStep      (dwDevID, dwParam1, (LPMCI_DGV_STEP_PARMS)      dwParam2);
    case MCI_COPY:		return MCIAVI_mciCopy      (dwDevID, dwParam1, (LPMCI_DGV_COPY_PARMS)      dwParam2);
    case MCI_CUT:		return MCIAVI_mciCut       (dwDevID, dwParam1, (LPMCI_DGV_CUT_PARMS)       dwParam2);
    case MCI_DELETE:		return MCIAVI_mciDelete    (dwDevID, dwParam1, (LPMCI_DGV_DELETE_PARMS)    dwParam2);
    case MCI_PASTE:		return MCIAVI_mciPaste     (dwDevID, dwParam1, (LPMCI_DGV_PASTE_PARMS)     dwParam2);
    case MCI_CUE:		return MCIAVI_mciCue       (dwDevID, dwParam1, (LPMCI_DGV_CUE_PARMS)       dwParam2);
	/* Digital Video specific */
    case MCI_CAPTURE:		return MCIAVI_mciCapture   (dwDevID, dwParam1, (LPMCI_DGV_CAPTURE_PARMSW)  dwParam2);
    case MCI_MONITOR:		return MCIAVI_mciMonitor   (dwDevID, dwParam1, (LPMCI_DGV_MONITOR_PARMS)   dwParam2);
    case MCI_RESERVE:		return MCIAVI_mciReserve   (dwDevID, dwParam1, (LPMCI_DGV_RESERVE_PARMSW)  dwParam2);
    case MCI_SETAUDIO:		return MCIAVI_mciSetAudio  (dwDevID, dwParam1, (LPMCI_DGV_SETAUDIO_PARMSW) dwParam2);
    case MCI_SIGNAL:		return MCIAVI_mciSignal    (dwDevID, dwParam1, (LPMCI_DGV_SIGNAL_PARMS)    dwParam2);
    case MCI_SETVIDEO:		return MCIAVI_mciSetVideo  (dwDevID, dwParam1, (LPMCI_DGV_SETVIDEO_PARMSW) dwParam2);
    case MCI_QUALITY:		return MCIAVI_mciQuality   (dwDevID, dwParam1, (LPMCI_DGV_QUALITY_PARMSW)  dwParam2);
    case MCI_LIST:		return MCIAVI_mciList      (dwDevID, dwParam1, (LPMCI_DGV_LIST_PARMSW)     dwParam2);
    case MCI_UNDO:		return MCIAVI_mciUndo      (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)       dwParam2);
    case MCI_CONFIGURE:		return MCIAVI_mciConfigure (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)       dwParam2);
    case MCI_RESTORE:		return MCIAVI_mciRestore   (dwDevID, dwParam1, (LPMCI_DGV_RESTORE_PARMSW)  dwParam2);

    case MCI_SPIN:
    case MCI_ESCAPE:
	WARN("Unsupported command [%u]\n", wMsg);
	break;
    case MCI_OPEN:
    case MCI_CLOSE:
	FIXME("Shouldn't receive a MCI_OPEN or CLOSE message\n");
	break;
    default:
	TRACE("Sending msg [%u] to default driver proc\n", wMsg);
	return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
    return MCIERR_UNRECOGNIZED_COMMAND;
}
