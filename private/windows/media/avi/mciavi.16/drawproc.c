/****************************************************************************
 *
 *  DRAWPROC.C
 *
 *  Standard AVI drawing handler.
 *
 *      InstallAVIDrawHandler()
 *
 *  Copyright (c) 1992 Microsoft Corporation.  All Rights Reserved.
 *
 *  You have a royalty-free right to use, modify, reproduce and
 *  distribute the Sample Files (and/or any modified version) in
 *  any way you find useful, provided that you agree that
 *  Microsoft has no warranty obligations or liability for any
 *  Sample Application Files which are modified.
 *
 ***************************************************************************/

#include <win32.h>
#include <vfw.h>
#include "common.h"
#include "mciavi.h"

#define FOURCC_VIDS         mmioFOURCC('v','i','d','s')
#define FOURCC_AVIDraw      mmioFOURCC('D','R','A','W')
#define VERSION_AVIDraw     0x00010000      // 1.00

#ifdef DEBUG
    HDRAWDIB ghdd;
#endif

#ifndef HUGE
    #define HUGE _huge
#endif

/***************************************************************************
 ***************************************************************************/

typedef struct {
    HDRAWDIB		hdd;

    HDC                 hdc;            // HDC to draw to
			
    int                 xDst;           // destination rectangle
    int                 yDst;
    int                 dxDst;
    int                 dyDst;
    int                 xSrc;           // source rectangle
    int                 ySrc;
    int                 dxSrc;
    int                 dySrc;
    BOOL                fBackground;

    BOOL                fRle;
    DWORD               biSizeImage;
    BOOL		fNeedUpdate;

    LONG                rate;           // playback rate (uSec / frame)
} INSTINFO, *PINSTINFO;

// static stuff in this file.
LONG FAR PASCAL _loadds ICAVIDrawProc(DWORD id, HDRVR hDriver, UINT uiMessage, LPARAM lParam1, LPARAM lParam2);

static LONG AVIDrawOpen(ICOPEN FAR * icopen);
static LONG AVIDrawClose(PINSTINFO pi);
static LONG AVIDrawGetInfo(ICINFO FAR *icinfo, LONG lSize);
static LONG AVIDrawQuery(PINSTINFO pi, LPBITMAPINFOHEADER lpbiIn);
static LONG AVIDrawSuggestFormat(PINSTINFO pi, ICDRAWSUGGEST FAR *lpicd, LONG cbicd);
static LONG AVIDrawBegin(PINSTINFO pi, ICDRAWBEGIN FAR *lpicd, LONG cbicd);
static LONG AVIDraw(PINSTINFO pi, ICDRAW FAR *lpicd, LONG cbicd);
static LONG AVIDrawEnd(PINSTINFO pi);
static LONG AVIDrawChangePalette(PINSTINFO pi, LPBITMAPINFOHEADER lpbi);

/***************************************************************************
 ***************************************************************************/

