/**************************************************************************

    DRAWDIB.C   - routines for drawing DIBs to the screen.

    Copyright (c) Microsoft Corporation 1992 - 1995. All rights reserved.

    this code handles stretching and dithering with custom code, none
    of this slow as hell GDI code.

    the following DIB formats are supported:

        4bpp (will just draw it with GDI...)
        8bpp
        16bpp
        24bpp
        compressed DIBs

    drawing to:

        16 color DC         (will dither 8bpp down)
        256 (paletized) DC  (will dither 16 and 24bpp down)
        Full-color DC       (will just draw it!)

**************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <profile.h>
#include "drawdibi.h"
#include "profdisp.h"

#define USE_DCI
#ifdef DAYTONA
// Daytona dynamically links to DCI - see dcilink.h for details
#include "dcilink.h"
#endif

#ifndef abs
#define abs(x)  ((x) < 0 ? -(x) : (x))
#endif

#ifndef ICMODE_FASTDECOMPRESS
    #define ICMODE_FASTDECOMPRESS 3
#endif

#define USE_SETDI 1

#define DIB_PAL_INDICES		2

#ifndef BI_BITMAP
    #define BI_BITMAP   0x4D544942      // 'BITM'
#endif

#ifdef _WIN32
#define FlatToHuge(a, b, c)
#define HugeToFlat(a, b, c)
#ifdef DEBUG
#define FlushBuffer()	GdiFlush()
#else
#define FlushBuffer()	
#endif

#else //Win16
extern FAR PASCAL FlatToHuge(LPVOID,DWORD,DWORD);
extern FAR PASCAL HugeToFlat(LPVOID,DWORD,DWORD);
#define FlushBuffer()
#endif

#define IsScreenDC(hdc) (WindowFromDC(hdc) != NULL)

STATICFN BOOL DrawDibCheckPalette(PDD pdd);

BOOL VFWAPI DrawDibTerm(void);
BOOL VFWAPI DrawDibInit(void);

//
// Local variables
//

#ifdef DEBUG_RETAIL
static int fDebug = -1;
#endif

/**************************************************************************
**************************************************************************/

WORD                gwScreenBitDepth = (WORD)-1;
UINT                gwScreenWidth = 0;
UINT                gwScreenHeight = 0;
UINT                gwRasterCaps = 0;
#ifndef _WIN32
BOOL                gf286= FALSE;
#else
    #define gf286 FALSE
#endif
static UINT         gUsage = 0;
static BOOL         gfInit = FALSE;
static BOOL         gfHalftone = FALSE;
static BOOL         gfBitmap   = FALSE;
static BOOL         gfBitmapX  = FALSE;
#ifdef USE_DCI
#ifdef UNICODE
// we expose the DCI variables on NT to aid with stress debugging
#define STATIC
#else // for Win95
#define STATIC static
#endif
STATIC BOOL         gfScreenX  = FALSE;
#ifdef WANT_DRAW_DIRECT_TO_SCREEN
STATIC BOOL         gfDrawX    = FALSE;
#endif
#endif // USE_DCI
static HBITMAP      hbmStockMono;               // the stock mono bitmap.

#ifdef USE_DCI
STATIC  BOOL gfDisplayHasBrokenRasters;
STATIC  HDC hdcDCI;
STATIC  DCISURFACEINFO FAR *pdci;

STATIC  struct {
	    BITMAPINFOHEADER bi;
	    DWORD            dwMask[4];
	} biScreen;
STATIC  LPVOID lpScreen;

#ifndef _WIN32
static  UINT ScreenSel;
static  DCISURFACEINFO dci;
#endif

SZCODEA szDVA[] = "dva";

#define DCAlignment 3

__inline int DCNotAligned(HDC hdc, int xDst) {
    POINT pt;

    pt.x = xDst; pt.y = 0;
    LPtoDP(hdc, &pt, 1);
    xDst = pt.x;

#ifdef _WIN32
    GetDCOrgEx(hdc, &pt);
#else
    pt.x = LOWORD(GetDCOrg(hdc));
#endif
    return (pt.x + xDst) & DCAlignment;
}
#endif // USE_DCI

/**************************************************************************
**************************************************************************/

SZCODEA  szDrawDib[]            = "DrawDib";
SZCODEA  szHalftone[]           = "Halftone";
SZCODEA  szDrawToBitmap[]       = "DrawToBitmap";
SZCODEA  szDecompressToBitmap[] = "DecompressToBitmap";
#ifdef USE_DCI
SZCODEA  szDecompressToScreen[]  = "DecompressToScreen";
SZCODEA  szDrawToScreen[]        = "DrawToScreen";
#endif

/**************************************************************************
**************************************************************************/

#if 0
/**************************************************************************
**************************************************************************/

typedef struct {
    UINT    Usage;
    HBITMAP hbm;
    int     dx,dy;
}   BITBUF, *PBITBUF;

static BITBUF bb;

static HBITMAP AllocBitmap(int dx, int dy)
{
    return NULL;
}
#endif

/**************************************************************************
**************************************************************************/

STATICFN BOOL NEAR PASCAL DrawDibFree(PDD pdd, BOOL fSameDib, BOOL fSameSize);
STATICFN HPALETTE CreateBIPalette(HPALETTE hpal, LPBITMAPINFOHEADER lpbi);
STATICFN BOOL NEAR IsIdentityPalette(HPALETTE hpal);
STATICFN BOOL NEAR AreColorsAllGDIColors(LPBITMAPINFOHEADER lpbi);
STATICFN BOOL SetPalFlags(HPALETTE hpal, int iIndex, int cntEntries, UINT wFlags);

static void DrawDibPalChange(PDD pdd, HDC hdc, HPALETTE hpal);
static void DrawDibClipChange(PDD pdd, UINT wFlags);
static BOOL FixUpCodecPalette(HIC hic, LPBITMAPINFOHEADER lpbi);
static BOOL SendSetPalette(PDD pdd);

#ifdef USE_DCI

#ifndef _WIN32

#define GetDS() SELECTOROF((LPVOID)&ScreenSel)

/****************************************************************************
 ***************************************************************************/
#pragma optimize("", off)
static void SetSelLimit(UINT sel, DWORD limit)
{
    if (limit >= 1024*1024l)
        limit = ((limit+4096) & ~4095) - 1;

    _asm
    {
        mov     ax,0008h            ; DPMI set limit
        mov     bx,sel
        mov     dx,word ptr limit[0]
        mov     cx,word ptr limit[2]
        int     31h
    }
}
#pragma optimize("", on)

#endif // _WIN32

/**************************************************************************
**************************************************************************/
static void InitDCI()
{
    UINT WidthBytes;

    //
    // initialize DCI and open a surface handle to it.
    //
    // if DVA = 0 in WIN.INI, don't use DCI or DVA.
    // PSS tells people to use this if they have video problems,
    // so we shouldn't change the string.
    // On NT the value is in the REGISTRY,
    // HKEY_CURRENT_USER\SOFTWARE\Microsoft\Multimedia\Drawdib
    //		REG_DWORD dva 1      dci enabled
    //		REG_DWORD dva 0      dci disabled
    // This value can also be set through the Video For Windows configuration
    // dialog (control panel, drivers, or via Mplay32 on an open avi file).
    if (!mmGetProfileIntA(szDrawDib, szDVA, TRUE))
	return;

#ifdef DAYTONA
    if (!InitialiseDCI()) return;
#endif

    hdcDCI = DCIOpenProvider();

    if (hdcDCI == NULL)	{
       DPF(("Failed to open DCI provider"));
       return;
    }

#ifndef _WIN32
    SetObjectOwner(hdcDCI, NULL); // on Win16, this is shared between
    // processes, so tell GDI not to clean it up or whine about it not
    // being freed.
#endif

#ifdef _WIN32
    DCICreatePrimary(hdcDCI, &pdci);
#else
    //
    // because we call 32-bit codecs we want a 32-bit DCI surface
    // (with a linear pointer, etc...)
    //
    dci.dwSize = sizeof(dci);
    if (DCICreatePrimary32(hdcDCI, &dci) == 0)
        pdci = &dci;
    else
        pdci = NULL;
#endif

    if (pdci == NULL) {
	DCICloseProvider(hdcDCI);
	hdcDCI = NULL;
        return;
    }

    WidthBytes = (UINT) abs(pdci->lStride);

    //
    // convert DCISURFACEINFO into a BITMAPINFOHEADER...
    //
    biScreen.bi.biSize          = sizeof(BITMAPINFOHEADER);
    biScreen.bi.biCompression   = pdci->dwCompression;

    biScreen.bi.biWidth         = WidthBytes*8/(UINT)pdci->dwBitCount;
    biScreen.bi.biHeight        = pdci->dwHeight;
    biScreen.bi.biPlanes        = 1;
    biScreen.bi.biBitCount      = (UINT)pdci->dwBitCount;
    biScreen.bi.biSizeImage     = pdci->dwHeight * WidthBytes;
    biScreen.bi.biXPelsPerMeter = WidthBytes;
    biScreen.bi.biYPelsPerMeter = 0;
    biScreen.bi.biClrUsed       = 0;
    biScreen.bi.biClrImportant  = 0;
    biScreen.dwMask[0]          = pdci->dwMask[0];
    biScreen.dwMask[1]          = pdci->dwMask[1];
    biScreen.dwMask[2]          = pdci->dwMask[2];

    if (pdci->dwCompression == 0) {
        if ((UINT)pdci->dwBitCount == 16)
	{
	    biScreen.dwMask[0] = 0x007C00;
	    biScreen.dwMask[1] = 0x0003E0;
	    biScreen.dwMask[2] = 0x00001F;
	}

	else if ((UINT)pdci->dwBitCount >= 24)

	{
	    biScreen.dwMask[0] = 0xFF0000;
	    biScreen.dwMask[1] = 0x00FF00;
	    biScreen.dwMask[2] = 0x0000FF;
	}
    }

    if (pdci->lStride > 0)
        biScreen.bi.biHeight = -(int)pdci->dwHeight;
#if 0   // BOGUS
    else
        pdci->dwOffSurface -= biScreen.bi.biSizeImage;
#endif

    if (pdci->dwDCICaps & DCI_1632_ACCESS)
    {
	biScreen.bi.biCompression = BI_1632;
	
	//
	// make sure the pointer is valid.
	//
	if (pdci->dwOffSurface >= 0x10000)
	{
	    DPF(("DCI Surface can't be supported: offset >64K"));

	    lpScreen = NULL;
	    biScreen.bi.biBitCount = 0;
	}
	else
	{
    #ifdef _WIN32
	    lpScreen = (LPVOID)MAKELONG((WORD)pdci->dwOffSurface, pdci->wSelSurface);
    #else
	    lpScreen = (LPVOID)MAKELP(pdci->wSelSurface,pdci->dwOffSurface);
    #endif
	}
    }
    else
    {
#ifdef _WIN32
	lpScreen = (LPVOID) pdci->dwOffSurface;
#else
	//
	// If we weren't given a selector, or the offset is >64K, we should
	// handle this case and reset the base of the selector.
	//
	// Also must do this for GDT selectors, or the Kernel thunking
	// code will kill us.
	//
	if (pdci->wSelSurface == 0 || pdci->dwOffSurface >= 0x10000)
	{
	    ScreenSel = AllocSelector(GetDS());

	    if (pdci->wSelSurface)
		SetSelectorBase(ScreenSel,
		    GetSelectorBase(pdci->wSelSurface) + pdci->dwOffSurface);
	    else
		SetSelectorBase(ScreenSel, pdci->dwOffSurface);

	    SetSelLimit(ScreenSel, biScreen.bi.biSizeImage - 1);

	    lpScreen = (LPVOID)MAKELP(ScreenSel,0);
	}
	else
	{
	    lpScreen = (LPVOID)MAKELP(pdci->wSelSurface,pdci->dwOffSurface);
	}
#endif
    }

    DPF(("DCI Surface: %ldx%ldx%ld, lpScreen = %08I", pdci->dwWidth, pdci->dwHeight, pdci->dwBitCount, (DWORD_PTR) lpScreen));
    DPF(("DCI Surface: biCompression= %ld '%4.4hs' Masks: %04lX %04lX %04lX",biScreen.bi.biCompression, (LPSTR) &biScreen.bi.biCompression,biScreen.dwMask[0],biScreen.dwMask[1],biScreen.dwMask[2]));

    //
    // check if the display has broken rasters.
    //

#if defined(DAYTONA) && !defined(_X86_)
    // On MIPS and other machines that have problems with
    // unaligned code we always set gfDisplayHasBrokenRasters to TRUE!
    gfDisplayHasBrokenRasters = TRUE;
#else
    gfDisplayHasBrokenRasters = (0x10000l % WidthBytes) != 0;
#endif

    if (gfDisplayHasBrokenRasters)
    {
        DPF(("*** Display has broken rasters"));
    }
}

void TermDCI()
{
    if (pdci)
    {
        DCIDestroy(pdci);
        pdci = NULL;
    }

    if (hdcDCI)
    {
	DCICloseProvider(hdcDCI);
        hdcDCI = NULL;
    }

#ifndef _WIN32
    if (ScreenSel)
    {
	SetSelLimit(ScreenSel, 0);
	FreeSelector(ScreenSel);
	ScreenSel = 0;
    }
#endif
#ifdef DAYTONA
    TerminateDCI();
#endif
}

#else
    #define InitDCI()
    #define TermDCI()
#endif

/**************************************************************************
* @doc INTERNAL DrawDib
*
* @api BOOL | DrawDibInit | This function initalizes the DrawDib library.
*
* @rdesc Returns TRUE if the library is initialized properly, otherwise
*        it returns FALSE.
*
* @comm Users don't need to call this, because <f DrawDibOpen> does it for them.
*
* @xref DrawDibTerm
*
**************************************************************************/
BOOL VFWAPI DrawDibInit()
{
    HDC hdc;

    WORD wScreenBitDepth;
    UINT wScreenWidth;
    UINT wScreenHeight;
    UINT wRasterCaps;

    hdc = GetDC(NULL);
    wScreenBitDepth = GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES);
    wScreenWidth    = GetSystemMetrics(SM_CXSCREEN);
    wScreenHeight   = GetSystemMetrics(SM_CYSCREEN);
    wRasterCaps     = GetDeviceCaps(hdc, RASTERCAPS);
    ReleaseDC(NULL, hdc);

    if (gfInit)
    {
        //
        // handle a screen res change.
        //
        if (gwScreenWidth    == wScreenWidth &&
            gwScreenHeight   == wScreenHeight &&
            gwScreenBitDepth == wScreenBitDepth)
        {
            return TRUE;
        }

        DPF(("Screen has changed from %dx%dx%d to %dx%dx%d", gwScreenWidth, gwScreenHeight, gwScreenBitDepth, wScreenWidth, wScreenHeight, wScreenBitDepth));

        DrawDibTerm();
    }

#ifndef _WIN32
    gf286 = (BOOL)(GetWinFlags() & WF_CPU286);
#endif

    gwScreenBitDepth = wScreenBitDepth;
    gwScreenWidth    = wScreenWidth;
    gwScreenHeight   = wScreenHeight;
    gwRasterCaps     = wRasterCaps;

    gfHalftone = mmGetProfileIntA(szDrawDib, szHalftone, FALSE);
    gfBitmap   = mmGetProfileIntA(szDrawDib, szDrawToBitmap, -1);
    gfBitmapX  = mmGetProfileIntA(szDrawDib, szDecompressToBitmap, TRUE);
#ifdef USE_DCI
    gfScreenX  = mmGetProfileIntA(szDrawDib, szDecompressToScreen, TRUE);
#ifdef WANT_DRAW_DIRECT_TO_SCREEN
    gfDrawX    = mmGetProfileIntA(szDrawDib, szDrawToScreen, TRUE);
#endif
#endif

#ifdef DEBUG
    gwRasterCaps = mmGetProfileIntA(szDrawDib, "RasterCaps", gwRasterCaps);
    gwScreenBitDepth = (WORD) mmGetProfileIntA(szDrawDib, "ScreenBitDepth", gwScreenBitDepth);
#ifndef _WIN32
    gf286 = GetProfileIntA(szDrawDib, "cpu", gf286 ? 286 : 386) == 286;
#endif
#endif

    InitDCI();

#ifdef DEBUG
    {
	UINT wType = GetBitmapType();

	switch(wType & BM_TYPE)
	{
	    case BM_VGA:
	  	DPF(("display format: VGA mode"));
		break;

	    case BM_1BIT:
		DPF(("display format: 1 bpp"));
		break;

	    case BM_4BIT:
		DPF(("display format: 4 bpp"));
		break;

	    case BM_8BIT:
		DPF(("display format: 8 bpp"));
		break;

	    case BM_16555:
		DPF(("display format: 16-bits, 555"));
		break;

	    case BM_24BGR:
		DPF(("display format: 24-bits BGR"));
		break;

	    case BM_32BGR:
		DPF(("display format: 32-bits BGR"));
		break;

	    case BM_16565:
		DPF(("display format: 16-bits, 565"));
		break;

	    case BM_24RGB:
	    case BM_32RGB:
		DPF(("display format: %d-bits RGB",
		    ((wType == BM_24RGB) ? 24 : 32)));
		break;

	    default:
		DPF(("display format: unknown (type %d)", wType));
		break;
	}
    }
#endif



    //
    // fix up the bit-depth of the display.
    //
    if (gwScreenBitDepth > 32)
        gwScreenBitDepth = 32;

    if (gwScreenBitDepth == 16 || gwScreenBitDepth == 32)
    {
        BITMAPINFOHEADER bi;
        UINT u;

        bi.biSize           = sizeof(bi);
        bi.biWidth          = 1;
        bi.biHeight         = 1;
        bi.biPlanes         = 1;
        bi.biBitCount       = gwScreenBitDepth;
        bi.biCompression    = 0;
        bi.biSizeImage      = 0;
        bi.biXPelsPerMeter  = 0;
        bi.biYPelsPerMeter  = 0;
        bi.biClrUsed        = 0;
        bi.biClrImportant   = 0;

        u = (UINT)DrawDibProfileDisplay(&bi);

        if (u == 0)
        {
            DPF(("Pretending display is 24 bit (not %d)", gwScreenBitDepth));
            gwScreenBitDepth = 24;
        }
    }

    gfInit = TRUE;
    return TRUE;
}

/**************************************************************************
* @doc INTERNAL DrawTerm
*
* @api BOOL | DrawDibTerm | This function teminates the DrawDib library.
*
* @rdesc Returns TRUE.
*
* @comm Users don't need to call this, because <f DrawDibClose> does it for them.
*
* @xref DrawDibInit
*
**************************************************************************/
BOOL VFWAPI DrawDibTerm()
{
    //
    //  free global stuff.
    //

    TermDCI();

    gfInit = FALSE;
    return TRUE;
}

/**************************************************************************
* @doc INTERNAL DrawDib
*
* @api void | DrawDibCleanup | clean up drawdib stuff
*   called in MSVIDEOs WEP()
*
**************************************************************************/
void FAR PASCAL DrawDibCleanup(HTASK hTask)
{
    if (gUsage > 0)
        RPF(("%d DrawDib handles left open", gUsage));

    DrawDibTerm();
}

/**************************************************************************
* @doc	INTERNAL
*
* @api BOOL | DibEq | This function compares two dibs.
*
* @parm LPBITMAPINFOHEADER | lpbi1 | Pointer to one bitmap.
*       this DIB is assumed to have the colors after biSize bytes.
*
* @parm LPBITMAPINFOHEADER | lpbi2 | Pointer to second bitmap.
*       this DIB is assumed to have the colors after biSize bytes.
*
* @rdesc Returns TRUE if bitmaps are identical, FALSE otherwise.
*
**************************************************************************/
INLINE BOOL NEAR PASCAL DibEq(LPBITMAPINFOHEADER lpbi1,LPBITMAPINFOHEADER lpbi2)
{
    if (lpbi1 == NULL || lpbi2 == NULL)
	return FALSE;

    return
             lpbi1->biCompression == lpbi2->biCompression   &&
        (int)lpbi1->biSize        == (int)lpbi2->biSize     &&
        (int)lpbi1->biWidth       == (int)lpbi2->biWidth    &&
        (int)lpbi1->biHeight      == (int)lpbi2->biHeight   &&
        (int)lpbi1->biBitCount    == (int)lpbi2->biBitCount &&
        ((int)lpbi1->biBitCount > 8 ||
            (int)lpbi1->biClrUsed == (int)lpbi2->biClrUsed  &&
            _fmemcmp((LPBYTE)lpbi1 + lpbi1->biSize,
		(LPBYTE)lpbi2 + lpbi2->biSize,
                (int)lpbi1->biClrUsed*sizeof(RGBQUAD)) == 0);
}

/**************************************************************************
* @doc INTERNAL
*
* @api PDD NEAR | DrawDibLockNoTaskCheck | Lock the DrawDib handle.
*
* @comm No check is made on the validity of the calling task
*
* @parm HDRAWDIB | hdd | DrawDib handle.
*
* @rdesc Returns a pointer to a <t DRAWDIB_STRUCT> if successful, NULL otherwise.
*
**************************************************************************/

