/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*
 * Animation control
 *
 * Copyright 1998, 1999 Eric Kohl
 * 		   1999 Eric Pouech
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * NOTES
 *   I will only improve this control once in a while.
 *     Eric <ekohl@abo.rhein-zeitung.de>
 *
 * TODO:
 *   - check for the 'rec ' list in some AVI files
 *   - concurrent access to infoPtr
 */

#define COM_NO_WINDOWS_H
#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "vfw.h"
#include "mmsystem.h"
#include "comctl32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(animate);

static struct {
    HMODULE	hModule;
    HIC         (WINAPI *fnICOpen)(DWORD, DWORD, UINT);
    LRESULT     (WINAPI *fnICClose)(HIC);
    LRESULT     (WINAPI *fnICSendMessage)(HIC, UINT, DWORD, DWORD);
    DWORD       (WINAPIV *fnICDecompress)(HIC,DWORD,LPBITMAPINFOHEADER,LPVOID,LPBITMAPINFOHEADER,LPVOID);
} fnIC;

typedef struct
{
   /* reference to input stream (file or resource) */
   HGLOBAL 		hRes;
   HMMIO		hMMio;	/* handle to mmio stream */
   HWND			hwndSelf;
   HWND			hwndNotify;
   /* information on the loaded AVI file */
   MainAVIHeader	mah;
   AVIStreamHeader	ash;
   LPBITMAPINFOHEADER	inbih;
   LPDWORD		lpIndex;
   /* data for the decompressor */
   HIC			hic;
   LPBITMAPINFOHEADER	outbih;
   LPVOID		indata;
   LPVOID		outdata;
   /* data for the background mechanism */
   CRITICAL_SECTION	cs;
   HANDLE		hThread;
   UINT			uTimer;
   /* data for playing the file */
   int			nFromFrame;
   int			nToFrame;
   int			nLoop;
   int			currFrame;
   /* tranparency info*/
   COLORREF         	transparentColor;
   HBRUSH           	hbrushBG;
   HBITMAP  	    	hbmPrevFrame;
} ANIMATE_INFO;

#define ANIMATE_GetInfoPtr(hWnd) ((ANIMATE_INFO *)GetWindowLongA(hWnd, 0))
#define ANIMATE_COLOR_NONE  	0xffffffff

static void ANIMATE_Notify(ANIMATE_INFO* infoPtr, UINT notif)
{
    SendMessageA(infoPtr->hwndNotify, WM_COMMAND,
		 MAKEWPARAM(GetDlgCtrlID(infoPtr->hwndSelf), notif),
		 (LPARAM)infoPtr->hwndSelf);
}

static BOOL ANIMATE_LoadResA(ANIMATE_INFO *infoPtr, HINSTANCE hInst, LPSTR lpName)
{
    HRSRC 	hrsrc;
    MMIOINFO	mminfo;
    LPVOID	lpAvi;

    hrsrc = FindResourceA(hInst, lpName, "AVI");
    if (!hrsrc)
	return FALSE;

    infoPtr->hRes = LoadResource(hInst, hrsrc);
    if (!infoPtr->hRes)
 	return FALSE;

    lpAvi = LockResource(infoPtr->hRes);
    if (!lpAvi)
	return FALSE;

    memset(&mminfo, 0, sizeof(mminfo));
    mminfo.fccIOProc = FOURCC_MEM;
    mminfo.pchBuffer = (LPSTR)lpAvi;
    mminfo.cchBuffer = SizeofResource(hInst, hrsrc);
    infoPtr->hMMio = mmioOpenA(NULL, &mminfo, MMIO_READ);
    if (!infoPtr->hMMio) {
	GlobalFree((HGLOBAL)lpAvi);
	return FALSE;
    }

    return TRUE;
}


static BOOL ANIMATE_LoadFileA(ANIMATE_INFO *infoPtr, LPSTR lpName)
{
    infoPtr->hMMio = mmioOpenA((LPSTR)lpName, NULL,
			       MMIO_ALLOCBUF | MMIO_READ | MMIO_DENYWRITE);

    if (!infoPtr->hMMio)
	return FALSE;

    return TRUE;
}


static LRESULT ANIMATE_DoStop(ANIMATE_INFO *infoPtr)
{
    EnterCriticalSection(&infoPtr->cs);

    /* should stop playing */
    if (infoPtr->hThread)
    {
        if (!TerminateThread(infoPtr->hThread,0))
            WARN("could not destroy animation thread!\n");
	    infoPtr->hThread = 0;
    }
    if (infoPtr->uTimer) {
	KillTimer(infoPtr->hwndSelf, infoPtr->uTimer);
	infoPtr->uTimer = 0;
    }

    LeaveCriticalSection(&infoPtr->cs);

    ANIMATE_Notify(infoPtr, ACN_STOP);

    return TRUE;
}