LONG FAR PASCAL _loadds ICAVIDrawProc(DWORD id, HDRVR hDriver, UINT uiMessage, LPARAM lParam1, LPARAM lParam2)
{
    INSTINFO *pi = (INSTINFO *)(UINT)id;

    switch (uiMessage)
    {
        case DRV_LOAD:
        case DRV_FREE:
            return 1;

        /*********************************************************************
            open
        *********************************************************************/

        case DRV_OPEN:
            if (lParam2 == 0L)
                return 1;

            return AVIDrawOpen((ICOPEN FAR *)lParam2);

	case DRV_CLOSE:
            return AVIDrawClose(pi);

        /*********************************************************************
            Configure/Info messages
        *********************************************************************/

        case DRV_QUERYCONFIGURE:    // configuration from drivers applet
            return 0;

        case DRV_CONFIGURE:
            return 1;

        case ICM_CONFIGURE:
        case ICM_ABOUT:
            return ICERR_UNSUPPORTED;

        /*********************************************************************
            state messages
        *********************************************************************/

        case ICM_GETSTATE:
        case ICM_SETSTATE:
            return 0L;

//      case ICM_GETINFO:
//          return AVIDrawGetInfo((ICINFO FAR *)lParam1, lParam2);

        /*********************************************************************
            decompress messages
        *********************************************************************/

        case ICM_DRAW_QUERY:
            return AVIDrawQuery(pi, (LPBITMAPINFOHEADER)lParam1);

	case ICM_DRAW_SUGGESTFORMAT:
	    return AVIDrawSuggestFormat(pi, (ICDRAWSUGGEST FAR *) lParam1, lParam2);

        case ICM_DRAW_BEGIN:
	    return AVIDrawBegin(pi, (ICDRAWBEGIN FAR *) lParam1, lParam2);

	case ICM_DRAW_REALIZE:
	    pi->hdc = (HDC) lParam1;
	
	    if (!pi->hdc || !pi->hdd)
		break;

	    pi->fBackground = (BOOL) lParam2;
	
	    return DrawDibRealize(pi->hdd, pi->hdc, pi->fBackground);

	case ICM_DRAW_GET_PALETTE:
	    if (!pi->hdd)
		break;

	    return (LONG) DrawDibGetPalette(pi->hdd);
	
        case ICM_DRAW:
            return AVIDraw(pi, (ICDRAW FAR *)lParam1, lParam2);

	case ICM_DRAW_CHANGEPALETTE:
	    return AVIDrawChangePalette(pi, (LPBITMAPINFOHEADER) lParam1);
	
        case ICM_DRAW_END:
            return AVIDrawEnd(pi);

        case ICM_DRAW_START:
            return DrawDibStart(pi->hdd, pi->rate);

        case ICM_DRAW_STOP:
            return DrawDibStop(pi->hdd);

        /*********************************************************************
            standard driver messages
        *********************************************************************/

        case DRV_DISABLE:
        case DRV_ENABLE:
            return 1;

        case DRV_INSTALL:
        case DRV_REMOVE:
            return 1;
    }

    return ICERR_UNSUPPORTED;
}

/*****************************************************************************
 *
 * AVIDrawOpen() is called from the DRV_OPEN message
 *
 ****************************************************************************/

static LONG AVIDrawOpen(ICOPEN FAR * icopen)
{
    INSTINFO *  pinst;

    if (icopen->dwFlags & ICMODE_COMPRESS)
        return 0;

    if (icopen->dwFlags & ICMODE_DECOMPRESS)
        return 0;

    pinst = (INSTINFO *)LocalAlloc(LPTR, sizeof(INSTINFO));

    if (!pinst)
    {
        icopen->dwError = ICERR_MEMORY;
        return 0;
    }

    DPF2(("*** AVIDrawOpen()\n"));

    //
    // init structure
    //
    pinst->hdd = DrawDibOpen();

#ifdef DEBUG
    ghdd = pinst->hdd;
#endif

    //
    // return success.
    //
    icopen->dwError = ICERR_OK;

    return (LONG) (UINT) pinst;
}

/*****************************************************************************
 *
 * Close() is called on the DRV_CLOSE message.
 *
 ****************************************************************************/
static LONG AVIDrawClose(PINSTINFO pi)
{
    DPF2(("*** AVIDrawClose()\n"));

    if (pi->hdd) {
	DrawDibClose(pi->hdd);
    }
    LocalFree((HLOCAL) pi);

    return 1;
}

#if 0
/*****************************************************************************
 *
 * AVIDrawGetInfo() implements the ICM_GETINFO message
 *
 ****************************************************************************/
static LONG AVIDrawGetInfo(ICINFO FAR *icinfo, LONG lSize)
{
    if (icinfo == NULL)
        return sizeof(ICINFO);

    if (lSize < sizeof(ICINFO))
        return 0;

    icinfo->dwSize	    = sizeof(ICINFO);
    icinfo->fccType         = FOURCC_VIDS;
    icinfo->fccHandler      = FOURCC_AVIDraw;
    icinfo->dwFlags	    = VIDCF_DRAW;    // supports inter-frame
    icinfo->dwVersion       = VERSION_AVIDraw;
    icinfo->dwVersionICM    = ICVERSION;
    icinfo->szName[0]       = 0;
    icinfo->szDescription[0]= 0;

    return sizeof(ICINFO);
}
#endif

/*****************************************************************************
 *
 * AVIDrawQuery() implements ICM_DRAW_QUERY
 *
 ****************************************************************************/