#define OffsetOf(s,m)	(DWORD_PTR)&(((s *)0)->m)

INLINE PDD NEAR PASCAL DrawDibLockNoTaskCheck(HDRAWDIB hdd)
{
#ifdef DEBUG
    if (OffsetOf(DRAWDIB_STRUCT, wSize) != 0) {
        DPF0(("INTERNAL FAILURE"));
        DebugBreak();
    }
#endif

    if (hdd == NULL ||
	IsBadWritePtr((LPVOID) (PDD) hdd, sizeof(DRAWDIB_STRUCT)) ||
#if defined(DAYTONA) && !defined(_X86_)
	(*(DWORD UNALIGNED *)hdd) != sizeof(DRAWDIB_STRUCT))
#else
 	((PDD)hdd)->wSize != sizeof(DRAWDIB_STRUCT))
#endif
    {

#ifndef _WIN32
#ifdef DEBUG_RETAIL
	LogParamError(ERR_BAD_HANDLE, DrawDibDraw, (LPVOID) (DWORD) (UINT) hdd);
#endif
#endif
	
	return NULL;
    }

    return (PDD) hdd;
}

/**************************************************************************
* @doc INTERNAL
*
* @api PDD NEAR | DrawDibLock | Lock the DrawDib handle.
*
* @parm HDRAWDIB | hdd | DrawDib handle.
*
* @rdesc Returns a pointer to a <t DRAWDIB_STRUCT> if successful, NULL otherwise.
*
**************************************************************************/
INLINE PDD NEAR PASCAL DrawDibLock(HDRAWDIB hdd)
{
    PDD pdd = DrawDibLockNoTaskCheck(hdd);

#ifndef _WIN32
    if (pdd && (pdd->htask != GetCurrentTask())) {
	DPF(("DrawDib handle used from wrong task!"));
#ifdef DEBUG_RETAIL
	LogParamError(ERR_BAD_HANDLE, DrawDibDraw, (LPVOID) (DWORD) (UINT) hdd);
#endif
	return NULL;

    }
#endif

    return pdd;
}


/**************************************************************************
* @doc EXTERNAL DrawDib
*
* @api HDRAWDIB | DrawDibOpen | This function opens a DrawDib context for drawing.
*
* @rdesc Returns a handle to a DrawDib context if successful,
*        otherwise it returns NULL.
*
* @comm Use this function to obtain a handle to a DrawDib context
*       before drawing device independent bitmaps.
*
*       If drawing multiple device independent bitmaps simultaneously,
*       obtain a handle to a DrawDib context for each bitmap.
*
* @xref <f DrawDibClose>
*
**************************************************************************/
HDRAWDIB VFWAPI DrawDibOpen(void)
{
    HDRAWDIB hdd;
    PDD      pdd;

    hdd = LocalAlloc(LPTR, sizeof(DRAWDIB_STRUCT));    /* zero init */

    if (hdd == NULL)
        return NULL;

    pdd = (PDD)hdd;
    pdd->wSize = sizeof(DRAWDIB_STRUCT);

#ifndef _WIN32
    pdd->htask = GetCurrentTask();
#endif

    if (gUsage++ == 0)
        DrawDibInit();

    return hdd;
}

/**************************************************************************
* @doc EXTERNAL DrawDib
*
* @api BOOL | DrawDibClose | This function closes a DrawDib context
*      and frees the resources DrawDib allocated for it.
*
* @parm HDRAWDIB | hdd | Specifies a handle to a DrawDib context.
*
* @rdesc Returns TRUE if the context closed successfully.
*
* @comm Use this function to free the <p hdd> handle
*       after the application has finished drawing.
*
* @xref <f DrawDibOpen>
**************************************************************************/
BOOL VFWAPI DrawDibClose(HDRAWDIB hdd)
{
    PDD pdd;

    if ((pdd = DrawDibLock(hdd)) == NULL)
        return FALSE;

    DrawDibFree(pdd, FALSE, FALSE);

    pdd->wSize = 0;
    LocalFree(hdd);

    if (--gUsage == 0)
        DrawDibTerm();

    return TRUE;
}

/**************************************************************************
* @doc INTERNAL
*
* @api BOOL | DrawDibFree | Free up everything in a <t DRAWDIB_STRUCT>.
*
* @parm PDD | pdd | Pointer to a <t DRAWDIB_STRUCT>.
*
* @rdesc Returns TRUE if successful, FALSE otherwise.
*
**************************************************************************/
STATICFN BOOL NEAR PASCAL DrawDibFree(PDD pdd, BOOL fSameDib, BOOL fSameSize)
{
    if (pdd == NULL)
        return FALSE;

    //
    // If the draw palette has changed, the compressor may now be giving us DIBs
    // mapped to a different palette, so we need to clean up so we'll produce
    // a new mapping table so we'll actually draw with the new palette.
    // (see SendSetPalette)
    //
    if (!fSameDib) {
        //
        // if this palette is selected as the foreground palette
        // and we delete it we are going to hose GDI!
        //
	if (pdd->hpal)
	    DeleteObject(pdd->hpal);
	if (pdd->hpalCopy)
	    DeleteObject(pdd->hpalCopy);

        pdd->hpal = NULL;
        pdd->hpalCopy = NULL;
    }

    if (!fSameDib) {

	if (pdd->lpbi) {
	    GlobalFreePtr(pdd->lpbi);
	    pdd->lpbi = NULL;
	    pdd->lpargbqIn = NULL;
	}

        if (pdd->lpDitherTable)
        {
            DitherTerm(pdd->lpDitherTable);
	    pdd->lpDitherTable = NULL;
	}

        if (pdd->hic && pdd->hic != (HIC)-1)
        {
            ICDecompressEnd(pdd->hic);
            ICDecompressExEnd(pdd->hic);
            ICClose(pdd->hic);
        }

        pdd->ulFlags &= ~(DDF_IDENTITYPAL);
        pdd->hic  = NULL;

        pdd->iAnimateStart = 0;
        pdd->iAnimateLen = 0;
        pdd->iAnimateEnd = 0;
    }

    if (!fSameSize || !fSameDib)
    {
        if (pdd->hdcDraw) {
	    if (hbmStockMono)
		SelectObject(pdd->hdcDraw, hbmStockMono);

            DeleteDC(pdd->hdcDraw);
	}

        if (pdd->hbmDraw) {
            DeleteObject(pdd->hbmDraw);

            //
            // if we have a bitmap pointer lose it
            //
            if (pdd->ulFlags & (DDF_CANBITMAPX))
                pdd->pbBitmap = NULL;

        }

        if ((pdd->pbStretch) && (pdd->pbStretch != pdd->lpDIBSection))
            GlobalFreePtr(pdd->pbStretch);

        if ((pdd->pbDither) && (pdd->pbDither != pdd->lpDIBSection))
            GlobalFreePtr(pdd->pbDither);

        if ((pdd->pbBuffer) && (pdd->pbBuffer != pdd->lpDIBSection))
            GlobalFreePtr(pdd->pbBuffer);

#if USE_SETDI
	if (pdd->hbmDraw)
            SetBitmapEnd(&pdd->sd);
#endif

        pdd->hdcDraw = NULL;
        pdd->hbmDraw = NULL;
	pdd->lpDIBSection = NULL;
        pdd->pbStretch = NULL;
        pdd->pbDither = NULL;
        pdd->pbBuffer = NULL;

        pdd->biDraw.biBitCount = 0;
        pdd->biDraw.biWidth    = 0;
        pdd->biDraw.biHeight   = 0;

        pdd->biBuffer.biBitCount = 0;
        pdd->biBuffer.biWidth    = 0;
        pdd->biBuffer.biHeight   = 0;

        // clear all the internal flags (except palette stuff)
        pdd->ulFlags &= ~(DDF_OURFLAGS ^ DDF_IDENTITYPAL);
        pdd->ulFlags |= DDF_DIRTY;

	pdd->iDecompress = 0;
    }

    return TRUE;
}

/**************************************************************************
* @doc INTERNAL
*
* @api UINT | QueryDraw | see if the current display device
*             (DISPDIB or GDI) can draw the given dib
*
* @parm PDD | pdd | pointer to a <t DRAWDIB_STRUCT>.
*
* @parm LPBITMAPINFOHEADER | lpbi | pointer to a bitmap.
*
* @rdesc Returns display flags, see profdisp.h
*
**************************************************************************/

#ifndef DEBUG
#define QueryDraw(pdd, lpbi)  (UINT)DrawDibProfileDisplay((lpbi))
#endif

#ifndef QueryDraw
STATICFN UINT NEAR QueryDraw(PDD pdd, LPBITMAPINFOHEADER lpbi)
{
    UINT    u;

    u = (UINT)DrawDibProfileDisplay(lpbi);

    DPF(("QueryDraw (%dx%dx%d): %d", PUSHBI(*lpbi), u));
    return u;
}
#endif


/**************************************************************************
* @doc INTERNAL DrawDib
*
* @comm Called from DrawDibBegin to try decompression to a bitmap.
*
**************************************************************************/
BOOL DrawDibQueryBitmapX(
    PDD pdd
)
{
    BITMAPINFOHEADER *pbi;

#ifndef _WIN32
    if (!CanLockBitmaps()) {
        return FALSE;
    }
#endif

    if (gwScreenBitDepth == 8 && !(gwRasterCaps & RC_PALETTE))
        return FALSE;

    if ((gwRasterCaps & RC_PALETTE) && !(pdd->ulFlags & DDF_IDENTITYPAL))
        return FALSE;

    pbi = &pdd->biStretch;

    if (!GetDIBBitmap(pdd->hbmDraw, pbi))
        return FALSE;

#ifdef XDEBUG
    if (ICDecompressQuery(pdd->hic, pdd->lpbi, pbi) != ICERR_OK)
    {
        if (mmGetProfileIntA(szDrawDib, "ForceDecompressToBitmap", FALSE))
        {
            pbi->biHeight = -pbi->biHeight;
            pbi->biCompression = 0;
        }
    }
#endif
    if (ICDecompressQuery(pdd->hic, pdd->lpbi, pbi) != ICERR_OK)
    {
        if (pbi->biCompression == BI_BITMAP &&
            pbi->biSizeImage <= 128*1024l &&
            (pbi->biXPelsPerMeter & 0x03) == 0 &&
            pbi->biSizeImage > 64*1024l)
        {
            pdd->ulFlags |= DDF_HUGEBITMAP;
            pbi->biCompression = 0;

            pbi->biSizeImage -= pbi->biYPelsPerMeter;   //FillBytes

            if (ICDecompressQuery(pdd->hic, pdd->lpbi, pbi) != ICERR_OK)
                return FALSE;
        }
        else
            return FALSE;
    }

    pdd->ulFlags |= DDF_NEWPALETTE;     // force check in DrawDibRealize
    pdd->ulFlags |= DDF_CANBITMAPX;     // can decompress to bitmaps

    if (pdd->ulFlags & DDF_HUGEBITMAP)
        RPF(("    Can decompress '%4.4hs' to a HUGE BITMAP (%dx%dx%d)",(LPSTR)&pdd->lpbi->biCompression, PUSHBI(*pbi)));
    else
        RPF(("    Can decompress '%4.4hs' to a BITMAP (%dx%dx%d)",(LPSTR)&pdd->lpbi->biCompression, PUSHBI(*pbi)));

    //
    // reuse the stretch buffer for the bitmap.
    //
    pdd->biStretch = *pbi;
#ifndef _WIN32
    pdd->pbStretch = LockBitmap(pdd->hbmDraw);

    if (pdd->pbStretch == NULL)
    {
        DPF(("    Unable to lock bitmap!"));
        pdd->ulFlags &= ~DDF_CANBITMAPX; // can't decompress to bitmaps
        return FALSE;
    }
#endif

    return TRUE;
}



#define Is565(bi)   (((bi)->biCompression == BI_BITFIELDS) &&   \
		    ((bi)->biBitCount == 16) &&			\
		    (((LPDWORD)((bi)+1))[0] == 0x00F800) &&	\
		    (((LPDWORD)((bi)+1))[1] == 0x0007E0) &&	\
		    (((LPDWORD)((bi)+1))[2] == 0x00001F) )



/**************************************************************************
* @doc EXTERNAL DrawDib
*
* @api BOOL | DrawDibBegin | This function changes parameters
*      of a DrawDib context or it initializes a new DrawDib context.
*
* @parm HDRAWDIB | hdd | Specifies a handle to a DrawDib context.
*
* @parm HDC | hdc | Specifies a handle to a display context for drawing (optional).
*
* @parm int | dxDest | Specifies the width of the destination rectangle.
*       Width is specified in MM_TEXT client units.
*
* @parm int | dyDest | Specifies the height of the destination rectangle.
*       Height is specified in MM_TEXT client units.
*
* @parm LPBITMAPINFOHEADER | lpbi | Specifies a pointer to a
*       <t BITMAPINFOHEADER> structure containing the
*       image format. The color table for the DIB follows the
*       image format.
*
* @parm int | dxSrc | Specifies the width of the source rectangle.
*       Width is specified in pixels.
*
* @parm int | dySrc | Specifies the height of the source rectangle.
*       Height is specified in pixels.
*
* @parm UNIT | wFlags | Specifies the applicable flags for
*       the function. The following flags are defined:
*
* @flag DDF_UPDATE | Indicates the last buffered bitmap is to be redrawn.
*       If drawing fails with this flag, a buffered image is not available
*       and a new image needs to be specified before the display is updated.
*
* @flag DDF_SAME_HDC | Assumes the handle to the display context
*       is already specified. When this flag is used,
*       DrawDib also assumes the correct palette has already been
*       realized into the device context (possibly by
*       <f DrawDibRealize>).
*
* @flag DDF_SAME_DRAW | Uses the drawing parameters previously
*       specified for this function.  Use this flag only
*       if <p lpbi>, <p dxDst>, <p dyDst>, <p dxSrc>, and <p dySrc>
*       have not changed since using <f DrawDibDraw> or <f DrawDibBegin>.
*
* @flag DDF_DONTDRAW | Indicates the frame is to be decompressed
*       and not drawn. The DDF_UPDATE flag can be used later
*       to actually draw the image.
*
* @flag DDF_ANIMATE | Allows palette animation. If this flag is present,
*       the palette <f DrawDib> creates will have the PC_RESERVED flag set for
*       as many entries as possible, and the palette can be animated by
*       <f DrawDibChangePalette>. If using <f DrawDibBegin> with
*       <f DrawDibDraw>, set this flag with <f DrawDibBegin>
*       rather than <f DrawDibDraw>.
*
* @flag DDF_JUSTDRAWIT | Uses GDI to draw the image. This prevents
*       the DrawDib functions from calling ICM to decompress
*       the image or prevents them from
*       using their own routines to stretch or dither the image.
*       This essentially reduces <f DrawDibDraw> to <f StretchDIBits>.
*
* @flag DDF_BACKGROUNDPAL | Realizes the palette used for drawing
*       in the background leaving the actual palette used for display
*       unchanged.  (This flag is valid only if DDF_SAME_HDC is not set.)
*
* @flag DDF_HALFTONE | Always dithers the DIB to a standard palette
*       regardless of the palette of the DIB. If using <f DrawDibBegin> with
*       <f DrawDibDraw>, set this flag with <f DrawDibBegin>
*       rather than <f DrawDibDraw>.
*
* @flag DDF_BUFFER | Indicates DrawDib should try to use a
*       offscreen buffer so DDF_UPDATE can be used. This
*       disables decompression and drawing directly to the screen.
*       If DrawDib is unable to create an offscreen buffer,
*       it will decompress or draw directly to the screen.
*
*       For more information, see the DDF_UPDATE and DDF_DONTDRAW
*       flags described for <f DrawDibDraw>.
*
*
* @rdesc Returns TRUE if successful.
*
* @comm This function prepares to draw a bitmap specified by <p lpbi>
*       to the display context <p hdc>. The image is stretched to
*       the size specified by <p dxDest> and <p dyDest>. If <p dxDest> and
*       <p dyDest> are (-1, -1), the bitmap is drawn to a
*       1:1 scale without stretching.
*
*       Use this function only if you want to prepare DrawDib
*       before using <f DrawDibDraw> to draw the image.
*       If you do not use this function, <f DrawDibDraw> implicitly
*       uses it when it draws the image.
*
*       To update the flags set with <f DrawDibBegin>, use
*       <f DrawDibEnd> to free the DrawDib context and reset
*       the flags with <f DrawDibBegin>, or specify the new flags
*       with changed values for <p dxDest>, <p dyDest>, <p lpbi>, <p dxSrc>,
*       or <p dySrc>.
*
*       When <f DrawDibBegin> is used, the <f DDF_SAME_DRAW>
*       flag is normally set for <f DrawDibDraw>.
*
*       If the parameters of <f DrawDibBegin> have not changed, subsequent
*       uses of it have not effect.
*
*       Use <f DrawDibEnd> to free memory used by the DrawDib context.
*
* @xref <f DrawDibEnd> <f DrawDibDraw>
**************************************************************************/
#ifndef _WIN32
#pragma message("Make DrawDibBegin faster for changing the size only!")
#endif

