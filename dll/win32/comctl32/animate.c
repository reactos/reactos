/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*
 * Animation control
 *
 * Copyright 1998, 1999 Eric Kohl
 * Copyright 1999 Eric Pouech
 * Copyright 2005 Dimitrie O. Paun
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
 *
 * TODO:
 *   - check for the 'rec ' list in some AVI files
 */

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
    LRESULT     (WINAPI *fnICSendMessage)(HIC, UINT, DWORD_PTR, DWORD_PTR);
    DWORD       (WINAPIV *fnICDecompress)(HIC,DWORD,LPBITMAPINFOHEADER,LPVOID,LPBITMAPINFOHEADER,LPVOID);
} fnIC;

typedef struct
{
   /* reference to input stream (file or resource) */
   HGLOBAL 		hRes;
   HMMIO		hMMio;	/* handle to mmio stream */
   HWND			hwndSelf;
   HWND			hwndNotify;
   DWORD		dwStyle;
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
   HANDLE		hStopEvent;
   HANDLE		hThread;
   DWORD		threadId;
   UINT			uTimer;
   /* data for playing the file */
   int			nFromFrame;
   int			nToFrame;
   int			nLoop;
   int			currFrame;
   /* transparency info*/
   COLORREF         	transparentColor;
   HBRUSH           	hbrushBG;
   HBITMAP  	    	hbmPrevFrame;
} ANIMATE_INFO;

#define ANIMATE_COLOR_NONE  	0xffffffff

static void ANIMATE_Notify(const ANIMATE_INFO *infoPtr, UINT notif)
{
    PostMessageW(infoPtr->hwndNotify, WM_COMMAND,
		 MAKEWPARAM(GetDlgCtrlID(infoPtr->hwndSelf), notif),
		 (LPARAM)infoPtr->hwndSelf);
}

static BOOL ANIMATE_LoadResW(ANIMATE_INFO *infoPtr, HINSTANCE hInst, LPCWSTR lpName)
{
    HRSRC 	hrsrc;
    MMIOINFO	mminfo;
    LPVOID	lpAvi;

    hrsrc = FindResourceW(hInst, lpName, L"AVI");
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
    mminfo.pchBuffer = lpAvi;
    mminfo.cchBuffer = SizeofResource(hInst, hrsrc);
    infoPtr->hMMio = mmioOpenW(NULL, &mminfo, MMIO_READ);
    if (!infoPtr->hMMio) 
    {
	FreeResource(infoPtr->hRes);
	return FALSE;
    }

    return TRUE;
}


static BOOL ANIMATE_LoadFileW(ANIMATE_INFO *infoPtr, LPWSTR lpName)
{
    infoPtr->hMMio = mmioOpenW(lpName, 0, MMIO_ALLOCBUF | MMIO_READ | MMIO_DENYWRITE);

    if(!infoPtr->hMMio) return FALSE;
    return TRUE;
}


static BOOL ANIMATE_DoStop(ANIMATE_INFO *infoPtr)
{
    BOOL stopped = FALSE;

    EnterCriticalSection(&infoPtr->cs);

    /* should stop playing */
    if (infoPtr->hThread)
    {
        HANDLE handle = infoPtr->hThread;

        TRACE("stopping animation thread\n");
        infoPtr->hThread = 0;
        SetEvent( infoPtr->hStopEvent );

        if (infoPtr->threadId != GetCurrentThreadId())
        {
            LeaveCriticalSection(&infoPtr->cs);  /* leave it a chance to run */
            WaitForSingleObject( handle, INFINITE );
            TRACE("animation thread stopped\n");
            EnterCriticalSection(&infoPtr->cs);
        }

        CloseHandle( handle );
        CloseHandle( infoPtr->hStopEvent );
        infoPtr->hStopEvent = 0;
        stopped = TRUE;
    }
    if (infoPtr->uTimer) {
	KillTimer(infoPtr->hwndSelf, infoPtr->uTimer);
	infoPtr->uTimer = 0;
	stopped = TRUE;
    }

    LeaveCriticalSection(&infoPtr->cs);

    if (stopped)
        ANIMATE_Notify(infoPtr, ACN_STOP);

    return TRUE;
}