static void ANIMATE_Free(ANIMATE_INFO *infoPtr)
{
    if (infoPtr->hMMio) {
	ANIMATE_DoStop(infoPtr);
	mmioClose(infoPtr->hMMio, 0);
	if (infoPtr->hRes) {
 	    FreeResource(infoPtr->hRes);
	    infoPtr->hRes = 0;
	}
	if (infoPtr->lpIndex) {
	    HeapFree(GetProcessHeap(), 0, infoPtr->lpIndex);
	    infoPtr->lpIndex = NULL;
	}
	if (infoPtr->hic) {
	    fnIC.fnICClose(infoPtr->hic);
	    infoPtr->hic = 0;
	}
	if (infoPtr->inbih) {
	    HeapFree(GetProcessHeap(), 0, infoPtr->inbih);
	    infoPtr->inbih = NULL;
	}
	if (infoPtr->outbih) {
	    HeapFree(GetProcessHeap(), 0, infoPtr->outbih);
	    infoPtr->outbih = NULL;
	}
        if( infoPtr->indata )
        {
	HeapFree(GetProcessHeap(), 0, infoPtr->indata);
            infoPtr->indata = NULL;
        }
    	if( infoPtr->outdata )
        {
	HeapFree(GetProcessHeap(), 0, infoPtr->outdata);
            infoPtr->outdata = NULL;
        }
    	if( infoPtr->hbmPrevFrame )
        {
	    DeleteObject(infoPtr->hbmPrevFrame);
            infoPtr->hbmPrevFrame = 0;
        }
	infoPtr->indata = infoPtr->outdata = NULL;
	infoPtr->hwndSelf = 0;
	infoPtr->hMMio = 0;

	memset(&infoPtr->mah, 0, sizeof(infoPtr->mah));
	memset(&infoPtr->ash, 0, sizeof(infoPtr->ash));
	infoPtr->nFromFrame = infoPtr->nToFrame = infoPtr->nLoop = infoPtr->currFrame = 0;
    }
    infoPtr->transparentColor = ANIMATE_COLOR_NONE;
}

static void ANIMATE_TransparentBlt(ANIMATE_INFO* infoPtr, HDC hdcDest, HDC hdcSource)
{
    HDC hdcMask;
    HBITMAP hbmMask;
    HBITMAP hbmOld;

    /* create a transparency mask */
    hdcMask = CreateCompatibleDC(hdcDest);
    hbmMask = CreateBitmap(infoPtr->inbih->biWidth, infoPtr->inbih->biHeight, 1,1,NULL);
    hbmOld = SelectObject(hdcMask, hbmMask);

    SetBkColor(hdcSource,infoPtr->transparentColor);
    BitBlt(hdcMask,0,0,infoPtr->inbih->biWidth, infoPtr->inbih->biHeight,hdcSource,0,0,SRCCOPY);

    /* mask the source bitmap */
    SetBkColor(hdcSource, RGB(0,0,0));
    SetTextColor(hdcSource, RGB(255,255,255));
    BitBlt(hdcSource, 0, 0, infoPtr->inbih->biWidth, infoPtr->inbih->biHeight, hdcMask, 0, 0, SRCAND);

    /* mask the destination bitmap */
    SetBkColor(hdcDest, RGB(255,255,255));
    SetTextColor(hdcDest, RGB(0,0,0));
    BitBlt(hdcDest, 0, 0, infoPtr->inbih->biWidth, infoPtr->inbih->biHeight, hdcMask, 0, 0, SRCAND);

    /* combine source and destination */
    BitBlt(hdcDest,0,0,infoPtr->inbih->biWidth, infoPtr->inbih->biHeight,hdcSource,0,0,SRCPAINT);

    SelectObject(hdcMask, hbmOld);
    DeleteObject(hbmMask);
    DeleteDC(hdcMask);
}