BOOL VFWAPI DrawDibBegin(HDRAWDIB hdd,
                             HDC      hdc,
                             int      dxDst,
                             int      dyDst,
                             LPBITMAPINFOHEADER lpbi,
                             int      dxSrc,
                             int      dySrc,
                             UINT     wFlags)
{
    PDD pdd;
    WORD ScreenBitDepth;
    int dxSave,dySave;
    BOOL     fNewPal;
    BOOL     fSameDib;
    BOOL     fSameSize;
    BOOL     fSameFlags;
    BOOL     fSameHdc;
    UINT    wFlagsChanged;
    DWORD   ulFlagsSave;
    LRESULT  dw;
    UINT     w;
    HPALETTE hPal;
    LONG    lSize;

    //
    // Quick sanity checks....
    //
    if (lpbi == NULL)
	return FALSE;

    if ((pdd = DrawDibLock(hdd)) == NULL)
	return FALSE;

    DrawDibInit();

    //
    // fill in defaults.
    //
    if (dxSrc < 0)
        dxSrc = (int)lpbi->biWidth;

    if (dySrc < 0)
        dySrc = (int)lpbi->biHeight;

    if (dxDst < 0)
        dxDst = dxSrc;

    if (dyDst < 0)
        dyDst = dySrc;

    if (dxSrc == 0 || dySrc == 0)	// !!! || dxDst == 0 || dyDst == 0)
	return FALSE;

    ulFlagsSave = pdd->ulFlags;
    wFlagsChanged = ((UINT)pdd->ulFlags ^ wFlags);

    fSameHdc = hdc == pdd->hdcLast;
    fSameDib  = DibEq(pdd->lpbi, lpbi) && !(wFlagsChanged & DDF_HALFTONE) &&
		    (pdd->hpalDraw == pdd->hpalDrawLast);

    fSameFlags = (pdd->ulFlags & DDF_BEGINFLAGS) == (wFlags & DDF_BEGINFLAGS);
    fSameSize = pdd->dxDst == dxDst && pdd->dyDst == dyDst &&
                pdd->dxSrc == dxSrc && pdd->dySrc == dySrc;
    pdd->hdcLast = hdc;

    // When the hpalDraw field changes, we need to tell the compressor about it
    // by calling SendSetPalette.
    // !!! NO we don't

    //
    // Do a quick check to see if the params have changed.
    // If the DIB, and size of the DIB, and flags used are the same as the last
    // time we called DrawDibBegin, then there's nothing to do and we'll only
    // waste time plodding through this code.
    // There is one case when all these could be the same, but the situation
    // has still changed enough so that we need to recompute things... if the
    // hdc is different than last time, and we're dealing with RLE.  You see,
    // we make some decisions about RLE (like we can go direct to a screen DC
    // but not a memory DC) that are affected by what hdc we're using.  So
    // we will not bail out early if we are using RLE and the hdc's are
    // different.
    //
    if (fSameDib && fSameSize && fSameFlags)
    {
	if ((lpbi->biCompression != BI_RLE8 && lpbi->biCompression != BI_RLE4)
								|| fSameHdc)
	    return TRUE;
    }

    pdd->hpalDrawLast = pdd->hpalDraw;

    RPF(("DrawDibBegin %dx%dx%d '%4.4hs' [%d %d] [%d %d]",
	    (int)lpbi->biWidth,
	    (int)lpbi->biHeight,
	    (int)lpbi->biBitCount,
                (lpbi->biCompression == BI_RGB  ? (LPSTR)"None" :
                 lpbi->biCompression == BI_RLE8 ? (LPSTR)"Rle8" :
                 lpbi->biCompression == BI_RLE4 ? (LPSTR)"Rle4" :
                 (LPSTR)&lpbi->biCompression),
	    dxSrc, dySrc, dxDst, dyDst));

    fNewPal = pdd->hpal == NULL || !fSameDib;

    //
    // make sure this palette is not the in the DC, because we
    // are going to delete it, and GDI get real upset if we do this.
    //

    if (fNewPal && pdd->hpal && hdc)
    {
        hPal = SelectPalette(hdc, GetStockObject(DEFAULT_PALETTE), TRUE);

        if (hPal == pdd->hpal)
            RPF(("    Warning unselecting palette..."));
    }

    DrawDibFree(pdd, fSameDib, fSameSize);

    pdd->dxSrc = dxSrc;
    pdd->dySrc = dySrc;
    pdd->dxDst = dxDst;
    pdd->dyDst = dyDst;

    //
    // copy the source DIB header and the colors.
    //
    if (lpbi->biClrUsed == 0 && lpbi->biBitCount <= 8)
        lpbi->biClrUsed = (1 << (int)lpbi->biBitCount);

////if (lpbi->biClrUsed != 0 && lpbi->biBitCount > 8)
////    lpbi->biClrUsed = 0;

    // Make a copy of the source format.  Remember, some codec could have
    // defined a custom format larger than a BITMAPINFOHEADER so make a copy
    // of EVERYTHING.
    if (!fSameDib) {
	lSize = lpbi->biSize + lpbi->biClrUsed * sizeof(RGBQUAD);
	pdd->lpbi = (LPBITMAPINFOHEADER)GlobalAllocPtr(GPTR, lSize);
	if (pdd->lpbi == NULL)
	    return FALSE;
	_fmemcpy(pdd->lpbi, lpbi, (int)lSize);
	// This is where the colour info is
	pdd->lpargbqIn = (LPVOID)((LPBYTE)lpbi + lpbi->biSize);
    }

    pdd->biBuffer = *lpbi;

    pdd->lpbi->biSizeImage = 0;
    pdd->biBuffer.biSizeImage = 0;

    //
    // init all other color tables to be the initial colors
    //
    if (lpbi->biBitCount <= 8)
    {
        _fmemcpy(pdd->argbq,   (LPBYTE)lpbi+(int)lpbi->biSize,
		(int)lpbi->biClrUsed * sizeof(RGBQUAD));
	_fmemcpy(pdd->aw,      (LPBYTE)lpbi+(int)lpbi->biSize,
		(int)lpbi->biClrUsed * sizeof(RGBQUAD));
    }

    // set PalUse to default: DIB_PAL_COLORS. This will be set
    // to DIB_PAL_INDICES if DrawdibCheckPalette is called and detects
    // that it is safe to use indices. DIB_PAL_COLORS is a safe
    // default.

    pdd->uiPalUse = DIB_RGB_COLORS;     // assume RGB colors for now.

    //
    // make sure the device is a palette device before dinking with
    // palette animation.
    //
    if (wFlags & DDF_ANIMATE)
    {
	if (!(gwRasterCaps & RC_PALETTE) ||
            (int)lpbi->biBitCount > 8 || pdd->hpalDraw)
	    wFlags &= ~DDF_ANIMATE;
    }

    //
    // copy the flags
    //
try_again:
    pdd->ulFlags &= ~DDF_USERFLAGS;
    pdd->ulFlags |= (wFlags & DDF_USERFLAGS);

    pdd->ulFlags &= ~DDF_UPDATE;

    //
    // deal with a decompressor if needed.
    //
    switch (lpbi->biCompression)
    {
        case BI_RGB:
            break;

        default:
            //
            //  see if the DISPLAY/DISPDIB can draw the format directly!
            //
	    //  if the buffer flag is set we MUST use a decompress buffer.
	    //  regardless of what the display can do
	    //
	    if (wFlags & DDF_BUFFER)
		w = 0;
	    else
                w = QueryDraw(pdd, lpbi);

            if (w & PD_CAN_DRAW_DIB)
            {
                if (((dxSrc == dxDst && dySrc == dyDst) && (w & PD_STRETCHDIB_1_1_OK)) ||
                    ((dxSrc != dxDst || dySrc != dyDst) && (w & PD_STRETCHDIB_1_N_OK)) ||
                    ((dxDst % dxSrc) == 0 && (dyDst % dySrc) == 0 && (w & PD_STRETCHDIB_1_2_OK)))
                {
		    // GDI can't handle drawing RLEs to a memory DC so we will
		    // have to pretend that RLE can't be drawn to the screen
		    // and decompress it first.
		    // We also can't DITHER RLE, so if we're running on a
		    // 16 colour display, make sure we're using a decompressor.
		    if (((lpbi->biCompression != BI_RLE8) &&
				(lpbi->biCompression != BI_RLE4)) ||
			(hdc && IsScreenDC(hdc) && gwScreenBitDepth >=8))
		    {
                	wFlags |= DDF_JUSTDRAWIT;

                	if (pdd->hic)
                	    ICClose(pdd->hic);

                	pdd->hic = NULL;
                	goto no_decomp;
		    }
                }
            }

            if (pdd->hic == NULL)
            {
		DWORD fccHandler;

		fccHandler = 0;

		if (lpbi->biCompression == BI_RLE8)
		    fccHandler = mmioFOURCC('R','L','E',' ');
		
                pdd->hic = ICLocate(ICTYPE_VIDEO,
				    fccHandler,
				    lpbi, NULL,
				    ICMODE_FASTDECOMPRESS);

                if (pdd->hic == NULL)
		    pdd->hic = ICDecompressOpen(ICTYPE_VIDEO,
						fccHandler,lpbi,NULL);
		
		if (pdd->hic)
		{
		    //
		    //  make sure the codec uses its default palette out of the gate
		    //
		    if (ICDecompressSetPalette(pdd->hic, NULL) == ICERR_OK)
		    {
			pdd->ulFlags |= DDF_CANSETPAL;
			RPF(("    codec supports ICM_SET_PALETTE"));
		    }
		    else
		    {
			pdd->ulFlags &= ~DDF_CANSETPAL;
		    }
		}
	    }

            if (pdd->hic == NULL || pdd->hic == (HIC)-1)
            {
                RPF(("    Unable to open compressor '%4.4ls'",(LPSTR)&lpbi->biCompression));
		pdd->hic = (HIC)-1;

		if (wFlags & DDF_BUFFER)
		{
		    RPF(("   Turning DDF_BUFFER off"));
		    wFlags &= ~DDF_BUFFER;
		    goto try_again;
		}

                return FALSE;
            }

            //
	    //  now find the best DIB format to decompress to.
	    //
            if (!ICGetDisplayFormat(pdd->hic, lpbi, &pdd->biBuffer,
                   (gfHalftone || (wFlags & DDF_HALFTONE)) ? 16 : 0,
                   MulDiv(dxDst,abs((int)lpbi->biWidth),dxSrc),
                   MulDiv(dyDst,abs((int)lpbi->biHeight),dySrc)))
            {
                RPF(("    Compressor error!"));
codec_error:
		// ICClose(pdd->hic);
                // pdd->hic = (HIC)-1;
                return FALSE;
            }

            //
            // we have new source params
            //
            dxSrc = MulDiv(dxSrc, abs((int)pdd->biBuffer.biWidth),  (int)pdd->lpbi->biWidth);
            dySrc = MulDiv(dySrc, abs((int)pdd->biBuffer.biHeight), (int)pdd->lpbi->biHeight);
//          xSrc  = MulDiv(xSrc,  abs((int)pdd->biBuffer.biWidth),  (int)pdd->lpbi->biWidth);
//          ySrc  = MulDiv(ySrc,  abs((int)pdd->biBuffer.biHeight), (int)pdd->lpbi->biHeight);

            //
            // now allocate the decompress buffer!
            //
            pdd->biBuffer.biSizeImage = DIBSIZEIMAGE(pdd->biBuffer);

//Question: do we need to zeroinit the allocated buffer??
//SD
            pdd->pbBuffer = GlobalAllocPtr(GMEM_MOVEABLE,pdd->biBuffer.biSizeImage);
//          pdd->pbBuffer = GlobalAllocPtr(GHND,pdd->biBuffer.biSizeImage);

            if (pdd->pbBuffer == NULL)
            {
                RPF(("    No Memory for decompress buffer"));
                ICClose(pdd->hic);
		pdd->hic = (HIC)-1;
                return FALSE;
            }
            pdd->ulFlags |= DDF_DIRTY;

            dw = ICDecompressBegin(pdd->hic, lpbi, &pdd->biBuffer);

            if (dw != ICERR_OK)
            {
                RPF(("    Compressor failed ICM_DECOMPRESS_BEGIN"));
		goto codec_error;
            }

            RPF(("    Decompressing '%4.4hs' to %dx%dx%d%s",(LPSTR)&lpbi->biCompression, PUSHBI(pdd->biBuffer),
			     	Is565(&pdd->biBuffer) ? (LPSTR) "(565)" : (LPSTR) ""
	       ));
	    pdd->iDecompress = DECOMPRESS_BUFFER;
	    _fmemcpy(pdd->aw,pdd->argbq, 256*sizeof(RGBQUAD));
            lpbi = &pdd->biBuffer;
            break;
    }
no_decomp:
    pdd->biDraw = pdd->biBuffer;
    pdd->biDraw.biSizeImage = 0;

    pdd->biDraw.biHeight = abs((int)pdd->biDraw.biHeight);

    if ((!(wFlags & DDF_JUSTDRAWIT)) && (lpbi->biCompression == BI_RGB))
    {
        //
        //  test the display device for this DIB format
        //
        w = QueryDraw(pdd, lpbi);

        //
        // get the bit depth of the screen device.
        //
        ScreenBitDepth = gwScreenBitDepth;

        if (ScreenBitDepth > 24)
	    ScreenBitDepth = 32;        //???!!!

        // does the display support drawing 16bpp DIBs?
        // if it does not, treat it like a 24bpp device.

        if (ScreenBitDepth >= 24 && lpbi->biBitCount == 32 && !(w & PD_CAN_DRAW_DIB))
            ScreenBitDepth = 24;

        if (ScreenBitDepth >= 16 && lpbi->biBitCount == 16 && !(w & PD_CAN_DRAW_DIB))
            ScreenBitDepth = 24;

        //
        // check if the display driver sucks, for this format
        //
        if (!(w & PD_STRETCHDIB_1_1_OK))
        {
	    pdd->ulFlags |= DDF_BITMAP;
        }

        //
        //  if the display driver sucks make a bitmap to copy into
        //  to draw.
        //
        switch (gfBitmap)
        {
            case 0:
                pdd->ulFlags &= ~DDF_BITMAP;
                break;

            case 1:
                pdd->ulFlags |= DDF_BITMAP;
                break;
        }

#ifndef _WIN32	// !!! why only !WIN32?
	//
	// for 16/32 bit DIBs, the display may not support DIBS at all and
	// we should use bitmaps anyway just in case, even if the user
	// tried to override
	//
	if ((pdd->biDraw.biBitCount == 16 || pdd->biDraw.biBitCount == 32) &&
	    w == PD_CAN_DRAW_DIB)
	{
	    pdd->ulFlags |= DDF_BITMAP;
	}
#endif

        if ((dxSrc != dxDst || dySrc != dyDst) && !(w & PD_STRETCHDIB_1_N_OK))
	    pdd->ulFlags |= DDF_STRETCH;

        if (dxSrc*2 == dxDst && dySrc*2 == dyDst && (w & PD_STRETCHDIB_1_2_OK))
	    pdd->ulFlags &= ~DDF_STRETCH;

        if ((dxDst % dxSrc) == 0 && (dyDst % dySrc) == 0 && (w & PD_STRETCHDIB_1_2_OK))
	    pdd->ulFlags &= ~DDF_STRETCH;

        if ((int)lpbi->biBitCount > ScreenBitDepth) {
            DPF(("Turning on DITHER as bitcount is greater than screen bit depth"));
            pdd->ulFlags |= DDF_DITHER;
        }

        //
        //  force halftone palette
        //
        if ((gfHalftone || (wFlags & DDF_HALFTONE)) && ScreenBitDepth <= 8) {
            DPF(("Turning on DITHER because of halftoning\n"));
            pdd->ulFlags |= DDF_DITHER;
        }

        // NOTE we treat a convert up (ie 16->24) as a dither too.
        if ((int)lpbi->biBitCount > 8 && (int)lpbi->biBitCount < ScreenBitDepth) {
            DPF(("Turning on DITHER as bitcount does not match screen bit depth"));
            pdd->ulFlags |= DDF_DITHER;
        }

	if (pdd->ulFlags & DDF_DITHER) {
	    if (lpbi->biBitCount == 16 && (w & PD_CAN_DRAW_DIB)) {
	    pdd->ulFlags &= ~DDF_DITHER;
		DPF(("Turning off DITHER for 16-bit DIBs, since we can draw them"));
	    }

	    if (lpbi->biBitCount == 32 && (w & PD_CAN_DRAW_DIB)) {
		pdd->ulFlags &= ~DDF_DITHER;
		DPF(("Turning off DITHER for 32-bit DIBs, since we can draw them"));
	    }

	    if (lpbi->biBitCount == 8 &&
				lpbi->biClrUsed <= 16 &&
				AreColorsAllGDIColors(lpbi)) {
		pdd->ulFlags &= ~DDF_DITHER;
		DPF(("Turning off DITHER for 8-bit DIBs already using the VGA colors"));
	    }
	}

	// force stretching in drawdib if we are dithering
	if ((pdd->ulFlags & DDF_DITHER) &&
	    ((dxSrc != dxDst) || (dySrc != dyDst))) {
		pdd->ulFlags |= DDF_STRETCH;
	}

        //
        // Force a buffer if we dont have one.  We only buffer if we're
	// decompressing or dithering or stretching or using bitmaps, so our
	// hacky way of forcing a buffer is to pretend we're stretching 1:1.
        //
        if ((pdd->ulFlags & DDF_BUFFER) &&
            pdd->hic == NULL &&
            !(pdd->ulFlags & DDF_DITHER) &&
            !(pdd->ulFlags & DDF_STRETCH) &&
            !(pdd->ulFlags & DDF_BITMAP))
        {
            RPF(("    Using a buffer because DDF_BUFFER is set."));
            pdd->ulFlags |= DDF_STRETCH;    // force a 1:1 stretch
        }

	if (lpbi->biBitCount != 8
		    && lpbi->biBitCount != 16
		    && lpbi->biBitCount != 24
#ifndef _WIN32
		    && lpbi->biBitCount != 32
#endif
	    ) {
            DPF(("Turning off stretch for an unsupported format...."));
            pdd->ulFlags &= ~(DDF_STRETCH);
        }

    }

    //
    // delete the palette if we are changing who dithers.
    //
    if (pdd->hpal &&
        pdd->lpbi->biBitCount > 8 &&
        ((pdd->ulFlags ^ ulFlagsSave) & (DDF_DITHER)))
    {
        DPF(("    Dither person has changed..."));

        if (pdd->lpDitherTable)
        {
            DitherTerm(pdd->lpDitherTable);
	    pdd->lpDitherTable = NULL;
	}

        if (hdc) {
            hPal = SelectPalette(hdc, GetStockObject(DEFAULT_PALETTE), TRUE);

            if (hPal == pdd->hpal)
	        RPF(("    Warning unselecting palette..."));
        }

        DeleteObject(pdd->hpal);
	if (pdd->hpalCopy)
            DeleteObject(pdd->hpalCopy);
        pdd->hpal = NULL;
        pdd->hpalCopy = NULL;
    }

    if (pdd->ulFlags & DDF_STRETCH)
    {
        /* the code for stretching *only* works on a 386+ */
        if (gf286 || pdd->biBuffer.biBitCount < 8)
        {
            RPF(("    Using GDI to stretch"));
	    pdd->ulFlags &= ~DDF_STRETCH;
        }
        else
        {
            //
            // we have stretching to do, this requires extra
            // headers and buffers.
            //
            pdd->biStretch = pdd->biBuffer;
            pdd->biStretch.biWidth = dxDst;
            pdd->biStretch.biHeight = dyDst;
            pdd->biStretch.biSizeImage = DIBSIZEIMAGE(pdd->biStretch);

            pdd->pbStretch = GlobalAllocPtr(GHND,pdd->biStretch.biSizeImage);

            if (pdd->pbStretch == NULL)
            {
                RPF(("    No memory for stretch buffer, using GDI"));
		pdd->ulFlags &= ~DDF_STRETCH;
            }
            else
            {
                RPF(("    Stretching %dx%dx%d%s --> %dx%dx%d",
			    dxSrc, dySrc, (int)lpbi->biBitCount,
			    (LPSTR) (Is565(lpbi) ? "(565)":""),
			    dxDst, dyDst, (int)pdd->biStretch.biBitCount,
			    (LPSTR) (Is565(&pdd->biStretch) ? "(565)":"")
		   ));
                pdd->biDraw.biWidth = dxDst;
                pdd->biDraw.biHeight = dyDst;
                dxSrc = dxDst;
                dySrc = dyDst;
                lpbi = &pdd->biStretch;
            }
        }
    }

    if (pdd->ulFlags & DDF_DITHER)
    {
	pdd->ulFlags &= ~DDF_ANIMATE;        // cant  animate and dither!

        if (ScreenBitDepth <= 8)
            pdd->biDraw.biBitCount = 8;
        else if (lpbi->biBitCount <= 8)
            pdd->biDraw.biBitCount = lpbi->biBitCount;
        else
////////////pdd->biDraw.biBitCount = 24; //!!! what about 16bit DIB support
            pdd->biDraw.biBitCount = ScreenBitDepth;

        w = QueryDraw(pdd, &pdd->biDraw);

        if (w & PD_STRETCHDIB_1_1_OK)
	    pdd->ulFlags &= ~DDF_BITMAP;
        else
	    pdd->ulFlags |= DDF_BITMAP;

	// this is wrong isn't it ? biDraw will be set to
	// dxDst if we are stretching, or dxSrc if not. If we are asked to
	// dither and stretch together, and we choose to leave the stretching
	// to GDI, we will here set biDraw so that we ask the dither code
	// to dither from dx/dySrc to dx/dyDst - ie stretch and dither in
	// one go. Our current dither code will crash if you ask it to do this.
        dxSave = (int)pdd->biDraw.biWidth;
        dySave = (int)pdd->biDraw.biHeight;
#if 0
        pdd->biDraw.biWidth  = lpbi->biWidth;
        pdd->biDraw.biHeight = lpbi->biHeight;
#endif

	// !!! So DrawDibDraw will not DebugBreak
	pdd->biDraw.biWidth = dxSrc;
	pdd->biDraw.biHeight = dySrc;
        pdd->biDraw.biSizeImage = DIBSIZEIMAGE(pdd->biDraw);

        RPF(("    Dithering %dx%dx%d --> %dx%dx%d", PUSHBI(*lpbi), PUSHBI(pdd->biDraw)));

        //
        //  NOTE we need to use &pdd->biBuffer *not* lpbi because in the
        //  stretched case lpbi will point to pdd->biStretch and biStretch
        //  has NO COLOR TABLE
        //
        pdd->lpDitherTable = DitherInit(&pdd->biBuffer, &pdd->biDraw,
            &pdd->DitherProc, pdd->lpDitherTable);

        if (pdd->lpDitherTable == (LPVOID)-1 ||
            pdd->DitherProc == NULL ||
            !(pdd->pbDither = GlobalAllocPtr(GHND,pdd->biDraw.biSizeImage)))
        {
            if (pdd->lpDitherTable == (LPVOID)-1)
                pdd->lpDitherTable = NULL;

            if (pdd->lpDitherTable)
                DitherTerm(pdd->lpDitherTable);

	    if ((pdd->pbDither) && (pdd->pbDither != pdd->lpDIBSection))
                GlobalFreePtr(pdd->pbDither);

            pdd->lpDitherTable = NULL;
            pdd->pbDither = NULL;
            pdd->biDraw.biBitCount = pdd->biBuffer.biBitCount;
            pdd->biDraw.biWidth  = dxSave;
            pdd->biDraw.biHeight = dySave;
            pdd->biDraw.biSizeImage = 0;
	    pdd->ulFlags &= ~DDF_DITHER;

#ifdef DEBUG_RETAIL
            if (pdd->DitherProc)
                RPF(("    No Memory for dither tables!"));
            else
                RPF(("    No DitherProc!"));
#endif
        }
    }

    //
    // create a palette (if needed)
    //
    if ((gwRasterCaps & RC_PALETTE) &&
	pdd->biDraw.biBitCount <= 8 &&
        pdd->hpal == NULL)
    {
	pdd->hpal = CreateBIPalette(pdd->hpal, &pdd->biDraw);
        pdd->ulFlags |= DDF_NEWPALETTE;
    }

    //
    // make sure we treat the palette as new when starting/stopping animate
    //
    if (wFlagsChanged & DDF_ANIMATE)
    {
        pdd->ulFlags |= DDF_NEWPALETTE;
    }

    //
    // check for a identity palette
    //
    if (pdd->hpal == NULL)
    {
        pdd->ClrUsed = 0;
    }
    else if (pdd->ulFlags & DDF_NEWPALETTE)
    {
#ifdef _WIN32
        if (HIWORD(pdd->ClrUsed!=0)) {
            DPF(("Hiword of variable non zero before calling GetObject\n"));
        }
#endif
        GetObject(pdd->hpal,sizeof(int),(LPVOID)&pdd->ClrUsed);

        if (wFlagsChanged & DDF_ANIMATE)
            SetPalFlags(pdd->hpal,0,pdd->ClrUsed,0);

	if (IsIdentityPalette(pdd->hpal))
	{
	    pdd->ulFlags |= DDF_IDENTITYPAL;
	    pdd->iAnimateStart = 10;
	}
	else
	{
	    pdd->ulFlags &= ~DDF_IDENTITYPAL;
	    pdd->iAnimateStart = 0;
	}

	pdd->iAnimateLen = min(236,pdd->ClrUsed);
	pdd->iAnimateEnd = pdd->iAnimateStart + pdd->iAnimateLen;

	if (pdd->ulFlags & DDF_ANIMATE)
        {
            RPF(("    Palette animation"));
            SetPalFlags(pdd->hpal,pdd->iAnimateStart,pdd->iAnimateLen,PC_RESERVED);
        }
    }

    //
    // because of bugs in GDIs StretchDIBits (doing a stretch) we
    // always set the number of colors to be the maximum.
    //
    // this is not a big deal because we are mostly drawing full-color-table
    // DIBs any way.
    //
    if (pdd->biDraw.biBitCount <= 8)
        pdd->biDraw.biClrUsed = (1 << (int)pdd->biDraw.biBitCount);
    else
        pdd->biDraw.biClrUsed = 0;

    DrawDibSetPalette(hdd, pdd->hpalDraw);

    if (pdd->hpal)
    {
        if (pdd->ulFlags & DDF_IDENTITYPAL)
            RPF(("    Drawing with an identity palette"));
        else
            RPF(("    Drawing with a non-identity palette"));
    }

    if (pdd->uiPalUse == DIB_RGB_COLORS)
	RPF(("    Using DIB_RGB_COLORS"));
    else
	RPF(("    Using DIB_PAL_COLORS"));

    if (pdd->hpalDraw)
        RPF(("    Mapping to another palette"));

    if (pdd->ulFlags & DDF_BITMAP)
    {
        BOOL fGetDC;
        BOOL f;
        HWND hwndActive;

        RPF(("    Display driver slow for DIBs, using bitmaps"));

        if (fGetDC = (hdc == NULL))
        {
            hwndActive = GetActiveWindow();
            hdc = GetDC(hwndActive);
        }

        if (pdd->hdcDraw) {
	    if (hbmStockMono) {
		SelectObject(pdd->hdcDraw, hbmStockMono);
	    }
	} else /* if (!pdd->hdcDraw) */ {
	    pdd->hdcDraw = CreateCompatibleDC(hdc);
	}

        if (pdd->hbmDraw) {
	    // This fixes a memory leak.  Perhaps we can just use the old one?
	    DPF(("Freeing hbmDraw!\n"));
            DeleteObject(pdd->hbmDraw);
	}

        //
        // NOTE the bitmap must be as wide as the source DIB when we are
        // using SetDIBits() because SetDIBits() only takes a start scan not a (x,y)
        //
//      pdd->hbmDraw = CreateCompatibleBitmap(hdc, (int)pdd->biDraw.biWidth, (int)pdd->biDraw.biHeight);
        pdd->hbmDraw = CreateCompatibleBitmap(hdc, (int)pdd->biDraw.biWidth, dySrc);

        if (pdd->hbmDraw == NULL || pdd->hdcDraw == NULL)
            goto bitmap_fail;

        hbmStockMono = SelectObject(pdd->hdcDraw,pdd->hbmDraw);

        pdd->ulFlags |= DDF_NEWPALETTE;

#if USE_SETDI
        f = SetBitmapBegin(
		    &pdd->sd,       //  structure
		    hdc,            //  device
		    pdd->hbmDraw,   //  bitmap to set into
		    &pdd->biDraw,   //  --> BITMAPINFO of source
                    pdd->uiPalUse);
#else
        f = TRUE;
#endif
        if (!f)
        {
bitmap_fail:
            if (pdd->hdcDraw) {
                DeleteDC(pdd->hdcDraw);
		pdd->hdcDraw = NULL;
	    }

            if (pdd->hbmDraw) {
                DeleteObject(pdd->hbmDraw);
		pdd->hbmDraw = NULL;
	    }

            pdd->ulFlags &= ~DDF_BITMAP;
        }

        if (fGetDC)
        {
            ReleaseDC(hwndActive, hdc);
            hdc = NULL;
        }
    }

    //
    // Use CreateDibSection unless we're in VGA mode, or we're not
    // decompressing, stretching, or dithering.
    //
    if (ScreenBitDepth > 4 &&
		(pdd->hic || (pdd->ulFlags & (DDF_STRETCH|DDF_DITHER)))) {
	BOOL fGetDC;
	HWND hwndActive;
	HPALETTE hpalOld = NULL;

	if (pdd->hbmDraw) {
	    // !!! Shouldn't we really not delete these until after we know
	    // we can use CreateDIBSection?
	    SelectObject(pdd->hdcDraw, hbmStockMono);
	    DeleteObject(pdd->hbmDraw);
	    DeleteDC(pdd->hdcDraw);
	    pdd->hdcDraw = NULL;
	    pdd->hbmDraw = NULL;
	}

	if (fGetDC = (hdc == NULL))
	{
	    hwndActive = GetActiveWindow();
	    hdc = GetDC(hwndActive);
	}

	if (pdd->hpalDraw || pdd->hpal) {
	    hpalOld = SelectPalette(hdc,
				    pdd->hpalDraw ? pdd->hpalDraw :
						    pdd->hpal,
				    TRUE);
	}
						
	pdd->hbmDraw = CreateDIBSection(
				hdc,
				(LPBITMAPINFO)&pdd->biDraw,
                                // we don't want to use DIB_PAL_INDICES for create dib section
                                (pdd->uiPalUse == DIB_RGB_COLORS) ? DIB_RGB_COLORS : DIB_PAL_COLORS,
				&pdd->lpDIBSection,
				0,	// handle to section
				0);	// offset within section

	pdd->hdcDraw = CreateCompatibleDC(hdc);

	if ((pdd->hdcDraw == NULL) ||
			(pdd->hbmDraw == NULL) ||
	    (pdd->lpDIBSection == NULL)) {

		if (pdd->hdcDraw)
		    DeleteDC(pdd->hdcDraw);

		if (pdd->hbmDraw)
		    DeleteObject(pdd->hbmDraw);

		pdd->lpDIBSection = NULL;
		pdd->hdcDraw = NULL;
		pdd->hbmDraw = NULL;

		RPF(("CreateDIBSection FAILED"));

	} else {
	    hbmStockMono = SelectObject(pdd->hdcDraw,pdd->hbmDraw);

	    // make sure we decomp, stretch or dither into the right place

	    if (pdd->pbDither) {
		GlobalFreePtr(pdd->pbDither);
		pdd->pbDither = pdd->lpDIBSection;
	    } else if (pdd->pbStretch) {
		GlobalFreePtr(pdd->pbStretch);
		pdd->pbStretch = pdd->lpDIBSection;
	    } else if (pdd->pbBuffer) {
		GlobalFreePtr(pdd->pbBuffer);
		pdd->pbBuffer = pdd->lpDIBSection;
	    }
	}

	if (fGetDC)
	{
	    if (hpalOld)
		SelectPalette(hdc, hpalOld, FALSE);
	    ReleaseDC(hwndActive, hdc);
	    hdc = NULL;
	}

    } else {

    // We are NOT using DIBSection.  We might have an old one sitting around,
    // so clear it out.  The reason we might have an old one sitting around is
    // that deciding whether or not to try to draw RLE directly or to decompress
    // it first to RGB (based on if our DC is a screen DC or not - because of a
    // GDI bug that can't draw RLE deltas to memory DCs) can affect whether or
    // not we use DIBSections, and our code at the beginning of DrawDibBegin is
    // not smart enough to call DrawDibFree in this case to clear this stuff
    // out.  I hate this code. - DannyMi

    // NOTE:  This code appears at least 3 times in this file.

        if (pdd->hdcDraw) {
	    if (hbmStockMono)
		SelectObject(pdd->hdcDraw, hbmStockMono);

            DeleteDC(pdd->hdcDraw);
	    pdd->hdcDraw = NULL;
	}

        if (pdd->hbmDraw) {
            DeleteObject(pdd->hbmDraw);
	    pdd->hbmDraw = NULL;
        }

	// I have to throw away these if I'm throwing away the DIB section, so
	// we don't end up trying to free it twice.
	if (pdd->pbDither == pdd->lpDIBSection)
	    pdd->pbDither = NULL;
	if (pdd->pbStretch == pdd->lpDIBSection)
	    pdd->pbStretch = NULL;
	if (pdd->pbBuffer == pdd->lpDIBSection)
	    pdd->pbBuffer = NULL;

	pdd->lpDIBSection = NULL;
    }

    //
    //  now try to decompress to a bitmap, we only decompress to
    //  bitmaps if the following is true.
    //
    //      the decompressor must decompress direct, we will not
    //      stretch/dither afterward
    //
    //      if on a palette device, the color table must be 1:1
    //
    //  we should check a decompressor flag
    //
    if (pdd->hic &&
	!(pdd->ulFlags & (DDF_STRETCH|DDF_DITHER)) &&
        gfBitmapX &&
	(pdd->lpDIBSection == NULL) &&
        (dxDst == pdd->lpbi->biWidth) &&
        (dyDst == pdd->lpbi->biHeight)
        )
    {

        if (pdd->ulFlags & DDF_BITMAP) {
            if (pdd->hbmDraw) {
                DrawDibQueryBitmapX(pdd);
            }
        } else {

            //even though we decided not to use bitmaps, it might still
            //be worth trying decompression to bitmaps. the DDF_BITMAP
            //flag is based on a comparison of StretchDIBits vs
            //SetDIBits+BitBlt. Decompressing to a bitmap could be
            //faster even when SetDIBits+Bitblt is slower
            // but in this case, if we fail, we have to make sure we don't
            // end up doing DDF_BITMAP as we know that's slower.

            if (QueryDraw(pdd, &pdd->biBuffer) & PD_BITBLT_FAST) {

                BOOL fGetDC;
                HWND hwndActive;

                RPF(("    Not using BITMAPS, but trying Decomp to Bitmap"));

                if (fGetDC = (hdc == NULL))
                {
                    hwndActive = GetActiveWindow();
                    hdc = GetDC(hwndActive);
                }

                pdd->hdcDraw = CreateCompatibleDC(hdc);
                pdd->hbmDraw = CreateCompatibleBitmap(hdc, (int)pdd->biDraw.biWidth, (int)pdd->biDraw.biHeight);

                if ((pdd->hbmDraw != NULL) && (pdd->hdcDraw != NULL)) {

                    hbmStockMono = SelectObject(pdd->hdcDraw,pdd->hbmDraw);

                    if (fGetDC)
                    {
                        ReleaseDC(hwndActive, hdc);
                        hdc = NULL;
                    }

                    DrawDibQueryBitmapX(pdd);
                }

                if (!(pdd->ulFlags & DDF_CANBITMAPX)) {
                        if (pdd->hdcDraw) {
                            DeleteDC(pdd->hdcDraw);
			    pdd->hdcDraw = NULL;
			}

                        if (pdd->hbmDraw) {
                            DeleteObject(pdd->hbmDraw);
			    pdd->hbmDraw = NULL;
			}
                }
            }
        }
    }

#ifdef USE_DCI
    //
    //  see if the decompressor can decompress directly to the screen
    //  doing everything, stretching and all.
    //
    if (pdd->hic && pdci && gfScreenX)
    {
        if (wFlags & DDF_BUFFER)
        {
            DPF(("    DDF_BUFFER specified, unable to decompres to screen"));
            goto cant_do_screen;
        }

	//
	// try to decompress to screen.
	//
        if (((gwRasterCaps & RC_PALETTE) && !(pdd->ulFlags & DDF_IDENTITYPAL)) ||
            (gwScreenBitDepth == 8 && !(gwRasterCaps & RC_PALETTE)) ||
            (pdd->ulFlags & (DDF_STRETCH|DDF_DITHER)) ||
            (ICDecompressExQuery(pdd->hic, 0,
				pdd->lpbi, NULL, 0, 0, pdd->dxSrc, pdd->dySrc,
				(LPBITMAPINFOHEADER) &biScreen, lpScreen,
				0, 0, pdd->dxDst, pdd->dyDst) != ICERR_OK))
	{
cant_do_screen:
	    ; // we can't decompress to the screen
	}
	else
	{   // we can decompress to the screen
	    pdd->ulFlags |= DDF_CLIPCHECK;  // we need clipping checking
	    pdd->ulFlags |= DDF_NEWPALETTE; // force check in DrawDibRealize
	    pdd->ulFlags |= DDF_CANSCREENX; // we can decompress to screen
	    pdd->ulFlags |= DDF_CLIPPED;    // we are initialized for clipped now

	    RPF(("    Can decompress '%4.4hs' to the SCREEN",(LPSTR)&pdd->lpbi->biCompression));
	}
    }

#ifdef WANT_DRAW_DIRECT_TO_SCREEN
    //
    //  see if we can draw direct to the screen
    //
    if (pdd->hic && pdci && gfDrawX)
    {
        if (TRUE)
            goto cant_draw_screen;

        pdd->ulFlags |= DDF_CLIPCHECK;  // we need clipping checking
        pdd->ulFlags |= DDF_NEWPALETTE; // force check in DrawDibRealize
        pdd->ulFlags |= DDF_CANDRAWX;   // we can decompress to screen
        pdd->ulFlags |= DDF_CLIPPED;    // we are initialized for clipped now

        RPF(("    Can draw to the SCREEN"));

cant_draw_screen:
        ;
    }
#endif

#endif

    //
    // see if the source cordinates need translated
    //
    if (abs((int)pdd->biBuffer.biWidth)  != (int)pdd->lpbi->biWidth ||
        abs((int)pdd->biBuffer.biHeight) != (int)pdd->lpbi->biHeight)
    {
        pdd->ulFlags |= DDF_XLATSOURCE;
    }

    return TRUE;
}

