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

#ifndef __WINE_PRIVATE_MCIAVI_H
#define __WINE_PRIVATE_MCIAVI_H

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "mmddk.h"
#include "digitalv.h"
#include "vfw.h"
#include "wownt32.h"
#include "mciavi.h"

struct MMIOPos {
    DWORD	dwOffset;
    DWORD	dwSize;
};

typedef struct {
    MCIDEVICEID         wDevID;
    int			nUseCount;          	/* Incremented for each shared open          */
    BOOL  		fShareable;         	/* TRUE if first open was shareable 	     */
    WORD		wCommandTable;		/* custom MCI command table */
    DWORD               dwStatus;               /* One of MCI_MODE_XXX                       */
    LPWSTR              lpFileName;
    DWORD		dwMciTimeFormat;	/* current time format */
    DWORD		dwSet;			/* what's turned on: video & audio l&r */
    /* information on the loaded AVI file */
    HMMIO		hFile;	            	/* mmio file handle open as Element          */
    DWORD               video_stream_n, audio_stream_n; /* stream #s */
    MainAVIHeader	mah;
    AVIStreamHeader	ash_video;
    AVIStreamHeader	ash_audio;
    LPBITMAPINFOHEADER	inbih;
    struct MMIOPos*	lpVideoIndex;
    LPWAVEFORMATEX	lpWaveFormat;
    struct MMIOPos*	lpAudioIndex;
    /* computed data from the file */
    DWORD		dwPlayableVideoFrames;	/* max number of frames to be played. Takes care of truncated files and audio skew */
    DWORD		dwPlayableAudioBlocks;
    /* data for the AVI decompressor */
    HIC			hic;
    LPBITMAPINFOHEADER	outbih;
    LPVOID		indata;
    LPVOID		outdata;
    HBITMAP  	    	hbmFrame;
    /* data for playing the audio part */
    HANDLE		hWave;
    HANDLE		hEvent;			/* for synchronization */
    LONG		dwEventCount;		/* for synchronization */
    /* data for play back */
    HWND               hWnd, hWndPaint;
    DWORD               dwCachedFrame; /* buffered frame */
    DWORD		dwCurrVideoFrame;	/* video frame to display and current position */
    DWORD               dwToVideoFrame; /* play to */
    DWORD		dwCurrAudioBlock;	/* current audio block being played */
    RECT                source, dest;
    /* data for the background mechanism */
    CRITICAL_SECTION	cs;
    HANDLE              hStopEvent;
    HANDLE              ack_event; /* acknowledge that an async command has started */
} WINE_MCIAVI;

extern HINSTANCE MCIAVI_hInstance;

/* info.c */
DWORD 	MCIAVI_ConvertTimeFormatToFrame(WINE_MCIAVI* wma, DWORD val);
DWORD	MCIAVI_mciGetDevCaps(UINT wDevID, DWORD dwFlags,  LPMCI_GETDEVCAPS_PARMS lpParms);
DWORD	MCIAVI_mciInfo(UINT wDevID, DWORD dwFlags, LPMCI_DGV_INFO_PARMSW lpParms);
DWORD	MCIAVI_mciSet(UINT wDevID, DWORD dwFlags, LPMCI_DGV_SET_PARMS lpParms);
DWORD	MCIAVI_mciStatus(UINT wDevID, DWORD dwFlags, LPMCI_DGV_STATUS_PARMSW lpParms);

/* mmoutput.c */
BOOL	MCIAVI_GetInfo(WINE_MCIAVI* wma);
DWORD	MCIAVI_OpenAudio(WINE_MCIAVI* wma, unsigned* nHdr, LPWAVEHDR* pWaveHdr);
BOOL	MCIAVI_OpenVideo(WINE_MCIAVI* wma);
void	MCIAVI_PlayAudioBlocks(WINE_MCIAVI* wma, unsigned nHdr, LPWAVEHDR waveHdr);
LRESULT MCIAVI_PaintFrame(WINE_MCIAVI* wma, HDC hDC);

/* mciavi.c */
WINE_MCIAVI*	MCIAVI_mciGetOpenDev(UINT wDevID);
DWORD MCIAVI_mciClose(UINT, DWORD, LPMCI_GENERIC_PARMS);

/* wnd.c */
BOOL    MCIAVI_RegisterClass(void);
BOOL    MCIAVI_UnregisterClass(void);
BOOL    MCIAVI_CreateWindow(WINE_MCIAVI* wma, DWORD dwFlags, LPMCI_DGV_OPEN_PARMSW lpOpenParms);
DWORD	MCIAVI_mciPut(UINT wDevID, DWORD dwFlags, LPMCI_DGV_PUT_PARMS lpParms);
DWORD	MCIAVI_mciWhere(UINT wDevID, DWORD dwFlags, LPMCI_DGV_RECT_PARMS lpParms);
DWORD	MCIAVI_mciWindow(UINT wDevID, DWORD dwFlags, LPMCI_DGV_WINDOW_PARMSW lpParms);

#endif  /* __WINE_PRIVATE_MCIAVI_H */