static LRESULT ANIMATE_PaintFrame(ANIMATE_INFO* infoPtr, HDC hDC)
{
    void* pBitmapData = NULL;
    LPBITMAPINFO pBitmapInfo = NULL;

    HDC hdcMem;
    HBITMAP hbmOld;

    int nOffsetX = 0;
    int nOffsetY = 0;

    int nWidth;
    int nHeight;

    if (!hDC || !infoPtr->inbih)
	return TRUE;

    if (infoPtr->hic )
    {
        pBitmapData = infoPtr->outdata;
        pBitmapInfo = (LPBITMAPINFO)infoPtr->outbih;

        nWidth = infoPtr->outbih->biWidth;
        nHeight = infoPtr->outbih->biHeight;
    } else
    {
        pBitmapData = infoPtr->indata;
        pBitmapInfo = (LPBITMAPINFO)infoPtr->inbih;

        nWidth = infoPtr->inbih->biWidth;
        nHeight = infoPtr->inbih->biHeight;
    }

    if(!infoPtr->hbmPrevFrame)
    {
        infoPtr->hbmPrevFrame=CreateCompatibleBitmap(hDC, nWidth,nHeight );
    }

    SetDIBits(hDC, infoPtr->hbmPrevFrame, 0, nHeight, pBitmapData, (LPBITMAPINFO)pBitmapInfo, DIB_RGB_COLORS);

    hdcMem = CreateCompatibleDC(hDC);
    hbmOld = SelectObject(hdcMem, infoPtr->hbmPrevFrame);

    /*
     * we need to get the transparent color even without ACS_TRANSPARENT,
     * because the style can be changed later on and the color should always
     * be obtained in the first frame
     */
    if(infoPtr->transparentColor == ANIMATE_COLOR_NONE)
    {
        infoPtr->transparentColor = GetPixel(hdcMem,0,0);
    }

    if(GetWindowLongA(infoPtr->hwndSelf, GWL_STYLE) & ACS_TRANSPARENT)
    {
        HDC hdcFinal = CreateCompatibleDC(hDC);
        HBITMAP hbmFinal = CreateCompatibleBitmap(hDC,nWidth, nHeight);
        HBITMAP hbmOld2 = SelectObject(hdcFinal, hbmFinal);
        RECT rect;

        rect.left = 0;
        rect.top = 0;
        rect.right = nWidth;
        rect.bottom = nHeight;

        if(!infoPtr->hbrushBG)
            infoPtr->hbrushBG = GetCurrentObject(hDC, OBJ_BRUSH);

        FillRect(hdcFinal, &rect, infoPtr->hbrushBG);
        ANIMATE_TransparentBlt(infoPtr, hdcFinal, hdcMem);

        SelectObject(hdcFinal, hbmOld2);
        SelectObject(hdcMem, hbmFinal);
        DeleteDC(hdcFinal);
        DeleteObject(infoPtr->hbmPrevFrame);
        infoPtr->hbmPrevFrame = hbmFinal;
         }

    if (GetWindowLongA(infoPtr->hwndSelf, GWL_STYLE) & ACS_CENTER)
    {
       RECT rect;

       GetWindowRect(infoPtr->hwndSelf, &rect);
       nOffsetX = ((rect.right - rect.left) - nWidth)/2;
       nOffsetY = ((rect.bottom - rect.top) - nHeight)/2;
    }
    BitBlt(hDC, nOffsetX, nOffsetY, nWidth, nHeight, hdcMem, 0, 0, SRCCOPY);

    SelectObject(hdcMem, hbmOld);
    DeleteDC(hdcMem);
    return TRUE;
}

static LRESULT ANIMATE_DrawFrame(ANIMATE_INFO* infoPtr)
{
    HDC		hDC;

    TRACE("Drawing frame %d (loop %d)\n", infoPtr->currFrame, infoPtr->nLoop);

    EnterCriticalSection(&infoPtr->cs);

    mmioSeek(infoPtr->hMMio, infoPtr->lpIndex[infoPtr->currFrame], SEEK_SET);
    mmioRead(infoPtr->hMMio, infoPtr->indata, infoPtr->ash.dwSuggestedBufferSize);

    if (infoPtr->hic &&
	fnIC.fnICDecompress(infoPtr->hic, 0, infoPtr->inbih, infoPtr->indata,
		     infoPtr->outbih, infoPtr->outdata) != ICERR_OK) {
	LeaveCriticalSection(&infoPtr->cs);
	WARN("Decompression error\n");
	return FALSE;
    }

    if ((hDC = GetDC(infoPtr->hwndSelf)) != 0) {
	ANIMATE_PaintFrame(infoPtr, hDC);
	ReleaseDC(infoPtr->hwndSelf, hDC);
    }

    if (infoPtr->currFrame++ >= infoPtr->nToFrame) {
	infoPtr->currFrame = infoPtr->nFromFrame;
	if (infoPtr->nLoop != -1) {
	    if (--infoPtr->nLoop == 0) {
		ANIMATE_DoStop(infoPtr);
	    }
	}
    }
    LeaveCriticalSection(&infoPtr->cs);

    return TRUE;
}

static DWORD CALLBACK ANIMATE_AnimationThread(LPVOID ptr_)
{
    ANIMATE_INFO*	infoPtr = (ANIMATE_INFO*)ptr_;
    HDC hDC;

    if(!infoPtr)
    {
        WARN("animation structure undefined!\n");
        return FALSE;
    }

    while(1)
    {
        if(GetWindowLongA(infoPtr->hwndSelf, GWL_STYLE) & ACS_TRANSPARENT)
        {
            hDC = GetDC(infoPtr->hwndSelf);
	    /* sometimes the animation window will be destroyed in between
	     * by the main program, so a ReleaseDC() error msg is possible */
            infoPtr->hbrushBG = (HBRUSH)SendMessageA(infoPtr->hwndNotify,
					     WM_CTLCOLORSTATIC, (WPARAM)hDC,
					     (LPARAM)infoPtr->hwndSelf);
            ReleaseDC(infoPtr->hwndSelf,hDC);
        }

        EnterCriticalSection(&infoPtr->cs);
        ANIMATE_DrawFrame(infoPtr);
        LeaveCriticalSection(&infoPtr->cs);

        /* time is in microseconds, we should convert it to milliseconds */
        Sleep((infoPtr->mah.dwMicroSecPerFrame+500)/1000);
    }
    return TRUE;
}

