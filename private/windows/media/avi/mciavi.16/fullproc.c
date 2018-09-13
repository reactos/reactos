/****************************************************************************
 *
 *  DRAWPROC.C
 *
 *  Standard AVI drawing handler.
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
#include <compman.h>
#include <dispdib.h>
#ifdef WIN32
#include <mmddk.h>
#endif

static SZCODE szDisplayDibLib[] = TEXT("DISPDIB.DLL");
static SZCODEA szDisplayDibEx[]  = "DisplayDibEx";

#define FOURCC_VIDS         mmioFOURCC('v','i','d','s')
#define FOURCC_AVIFull      mmioFOURCC('F','U','L','L')
#define VERSION_AVIFull     0x00010000      // 1.00

#ifndef HUGE
    #define HUGE _huge
#endif

extern FAR PASCAL LockCurrentTask(BOOL);

static int siUsage = 0;

HINSTANCE	ghDISPDIB = NULL; // handle to DISPDIB.DLL module
UINT (FAR PASCAL *DisplayDibExProc)(LPBITMAPINFOHEADER lpbi, int x, int y, BYTE HUGE * hpBits, UINT wFlags);

/***************************************************************************
 ***************************************************************************/

typedef struct {
    int                 xDst;           // destination rectangle
    int                 yDst;
    int                 dxDst;
    int                 dyDst;
    int                 xSrc;           // source rectangle
    int                 ySrc;
    int                 dxSrc;
    int                 dySrc;
    HWND		hwnd;
    HWND                hwndOldFocus;
    BOOL                fRle;
    DWORD               biSizeImage;
} INSTINFO, *PINSTINFO;

// static stuff in this file.
LONG FAR PASCAL _loadds ICAVIFullProc(DWORD id, HDRVR hDriver, UINT uiMessage, LPARAM lParam1, LPARAM lParam2);
static LONG AVIFullOpen(ICOPEN FAR * icopen);
static LONG AVIFullClose(PINSTINFO pi);
static LONG AVIFullGetInfo(ICINFO FAR *icinfo, LONG lSize);
static LONG AVIFullQuery(PINSTINFO pi, LPBITMAPINFOHEADER lpbiIn);
static LONG AVIFullSuggestFormat(PINSTINFO pi, ICDRAWSUGGEST FAR *lpicd, LONG cbicd);
static LONG AVIFullBegin(PINSTINFO pi, ICDRAWBEGIN FAR *lpicd, LONG cbicd);
static LONG AVIFullDraw(PINSTINFO pi, ICDRAW FAR *lpicd, LONG cbicd);
static LONG AVIFullEnd(PINSTINFO pi);

/***************************************************************************
 ***************************************************************************/

LONG FAR PASCAL _loadds ICAVIFullProc(DWORD id, HDRVR hDriver, UINT uiMessage, LPARAM lParam1, LPARAM lParam2)
{
    INSTINFO *pi = (INSTINFO *)(UINT)id;

    switch (uiMessage)
    {
        case DRV_LOAD:
	    return 1;
	
        case DRV_FREE:
            return 1;

        /*********************************************************************
            open
        *********************************************************************/

        case DRV_OPEN:
	    if (ghDISPDIB == NULL) {
		UINT w;

		w = SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);

		if ((ghDISPDIB = LoadLibrary(szDisplayDibLib)) > HINSTANCE_ERROR) {
		    (FARPROC)DisplayDibExProc = GetProcAddress(ghDISPDIB, szDisplayDibEx);
		}
		else
		    ghDISPDIB = (HINSTANCE)-1;

		SetErrorMode(w);
	    }

	    if (DisplayDibExProc == NULL)
		return 0;
	
            if (lParam2 == 0L)
                return 1;

            return AVIFullOpen((ICOPEN FAR *)lParam2);

	case DRV_CLOSE:
	    if (id == 1)
		return 1;

            return AVIFullClose(pi);

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

#if 0
        case ICM_GETINFO:
            return AVIFullGetInfo((ICINFO FAR *)lParam1, lParam2);
#endif

        /*********************************************************************
            decompress messages
        *********************************************************************/

        case ICM_DRAW_QUERY:
            return AVIFullQuery(pi, (LPBITMAPINFOHEADER)lParam1);

	case ICM_DRAW_SUGGESTFORMAT:
	    return AVIFullSuggestFormat(pi, (ICDRAWSUGGEST FAR *) lParam1, lParam2);

        case ICM_DRAW_BEGIN:
	    return AVIFullBegin(pi, (ICDRAWBEGIN FAR *) lParam1, lParam2);

        case ICM_DRAW:
            return AVIFullDraw(pi, (ICDRAW FAR *)lParam1, lParam2);

	case ICM_DRAW_CHANGEPALETTE:
	    DisplayDibExProc((LPBITMAPINFOHEADER) lParam1, 0, 0, NULL,
			DISPLAYDIB_NOWAIT | DISPLAYDIB_NOIMAGE);

	    return ICERR_OK;

        case ICM_DRAW_END:
            return AVIFullEnd(pi);

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

    if (uiMessage < DRV_USER)
        return DefDriverProc(id,hDriver,uiMessage,lParam1,lParam2);
    else
        return ICERR_UNSUPPORTED;
}