static void ANIMATE_Free(ANIMATE_INFO *infoPtr)
{
    if (infoPtr->hMMio)
    {
        ANIMATE_DoStop(infoPtr);
        mmioClose(infoPtr->hMMio, 0);
        if (infoPtr->hRes)
        {
            FreeResource(infoPtr->hRes);
            infoPtr->hRes = 0;
        }
        Free (infoPtr->lpIndex);
        infoPtr->lpIndex = NULL;
        if (infoPtr->hic)
        {
            fnIC.fnICClose(infoPtr->hic);
            infoPtr->hic = 0;
        }
        Free (infoPtr->inbih);
        infoPtr->inbih = NULL;
        Free (infoPtr->outbih);
        infoPtr->outbih = NULL;
        Free (infoPtr->indata);
        infoPtr->indata = NULL;
        Free (infoPtr->outdata);
        infoPtr->outdata = NULL;
        if (infoPtr->hbmPrevFrame)
        {
            DeleteObject(infoPtr->hbmPrevFrame);
            infoPtr->hbmPrevFrame = 0;
        }

        memset(&infoPtr->mah, 0, sizeof(infoPtr->mah));
        memset(&infoPtr->ash, 0, sizeof(infoPtr->ash));
        infoPtr->nFromFrame = infoPtr->nToFrame = infoPtr->nLoop = infoPtr->currFrame = 0;
    }
    infoPtr->transparentColor = ANIMATE_COLOR_NONE;
}

static void ANIMATE_TransparentBlt(ANIMATE_INFO const *infoPtr, HDC hdcDest, HDC hdcSource)
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