/**************************************************************************
* @doc EXTERNAL DrawDib
*
* @api BOOL | DrawDibEnd | This function frees a DrawDib context.
*
* @parm HDRAWDIB | hdd | Specifies the handle to the DrawDib context to free.
*
* @rdesc Returns TRUE if successful.
*
* @comm Any flags set or palette changes made by <f DrawDibBegin> or
*       <f DrawDibDraw> is discarded by <f DrawDibEnd>.
*
**************************************************************************/
BOOL VFWAPI DrawDibEnd(HDRAWDIB hdd)
{
    PDD pdd;

    if ((pdd = DrawDibLock(hdd)) == NULL)
        return FALSE;

    DrawDibFree(pdd, FALSE, FALSE);

    return TRUE;
}

/**************************************************************************
* @doc EXTERNAL DrawDib
*
* @api BOOL | DrawDibTime | Returns timing information about
*       the drawing during debug operation.
*
* @parm HDRAWDIB | hdd | Specifies a handle to a DrawDib context.
*
* @parm LPDRAWDIBTIME | lpddtime | Specifies a pointer to
*       a <t DRAWDIBTIME> structure.
*
* @rdesc Returns TRUE if successful.
*
**************************************************************************/
BOOL VFWAPI DrawDibTime(HDRAWDIB hdd, LPDRAWDIBTIME lpddtime)
{
#ifdef DEBUG_RETAIL
    PDD pdd;

    if ((pdd = DrawDibLockNoTaskCheck(hdd)) == NULL)
        return FALSE;

    if (lpddtime)
        *lpddtime = pdd->ddtime;

    if (pdd->ddtime.timeCount > 0)
    {
        RPF(("timeCount:       %u",        (UINT)pdd->ddtime.timeCount));
        RPF(("timeDraw:        %ums (%u)", (UINT)pdd->ddtime.timeDraw, (UINT)pdd->ddtime.timeDraw/(UINT)pdd->ddtime.timeCount));
        RPF(("timeDecompress:  %ums (%u)", (UINT)pdd->ddtime.timeDecompress, (UINT)pdd->ddtime.timeDecompress/(UINT)pdd->ddtime.timeCount));
        RPF(("timeDither:      %ums (%u)", (UINT)pdd->ddtime.timeDither, (UINT)pdd->ddtime.timeDither/(UINT)pdd->ddtime.timeCount));
        RPF(("timeStretch:     %ums (%u)", (UINT)pdd->ddtime.timeStretch, (UINT)pdd->ddtime.timeStretch/(UINT)pdd->ddtime.timeCount));
        RPF(("timeSetDIBits:   %ums (%u)", (UINT)pdd->ddtime.timeSetDIBits, (UINT)pdd->ddtime.timeSetDIBits/(UINT)pdd->ddtime.timeCount));
        RPF(("timeBlt:         %ums (%u)", (UINT)pdd->ddtime.timeBlt, (UINT)pdd->ddtime.timeBlt/(UINT)pdd->ddtime.timeCount));
    }

    pdd->ddtime.timeCount      = 0;
    pdd->ddtime.timeDraw       = 0;
    pdd->ddtime.timeDecompress = 0;
    pdd->ddtime.timeDither     = 0;
    pdd->ddtime.timeStretch    = 0;
    pdd->ddtime.timeSetDIBits  = 0;
    pdd->ddtime.timeBlt        = 0;

    return TRUE;
#else
    return FALSE;
#endif
}


/*
 * CopyPal -- copy a palette
 */
HPALETTE CopyPal(HPALETTE hpal)
{
    NPLOGPALETTE    pLogPal = NULL;
    HPALETTE        hpalNew = NULL;
    int             iSizePalette = 0;       // size of entire palette

    if (hpal == NULL)
	return NULL;

    DPF(("CopyPal routine\n"));
    GetObject(hpal,sizeof(iSizePalette),(LPSTR)&iSizePalette);

    pLogPal = (NPLOGPALETTE)LocalAlloc(LPTR, sizeof(LOGPALETTE)
            + iSizePalette * sizeof(PALETTEENTRY));

    if (!pLogPal)
	return NULL;

    pLogPal->palVersion = 0x300;
    pLogPal->palNumEntries = (WORD) iSizePalette;

    GetPaletteEntries(hpal, 0, iSizePalette, pLogPal->palPalEntry);

    hpal = CreatePalette(pLogPal);

    LocalFree((HLOCAL) pLogPal);

    return hpal;
}

/**************************************************************************
* @doc EXTERNAL DrawDib
*
* @api HPALETTE | DrawDibGetPalette | This function obtains the palette
*      used by a DrawDib context.
*
* @parm HDRAWDIB | hdd | Specifies a handle to a DrawDib context.
*
* @rdesc Returns a handle for the palette if successful, otherwise
*        it returns NULL.
*
* @comm Use <f DrawDibRealize> instead of this function
*       to realize the correct palette in response to a window
*       message. You should rarely need to call this function.
*
*       Applications do not have exclusive use of the palette
*       obtained with this function. Applications should not
*       free the palette or assign it to a display context
*       with functions such as <f SelectPalette>. Applications
*       should also anticipate that some other
*       application can invalidate the handle. The palette
*       handle might also become invalid after the next use of a DrawDib function.
*
*       This function returns a valid handle only after
*       <f DrawDibBegin> has been used without pairing it with
*       <f DrawDibEnd>, or if <f DrawDibDraw> has been used.
*
* @xref <f DrawDibSetPalette> <f DrawDibRealize>
*
**************************************************************************/
HPALETTE VFWAPI DrawDibGetPalette(HDRAWDIB hdd)
{
    PDD pdd;

    if ((pdd = DrawDibLockNoTaskCheck(hdd)) == NULL)
        return NULL;

    if (pdd->hpalDraw)
        return pdd->hpalDraw;
    else {
	// For palette animation we can't return a different palette than the
	// real palette, so return hpal, not hpalCopy.
        // Just trust me.  - Toddla

        if (pdd->ulFlags & DDF_ANIMATE)
            return pdd->hpal;

	// In order for us to play direct to screen, etc, all palette
	// realization has to come through DrawDibRealize.  But that won't
	// always happen.  Some apps will always ask us for our palette and
	// realize it themselves.  So if we give them a copy of our palette,
	// and never our true palette, when our play code realizes the true
	// palette it's guarenteed to cause an actual palette change and we'll
        // correctly detect playing to screen. (BUG 1761)

	if (pdd->hpalCopy == NULL)
            pdd->hpalCopy = CopyPal(pdd->hpal);

	return pdd->hpalCopy;
    }
}