/*****************************************************************************
 *
 * AVIFullOpen() is called from the DRV_OPEN message
 *
 ****************************************************************************/

static LONG AVIFullOpen(ICOPEN FAR * icopen)
{
    INSTINFO *  pinst;

    //
    // refuse to open if we are not being opened as a Video compressor
    //
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

    ++siUsage;

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
static LONG AVIFullClose(PINSTINFO pi)
{
    LocalFree((HLOCAL) pi);

    if (--siUsage == 0) {
	/* unload DISPDIB library (if loaded) */
	if (ghDISPDIB != NULL && ghDISPDIB != (HINSTANCE) -1)
	    FreeLibrary(ghDISPDIB), ghDISPDIB = NULL;
    }

    return 1;
}

#if 0
/*****************************************************************************
 *
 * AVIFullGetInfo() implements the ICM_GETINFO message
 *
 ****************************************************************************/
static LONG AVIFullGetInfo(ICINFO FAR *icinfo, LONG lSize)
{
    if (icinfo == NULL)
        return sizeof(ICINFO);

    if (lSize < sizeof(ICINFO))
        return 0;

    icinfo->dwSize	    = sizeof(ICINFO);
    icinfo->fccType         = FOURCC_VIDS;
    icinfo->fccHandler      = FOURCC_AVIFull;
    icinfo->dwFlags	    = VIDCF_DRAW;
    icinfo->dwVersion       = VERSION_AVIFull;
    icinfo->dwVersionICM    = ICVERSION;
    lstrcpy(icinfo->szDescription, szDescription);
    lstrcpy(icinfo->szName, szName);

    return sizeof(ICINFO);
}
#endif

/*****************************************************************************
 *
 * AVIFullQuery() implements ICM_DRAW_QUERY
 *
 ****************************************************************************/
static LONG AVIFullQuery(PINSTINFO pi,
			 LPBITMAPINFOHEADER lpbiIn)
{
    //
    // determine if the input DIB data is in a format we like.
    //
    if (lpbiIn == NULL)
        return ICERR_BADFORMAT;

    if (DisplayDibExProc(lpbiIn, 0, 0, 0,
                DISPLAYDIB_MODE_DEFAULT|DISPLAYDIB_NOWAIT|DISPLAYDIB_TEST) != 0)
	return ICERR_BADFORMAT;

    return ICERR_OK;
}


static LONG AVIFullSuggestFormat(PINSTINFO pi, ICDRAWSUGGEST FAR *lpicd, LONG cbicd)
{
    HIC hic;
    static int iFull = -1;
    int	iDepth;

    if (iFull < 0) {
	BITMAPINFOHEADER bih;

	bih.biSize = sizeof(bih);
	bih.biBitCount = 16;
	bih.biCompression = BI_RGB;
	bih.biWidth = 160;
	bih.biHeight = 120;

	iFull = (AVIFullQuery(pi, &bih) == ICERR_OK) ? 1 : 0;
    }

    iDepth = lpicd->lpbiIn->biBitCount > 8 && iFull == 1 ? 16 : 8;

    if (lpicd->lpbiSuggest == NULL)
	return sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD);

    hic = ICGetDisplayFormat(NULL, lpicd->lpbiIn,
			     lpicd->lpbiSuggest,
			     iDepth, 0, 0);

    if (hic)
	ICClose(hic);


    return sizeof(BITMAPINFOHEADER) + lpicd->lpbiSuggest->biClrUsed * sizeof(RGBQUAD);
}

/*****************************************************************************
 *
 * AVIFullBegin() implements ICM_DRAW_BEGIN
 *
 ****************************************************************************/