static LRESULT ANIMATE_Play(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    ANIMATE_INFO *infoPtr = ANIMATE_GetInfoPtr(hWnd);

    /* nothing opened */
    if (!infoPtr->hMMio)
	return FALSE;

    if (infoPtr->hThread || infoPtr->uTimer) {
	FIXME("Already playing ? what should I do ??\n");
	ANIMATE_DoStop(infoPtr);
    }

    infoPtr->nFromFrame = (INT)LOWORD(lParam);
    infoPtr->nToFrame   = (INT)HIWORD(lParam);
    infoPtr->nLoop      = (INT)wParam;

    if (infoPtr->nToFrame == 0xFFFF)
	infoPtr->nToFrame = infoPtr->mah.dwTotalFrames - 1;

    TRACE("(repeat=%d from=%d to=%d);\n",
	  infoPtr->nLoop, infoPtr->nFromFrame, infoPtr->nToFrame);

    if (infoPtr->nFromFrame >= infoPtr->nToFrame ||
	infoPtr->nToFrame >= infoPtr->mah.dwTotalFrames)
	return FALSE;

    infoPtr->currFrame = infoPtr->nFromFrame;

    if (GetWindowLongA(hWnd, GWL_STYLE) & ACS_TIMER) {
	TRACE("Using a timer\n");
	/* create a timer to display AVI */
	infoPtr->uTimer = SetTimer(hWnd, 1, infoPtr->mah.dwMicroSecPerFrame / 1000, NULL);
    } else {
        DWORD threadID;

	TRACE("Using an animation thread\n");
        infoPtr->hThread = CreateThread(0,0,ANIMATE_AnimationThread,(LPVOID)infoPtr, 0, &threadID);
        if(!infoPtr->hThread)
        {
           ERR("Could not create animation thread!\n");
           return FALSE;
    }

    }

    ANIMATE_Notify(infoPtr, ACN_START);

    return TRUE;
}