static BOOL ANIMATE_PaintFrame(ANIMATE_INFO* infoPtr, HDC hDC)
{
    void const *pBitmapData;
    BITMAPINFO const *pBitmapInfo;
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
    } 
    else
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

    hdcMem = CreateCompatibleDC(hDC);
    hbmOld = SelectObject(hdcMem, infoPtr->hbmPrevFrame);

    SetDIBits(hdcMem, infoPtr->hbmPrevFrame, 0, nHeight, pBitmapData, pBitmapInfo, DIB_RGB_COLORS);

    /*
     * we need to get the transparent color even without ACS_TRANSPARENT,
     * because the style can be changed later on and the color should always
     * be obtained in the first frame
     */
    if(infoPtr->transparentColor == ANIMATE_COLOR_NONE)
    {
        infoPtr->transparentColor = GetPixel(hdcMem,0,0);
    }

    if(infoPtr->dwStyle & ACS_TRANSPARENT)
    {
        HDC hdcFinal = CreateCompatibleDC(hDC);
        HBITMAP hbmFinal = CreateCompatibleBitmap(hDC,nWidth, nHeight);
        HBITMAP hbmOld2 = SelectObject(hdcFinal, hbmFinal);
        RECT rect;

        SetRect(&rect, 0, 0, nWidth, nHeight);

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

    if (infoPtr->dwStyle & ACS_CENTER)
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

static BOOL ANIMATE_DrawFrame(ANIMATE_INFO *infoPtr, HDC hDC)
{
    TRACE("Drawing frame %d (loop %d)\n", infoPtr->currFrame, infoPtr->nLoop);

    mmioSeek(infoPtr->hMMio, infoPtr->lpIndex[infoPtr->currFrame], SEEK_SET);
    mmioRead(infoPtr->hMMio, infoPtr->indata, infoPtr->ash.dwSuggestedBufferSize);

    if (infoPtr->hic &&
	fnIC.fnICDecompress(infoPtr->hic, 0, infoPtr->inbih, infoPtr->indata,
		     infoPtr->outbih, infoPtr->outdata) != ICERR_OK) {
	WARN("Decompression error\n");
	return FALSE;
    }

    ANIMATE_PaintFrame(infoPtr, hDC);

    if (infoPtr->currFrame++ >= infoPtr->nToFrame) {
	infoPtr->currFrame = infoPtr->nFromFrame;
	if (infoPtr->nLoop != -1) {
	    if (--infoPtr->nLoop == 0) {
		ANIMATE_DoStop(infoPtr);
	    }
	}
    }

    return TRUE;
}

static LRESULT ANIMATE_Timer(ANIMATE_INFO *infoPtr)
{
    HDC	hDC;

    if ((hDC = GetDC(infoPtr->hwndSelf)) != 0)
    {
        EnterCriticalSection(&infoPtr->cs);
        ANIMATE_DrawFrame(infoPtr, hDC);
        LeaveCriticalSection(&infoPtr->cs);

	ReleaseDC(infoPtr->hwndSelf, hDC);
    }

    return 0;
}

static DWORD CALLBACK ANIMATE_AnimationThread(LPVOID ptr_)
{
    ANIMATE_INFO *infoPtr = ptr_;
    HANDLE event;
    DWORD timeout;

    while(1)
    {
        HDC hDC = GetDC(infoPtr->hwndSelf);

        EnterCriticalSection(&infoPtr->cs);
        ANIMATE_DrawFrame(infoPtr, hDC);
        timeout = infoPtr->mah.dwMicroSecPerFrame;
        event = infoPtr->hStopEvent;
        LeaveCriticalSection(&infoPtr->cs);

        ReleaseDC(infoPtr->hwndSelf, hDC);

        /* time is in microseconds, we should convert it to milliseconds */
        if ((event == 0) || WaitForSingleObject( event, (timeout+500)/1000) == WAIT_OBJECT_0)
            break;
    }
    return TRUE;
}

static LRESULT ANIMATE_Play(ANIMATE_INFO *infoPtr, UINT cRepeat, WORD wFrom, WORD wTo)
{
    /* nothing opened */
    if (!infoPtr->hMMio)
	return FALSE;

    if (infoPtr->hThread || infoPtr->uTimer) {
	TRACE("Already playing\n");
	return TRUE;
    }

    infoPtr->nFromFrame = wFrom;
    infoPtr->nToFrame   = wTo;
    infoPtr->nLoop      = cRepeat;

    if (infoPtr->nToFrame == 0xFFFF)
	infoPtr->nToFrame = infoPtr->mah.dwTotalFrames - 1;

    TRACE("(repeat=%d from=%d to=%d);\n",
	  infoPtr->nLoop, infoPtr->nFromFrame, infoPtr->nToFrame);

    if (infoPtr->nFromFrame >= infoPtr->mah.dwTotalFrames &&
        (SHORT)infoPtr->nFromFrame < 0)
        infoPtr->nFromFrame = 0;

    if (infoPtr->nFromFrame > infoPtr->nToFrame ||
	infoPtr->nToFrame >= infoPtr->mah.dwTotalFrames)
	return FALSE;

    infoPtr->currFrame = infoPtr->nFromFrame;

    /* seek - doesn't need to start a thread or set a timer and neither
     * does it send a notification */
    if (infoPtr->nFromFrame == infoPtr->nToFrame)
    {
        HDC hDC;

        if ((hDC = GetDC(infoPtr->hwndSelf)) != 0)
        {
            ANIMATE_DrawFrame(infoPtr, hDC);

	    ReleaseDC(infoPtr->hwndSelf, hDC);
        }
        return TRUE;
    }

    if (infoPtr->dwStyle & ACS_TIMER) 
    {
	TRACE("Using a timer\n");
	/* create a timer to display AVI */
	infoPtr->uTimer = SetTimer(infoPtr->hwndSelf, 1, 
                                   infoPtr->mah.dwMicroSecPerFrame / 1000, NULL);
    } 
    else 
    {
	TRACE("Using an animation thread\n");
        infoPtr->hStopEvent = CreateEventW( NULL, TRUE, FALSE, NULL );
        infoPtr->hThread = CreateThread(0, 0, ANIMATE_AnimationThread,
                                        infoPtr, 0, &infoPtr->threadId);
        if(!infoPtr->hThread) return FALSE;

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
    TRACE("mah.dwMaxBytesPerSec=%ld\n",		infoPtr->mah.dwMaxBytesPerSec);
    TRACE("mah.dwPaddingGranularity=%ld\n", 	infoPtr->mah.dwPaddingGranularity);
    TRACE("mah.dwFlags=%ld\n",			infoPtr->mah.dwFlags);
    TRACE("mah.dwTotalFrames=%ld\n",		infoPtr->mah.dwTotalFrames);
    TRACE("mah.dwInitialFrames=%ld\n",		infoPtr->mah.dwInitialFrames);
    TRACE("mah.dwStreams=%ld\n",		infoPtr->mah.dwStreams);
    TRACE("mah.dwSuggestedBufferSize=%ld\n",	infoPtr->mah.dwSuggestedBufferSize);
    TRACE("mah.dwWidth=%ld\n",			infoPtr->mah.dwWidth);
    TRACE("mah.dwHeight=%ld\n",			infoPtr->mah.dwHeight);

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

    TRACE("ash.fccType=%s\n",			debugstr_fourcc(infoPtr->ash.fccType));
    TRACE("ash.fccHandler=%s\n",		debugstr_fourcc(infoPtr->ash.fccHandler));
    TRACE("ash.dwFlags=%ld\n", 			infoPtr->ash.dwFlags);
    TRACE("ash.wPriority=%d\n", 		infoPtr->ash.wPriority);
    TRACE("ash.wLanguage=%d\n", 		infoPtr->ash.wLanguage);
    TRACE("ash.dwInitialFrames=%ld\n", 		infoPtr->ash.dwInitialFrames);
    TRACE("ash.dwScale=%ld\n", 			infoPtr->ash.dwScale);
    TRACE("ash.dwRate=%ld\n", 			infoPtr->ash.dwRate);
    TRACE("ash.dwStart=%ld\n", 			infoPtr->ash.dwStart);
    TRACE("ash.dwLength=%lu\n",			infoPtr->ash.dwLength);
    TRACE("ash.dwSuggestedBufferSize=%lu\n", 	infoPtr->ash.dwSuggestedBufferSize);
    TRACE("ash.dwQuality=%lu\n", 		infoPtr->ash.dwQuality);
    TRACE("ash.dwSampleSize=%lu\n", 		infoPtr->ash.dwSampleSize);
    TRACE("ash.rcFrame=(%d,%d,%d,%d)\n", 	infoPtr->ash.rcFrame.top, infoPtr->ash.rcFrame.left,
	  infoPtr->ash.rcFrame.bottom, infoPtr->ash.rcFrame.right);

    mmioAscend(infoPtr->hMMio, &mmckInfo, 0);

    mmckInfo.ckid = mmioFOURCC('s', 't', 'r', 'f');
    if (mmioDescend(infoPtr->hMMio, &mmckInfo, &mmckList, MMIO_FINDCHUNK) != 0) {
	WARN("Can't find 'strh' chunk\n");
	return FALSE;
    }

    infoPtr->inbih = Alloc(mmckInfo.cksize);
    if (!infoPtr->inbih) {
	WARN("Can't alloc input BIH\n");
	return FALSE;
    }

    mmioRead(infoPtr->hMMio, (LPSTR)infoPtr->inbih, mmckInfo.cksize);

    TRACE("bih.biSize=%lu\n", 		infoPtr->inbih->biSize);
    TRACE("bih.biWidth=%ld\n", 		infoPtr->inbih->biWidth);
    TRACE("bih.biHeight=%ld\n",		infoPtr->inbih->biHeight);
    TRACE("bih.biPlanes=%d\n", 		infoPtr->inbih->biPlanes);
    TRACE("bih.biBitCount=%d\n", 	infoPtr->inbih->biBitCount);
    TRACE("bih.biCompression=%lu\n", 	infoPtr->inbih->biCompression);
    TRACE("bih.biSizeImage=%lu\n", 	infoPtr->inbih->biSizeImage);
    TRACE("bih.biXPelsPerMeter=%lu\n", 	infoPtr->inbih->biXPelsPerMeter);
    TRACE("bih.biYPelsPerMeter=%lu\n", 	infoPtr->inbih->biYPelsPerMeter);
    TRACE("bih.biClrUsed=%lu\n", 	infoPtr->inbih->biClrUsed);
    TRACE("bih.biClrImportant=%lu\n", 	infoPtr->inbih->biClrImportant);

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

    infoPtr->lpIndex = Alloc(infoPtr->mah.dwTotalFrames * sizeof(DWORD));
    if (!infoPtr->lpIndex) 
	return FALSE;

    numFrame = insize = 0;
    while (mmioDescend(infoPtr->hMMio, &mmckInfo, &mmckList, 0) == 0 &&
	   numFrame < infoPtr->mah.dwTotalFrames) {
	infoPtr->lpIndex[numFrame] = mmckInfo.dwDataOffset;
	if (insize < mmckInfo.cksize)
	    insize = mmckInfo.cksize;
	numFrame++;
	mmioAscend(infoPtr->hMMio, &mmckInfo, 0);
    }
    if (numFrame != infoPtr->mah.dwTotalFrames)
    {
        WARN("Found %lu frames (/%lu)\n", numFrame, infoPtr->mah.dwTotalFrames);
        return FALSE;
    }
    if (insize > infoPtr->ash.dwSuggestedBufferSize)
    {
        WARN("insize %lu suggestedSize %lu\n", insize, infoPtr->ash.dwSuggestedBufferSize);
        infoPtr->ash.dwSuggestedBufferSize = insize;
    }

    infoPtr->indata = Alloc(infoPtr->ash.dwSuggestedBufferSize);
    if (!infoPtr->indata) 
	return FALSE;

    return TRUE;
}


static BOOL ANIMATE_GetAviCodec(ANIMATE_INFO *infoPtr)
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
			    (DWORD_PTR)infoPtr->inbih, 0L);

    if (!(infoPtr->outbih = Alloc(outSize)))
        return FALSE;

    if (fnIC.fnICSendMessage(infoPtr->hic, ICM_DECOMPRESS_GET_FORMAT,
		      (DWORD_PTR)infoPtr->inbih, (DWORD_PTR)infoPtr->outbih) != ICERR_OK) 
    {
	WARN("Can't get output BIH\n");
	return FALSE;
    }

    if (!(infoPtr->outdata = Alloc(infoPtr->outbih->biSizeImage)))
        return FALSE;

    if (fnIC.fnICSendMessage(infoPtr->hic, ICM_DECOMPRESS_BEGIN,
		      (DWORD_PTR)infoPtr->inbih, (DWORD_PTR)infoPtr->outbih) != ICERR_OK) {
	WARN("Can't begin decompression\n");
	return FALSE;
    }

    return TRUE;
}