static LONG AVIFullBegin(PINSTINFO pi, ICDRAWBEGIN FAR *lpicd, LONG cbicd)
{
    UINT	w;
    LONG	lRet;

    if (!(lpicd->dwFlags & ICDRAW_FULLSCREEN))
	return ICERR_UNSUPPORTED; // !!! Necessary?

    lRet = AVIFullQuery(pi, lpicd->lpbi);
    if (lRet != 0 || (lpicd->dwFlags & ICDRAW_QUERY))
	return lRet;

    // Copy over whatever we want to remember
    pi->hwnd = lpicd->hwnd;
    pi->xDst = lpicd->xDst;
    pi->yDst = lpicd->yDst;
    pi->dxDst = lpicd->dxDst;
    pi->dyDst = lpicd->dyDst;
    pi->xSrc = lpicd->xSrc;
    pi->ySrc = lpicd->ySrc;
    pi->dxSrc = lpicd->dxSrc;
    pi->dySrc = lpicd->dySrc;

    //
    //  remember if this is RLE because we may need to hack it later.
    //
    pi->fRle = lpicd->lpbi->biCompression == BI_RLE8;
    pi->biSizeImage = (DWORD)(((UINT)lpicd->lpbi->biWidth+3)&~3)*(DWORD)(UINT)lpicd->lpbi->biHeight;

    pi->hwndOldFocus = GetFocus();
    SetFocus(NULL);

    //
    // we dont need to do this, DISPDIB will do it for us
    //
#if 0
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    LockCurrentTask(TRUE);
#endif

    /* Capture the mouse, so other apps don't get called. */
    SetCapture(pi->hwnd);

    /* We don't explicitly specify a graphics mode; DispDib will
    ** choose one for us.
    */
    w = DisplayDibExProc(lpicd->lpbi, 0, 0, NULL,
		DISPLAYDIB_BEGIN | DISPLAYDIB_NOWAIT);

    switch (w) {
	case DISPLAYDIB_INVALIDFORMAT:
	    return ICERR_BADFORMAT;
	
	case 0:
	    return ICERR_OK;

	default:
	    return ICERR_UNSUPPORTED;
    }
}


/*****************************************************************************
 *
 * AVIFullDraw() implements ICM_DRAW
 *
 ****************************************************************************/

static LONG AVIFullDraw(PINSTINFO pi, ICDRAW FAR *lpicd, LONG cbicd)
{
    UINT    wFlags;
    UINT    w;

    wFlags = DISPLAYDIB_NOPALETTE | DISPLAYDIB_NOWAIT;

    if (pi->dxDst > pi->dxSrc)
	wFlags |= DISPLAYDIB_ZOOM2;

    if (lpicd->dwFlags & ICDRAW_NULLFRAME) {
	return ICERR_OK;  // !!!
    }

    if (lpicd->dwFlags & ICDRAW_PREROLL) {
	if (((LPBITMAPINFOHEADER)lpicd->lpFormat)->biCompression == BI_RGB) // !!!
	    return ICERR_OK;
    }

    if (lpicd->dwFlags & ICDRAW_HURRYUP)
	; // !!! DONTDRAW?

    if (lpicd->lpData == NULL)
        return ICERR_UNSUPPORTED;

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

    w = DisplayDibExProc(lpicd->lpFormat, 0, 0, lpicd->lpData, wFlags);

    if (pi->fRle)
        ((LPBITMAPINFOHEADER)lpicd->lpFormat)->biCompression = BI_RLE8;

    if (w != DISPLAYDIB_NOERROR)
	return ICERR_ERROR;

    return ICERR_OK;
}

/*****************************************************************************
 *
 * AVIFullEnd() implements ICM_DRAW_END
 *
 ****************************************************************************/

static LONG AVIFullEnd(PINSTINFO pi)
{
    MSG  msg;
	
    DisplayDibExProc(NULL, 0, 0, NULL, DISPLAYDIB_END | DISPLAYDIB_NOWAIT);

    //
    // we dont need to do this, DISPDIB will do it for us
    //
#if 0
    LockCurrentTask(FALSE);

    /* Can we assume the error mode should be 0? */
    SetErrorMode(0);
#endif

    ReleaseCapture();

    /* Clear out left-over key messages */
    while (PeekMessage(&msg, NULL, WM_KEYFIRST, WM_KEYLAST,
			PM_REMOVE | PM_NOYIELD))
	;
    /* Clear out left-over mouse messages */
    while (PeekMessage(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST,
			PM_REMOVE | PM_NOYIELD))
	;
    SetFocus(pi->hwndOldFocus);

    return ICERR_OK;
}

