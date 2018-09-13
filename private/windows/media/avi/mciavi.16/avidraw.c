/******************************************************************************

   Copyright (C) Microsoft Corporation 1991-1992. All rights reserved.

   Title:   avidraw.c - Functions that actually draw video for AVI.

*****************************************************************************/
#include "graphic.h"

//
// if the average key frame spacing is greater than this value, always
// force a buffer.
//
#define KEYFRAME_PANIC_SPACE       2500

#define YIELDATFUNNYTIMES

#define ALIGNULONG(i)     ((i+3)&(~3))                  /* ULONG aligned ! */
#define WIDTHBYTES(i)     ((unsigned)((i+31)&(~31))/8)  /* ULONG aligned ! */
#define DIBWIDTHBYTES(bi) (DWORD)WIDTHBYTES((int)(bi).biWidth * (int)(bi).biBitCount)

#ifdef WIN32
#define LockCurrentTask(x)	(x)
#else
extern FAR PASCAL LockCurrentTask(BOOL);
#endif

BOOL NEAR PASCAL DrawBits(NPMCIGRAPHIC npMCI, DWORD ckid, DWORD cksize, BOOL fHurryUp);
void NEAR PASCAL UpdateDisplayDibPalette(NPMCIGRAPHIC npMCI);

BOOL NEAR PASCAL ProcessPaletteChange(NPMCIGRAPHIC npMCI, DWORD cksize)
{
    UINT wStartIndex;
    UINT wNumEntries;
    UINT w;
    LPPALETTEENTRY ppe;

    npMCI->dwFlags |= MCIAVI_PALCHANGED;
    DPF2(("Setting PALCHANGED\n"));

    while (cksize > 4) {
	wStartIndex = GET_BYTE();
	wNumEntries = GET_BYTE();

	/* Skip filler word */
	GET_WORD();

	/* Zero is used as a shorthand for 256 */
	if (wNumEntries == 0)
            wNumEntries = 256;

        ppe = (LPVOID)npMCI->lp;

        for (w=0; w<wNumEntries; w++)
        {
            npMCI->argb[wStartIndex+w].rgbRed   = ppe[w].peRed;
            npMCI->argb[wStartIndex+w].rgbGreen = ppe[w].peGreen;
            npMCI->argb[wStartIndex+w].rgbBlue  = ppe[w].peBlue;
        }

	SKIP_BYTES(wNumEntries * sizeof(PALETTEENTRY));
	cksize -= 4 + wNumEntries * sizeof(PALETTEENTRY);
    }

    if (npMCI->pbiFormat->biBitCount == 8) {
	hmemcpy((LPBYTE) npMCI->pbiFormat + npMCI->pbiFormat->biSize,
		(LPBYTE) npMCI->argb,
		sizeof(RGBQUAD) * npMCI->pbiFormat->biClrUsed);
    }

#ifdef DEBUG	
    /* Make sure we've used up the entire chunk... */
    if (cksize != 0) {
	DPF(("MCIAVI: Problem with palc chunk\n"));
    }
#endif

    return TRUE;
}

/* Display the video from the current record.
*/
BOOL NEAR PASCAL DisplayVideoFrame(NPMCIGRAPHIC npMCI, BOOL fHurryUp)
{
    DWORD	ckid;
    DWORD	cksize;
    BOOL	fRet;
    int		stream;
    DWORD	dwRet;
    LONG        len;
    DWORD	dwDrawStart;
    LPVOID      lpSave;
    LPVOID      lpChunk;

    /* If we're allowed to skip frames, apply some relatively
    ** bogus heuristics to decide if we should do it, and
    ** pass the appropriate flag on to the driver.
    */
    if ((npMCI->lCurrentFrame & 0x0f) == 0) {
	fHurryUp = FALSE;
    }

    /* Even if SKIPFRAMES is off, count how many frames we _would_ have
    ** skipped if we could.
    */
    if (fHurryUp)
	++npMCI->dwSkippedFrames;

    if (!(npMCI->dwOptionFlags & MCIAVIO_SKIPFRAMES))
	fHurryUp = FALSE;

    /* Keep track of what we've drawn. */
    npMCI->lFrameDrawn = npMCI->lCurrentFrame;
    len = (LONG)npMCI->dwThisRecordSize;
    lpSave = npMCI->lp;

    /* If it's interleaved, adjust for the next record header.... */
    // !!! Only if not last frame?
    if (npMCI->wPlaybackAlg == MCIAVI_ALG_INTERLEAVED)
	len -= 3 * sizeof(DWORD);

    while (len >= 2 * sizeof(DWORD)) {

	/* Look at the next chunk */
	ckid = GET_DWORD();
	cksize = GET_DWORD();

	DPF3(("'%.4s': %lu bytes\n", (LPSTR) &ckid, cksize));
	
	if ((LONG) cksize > len) {
            AssertSz(FALSE, "Chunk obviously too big!");
	    break;
	}
	
        len -= ((cksize+1)&~1) + 8;

        if (len < -1) {
            AssertSz(FALSE, "Chunk overflowed what was read in!");
	    break;
	}

	lpChunk = npMCI->lp;
	
        stream = StreamFromFOURCC(ckid);

        if (stream == npMCI->nVideoStream) {

            if ((npMCI->lCurrentFrame < npMCI->lVideoStart) &&
                    !(npMCI->dwFlags & MCIAVI_REVERSE))
                goto skip;

            switch(TWOCCFromFOURCC(ckid)) {

            case cktypePALchange:
                ProcessPaletteChange(npMCI, cksize);
                npMCI->lLastPaletteChange = npMCI->lCurrentFrame;
                break;

            default:
                /* Some other chunk... */
                if (!fHurryUp && ckid)
                    dwDrawStart = timeGetTime();

                //!!! we need to handle half frames!!!

                fRet = DrawBits(npMCI, ckid, cksize, fHurryUp);

                if (!fRet)
                    return FALSE;

                if (!fHurryUp && ckid)
                    npMCI->dwLastDrawTime = timeGetTime() - dwDrawStart;

                if (npMCI->dwBufferedVideo)
                    npMCI->dwLastDrawTime = 0;

                break;
            }
        } else if (stream >= 0 && stream < npMCI->streams &&
                        SI(stream)->hicDraw) {
            dwRet = ICDraw(SI(stream)->hicDraw, (fHurryUp ? ICDRAW_HURRYUP : 0L),
                                SI(stream)->lpFormat,
                                (ckid == 0) ? 0L : npMCI->lp, cksize, npMCI->lCurrentFrame);
            // !!! Error check?
        }
skip:
	/* If not interleaved, we're done. */
	if (npMCI->wPlaybackAlg != MCIAVI_ALG_INTERLEAVED)
	    return TRUE;

        /* Skip to the next chunk */
        npMCI->lp = (HPSTR) lpChunk + ((cksize+1)&~1);
    }

    npMCI->lp = lpSave;

    return TRUE;
}