static BOOL ANIMATE_OpenW(ANIMATE_INFO *infoPtr, HINSTANCE hInstance, LPWSTR lpszName)
{
    HDC hdc;

    ANIMATE_Free(infoPtr);

    if (!lpszName) 
    {
	TRACE("Closing avi.\n");
        /* installer of thebat! v1.62 requires FALSE here */
	return (infoPtr->hMMio != 0);
    }

    if (!hInstance)
        hInstance = (HINSTANCE)GetWindowLongPtrW(infoPtr->hwndSelf, GWLP_HINSTANCE);

    TRACE("(%s)\n", debugstr_w(lpszName));

    if (!IS_INTRESOURCE(lpszName))
    {
	if (!ANIMATE_LoadResW(infoPtr, hInstance, lpszName)) 
        {
	    TRACE("No AVI resource found.\n");
	    if (!ANIMATE_LoadFileW(infoPtr, lpszName)) 
            {
		WARN("No AVI file found.\n");
		return FALSE;
	    }
	}
    } 
    else 
    {
	if (!ANIMATE_LoadResW(infoPtr, hInstance, lpszName))
        {
	    WARN("No AVI resource found.\n");
	    return FALSE;
	}
    }

    if (!ANIMATE_GetAviInfo(infoPtr)) 
    {
	WARN("Can't get AVI information\n");
	ANIMATE_Free(infoPtr);
	return FALSE;
    }

    if (!ANIMATE_GetAviCodec(infoPtr)) 
    {
	WARN("Can't get AVI Codec\n");
	ANIMATE_Free(infoPtr);
	return FALSE;
    }

    hdc = GetDC(infoPtr->hwndSelf);
    /* native looks at the top left pixel of the first frame here too. */
    infoPtr->hbrushBG = (HBRUSH)SendMessageW(infoPtr->hwndNotify, WM_CTLCOLORSTATIC,
                                             (WPARAM)hdc, (LPARAM)infoPtr->hwndSelf);
    ReleaseDC(infoPtr->hwndSelf, hdc);

    if (!(infoPtr->dwStyle & ACS_CENTER))
	SetWindowPos(infoPtr->hwndSelf, 0, 0, 0, infoPtr->mah.dwWidth, infoPtr->mah.dwHeight,
		     SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);

    if (infoPtr->dwStyle & ACS_AUTOPLAY) 
	return ANIMATE_Play(infoPtr, -1, 0, infoPtr->mah.dwTotalFrames - 1);

    return TRUE;
}


