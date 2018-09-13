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

#ifdef _WIN32
#include "graphic.h"
#include <mmddk.h>
#include "profile.h"
#endif
#include <win32.h>
#include <vfw.h>
#include <dispdib.h>

#ifdef _WIN32
static SZCODE szAtomFlag[] = TEXT("aviFullscreen");

static SZCODE szDisplayDibLib[] = TEXT("DISPDB32.DLL");
#else
static SZCODE szDisplayDibLib[] = TEXT("DISPDIB.DLL");
#endif
static SZCODEA szDisplayDibEx[]  = "DisplayDibEx";

#define FOURCC_VIDS         mmioFOURCC('v','i','d','s')
#define FOURCC_AVIFull      mmioFOURCC('F','U','L','L')
#define VERSION_AVIFull     0x00010000      // 1.00

#ifndef HUGE
    #define HUGE _huge
#endif

extern FAR PASCAL LockCurrentTask(BOOL);

static int siUsage = 0;

static HINSTANCE	ghDISPDIB = NULL; // handle to DISPDIB.DLL module
UINT (FAR PASCAL *DisplayDibExProc)(LPBITMAPINFOHEADER lpbi, int x, int y, LPSTR hpBits, UINT wFlags)=NULL;

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
LRESULT FAR PASCAL _loadds ICAVIFullProc(DWORD_PTR id, HDRVR hDriver, UINT uiMessage, LPARAM lParam1, LPARAM lParam2);
static LRESULT AVIFullOpen(ICOPEN FAR * icopen);
static LONG AVIFullClose(PINSTINFO pi);
static LONG AVIFullGetInfo(ICINFO FAR *icinfo, LONG lSize);
static LONG AVIFullQuery(PINSTINFO pi, LPBITMAPINFOHEADER lpbiIn);
static LONG AVIFullSuggestFormat(PINSTINFO pi, ICDRAWSUGGEST FAR *lpicd, LONG cbicd);
static LONG AVIFullBegin(PINSTINFO pi, ICDRAWBEGIN FAR *lpicd, LONG cbicd);
static LONG AVIFullDraw(PINSTINFO pi, ICDRAW FAR *lpicd, LONG cbicd);
static LONG AVIFullEnd(PINSTINFO pi);


/* -------------------------------------------------------------------------
** Private Globals
** These are only valid in the process that started playing the movie.
** -------------------------------------------------------------------------
*/
#include "common.h"

HWND        hwndFullScreen;
HDC         hdcFullScreen;
HDRAWDIB    hdd;
BOOL        fClassRegistered;
int         dxScreen;
int         dyScreen;
int         iMovieSizeMultiplier;


/***************************************************************************
 ***************************************************************************/

LRESULT FAR PASCAL _loadds ICAVIFullProc(DWORD_PTR id, HDRVR hDriver, UINT uiMessage, LPARAM lParam1, LPARAM lParam2)
{
    INSTINFO *pi = (INSTINFO *)id;

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

		w = SetErrorMode(SEM_NOOPENFILEERRORBOX);

		if ((INT_PTR)(ghDISPDIB = (HINSTANCE)LoadLibrary(szDisplayDibLib)) > HINSTANCE_ERROR) {
		    (FARPROC)DisplayDibExProc = GetProcAddress(ghDISPDIB, szDisplayDibEx);
		}
		else
		    ghDISPDIB = (HINSTANCE)-1;

		SetErrorMode(w);
	    }

	    if (DisplayDibExProc == NULL)
		DisplayDibExProc = DisplayDibEx;
	
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
	    return AVIFullSuggestFormat(pi, (ICDRAWSUGGEST FAR *) lParam1, (LONG) lParam2);

        case ICM_DRAW_BEGIN:
	    return AVIFullBegin(pi, (ICDRAWBEGIN FAR *) lParam1, (LONG) lParam2);

        case ICM_DRAW_REALIZE:
            if (DisplayDibExProc == DisplayDibEx) {

                if (hdd == NULL || hdcFullScreen == NULL) {
                    break;
                }

                return DrawDibRealize( hdd, hdcFullScreen, (BOOL)lParam2 );
            }
            break;

	case ICM_DRAW_GET_PALETTE:
            if (DisplayDibExProc == DisplayDibEx) {

	        if (NULL != hdd) {
	            return (LONG_PTR)DrawDibGetPalette(hdd);
                }
            }
            break;


        case ICM_DRAW:
            return AVIFullDraw(pi, (ICDRAW FAR *)lParam1, (LONG) lParam2);

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