static BOOL ANIMATE_GetAviInfo(ANIMATE_INFO *infoPtr)
{
    MMCKINFO		ckMainRIFF;
    MMCKINFO		mmckHead;
    MMCKINFO		mmckList;
    MMCKINFO		mmckInfo;
    DWORD		numFrame;
    DWORD		insize;

    if (mmioDescend(infoPtr->hMMio, &ckMainRIFF, NULL, 0) != 0) {
	WARN("Can't find 'RIFF' chunk\n");
	return FALSE;
    }

    if ((ckMainRIFF.ckid != FOURCC_RIFF) ||
	(ckMainRIFF.fccType != mmioFOURCC('A', 'V', 'I', ' '))) {
	WARN("Can't find 'AVI ' chunk\n");
	return FALSE;
    }

    mmckHead.fccType = mmioFOURCC('h', 'd', 'r', 'l');
    if (mmioDescend(infoPtr->hMMio, &mmckHead, &ckMainRIFF, MMIO_FINDLIST) != 0) {
	WARN("Can't find 'hdrl' list\n");
	return FALSE;
    }

    mmckInfo.ckid = mmioFOURCC('a', 'v', 'i', 'h');
    if (mmioDescend(infoPtr->hMMio, &mmckInfo, &mmckHead, MMIO_FINDCHUNK) != 0) {
	WARN("Can't find 'avih' chunk\n");
	return FALSE;
    }

    mmioRead(infoPtr->hMMio, (LPSTR)&infoPtr->mah, sizeof(infoPtr->mah));

    TRACE("mah.dwMicroSecPerFrame=%ld\n", 	infoPtr->mah.dwMicroSecPerFrame);
    TRACE("mah.dwMaxBytesPerSec=%ld\n", 	infoPtr->mah.dwMaxBytesPerSec);
    TRACE("mah.dwPaddingGranularity=%ld\n", 	infoPtr->mah.dwPaddingGranularity);
    TRACE("mah.dwFlags=%ld\n", 			infoPtr->mah.dwFlags);
    TRACE("mah.dwTotalFrames=%ld\n", 		infoPtr->mah.dwTotalFrames);
    TRACE("mah.dwInitialFrames=%ld\n", 		infoPtr->mah.dwInitialFrames);
    TRACE("mah.dwStreams=%ld\n", 		infoPtr->mah.dwStreams);
    TRACE("mah.dwSuggestedBufferSize=%ld\n",	infoPtr->mah.dwSuggestedBufferSize);
    TRACE("mah.dwWidth=%ld\n", 			infoPtr->mah.dwWidth);
    TRACE("mah.dwHeight=%ld\n", 		infoPtr->mah.dwHeight);

    mmioAscend(infoPtr->hMMio, &mmckInfo, 0);

    mmckList.fccType = mmioFOURCC('s', 't', 'r', 'l');
    if (mmioDescend(infoPtr->hMMio, &mmckList, &mmckHead, MMIO_FINDLIST) != 0) {
	WARN("Can't find 'strl' list\n");
	return FALSE;
    }

    mmckInfo.ckid = mmioFOURCC('s', 't', 'r', 'h');
    if (mmioDescend(infoPtr->hMMio, &mmckInfo, &mmckList, MMIO_FINDCHUNK) != 0) {
	WARN("Can't find 'strh' chunk\n");
	return FALSE;
    }

    mmioRead(infoPtr->hMMio, (LPSTR)&infoPtr->ash, sizeof(infoPtr->ash));

    TRACE("ash.fccType='%c%c%c%c'\n", 		LOBYTE(LOWORD(infoPtr->ash.fccType)),
	                                        HIBYTE(LOWORD(infoPtr->ash.fccType)),
	                                        LOBYTE(HIWORD(infoPtr->ash.fccType)),
	                                        HIBYTE(HIWORD(infoPtr->ash.fccType)));
    TRACE("ash.fccHandler='%c%c%c%c'\n",	LOBYTE(LOWORD(infoPtr->ash.fccHandler)),
	                                        HIBYTE(LOWORD(infoPtr->ash.fccHandler)),
	                                        LOBYTE(HIWORD(infoPtr->ash.fccHandler)),
	                                        HIBYTE(HIWORD(infoPtr->ash.fccHandler)));
    TRACE("ash.dwFlags=%ld\n", 			infoPtr->ash.dwFlags);
    TRACE("ash.wPriority=%d\n", 		infoPtr->ash.wPriority);
    TRACE("ash.wLanguage=%d\n", 		infoPtr->ash.wLanguage);
    TRACE("ash.dwInitialFrames=%ld\n", 		infoPtr->ash.dwInitialFrames);
    TRACE("ash.dwScale=%ld\n", 			infoPtr->ash.dwScale);
    TRACE("ash.dwRate=%ld\n", 			infoPtr->ash.dwRate);
    TRACE("ash.dwStart=%ld\n", 			infoPtr->ash.dwStart);
    TRACE("ash.dwLength=%ld\n", 		infoPtr->ash.dwLength);
    TRACE("ash.dwSuggestedBufferSize=%ld\n", 	infoPtr->ash.dwSuggestedBufferSize);
    TRACE("ash.dwQuality=%ld\n", 		infoPtr->ash.dwQuality);
    TRACE("ash.dwSampleSize=%ld\n", 		infoPtr->ash.dwSampleSize);
    TRACE("ash.rcFrame=(%d,%d,%d,%d)\n", 	infoPtr->ash.rcFrame.top, infoPtr->ash.rcFrame.left,
	  infoPtr->ash.rcFrame.bottom, infoPtr->ash.rcFrame.right);

    mmioAscend(infoPtr->hMMio, &mmckInfo, 0);

    mmckInfo.ckid = mmioFOURCC('s', 't', 'r', 'f');
    if (mmioDescend(infoPtr->hMMio, &mmckInfo, &mmckList, MMIO_FINDCHUNK) != 0) {
	WARN("Can't find 'strh' chunk\n");
	return FALSE;
    }

    infoPtr->inbih = HeapAlloc(GetProcessHeap(), 0, mmckInfo.cksize);
    if (!infoPtr->inbih) {
	WARN("Can't alloc input BIH\n");
	return FALSE;
    }

    mmioRead(infoPtr->hMMio, (LPSTR)infoPtr->inbih, mmckInfo.cksize);

    TRACE("bih.biSize=%ld\n", 		infoPtr->inbih->biSize);
    TRACE("bih.biWidth=%ld\n", 		infoPtr->inbih->biWidth);
    TRACE("bih.biHeight=%ld\n", 	infoPtr->inbih->biHeight);
    TRACE("bih.biPlanes=%d\n", 		infoPtr->inbih->biPlanes);
    TRACE("bih.biBitCount=%d\n", 	infoPtr->inbih->biBitCount);
    TRACE("bih.biCompression=%ld\n", 	infoPtr->inbih->biCompression);
    TRACE("bih.biSizeImage=%ld\n", 	infoPtr->inbih->biSizeImage);
    TRACE("bih.biXPelsPerMeter=%ld\n", 	infoPtr->inbih->biXPelsPerMeter);
    TRACE("bih.biYPelsPerMeter=%ld\n", 	infoPtr->inbih->biYPelsPerMeter);
    TRACE("bih.biClrUsed=%ld\n", 	infoPtr->inbih->biClrUsed);
    TRACE("bih.biClrImportant=%ld\n", 	infoPtr->inbih->biClrImportant);

    mmioAscend(infoPtr->hMMio, &mmckInfo, 0);

    mmioAscend(infoPtr->hMMio, &mmckList, 0);

#if 0
    /* an AVI has 0 or 1 video stream, and to be animated should not contain
     * an audio stream, so only one strl is allowed
     */
    mmckList.fccType = mmioFOURCC('s', 't', 'r', 'l');
    if (mmioDescend(infoPtr->hMMio, &mmckList, &mmckHead, MMIO_FINDLIST) == 0) {
	WARN("There should be a single 'strl' list\n");
	return FALSE;
    }
#endif

    mmioAscend(infoPtr->hMMio, &mmckHead, 0);

    /* no need to read optional JUNK chunk */

    mmckList.fccType = mmioFOURCC('m', 'o', 'v', 'i');
    if (mmioDescend(infoPtr->hMMio, &mmckList, &ckMainRIFF, MMIO_FINDLIST) != 0) {
	WARN("Can't find 'movi' list\n");
	return FALSE;
    }

    /* FIXME: should handle the 'rec ' LIST when present */

    infoPtr->lpIndex = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
				 infoPtr->mah.dwTotalFrames * sizeof(DWORD));
    if (!infoPtr->lpIndex) {
	WARN("Can't alloc index array\n");
	return FALSE;
    }

    numFrame = insize = 0;
    while (mmioDescend(infoPtr->hMMio, &mmckInfo, &mmckList, 0) == 0 &&
	   numFrame < infoPtr->mah.dwTotalFrames) {
	infoPtr->lpIndex[numFrame] = mmckInfo.dwDataOffset;
	if (insize < mmckInfo.cksize)
	    insize = mmckInfo.cksize;
	numFrame++;
	mmioAscend(infoPtr->hMMio, &mmckInfo, 0);
    }
    if (numFrame != infoPtr->mah.dwTotalFrames) {
	WARN("Found %ld frames (/%ld)\n", numFrame, infoPtr->mah.dwTotalFrames);
	return FALSE;
    }
    if (insize > infoPtr->ash.dwSuggestedBufferSize) {
	WARN("insize=%ld suggestedSize=%ld\n", insize, infoPtr->ash.dwSuggestedBufferSize);
	infoPtr->ash.dwSuggestedBufferSize = insize;
    }

    infoPtr->indata = HeapAlloc(GetProcessHeap(), 0, infoPtr->ash.dwSuggestedBufferSize);
    if (!infoPtr->indata) {
	WARN("Can't alloc input buffer\n");
	return FALSE;
    }

    return TRUE;
}