static BOOL ANIMATE_OpenA(ANIMATE_INFO *infoPtr, HINSTANCE hInstance, LPSTR lpszName)
{
    LPWSTR lpwszName;
    LRESULT result;
    INT len;

    if (IS_INTRESOURCE(lpszName))
        return ANIMATE_OpenW(infoPtr, hInstance, (LPWSTR)lpszName);

    len = MultiByteToWideChar(CP_ACP, 0, lpszName, -1, NULL, 0);
    lpwszName = Alloc(len * sizeof(WCHAR));
    if (!lpwszName) return FALSE;
    MultiByteToWideChar(CP_ACP, 0, lpszName, -1, lpwszName, len);

    result = ANIMATE_OpenW(infoPtr, hInstance, lpwszName);
    Free (lpwszName);
    return result;
}


static BOOL ANIMATE_Stop(ANIMATE_INFO *infoPtr)
{
    /* nothing opened */
    if (!infoPtr->hMMio)
	return FALSE;

    ANIMATE_DoStop(infoPtr);
    return TRUE;
}


static BOOL ANIMATE_Create(HWND hWnd, const CREATESTRUCTW *lpcs)
{
    ANIMATE_INFO *infoPtr;

    if (!fnIC.hModule)
    {
	fnIC.hModule = LoadLibraryW(L"msvfw32.dll");
	if (!fnIC.hModule) return FALSE;

	fnIC.fnICOpen        = (void*)GetProcAddress(fnIC.hModule, "ICOpen");
	fnIC.fnICClose       = (void*)GetProcAddress(fnIC.hModule, "ICClose");
	fnIC.fnICSendMessage = (void*)GetProcAddress(fnIC.hModule, "ICSendMessage");
	fnIC.fnICDecompress  = (void*)GetProcAddress(fnIC.hModule, "ICDecompress");
    }

    /* allocate memory for info structure */
    infoPtr = Alloc(sizeof(*infoPtr));
    if (!infoPtr) return FALSE;

    /* store crossref hWnd <-> info structure */
    SetWindowLongPtrW(hWnd, 0, (DWORD_PTR)infoPtr);
    infoPtr->hwndSelf = hWnd;
    infoPtr->hwndNotify = lpcs->hwndParent;
    infoPtr->transparentColor = ANIMATE_COLOR_NONE;
    infoPtr->hbmPrevFrame = 0;
    infoPtr->dwStyle = lpcs->style;

    TRACE("Animate style %#lx, parent %p\n", infoPtr->dwStyle, infoPtr->hwndNotify);

    InitializeCriticalSectionEx(&infoPtr->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    infoPtr->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": ANIMATE_INFO*->cs");

    return TRUE;
}