static LONG_PTR AVIFullOpen(ICOPEN FAR * icopen)
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

    return (LONG_PTR) (UINT_PTR) pinst;
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
    UINT        wFlags = DISPLAYDIB_BEGIN | DISPLAYDIB_NOWAIT;

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

    if (pi->dxDst > pi->dxSrc)
	wFlags |= DISPLAYDIB_ZOOM2;

    //
    //  remember if this is RLE because we may need to hack it later.
    //
    pi->fRle = lpicd->lpbi->biCompression == BI_RLE8;
    pi->biSizeImage = (DWORD)(((UINT)lpicd->lpbi->biWidth+3)&~3)*(DWORD)(UINT)lpicd->lpbi->biHeight;

    pi->hwndOldFocus = GetFocus();
    SetFocus(NULL);

    /*
    ** If we are using the built in fullscreen support we have to
    ** get the hdd and set its palette here.  This is because I am unable to
    ** pass this information to DispDib code (there arn't any free parameters).
    */
    if (DisplayDibExProc == DisplayDibEx) {

        hdd = DrawDibOpen();

        if (lpicd->hpal == (HPALETTE)MCI_AVI_SETVIDEO_PALETTE_HALFTONE) {
            DrawDibSetPalette(hdd, NULL);
        }
        else {
            DrawDibSetPalette(hdd, lpicd->hpal);
        }
    }


    // Don't animate if we're realizing in the background
    if (lpicd->dwFlags & ICDRAW_ANIMATE) {
        wFlags |= DISPLAYDIB_ANIMATE;
    }

    if (lpicd->hpal == (HPALETTE)MCI_AVI_SETVIDEO_PALETTE_HALFTONE) {
        wFlags |= DISPLAYDIB_HALFTONE;
    }

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
    w = DisplayDibExProc(lpicd->lpbi, 0, 0, NULL, wFlags );

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

STATICFN LONG AVIFullDraw(PINSTINFO pi, ICDRAW FAR *lpicd, LONG cbicd)
{
    UINT    wFlags;
    UINT    w;

    wFlags = DISPLAYDIB_NOPALETTE | DISPLAYDIB_NOWAIT;

    if (pi->dxDst > pi->dxSrc) {
	wFlags |= DISPLAYDIB_ZOOM2;
    }

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
	else {
            ((LPBITMAPINFOHEADER)lpicd->lpFormat)->biCompression = BI_RLE8;
	    // We MUST set the correct size
	    ((LPBITMAPINFOHEADER)lpicd->lpFormat)->biSizeImage = lpicd->cbData;
	}
    }

    w = DisplayDibExProc(lpicd->lpFormat, 0, 0, lpicd->lpData, wFlags);

    if (pi->fRle)
        ((LPBITMAPINFOHEADER)lpicd->lpFormat)->biCompression = BI_RLE8;

    switch (w) {
	case DISPLAYDIB_STOP: 		return ICERR_STOPDRAWING;
	case DISPLAYDIB_NOERROR: 	return ICERR_OK;
	default: 			return ICERR_ERROR;
    }
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




/* -------------------------------------------------------------------------
** Private constants
** -------------------------------------------------------------------------
*/
#define CX_MAX_MOVIE_DEFAULT  640
#define CY_MAX_MOVIE_DEFAULT  480

/* -------------------------------------------------------------------------
** Private functions prototypes
** -------------------------------------------------------------------------
*/
LRESULT CALLBACK
FullScreenWndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

LRESULT CALLBACK
KeyboardHookProc(
    int nCode,
    WPARAM wParam,
    LPARAM lParam
    );