static LONG AVIDrawQuery(PINSTINFO pi,
			 LPBITMAPINFOHEADER lpbiIn)
{
    //
    // determine if the input DIB data is in a format we like.
    //
    if (lpbiIn == NULL)
        return ICERR_BADFORMAT;

    //
    // determine if the input DIB data is in a format we like.
    //

    // !!! Do we need a DrawDibQuery or something here to let this handle
    // any compressed format?

#ifdef DRAWDIBNODECOMPRESS
    if (lpbiIn->biCompression != BI_RGB &&
#if 0
        !(lpbiIn->biBitCount == 8 && lpbiIn->biCompression == BI_RLE8))
#else
	1)
#endif
        return ICERR_BADFORMAT;
#endif

    return ICERR_OK;
}


static LONG AVIDrawSuggestFormat(PINSTINFO pi, ICDRAWSUGGEST FAR *lpicd, LONG cbicd)
{
    HIC hic;

    if (lpicd->lpbiSuggest == NULL)
	return sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD);

    hic = ICGetDisplayFormat(NULL, lpicd->lpbiIn, lpicd->lpbiSuggest,
			     0, lpicd->dxDst, lpicd->dyDst);

    if (hic)
	ICClose(hic);


    return sizeof(BITMAPINFOHEADER) + lpicd->lpbiSuggest->biClrUsed * sizeof(RGBQUAD);
}

/*****************************************************************************
 *
 * AVIDrawBegin() implements ICM_DRAW_BEGIN
 *
 ****************************************************************************/

static LONG AVIDrawBegin(PINSTINFO pi, ICDRAWBEGIN FAR *lpicd, LONG cbicd)
{
    LONG    l;
    UINT    wFlags;

    if (lpicd->dwFlags & ICDRAW_FULLSCREEN)
        return ICERR_UNSUPPORTED;

    l = AVIDrawQuery(pi, lpicd->lpbi);

    if ((l != 0) || (lpicd->dwFlags & ICDRAW_QUERY))
	return l;

    // Copy over whatever we want to remember
    pi->hdc = lpicd->hdc;
    pi->xDst = lpicd->xDst;
    pi->yDst = lpicd->yDst;
    pi->dxDst = lpicd->dxDst;
    pi->dyDst = lpicd->dyDst;
    pi->xSrc = lpicd->xSrc;
    pi->ySrc = lpicd->ySrc;
    pi->dxSrc = lpicd->dxSrc;
    pi->dySrc = lpicd->dySrc;
    pi->rate = muldiv32(lpicd->dwScale,1000000,lpicd->dwRate);

    // !!! Should this be done somewhere else? drawdib mabey!

    if (pi->hdc)
        SetStretchBltMode(pi->hdc, COLORONCOLOR);

    wFlags = 0;

    // !!! We need some way to have a "stupid" mode here....
    if (lpicd->dwFlags & ICDRAW_BUFFER)
        wFlags |= DDF_BUFFER;

    // Don't animate if we're realizing in the background
    if (lpicd->dwFlags & ICDRAW_ANIMATE && !(pi->fBackground))
        wFlags |= DDF_ANIMATE;

    //
    //  remember if this is RLE because we may need to hack it later.
    //
    pi->fRle = lpicd->lpbi->biCompression == BI_RLE8;
    pi->biSizeImage = (DWORD)(((UINT)lpicd->lpbi->biWidth+3)&~3)*(DWORD)(UINT)lpicd->lpbi->biHeight;

    DPF2(("*** AVIDrawBegin()\n"));

    if (lpicd->hpal == (HPALETTE)MCI_AVI_SETVIDEO_PALETTE_HALFTONE) {
        DrawDibSetPalette(pi->hdd, NULL);
        wFlags |= DDF_HALFTONE;
    }
    else
        DrawDibSetPalette(pi->hdd, lpicd->hpal);

    if (!DrawDibBegin(pi->hdd, pi->hdc,
		 pi->dxDst, pi->dyDst,
		 lpicd->lpbi,
		 pi->dxSrc, pi->dySrc,
		 wFlags))
	return ICERR_UNSUPPORTED;

    if (pi->hdc)
        DrawDibRealize(pi->hdd, pi->hdc, pi->fBackground);

    return ICERR_OK;
}