static LRESULT ANIMATE_Destroy(ANIMATE_INFO *infoPtr)
{
    /* free avi data */
    ANIMATE_Free(infoPtr);

    /* free animate info data */
    SetWindowLongPtrW(infoPtr->hwndSelf, 0, 0);

    infoPtr->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&infoPtr->cs);
    Free(infoPtr);

    return 0;
}


static BOOL ANIMATE_EraseBackground(ANIMATE_INFO const *infoPtr, HDC hdc)
{
    RECT rect;
    HBRUSH hBrush;

    hBrush = (HBRUSH)SendMessageW(infoPtr->hwndNotify, WM_CTLCOLORSTATIC,
                                  (WPARAM)hdc, (LPARAM)infoPtr->hwndSelf);
    GetClientRect(infoPtr->hwndSelf, &rect);
    FillRect(hdc, &rect, hBrush ? hBrush : GetCurrentObject(hdc, OBJ_BRUSH));

    return TRUE;
}


static LRESULT ANIMATE_StyleChanged(ANIMATE_INFO *infoPtr, WPARAM wStyleType, const STYLESTRUCT *lpss)
{
    TRACE("%#Ix, styleOld %#lx, styleNew %#lx.\n", wStyleType, lpss->styleOld, lpss->styleNew);

    if (wStyleType != GWL_STYLE) return 0;
  
    infoPtr->dwStyle = lpss->styleNew;
    return 0;
}