UINT
DisplayDibEnter(
    LPBITMAPINFOHEADER lpbi,
    UINT wFlags
    );

void
DisplayDibLeave(
    UINT wFlags
    );

int
DisplayCalcMovieMultiplier(
    int cxOriginal,
    int cyOriginal,
    DWORD dwCompression
    );







/* -------------------------------------------------------------------------
** Global data shared between all processes that attach to this library.
** This is required to make the keyboard hook work correctly.
** -------------------------------------------------------------------------
*/
//#define StopRequested()  (fStop)
#define   StopRequested()  (GlobalFindAtom(szAtomFlag))

#pragma data_seg( ".sdata" , "DATA")
BOOL    fStop;
HHOOK   hHookK;
#pragma data_seg()





/******************************Public*Routine******************************\
* @doc EXTERNAL DISPDIB
*
* @api UINT | DisplayDibEx | This function displays a 256-color bitmap on a
*    standard VGA display. It reduces the display resolution to 320-by-200
*    or 320-by-240 and uses the full screen to display the bitmap, clipping
*    and centering it as necessary. The function normally does not return to
*    the application until the user presses a key or clicks a mouse button.
*
*    To call <f DisplayDibEx>, an application must be the active
*    application. All inactive applications and GDI screen updates
*    are suspended while <f DisplayDib> temporarily reconfigures
*    the display.
*
* @parm LPBITMAPINFO | lpbi | Specifies a pointer to a <t BITMAPINFO>
*    header describing the bitmap to be displayed.
*
* @parm int | x | x position to place DIB iff DISPLAYDIB_NOCENTER flags is set
*      the lower left is (0,0)
*
* @parm int | y | y position to place DIB iff DISPLAYDIB_NOCENTER flags is set
*      the lower left is (0,0)
*
* @parm LPSTR | lpBits | Specifies a pointer to the bitmap bits. If this
*     parameter is NULL, the bits are assumed to follow the
*     <t BITMAPINFO> structure pointed to by <p lpbi>.
*
* @parm UINT | wFlags | Specifies options for displaying the bitmap. Use
*  the following flags:
*
* @flag  DISPLAYDIB_MODE_DEFAULT | Use the default mode (320 by 240)
*    to display the bitmap.
* @flag  DISPLAYDIB_MODE_320x200x8 | Use 320-by-200 mode to display
*    the bitmap.
* @flag  DISPLAYDIB_MODE_320x240x8 | Use 320-by-240 mode to display
*    the bitmap. This is the default.
* @flag  DISPLAYDIB_NOWAIT | Return immediately after displaying the
*  bitmap; don't wait for a key press or mouse click before returning.
* @flag  DISPLAYDIB_NOPALETTE      | Ignore the palette associated
*    with the bitmap. You can use this flag when displaying a series
*    of bitmaps that use a common palette.
* @flag  DISPLAYDIB_NOCENTER       | Don't center the image. The function
*  displays the bitmap in the lower-left corner of the display.
* @flag  DISPLAYDIB_NOIMAGE        | Don't draw image
* @flag  DISPLAYDIB_ZOOM2          | Stretch image by 2
* @flag  DISPLAYDIB_DONTLOCKTASK   | dont lock out other tasks
* @flag  DISPLAYDIB_TEST           | dont do any thing just test for support
* @flag  DISPLAYDIB_BEGIN          | Switch to the low-resolution
*    display mode and set the palette. The bitmap is not displayed.
*
*    If you are displaying a series of images that use the same palette,
*    you can call <f DisplayDib> with this flag to prepare the display for
*    the bitmaps, then make a series of <f DisplayDib> calls with the
*    DISPLAYDIB_NOPALETTE flag. This technique
*    eliminates the screen flicker that occurs when the display is
*    switched between the low-resolution and standard VGA modes.
*    To return the display to standard VGA mode, subsequently
*    call <f DisplayDib> with the DISPLAYDIB_END flag.
*
* @flag  DISPLAYDIB_END            | Switch back to standard VGA mode
*    and return without displaying a bitmap. Signifies the end of multiple
*    calls to <f DisplayDib>. With this flag, you can specify
*    NULL for the <p lpbi> and <p lpBits> parameters.
*
* @rdesc Returns zero if successful, otherwise returns an error code.
*  Error codes are as follows:
*
* @flag  DISPLAYDIB_NOTSUPPORTED   | <f DisplayDib> is not supported
*   in the current mode.
* @flag  DISPLAYDIB_INVALIDDIB     | The bitmap specified by
*   <p lpbi> is not a valid bitmap.
* @flag  DISPLAYDIB_INVALIDFORMAT  | The bitmap specified by
*   <p lpbi> specifes a type of bitmap that is not supported.
* @flag  DISPLAYDIB_INVALIDTASK    | The caller is an inactive application.
*   <f DisplayDib> can only be called by an active application.
*
* @comm The <f DisplayDib> function displays bitmaps described with
*    the Windows 3.0 <t BITMAPINFO> data structure in either BI_RGB
*    or BI_RLE8 format; it does not support bitmaps described with
*    the OS/2 <t BITMAPCOREHEADER> data structure.
*
*    When <f DisplayDib> switches to a low-resolution display, it
*    disables the current display driver. As a result, you cannot use GDI
*    functions to update the display while <f DisplayDib> is displaying a
*    bitmap.
*
*
* History:
* 23-03-94 - StephenE - Created
*
\**************************************************************************/
UINT FAR PASCAL
DisplayDibEx(
    LPBITMAPINFOHEADER lpbi,
    int x,
    int y,
    LPSTR lpBits,
    UINT wFlags
    )
{
    DWORD       wNumColors;
    LONG        yExt;
    LONG        xExt;
    int         xScreen,yScreen;

    /*
    ** If not already done so:
    **      Register our class and Create our window "fullscreen"
    */
    if (wFlags & DISPLAYDIB_BEGIN) {

        DPF4(( "DISPLAYDIB_BEGIN..." ));

        return DisplayDibEnter( lpbi, wFlags );
    }

    /*
    ** Just testing return OK
    */
    else if (wFlags & DISPLAYDIB_TEST) {

        DPF1(( "lpbi->biCompression = 0x%X = %c%c%c%c",
                lpbi->biCompression,
                *((LPSTR)&lpbi->biCompression + 0),
                *((LPSTR)&lpbi->biCompression + 1),
                *((LPSTR)&lpbi->biCompression + 2),
                *((LPSTR)&lpbi->biCompression + 3) ));

        DPF4(( "DISPLAYDIB_TEST... returning OK" ));
        return DISPLAYDIB_NOERROR;
    }

    /*
    ** Palette change message
    */
    else if ( (wFlags & (DISPLAYDIB_NOWAIT | DISPLAYDIB_NOIMAGE)) ==
              (DISPLAYDIB_NOWAIT | DISPLAYDIB_NOIMAGE) ) {

        PALETTEENTRY    ape[256];
        LPRGBQUAD       lprgb;
        int             i;

        lprgb = (LPRGBQUAD) ((LPBYTE) lpbi + lpbi->biSize);

        for (i = 0; i < (int) lpbi->biClrUsed; i++) {
            ape[i].peRed = lprgb[i].rgbRed;
            ape[i].peGreen = lprgb[i].rgbGreen;
            ape[i].peBlue = lprgb[i].rgbBlue;
            ape[i].peFlags = 0;
        }

        DrawDibChangePalette(hdd, 0, (int)lpbi->biClrUsed, (LPPALETTEENTRY)ape);

        return DISPLAYDIB_NOERROR;
    }

    /*
    ** Time to kill the window and the class
    */
    else if (wFlags & DISPLAYDIB_END) {

        DPF4(( "DISPLAYDIB_END..." ));
        DisplayDibLeave( wFlags );
        return DISPLAYDIB_NOERROR;
    }

    /*
    ** Do the drawing here !!
    */
    else if ( !StopRequested() ) {

        /*
        ** If we were'nt asked to draw anything just return.
        */
        if ( wFlags & DISPLAYDIB_NOIMAGE ) {
            return DISPLAYDIB_NOERROR;
        }

        xExt = lpbi->biWidth;
        yExt = lpbi->biHeight;

        if ( wFlags & DISPLAYDIB_ZOOM2 ) {

            xExt <<= 1;
            yExt <<= 1;
        }
        else if ( iMovieSizeMultiplier ) {
			//The movie needs to be stretched to full screen.
			xExt = GetSystemMetrics( SM_CXSCREEN );
            yExt = GetSystemMetrics( SM_CYSCREEN );
        }

        wNumColors  = lpbi->biClrUsed;
        if (wNumColors == 0 && lpbi->biBitCount <= 8) {
            wNumColors = 1 << (UINT)lpbi->biBitCount;
        }

        /*
        ** setup pointers
        */
        if (lpBits == NULL) {
            lpBits = (LPBYTE)lpbi + lpbi->biSize + wNumColors * sizeof(RGBQUAD);
        }

        /*
        **  center the image
        */
        if (!(wFlags & DISPLAYDIB_NOCENTER)) {

            xScreen = ((int)dxScreen - xExt) / 2;
            yScreen = ((int)dyScreen - yExt) / 2;
        }
        else {

            xScreen = 0;
            yScreen = 0;
        }

        DPF4(( "Drawing to the screen..." ));
        DrawDibDraw( hdd, hdcFullScreen,
                     xScreen, yScreen, xExt, yExt,
                     lpbi, lpBits,
                     0, 0, lpbi->biWidth, lpbi->biHeight,
                     DDF_SAME_HDC | DDF_SAME_DRAW );


        /*
        ** Hack time !!
        **
        ** We have to remove keyboard message from the queue to enable the
        ** keyboard hook to see them !!
        */
        {
            MSG msg;

            PeekMessage( &msg, NULL, WM_KEYFIRST, WM_KEYLAST,
                         PM_REMOVE | PM_NOYIELD );
        }

        return DISPLAYDIB_NOERROR;
        // return fStop;
    }

    /*
    ** The user pressed a key... time to stop
    */
    else {

        DPF1(( "The keyboard hook is telling us to stop..." ));
        //DisplayDibLeave( wFlags );
        return DISPLAYDIB_STOP;
    }

}