static BOOL    ANIMATE_GetAviCodec(ANIMATE_INFO *infoPtr)
{
    DWORD	outSize;

    /* check uncompressed AVI */
    if ((infoPtr->ash.fccHandler == mmioFOURCC('D', 'I', 'B', ' ')) ||
       (infoPtr->ash.fccHandler == mmioFOURCC('R', 'L', 'E', ' ')) ||
       (infoPtr->ash.fccHandler == mmioFOURCC(0, 0, 0, 0)))
    {
        infoPtr->hic = 0;
	return TRUE;
    }

    /* try to get a decompressor for that type */
    infoPtr->hic = fnIC.fnICOpen(ICTYPE_VIDEO, infoPtr->ash.fccHandler, ICMODE_DECOMPRESS);
    if (!infoPtr->hic) {
	WARN("Can't load codec for the file\n");
	return FALSE;
    }

    outSize = fnIC.fnICSendMessage(infoPtr->hic, ICM_DECOMPRESS_GET_FORMAT,
			    (DWORD)infoPtr->inbih, 0L);

    infoPtr->outbih = HeapAlloc(GetProcessHeap(), 0, outSize);
    if (!infoPtr->outbih) {
	WARN("Can't alloc output BIH\n");
	return FALSE;
    }

    if (fnIC.fnICSendMessage(infoPtr->hic, ICM_DECOMPRESS_GET_FORMAT,
		      (DWORD)infoPtr->inbih, (DWORD)infoPtr->outbih) != outSize) {
	WARN("Can't get output BIH\n");
	return FALSE;
    }

    infoPtr->outdata = HeapAlloc(GetProcessHeap(), 0, infoPtr->outbih->biSizeImage);
    if (!infoPtr->outdata) {
	WARN("Can't alloc output buffer\n");
	return FALSE;
    }

    if (fnIC.fnICSendMessage(infoPtr->hic, ICM_DECOMPRESS_BEGIN,
		      (DWORD)infoPtr->inbih, (DWORD)infoPtr->outbih) != ICERR_OK) {
	WARN("Can't begin decompression\n");
	return FALSE;
    }

    return TRUE;
}