static LRESULT WINAPI ANIMATE_WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    ANIMATE_INFO *infoPtr = (ANIMATE_INFO *)GetWindowLongPtrW(hWnd, 0);

    TRACE("hwnd %p, msg %x, wparam %#Ix, lparam %#Ix.\n", hWnd, uMsg, wParam, lParam);

    if (!infoPtr && (uMsg != WM_NCCREATE))
	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    switch (uMsg)
    {
    case ACM_OPENA:
	return ANIMATE_OpenA(infoPtr, (HINSTANCE)wParam, (LPSTR)lParam);

    case ACM_OPENW:
	return ANIMATE_OpenW(infoPtr, (HINSTANCE)wParam, (LPWSTR)lParam);

    case ACM_PLAY:
	return ANIMATE_Play(infoPtr, (INT)wParam, LOWORD(lParam), HIWORD(lParam));

    case ACM_STOP:
	return ANIMATE_Stop(infoPtr);

    case WM_CLOSE:
	ANIMATE_Free(infoPtr);
	return 0;

    case WM_NCCREATE:
	return ANIMATE_Create(hWnd, (LPCREATESTRUCTW)lParam);

    case WM_NCHITTEST:
	return HTTRANSPARENT;

    case WM_DESTROY:
	return ANIMATE_Destroy(infoPtr);

    case WM_ERASEBKGND:
	return ANIMATE_EraseBackground(infoPtr, (HDC)wParam);

    case WM_STYLECHANGED:
        return ANIMATE_StyleChanged(infoPtr, wParam, (LPSTYLESTRUCT)lParam);

    case WM_TIMER:
        return ANIMATE_Timer(infoPtr);

    case WM_PRINTCLIENT:
    case WM_PAINT:
        {
            /* the animation has not decompressed
             * (and displayed) the first frame yet, don't paint
             */
            if (!infoPtr->hbmPrevFrame)
            {
                /* default paint handling */
                return DefWindowProcW(hWnd, uMsg, wParam, lParam);
            }

            if (wParam)
            {
                EnterCriticalSection(&infoPtr->cs);
                ANIMATE_PaintFrame(infoPtr, (HDC)wParam);
                LeaveCriticalSection(&infoPtr->cs);
            }
            else
            {
                PAINTSTRUCT ps;
                HDC hDC = BeginPaint(infoPtr->hwndSelf, &ps);

                EnterCriticalSection(&infoPtr->cs);
                ANIMATE_PaintFrame(infoPtr, hDC);
                LeaveCriticalSection(&infoPtr->cs);

                EndPaint(infoPtr->hwndSelf, &ps);
            }
        }
        break;

    case WM_SIZE:
        if (infoPtr->dwStyle & ACS_CENTER) 
	    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
	return DefWindowProcW(hWnd, uMsg, wParam, lParam);

    default:
	if ((uMsg >= WM_USER) && (uMsg < WM_APP) && !COMCTL32_IsReflectedMessage(uMsg))
	    ERR("unknown msg %#x, wp %#Ix, lp %#Ix.\n", uMsg, wParam, lParam);

	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

void ANIMATE_Register(void)
{
    WNDCLASSW wndClass;

    ZeroMemory(&wndClass, sizeof(WNDCLASSW));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS;
    wndClass.lpfnWndProc   = ANIMATE_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(ANIMATE_INFO *);
    wndClass.hCursor       = LoadCursorW(0, (LPCWSTR)IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wndClass.lpszClassName = ANIMATE_CLASSW;

    RegisterClassW(&wndClass);
}


void ANIMATE_Unregister(void)
{
    UnregisterClassW(ANIMATE_CLASSW, NULL);
}