//
// mark all streams in the passed RECT as dirty
//
void NEAR PASCAL StreamInvalidate(NPMCIGRAPHIC npMCI, LPRECT prc)
{
    int i;
    int n;
    STREAMINFO *psi;
    RECT rc;

    if (prc)
        DPF2(("StreamInvalidate: [%d, %d, %d, %d]\n", *prc));
    else
        DPF2(("StreamInvalidate: NULL\n", *prc));

    for (n=i=0; i<npMCI->streams; i++) {

        psi = SI(i);

        // we always update any visible error streams

        if (!(psi->dwFlags & STREAM_ERROR) &&
            !(psi->dwFlags & STREAM_ENABLED))
            continue;

        if (IsRectEmpty(&psi->rcDest))
            continue;

        if (prc && !IntersectRect(&rc, prc, &psi->rcDest))
            continue;

        n++;
        psi->dwFlags |= STREAM_NEEDUPDATE;
    }

    //
    // !!!is this right? or should we always dirty the movie?
    //
    if (n > 0)
        npMCI->dwFlags |= MCIAVI_NEEDUPDATE;
    else
        npMCI->dwFlags &= ~MCIAVI_NEEDUPDATE;
}

//
//  update all dirty streams
//
//  if fPaint is set paint the area even if the stream handler does not
//
BOOL NEAR PASCAL DoStreamUpdate(NPMCIGRAPHIC npMCI, BOOL fPaint)
{
    int i;
    BOOL f=TRUE;
    STREAMINFO *psi;

    Assert(npMCI->hdc);
    SaveDC(npMCI->hdc);

    for (i=0; i<npMCI->streams; i++) {

        psi = SI(i);

        //
        // this stream is clean, dont paint it.
        //
        if (!(psi->dwFlags & (STREAM_DIRTY|STREAM_NEEDUPDATE))) {

            ExcludeClipRect(npMCI->hdc,
                DEST(i).left,DEST(i).top,DEST(i).right,DEST(i).bottom);

            continue;
        }

        psi->dwFlags &= ~STREAM_NEEDUPDATE;
        psi->dwFlags &= ~STREAM_DIRTY;

        if (psi->dwFlags & STREAM_ERROR) {
            UINT u;
            TCHAR ach[80];
            TCHAR szMessage[80];
            HBRUSH hbr = CreateHatchBrush(HS_BDIAGONAL, RGB(128,0,0));

            if (psi->sh.fccType == streamtypeVIDEO)
                 LoadString(ghModule, MCIAVI_CANT_DRAW_VIDEO, ach, sizeof(ach));
            else
                 LoadString(ghModule, MCIAVI_CANT_DRAW_STREAM, ach, sizeof(ach));

            FillRect(npMCI->hdc, &DEST(i), hbr);
            u = SetBkMode(npMCI->hdc, TRANSPARENT);
            wsprintf(szMessage, ach,
                (LPVOID)&psi->sh.fccType,
                (LPVOID)&psi->sh.fccHandler);
            DrawText(npMCI->hdc, szMessage, lstrlen(szMessage), &DEST(i),
                 DT_WORDBREAK|DT_VCENTER|DT_CENTER);
            SetBkMode(npMCI->hdc, u);
            DeleteObject(hbr);

            FrameRect(npMCI->hdc, &DEST(i), GetStockObject(BLACK_BRUSH));
        }

        else if (!(psi->dwFlags & STREAM_ENABLED)) {
            FillRect(npMCI->hdc, &DEST(i), GetStockObject(DKGRAY_BRUSH));
        }

        else if (psi->sh.fccType == streamtypeVIDEO &&
            !(npMCI->dwFlags & MCIAVI_SHOWVIDEO)) {

            continue;   // we will paint black here.
        }

        else if (npMCI->nVideoStreams > 0 && i == npMCI->nVideoStream) {

            if (!DrawBits(npMCI, 0L, 0L, FALSE)) {

                psi->dwFlags |= STREAM_NEEDUPDATE;
                f = FALSE;

                if (fPaint)         // will paint back if told to.
                    continue;
            }
        }
        else if (psi->hicDraw == NULL) {
            FillRect(npMCI->hdc, &DEST(i), GetStockObject(DKGRAY_BRUSH));
        }
        else if (ICDraw(psi->hicDraw,ICDRAW_UPDATE,psi->lpFormat,NULL,0,0) != 0) {

            psi->dwFlags |= STREAM_NEEDUPDATE;
            f = FALSE;

            // should other streams work like this?

            if (fPaint)             // will paint back if told to.
                continue;
        }

        //
        //  we painted so clean this area
        //
        ExcludeClipRect(npMCI->hdc,
            DEST(i).left,DEST(i).top,DEST(i).right,DEST(i).bottom);
    }

    // now paint black every where else

    FillRect(npMCI->hdc,&npMCI->rcDest,GetStockObject(BLACK_BRUSH));
    RestoreDC(npMCI->hdc, -1);

    //
    // do we still still need a update?
    //
    if (f) {
        npMCI->dwFlags &= ~MCIAVI_NEEDUPDATE;
    }
    else {
        DPF2(("StreamUpdate: update failed\n"));
        npMCI->dwFlags |= MCIAVI_NEEDUPDATE;
    }

    return f;
}