/**************************************************************************
* @doc EXTERNAL DrawDib
*
* @api BOOL | DrawDibSetPalette | This function sets the palette
*      used for drawing device independent bitmaps.
*
* @parm HDRAWDIB | hdd | Specifies a handle to a DrawDib context.
*
* @parm HPALETTE | hpal | Specifies a handle to the palette.
*       Specify NULL to use the default palette.
*
* @rdesc Returns TRUE if successful.
*
* @comm Use this function when the application needs to realize an
*   alternate palette. The function forces the DrawDib context to use the
*   specified palette, possibly at the expense of image quality.
*
*   Do not free a palette assigned to a DrawDib context until
*   either a new palette replaces it (for example, if hpal1 is the
*   current palette, replacing it with DrawDibSetPalette(hdd, hpal2)),
*   or until the palette handle for the DrawDib context is set to
*   to the default palette (for example, DrawDibSetPalette(hdd, NULL)).
*
* @xref <f DrawDibGetPalette>
*
**************************************************************************/
BOOL VFWAPI DrawDibSetPalette(HDRAWDIB hdd, HPALETTE hpal)
{
    PDD pdd;
    int i;

    if ((pdd = DrawDibLock(hdd)) == NULL)
        return FALSE;

    if (hpal == pdd->hpalCopy)
        hpal = NULL;

    if (pdd->hpalDraw != hpal)
        pdd->ulFlags |= DDF_NEWPALETTE;

    pdd->hpalDraw = hpal;       // always set this variable

    if (pdd->hpal == NULL)      // no palette to dink with
	return TRUE;

    if (pdd->biDraw.biBitCount > 8) // make sure we are drawing palettized
        return TRUE;

    if (pdd->ulFlags & DDF_ANIMATE)
    {
        DPF(("DrawDibSetPalette called while in DDF_ANIMATE mode!"));
    }

    //
    //  we are now using PAL colors...
    //
    pdd->uiPalUse = DIB_PAL_COLORS;

    if (pdd->hpalDraw != NULL)
    {
        /* Set up table for BI_PAL_COLORS non 1:1 drawing */

        //
        //  map all of our colors onto the given palette
        //  NOTE we can't use the select background trick
        //  because the given palette <hpalDraw> may have
        //  PC_RESERVED entries in it.
        //
	// SendSetPalette(pdd);
	
        for (i=0; i < 256; i++)
        {
	    if (pdd->biBuffer.biBitCount == 8)
	    {
		pdd->aw[i] = (WORD) GetNearestPaletteIndex(pdd->hpalDraw,
						    RGB(pdd->argbq[i].rgbRed,
							pdd->argbq[i].rgbGreen,
							pdd->argbq[i].rgbBlue));
	    }
	    else
	    {
		PALETTEENTRY pe;
		GetPaletteEntries(pdd->hpal, i, 1, &pe);
		pdd->aw[i] = (WORD) GetNearestPaletteIndex(pdd->hpalDraw,
						    RGB(pe.peRed,
							pe.peGreen,
							pe.peBlue));
	    }
        }

        for (; i<256; i++)
            pdd->aw[i] = 0;
    }
    else
    {
        /* Set up table for BI_PAL_COLORS 1:1 drawing */

	// SendSetPalette(pdd);
        for (i=0; i<(int)pdd->ClrUsed; i++)
            pdd->aw[i] = (WORD) i;

        for (; i<256; i++)
            pdd->aw[i] = 0;
    }

    return TRUE;
}

/**************************************************************************
* @doc EXTERNAL DrawDib
*
* @api BOOL | DrawDibChangePalette | This function sets the palette entries
*      used for drawing device independent bitmaps.
*
* @parm HDRAWDIB | hdd | Specifies a handle to a DrawDib context.
*
* @parm int | iStart | Specifies the starting palette entry number.
*
* @parm int | iLen | Specifies the number of palette entries.
*
* @parm LPPALETTEENTRY | lppe | Specifies a pointer to an
*       array of palette entries.
*
* @rdesc Returns TRUE if successful.
*
* @comm
*   Use this function when the DIB color table changes and
*   other parameters stay constant. This function changes
*   the physical palette only if the current
*   DrawDib palette is curently realized by calling <f DrawDibRealize>.
*
*   The DIB color table must be changed by the user or
*   the next use of <f DrawDibDraw> without the DDF_SAME_DRAW flag
*   implicity calls <f DrawDibBegin>.
*
*   If the DDF_ANIMATE flag is not set in the previous call to
*   <f DrawDibBegin> or <f DrawDibDraw>, this function will
*   animate the palette. In this case, update the DIB color
*   table from the palette specified by <p lppe> and use
*   <f DrawDibRealize> to realize the updated palette. Redraw
*   the image to see the updated colors.
*

* @xref <f DrawDibSetPalette> <f DrawDibRealize>
*
**************************************************************************/
BOOL VFWAPI DrawDibChangePalette(HDRAWDIB hdd, int iStart, int iLen, LPPALETTEENTRY lppe)
{
    PDD pdd;
    int i;

    int iStartSave;
    int iLenSave;
    LPPALETTEENTRY lppeSave;

    if ((pdd = DrawDibLock(hdd)) == NULL)
        return FALSE;

    if (pdd->biBuffer.biBitCount != 8)
        return FALSE;

    if (lppe == NULL || iStart < 0 || iLen + iStart > 256)
        return FALSE;

    for (i=0; i<iLen; i++)
    {
	(*(pdd->lpargbqIn))[iStart+i].rgbRed   = lppe[i].peRed;
	(*(pdd->lpargbqIn))[iStart+i].rgbGreen = lppe[i].peGreen;
	(*(pdd->lpargbqIn))[iStart+i].rgbBlue  = lppe[i].peBlue;
    }

    //
    // handle a palette change for 8bit dither
    //
    if (pdd->lpDitherTable)
    {
        for (i=0; i<iLen; i++)
        {
            pdd->argbq[iStart+i].rgbRed   = lppe[i].peRed;
            pdd->argbq[iStart+i].rgbGreen = lppe[i].peGreen;
            pdd->argbq[iStart+i].rgbBlue  = lppe[i].peBlue;
        }

        pdd->lpDitherTable = DitherInit(pdd->lpbi, &pdd->biDraw, &pdd->DitherProc, pdd->lpDitherTable);
    }
    else if (pdd->hpalDraw)
    {
        SetPaletteEntries(pdd->hpal, iStart, iLen, lppe);
        pdd->ulFlags |= DDF_NEWPALETTE;

        for (i=iStart; i<iLen; i++)
        {
            pdd->aw[i] = (WORD) GetNearestPaletteIndex(pdd->hpalDraw,
                RGB(lppe[i].peRed,lppe[i].peGreen,lppe[i].peBlue));
        }
    }
    else if (pdd->ulFlags & DDF_ANIMATE)
    {
        for (i=iStart; i<iStart+iLen; i++)
        {
            if (i >= pdd->iAnimateStart && i < pdd->iAnimateEnd)
                lppe[i-iStart].peFlags = PC_RESERVED;
            else
                lppe[i-iStart].peFlags = 0;
        }

        /* Change iLen, iStart so that they only include the colors
        ** we can actually animate.  If we don't do this, the
        ** AnimatePalette() call just returns without doing anything.
        */

        iStartSave = iStart;
        iLenSave   = iLen;
        lppeSave   = lppe;

        if (iStart < pdd->iAnimateStart)
        {
            iLen -= (pdd->iAnimateStart - iStart);
            lppe += (pdd->iAnimateStart - iStart);
            iStart = pdd->iAnimateStart;
        }

        if (iStart + iLen > pdd->iAnimateEnd)
            iLen = pdd->iAnimateEnd - iStart;

        AnimatePalette(pdd->hpal, iStart, iLen, lppe);

        //
        //  any colors we could not animate, map to nearest
        //
        for (i=iStartSave; i<iStartSave+iLenSave; i++)
        {
            if (i >= pdd->iAnimateStart && i < pdd->iAnimateEnd)
                pdd->aw[i] = (WORD) i;
            else
                pdd->aw[i] = (WORD) GetNearestPaletteIndex(pdd->hpal,
                    RGB(lppeSave[i-iStartSave].peRed,
                        lppeSave[i-iStartSave].peGreen,
                        lppeSave[i-iStartSave].peBlue));
        }
    }
    else if (pdd->hpal)
    {
        SetPaletteEntries(pdd->hpal, iStart, iLen, lppe);
        pdd->ulFlags |= DDF_NEWPALETTE;
    }
    else
    {
        DPF(("Copying palette entries \n"));
        for (i=0; i<iLen; i++)
        {
            ((RGBQUAD *)pdd->aw)[iStart+i].rgbRed   = lppe[i].peRed;
            ((RGBQUAD *)pdd->aw)[iStart+i].rgbGreen = lppe[i].peGreen;
            ((RGBQUAD *)pdd->aw)[iStart+i].rgbBlue  = lppe[i].peBlue;
        }

        if (pdd->hbmDraw)
            pdd->ulFlags |= DDF_NEWPALETTE;
    }

    if (pdd->lpDIBSection) {

	// the colour table of a DIB Section is not changed when the palette
	// used to create it changes. We need to explicitly change it.
	SetDIBColorTable(pdd->hdcDraw, iStart, iLen,
		&(*(pdd->lpargbqIn))[iStart]);

    }

//    We'll break stupid buggy apps if we delete a palette we've given them
//    even though we told them not to use it.
//
//    if (pdd->hpalCopy)
//	DeleteObject(pdd->hpalCopy);
//    pdd->hpalCopy = NULL;

    return TRUE;
}

/**************************************************************************
*
* @doc EXTERNAL DrawDib
*
* @api UINT | DrawDibRealize | This function realizes palette
*      of the display context specified into the DrawDib context.
*
* @parm HDRAWDIB | hdd | Specifies a handle to a DrawDib context.
*
* @parm HDC | hdc | Specifies a handle to the display context containing
*       the palette.
*
* @parm BOOL | fBackground | If set to a nonzero value,
*       the selected palette is selected as a background palette.
*       If this is set to zero and the device context is attached
*       to a window, the logical palette is a foreground palette when
*       the window has the input focus. (The device context is attached
*       to a window if it was obtained by using the <f GetDC> function
*       or if the window-class style is CS_OWNDC.)
*
* @rdesc Returns number of entries in the logical palette
*        mapped to different values in the system palette. If
*        an error occurs or no colors were updated, it returns zero.
*
* @comm  This function should only be used to
*        handle a <m WM_PALETTECHANGE> or <m WM_QUERYNEWPALETTE>
*        message, or used in conjunction with the DDF_SAME_HDC flag
*        to prepare a display context prior to calling <f DrawDibDraw>
*        multiple times.
*
* @ex    The following example shows how the function is used to
*        handle a <m WM_PALETTECHANGE> or <m WM_QUERYNEWPALETTE>
*        message: |
*
*               case WM_PALETTECHANGE:
*                   if ((HWND)wParam == hwnd)
*                       break;
*
*               case WM_QUERYNEWPALETTE:
*                   hdc = GetDC(hwnd);
*
*                   f = DrawDibRealize(hdd, hdc, FALSE) > 0;
*
*                   ReleaseDC(hwnd, hdc);
*
*                   if (f)
*                       InvalidateRect(hwnd, NULL, TRUE);
*                   break;
*
* @ex   The following example shows using <f DrawDibRealize> use prior to
*       calling <f DrawDibDraw> multiple times: |
*
*               hdc = GetDC(hwnd);
*               DrawDibRealize(hdd, hdc, fBackground);
*               DrawDibDraw(hdd, hdc, ..........., DDF_SAME_DRAW|DDF_SAME_HDC);
*               DrawDibDraw(hdd, hdc, ..........., DDF_SAME_DRAW|DDF_SAME_HDC);
*               DrawDibDraw(hdd, hdc, ..........., DDF_SAME_DRAW|DDF_SAME_HDC);
*               ReleaseDC(hwnd, hdc);
*
* @ex   The following example shows using <f DrawDibRealize> with <f DDF_ANIMATE>
*       and (f DrawDibChangePalette> to do palette animation |
*
*               hdc = GetDC(hwnd);
*               DrawDibBegin(hdd, ....., DDF_ANIMATE);
*               DrawDibRealize(hdd, hdc, fBackground);
*               DrawDibDraw(hdd, hdc, ...., DDF_SAME_DRAW|DDF_SAME_HDC);
*               DrawDibChangePalette(hdd, ....);
*               ReleaseDC(hwnd, hdc);
*
* @comm To draw an image mapped to another palette use <f DrawDibSetPalette>.
*
*        To make <f DrawDibDraw> select its palette as a background palette
*        use the DDF_BACKGROUNDPAL flag and not this function.
*
*        While the DrawDib palette is selected into the display context,
*        do not call <f DrawDibEnd>, <f DrawDibClose>, <f DrawDibBegin>, or
*        <f DrawDibDraw> (with a different draw/format) on the same DrawDib
*        context <p hdd>. These can free the selected palette
*        while it is being used by your display context and cause
*        a GDI error.
*
* @xref <f SelectPalette>
*
**************************************************************************/
UINT VFWAPI DrawDibRealize(HDRAWDIB hdd, HDC hdc, BOOL fBackground)
{
    PDD pdd;
    HPALETTE hpal;
    UINT u;

    if (hdc == NULL)
	return 0;

    if ((pdd = DrawDibLock(hdd)) == NULL)
        return 0;

    if (IsScreenDC(hdc))
        pdd->ulFlags &= ~DDF_MEMORYDC;
    else {
        pdd->ulFlags |= DDF_MEMORYDC;

        DPF(("Drawing to a memory DC"));
    }

    SetStretchBltMode(hdc, COLORONCOLOR);

    //
    // what palette should we realize
    //
    hpal = pdd->hpalDraw ? pdd->hpalDraw : pdd->hpal;

    //
    // if we dont have a palette, we have nothing to realize
    // still call DrawDibPalChange though
    //
    if (hpal == NULL)
    {
        if (pdd->ulFlags & DDF_NEWPALETTE)
        {
            DrawDibPalChange(pdd, hdc, hpal);
            pdd->ulFlags &= ~DDF_NEWPALETTE;
        }

        return 0;
    }

// !!! There is a bug in GDI that will not map an identity palette 1-1 into
// !!! the system palette every time, which hoses us and makes it look like
// !!! dog spew.  This ICKITY-ACKITY-OOP code will flush the palette and
// !!! prevent the bug... BUT it introduces another bug where if we are a
// !!! background app, we hose everybody else's palette but ours.  So let's
// !!! live with the GDI bug.  One other thing... attempting this fix will
// !!! cause the bug to repro more often than it would have if you had left
// !!! it alone, unless you do the fix JUST RIGHT!  I don't trust myself
// !!! that much.
#if 0
    if ((pdd->ulFlags & DDF_NEWPALETTE) && (pdd->ulFlags & DDF_IDENTITYPAL) &&
		!fBackground)
    {
	//
	// this will flush the palette clean to avoid a GDI BUG!!!
	//
	SetSystemPaletteUse(hdc, SYSPAL_NOSTATIC);
	SetSystemPaletteUse(hdc, SYSPAL_STATIC);
    }
#endif

    //
    // select and realize the sucker
    //
    SelectPalette(hdc, hpal, fBackground);
    u = RealizePalette(hdc);

    // !!! If two DrawDib instances share the same palette handle, the second
    // one will not change any colours and u will be 0, and it will not stop
    // decompressing to screen or recompute stuff for bitmaps when it goes
    // into the background and it will get a screwed up palette.
    // !!! This is a known bug we don't care about
    //
    // this should be fixed by the hpalCopy stuff.

    if (u > 0 || (pdd->ulFlags & DDF_NEWPALETTE))
    {
	pdd->ulFlags |= DDF_NEWPALETTE;
        DrawDibPalChange(pdd, hdc, hpal);
        pdd->ulFlags &= ~DDF_NEWPALETTE;
    }

    return u;
}

/**************************************************************************
* @doc EXTERNAL DrawDib VFW11
*
* @api LPVOID | DrawDibGetBuffer | This function returns the pointer
*      to the DrawDib decompress buffer.
*
* @parm HDRAWDIB | hdd | Specifies a handle to a DrawDib context.
*
* @parm LPBITMAPINFOHEADER | lpbi | Specifies a pointer to a
*        <t BITMAPINFOHEADER> structure.
*
* @parm DWORD | dwSize | Specifies the size of the buffer pointed to by <p lpbi>
*
* @parm DWORD | dwFlags | Set to zero.
*
* @rdesc Returns a pointer to the buffer used by DrawDib for decompression,
*        or NULL if no buffer is used. If <p lpbi> is not NULL,
*        it is filled in with a copy of the <t BITMAPINFOHEADER>
*        describing the buffer.
*
*        The structure for <p lpbi> must have room for a
*        <t BITMAPINFOHEADER> and 256 colors.
*
**************************************************************************/

LPVOID VFWAPI DrawDibGetBuffer(HDRAWDIB hdd, LPBITMAPINFOHEADER lpbi, DWORD dwSize, DWORD dwFlags)
{
    PDD pdd;

    if ((pdd = DrawDibLock(hdd)) == NULL)
        return NULL;

    if (lpbi)
    {
        hmemcpy(lpbi, &pdd->biBuffer,
            min(dwSize, pdd->biBuffer.biSize + 256*sizeof(RGBQUAD)));
    }

    return pdd->pbBuffer;
}

LPVOID VFWAPI DrawDibGetBufferOld(HDRAWDIB hdd, LPBITMAPINFOHEADER lpbi)
{
    return DrawDibGetBuffer(hdd, lpbi, sizeof(BITMAPINFOHEADER), 0);
}

/**************************************************************************
* @doc EXTERNAL DrawDibStart
*
* @api BOOL | DrawDibStart | This function prepares a DrawDib
*      context for streaming playback.
*
* @parm HDRAWDIB | hdd | Specifies a handle to a DrawDib context.
*
* @parm LONG | rate | Specifies the playback rate (in microseconds per frame).
*
* @rdesc Returns TRUE if successful.
*
* @xref <f DrawDibStop>
*
**************************************************************************/
BOOL VFWAPI DrawDibStart(HDRAWDIB hdd, DWORD rate)
{
    PDD pdd;

    if ((pdd = DrawDibLock(hdd)) == NULL)
        return FALSE;

    if (pdd->hic == (HIC)-1)
        return FALSE;

    // if the codec does not care about this message dont fail.

    if (pdd->hic != NULL)
        ICSendMessage(pdd->hic, ICM_DRAW_START, rate, 0);

    return TRUE;
}

/**************************************************************************
* @doc EXTERNAL DrawDibStop
*
* @api BOOL | DrawDibStop | This function frees the resources
*      used by a DrawDib context for streaming playback.
*
* @parm HDRAWDIB | hdd | Specifies a handle to a DrawDib context.
*
* @rdesc Returns TRUE if successful.
*
* @xref <f DrawDibStart>
*
**************************************************************************/
BOOL VFWAPI DrawDibStop(HDRAWDIB hdd)
{
    PDD pdd;

    if ((pdd = DrawDibLock(hdd)) == NULL)
        return FALSE;

    if (pdd->hic == (HIC)-1)
        return FALSE;

    if (pdd->hic != NULL)
        ICSendMessage(pdd->hic, ICM_DRAW_STOP, 0, 0);

    return TRUE;
}

/**************************************************************************
* @doc EXTERNAL DrawDib
*
* @api BOOL | DrawDibUpdate | This macro updates the last
*      buffered frame drawn.
*
* @parm HDRAWDIB | hdd | Specifies a handle to a DrawDib context.
*
* @parm HDC | hdc | Specifies a handle to the display context.
*
* @parm int | xDst | Specifies the x-coordinate of the upper left-corner
*       of the destination rectangle. Coordinates are specified
*       in MM_TEXT client coordinates.
*
* @parm int | yDst | Specifies the y-coordinate of the upper-left corner
*       of the destination rectangle.  Coordinates are specified
*       in MM_TEXT client coordinates.
*
* @rdesc Returns TRUE if successful.
*
* @comm This macro uses <f DrawDibDraw> to send the DDF_UPDATE flag
*       to the DrawDib context.
*
**************************************************************************/