/*****************************Private*Routine******************************\
* DisplayDibEnter
*
*
*
* History:
* 23-03-94 - StephenE - Created
*
\**************************************************************************/
UINT
DisplayDibEnter(
    LPBITMAPINFOHEADER lpbi,
    UINT wFlags
    )
{
    WNDCLASS    wc;
    HINSTANCE   hInst = GetModuleHandle( NULL );


    /*
    ** If our class isn't already registered with windows register it
    */
    fClassRegistered = GetClassInfo( hInst, TEXT("SJE_FULLSCREEN"), &wc );
    if ( fClassRegistered == FALSE ) {

        ZeroMemory( &wc, sizeof(wc) );

        wc.style         = CS_OWNDC;
        wc.lpfnWndProc   = FullScreenWndProc;
        wc.hInstance     = hInst;
        wc.hbrBackground = (HBRUSH)GetStockObject( BLACK_BRUSH );
        wc.lpszClassName = TEXT("SJE_FULLSCREEN");
        fClassRegistered = RegisterClass( &wc );
        DPF4(( "Class registered... %s", fClassRegistered ? "OK" : "FAILED" ));
    }


    if ( fClassRegistered ) {

        /*
        ** Do we already have a window ??
        */
        if ( hwndFullScreen == NULL ) {

            hwndFullScreen = CreateWindowEx( WS_EX_TOPMOST,
                                            TEXT("SJE_FULLSCREEN"),
                                            NULL,
                                            WS_POPUP,
                                            0, 0, 0, 0,
                                            NULL, NULL,
                                            hInst, NULL );

            DPF4(( "Window created... %s", hwndFullScreen ? "OK" : "FAILED" ));
        }

        if ( hwndFullScreen ) {

            LONG    yExt;
            LONG    xExt;

            fStop = FALSE;
            hHookK = SetWindowsHookEx( WH_KEYBOARD, KeyboardHookProc,
                                       ghModule,
                                       0 );
            DPF4(( "Hook created... %s", hHookK ? "OK" : "FAILED" ));

            dxScreen = GetSystemMetrics( SM_CXSCREEN );
            dyScreen = GetSystemMetrics( SM_CYSCREEN );

            hdcFullScreen = GetDC( hwndFullScreen );
            SetStretchBltMode(hdcFullScreen, COLORONCOLOR);

            xExt = lpbi->biWidth;
            yExt = lpbi->biHeight;

            iMovieSizeMultiplier =
                DisplayCalcMovieMultiplier( xExt, yExt, lpbi->biCompression );

            if ( wFlags & DISPLAYDIB_ZOOM2 ) {

                xExt <<= 1;
                yExt <<= 1;
            }
            else if ( iMovieSizeMultiplier ) {
				//The movie needs to be stretched to full screen.
                xExt = GetSystemMetrics( SM_CXSCREEN );
                yExt = GetSystemMetrics( SM_CYSCREEN );
            }

            if ( wFlags & DISPLAYDIB_ANIMATE ) {
                wFlags = DDF_ANIMATE;
            }
            else if ( wFlags & DISPLAYDIB_HALFTONE ) {
                wFlags = DDF_HALFTONE;
            }
            else {
                wFlags = 0;
            }

            DPF1(( "Drawing at %d by %d... Flags = 0x%X", xExt, yExt, wFlags ));
            DrawDibBegin( hdd, hdcFullScreen, xExt, yExt,
                          lpbi, lpbi->biWidth, lpbi->biHeight, wFlags );

            MoveWindow( hwndFullScreen, 0, 0, dxScreen, dyScreen, FALSE );
            ShowWindow( hwndFullScreen, SW_SHOW );
            UpdateWindow( hwndFullScreen );

            ShowCursor( FALSE );
            SetForegroundWindow( hwndFullScreen );
            SetFocus( hwndFullScreen );
        }
    }

    return hwndFullScreen != NULL ? DISPLAYDIB_NOERROR : DISPLAYDIB_NOTSUPPORTED;
}