void NEAR PASCAL AlignPlaybackWindow(NPMCIGRAPHIC npMCI)
{
#ifndef WIN32
    DWORD dw;
    int x,y;
    HWND hwnd;      // the window we will move.
    RECT rc;

    // if (npMCI->hicDraw != npMCI->hicDrawInternal)
    //	    return;  !!! only align if using the default draw guy?

#pragma message("**** move this into the draw handler and/or DrawDib")
#pragma message("**** we need to query the alignment from the codec????")
    #define X_ALIGN 4
    #define Y_ALIGN 4

    // the MCIAVI_RELEASEDC flags means the DC came from a GetDC(npMCI->hwnd)

    if (!(npMCI->dwFlags & MCIAVI_RELEASEDC))
        return;

    //
    // dont align if the dest rect is not at 0,0
    //
    if (npMCI->rcMovie.left != 0 || npMCI->rcMovie.top != 0)
        return;

    dw = GetDCOrg(npMCI->hdc);

    x = LOWORD(dw) + npMCI->rcMovie.left;
    y = HIWORD(dw) + npMCI->rcMovie.top;

    if ((x & (X_ALIGN-1)) || (y & (Y_ALIGN-1)))
    {
        DPF2(("*** warning movie is not aligned! (%d,%d)***\n",x,y));

        //
        // find the first moveable window walking up the tree.
        //
        for (hwnd = npMCI->hwnd; hwnd; hwnd = GetParent(hwnd))
        {
            LONG l = GetWindowLong(hwnd, GWL_STYLE);
	
            // this window is toplevel stop
            if (!(l & WS_CHILD))
                break;

            // this window is sizeable (should be movable too)
            if (l & WS_THICKFRAME)
                break;

            // this window has a caption (is moveable)
            if ((l & WS_CAPTION) == WS_CAPTION)
                break;
	}
	
        //
        // dont move the window if it does not want to be moved.
        //
        if (IsWindowVisible(hwnd) &&
           !IsZoomed(hwnd) &&
           !IsIconic(hwnd) &&
            IsWindowEnabled(hwnd))
        {
            GetClientRect(hwnd, &rc);
            ClientToScreen(hwnd, (LPPOINT)&rc);

            //
            // if the movie is not in the upper corner of the window
            // don't align
            //
            if (x < rc.left || x-rc.left > 16 ||
                y < rc.top  || y-rc.top > 16)
                return;

            GetWindowRect(hwnd, &rc);
            OffsetRect(&rc, -(x & (X_ALIGN-1)), -(y & (Y_ALIGN-1)));

            if (GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD)
                ScreenToClient(GetParent(hwnd), (LPPOINT)&rc);

            // dont move window off of the screen.

            if (rc.left < 0 || rc.top < 0)
                return;

            DPF2(("*** moving window to [%d,%d,%d,%d]\n",rc));

            SetWindowPos(hwnd,NULL,rc.left,rc.top,0,0,
                SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
#ifdef DEBUG
            dw = GetDCOrg(npMCI->hdc);
            x = LOWORD(dw) + npMCI->rcMovie.left;
            y = HIWORD(dw) + npMCI->rcMovie.top;

            Assert(!(x & (X_ALIGN-1)) && !(y & (Y_ALIGN-1)));
#endif
	    }
	}
#endif
}
	
UINT NEAR PASCAL PrepareDC(NPMCIGRAPHIC npMCI)
{
    UINT u;
    int i;
    STREAMINFO *psi;


    DPF2(("*** PrepareDC(%04X)\n",npMCI->hdc));
    Assert(npMCI->hdc != NULL);

    if (!(npMCI->dwFlags & MCIAVI_FULLSCREEN) &&
        !(npMCI->dwFlags & MCIAVI_SEEKING) &&
        !(npMCI->dwFlags & MCIAVI_UPDATING) &&
         (npMCI->dwFlags & MCIAVI_SHOWVIDEO) ) {
        AlignPlaybackWindow(npMCI);
    }

    if (npMCI->hicDraw) {
	DPF2(("Calling ICDrawRealize\n"));
        u = (UINT)ICDrawRealize(npMCI->hicDraw, npMCI->hdc, npMCI->fForceBackground);
    } else {
        u = 0;
    }

    //
    //  realize the other strems, but force them into the background.
    //
    for (i=0; i<npMCI->streams; i++) {
        psi = SI(i);

        if (!(psi->dwFlags & STREAM_ENABLED))
            continue;

        if (psi->dwFlags & STREAM_ERROR)
            continue;

        if (psi == npMCI->psiVideo)
            continue;

        if (psi->hicDraw == NULL)
            continue;

        if (psi->hicDraw == npMCI->hicDraw)
            continue;

        ICDrawRealize(psi->hicDraw, npMCI->hdc, TRUE);
    }

    //
    // return "master" stream realize value.
    //
    return u;
}

void NEAR PASCAL UnprepareDC(NPMCIGRAPHIC npMCI)
{
    Assert(npMCI->hdc);
    DPF2(("*** UnprepareDC(%04X)\n",npMCI->hdc));
    SelectPalette(npMCI->hdc, GetStockObject(DEFAULT_PALETTE), FALSE);
    RealizePalette(npMCI->hdc);
    //RestoreDC(npMCI->hdc, -1);
}

/* This function is called to actually handle drawing.
**
** ckid and cksize specify the type and size of the data to be drawn;
** it's located at npMCI->lp.
**
** If the fHurryUp flag is set, that means that we're behind and we
** shouldn't draw now. all we do it update the current buffered image
** and return...
*/

BOOL NEAR PASCAL DrawBits(NPMCIGRAPHIC npMCI, DWORD ckid, DWORD cksize, BOOL fHurryUp)
{
    LPVOID	lp = npMCI->lp;
    LPBITMAPINFOHEADER	lpFormat = npMCI->pbiFormat;
    DWORD       dwRet;
    DWORD       dwFlags;
    STREAMINFO *psi;

    if (!npMCI->pbiFormat)
	return TRUE;

    if (npMCI->fNoDrawing || !(npMCI->dwFlags & MCIAVI_SHOWVIDEO))
        return TRUE;

    psi = SI(npMCI->nVideoStream);

    //
    //  let's compute the flags we need to pass to ICDecompress() and
    //  to ICDraw()
    //
    //      ICDRAW_HURRYUP      - we are behind
    //      ICDRAW_PREROLL      - we are seeking (before a play)
    //      ICDRAW_UPDATE       - update of frame (repaint, ...)
    //      ICDRAW_NOTKEYFRAME  - this frame data is not a key.
    //

    dwFlags = 0;

    if (psi->dwFlags & STREAM_NEEDUPDATE)
        dwFlags |= ICDRAW_UPDATE;

    if (cksize == 0)
        dwFlags |= ICDRAW_NULLFRAME;

    if (ckid == 0) {
        dwFlags |= ICDRAW_UPDATE;
        lp = 0;
    }
    else if (fHurryUp) {
        dwFlags |= ICDRAW_HURRYUP;
        psi->dwFlags |= STREAM_DIRTY;
    }
    else if (!(npMCI->dwFlags & MCIAVI_REVERSE) &&
             (npMCI->lCurrentFrame < npMCI->lRealStart)) {
        dwFlags |= ICDRAW_PREROLL;
        psi->dwFlags |= STREAM_DIRTY;
    }

    if (npMCI->hpFrameIndex) {
        if ((ckid == 0L || cksize == 0) ||
                    FramePrevKey(npMCI->lCurrentFrame) != npMCI->lCurrentFrame)
            dwFlags |= ICDRAW_NOTKEYFRAME;
    }

    //
    //  now draw the frame, decompress first if needed.
    //
    if (npMCI->hic) {

        if (ckid != 0L && cksize != 0) {

            TIMESTART(timeDecompress);

	    npMCI->pbiFormat->biSizeImage = cksize; // !!! Is this safe?

	    dwRet = ICDecompress(npMCI->hic,
		    dwFlags,
		    npMCI->pbiFormat,
		    npMCI->lp,
		    &npMCI->bih,
		    npMCI->hpDecompress);

	    TIMEEND(timeDecompress);

	    if (dwRet == ICERR_DONTDRAW) {
		return TRUE; // !!!???
            }

	    // ICERR_NEWPALETTE?

	    dwFlags &= (~ICDRAW_NOTKEYFRAME);	// It's a key frame now....
        }

        if (dwFlags & (ICDRAW_HURRYUP|ICDRAW_PREROLL))
            return TRUE;

	lpFormat = &npMCI->bih;
	lp = npMCI->hpDecompress;
	cksize = npMCI->bih.biSizeImage;
    }

    TIMESTART(timeDraw);

    if ((npMCI->dwFlags & MCIAVI_PALCHANGED) &&
        !(dwFlags & (ICDRAW_HURRYUP|ICDRAW_PREROLL))) {

        if (psi->ps) {
            if (npMCI->hic) {
                //!!! should be psi->lpFormat *not* npMCI->pbiFormat
                ICDecompressGetPalette(npMCI->hic, npMCI->pbiFormat, &npMCI->bih);
                ICDrawChangePalette(npMCI->hicDraw, &npMCI->bih);
            }
            else {
                ICDrawChangePalette(npMCI->hicDraw, npMCI->pbiFormat);
            }
        }
        else {
            DPF2(("Calling ICDrawChangePalette\n"));
            ICDrawChangePalette(npMCI->hicDraw, &npMCI->bih);
        }

        npMCI->dwFlags &= ~(MCIAVI_PALCHANGED);

        dwFlags &= ~ICDRAW_HURRYUP; // should realy draw this!
    }

    if ((npMCI->dwFlags & MCIAVI_SEEKING) &&
        !(dwFlags & ICDRAW_PREROLL))
        PrepareDC(npMCI);

    lpFormat->biSizeImage = cksize; // !!! ??? Is this safe?

    //
    // !!!do we realy realy want to do this here?
    // or just relay on the MPlay(er) status function
    //
////if (npMCI->dwFlags & MCIAVI_WANTMOVE)
////    CheckWindowMoveFast(npMCI);

    DPF3(("Calling ICDraw on frame %ld  (%08lx)\n", npMCI->lCurrentFrame, dwFlags));

    dwRet = ICDraw(npMCI->hicDraw, dwFlags, lpFormat, lp, cksize,
		   npMCI->lCurrentFrame - npMCI->lFramePlayStart);

    TIMEEND(timeDraw);

    if ((LONG) dwRet < ICERR_OK) {
	DPF(("Driver failed ICM_DRAW message err=%ld\n", dwRet));
	return FALSE;
    }
    else {

        psi->dwFlags &= ~STREAM_NEEDUPDATE;

        if (!(dwFlags & (ICDRAW_HURRYUP|ICDRAW_PREROLL)))
            psi->dwFlags &= ~STREAM_DIRTY;
    }

    return TRUE;
}

/***************************************************************************
 ***************************************************************************/

static void FreeDecompressBuffer(NPMCIGRAPHIC npMCI)
{
    if (npMCI->hpDecompress)
	GlobalFreePtr(npMCI->hpDecompress);

    npMCI->hpDecompress = NULL;
    npMCI->cbDecompress = 0;
}

/***************************************************************************
 ***************************************************************************/

static BOOL GetDecompressBuffer(NPMCIGRAPHIC npMCI)
{
    int n = npMCI->nVideoStream;
    int dxDest = RCW(DEST(n));
    int dyDest = RCH(DEST(n));
    HPSTR   hp;

    npMCI->bih.biSizeImage = npMCI->bih.biHeight * DIBWIDTHBYTES(npMCI->bih);

    if ((LONG) npMCI->bih.biSizeImage <= npMCI->cbDecompress)
	return TRUE;

    if (!npMCI->hpDecompress)
	hp = GlobalAllocPtr(GHND|GMEM_SHARE, npMCI->bih.biSizeImage);
    else
	hp = GlobalReAllocPtr(npMCI->hpDecompress,
			      npMCI->bih.biSizeImage,
			      GMEM_MOVEABLE | GMEM_SHARE);

    if (hp == NULL) {
        npMCI->dwTaskError = MCIERR_OUT_OF_MEMORY;
        return FALSE;
    }

    npMCI->hpDecompress = hp;
    npMCI->cbDecompress = npMCI->bih.biSizeImage;

    return TRUE;
}



/*
Possibilities:

1. We're starting to play.
We may need to switch into fullscreen mode.
We need a DrawBegin.


2. We're updating the screen.
Do we send a new DrawBegin?
Has anything changed since we last updated?  Perhaps we can use
a flag to say whether something has changed, and set it when we leave
fullscreen mode or when the window is stretched.

What if we're updating to memory?

3. We're playing, and the user has stretched the window.
The Draw device may need us to go back to a key frame.
If we have a separate decompressor, it may need us to go back to a key frame.


*/

#if 0
RestartCompressor()
{
    DWORD dwDrawFlags;

    dwDrawFlags = (npMCI->dwFlags & MCIAVI_FULLSCREEN) ?
				    ICDRAW_FULLSCREEN : ICDRAW_HDC;

    if (pfRestart)
	dwDrawFlags |= ICDRAW_CONTINUE;

    if (npMCI->dwFlags & MCIAVI_UPDATETOMEMORY)
	dwDrawFlags |= ICDRAW_MEMORYDC;


    if (npMCI->hic) {
	static struct  {
	    BITMAPINFOHEADER bi;
	    RGBQUAD          rgbq[256];
	}   dib;


    }
}
#endif


BOOL TryDrawDevice(NPMCIGRAPHIC npMCI, HIC hicDraw, DWORD dwDrawFlags, BOOL fTryDecompress)
{
    DWORD   dw;
    int     n = npMCI->nVideoStream;
    STREAMINFO *psi = SI(n);

    Assert(psi);

    if (hicDraw == NULL)
        return FALSE;

    // See if the standard draw device can handle the format
    dw = ICDrawBegin(hicDraw,
        dwDrawFlags,

        npMCI->hpal,           // palette to draw with
        npMCI->hwnd,           // window to draw to
        npMCI->hdc,            // HDC to draw to

	RCX(DEST(n)),
	RCY(DEST(n)),
	RCW(DEST(n)),
	RCH(DEST(n)),

        npMCI->pbiFormat,

	RCX(SOURCE(n)),
	RCY(SOURCE(n)),
	RCW(SOURCE(n)),
	RCH(SOURCE(n)),

	// !!! First of all, these two are backwards.
	// !!! Secondly, what if PlayuSec == 0?
	npMCI->dwPlayMicroSecPerFrame,
	1000000L);

    if (dw == ICERR_OK) {
	npMCI->hic = 0;
	npMCI->hicDraw = hicDraw;

	return TRUE;
    }

    if (npMCI->hicDecompress && fTryDecompress) {
	RECT	rc;

	// Ask the draw device to suggest a format, then try to get our
	// decompressor to make that format.
	dw = ICDrawSuggestFormat(hicDraw,
				 npMCI->pbiFormat,
				 &npMCI->bih,
				 RCW(SOURCE(n)),
				 RCH(SOURCE(n)),
				 RCW(DEST(n)),
				 RCH(DEST(n)),
				 npMCI->hicDecompress);

        if ((LONG)dw >= 0)
            dw = ICDecompressQuery(npMCI->hicDecompress,
                    npMCI->pbiFormat,&npMCI->bih);

        if ((LONG)dw < 0) {
            //
            //  default to the right format for the screen, in case the draw guy
            //  fails the draw suggest.
            //
            ICGetDisplayFormat(npMCI->hicDecompress,
                    npMCI->pbiFormat,&npMCI->bih, 0,
                    MulDiv((int)npMCI->pbiFormat->biWidth, RCW(psi->rcDest),RCW(psi->rcSource)),
                    MulDiv((int)npMCI->pbiFormat->biHeight,RCH(psi->rcDest),RCH(psi->rcSource)));

            dw = ICDecompressQuery(npMCI->hicDecompress,
                    npMCI->pbiFormat,&npMCI->bih);

	    if (dw != ICERR_OK) {
		npMCI->dwTaskError = MCIERR_INTERNAL;
		return FALSE;
	    }
        }

        if (npMCI->bih.biBitCount <= 8) {
            ICDecompressGetPalette(npMCI->hicDecompress,
                        npMCI->pbiFormat, &npMCI->bih);
        }
	
#ifdef DEBUG
	DPF(("InitDecompress: Decompressing %dx%dx%d '%4.4ls' to %dx%dx%d\n",
	    (int)npMCI->pbiFormat->biWidth,
	    (int)npMCI->pbiFormat->biHeight,
	    (int)npMCI->pbiFormat->biBitCount,
	    (LPSTR)(
	    npMCI->pbiFormat->biCompression == BI_RGB ? "None" :
	    npMCI->pbiFormat->biCompression == BI_RLE8 ? "Rle8" :
	    npMCI->pbiFormat->biCompression == BI_RLE4 ? "Rle4" :
	    (LPSTR)&npMCI->pbiFormat->biCompression),
	    (int)npMCI->bih.biWidth,
	    (int)npMCI->bih.biHeight,
	    (int)npMCI->bih.biBitCount));
#endif

	if (!GetDecompressBuffer(npMCI))
	    return FALSE;

	//
	// setup the "real" source rect we will draw with.
	//
#if 0
	rc.left = (int) ((SOURCE(n).left * npMCI->bih.biWidth) / npMCI->pbiFormat->biWidth);
	rc.right = (int) ((SOURCE(n).right * npMCI->bih.biWidth) / npMCI->pbiFormat->biWidth);
	rc.top = (int) ((SOURCE(n).top * npMCI->bih.biHeight) / npMCI->pbiFormat->biHeight);
	rc.bottom = (int) ((SOURCE(n).bottom * npMCI->bih.biHeight) / npMCI->pbiFormat->biHeight);
#else
	rc = SOURCE(n);
	rc.left = (int) ((rc.left * npMCI->bih.biWidth) / npMCI->pbiFormat->biWidth);
	rc.right = (int) ((rc.right * npMCI->bih.biWidth) / npMCI->pbiFormat->biWidth);
	rc.top = (int) ((rc.top * npMCI->bih.biHeight) / npMCI->pbiFormat->biHeight);
	rc.bottom = (int) ((rc.bottom * npMCI->bih.biHeight) / npMCI->pbiFormat->biHeight);
#endif
	dw = ICDrawBegin(hicDraw,
	    dwDrawFlags,
	    npMCI->hpal,           // palette to draw with
	    npMCI->hwnd,           // window to draw to
	    npMCI->hdc,            // HDC to draw to
	    RCX(DEST(n)),
	    RCY(DEST(n)),
	    RCW(DEST(n)),
	    RCH(DEST(n)),
	    &npMCI->bih,

	    rc.left, rc.top,
	    rc.right  - rc.left,
	    rc.bottom - rc.top,

	    // !!! First of all, these two are backwards.
	    // !!! Secondly, what if PlayuSec == 0?
	    npMCI->dwPlayMicroSecPerFrame,
	    1000000L);

	if (dw == ICERR_OK) {
	    npMCI->hic = npMCI->hicDecompress;
	    npMCI->hicDraw = hicDraw;
	    
	    // Now, we have the format we'd like the decompressor to decompress to...
            dw = ICDecompressBegin(npMCI->hicDecompress,
				   npMCI->pbiFormat,
				   &npMCI->bih);

	    if (dw != ICERR_OK) {
		DPF(("DrawBegin: decompressor succeeded query, failed begin!\n"));
		ICDrawEnd(npMCI->hicDraw);

		return FALSE;
	    }
	    return TRUE;
	}

	if (npMCI->dwFlags & MCIAVI_FULLSCREEN) {
	    npMCI->dwTaskError = MCIERR_AVI_NODISPDIB;
	}
    }

    return FALSE;
}

BOOL FindDrawDevice(NPMCIGRAPHIC npMCI, DWORD dwDrawFlags)
{
    if (npMCI->dwFlags & MCIAVI_USERDRAWPROC) {
	// If the user has set a draw procedure, try it.
	if (TryDrawDevice(npMCI, npMCI->hicDrawDefault, dwDrawFlags, TRUE)) {
	    if (npMCI->hic) {
                DPF2(("Using decompressor, then application's draw device...\n"));
	    } else {
                DPF2(("Using application's draw device...\n"));
            }
	    return TRUE;
        }

	// If it fails, it fails.
	DPF(("Can't use application's draw device!\n"));
	return FALSE;
    }

    // First, try a pure draw device we've found.
    if (TryDrawDevice(npMCI, SI(npMCI->nVideoStream)->hicDraw, dwDrawFlags, FALSE)) {
        DPF2(("Draw device is drawing to the screen...\n"));
	return TRUE;
    }

    // Next, try see if the decompressor we found can draw too.
    // Should this even get asked before the guy above?!!!!
    if (TryDrawDevice(npMCI, npMCI->hicDecompress, dwDrawFlags, FALSE)) {
        DPF2(("Decompressor is drawing to the screen...\n"));
	return TRUE;
    }

    // No?  Then, get the standard draw device, for fullscreen or not.
    if (npMCI->dwFlags & MCIAVI_FULLSCREEN) {
	// !!! If it's fullscreen, should we force a re-begin?
	// !!! Assume fullscreen only happens when play is starting?

	if (npMCI->hicDrawFull == NULL) {
            DPF2(("Opening default fullscreen codec...\n"));
            npMCI->hicDrawFull = ICOpen(streamtypeVIDEO,
                FOURCC_AVIFull,ICMODE_DRAW);

	    if (!npMCI->hicDrawFull)
		npMCI->hicDrawFull = (HIC) -1;
	}

	npMCI->hicDraw = npMCI->hicDrawFull;
    } else {
	if (npMCI->hicDrawDefault == NULL) {
            DPF2(("Opening default draw codec...\n"));
            npMCI->hicDrawDefault = ICOpen(streamtypeVIDEO,
                FOURCC_AVIDraw,ICMODE_DRAW);

	if (!npMCI->hicDrawDefault)
	    npMCI->hicDrawDefault = (HIC) -1;
        }

	npMCI->hicDraw = npMCI->hicDrawDefault;
    }

    // If there's an installed draw device, try it.
    if (npMCI->hicDraw && npMCI->hicDraw != (HIC) -1) {
	if (TryDrawDevice(npMCI, npMCI->hicDraw, dwDrawFlags, TRUE)) {
	    if (npMCI->hic) {
                DPF2(("Using decompressor, then default draw device...\n"));
	    } else {
                DPF2(("Using default draw device...\n"));
	    }
            return TRUE;
	}
    }

    if (npMCI->dwFlags & MCIAVI_FULLSCREEN) {
	if (!npMCI->hicInternalFull)
	    npMCI->hicInternalFull = ICOpenFunction(streamtypeVIDEO,
		FOURCC_AVIFull,ICMODE_DRAW,(FARPROC)ICAVIFullProc);

	npMCI->hicDraw = npMCI->hicInternalFull;
    } else {
	if (!npMCI->hicInternal) {
	    npMCI->hicInternal = ICOpenFunction(streamtypeVIDEO,
		FOURCC_AVIDraw,ICMODE_DRAW,(FARPROC)ICAVIDrawProc);
#ifdef DEBUG
	    {
		extern HANDLE ghdd;
		npMCI->hdd = ghdd;
		ghdd = NULL;
	    }
#endif
	}

	npMCI->hicDraw = npMCI->hicInternal;
    }

    // As a last resort, try the built-in draw device.
    if (TryDrawDevice(npMCI, npMCI->hicDraw, dwDrawFlags, TRUE)) {
	if (npMCI->hic) {
            DPF2(("Using decompressor, then built-in draw device...\n"));
	} else {
            DPF2(("Using built-in draw device...\n"));
	}
	return TRUE;
    }

    return FALSE;
}

/**************************************************************************
* @doc  INTERNAL DRAWDIB
*
* @api BOOL | DibEq | This function compares two dibs.
*
* @parm LPBITMAPINFOHEADER lpbi1 | Pointer to one bitmap.
*       this DIB is assumed to have the colors after the BITMAPINFOHEADER
*
* @parm LPBITMAPINFOHEADER | lpbi2 | Pointer to second bitmap.
*       this DIB is assumed to have the colors after biSize bytes.
*
* @rdesc Returns TRUE if bitmaps are identical, FALSE otherwise.
*
**************************************************************************/
BOOL DibEq(LPBITMAPINFOHEADER lpbi1, LPBITMAPINFOHEADER lpbi2)
{
    return
        lpbi1->biCompression == lpbi2->biCompression   &&
        lpbi1->biSize        == lpbi2->biSize          &&
        lpbi1->biWidth       == lpbi2->biWidth         &&
        lpbi1->biHeight      == lpbi2->biHeight        &&
        lpbi1->biBitCount    == lpbi2->biBitCount;
}


/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api void | DrawBegin
 *
 *
 ***************************************************************************/
BOOL FAR PASCAL DrawBegin(NPMCIGRAPHIC npMCI, BOOL FAR *pfRestart)
{
    DWORD	dwDrawFlags;
    HIC		hicLast = npMCI->hic;
    HIC         hicLastDraw = npMCI->hicDraw;
    BITMAPINFOHEADER	bihDecompLast = npMCI->bih;

    if (npMCI->nVideoStreams == 0)
	return TRUE;

    if (!npMCI->pbiFormat)
        return TRUE;

    // if fullscreen, make sure we re-initialize....
    if (npMCI->dwFlags & MCIAVI_FULLSCREEN) {
	npMCI->dwFlags |= MCIAVI_NEEDDRAWBEGIN;
    }

    npMCI->fNoDrawing = FALSE;

    dwDrawFlags = (npMCI->dwFlags & MCIAVI_FULLSCREEN) ?
				    ICDRAW_FULLSCREEN : ICDRAW_HDC;

    if (pfRestart) {
	dwDrawFlags |= ICDRAW_CONTINUE;
	*pfRestart = TRUE;
    }

    if (npMCI->dwFlags & MCIAVI_UPDATETOMEMORY)
	dwDrawFlags |= ICDRAW_MEMORYDC;

    // !!! What about "stupid mode"?

    //
    // if the file has no keyframes force a buffer
    //
    if (npMCI->dwKeyFrameInfo == 0)
        dwDrawFlags |= ICDRAW_BUFFER;

    //
    // if the file has few keyframes also force a buffer.
    //
    if (MovieToTime(npMCI->dwKeyFrameInfo) > KEYFRAME_PANIC_SPACE)
        dwDrawFlags |= ICDRAW_BUFFER;

    if (dwDrawFlags & ICDRAW_BUFFER)
        DPF(("Forcing a decompress buffer because too few key frames\n"));

    if (npMCI->wTaskState > TASKIDLE &&
	    !(npMCI->dwFlags & MCIAVI_SEEKING) &&
	    !(npMCI->dwFlags & MCIAVI_FULLSCREEN) &&
	    (npMCI->dwFlags & MCIAVI_ANIMATEPALETTE)) {
	dwDrawFlags |= ICDRAW_ANIMATE;
#if 0
//
// I moved all this into ShowStage() where you could claim it realy belongs.
//
        if (npMCI->hwnd == npMCI->hwndDefault &&
	    !(GetWindowLong(npMCI->hwnd, GWL_STYLE) & WS_CHILD))
            SetActiveWindow(npMCI->hwnd);
#endif
    }

    if (npMCI->hdc == NULL) {
        DPF2(("DrawBegin() with NULL hdc!\n"));
    }

    if (FindDrawDevice(npMCI, dwDrawFlags)) {
	if (npMCI->hicDraw != hicLastDraw || (npMCI->hic != hicLast) ||
	    (npMCI->hic && !DibEq(&npMCI->bih, &bihDecompLast))) {
	    // !!! This obviously shouldn't always be invalidated!
            //
	    // make sure the, current image buffer is invalidated
            //
            DPF2(("Draw device is different; restarting....\n"));
            npMCI->lFrameDrawn = (- (LONG) npMCI->wEarlyRecords) - 1;

            npMCI->dwFlags |= MCIAVI_WANTMOVE;

	    if (pfRestart)
		*pfRestart = TRUE;
        }

        if (npMCI->dwFlags & MCIAVI_WANTMOVE)
            CheckWindowMove(npMCI, TRUE);

//	if (pfRestart)
//           *pfRestart = (dw == ICERR_GOTOKEYFRAME);

	npMCI->dwFlags &= ~(MCIAVI_NEEDDRAWBEGIN);

#if 0
	//
	// tell the compressor some interesting info.
	//

	if (npMCI->hicDraw) { // !!! Does npMCI->hic need to know this?
	    ICSendMessage(npMCI->hic, ICM_SET, ICM_FRAMERATE, npMCI->dwPlayMicroSecPerFrame);
	    ICSendMessage(npMCI->hic, ICM_SET, ICM_KEYFRAMERATE, npMCI->dwKeyFrameInfo);
	}
#endif

	return TRUE;
    }

    return FALSE;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api void | DrawEnd
 *
 * @parm NPMCIGRAPHIC | npMCI | pointer to instance data block.
 *
 ***************************************************************************/
void NEAR PASCAL DrawEnd(NPMCIGRAPHIC npMCI)
{
    if (!npMCI->pbiFormat)
	return;

    ICDrawEnd(npMCI->hicDraw);

    // if we were fullscreen, we now need to repaint and things....
    if (npMCI->dwFlags & MCIAVI_FULLSCREEN) {
	npMCI->dwFlags |= MCIAVI_NEEDDRAWBEGIN;
    }

    /*
    ** let DrawDib clean up if we're animating the palette.
    */
    if (npMCI->wTaskState > TASKIDLE &&
        !(npMCI->dwFlags & MCIAVI_SEEKING) &&
        !(npMCI->dwFlags & MCIAVI_FULLSCREEN) &&
        !(npMCI->dwFlags & MCIAVI_UPDATING) &&
         (npMCI->dwFlags & MCIAVI_ANIMATEPALETTE)) {
	npMCI->dwFlags |= MCIAVI_NEEDDRAWBEGIN;
	InvalidateRect(npMCI->hwnd, NULL, FALSE);
    }
}