/**************************************************************************
* @doc EXTERNAL DrawDib
*
* @api BOOL | DrawDibDraw | This function draws a device independent
*      bitmap to the screen.
*
* @parm HDRAWDIB | hdd | Specifies a handle to a DrawDib context.
*
* @parm HDC | hdc | Specifies a handle to the display context.
*
* @parm int | xDst | Specifies the x-coordinate of the upper left-corner
*       of the destination rectangle. Coordinates are specified
*       in MM_TEXT client coordinates.
*
* @parm int | yDst | Specifies the y-coordinate of the upper-left corner
*       of the destination rectangle. Coordinates are specified
*       in MM_TEXT client coordinates.
*
* @parm int | dxDst | Specifies the width of the destination rectangle.
*       The width is specified in MM_TEXT client coordinates. If
*       <p dxDst> is -1, the width of the bitmap is used.
*
* @parm int | dyDst | Specifies the height of the destination rectangle.
*       The height is specified in MM_TEXT client coordinates. If
*       <p dyDst> is -1, the height of the bitmap is used.
*
* @parm LPBITMAPINFOHEADER | lpbi | Specifies a pointer to the
*       <t BITMAPINFOHEADER> structure for the bitmap. The color
*       table for the DIB follows the format information. The
*       height specified for the DIB in the structure must be
*       positive (that is, this function will not draw inverted DIBs).
*
* @parm LPVOID | lpBits | Specifies a pointer to the buffer
*       containing the bitmap bits.
*
* @parm int | xSrc | Specifies the x-coordinate of the upper-left corner
*       source rectangle. Coordinates are specified in pixels.
*       The coordinates (0,0) represent the upper left corner
*       of the bitmap.
*
* @parm int | ySrc | Specifies the y-coordinate of the upper left corner
*       source rectangle. Coordinates are specified in pixels.
*       The coordinates (0,0) represent the upper left corner
*       of the bitmap.
*
* @parm int | dxSrc | Specifies the width of the source rectangle.
*       The width is specified in pixels.
*
* @parm int | dySrc | Specifies the height of the source rectangle.
*       The height is specified in pixels.
*
* @parm UINT | wFlags | Specifies any applicable flags for drawing.
*       The following flags are defined:
*
* @flag DDF_UPDATE | Indicates the last buffered bitmap is to be redrawn.
*       If drawing fails with this flag, a buffered image is not available
*       and a new image needs to be specified before the display is updated.
*
* @flag DDF_SAME_HDC | Assumes the handle to the display context
*       is already specified. When this flag is used,
*       DrawDib also assumes the correct palette has already been
*       realized into the device context (possibly by
*       <f DrawDibRealize>).
*
* @flag DDF_SAME_DRAW | Uses the drawing parameters previously
*       specified for this function.  Use this flag only
*       if <p lpbi>, <p dxDst>, <p dyDst>, <p dxSrc>, and <p dySrc>
*       have not changed since using <f DrawDibDraw> or <f DrawDibBegin>.
*       Normally <f DrawDibDraw> checks the parameters, and if they
*       have changed, <f DrawDibBegin> prepares the DrawDib context
*       for drawing.
*
* @flag DDF_DONTDRAW | Indicates the frame is not to be drawn and will
*       later be recalled with the <f DDF_UPDATE> flag. DrawDib does
*       not buffer an image if an offscreen buffer does not exist.
*       In this case, DDF_DONTDRAW draws the frame to the screen and
*       the subsequent use of DDF_UPDATE fails. DrawDib does
*       guarantee that the following will
*       always draw "image" B to the screen.
*
*           DrawDibDraw(hdd, ..., lpbiA, ..., DDF_DONTDRAW);
*           DrawDibDraw(hdd, ..., lpbiB, ..., DDF_DONTDRAW);
*           DrawDibDraw(hdd, ..., NULL,  ..., DDF_UPDATE);
*
*       The DDF_UPDATE and DDF_DONTDRAW flags are used
*       together to create composite images
*       offscreen, and then do a final update when finished.
*
* @flag DDF_HURRYUP | Indicates the data does not have to
*       drawn (that is, it can be dropped) and the DDF_UPDATE flags will
*       not be used to recall this information. DrawDib looks at
*       this data only if it is required to build the next frame, otherwise
*       the data is ignored.
*
*       This flag is usually used to resynchronize video and audio. When
*       resynchronizing data, applications should send the image
*       with this flag in case the driver needs to
*       to buffer the frame to decompress subsequent frames.
*
* @flag DDF_UPDATE | Indicates the last buffered bitmap is to be redrawn.
*       If drawing fails with this flag, a buffered image is not available
*       and a new image needs to be specified before the display is updated.
*       For more information, see the <f DDF_DONTDRAW> flag.
*
* @flag DDF_BACKGROUNDPAL | Realizes the palette used for drawing
*       in the background leaving the actual palette used for display
*       unchanged.  (This flag is valid only if DDF_SAME_HDC is not set.)
*
* @flag DDF_HALFTONE | Always dithers the DIB to a standard palette
*       regardless of the palette of the DIB. If using <f DrawDibBegin>,
*       set this flag for it rather than <f DrawDibDraw>.
*
* @flag DDF_NOTKEYFRAME | Indicates the DIB data is not a key frame.
*
* @flag DDF_HURRYUP | Indicates the DIB data does not have to
*       drawn (that is, it can be dropped). This flag is usually
*       used to resynchronize the video to the audio. When
*       resynchronizing data, applications should send the image
*       with this flag in case the driver needs to
*       to buffer the frame to decompress subsequent frames.
*
* @rdesc Returns TRUE if successful.
*
* @comm This function replaces <f StretchDIBits> and supports
*       decompression of bitmaps by installable compressors.
*       This function dithers true color bitmaps properly on
*       8-bit display devices.
*
**************************************************************************/

BOOL VFWAPI DrawDibDraw(HDRAWDIB hdd,
                            HDC      hdc,
                            int      xDst,
                            int      yDst,
                            int      dxDst,
                            int      dyDst,
                            LPBITMAPINFOHEADER lpbi,
                            LPVOID   lpBits,
                            int      xSrc,
                            int      ySrc,
                            int      dxSrc,
                            int      dySrc,
                            UINT     wFlags)
{
    PDD	    pdd;
    BOOL    f;
    RECT    rc;
    DWORD   icFlags;
    LRESULT   dw;

    if ((pdd = DrawDibLock(hdd)) == NULL)
        return FALSE;

    if (hdc == NULL)
        return FALSE;

    if (wFlags & DDF_UPDATE)
    {
        lpbi = pdd->lpbi;

        dxDst = pdd->dxDst;
        dyDst = pdd->dyDst;
        dxSrc = pdd->dxSrc;
        dySrc = pdd->dySrc;
    }
    else
    {
        if (lpbi == NULL)
	    return FALSE;

        //
        // fill in defaults.
        //
        if (dxSrc < 0)
	    dxSrc = (int)lpbi->biWidth - xSrc;

	if (dySrc < 0)
	    dySrc = (int)lpbi->biHeight - ySrc;
	
        if (dxDst < 0)
	    dxDst = dxSrc;

        if (dyDst < 0)
	    dyDst = dySrc;
    }

#ifdef DEBUG_RETAIL
    if (xSrc  <  0 ||
        ySrc  <  0 ||
        dxSrc <= 0 ||
        dySrc <= 0 ||
        xSrc + dxSrc > (int)lpbi->biWidth ||
        ySrc + dySrc > (int)lpbi->biHeight)
    {
        RPF(("DrawDibBegin(): bad source parameters [%d %d %d %d]", xSrc, ySrc, dxSrc, dySrc));
//      return 0;   // see what happens.
    }
#endif

    if (dxSrc == 0 || dySrc == 0)	// !!! || dxDst == 0 || dyDst == 0)
        return FALSE;

    //
    // check and make sure the params of the draw has not changed
    //
    if (!(wFlags & (DDF_SAME_DRAW|DDF_UPDATE)) &&
        !(DibEq(pdd->lpbi, lpbi) &&
          !(((UINT)pdd->ulFlags ^ wFlags) & DDF_HALFTONE) &&
        pdd->dxDst == dxDst &&
        pdd->dyDst == dyDst &&
        pdd->dxSrc == dxSrc &&
        pdd->dySrc == dySrc))
    {
        wFlags &= ~(DDF_UPDATE | DDF_FULLSCREEN);
        if (!DrawDibBegin(hdd, hdc, dxDst, dyDst, lpbi, dxSrc, dySrc, wFlags))
	    return FALSE;
    }

    TIMEINC();      // should we include DibEq?
    TIMESTART(timeDraw);

    // convert to DIB cordinates
    ySrc = (int)pdd->lpbi->biHeight - (ySrc + dySrc);

    //
    // Initialize the DC:  We need to realize the palette if we are not
    // guarenteed to be using the same DC as before, if we've been told we
    // have a new palette, or if we are mapping to somebody else's palette.
    // The owner of the palette could be changing it on us all the time or
    // doing who knows what, so to be safe we will realize it every frame.
    // If nothing's changed, this should be a really cheap operation, and
    // it doesn't appear to be causing any palette fights that end in somebody
    // getting hurt.  This is required for Magic School Bus, and PageMaster,
    // at the very least. (WIN95B 12204 and 9637)
    //
    if (!(wFlags & DDF_SAME_HDC) || (pdd->ulFlags & DDF_NEWPALETTE) ||
							pdd->hpalDraw)
    {
        //
        // image will be totally clipped anyway
        //
        if (GetClipBox(hdc, &rc) == NULLREGION)
        {
	    wFlags |= DDF_DONTDRAW;
        }

        //
        // select and realize the palette.
        //
        // NOTE you must unselect this thing, dont return early
        //
        DrawDibRealize(hdd, hdc, (wFlags & DDF_BACKGROUNDPAL) != 0);
    }

#ifdef USE_DCI
    //
    //  do a clipping check
    //
    if (pdd->ulFlags & DDF_CLIPCHECK)
    {
        RECT  rc;

        if (!(pdd->ulFlags & DDF_CLIPPED) &&
            (pdd->iDecompress == DECOMPRESS_SCREEN) && // (pdd->ulFlags & DDF_SCREENX) &&
            (wFlags & (DDF_PREROLL|DDF_DONTDRAW)))
        {
	    DPF(("DDF_DONTDRAW while decompressing to screen, staying clipped"));
        }

        if ((pdd->ulFlags & DDF_MEMORYDC) ||
	    GetClipBox(hdc, &rc) != SIMPLEREGION ||
	    xDst < rc.left ||
	    yDst < rc.top ||
	    xDst + dxDst > rc.right ||
	    yDst + dyDst > rc.bottom ||
	    (wFlags & (DDF_PREROLL|DDF_DONTDRAW)) ||
	    (gfDisplayHasBrokenRasters &&
		     (DCNotAligned(hdc, xDst) || gwScreenBitDepth == 24)))
	    // Note: if we're on a 24-bit display with broken rasters, we don't
	    // decompress to the screen even if the rectangle is aligned,
	    // because it's just too easy for somebody to try to write out
	    // a whole pixel in one gulp and hit the 64K boundary.
	{
	    //
	    //  we are clipped, check for a change.
	    //
	    if (!(pdd->ulFlags & DDF_CLIPPED))
	    {
	        pdd->ulFlags |= DDF_CLIPPED;
	        DrawDibClipChange(pdd, wFlags);
	    }
	}
	else
	{
	    //  !!!
	    //  check for the screen width changing, Chicago
	    //  currently can't change the bitdepth so don't
	    //  worry about that right now.
	    //
	    if (GetSystemMetrics(SM_CXSCREEN) != (int)gwScreenWidth)
	    {
		pdd->ulFlags |= DDF_CLIPPED;
		DrawDibClipChange(pdd, wFlags);
		DrawDibInit();
	    }

	    //
	    // we are now unclipped, check for a change
	    //
	    if ((pdd->ulFlags & DDF_CLIPPED) && !(pdd->ulFlags & DDF_UPDATE))
	    {
#ifdef DEBUG
		if (DCNotAligned(hdc, xDst))
		    DPF(("Warning draw is not aligned on 4 pixel boundary"));
#endif
		pdd->ulFlags &= ~DDF_CLIPPED;
		DrawDibClipChange(pdd, wFlags);
	    }
        }

    DPF(("Clip Box: %d %d %d %d    Dest: %d %d %d %d", rc.left, rc.top, rc.right, rc.bottom, xDst, yDst, xDst + dxDst, yDst + dyDst));

    }

#endif  //_WIN32
    if (pdd->ulFlags & DDF_WANTKEY)
    {
        //
        // Adobe hack: If the DDF_UPDATE flag is on in our internal
        // flags, that means we've just been getting a bunch of frames
        // with the DONTDRAW flag set.  In that case, if this frame
        // immediately after those frames is marked as a key frame
        // we assume that it might not be a key frame and refrain from
        // switching immediately to decompressing to screen.
        //
        if (!(wFlags & DDF_NOTKEYFRAME) && !(pdd->ulFlags & DDF_UPDATE))
        {
    	pdd->ulFlags &= ~DDF_WANTKEY;
    	DrawDibClipChange(pdd, wFlags);
        }
    }

    //
    // if update is set re-draw what ever we drew last time
    //
    if (wFlags & DDF_UPDATE)
    {
        if (pdd->hic == (HIC)-1 || (pdd->ulFlags & DDF_DIRTY))
        {
	    f = FALSE;
	    DPF(("Can't update: no decompress buffer!"));
	    goto exit;
        }

        if (pdd->hic)
        {
	    if (pdd->ulFlags & DDF_UPDATE)
	    {
		goto redraw;
	    }

	    lpbi = &pdd->biBuffer;
	    lpBits = pdd->pbBuffer;

	    //!!! set the source right.

	    if ((pdd->ulFlags & DDF_XLATSOURCE))
	    {
		dxSrc = MulDiv(dxSrc, abs((int)pdd->biBuffer.biWidth),  (int)pdd->lpbi->biWidth);
		dySrc = MulDiv(dySrc, abs((int)pdd->biBuffer.biHeight), (int)pdd->lpbi->biHeight);
		xSrc  = MulDiv(xSrc,  abs((int)pdd->biBuffer.biWidth),  (int)pdd->lpbi->biWidth);
		ySrc  = MulDiv(ySrc,  abs((int)pdd->biBuffer.biHeight), (int)pdd->lpbi->biHeight);
	    }
        }

        if (pdd->ulFlags & DDF_STRETCH)
        {
	    lpbi  = &pdd->biStretch;
	    lpBits = pdd->pbStretch;
	    dxSrc  = dxDst;
	    dySrc  = dyDst;
	    xSrc   = 0;
	    ySrc   = 0;
        }

        if (pdd->ulFlags & DDF_DITHER)
        {
	    lpBits = pdd->pbDither;
	    xSrc = 0;
	    ySrc = 0;
        }

        if (pdd->lpDIBSection != NULL)
	    goto bltDIB;

        if (pdd->hbmDraw && (pdd->ulFlags & DDF_BITMAP))
            goto bltit;


        if (lpBits == NULL)
        {
	    f = FALSE;       // no buffer, can't update....
	    goto exit;
        }

        goto drawit;
    }

    //
    // silly default for bits pointerdefault
    //
    if (lpBits == NULL)
        lpBits = (LPBYTE)lpbi+(int)lpbi->biSize + (int)lpbi->biClrUsed * sizeof(RGBQUAD);

    //
    // call any decompressor if needed
    //
    if (pdd->hic)
    {
        if (pdd->hic == (HIC)-1)
        {
	    f = FALSE;
	    goto exit;
        }

#ifdef USE_DCI // exclude all code that references biscreen

        if (pdd->iDecompress == DECOMPRESS_SCREEN) // pdd->ulFlags & DDF_SCREENX
        {
	
	    DCIRVAL    DCIResult;
	    int		xDstC = xDst, yDstC = yDst;
	    POINT   pt;
	    pt.x = xDst; pt.y = yDst;
	    LPtoDP(hdc, &pt, 1);
	    xDst = pt.x;
	    yDst = pt.y;
	
#ifdef _WIN32
	    GetDCOrgEx(hdc, &pt);
	    xDst += pt.x;
	    yDst += pt.y;
#else
	    DWORD dwOrg;
	    dwOrg = GetDCOrg(hdc);
	    xDst += LOWORD(dwOrg);
	    yDst += HIWORD(dwOrg);
#endif
	    //
	    // we are decompressing to the screen not the buffer, so
	    // the buffer is dirty now
	    //
	    pdd->ulFlags |= DDF_DIRTY;

#define USEGETCLIPBOXEARLY
    // IF we call GetClipBox AFTER doing DCIBeginAccess we end up with
    // EXTREMELY slow drawing.
#ifdef USEGETCLIPBOXEARLY
	    // To see whether or not decompressing to the screen was OK, we
	    // called GetClipBox.  But that was a long time ago, and as of
	    // Chicago, things could have changed since then.  We call 32 bit
	    // code in the meantime, which lets other 16 bit code run, like USER
	    // which might clip the window with another window or worse yet,
	    // move this window off the screen so decompressing to the screen
	    // will crash!  Calling GetClipBox way back then was just not valid.
	    // We have to call it again now.  I'm not removing the call way back
	    // then because touching this code is known to ruin the stability
	    // of the universe. (WIN95 BUG#20754 and WIN95B BUG#8374)
	    // WARNING: xDst, yDst are in SCREEN co-ords now.  Use the client
	    // ones.
	    if (GetClipBox(hdc, &rc) != SIMPLEREGION ||
	    		xDstC < rc.left ||
	    		yDstC < rc.top ||
	    		xDstC + dxDst > rc.right ||
	    		yDstC + dyDst > rc.bottom) {
		dw = ICERR_OK;
		f = TRUE;
		goto exit;
	    }
#endif
	    DCIResult = DCIBeginAccess(pdci, xDst, yDst, dxDst, dyDst);

	    //
	    // if DCIBeginAccess fails we are in the background, and should
	    // not draw.
	    //
	    if (DCIResult < 0)
	    {
		DPF(("DCIBeginAccess returns %d\n", DCIResult));
		f = TRUE;       //!!! handle more error values!
		goto exit;
	    }
            else if (DCIResult > 0)
            {
                if (DCIResult & DCI_STATUS_POINTERCHANGED)
                {
#ifdef _WIN32
                    if (pdci->dwDCICaps & DCI_1632_ACCESS)
                    {
                        //
                        // make sure the pointer is valid.
                        //
                        if (pdci->dwOffSurface >= 0x10000)
                        {
                            DPF(("DCI Surface can't be supported: offset >64K"));

                            lpScreen = NULL;
                        }
                        else
                            lpScreen = (LPVOID)MAKELONG((WORD)pdci->dwOffSurface, pdci->wSelSurface);
                    }
                    else
                    {
                        lpScreen = (LPVOID) pdci->dwOffSurface;
                    }
#else
                    //
                    // make sure the pointer is valid.
                    //
                    if (pdci->dwOffSurface >= 0x10000)
                    {
                        DPF(("DCI Surface can't be supported: offset >64K"));

                        lpScreen = NULL;
                    }
                    else
                        lpScreen = (LPVOID)MAKELP(pdci->wSelSurface,pdci->dwOffSurface);
#endif
                }

                if (DCIResult & (DCI_STATUS_STRIDECHANGED |
                                 DCI_STATUS_FORMATCHANGED |
                                 DCI_STATUS_SURFACEINFOCHANGED |
                                 DCI_STATUS_CHROMAKEYCHANGED))
                {
                    DPF(("Unhandled DCI Flags!  (%04X)", DCIResult));
                }

                if (DCIResult & DCI_STATUS_WASSTILLDRAWING)
                {
                    DPF(("DCI still drawing!?!", DCIResult));
                    f = TRUE;       //!!!
                    goto EndAccess;
                }
            }

	    if (lpScreen == NULL)
	    {
		DPF(("DCI pointer is NULL when about to draw!"));
		f = FALSE;
		goto EndAccess;
	    }
	
	    //convert to DIB corrds.
	    yDst = (int)pdci->dwHeight - (yDst + dyDst);

	    TIMESTART(timeDecompress);

	    icFlags = 0;

	    if (wFlags & DDF_HURRYUP)
		icFlags |= ICDECOMPRESS_HURRYUP;

	    if (wFlags & DDF_NOTKEYFRAME)
		icFlags |= ICDECOMPRESS_NOTKEYFRAME;

	    DPF2(("Drawing To Screen: %d %d %d %d", xDst, yDst, xDst + dxDst, yDst + dyDst));
#ifdef USEGETCLIPBOXLATE
	    // To see whether or not decompressing to the screen was OK, we
	    // called GetClipBox.  But that was a long time ago, and as of
	    // Chicago, things could have changed since then.  We call 32 bit
	    // code in the meantime, which lets other 16 bit code run, like USER
	    // which might clip the window with another window or worse yet,
	    // move this window off the screen so decompressing to the screen
	    // will crash!  Calling GetClipBox way back then was just not valid.
	    // We have to call it again now.  I'm not removing the call way back
	    // then because touching this code is known to ruin the stability
	    // of the universe. (WIN95 BUG#20754 and WIN95B BUG#8374)
	    // WARNING: xDst, yDst are in SCREEN co-ords now.  Use the client
	    // ones.
	    if (GetClipBox(hdc, &rc) != SIMPLEREGION ||
	    		xDstC < rc.left ||
	    		yDstC < rc.top ||
	    		xDstC + dxDst > rc.right ||
	    		yDstC + dyDst > rc.bottom) {
		dw = ICERR_OK;
	    }
	    else
#endif
	    {
	        dw = ICDecompressEx(pdd->hic, icFlags,
				lpbi, lpBits, xSrc, ySrc, dxSrc, dySrc,
				(LPBITMAPINFOHEADER) &biScreen, lpScreen,
				xDst, yDst, dxDst, dyDst);
	    }

	    if (dw == ICERR_DONTDRAW)
		dw = ICERR_OK;

	    f = (dw == ICERR_OK);

	    TIMEEND(timeDecompress);
EndAccess:
	    DCIEndAccess(pdci);
	    goto exit;
        }
        else
#endif // biscreen references
	{
	    //
	    //  if the offscreen buffer is dirty, only a key frame will
	    //  clean our soul.
	    //
	    if (pdd->ulFlags & DDF_DIRTY)
	    {
	        if (wFlags & DDF_NOTKEYFRAME)
	        {
		    //!!! playing files with no key frames we will get into
		    //a state where we will never draw a frame ever again.
		    //we need a punt count?

		    DPF(("punt frame"));

		    f = TRUE;
		    goto exit;
	        }
	        else // if (!(wFlags & DDF_HURRYUP))
		{
		    pdd->ulFlags &= ~DDF_DIRTY;
	        }
	    }

	    TIMESTART(timeDecompress);

	    icFlags = 0;

	    if (wFlags & DDF_HURRYUP)
	        icFlags |= ICDECOMPRESS_HURRYUP;

	    if (wFlags & DDF_NOTKEYFRAME)
	        icFlags |= ICDECOMPRESS_NOTKEYFRAME;

	    if (pdd->lpDIBSection != 0)
	    {

	        dw = ICDecompress(pdd->hic, icFlags, lpbi, lpBits, &pdd->biBuffer, pdd->pbBuffer);
	    }
	    else if (pdd->iDecompress == DECOMPRESS_BITMAP) //pdd->ulFlags & DDF_BITMAPX
	    {
                //!!! should we check FASTTEMPORALD?
                if (pdd->ulFlags & DDF_HUGEBITMAP)
                    HugeToFlat(pdd->pbBitmap,pdd->biBitmap.biSizeImage,pdd->biBitmap.biYPelsPerMeter);

#ifdef _WIN32
                // Win32: still use the decomp buffer, then SetBitmapBits
                dw = ICDecompress(pdd->hic, icFlags, lpbi, lpBits, &pdd->biBitmap, pdd->pbBuffer);
                SetBitmapBits(pdd->hbmDraw, pdd->biBitmap.biSizeImage, pdd->pbBuffer);
#else
                dw = ICDecompress(pdd->hic, icFlags, lpbi, lpBits, &pdd->biBitmap, pdd->pbBitmap);
#endif

                if (pdd->ulFlags & DDF_HUGEBITMAP)
                    FlatToHuge(pdd->pbBitmap,pdd->biBitmap.biSizeImage,pdd->biBitmap.biYPelsPerMeter);
            }
            else
            {
                dw = ICDecompress(pdd->hic, icFlags, lpbi, lpBits, &pdd->biBuffer, pdd->pbBuffer);
            }

            TIMEEND(timeDecompress);

            FlushBuffer();

	    if (dw == ICERR_DONTDRAW) {
		// Decompressor doesn't want us to draw, for some reason....
		wFlags |= DDF_DONTDRAW;
	    } else if (dw != 0) {
		f = FALSE;
		DPF(("Error %ld from decompressor!\n", dw));
		goto exit;
	    }
        }

        //
        // if don't draw is set we just need to decompress
        //
        if (wFlags & (DDF_DONTDRAW|DDF_HURRYUP))
        {
	    f = TRUE;
            pdd->ulFlags |= DDF_UPDATE|DDF_DONTDRAW;    // make sure update knows what to do
	    goto exit;
        }

#ifndef DAYTONA
// this does not work on Daytona (because the biSizeImage is confused?)
// but in any case it's not clear that a  delta draw from client side is a
// win over a DIBSection blt from server side.
        //
        //  draw RLE delta's to the screen even when we are buffering,
	//  as long as we're not stretching, dithering or using a memory DC.
	//  Drawing RLE's to a memory DC won't work in GDI.
	//  I will also refuse to draw RLE directly if we are using a
	//  decompressor (pdd->hic).  I do this for no good reason other than
	//  it fixes Win95 bug 24412
        //
        if (!(pdd->ulFlags & (DDF_MEMORYDC|DDF_DONTDRAW|DDF_STRETCH|DDF_DITHER))
	    && lpbi->biCompression == BI_RLE8 &&
	    (dxDst == dxSrc) && (dyDst == dySrc) &&
            lpbi->biSizeImage != pdd->biBuffer.biSizeImage &&
	    !(pdd->hic))
        {
            pdd->ulFlags |= DDF_UPDATE;    // make sure update knows what to do
            pdd->biDraw.biCompression = BI_RLE8;
            goto drawit;
        }
#endif
redraw:
        pdd->ulFlags &= ~(DDF_UPDATE|DDF_DONTDRAW);

        if ((pdd->ulFlags & DDF_XLATSOURCE))
        {
            dxSrc = MulDiv(dxSrc, abs((int)pdd->biBuffer.biWidth),  (int)pdd->lpbi->biWidth);
            dySrc = MulDiv(dySrc, abs((int)pdd->biBuffer.biHeight), (int)pdd->lpbi->biHeight);
            xSrc  = MulDiv(xSrc,  abs((int)pdd->biBuffer.biWidth),  (int)pdd->lpbi->biWidth);
            ySrc  = MulDiv(ySrc,  abs((int)pdd->biBuffer.biHeight), (int)pdd->lpbi->biHeight);
        }

        lpbi = &pdd->biBuffer;
	lpBits = pdd->pbBuffer;

        pdd->biDraw.biCompression = pdd->biBuffer.biCompression;
    }
    else
    {

	// if we are using DIB Sections, and there is no decompression,
	// dither or stretch step, then we need to explicitly copy the data
	// into the dib section.
	// maybe with the revised CreateDIBSection we could map the existing
	// read buffer and avoid this step
	if (pdd->lpDIBSection && ((pdd->ulFlags & (DDF_STRETCH|DDF_DITHER)) == 0)) {
	    // Include time taken here as 'stretching'.
	    // Really, though, we shouldn't be using DIB Sections in this case.
            TIMESTART(timeStretch);
	    if (lpbi->biCompression == BI_RGB) {
		lpbi->biSizeImage = DIBSIZEIMAGE(*lpbi);
	    }

	    hmemcpy(pdd->lpDIBSection, lpBits, lpbi->biSizeImage);
            TIMEEND(timeStretch);
	}

        //
        // when directly drawing RLE data we cant hurry
        //
        if (pdd->lpbi->biCompression == BI_RLE8)
            wFlags &= ~DDF_HURRYUP;

        //
        // if don't draw is set we just need to stretch/dither
        //
        if (wFlags & DDF_HURRYUP)
        {
            f = TRUE;
            pdd->ulFlags |= DDF_DIRTY;
            goto exit;
        }

        pdd->ulFlags &= ~DDF_DIRTY;
        pdd->biDraw.biCompression = lpbi->biCompression;
    }

    if (pdd->biDraw.biCompression == BI_RGB &&
        (pdd->ulFlags & (DDF_DITHER|DDF_STRETCH)))
    {
        if (pdd->ulFlags & DDF_STRETCH)
        {
            TIMESTART(timeStretch);

            StretchDIB(&pdd->biStretch, pdd->pbStretch,
                0, 0, dxDst, dyDst,
		lpbi,lpBits,
                xSrc,ySrc,dxSrc,dySrc);

            TIMEEND(timeStretch);

            lpbi  = &pdd->biStretch;
            lpBits = pdd->pbStretch;
            dxSrc  = dxDst;
            dySrc  = dyDst;
            xSrc   = 0;
            ySrc   = 0;
        }

	if (pdd->ulFlags & DDF_DITHER)
        {
            TIMESTART(timeDither);

#if 0 // this check isn't right.
	    // current dither code can only handle 1:1 sizing
	    if ((pdd->biDraw.biWidth != dxSrc) ||
	        (pdd->biDraw.biHeight != dySrc)) {
#ifdef DEBUG
		    DPF(("dither expected to stretch?"));
		    DebugBreak();
#endif
		    // fix it up somehow to avoid crash
		    dxSrc = (int) pdd->biDraw.biWidth;
		    dySrc = (int) pdd->biDraw.biHeight;
	    }
#endif
            pdd->DitherProc(&pdd->biDraw, pdd->pbDither,0,0,dxSrc,dySrc,
                lpbi,lpBits,xSrc, ySrc, pdd->lpDitherTable);

            TIMEEND(timeDither);

            lpBits = pdd->pbDither;
            xSrc = 0;
            ySrc = 0;
        }

        if ((wFlags & DDF_DONTDRAW) && !pdd->hbmDraw)
        {
            f = TRUE;
            goto exit;
        }
    }
#ifdef _WIN32
    else if (pdd->biDraw.biCompression == BI_RLE8)
    {
	/*
	 * if drawing RLE deltas on NT, the biSizeImage field needs to
	 * accurately reflect the amount of RLE data present in lpBits.
	 */
	pdd->biDraw.biSizeImage = lpbi->biSizeImage;
    }
#endif

    if (pdd->lpDIBSection != NULL) {

	//ASSERT(pdd->hbmDraw != NULL);

        if (wFlags & DDF_DONTDRAW)
        {
            f = TRUE;
            goto exit;
        }

bltDIB:
        TIMESTART(timeBlt);

	// Put things back in right-side-up coordinates
	ySrc = (int)pdd->biDraw.biHeight - (ySrc + dySrc);
//      ySrc = 0; // Was like this for Chicago M6!

        f = StretchBlt(hdc,xDst,yDst,dxDst,dyDst,pdd->hdcDraw,
            xSrc,ySrc,dxSrc,dySrc,SRCCOPY) != 0;

	FlushBuffer();

        TIMEEND(timeBlt);


    } else if (pdd->hbmDraw)
    {
#ifndef _WIN32
        //
        //  when MCIAVI is playing we need realize our palette for each
        //  draw operation because another app may have drawn a translated
        //  bitmap thus screwing up the GDI *global* device translate table.
        //  RonG I hate you some times
        //
        if (pdd->hpal && (wFlags & DDF_SAME_HDC))
            RealizePalette(hdc);
#endif

	if (pdd->iDecompress != DECOMPRESS_BITMAP) // !(pdd->ulFlags & DDF_BITMAPX)
	{
            TIMESTART(timeSetDIBits);
#if USE_SETDI
            pdd->sd.hdc = hdc;      //!!!ack!
            SetBitmap(&pdd->sd,xSrc,0,dxSrc,dySrc,lpBits,xSrc,ySrc,dxSrc,dySrc);
            pdd->sd.hdc = NULL;     //!!!ack!
            ySrc = 0;
#else
            SetDIBits(hdc, pdd->hbmDraw, 0, dySrc,
                lpBits, (LPBITMAPINFO)&pdd->biDraw, pdd->uiPalUse);
#endif
            FlushBuffer();

            TIMEEND(timeSetDIBits);
        }

        if (wFlags & DDF_DONTDRAW)
        {
            f = TRUE;
            goto exit;
        }
bltit:
        TIMESTART(timeBlt);

	// Put things back in right-side-up coordinates
	ySrc = (int)pdd->biDraw.biHeight - (ySrc + dySrc);
//      ySrc = 0; // Was like this for Chicago M6!

        f = StretchBlt(hdc,xDst,yDst,dxDst,dyDst,pdd->hdcDraw,
            xSrc,ySrc,dxSrc,dySrc,SRCCOPY) != 0;

	FlushBuffer();

        TIMEEND(timeBlt);
    }
    else
drawit:
    {

	// Sometimes when you read an RLE file, you get RGB data back (ie. the
	// first frame).  Passing RGB data to a display driver who thinks it
	// is getting RLE data will blow it up.  If the RLE data is the exact
	// size of RGB data, we decide that's just too much of a coincidence.
	BOOL fNotReallyRLE = (pdd->biDraw.biCompression == BI_RLE8 &&
	    lpbi->biSizeImage == DIBWIDTHBYTES(*lpbi) * (DWORD)lpbi->biHeight);

        if (wFlags & DDF_DONTDRAW)
        {
            f = TRUE;
            goto exit;
        }

	if (fNotReallyRLE)
	    pdd->biDraw.biCompression = BI_RGB;

        TIMESTART(timeBlt);

// NT StretchDIBits does not work with RLE deltas, even 1:1
#ifndef CHICAGO
	/*
	 * also note use of pdd->uiPalUse: this is DIB_PAL_COLORS by
	 * default, but may be set to DIB_PAL_INDICES if we detect that
	 * the system palette is identical to ours, and thus
	 * we can safely take this huge performance benefit (on NT,
	 * DIB_PAL_INDICES nearly halves the cost of this call)
	 */
        if ((dxDst == dxSrc) && (dyDst == dySrc))
        {
            f = SetDIBitsToDevice(hdc, xDst, yDst, dxDst, dyDst,
                    xSrc, ySrc, 0, (UINT)pdd->biDraw.biHeight, lpBits,
                    (LPBITMAPINFO)&pdd->biDraw, pdd->uiPalUse) != 0;
        }
        else
#endif
        {
            f = StretchDIBits(hdc,xDst,yDst,dxDst,dyDst,
                xSrc,ySrc,dxSrc,dySrc,
                lpBits, (LPBITMAPINFO)&pdd->biDraw,
                pdd->uiPalUse, SRCCOPY) != 0;
	}

        FlushBuffer();

        TIMEEND(timeBlt);

	if (fNotReallyRLE)
	    pdd->biDraw.biCompression = BI_RLE8;
    }

exit:

#ifdef _WIN32
    // from build 549 (or thereabouts) we need at least one of these per
    // frame!
    // But not if we are using DCI??  Although the cost is small so leave it in
    GdiFlush();
#endif

    if (!(wFlags & DDF_SAME_HDC) && pdd->hpal)
	SelectPalette(hdc, GetStockObject(DEFAULT_PALETTE), TRUE);

    TIMEEND(timeDraw);
    return f;
}