/*****************************Private*Routine******************************\
* DisplayDibLeave
*
*
*
* History:
* 23-03-94 - StephenE - Created
*
\**************************************************************************/
void
DisplayDibLeave(
    UINT wFlags
    )
{
    if (hwndFullScreen) {
        DestroyWindow( hwndFullScreen );
        hwndFullScreen = NULL;
    }

}

/*****************************Private*Routine******************************\
* DisplayCalcMovieMultiplier
*
* Determines the largest movie that the display is capable of displaying.
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
int
DisplayCalcMovieMultiplier(
    int cxOriginal,
    int cyOriginal,
    DWORD dwCompression
    )
{
    SYSTEM_INFO     SysInfo;
    int             iMult;
    int             iMultTemp;
    int             cxOriginalSave, cyOriginalSave;
    int             iMax = 8;


    GetSystemInfo( &SysInfo );
    iMultTemp = iMult = 0;
    cxOriginalSave = cxOriginal;
    cyOriginalSave = cyOriginal;


    switch ( SysInfo.wProcessorArchitecture ) {

    case PROCESSOR_ARCHITECTURE_INTEL:
        if ( SysInfo.wProcessorLevel <= 3 ) {
            break;
        } else
        if ( SysInfo.wProcessorLevel == 4 ) {
            iMax = 2;
            iMax = mmGetProfileInt(szIni, TEXT("MaxFullScreenShift"), iMax);
            //DPF0(("Setting the maximum shift multiplier to %d\n", iMax));
        }

        /*
        ** maybe later we will do something more different for i486's
        ** for now they just fall through to the RISC / Pentium default
        ** case below.
        */

    default:

        while ( ( (cxOriginal<<=1) <= CX_MAX_MOVIE_DEFAULT)
             && ( (cyOriginal<<=1) <= CY_MAX_MOVIE_DEFAULT)
             && (iMax >= iMult)) {
            ++iMult;
        }
        break;
    }

    return iMult;
}