/*****************************************************************************
 *
 * AVIDraw() implements ICM_DRAW
 *
 ****************************************************************************/

static LONG AVIDraw(PINSTINFO pi, ICDRAW FAR *lpicd, LONG cbicd)
{
    UINT  wFlags;
    BOOL  f;

    wFlags = DDF_SAME_DRAW|DDF_SAME_HDC;  // !!! Right flags?

    if ((lpicd->lpData == NULL) || (lpicd->cbData == 0)) {

        if ((lpicd->dwFlags & ICDRAW_UPDATE) || pi->fNeedUpdate) {
            DrawDibRealize(pi->hdd, pi->hdc, pi->fBackground);
            wFlags |= DDF_UPDATE;
	    pi->fNeedUpdate = FALSE;
        }
	else
            return ICERR_OK;  // no data to draw.
    }
    else {
        if (lpicd->dwFlags & ICDRAW_PREROLL) {
            wFlags |= DDF_DONTDRAW;
	    pi->fNeedUpdate = TRUE;
	} else if (lpicd->dwFlags & ICDRAW_HURRYUP) {
            wFlags |= DDF_HURRYUP;
	    pi->fNeedUpdate = TRUE;
	} else
	    pi->fNeedUpdate = FALSE;

        if (lpicd->dwFlags & ICDRAW_NOTKEYFRAME)
            wFlags |= DDF_NOTKEYFRAME;

        //
        // if we get a update while playing and we are drawing RLE delta's
        // make sure we update.
        //
        if (pi->fRle && (lpicd->dwFlags & ICDRAW_UPDATE)) {
            DrawDibDraw(pi->hdd, pi->hdc, pi->xDst, pi->yDst,
                0,0,NULL,NULL,0,0,0,0,DDF_UPDATE|DDF_SAME_HDC|DDF_SAME_DRAW);
        }
    }

    //
    // We need a hack here for the RLE case, to make sure that
    // DIBs are marked correctly as BI_RLE8 or BI_RGB....
    //
    if (pi->fRle) {
        if (lpicd->cbData == pi->biSizeImage)
            ((LPBITMAPINFOHEADER)lpicd->lpFormat)->biCompression = BI_RGB;
        else
            ((LPBITMAPINFOHEADER)lpicd->lpFormat)->biCompression = BI_RLE8;
    }

    f = DrawDibDraw(pi->hdd, pi->hdc,
		pi->xDst, pi->yDst,
		pi->dxDst, pi->dyDst,
		lpicd->lpFormat,
                lpicd->lpData,
		pi->xSrc, pi->ySrc,
		pi->dxSrc, pi->dySrc,
                wFlags);

    if (pi->fRle)
        ((LPBITMAPINFOHEADER)lpicd->lpFormat)->biCompression = BI_RLE8;

    if (!f) {

	if (wFlags & DDF_UPDATE)
            DPF(("DrawDibUpdate failed\n"));
        else
            DPF(("DrawDibDraw failed\n"));

	if (wFlags & DDF_UPDATE)
	    return ICERR_CANTUPDATE;
	else
            return ICERR_ERROR;
    }

    return ICERR_OK;
}

static LONG AVIDrawChangePalette(PINSTINFO pi, LPBITMAPINFOHEADER lpbi)
{
    PALETTEENTRY    ape[256];
    LPRGBQUAD	    lprgb;
    int i;

    lprgb = (LPRGBQUAD) ((LPBYTE) lpbi + lpbi->biSize);

    for (i = 0; i < (int) lpbi->biClrUsed; i++) {
	ape[i].peRed = lprgb[i].rgbRed;
	ape[i].peGreen = lprgb[i].rgbGreen;
	ape[i].peBlue = lprgb[i].rgbBlue;
	ape[i].peFlags = 0;
    }
	
    DrawDibChangePalette(pi->hdd, 0, (int) lpbi->biClrUsed,
				 (LPPALETTEENTRY)ape);

    return ICERR_OK;
}

/*****************************************************************************
 *
 * AVIDrawEnd() implements ICM_DRAW_END
 *
 ****************************************************************************/

static LONG AVIDrawEnd(PINSTINFO pi)
{
    return ICERR_OK;
}