#if 0
/**************************************************************************
* @doc INTERNAL
*
* @api BOOL| InitDrawToScreen | init drawing to the screen via DCI
*
**************************************************************************/
static BOOL InitDrawToScreen(PDD pdd)
{
    BOOL f;

    if (!(pdd->ulFlags & DDF_CANDRAWX))
        return FALSE;

    f = !(pdd->ulFlags & DDF_CLIPPED);

    if (f && !(pdd->ulFlags & DDF_DRAWX))
    {
        DPF(("drawing to SCREEN now"));

        pdd->ulFlags |= DDF_DRAWX;
    }
    else if (!f && (pdd->ulFlags & DDF_DRAWX))
    {
        DPF(("not drawing to SCREEN anymore"));

        pdd->ulFlags &= ~DDF_DRAWX;
    }
}
#endif

/**************************************************************************
* @doc INTERNAL
*
* @api BOOL| InitDecompress | init every thing for decompressing
*   to the screen or a bitmap or a memory buffer.
*
* we can decompress to the screen if the following is true:
*
*   palette must be 1:1
*   must be unclipped
*
**************************************************************************/
static BOOL InitDecompress(PDD pdd)
{
    BOOL f;
    BOOL fBitmap;
    BOOL fScreen;

    //
    // nothing to init
    //
    if (!(pdd->ulFlags & (DDF_CANSCREENX|DDF_CANBITMAPX)))
        return TRUE;

    //
    // make sure we rebegin when the palette changes
    //
    if (pdd->ulFlags & (DDF_NEWPALETTE|DDF_WANTKEY))
        pdd->iDecompress = 0;

    //
    // we need to decompress to either a memory bitmap or buffer.
    //
    fBitmap = (pdd->ulFlags & DDF_CANBITMAPX) &&
              (pdd->ulFlags & DDF_IDENTITYPAL|DDF_CANSETPAL);

    fScreen = (pdd->ulFlags & DDF_CANSCREENX) &&
             !(pdd->ulFlags & DDF_CLIPPED)    &&
              (pdd->ulFlags & DDF_IDENTITYPAL|DDF_CANSETPAL);

    //
    // should we be decompressing to the screen?
    //
#ifdef USE_DCI
    if (fScreen && pdd->iDecompress != DECOMPRESS_SCREEN)
    {
        if (pdd->ulFlags & DDF_IDENTITYPAL)
        {
            if (pdd->hpalDraw)
                ICDecompressSetPalette(pdd->hic, &pdd->biBuffer);
            else
                ICDecompressSetPalette(pdd->hic, NULL);
        }
        else
        {
            if (FixUpCodecPalette(pdd->hic, pdd->lpbi))
            {
                DPF(("Codec notified of palette change...."));
            }
            else
            {
                DPF(("Codec failed palette change...."));
		pdd->iDecompress = 0;
                goto ack;
            }
        }

	//
        // now init the compressor for screen decompress.
        //
        f = ICDecompressExBegin(pdd->hic, 0,
				pdd->lpbi, NULL, 0, 0, pdd->dxSrc, pdd->dySrc,
				(LPBITMAPINFOHEADER) &biScreen, lpScreen,
				0, 0, pdd->dxDst, pdd->dyDst) == ICERR_OK;

        if (f)
        {
	    pdd->ulFlags |= DDF_DIRTY;          // buffer is dirty now?
            RPF(("Decompressing to screen now"));
	    pdd->iDecompress = DECOMPRESS_SCREEN;
            return TRUE;
        }
        else
        {
ack:        DPF(("Compressor failed decompress to SCREEN, so not decompressing to screen!!!!"));
	    pdd->iDecompress = 0;
            pdd->ulFlags &= ~DDF_CANSCREENX;
        }
    }
    else if (fScreen)
    {
        //
        //  already decompressing to screen.
        //
        return TRUE;
    }
#endif

    if (fBitmap && pdd->iDecompress != DECOMPRESS_BITMAP)
    {
        if (pdd->ulFlags & DDF_IDENTITYPAL)
        {
            if (pdd->hpalDraw)
                ICDecompressSetPalette(pdd->hic, &pdd->biBuffer);
            else
                ICDecompressSetPalette(pdd->hic, NULL);
        }
        else
        {
            if (FixUpCodecPalette(pdd->hic, pdd->lpbi))
            {
                DPF(("Codec notified of palette change...."));
            }
            else
            {
                DPF(("Codec failed palette change...."));
		pdd->iDecompress = 0;
                goto ackack;
            }
        }

        f = ICDecompressBegin(pdd->hic, pdd->lpbi, &pdd->biBitmap) == ICERR_OK;

	if (f)
	{
	    DPF(("decompressing to BITMAP now"));

	    pdd->ulFlags |= DDF_DIRTY;          // buffer is dirty now?
	    pdd->iDecompress = DECOMPRESS_BITMAP;

	    // naked bitmap translate stuff?

	    return TRUE;
	}
	else
	{
ackack:     DPF(("Unable to init decompress to bitmap"));
	    pdd->iDecompress = 0;
        }
    }
    else if (fBitmap)
    {
        //
        //  already decompressing to bitmap.
        //
        return TRUE;
    }
	
    //
    // should we decompress to a buffer?
    //
    if (pdd->iDecompress != DECOMPRESS_BUFFER)
    {
	DPF(("decompressing to DIB now"));

        pdd->ulFlags |= DDF_DIRTY;          // buffer is dirty now?
        pdd->iDecompress = DECOMPRESS_BUFFER;

        if (pdd->hpalDraw)
            ICDecompressSetPalette(pdd->hic, &pdd->biBuffer);
        else
            ICDecompressSetPalette(pdd->hic, NULL);

        f = ICDecompressBegin(pdd->hic, pdd->lpbi, &pdd->biBuffer) == ICERR_OK;

        if (!f)
        {
            DPF(("Unable to re-begin compressor"));
        }
    }

    return TRUE;    // nothing to change
}

/**************************************************************************
* @doc INTERNAL
*
* @api void | DrawDibClipChange | called when the clipping has changed
*   from clipped to totaly un-clipped or whatever.
*
**************************************************************************/
static void DrawDibClipChange(PDD pdd, UINT wFlags)
{
    if (!(pdd->ulFlags & DDF_NEWPALETTE))
    {
	if (pdd->ulFlags & DDF_CLIPPED)
	    DPF(("now clipped"));
	else
	    DPF(("now un-clipped"));
    }

////InitDrawToScreen(pdd);

    //
    // dont change Decompressors on a non key frame, unless we have
    // to (ie getting clipped while decompressing to screen)
    //
    if (pdd->ulFlags & DDF_NEWPALETTE)
    {
	if (wFlags & DDF_NOTKEYFRAME)
	{
	    if (pdd->iDecompress == DECOMPRESS_BUFFER)
	    {
		DPF(("waiting for a key frame to change decompressor (palette change)"));
		pdd->ulFlags |= DDF_WANTKEY;
		return;
	    }
	}
    }
    else
    {
        if (wFlags & DDF_NOTKEYFRAME)
        {
            if (pdd->iDecompress != DECOMPRESS_SCREEN) // !(pdd->ulFlags & DDF_SCREENX))
            {
                DPF(("waiting for a key frame to change (clipped) decompressor"));
                pdd->ulFlags |= DDF_WANTKEY;
                return;
            }
        }
    }

    InitDecompress(pdd);
    pdd->ulFlags &= ~DDF_WANTKEY;
}