/******************************Public*Routine******************************\
* FullScreenWndProc
*
*
*
* History:
* 23-03-94 - StephenE - Created
*
\**************************************************************************/
LRESULT CALLBACK
FullScreenWndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch ( message ) {

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            RECT        rc;

            DPF4(( "Window needs painting" ));
            BeginPaint( hwnd, &ps );
            GetUpdateRect( hwnd, &rc, FALSE );
            FillRect( hdcFullScreen, &rc, GetStockObject( BLACK_BRUSH ) );
            EndPaint( hwnd, &ps );
        }
        break;

    case WM_PALETTECHANGED:
        if ( (HWND)wParam == hwnd ) {
            break;
        }

        /* fall thru */

    case WM_QUERYNEWPALETTE:
        if ( DrawDibRealize( hdd, hdcFullScreen, FALSE ) > 0 ) {
            InvalidateRect( hwnd, NULL, TRUE );
            return TRUE;
        }
        break;

    case WM_DESTROY:
        {
            ATOM atm;
            DPF4(( "Window destroyed releasing DC" ));
            ReleaseDC( hwnd, hdcFullScreen );
            DrawDibEnd( hdd );
            DrawDibClose( hdd );
            hdd = NULL;
            hdcFullScreen = NULL;

            UnregisterClass( TEXT("SJE_FULLSCREEN"), GetModuleHandle( NULL ) );

            fClassRegistered = FALSE;

            ShowCursor( TRUE );
            UnhookWindowsHookEx( hHookK );
            while (atm = GlobalFindAtom(szAtomFlag)) {
                GlobalDeleteAtom(atm);
            }
        }

        break;

        //case WM_KILLFOCUS:
        //case WM_ACTIVATE:
        //case WM_SETFOCUS:
        //    DPF0(("FullWindowProc, message==%8x, wp/lp  %8x/%8x\n", message, wParam, lParam));

    default:
        return DefWindowProc( hwnd, message, wParam, lParam );
    }

    return (LRESULT)FALSE;
}