static LRESULT ANIMATE_OpenA(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    ANIMATE_INFO *infoPtr = ANIMATE_GetInfoPtr(hWnd);
    HINSTANCE hInstance = (HINSTANCE)wParam;

    ANIMATE_Free(infoPtr);
    infoPtr->hwndSelf = hWnd;

    if (!lParam) {
	TRACE("Closing avi!\n");
        /* installer of thebat! v1.62 requires FALSE here */
	return (infoPtr->hMMio != 0);
    }

    if (!hInstance)
       hInstance = (HINSTANCE)GetWindowLongA(hWnd, GWL_HINSTANCE);

    if (HIWORD(lParam)) {
	TRACE("(\"%s\");\n", (LPSTR)lParam);

	if (!ANIMATE_LoadResA(infoPtr, hInstance, (LPSTR)lParam)) {
	    TRACE("No AVI resource found!\n");
	    if (!ANIMATE_LoadFileA(infoPtr, (LPSTR)lParam)) {
		WARN("No AVI file found!\n");
		return FALSE;
	    }
	}
    } else {
	TRACE("(%u);\n", (WORD)LOWORD(lParam));

	if (!ANIMATE_LoadResA(infoPtr, hInstance,
			      MAKEINTRESOURCEA((INT)lParam))) {
	    WARN("No AVI resource found!\n");
	    return FALSE;
	}
    }

    if (!ANIMATE_GetAviInfo(infoPtr)) {
	WARN("Can't get AVI information\n");
	ANIMATE_Free(infoPtr);
	return FALSE;
    }

    if (!ANIMATE_GetAviCodec(infoPtr)) {
	WARN("Can't get AVI Codec\n");
	ANIMATE_Free(infoPtr);
	return FALSE;
    }

    if (!GetWindowLongA(hWnd, GWL_STYLE) & ACS_CENTER) {
	SetWindowPos(hWnd, 0, 0, 0, infoPtr->mah.dwWidth, infoPtr->mah.dwHeight,
		     SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
    }

    if (GetWindowLongA(hWnd, GWL_STYLE) & ACS_AUTOPLAY) {
	return ANIMATE_Play(hWnd, -1, (LPARAM)MAKELONG(0, infoPtr->mah.dwTotalFrames-1));
    }

    return TRUE;
}


/* << ANIMATE_Open32W >> */

static LRESULT ANIMATE_Stop(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    ANIMATE_INFO *infoPtr = ANIMATE_GetInfoPtr(hWnd);

    /* nothing opened */
    if (!infoPtr->hMMio)
	return FALSE;

    ANIMATE_DoStop(infoPtr);
    return TRUE;
}


static LRESULT ANIMATE_Create(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    ANIMATE_INFO*	infoPtr;

    if (!fnIC.hModule) /* FIXME: not thread safe */
    {
	/* since there's a circular dep between msvfw32 and comctl32, we could either:
	 * - fix the build chain to allow this circular dep
	 * - handle it by hand
	 * AJ wants the latter :-(
	 */
	fnIC.hModule = LoadLibraryA("msvfw32.dll");
	if (!fnIC.hModule) return FALSE;

	fnIC.fnICOpen        = (void*)GetProcAddress(fnIC.hModule, "ICOpen");
	fnIC.fnICClose       = (void*)GetProcAddress(fnIC.hModule, "ICClose");
	fnIC.fnICSendMessage = (void*)GetProcAddress(fnIC.hModule, "ICSendMessage");
	fnIC.fnICDecompress  = (void*)GetProcAddress(fnIC.hModule, "ICDecompress");
    }

    /* allocate memory for info structure */
    infoPtr = (ANIMATE_INFO *)Alloc(sizeof(ANIMATE_INFO));
    if (!infoPtr) {
	ERR("could not allocate info memory!\n");
	return 0;
    }

    /* store crossref hWnd <-> info structure */
    SetWindowLongA(hWnd, 0, (DWORD)infoPtr);
    infoPtr->hwndSelf = hWnd;
    infoPtr->hwndNotify = ((LPCREATESTRUCTA)lParam)->hwndParent;
    infoPtr->transparentColor = ANIMATE_COLOR_NONE;
    infoPtr->hbmPrevFrame = 0;

    TRACE("Animate style=0x%08lx, parent=%08lx\n", GetWindowLongA(hWnd, GWL_STYLE), (DWORD)infoPtr->hwndNotify);

    InitializeCriticalSection(&infoPtr->cs);

    return 0;
}


static LRESULT ANIMATE_Destroy(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    ANIMATE_INFO *infoPtr = ANIMATE_GetInfoPtr(hWnd);


    /* free avi data */
    ANIMATE_Free(infoPtr);

    /* free animate info data */
    Free(infoPtr);
    SetWindowLongA(hWnd, 0, 0);

    return 0;
}


static LRESULT ANIMATE_EraseBackground(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    ANIMATE_INFO *infoPtr = ANIMATE_GetInfoPtr(hWnd);
    RECT rect;
    HBRUSH hBrush = 0;

    if(GetWindowLongA(hWnd, GWL_STYLE) & ACS_TRANSPARENT)
    {
        hBrush = (HBRUSH)SendMessageA(infoPtr->hwndNotify,WM_CTLCOLORSTATIC,
				      wParam, (LPARAM)hWnd);
    }

    GetClientRect(hWnd, &rect);
    FillRect((HDC)wParam, &rect, hBrush ? hBrush : GetCurrentObject((HDC)wParam, OBJ_BRUSH));

    return TRUE;
}

static LRESULT WINAPI ANIMATE_Size(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    if (GetWindowLongA(hWnd, GWL_STYLE) & ACS_CENTER) {
	InvalidateRect(hWnd, NULL, TRUE);
    }
    return TRUE;
}

static LRESULT WINAPI ANIMATE_WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TRACE("hwnd=%p msg=%x wparam=%x lparam=%lx\n", hWnd, uMsg, wParam, lParam);
    if (!ANIMATE_GetInfoPtr(hWnd) && (uMsg != WM_NCCREATE))
	return DefWindowProcA(hWnd, uMsg, wParam, lParam);
    switch (uMsg)
    {
    case ACM_OPENA:
	return ANIMATE_OpenA(hWnd, wParam, lParam);

	/*	case ACM_OPEN32W: FIXME!! */
	/*	    return ANIMATE_Open32W(hWnd, wParam, lParam); */

    case ACM_PLAY:
	return ANIMATE_Play(hWnd, wParam, lParam);

    case ACM_STOP:
	return ANIMATE_Stop(hWnd, wParam, lParam);

    case WM_NCCREATE:
	ANIMATE_Create(hWnd, wParam, lParam);
	return DefWindowProcA(hWnd, uMsg, wParam, lParam);

    case WM_NCHITTEST:
	return HTTRANSPARENT;

    case WM_DESTROY:
	ANIMATE_Destroy(hWnd, wParam, lParam);
	return DefWindowProcA(hWnd, uMsg, wParam, lParam);

    case WM_ERASEBKGND:
	ANIMATE_EraseBackground(hWnd, wParam, lParam);
	break;

    /*	case WM_STYLECHANGED: FIXME shall we do something ?? */

    case WM_TIMER:
    	if (GetWindowLongA(hWnd, GWL_STYLE) & ACS_TRANSPARENT)
        {
            ANIMATE_INFO* infoPtr = ANIMATE_GetInfoPtr(hWnd);
            infoPtr->hbrushBG = (HBRUSH)SendMessageA(infoPtr->hwndNotify,
						     WM_CTLCOLORSTATIC,
						     wParam, (LPARAM)hWnd);
        }
	return ANIMATE_DrawFrame(ANIMATE_GetInfoPtr(hWnd));

    case WM_CLOSE:
	ANIMATE_Free(ANIMATE_GetInfoPtr(hWnd));
	return TRUE;

    case WM_PAINT:
        {
            ANIMATE_INFO* infoPtr = ANIMATE_GetInfoPtr(hWnd);

            /* the animation isn't playing, or has not decompressed
             * (and displayed) the first frame yet, don't paint
             */
            if ((!infoPtr->uTimer && !infoPtr->hThread) ||
                !infoPtr->hbmPrevFrame)
            {
                /* default paint handling */
                return DefWindowProcA(hWnd, uMsg, wParam, lParam);
            }

            if (GetWindowLongA(hWnd, GWL_STYLE) & ACS_TRANSPARENT)
                infoPtr->hbrushBG = (HBRUSH)SendMessageA(infoPtr->hwndNotify,
							 WM_CTLCOLORSTATIC,
							 wParam, (LPARAM)hWnd);

            if (wParam)
            {
                EnterCriticalSection(&infoPtr->cs);
                ANIMATE_PaintFrame(infoPtr, (HDC)wParam);
                LeaveCriticalSection(&infoPtr->cs);
            }
            else
            {
	        PAINTSTRUCT ps;
 	        HDC hDC = BeginPaint(hWnd, &ps);

                EnterCriticalSection(&infoPtr->cs);
                ANIMATE_PaintFrame(infoPtr, hDC);
                LeaveCriticalSection(&infoPtr->cs);

	        EndPaint(hWnd, &ps);
	    }
        }
	break;

    case WM_SIZE:
	ANIMATE_Size(hWnd, wParam, lParam);
	return DefWindowProcA(hWnd, uMsg, wParam, lParam);

    default:
	if ((uMsg >= WM_USER) && (uMsg < WM_APP))
	    ERR("unknown msg %04x wp=%08x lp=%08lx\n", uMsg, wParam, lParam);

	return DefWindowProcA(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

void ANIMATE_Register(void)
{
    WNDCLASSA wndClass;

    ZeroMemory(&wndClass, sizeof(WNDCLASSA));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS;
    wndClass.lpfnWndProc   = (WNDPROC)ANIMATE_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(ANIMATE_INFO *);
    wndClass.hCursor       = LoadCursorA(0, (LPSTR)IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wndClass.lpszClassName = ANIMATE_CLASSA;

    RegisterClassA(&wndClass);
}


void ANIMATE_Unregister(void)
{
    UnregisterClassA(ANIMATE_CLASSA, NULL);
}