/**************************************************************************
* @doc INTERNAL
*
* @api void | DrawDibPalChange | called when the physical palette mapping
*   has changed.
*
**************************************************************************/
static void DrawDibPalChange(PDD pdd, HDC hdc, HPALETTE hpal)
{
#ifndef _WIN32
#ifdef DEBUG
	extern BOOL FAR PASCAL IsDCCurrentPalette(HDC hdc);

	BOOL fForeground = IsDCCurrentPalette(hdc);

        if (fForeground)
	    DPF(("Palette mapping has changed (foreground)..."));
	else
            DPF(("Palette mapping has changed (background)..."));
#endif
#endif

    //
    // if we are on a palette device we need to do some special stuff.
    //
    if (gwScreenBitDepth == 8 && (gwRasterCaps & RC_PALETTE))
    {
        //
        // get the logical->physical mapping
        //
        if (GetPhysDibPaletteMap(hdc, &pdd->biDraw, pdd->uiPalUse, pdd->ab))
            pdd->ulFlags |= DDF_IDENTITYPAL;
        else
	    pdd->ulFlags &= ~DDF_IDENTITYPAL;

        if (pdd->ulFlags & DDF_IDENTITYPAL)
            DPF(("Palette mapping is 1:1"));
	else
	    DPF(("Palette mapping is not 1:1"));

#ifdef DAYTONA // !!! Not on Chicago!
	if (pdd->ulFlags & DDF_IDENTITYPAL) {
            DPF(("using DIB_PAL_INDICES"));
	    pdd->uiPalUse = DIB_PAL_INDICES;
	} else {
	    pdd->uiPalUse = DIB_PAL_COLORS;
        }
#endif
    }
    else
    {
        //
        // we are not on a palette device, some code checks DDF_IDENTITYPAL
        // anyway so set it.
        //
        pdd->ulFlags |= DDF_IDENTITYPAL;
    }

    if (pdd->hbmDraw && (pdd->ulFlags & DDF_BITMAP))
    {
        //!!! we should pass pdd->ab to this function!
        //!!! and use a naked translate.
        SetBitmapColorChange(&pdd->sd, hdc, hpal);
    }

    DrawDibClipChange(pdd, DDF_NOTKEYFRAME);
}

/**************************************************************************
* @doc INTERNAL
*
* @api HPALETTE | CreateBIPalette | Create palette from bitmap.
*
* @parm LPBITMAPINFOHEADER | lpbi | Pointer to bitmap.
*
* @rdesc Returns handle to the palette, NULL if error.
*
**************************************************************************/
STATICFN HPALETTE CreateBIPalette(HPALETTE hpal, LPBITMAPINFOHEADER lpbi)
{
    LPRGBQUAD prgb;
    int i;

    // This structure is the same as LOGPALETTE EXCEPT for the array of
    // palette entries which here is 256 long.  The "template" in the
    // SDK header files only has an array of size one, hence the "duplication".
    struct {
	WORD         palVersion;                /* tomor - don't mess with word */
	WORD         palNumEntries;
	PALETTEENTRY palPalEntry[256];
    }   pal;

    pal.palVersion = 0x300;
    pal.palNumEntries = (int)lpbi->biClrUsed;

    if (pal.palNumEntries == 0 && lpbi->biBitCount <= 8)
        pal.palNumEntries = (1 << (int)lpbi->biBitCount);

    if (pal.palNumEntries == 0)
        return NULL;

    prgb = (LPRGBQUAD)(lpbi+1);

    for (i=0; i<(int)pal.palNumEntries; i++)
    {
        pal.palPalEntry[i].peRed   = prgb[i].rgbRed;
        pal.palPalEntry[i].peGreen = prgb[i].rgbGreen;
        pal.palPalEntry[i].peBlue  = prgb[i].rgbBlue;
        pal.palPalEntry[i].peFlags = 0;
    }

    if (hpal)
    {
	ResizePalette(hpal, pal.palNumEntries);
	SetPaletteEntries(hpal, 0, pal.palNumEntries, pal.palPalEntry);
    }
    else
    {
	hpal = CreatePalette((LPLOGPALETTE)&pal);
    }

    return hpal;
}

/**************************************************************************
* @doc INTERNAL
*
* @api BOOL | SetPalFlags | Modifies the palette flags.
*
* @parm HPALETTE | hpal | Handle to the palette.
*
* @parm int | iIndex | Starting palette index.
*
* @parm int | cntEntries | Number of entries to set flags on.
*
* @parm UINT | wFlags | Palette flags.
*
* @rdesc Returns TRUE if successful, FALSE otherwise.
*
**************************************************************************/
STATICFN BOOL SetPalFlags(HPALETTE hpal, int iIndex, int cntEntries, UINT wFlags)
{
    int     i;
    PALETTEENTRY ape[256];

    if (hpal == NULL)
        return FALSE;

    if (cntEntries < 0) {
	cntEntries = 0; // GetObject returns 2 bytes
        GetObject(hpal,sizeof(int),(LPSTR)&cntEntries);
    }

    GetPaletteEntries(hpal, iIndex, cntEntries, ape);

    for (i=0; i<cntEntries; i++)
        ape[i].peFlags = (BYTE)wFlags;

    return SetPaletteEntries(hpal, iIndex, cntEntries, ape);
}


/**************************************************************************
* @doc INTERNAL
*
* @api BOOL | IsIdentityPalette | Check if palette is an identity palette.
*
* @parm HPALETTE | hpal | Handle to the palette.
*
* @rdesc Returns TRUE if the palette is an identity palette, FALSE otherwise.
*
**************************************************************************/

#define CODE _based(_segname("_CODE"))

//
// These are the standard VGA colors, we will be stuck with until the
// end of time!
//
static PALETTEENTRY CODE apeCosmic[16] = {
    0x00, 0x00, 0x00, 0x00,     // 0000  black
    0x80, 0x00, 0x00, 0x00,     // 0001  dark red
    0x00, 0x80, 0x00, 0x00,     // 0010  dark green
    0x80, 0x80, 0x00, 0x00,     // 0011  mustard
    0x00, 0x00, 0x80, 0x00,     // 0100  dark blue
    0x80, 0x00, 0x80, 0x00,     // 0101  purple
    0x00, 0x80, 0x80, 0x00,     // 0110  dark turquoise
    0xC0, 0xC0, 0xC0, 0x00,     // 1000  gray
    0x80, 0x80, 0x80, 0x00,     // 0111  dark gray
    0xFF, 0x00, 0x00, 0x00,     // 1001  red
    0x00, 0xFF, 0x00, 0x00,     // 1010  green
    0xFF, 0xFF, 0x00, 0x00,     // 1011  yellow
    0x00, 0x00, 0xFF, 0x00,     // 1100  blue
    0xFF, 0x00, 0xFF, 0x00,     // 1101  pink (magenta)
    0x00, 0xFF, 0xFF, 0x00,     // 1110  cyan
    0xFF, 0xFF, 0xFF, 0x00      // 1111  white
    };

static PALETTEENTRY CODE apeFake[16] = {
    0x00, 0x00, 0x00, 0x00,     // 0000  black
    0xBF, 0x00, 0x00, 0x00,     // 0001  dark red
    0x00, 0xBF, 0x00, 0x00,     // 0010  dark green
    0xBF, 0xBF, 0x00, 0x00,     // 0011  mustard
    0x00, 0x00, 0xBF, 0x00,     // 0100  dark blue
    0xBF, 0x00, 0xBF, 0x00,     // 0101  purple
    0x00, 0xBF, 0xBF, 0x00,     // 0110  dark turquoise
    0xC0, 0xC0, 0xC0, 0x00,     // 1000  gray
    0x80, 0x80, 0x80, 0x00,     // 0111  dark gray
    0xFF, 0x00, 0x00, 0x00,     // 1001  red
    0x00, 0xFF, 0x00, 0x00,     // 1010  green
    0xFF, 0xFF, 0x00, 0x00,     // 1011  yellow
    0x00, 0x00, 0xFF, 0x00,     // 1100  blue
    0xFF, 0x00, 0xFF, 0x00,     // 1101  pink (magenta)
    0x00, 0xFF, 0xFF, 0x00,     // 1110  cyan
    0xFF, 0xFF, 0xFF, 0x00,     // 1111  white
    };

static PALETTEENTRY CODE apeBlackWhite[16] = {
    0x00, 0x00, 0x00, 0x00,     // 0000  black
    0x00, 0x00, 0x00, 0x00,     // 0000  black
    0x00, 0x00, 0x00, 0x00,     // 0000  black
    0x00, 0x00, 0x00, 0x00,     // 0000  black
    0x00, 0x00, 0x00, 0x00,     // 0000  black
    0x00, 0x00, 0x00, 0x00,     // 0000  black
    0x00, 0x00, 0x00, 0x00,     // 0000  black
    0x00, 0x00, 0x00, 0x00,     // 0000  black
    0x00, 0x00, 0x00, 0x00,     // 0000  black
    0x00, 0x00, 0x00, 0x00,     // 0000  black
    0x00, 0x00, 0x00, 0x00,     // 0000  black
    0x00, 0x00, 0x00, 0x00,     // 0000  black
    0x00, 0x00, 0x00, 0x00,     // 0000  black
    0x00, 0x00, 0x00, 0x00,     // 0000  black
    0xFF, 0xFF, 0xFF, 0x00      // 1111  white
    };

STATICFN BOOL NEAR IsIdentityPalette(HPALETTE hpal)
{
    int i,n=0;    // n is initialised as GetObject returns a 2 byte value
    HDC hdc;

    PALETTEENTRY ape[256];
    PALETTEENTRY apeSystem[16];

    if (hpal == NULL || !(gwRasterCaps & RC_PALETTE) || gwScreenBitDepth != 8)
        return FALSE;

    // Some wierd display cards actually have different numbers of system
    // colours! We definitely don't want to think we can do identity palettes.
    hdc = GetDC(NULL);
    n = GetDeviceCaps(hdc, NUMRESERVED);
    ReleaseDC(NULL, hdc);

    if (n != 20)
	return FALSE;

    GetObject(hpal, sizeof(n), (LPVOID)&n);

    if (n != 256)
	return FALSE;

    GetPaletteEntries(hpal, 0,	 8, &ape[0]);
    GetPaletteEntries(hpal, 248, 8, &ape[8]);

    for (i=0; i<16; i++)
	ape[i].peFlags = 0;

    if (!_fmemcmp(ape, apeCosmic, sizeof(apeCosmic)))
        goto DoneChecking;

    if (!_fmemcmp(ape, apeFake, sizeof(apeFake)))
	goto DoneChecking;

    if (!_fmemcmp(ape, apeBlackWhite, sizeof(apeBlackWhite)))
        goto DoneChecking;

    hdc = GetDC(NULL);
    GetSystemPaletteEntries(hdc, 0,   8, &apeSystem[0]);
    GetSystemPaletteEntries(hdc, 248, 8, &apeSystem[8]);
    ReleaseDC(NULL, hdc);

    for (i=0; i<16; i++)
	apeSystem[i].peFlags = 0;

    if (!_fmemcmp(ape, apeSystem, sizeof(apeSystem)))
        goto DoneChecking;

    return FALSE;

DoneChecking:
    //
    // if we have an identity palette then, patch the colors to match
    // the driver ones exactly.
    //
    GetPaletteEntries(hpal, 0, 256, ape);

    hdc = GetDC(NULL);
    GetSystemPaletteEntries(hdc, 0,   10, &ape[0]);
    GetSystemPaletteEntries(hdc, 246, 10, &ape[246]);
    ReleaseDC(NULL, hdc);

    for (i=0; i<10; i++)
        ape[i].peFlags = 0;

    for (i=10; i<246; i++)
        ape[i].peFlags = PC_NOCOLLAPSE;

    for (i=246; i<256; i++)
        ape[i].peFlags = 0;

    SetPaletteEntries(hpal, 0, 256, ape);
    DPF(("Calling UnrealizeObject on hpal==%8x\n", hpal));
    UnrealizeObject(hpal);      //??? needed

    return TRUE;
}

#define COLORMASK 0xF8
STATICFN BOOL NEAR AreColorsAllGDIColors(LPBITMAPINFOHEADER lpbi)
{
    int	    cColors;
    LPRGBQUAD lprgb = (LPRGBQUAD) ((LPBYTE) lpbi + lpbi->biSize);
    int	    i;

    for (cColors = (int) lpbi->biClrUsed; cColors > 0; cColors--, lprgb++) {
	for (i = 0; i < 16; i++) {
	    if (((lprgb->rgbRed & COLORMASK) == (apeCosmic[i].peRed & COLORMASK)) &&
		((lprgb->rgbGreen & COLORMASK) == (apeCosmic[i].peGreen & COLORMASK)) &&
		((lprgb->rgbBlue & COLORMASK) == (apeCosmic[i].peBlue & COLORMASK)))
		goto Onward;
	
	    if (((lprgb->rgbRed & COLORMASK) == (apeFake[i].peRed & COLORMASK)) &&
		((lprgb->rgbGreen & COLORMASK) == (apeFake[i].peGreen & COLORMASK)) &&
		((lprgb->rgbBlue & COLORMASK) == (apeFake[i].peBlue & COLORMASK)))
		goto Onward;
	}

	return FALSE;
Onward:
	;	// There's got to be a nicer way to arrange this code!
    }

    return TRUE; // !!!!!
}

#if 0
#ifdef _WIN32
/*
 * check if the system palette is identical to the palette we want
 * to draw with. This should be the same as checking both IsIdentityPalette
 * and also that we have the foreground window. If the palettes are the same,
 * then set a flag showing that we can safely use DIB_PAL_INDICES instead of
 * DIB_PAL_COLORS.
 *
 * On NT at least, DIB_PAL_INDICES saves a large amount of time from the
 * critical GDI drawing call (SetDIBitsToDevice). But we can only use it
 * if our palette is really the same as the system palette. This function
 * should be called from the WM_NEWPALETTE message so that every time
 * a new palette is realised (by us or anyone else) we will accurately set
 * this flag.
 */
static void DrawDibCheckPalette(PDD pdd)
{
    PALETTEENTRY apeSystem[256];
    PALETTEENTRY apeLocal[256];
    UINT palcount = 0;  // GetObject stores two bytes.  Rest of code prefers 32 bits
    HDC hdc;
    HPALETTE hpal;

    hpal = (pdd->hpalDraw ? pdd->hpalDraw : pdd->hpal);

    if (hpal == NULL)
        return;

    /*
     * check that it is 8-bit colour in use
     */

    if (gwScreenBitDepth != 8 || !(gwRasterCaps & RC_PALETTE))
	return ;

    GetObject(hpal, sizeof(palcount), (LPVOID)&palcount);

    if (palcount != 256)
	return ;

    /*
     * read all the system palette
     */
    hdc = GetDC(NULL);
    GetSystemPaletteEntries(hdc, 0, 256, apeSystem);
    ReleaseDC(NULL, hdc);

    /* read local palette entries */
    GetPaletteEntries(hpal, 0, 256, apeLocal);

    /* compare colours */
#define BETTER_PAL_INDICES   // Faster when the result is DIB_PAL_INDICES
#ifdef BETTER_PAL_INDICES    // but slower when DIB_PAL_COLORS is the outcome
			     // unless the quick check is OK
    StartCounting();
    if (apeLocal[17].peRed == apeSystem[17].peRed) { // Quick check
	for (palcount=256; palcount--; ) {
	    apeLocal[palcount].peFlags = apeSystem[palcount].peFlags = 0;
	}  // its a shame we have to clear the flags out

	if (!memcmp(apeLocal, apeSystem, sizeof(apeSystem))) {
	    /* all ok - we can use INDICES */
	    RPF(("\tUsing PAL_INDICES"));
	    pdd->uiPalUse = DIB_PAL_INDICES;
	    EndCounting("(memcmp) DIB_PAL_INDICES");
	    return;
        }
    }
    /* comparison failed - forget it */
    RPF(("\tUsing DIB_PAL_COLORS"));
    pdd->uiPalUse = DIB_PAL_COLORS;
    EndCounting("(memcmp) DIB_PAL_COLORS");

#else

    StartCounting();
    for (palcount = 0; palcount < 256; palcount++) {
	if ((apeLocal[palcount].peRed != apeSystem[palcount].peRed) ||
	    (apeLocal[palcount].peGreen != apeSystem[palcount].peGreen) ||
	    (apeLocal[palcount].peBlue != apeSystem[palcount].peBlue))  {

		/* comparison failed - forget it */
		DPF(("\tUsing DIB_PAL_COLORS  Failed with palcount=%d",palcount));

		pdd->uiPalUse = DIB_PAL_COLORS;
		EndCounting("         DIB_PAL_COLORS");
		return;
	}
    }

    /* all ok - we can use INDICES */
    RPF(("\tUsing PAL_INDICES"));
    pdd->uiPalUse = DIB_PAL_INDICES;
    EndCounting("         DIB_PAL_INDICES");
#endif
}

#endif
#endif



/**************************************************************************

let codec adapt to the system palette.

**************************************************************************/

static BOOL FixUpCodecPalette(HIC hic, LPBITMAPINFOHEADER lpbi)
{
    struct {
	BITMAPINFOHEADER bi;
        RGBQUAD          argbq[256];
    } s;
    int                 i;
    HDC                 hdc;

    s.bi.biSize           = sizeof(s.bi);
    s.bi.biWidth          = lpbi->biWidth;
    s.bi.biHeight         = lpbi->biHeight;
    s.bi.biPlanes         = 1;
    s.bi.biBitCount       = 8;
    s.bi.biCompression    = 0;
    s.bi.biSizeImage      = 0;
    s.bi.biXPelsPerMeter  = 0;
    s.bi.biYPelsPerMeter  = 0;
    s.bi.biClrUsed        = 256;
    s.bi.biClrImportant   = 0;

    hdc = GetDC(NULL);
    GetSystemPaletteEntries(hdc, 0, 256, (LPPALETTEENTRY) &s.argbq);
    ReleaseDC(NULL, hdc);

    for (i = 0; i < 256; i++)
	((DWORD FAR*)s.argbq)[i] = i < 8 || i >= 248 ? 0 :
	    RGB(s.argbq[i].rgbRed,s.argbq[i].rgbGreen,s.argbq[i].rgbBlue);

    return ICDecompressSetPalette(hic, &s.bi) == ICERR_OK;
}

/**************************************************************************

let codec adapt to a palette passed by the app.

**************************************************************************/

static BOOL NEAR SendSetPalette(PDD pdd)
{
    int  i;
    int  iPalColors = 0;
    BOOL f;

    if (pdd->hic == NULL)               // nobody to send too
        return FALSE;

    if (pdd->biBuffer.biBitCount != 8)  // not decompressing to 8bit
        return FALSE;

    if (!(gwRasterCaps & RC_PALETTE))   // not a palette device who cares.
        return FALSE;

    if (pdd->hpalDraw)
    {
        GetObject(pdd->hpalDraw, sizeof(iPalColors), (void FAR *)&iPalColors);

        if (iPalColors == 0)
            return FALSE;

        if (iPalColors > 256)
            iPalColors = 256;

        pdd->biBuffer.biClrUsed = iPalColors;
        GetPaletteEntries(pdd->hpalDraw, 0, iPalColors, (PALETTEENTRY FAR *)pdd->argbq);

        for (i = 0; i < iPalColors; i++)
            ((DWORD*)pdd->argbq)[i] = RGB(pdd->argbq[i].rgbRed,pdd->argbq[i].rgbGreen,pdd->argbq[i].rgbBlue);

        f = ICDecompressSetPalette(pdd->hic, &pdd->biBuffer) == ICERR_OK;
        ICDecompressGetPalette(pdd->hic, pdd->lpbi, &pdd->biBuffer);
    }
    else
    {
        pdd->biBuffer.biClrUsed = pdd->ClrUsed;
        f = ICDecompressSetPalette(pdd->hic, NULL) == ICERR_OK;
        ICDecompressGetPalette(pdd->hic, pdd->lpbi, &pdd->biBuffer);
    }

    return f;
}

#ifdef DEBUG_RETAIL

#define _WINDLL
#include <stdarg.h>
#include <stdio.h>

void FAR CDECL ddprintf(LPSTR szFormat, ...)
{
    char ach[128];
    va_list va;
    UINT n;

    if (fDebug == -1)
        fDebug = mmGetProfileIntA("Debug", MODNAME, FALSE);

    if (!fDebug)
        return;

    va_start(va, szFormat);
#ifdef _WIN32
    if ('+' == *szFormat) {
	n = 0;
	++szFormat;
    } else {
	n = sprintf(ach, MODNAME ": (tid %x) ", GetCurrentThreadId());
    }

    n += vsprintf(ach+n, szFormat, va);
#else
    lstrcpy(ach, MODNAME ": ");
    n = lstrlen(ach);
    n += wvsprintf(ach+n, szFormat, va);
#endif
    va_end(va);
    if ('+' == ach[n-1]) {
	--n;
    } else {
	ach[n++] = '\r';
	ach[n++] = '\n';
    }
    ach[n] = 0;
    OutputDebugStringA(ach);
}
#endif