/******************************Public*Routine******************************\
* KeyboardHookProc
*
*
*
* History:
* 23-03-94 - StephenE - Created
*
\**************************************************************************/

LRESULT CALLBACK
KeyboardHookProc(
    int nCode,
    WPARAM wParam,
    LPARAM lParam
    )
{
    //DPF0(("HookProc, ncode == %d, lParam==%8x\n", nCode, lParam));
    if ( nCode == HC_ACTION) {

        DPF1(( "lParam = 0x%X", lParam ));
        DPF1(( "!  wParam = 0x%X\n", wParam ));

        /*
        ** Don't bugger about with the control or shift key.  This is because
        ** mciwnd uses them to start playing fullscreen.  This causes the movie
        ** to start start playing and then immediately stop.  0x001D0000 is
        ** the scan code for the control keys, 0x002A0000 is the scan code
        ** for the shift key.
        */
        if ( (lParam & 0x00FF0000) == 0x001D0000
          || (lParam & 0x00FF0000) == 0x002A0000 ) {

            return CallNextHookEx( hHookK, nCode, wParam, lParam );
        }


        /*
        ** The most significant bit of lParam is set if the key is being
        ** released.  We are only interested in keydowns.  Bits 16 - 23 are
        ** the hardware scan code of the key being pressed, 0x00010000
        ** is the scan code for the escape key.
        */
        if ( !(lParam & 0x80000000) || ((lParam & 0x00FF0000) == 0x00010000)) {

            if (!fStop) {

                fStop = TRUE;
                GlobalAddAtom(szAtomFlag);

                /*
                ** Don't let windows see this message.
                */
                return -1;
            }

            DPF1(( "Stop requested from the keyboard hook" ));
        }
    }

    return CallNextHookEx( hHookK, nCode, wParam, lParam );
}
