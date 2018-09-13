
/*****************************************************************************

                        C L I P B O O K   D I S P L A Y

    Name:       clipdsp.c
    Date:       21-Jan-1994
    Creator:    Unknown

    Description:
        This module handles the drawing of the clipbook displays.

    History:
        19-Apr-1994 John Fu     Changed the priority of CF_UNICODETEXT to
                                be higher than CF_TEXT.
                                Changed DIB display to use DIB_RGB_COLORS.
                                Made menus for all displayable formats above
                                all nondisplayables.

        16-Jun-1994 John Fu     Fix ShowString() to allocate bigger buffer
                                if the buffer is filled out completely.

        15-Mar-1995 John Fu     Changed DrawDib to use DIB_RGB_COLORS only.

*****************************************************************************/





#define WIN31
#include <windows.h>
#include "common.h"
#include "clipbook.h"
#include "clpbkrc.h"
#include "clipbrd.h"
#include "clipdsp.h"
#include "debugout.h"
#include "cvutil.h"




#define MAXBITSPERPIXEL     24
#define ifmtMax             (sizeof(rgfmt)/sizeof(WORD))



static MFENUMPROC   lpEnumMetaProc;


BOOL                fOwnerDisplay;
TCHAR               szMemErr[100];
HBRUSH              hbrBackground;
HMENU               hDispMenu;




/* The scroll information for OWNER display is to be preserved, whenever
 * the display changes between OWNER and NON-OWNER; The following globals
 * are used to save and restore the scroll info.
 */

// winball: since only the Clipboard window supports owner display,
// this info is not replicated for each MDI child...

int   OwnVerMin;
int   OwnVerMax;
int   OwnHorMin;
int   OwnHorMax;

int   OwnVerPos;
int   OwnHorPos;



/* Defines priority order for show format */
WORD   rgfmt[] = {
                  CF_OWNERDISPLAY,

                  CF_UNICODETEXT,   // JYF, moved CF_UNICODETEXT above CF_TEXT
                  CF_TEXT,
                  CF_OEMTEXT,


                  CF_ENHMETAFILE,
                  CF_METAFILEPICT,
                  CF_DIB,
                  CF_BITMAP,

                  CF_DSPTEXT,
                  CF_DSPBITMAP,
                  CF_DSPMETAFILEPICT,
                  CF_DSPENHMETAFILE,

                  CF_PALETTE,
                  CF_RIFF,
                  CF_WAVE,
                  CF_PENDATA,
                  CF_SYLK,
                  CF_DIF,
                  CF_TIFF,
                  CF_LOCALE
                  };



void ShowString( HWND, HDC, WORD);










/*
 *      MyOpenClipBoard
 */

BOOL MyOpenClipboard(
    HWND    hWnd)
{
HDC   hDC;
RECT  Rect;

    if( VOpenClipboard( GETMDIINFO(hWnd)->pVClpbrd, hWnd ))
        return(TRUE);


    PERROR(TEXT("MyOpenClipboard fail\r\n"));


    /* Some stupid app forgot to close the clipboard */
    hDC = GetDC(hWnd);

    GetClientRect(hWnd, (LPRECT)&Rect);
    FillRect(hDC, (LPRECT)&Rect, hbrBackground);
    ShowString( hWnd, hDC, IDS_ALREADYOPEN);

    ReleaseDC(hWnd, hDC);


    return(FALSE);

}






/*
 *      SetCharDimensions
 */

void SetCharDimensions(
    HWND    hWnd,
    HFONT   hFont)

{
register HDC    hdc;
TEXTMETRIC      tm;
PMDIINFO        pMDI;



    pMDI = GETMDIINFO(hWnd);


    hdc = GetDC(hWnd);
    SelectObject(hdc, hFont);
    GetTextMetrics(hdc, (LPTEXTMETRIC)&tm);
    ReleaseDC(hWnd, hdc);

    pMDI->cxChar         = (WORD)tm.tmAveCharWidth;
    pMDI->cxMaxCharWidth = (WORD)tm.tmMaxCharWidth;
    pMDI->cyLine         = (WORD)(tm.tmHeight + tm.tmExternalLeading);
    pMDI->cxMargin       = pMDI->cxChar / 2;
    pMDI->cyMargin       = pMDI->cyLine / 4;

}






/*
 *      ChangeCharDimensions
 */

void ChangeCharDimensions(
    HWND    hwnd,
    UINT    wOldFormat,
    UINT    wNewFormat)
{
    /* Check if the font has changed. */
    if (wOldFormat == CF_OEMTEXT)
        {
        if (wNewFormat != CF_OEMTEXT)       // Select default system font sizes
            SetCharDimensions(hwnd, GetStockObject ( SYSTEM_FONT ) );
        }
    else if (wNewFormat == CF_OEMTEXT)      // Select OEM font sizes
        SetCharDimensions(hwnd, GetStockObject ( OEM_FIXED_FONT ) );
}






/*
 *      ClipbrdVScroll
 *
 *  Scroll contents of window vertically, according to action code in wParam.
 */

void ClipbrdVScroll (
    HWND    hwnd,
    WORD    wParam,
    WORD    wThumb)
{
int         cyWindow;
long        dyScroll;
long        cyScrollT;
long        dyScrollAbs;
long        cyPartialChar;
PMDIINFO    pMDI;



    pMDI = GETMDIINFO(hwnd);

    /* Ensure that all the bits are valid first, before scrolling them */
    UpdateWindow(hwnd);

    cyScrollT = pMDI->cyScrollNow;
    cyWindow = pMDI->rcWindow.bottom - pMDI->rcWindow.top;

    /* Compute scroll results as an effect on cyScrollNow */
    switch (wParam)
        {
        case SB_LINEUP:
            cyScrollT -= pMDI->cyLine;
            break;

        case SB_LINEDOWN:
            cyScrollT += pMDI->cyLine;
            break;

        case SB_THUMBPOSITION:
            cyScrollT = (LONG)(((LONG)wThumb * pMDI->cyScrollLast) / VPOSLAST);
            break;

        case SB_PAGEUP:
        case SB_PAGEDOWN:
            {
            int   cyPageScroll;

            cyPageScroll = cyWindow - pMDI->cyLine;

            if (cyPageScroll < (int)(pMDI->cyLine))
                cyPageScroll = pMDI->cyLine;

            cyScrollT += (wParam == SB_PAGEUP) ? -cyPageScroll : cyPageScroll;
            break;
            }

        default:
            return;
        }



    if ((cyScrollT < 0) || (pMDI->cyScrollLast <= 0))
        cyScrollT = 0;
    else if (cyScrollT > pMDI->cyScrollLast)
        cyScrollT = pMDI->cyScrollLast;
    else if (cyPartialChar = cyScrollT % pMDI->cyLine)
        {
        /* Round to the nearest character increment. */
        if (cyPartialChar > ((int)(pMDI->cyLine) >> 1))
            cyScrollT += pMDI->cyLine;
            cyScrollT -= cyPartialChar;
        }



    dyScroll = pMDI->cyScrollNow - cyScrollT;

    if (dyScroll > 0)
        dyScrollAbs = dyScroll;
    else if (dyScroll < 0)
        dyScrollAbs = -dyScroll;
    else
        return;             /* Scrolling has no effect here. */

    pMDI->cyScrollNow = cyScrollT;

    if (dyScrollAbs >= pMDI->rcWindow.bottom - pMDI->rcWindow.top)
        /* ScrollWindow does not handle this case */
        InvalidateRect(hwnd, (LPRECT)&(pMDI->rcWindow), TRUE);
    else
        ScrollWindow(hwnd, 0,(int)dyScroll, &(pMDI->rcWindow), &(pMDI->rcWindow));


    UpdateWindow(hwnd);

    SetScrollPos (pMDI->hwndVscroll,
                  SB_CTL,
                  (pMDI->cyScrollLast <= 0) ?
                  0 :
                  (int)((cyScrollT * (DWORD)VPOSLAST) / pMDI->cyScrollLast),
                  TRUE);

}







/*
 *      ClipbrdHScroll
 *
 *  Scroll contents of window horizontally, according to op code in wParam.
 */

void ClipbrdHScroll (
    HWND    hwnd,
    WORD    wParam,
    WORD    wThumb)
{
register int    dxScroll;
register int    cxScrollT;
int             cxWindow;
int             dxScrollAbs;
int             cxPartialChar;
PMDIINFO        pMDI;



    pMDI = GETMDIINFO(hwnd);

    cxScrollT = pMDI->cxScrollNow;
    cxWindow = pMDI->rcWindow.right - pMDI->rcWindow.left;

    /* Compute scroll results as an effect on cxScrollNow */
    switch (wParam)
        {
        case SB_LINEUP:
            cxScrollT -= pMDI->cxChar;
            break;

        case SB_LINEDOWN:
            cxScrollT += pMDI->cxChar;
            break;

        case SB_THUMBPOSITION:
            cxScrollT = (int)(((LONG)wThumb * (LONG)pMDI->cxScrollLast) / HPOSLAST);
            break;

        case SB_PAGEUP:
        case SB_PAGEDOWN:
            {
            int   cxPageScroll;

            cxPageScroll = cxWindow - pMDI->cxChar;
            if (cxPageScroll < (int)(pMDI->cxChar))
                cxPageScroll = pMDI->cxChar;

            cxScrollT += (wParam == SB_PAGEUP) ? -cxPageScroll : cxPageScroll;
            break;
            }

        default:
            return;
        }



    if ((cxScrollT < 0) || (pMDI->cxScrollLast <= 0))
        cxScrollT = 0;
    else if (cxScrollT > pMDI->cxScrollLast)
        cxScrollT = pMDI->cxScrollLast;
    else if (cxPartialChar = cxScrollT % pMDI->cxChar)
        { /* Round to the nearest character increment */
        if (cxPartialChar > ((int)(pMDI->cxChar) >> 1))
            cxScrollT += pMDI->cxChar;
            cxScrollT -= cxPartialChar;
        }



    /* Now we have a good cxScrollT value */

    dxScroll = pMDI->cxScrollNow - cxScrollT;
    if (dxScroll > 0)
        dxScrollAbs = dxScroll;
    else if (dxScroll < 0)
        dxScrollAbs = -dxScroll;
    else
        return;             /* Scrolling has no effect here. */


    pMDI->cxScrollNow = cxScrollT;

    if (dxScrollAbs >= pMDI->rcWindow.right - pMDI->rcWindow.left)
        /* ScrollWindow does not handle this case */
        InvalidateRect( hwnd, (LPRECT) &(pMDI->rcWindow), TRUE );
    else
        ScrollWindow(hwnd, dxScroll, 0, (LPRECT)&(pMDI->rcWindow),
        (LPRECT)&(pMDI->rcWindow));

    UpdateWindow(hwnd);

    SetScrollPos (pMDI->hwndHscroll,
                  SB_CTL,
                  (pMDI->cxScrollLast <= 0) ?
                  0 :
                  (int)(((DWORD)cxScrollT * (DWORD)HPOSLAST) / (DWORD)(pMDI->cxScrollLast)),
                  TRUE);

}






/*
 *      DibPaletteSize
 */

int DibPaletteSize(
    LPBITMAPINFOHEADER  lpbi)
{
register int bits;
register int nRet;

    /* With the new format headers, the size of the palette is in biClrUsed
     * else is dependent on bits per pixel.
     */

    if (lpbi->biSize != sizeof(BITMAPCOREHEADER))
       {
       if (lpbi->biClrUsed != 0)
          {
          nRet = lpbi->biClrUsed * sizeof(RGBQUAD);
          }
       else
          {
          bits = lpbi->biBitCount;

          if (24 == bits)
             {
             nRet = 0;
             }
          else if (16 == bits || 32 == bits)
             {
             nRet = 3 * sizeof(DWORD);
             }
          else
             {
             nRet = (1 << bits) * sizeof(RGBQUAD);
             }
          }
       }
    else
       {
       bits = ((LPBITMAPCOREHEADER)lpbi)->bcBitCount;
       nRet = (bits == 24) ? 0 : (1 << bits) * sizeof(RGBTRIPLE);
       }


    return(nRet);

}








/*
 *      DibGetInfo
 */

void DibGetInfo(
    HANDLE      hdib,
    LPBITMAP    pbm)

{
LPBITMAPINFOHEADER lpbi;

    lpbi = (LPBITMAPINFOHEADER)GlobalLock(hdib);

    if (lpbi->biSize != sizeof(BITMAPCOREHEADER))
        {
        pbm->bmWidth  = (int)lpbi->biWidth;
        pbm->bmHeight = (int)lpbi->biHeight;
        }
    else
        {
        pbm->bmWidth  = (int)((LPBITMAPCOREHEADER)lpbi)->bcWidth;
        pbm->bmHeight = (int)((LPBITMAPCOREHEADER)lpbi)->bcHeight;
        }

    GlobalUnlock(hdib);
}








/*
 *      DrawDib
 */

BOOL DrawDib(
    HWND    hwnd,
    HDC     hdc,
    int     x0,
    int     y0,
    HANDLE  hdib)
{
LPBITMAPINFOHEADER  lpbi;
BITMAP              bm;
LPSTR               lpBits;
BOOL                fOK;

    if (hdib)
        {
        lpbi = (LPBITMAPINFOHEADER)GlobalLock(hdib);

        if (lpbi)
            {
            DibGetInfo(hdib, (LPBITMAP)&bm);

            lpBits = (LPSTR)lpbi + (WORD)lpbi->biSize + DibPaletteSize(lpbi);

            SetDIBitsToDevice (hdc,
                               x0,
                               y0,
                               bm.bmWidth,
                               bm.bmHeight,
                               0,
                               0,
                               0,
                               bm.bmHeight,
                               lpBits,
                               (LPBITMAPINFO)lpbi,
                               DIB_RGB_COLORS);

            GlobalUnlock(hdib);
            fOK = TRUE;
            }
        }

    return(fOK);

}








/*
 *      FShowDIBitmap
 */

BOOL FShowDIBitmap (
    HWND            hwnd,
    register HDC    hdc,
    PRECT           prc,
    HANDLE          hdib,   //Bitmap in DIB format
    int             cxScroll,
    int             cyScroll)
{
BITMAP    bm;
PMDIINFO pMDI;

    pMDI = GETMDIINFO(hwnd);
    DibGetInfo(hdib, (LPBITMAP)&bm);


    // If window's been resized, determine maximum scroll positions.
    if (pMDI->cyScrollLast == -1)
        {
        /* Compute last scroll offset into bitmap */
        pMDI->cyScrollLast = bm.bmHeight -
            (pMDI->rcWindow.bottom - pMDI->rcWindow.top);
        if (pMDI->cyScrollLast < 0)
           {
           pMDI->cyScrollLast = 0;
           }
        }

    if (pMDI->cxScrollLast == -1)
        {
        /* Compute last scroll offset into bitmap */
        pMDI->cxScrollLast = bm.bmWidth -
            (pMDI->rcWindow.right - pMDI->rcWindow.left);
        if (pMDI->cxScrollLast < 0)
           {
           pMDI->cxScrollLast = 0;
           }
        }

    SaveDC(hdc);
    IntersectClipRect (hdc, prc->left, prc->top, prc->right, prc->bottom);
    SetViewportOrgEx (hdc,prc->left - cxScroll, prc->top - cyScroll,NULL);
    DrawDib (hwnd, hdc, 0, 0, hdib);
    RestoreDC(hdc, -1);

    return(TRUE);

}







/*
 *      FShowBitmap
 */

BOOL FShowBitmap (
    HWND            hwnd,
    HDC             hdc,
    register PRECT  prc,
    HBITMAP         hbm,
    int             cxScroll,
    int             cyScroll)
{
register HDC    hMemDC;
BITMAP          bitmap;
int             cxBlt, cyBlt;
int             cxRect, cyRect;
PMDIINFO        pMDI;

    pMDI = GETMDIINFO(hwnd);

    if ((hMemDC = CreateCompatibleDC(hdc)) == NULL)
        return(FALSE);

    if (!SelectObject(hMemDC, (HBITMAP)hbm))
        {
        DeleteDC(hMemDC);
        // kind of a hack... want to display a more informative
        // message, so put up our own message and return OK
        ShowString( hwnd, hdc, IDS_BADBMPFMT );
        return TRUE;
        }

    GetObject((HBITMAP)hbm, sizeof(BITMAP), (LPSTR)&bitmap);

    // does this DDB match the DC? (clausgi)
    //  if ( bitmap.bmPlanes != GetDeviceCaps(hMemDC, PLANES) ||
    //         bitmap.bmBitsPixel != GetDeviceCaps(hMemDC, BITSPIXEL ) ) {
    //      DeleteDC(hMemDC);
          // kind of a hack... want to display a more informative
          // message, so put up our own message and return OK
    //       ShowString( hwnd, hdc, IDS_BADBMPFMT );
    //      return TRUE;
    //   }

    if (pMDI->cyScrollLast == -1)
        {
        /* Compute last scroll offset into bitmap */
        pMDI->cyScrollLast = bitmap.bmHeight - (pMDI->rcWindow.bottom - pMDI->rcWindow.top);
        if (pMDI->cyScrollLast < 0)
            pMDI->cyScrollLast = 0;
        }

    if ( pMDI->cxScrollLast == -1)
        {
         /* Compute last scroll offset into bitmap */
        pMDI->cxScrollLast = bitmap.bmWidth - (pMDI->rcWindow.right - pMDI->rcWindow.left);
        if ( pMDI->cxScrollLast < 0)
            pMDI->cxScrollLast = 0;
        }


    cxRect = prc->right - prc->left;
    cyRect = prc->bottom - prc->top;
    cxBlt = min(cxRect, bitmap.bmWidth - cxScroll);
    cyBlt = min(cyRect, bitmap.bmHeight - cyScroll);

    BitBlt (hdc,
            prc->left,
            prc->top,
            cxBlt,
            cyBlt,
            hMemDC,
            cxScroll,
            cyScroll,    /* X,Y offset into source DC */
            SRCCOPY);

    DeleteDC(hMemDC);

    return(TRUE);

}






#define DXPAL  (pMDI->cyLine)
#define DYPAL  (pMDI->cyLine)

////////////////////////////////////////////////////////////////////////////
//
//  FShowPalette()
//
//  Parameters:
//    hwnd - The wMDI child we're drawing in.
//    hdc - DC for the window.
//    prc - Rectangle to draw.
//    hpal - The palette to display.
//    cxScroll, cyScroll - Scroll position in pels OF PRC. NOT OF THE WINDOW.
//       Derive window scroll position by doing a cxScroll -= pMDI->cxScrollNow
//
////////////////////////////////////////////////////////////////////////////

BOOL FShowPalette(
    HWND hwnd,
    register HDC    hdc,
    register PRECT  prc,
    HPALETTE        hpal,
    int             cxScroll,
    int             cyScroll)
{
int         n;
int         x, y;
int         nx, ny;
int         nNumEntries;
RECT        rc;
HBRUSH      hbr;
PMDIINFO    pMDI;
BOOL        fOK = FALSE;
TCHAR       achHex[] = TEXT("0123456789ABCDEF");
int         nFirstLineDrawn;



    PINFO(TEXT("Palette: (%d,%d-%d,%d),cx %d, cy %d\r\n"),
       prc->left, prc->top, prc->right, prc->bottom, cxScroll, cyScroll);


    pMDI = GETMDIINFO(hwnd);

    if (hpal)
       {
       // Correct cyScroll to show window's scroll position, not prc's.
       cyScroll -= prc->top - pMDI->rcWindow.top;
       PINFO(TEXT("Corrected cyScroll %d\r\n"), cyScroll);

       // GetObject does not return an int-- it returns a USHORT. Thus,
       // we zero out nNumEntries before getobjecting the palette.
       nNumEntries = 0;
       GetObject(hpal, sizeof(int), (LPSTR)&nNumEntries);

       // Figure how many boxes across and tall the array of color boxes
       // is
       nx = ((pMDI->rcWindow.right - pMDI->rcWindow.left) / DXPAL);
       if (nx == 0)
          {
          nx = 1;
          }
       ny = (nNumEntries + nx - 1) / nx;
       PINFO(TEXT("%d entries, %d by %d array\r\n"), nNumEntries, nx, ny);

       // If the window's been resized, we have to tell it how far you
       // can scroll off to the right and down.
       if ( pMDI->cyScrollLast == -1)
          {
          pMDI->cyScrollLast = ny * DYPAL -                  // Height of palette minus
                pMDI->rcWindow.bottom - pMDI->rcWindow.top + // height of window plus
                DYPAL;                                       // one palette entry height.

          if ( pMDI->cyScrollLast < 0)
             {
             pMDI->cyScrollLast = 0;
             }
          PINFO(TEXT("Last allowed scroll: %d\r\n"), pMDI->cyScrollLast);
          }
       if ( pMDI->cxScrollLast == -1)
          {
          /* Can't scroll palettes horizontally. */
          pMDI->cxScrollLast = 0;
          }

       SaveDC(hdc);
       IntersectClipRect(hdc, prc->left, prc->top, prc->right, prc->bottom);
       // SetMapMode(hdc, MM_TEXT);
       SetWindowOrgEx(hdc, -pMDI->rcWindow.left, -pMDI->rcWindow.top, NULL);

       // Set up the x and y positions of the first palette entry to draw
       // and figure out which palette entry IS the first that needs drawing.
       x = 0;
       nFirstLineDrawn = (cyScroll + prc->top - pMDI->rcWindow.top)/ DYPAL;
       n = nx * nFirstLineDrawn;
       y = DYPAL * nFirstLineDrawn - cyScroll; // + pMDI->rcWindow.top;
       PINFO(TEXT("First entry %d at %d, %d\r\n"), n, x, y);

       // While n < number of entries and the current entry isn't off the bottom
       // of the window
       while (n < nNumEntries && y < prc->bottom)
          {
          // Figure out a DXPAL by DYPAL rect going down/right from x,y
          rc.left   = x;
          rc.top    = y;
          rc.right  = rc.left + DXPAL;
          rc.bottom = rc.top + DYPAL;
          // PINFO(TEXT("(%d,%d) "), rc.left, rc.top);

          // Draw a black box with the appropriate color inside.
          if (RectVisible(hdc, &rc))
             {
             // PINFO(TEXT("<"));

             // If you change this one to zero, you get a text display of
             // the palette indices-- I used it to debug the draw code, 'cause
             // it's damn near impossible, when you've got little colored
             // squares, to figure out just which color is on the bottom of THAT
             // square THERE, the one that was scrolled halfway off the bottom
             // of the window, and you just scrolled it on. ("Well, it's sorta
             // purple... of course, this entire stupid palette is sorta purple..")
             #if 1
             InflateRect(&rc, -1, -1);
             FrameRect(hdc, &rc, GetStockObject(BLACK_BRUSH));
             InflateRect(&rc, -1, -1);
             hbr = CreateSolidBrush(PALETTEINDEX(n));
             FillRect(hdc, &rc, hbr);
             DeleteObject(hbr);
             #else
             SetBkMode(hdc, TRANSPARENT);
             TextOut(hdc, rc.left + 2, rc.top + 2, &achHex[(n / 16)&0x0f], 1);
             TextOut(hdc, (rc.left + rc.right) / 2, rc.top + 2,
                   &achHex[n & 0x0f], 1);
             #endif
             }

          // Go to next entry and advance x to the next position, "word
          // wrapping" to next line if we need to
          n++;
          x += DXPAL;
          if (0 == n % nx)
             {
             x = 0;
             y += DYPAL;
             PINFO(TEXT("Wrap at %d\r\n"), n);
             }
          }
       RestoreDC(hdc, -1);
       fOK = TRUE;
       }
    else
       {
       PERROR(TEXT("Bad palette!\r\n"));
       }
    return(fOK);
}








/*
 *      PxlConvert
 *
 * Return the # of pixels spanned by 'val', a measurement in coordinates
 * appropriate to mapping mode mm.  'pxlDeviceRes' gives the resolution
 * of the device in pixels, along the axis of 'val'. 'milDeviceRes' gives
 * the same resolution measurement, but in millimeters.
 */

int PxlConvert(
    int mm,
    int val,
    int pxlDeviceRes,
    int milDeviceRes)
{
register WORD   wMult = 1;
register WORD   wDiv = 1;
DWORD           ulPxl;
DWORD           ulDenom;
/* Should be a constant!  This works around a compiler bug as of 07/14/85. */
DWORD           ulMaxInt = 0x7FFF;

    if (milDeviceRes == 0)
        {
        /* to make sure we don't get divide-by-0 */
        return(0);
        }

    switch (mm)
        {
        case MM_LOMETRIC:
            wDiv = 10;
            break;

        case MM_HIMETRIC:
            wDiv = 100;
            break;

        case MM_TWIPS:
            wMult = 254;
            wDiv = 14400;
            break;

        case MM_LOENGLISH:
            wMult = 2540;
            wDiv = 10000;
            break;

        case MM_HIENGLISH:
            wMult = 254;
            wDiv = 10000;
            break;

        case MM_TEXT:
            return(val);

        case MM_ISOTROPIC:
        case MM_ANISOTROPIC:
            /* These picture types have no original size */
        default:
            return(0);
        }

    /* Add denominator - 1 to numerator, to avoid roundoff */

    ulDenom = (DWORD)wDiv * (DWORD)milDeviceRes;
    ulPxl = (((DWORD)((DWORD)wMult * (DWORD)val * (DWORD)pxlDeviceRes)) + ulDenom - 1) / ulDenom;

    return((ulPxl > ulMaxInt) ? 0 : (int)ulPxl);
}









/*
 *      FShowEnhMetaFile
 *
 * Display an enhanced metafile in the specified rectangle.
 */

BOOL FShowEnhMetaFile(
    HWND            hwnd,
    register HDC    hdc,
    register PRECT  prc,
    HANDLE          hemf,
    int             cxScroll,
    int             cyScroll)
{
int         cxBitmap;
int         cyBitmap;
RECT        rcWindow;
int         f = FALSE;
PMDIINFO    pMDI;



    pMDI = GETMDIINFO(hwnd);

    /* Not scrollable.  Resize these into the given rect. */

    pMDI->cyScrollLast = 0;
    pMDI->cxScrollLast = 0;

    cxBitmap = pMDI->rcWindow.right - pMDI->rcWindow.left;
    cyBitmap = pMDI->rcWindow.bottom - pMDI->rcWindow.top;


    /* We make the "viewport" to be an area the same size as the
     * clipboard object, and set the origin and clip region so as
     * to show the area we want. Note that the viewport may well be
     * bigger than the window.
     */

    SetMapMode(hdc, MM_TEXT);

    rcWindow.left   = prc->left - cxScroll;
    rcWindow.top    = prc->top  - cyScroll;
    rcWindow.right  = rcWindow.left + cxBitmap;
    rcWindow.bottom = rcWindow.top  + cyBitmap;

    f = PlayEnhMetaFile (hdc, hemf, &rcWindow);



    // Always return TRUE. PlayEnhMetaFile() can return
    // FALSE even when the metafile can be displayed
    // properly.  Things such as printer escap can cause
    // the call to return FALSE when painting to screen
    // but the image will be displayed fine.
    //
    // We return TRUE so we don't blank the display and
    // put "Clipbook can't display..." message.

    return TRUE;

}





/*
 *      EnumMetafileProc
 *
 *  Metafile record play callback function used to work around problem
 *  with non active MDI children playing a metafile that causes a foreground
 *  palette selection
 */

BOOL CALLBACK EnumMetafileProc (
    HDC             hdc,
    HANDLETABLE FAR *lpht,
    METARECORD FAR  *lpmr,
    int             cObj,
    LPARAM          lParam )
{
    if ( lpmr->rdFunction == META_SELECTPALETTE )
       {
       return SelectPalette ( hdc, lpht[(lpmr->rdParm[0])].objectHandle[0],
             TRUE ) != NULL;
       }
    else
       {
       PlayMetaFileRecord ( hdc, lpht, lpmr, cObj );
       return TRUE;
       }
}








/*
 *      FShowMetaFilePict
 *
 *  Display a metafile in the specified rectangle.
 */

BOOL FShowMetaFilePict(
    HWND            hwnd,
    register HDC    hdc,
    register PRECT  prc,
    HANDLE          hmfp,
    int             cxScroll,
    int             cyScroll)
{
int             level;
int             cxBitmap;
int             cyBitmap;
int             f = FALSE;
LPMETAFILEPICT  lpmfp;
PMDIINFO        pMDI;



    pMDI = GETMDIINFO(hwnd);


    if ((lpmfp = (LPMETAFILEPICT)GlobalLock( hmfp )) != NULL)
        {
        METAFILEPICT mfp;

        mfp = *lpmfp;
        GlobalUnlock( hmfp );

        if ((level = SaveDC( hdc )) != 0)
            {

            /* Compute size of picture to be displayed */
            switch (mfp.mm)
                {
                case MM_ISOTROPIC:
                case MM_ANISOTROPIC:
                    /* Not scrollable.  Resize these into the given rect. */
                    pMDI->cyScrollLast = 0;
                    pMDI->cxScrollLast = 0;
                    cxBitmap = pMDI->rcWindow.right - pMDI->rcWindow.left;
                    cyBitmap = pMDI->rcWindow.bottom - pMDI->rcWindow.top;
                    break;

                default:
                    cxBitmap = PxlConvert(mfp.mm, mfp.xExt, GetDeviceCaps(hdc, HORZRES), GetDeviceCaps(hdc, HORZSIZE));
                    cyBitmap = PxlConvert(mfp.mm, mfp.yExt, GetDeviceCaps(hdc, VERTRES), GetDeviceCaps(hdc, VERTSIZE));
                    if (!cxBitmap || !cyBitmap)
                        {
                        goto NoDisplay;
                        }

                    if ( pMDI->cxScrollLast == -1)
                        {
                        pMDI->cxScrollLast =
                            cxBitmap - (pMDI->rcWindow.right - pMDI->rcWindow.left);
                        if ( pMDI->cxScrollLast < 0)
                            {
                            pMDI->cxScrollLast = 0;
                            }
                        }

                    if (pMDI->cyScrollLast == -1)
                        {
                        pMDI->cyScrollLast =
                            cyBitmap - (pMDI->rcWindow.bottom - pMDI->rcWindow.top);
                        if (pMDI->cyScrollLast < 0)
                            {
                            pMDI->cyScrollLast = 0;
                            }
                        }
                    break;
                }

                /* We make the "viewport" to be an area the same size as the
                 * clipboard object, and set the origin and clip region so as
                 * to show the area we want. Note that the viewport may well be
                 * bigger than the window.
                 */
                // IntersectClipRect(hdc, prc->left, prc->top, prc->right, prc->bottom);
                SetMapMode(hdc, mfp.mm);

                SetViewportOrgEx(hdc, prc->left - cxScroll, prc->top - cyScroll, NULL);
                switch (mfp.mm)
                    {
                    case MM_ISOTROPIC:
                        if (mfp.xExt && mfp.yExt)
                           {
                           // So we get the correct shape rectangle when
                           // SetViewportExt gets called.
                           //
                           SetWindowExtEx(hdc, mfp.xExt, mfp.yExt, NULL);
                           }
                        //else
                        //  FALL THRU

                    case MM_ANISOTROPIC:
                        SetViewportExtEx(hdc, cxBitmap, cyBitmap, NULL);
                        break;
                    }

            /* Since we may have scrolled, force brushes to align */
            SetBrushOrgEx(hdc, cxScroll - prc->left, cyScroll - prc->top, NULL);

            // f = PlayMetaFile(hdc, mfp.hMF);
            f = EnumMetaFile(hdc, mfp.hMF, EnumMetafileProc, 0L );
            FreeProcInstance ( (FARPROC) lpEnumMetaProc );

      NoDisplay:
            RestoreDC(hdc, level);
           }
        }


    return(f);
}






/*
 *      ShowString
 *
 *  Blank rcWindow and show the string on the top line of the client area
 */

void ShowString(
    HWND    hwnd,
    HDC     hdc,
    WORD    id)
{
TCHAR   szBuffer[BUFFERLEN];
LPTSTR  pszBuffer   = szBuffer;
INT     iBufferSize = BUFFERLEN;

INT     iStringLen;


    /* Cancel any scrolling effects. */
    GETMDIINFO(hwnd)->cyScrollNow = 0;
    GETMDIINFO(hwnd)->cxScrollNow = 0;


    iStringLen = LoadString(hInst, id, pszBuffer, BUFFERLEN);


    // Is the buffer completely filled out?
    // We need a bigger one if yes.

    while (iStringLen == BUFFERLEN -1)
        {
        if (pszBuffer != szBuffer && pszBuffer)
            LocalFree (pszBuffer);

        iBufferSize *= 2;
        pszBuffer = LocalAlloc (LPTR, iBufferSize);

        if (!pszBuffer)
            goto done;

        iStringLen = LoadString (hInst, id, pszBuffer, iBufferSize);
        }



    FillRect (hdc, &(GETMDIINFO(hwnd)->rcWindow), hbrBackground);
    DrawText (hdc, pszBuffer, -1, &(GETMDIINFO(hwnd)->rcWindow),
              DT_CENTER | DT_WORDBREAK | DT_TOP);



done:

    if (pszBuffer != szBuffer && pszBuffer)
        LocalFree (pszBuffer);

}









/*
 *      CchLineA
 *
 *
 * Determine the # of characters in one display line's worth of lpch.
 * lpch is assumed to be an ansi string.
 *
 * Return the following:
 *       HI WORD:    # of chars to display (excludes CR, LF; will not
 *                   exceed cchLine)
 *       LO WORD:    offset of start of next line in lpch; If the current line
 *                   is NULL terminated, this contains offset to the NULL char;
 *       In RgchBuf: characters to display
 *
 *   Expands Tabs
 *
 *   Accepts any of the following as valid end-of-line terminators:
 *       CR, LF, CR-LF, LF-CR, NULL
 *   Callers may test for having reached NULL by (lpch[LOWORD] == '\0')
 */

LONG CchLineA(
    PMDIINFO    pMDI,
    HDC         hDC,
    CHAR        rgchBuf[],
    CHAR FAR    *lpch,
    INT         cchLine,
    WORD        wWidth)
{
CHAR            ch;
CHAR            *pch = rgchBuf;
register INT    cchIn = 0;
register INT    cchOut = 0;
INT             iMinNoOfChars;
SIZE            size;
INT             iTextWidth = 0;



    iMinNoOfChars = wWidth / pMDI->cxMaxCharWidth;

    while (cchOut < cchLine)
        {
        switch (ch = *(lpch + (DWORD)cchIn++))
            {
            case '\0':
                /* cchIn is already incremented; So, it is pointing to
                 * a character beyond the NULL; So, decrement it.
                 */
                cchIn--;
                goto DoubleBreak;

            case '\015':  /* CR */
            case '\012':  /* LF */
                if ((lpch[cchIn] == '\015') || (lpch[cchIn] == '\012'))
                    cchIn++;
                goto DoubleBreak;

            case '\011':  /* TAB */
                {
                INT   cchT = 8 - (cchOut % 8);

                /* Check if the width has exceeded or the total
                 * number of characters has exceeded
                 */
                if (((WORD)(iTextWidth + cchT * pMDI->cxChar) > wWidth) || ((cchOut+cchT) >= cchLine))
                   /* Tab causes wrap to next line */
                    goto DoubleBreak;

                while (cchT--)
                    rgchBuf[cchOut++] = ' ';
                break;
                }

            default:
                rgchBuf[cchOut++] = ch;
                if( IsDBCSLeadByte(ch) )
                    rgchBuf[cchOut++] = *(lpch + (DWORD)cchIn++);

            break;
            }

        /* Check if the width has been exceeded. */
        if (cchOut >= iMinNoOfChars)
            {
            GetTextExtentPointA(hDC, rgchBuf, cchOut, (LPSIZE)&size);
            iTextWidth = size.cx;
            if ((WORD)iTextWidth == wWidth)
                break;
            else if((WORD)iTextWidth > wWidth)
                {
                    if (IsDBCSLeadByte(ch))
                        {
                        cchOut--;
                        cchIn--;
                        }

                cchOut--;
                cchIn--;
                break;
                }

            iMinNoOfChars += (wWidth - iTextWidth) / pMDI->cxMaxCharWidth;
            }
        }


DoubleBreak:
    return(MAKELONG(cchIn, cchOut));

}






/*--------------------------------------------------------------------------*/
/*                                                                            */
/*  CchLineW() -                                                             */
/*                                                                            */
/*--------------------------------------------------------------------------*/

/*Same as previous function but takes unicode strings.
 */

LONG CchLineW(
    PMDIINFO    pMDI,
    HDC         hDC,
    WCHAR       rgchBuf[],
    WCHAR FAR   *lpch,
    INT         cchLine,
    WORD        wWidth)
{
register INT    cchIn = 0;
register INT    cchOut = 0;
WCHAR           ch;
WCHAR           *pch = rgchBuf;
INT             iMinNoOfChars;
INT             iTextWidth = 0;
SIZE            size;


    iMinNoOfChars = wWidth / pMDI->cxMaxCharWidth;

    while (cchOut < cchLine)
        {
        switch (ch = *(lpch + (DWORD)cchIn++))
            {
            case L'\0':
                 /* cchIn is already incremented; So, it is pointing to
                 * a character beyond the NULL; So, decrement it.
                 */
                cchIn--;
                goto DoubleBreak;

            case L'\015':  /* CR */
            case L'\012':  /* LF */
                if ((lpch[cchIn] == L'\015') || (lpch[cchIn] == L'\012'))
                    cchIn++;
                goto DoubleBreak;

            case L'\011':  /* TAB */
                {
                INT   cchT = 8 - (cchOut % 8);

                /* Check if the width has exceeded or the total
                 * number of characters has exceeded
                 */
                if (((WORD)(iTextWidth + cchT * pMDI->cxChar) > wWidth) || ((cchOut+cchT) >= cchLine))
                    /* Tab causes wrap to next line */
                    goto DoubleBreak;

                while (cchT--)
                    rgchBuf[cchOut++] = L' ';
                break;
                }

            default:
                rgchBuf[cchOut++] = ch;
                break;
            }


        /* Check if the width has been exceeded. */
        if (cchOut >= iMinNoOfChars)
            {
            GetTextExtentPointW(hDC, rgchBuf, cchOut, &size);
            iTextWidth = size.cx;
            if ((WORD)iTextWidth == wWidth)
                break;
            else if((WORD)iTextWidth > wWidth)
                {
                  cchOut--;
                  cchIn--;
                  break;
                }

            iMinNoOfChars += (wWidth - iTextWidth) / pMDI->cxMaxCharWidth;
            }
        }


DoubleBreak:

  return(MAKELONG(cchIn, cchOut));

}






#define cchLineMax  200

/*--------------------------------------------------------------------------*/
/*                                                                            */
/*  ShowText() -                                                            */
/*                                                                            */
/*--------------------------------------------------------------------------*/

void ShowText(
    HWND            hwnd,
    register HDC    hdc,
    PRECT           prc,
    HANDLE          h,
    INT             cyScroll,
    BOOL            fUnicode)
{
#ifdef WIN16
  CHAR huge *lpch;
#else
  CHAR FAR  *lpch;
#endif

INT       yT;
INT       cLine;
INT       cLineAllText = 0;
RECT      rc;
INT       yLine;
INT       iLineFirstShow;
WORD      wLen;
WORD      wWidth;
CHAR      rgch[cchLineMax*sizeof(WCHAR)];
PMDIINFO  pMDI;





    pMDI= GETMDIINFO(hwnd);

    rc = *prc;

    /* Expand repaint rectangle as necessary to hold an exact number of
     * lines and start on an even line boundary. This is because we may
     * get arbitrarily weird repaint rectangles when popups are moved.
     * Scrolling repaint areas should require no adjustment.
     */

    rc.top -= (rc.top - pMDI->rcWindow.top) % pMDI->cyLine;



    /* If expanding the repaint rectangle to the next line expands it */
    /* beyond the bottom of my window, contract it one line.          */
    if ((yT = (rc.bottom - rc.top) % pMDI->cyLine) != 0)
        if ((rc.bottom += pMDI->cyLine - yT) > pMDI->rcWindow.bottom)
            rc.bottom -= pMDI->cyLine;

    if (rc.bottom <= rc.top)
        return;

    if (((wWidth = (WORD)(pMDI->rcWindow.right - pMDI->rcWindow.left)) <= 0) ||
        ((cLine = (rc.bottom - rc.top) / pMDI->cyLine) <= 0)         ||
        (NULL == (lpch = (LPSTR)GlobalLock(h))) )
        {
        /* Bad Rectangle or Bad Text Handle */
        ShowString(hwnd, hdc, IDS_ERROR);
        return;
        }



    /* Advance lpch to point at the text for the first line to show. */
    iLineFirstShow = cyScroll / pMDI->cyLine;


    /* Advance lpch to point at text for that line. */
    if (!fUnicode)
        while ((*lpch) && (iLineFirstShow--))
            {
            lpch += LOWORD(CchLineA(pMDI,hdc, rgch, lpch, cchLineMax, wWidth));
            cLineAllText++;
            }
    else
        while ((*((WCHAR *)lpch)) && (iLineFirstShow--))
            {
            lpch += ((LOWORD(CchLineW(pMDI, hdc, (WCHAR *)rgch, (WCHAR FAR *)lpch,
                  cchLineMax, wWidth)))*sizeof(WCHAR));
            cLineAllText++;
            }


    /* Display string, line by line */
    yLine = rc.top;
    while (cLine--)
        {
        LONG lT;

        if (!fUnicode)
            {
            lT = CchLineA(pMDI, hdc, rgch, lpch, cchLineMax, wWidth);
            }
        else
            {
            lT = CchLineW(pMDI, hdc, (WCHAR *)rgch, (WCHAR FAR *)lpch, cchLineMax, wWidth);
            }
        wLen = LOWORD(lT);
        if (!fUnicode)
            {
            TextOutA(hdc, rc.left, yLine, (LPSTR) rgch, HIWORD(lT));
            lpch += wLen;
            }
        else
            {
            if (!TextOutW(hdc, rc.left, yLine, (LPCWSTR) rgch, HIWORD(lT)))
                {
                GetLastError();
                }
            lpch += (wLen * sizeof(WCHAR));
            }
        yLine += pMDI->cyLine;
        cLineAllText++;
        if ((!fUnicode && (*lpch == 0)) || (fUnicode && (*((WCHAR *)lpch) == L'\0')))
            {
            break;
            }
        }


    if (pMDI->cxScrollLast == -1)
        {
        /* We don't use horiz scroll for text */
        pMDI->cxScrollLast = 0;
        }

    if (pMDI->cyScrollLast == -1)
        {
        INT   cLineInRcWindow;

        /* Validate y-size of text in clipboard. */
        /* Adjust rcWindow dimensions for text display */
        cLineInRcWindow = (pMDI->rcWindow.bottom - pMDI->rcWindow.top) / pMDI->cyLine;

        do {
           if (!fUnicode)
               {
               lpch += LOWORD(CchLineA(pMDI, hdc, rgch, lpch, cchLineMax, wWidth));
               }
           else
               {
               lpch += ((LOWORD(CchLineW(pMDI, hdc, (WCHAR *)rgch,
                   (WCHAR FAR *)lpch, cchLineMax, wWidth)))*sizeof(WCHAR));
               }
           cLineAllText++;
           }
           while ((!fUnicode && (*lpch != 0)) || (fUnicode && ((*lpch != 0) || (*(lpch+1) != 0))));

        pMDI->cyScrollLast = (cLineAllText - cLineInRcWindow) * pMDI->cyLine;
        if (pMDI->cyScrollLast < 0)
            {
            pMDI->cyScrollLast = 0;
            }

       /* Restrict rcWindow so that it holds an exact # of text lines */
        pMDI->rcWindow.bottom = pMDI->rcWindow.top + (cLineInRcWindow * pMDI->cyLine);
        }


    GlobalUnlock(h);

}











/*
 *      SendOwnerMessage
 */

void SendOwnerMessage(
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam)
{
register HWND hwndOwner;

    /* Send a message to the clipboard owner, if there is one */
    hwndOwner = GetClipboardOwner();

    if (hwndOwner != NULL)
        SendMessage(hwndOwner, message, wParam, lParam);

}









/*
 *      SendOwnerSizeMessage
 *
 *  Send WM_SIZECLIPBOARD message to clipboard owner.
 *    wParam is a handle to the clipboard window
 *    LOWORD(lParam) is a handle to the passed rect
 */

void SendOwnerSizeMessage (
    HWND    hwnd,
    int     left,
    int     top,
    int     right,
    int     bottom)
{
register HANDLE hrc;
LPRECT          lprc;



    if ((hrc = GlobalAlloc (GMEM_MOVEABLE | GMEM_LOWER, (LONG)sizeof(RECT))) != NULL )
        {
        if ((lprc = (LPRECT)GlobalLock(hrc)) != NULL )
            {
            lprc->top    = top;
            lprc->bottom = bottom;
            lprc->left   = left;
            lprc->right  = right;
            GlobalUnlock(hrc);
            SendOwnerMessage(WM_SIZECLIPBOARD, (WPARAM)hwnd, (LPARAM)hrc);
            }
        GlobalFree(hrc);
        }

}









/*
 *      GetBestFormat
 *
 *  This routine decides which one of the existing formats is to be
 *  displayed in the viewer.
 */

UINT GetBestFormat(
    HWND    hwnd,
    UINT    wFormat)
{
register WORD   cFmt;
register WORD   *pfmt;


    // PINFO(TEXT("GBFormat %d\r\n"), wFormat);

    if (wFormat == CBM_AUTO)
        {
        for (cFmt=ifmtMax, pfmt=&rgfmt[0]; cFmt--; pfmt++)
            {
            // PINFO(TEXT("Looking at # %d, (%d)\r\n"), cFmt, *pfmt);
            if ( VIsClipboardFormatAvailable( GETMDIINFO(hwnd)->pVClpbrd, *pfmt ))
                {
                return(*pfmt);
                }
            }
        return(0);              /* Just in case... */
        }

    return(wFormat);

}











/*
 *      GetClipboardName
 */

void GetClipboardName (
    register int    fmt,
    LPTSTR          szName,
    register int    iSize)
{
LPTSTR  lprgch;
HANDLE  hrgch;



    *szName = '\0';


    /* Get global memory that everyone can get to */
    if ((hrgch = GlobalAlloc(GMEM_MOVEABLE | GMEM_LOWER, (LONG)(iSize + 1))) == NULL)
        {
        PERROR(TEXT("GetClipboardName: alloc failure\n\r"));
        return;
        }

    if (!(lprgch = (LPTSTR)GlobalLock(hrgch)))
        goto ExitPoint;

    switch (fmt)
        {
        // These are all of the formats we have know the names of.
        case CF_RIFF:
        case CF_WAVE:
        case CF_PENDATA:
        case CF_SYLK:
        case CF_DIF:
        case CF_TIFF:

        case CF_TEXT:
        case CF_UNICODETEXT:
        case CF_OEMTEXT:
        case CF_DSPTEXT:
        case CF_LOCALE:

        case CF_BITMAP:
        case CF_DIB:
        case CF_PALETTE:
        case CF_DSPBITMAP:

        case CF_METAFILEPICT:
        case CF_DSPMETAFILEPICT:
        case CF_ENHMETAFILE:
        case CF_DSPENHMETAFILE:
        case CF_HDROP:
            LoadString(hInst, fmt, lprgch, iSize);
            break;

        case CF_OWNERDISPLAY:         /* Clipbrd owner app supplies name */
            *lprgch = '\0';
            SendOwnerMessage(WM_ASKCBFORMATNAME, (WPARAM)iSize, (LPARAM)(LPSTR)lprgch);

            if (!*lprgch)
                LoadString(hInst, fmt, lprgch, iSize);
            break;

        default:
            *lprgch = '\0';
            GetClipboardFormatName(fmt, lprgch, iSize);
            break;
        }

    lstrcpy(szName, lprgch);

    GlobalUnlock(hrgch);


ExitPoint:
    GlobalFree(hrgch);

}



//Seperate menu item and DDE transaction Data.

/*
 *      GetClipboardMenuName
 */

void GetClipboardMenuName (
    register int    fmt,
    LPTSTR          szName,
    register int    iSize)
{
LPTSTR  lprgch;
HANDLE  hrgch;



    *szName = '\0';


    /* Get global memory that everyone can get to */
    if ((hrgch = GlobalAlloc(GMEM_MOVEABLE | GMEM_LOWER, (LONG)(iSize + 1))) == NULL)
        {
        PERROR(TEXT("GetClipboardName: alloc failure\n\r"));
        return;
        }

    if (!(lprgch = (LPTSTR)GlobalLock(hrgch)))
        goto ExitPoint;

    switch (fmt)
        {
        // These are all of the formats we have know the names of.
        case CF_RIFF:
        case CF_WAVE:
        case CF_PENDATA:
        case CF_SYLK:
        case CF_DIF:
        case CF_TIFF:

        case CF_TEXT:
        case CF_UNICODETEXT:
        case CF_OEMTEXT:
        case CF_DSPTEXT:

        case CF_BITMAP:
        case CF_DIB:
        case CF_PALETTE:
        case CF_DSPBITMAP:

        case CF_METAFILEPICT:
        case CF_DSPMETAFILEPICT:
        case CF_ENHMETAFILE:
        case CF_DSPENHMETAFILE:

        case CF_HDROP:
        case CF_LOCALE:
            LoadString(hInst, fmt+MNDELTA, lprgch, iSize);
            break;

        case CF_OWNERDISPLAY:         /* Clipbrd owner app supplies name */
            *lprgch = '\0';
            SendOwnerMessage(WM_ASKCBFORMATNAME, (WPARAM)iSize, (LPARAM)(LPSTR)lprgch);

            if (!*lprgch)
                LoadString(hInst, CF_MN_OWNERDISPLAY, lprgch, iSize);
            break;

        default:
            GetClipboardFormatName(fmt, lprgch, iSize);
            break;
        }

    lstrcpy(szName, lprgch);

    GlobalUnlock(hrgch);


ExitPoint:
    GlobalFree(hrgch);

}








/*
 *      DrawFormat
 *
 * Parameters:
 *    hdc - the hdc to draw in.
 *    prc - The rectangle to paint
 *    cxScroll - The scroll position of the window.
 *    cyScroll - The scroll position OF THE PAINT RECTANGLE. NOT THE WINDOW.
 *       (Gawd. Who DESIGNED this?) Measured in pels.
 *    BestFormat - The format to draw.
 *    hwndMDI - The window we're drawing in.
 *
 */

void DrawFormat(
    register HDC    hdc,
    PRECT           prc,
    int             cxScroll,
    int             cyScroll,
    WORD            BestFormat,
    HWND            hwndMDI)
{
register HANDLE h;
HFONT           hFont;
int             fOK = TRUE;
WORD            wFormat = 0;
PMDIINFO        pMDI;



    pMDI = GETMDIINFO(hwndMDI);

    PINFO(TEXT("DrawFormat: (%d, %d), %d"), cxScroll, cyScroll, BestFormat);

    if (hwndMDI == hwndClpbrd && pMDI->pVClpbrd)
        {
        PERROR(TEXT("Clipboard window shouldn't have vClp!\r\n"));
        }


    /* If "Auto" is chosen and only data in unrecognised formats is
     * available, then display "Can't display data in this format".
     */
    if ((BestFormat == 0) &&
        VCountClipboardFormats( pMDI->pVClpbrd ))
        {
        if ((wFormat = (WORD)RegisterClipboardFormat(TEXT("FileName"))) &&
             VIsClipboardFormatAvailable(pMDI->pVClpbrd, wFormat))
            {
            BestFormat = CF_TEXT;
            }
        else
            {
            PINFO(TEXT("no displayable format\n\r"));
            ShowString( hwndMDI, hdc, IDS_CANTDISPLAY);
            return;
            }
        }


    PINFO(TEXT("format %x\n\r"), BestFormat);

    h = VGetClipboardData( pMDI->pVClpbrd, wFormat ? wFormat : BestFormat );




    if ( h != NULL)
        {
        PINFO(TEXT("Got format %x from VGetClipboardData\n\r"), BestFormat );

        switch (BestFormat)
            {

            case CF_DSPTEXT:
            case CF_TEXT:
                ShowText( hwndMDI, hdc, prc, h, cyScroll, FALSE);
                break;

            case CF_UNICODETEXT:
                hFont = SelectObject(hdc, hfontUni);
                ShowText(hwndMDI, hdc, prc, h, cyScroll, TRUE);
                SelectObject(hdc, hFont);
                break;

            case CF_OEMTEXT:
                hFont = SelectObject(hdc, GetStockObject ( OEM_FIXED_FONT ) );
                ShowText(hwndMDI, hdc, prc, h, cyScroll, FALSE);
                SelectObject(hdc, hFont);
                break;

            case CF_DSPBITMAP:
            case CF_BITMAP:
                fOK = FShowBitmap( hwndMDI, hdc, prc, h, cxScroll, cyScroll);
                break;

            case CF_DIB:
                fOK = FShowDIBitmap( hwndMDI, hdc, prc, h, cxScroll, cyScroll);
                break;

            case CF_PALETTE:
                fOK = FShowPalette( hwndMDI, hdc, prc, h, cxScroll, cyScroll);
                break;

            case CF_WAVE:
            case CF_RIFF:
            case CF_PENDATA:
            case CF_DIF:
            case CF_SYLK:
            case CF_TIFF:
            case CF_LOCALE:
                ShowString( hwndMDI, hdc, IDS_BINARY);
                break;

            case CF_DSPMETAFILEPICT:
            case CF_METAFILEPICT:
                fOK = FShowMetaFilePict( hwndMDI, hdc, prc, h, cxScroll, cyScroll);
                break;

            case CF_DSPENHMETAFILE:
            case CF_ENHMETAFILE:
                fOK = FShowEnhMetaFile( hwndMDI, hdc, prc, h, cxScroll, cyScroll);
                break;

            default:
                ShowString( hwndMDI, hdc, IDS_BINARY);
                break;
            }

        // Disable scroll bars that don't work
        EnableWindow(pMDI->hwndVscroll, pMDI->cyScrollLast > 1 ? TRUE : FALSE);
        EnableWindow(pMDI->hwndHscroll, pMDI->cxScrollLast > 1 ? TRUE : FALSE);
        }
    else
        {
        PERROR(TEXT("VGetClpDta fail\r\n"));
        }

    /* Check if the Data was not rendered by the application */
    if ((h == NULL) &&
        VCountClipboardFormats( pMDI->pVClpbrd ))
        {
        ShowString( hwndMDI, hdc, IDS_NOTRENDERED);
        }
    else
        {
        /* If we are unable to display the data, display "<Error>" */
        if (!fOK)
            {
            ShowString( hwndMDI, hdc, IDS_ERROR);
            }
        }

}









/*
 *      DrawStuff
 *
 *  Paint portion of current clipboard contents given by PAINT struct
 *  NOTE: If the paintstruct rectangle includes any part of the header, the
 *    whole header is redrawn.
 */

void DrawStuff(
    HWND                    hwnd,
    register PAINTSTRUCT    *pps,
    HWND                    hwndMDI)
{
register HDC    hdc;
RECT            rcPaint;
RECT            rcClient;
WORD            BestFormat;
PMDIINFO        pMDI;



    pMDI = GETMDIINFO(hwnd);
    hdc  = pps->hdc;


    if (pps->fErase)
        FillRect(hdc, (LPRECT)&pps->rcPaint, hbrBackground);

    GetClientRect(hwnd, (LPRECT)&rcClient);




    // make room for scroll controls:

    BestFormat = (WORD)GetBestFormat( hwnd, pMDI->CurSelFormat );

    fOwnerDisplay = (BestFormat == CF_OWNERDISPLAY);

    if ( !fOwnerDisplay )
        {
        ShowScrollBar ( hwnd, SB_BOTH, FALSE );
        rcClient.right  -= GetSystemMetrics ( SM_CXVSCROLL );
        rcClient.bottom -= GetSystemMetrics ( SM_CYHSCROLL );
        }




    /* If the display format has changed, Set rcWindow,
     * the display area for clip info.
     */

    if ( pMDI->fDisplayFormatChanged )
        {
        CopyRect((LPRECT)&(pMDI->rcWindow), (LPRECT)&rcClient);

        /* We have changed the size of the clipboard. Tell the owner,
         * if fOwnerDisplay is active.
         */

        // ShowHideControls ( hwnd );

        if (fOwnerDisplay)
            {
            SendOwnerSizeMessage(hwnd,
                                 pMDI->rcWindow.left,
                                 pMDI->rcWindow.top,
                                 pMDI->rcWindow.right,
                                 pMDI->rcWindow.bottom);
            }
        else
            {
            /* Give the window a small margin, for looks */
            InflateRect (&(pMDI->rcWindow),
                         -(int)(pMDI->cxMargin),
                         -(int)(pMDI->cyMargin));
            }

        pMDI->fDisplayFormatChanged = FALSE;
        }

    if (fOwnerDisplay)
        {
        /* Clipboard Owner handles display */
        HANDLE hps;

        hps = GlobalAlloc(GMEM_MOVEABLE | GMEM_LOWER, (LONG)sizeof(PAINTSTRUCT));

        if (hps != NULL)
            {
            LPPAINTSTRUCT lppsT;

            if ((lppsT = (LPPAINTSTRUCT)GlobalLock(hps)) != NULL)
                {
                *lppsT = *pps;
                IntersectRect(&lppsT->rcPaint, &pps->rcPaint, &(pMDI->rcWindow));
                GlobalUnlock(hps);
                SendOwnerMessage(WM_PAINTCLIPBOARD, (WPARAM)hwnd, (LPARAM)hps);
                GlobalFree(hps);
                }
            }
        }
    else
        {
        /* We handle display */
        /* Redraw the portion of the paint rectangle that is in the clipbrd rect */
        IntersectRect(&rcPaint, &pps->rcPaint, &(pMDI->rcWindow));

        /* Always draw from left edge of window */
        rcPaint.left = pMDI->rcWindow.left;

        if ((rcPaint.bottom > rcPaint.top) && (rcPaint.right > rcPaint.left))
            {
            DrawFormat (hdc,
                        &rcPaint,
                        (int)(pMDI->cxScrollNow),
                        (int)(pMDI->cyScrollNow + rcPaint.top - pMDI->rcWindow.top),
                        BestFormat,
                        hwndMDI );
            }
        }

}







/*
 *      SaveOwnerScrollInfo
 *
 * When the user switched the clipboard display from owner disp to
 *  a non-owner display, all the information about the scroll bar
 *  positions are to be saved. This routine does that.
 *  This is required because, when the user returns back to owner
 *  display, the scroll bar positions are to be restored.
 */

void SaveOwnerScrollInfo (
    register HWND   hwnd)

{
    GetScrollRange (hwnd, SB_VERT, (LPINT) & OwnVerMin, (LPINT) & OwnVerMax);
    GetScrollRange (hwnd, SB_HORZ, (LPINT) & OwnHorMin, (LPINT) & OwnHorMax);

    OwnVerPos = GetScrollPos( hwnd, SB_VERT );
    OwnHorPos = GetScrollPos( hwnd, SB_HORZ );
}






/*
 *      RestoreOwnerScrollInfo
 *
 *  When the user sitches back to owner-display, the scroll bar
 *  positions are restored by this routine.
 */

void RestoreOwnerScrollInfo (
    register HWND   hwnd)

{
    PINFO(TEXT("SETSCROLLRANGE in RestoreOwnerScrollInfo\n\r"));
    SetScrollRange( hwnd, SB_VERT, OwnVerMin, OwnVerMax, FALSE);
    SetScrollRange( hwnd, SB_HORZ, OwnHorMin, OwnHorMax, FALSE);

    SetScrollPos( hwnd, SB_VERT, OwnVerPos, TRUE);
    SetScrollPos( hwnd, SB_HORZ, OwnHorPos, TRUE);
}







/*
 *      InitOwnerScrollInfo
 */

void InitOwnerScrollInfo(void)

{
    OwnVerPos = OwnHorPos = OwnVerMin = OwnHorMin = 0;
    OwnVerMax = VPOSLAST;
    OwnHorMax = HPOSLAST;
}






/*
 *      UpdateCBMenu
 *
 * This routine is called once during initialisation and everytime
 * the contents of the clipboard change. This updates the entries
 * in the "Display" popup menu and the "grey" and "checked" status
 * based on the data formats available in the clipboard.
 */
void UpdateCBMenu(
    HWND    hwnd,
    HWND    hwndMDI)
{
register WORD   wFlags;         // Used to store the status flags for menu items
register UINT   fmt;
WORD            cFmt;
WORD            cCBCount;       // Number of data items in CB
int             iIndex;
int             nPopupCount;
BOOL            bAutoSelect;
TCHAR           szName[40];



    // Now clipboard contains at least one item...
    // Find out the number entries in the popup menu at present.

    // make sure child window is valid
    if ( !hwndMDI || !IsWindow(hwndMDI))
        {
        PERROR(TEXT("bad window arg to UpdateCBMenu\n\r"));
        return;
        }

    nPopupCount = GetMenuItemCount(hDispMenu);

    if (nPopupCount > 6)
        {
        // Delete all the entries in the popup menu below menu break. */
        for (iIndex = 6; iIndex < nPopupCount; iIndex++)
            {
            // NOTE: The second parameter must always be 6! (because we use
            // MF_BYPOSITION, when 6 is deleted, 7 becomes 6!).
            DeleteMenu(hDispMenu, 6, MF_BYPOSITION | MF_DELETE);
            }
        }


    // If this is not a page MDI window we don't want to show any entries
    if ( GETMDIINFO(hwndMDI)->DisplayMode  != DSP_PAGE )
        {
        return;
        }

    bAutoSelect = TRUE;



    if ((cCBCount = (WORD)VCountClipboardFormats( GETMDIINFO(hwndMDI)->pVClpbrd ))
        && VOpenClipboard( GETMDIINFO(hwndMDI)->pVClpbrd, hwnd))
        {
        AppendMenu ( hDispMenu, MF_SEPARATOR, 0, 0 );
        AppendMenu ( hDispMenu, MF_STRING, CBM_AUTO, szDefaultFormat );
        AppendMenu ( hDispMenu, MF_SEPARATOR, 0, 0 );

        for (fmt=0, cFmt=1; cFmt <= cCBCount; cFmt++)
            {
            wFlags = 0;
            fmt = VEnumClipboardFormats( GETMDIINFO(hwndMDI)->pVClpbrd, fmt );

            // don't show preview format in menu...
            if ( fmt != cf_preview )
                {
                switch (fmt)
                    {
                    case CF_TEXT:
                    case CF_OEMTEXT:
                    case CF_DSPTEXT:
                    case CF_UNICODETEXT:
                    case CF_DSPBITMAP:

                    case CF_DIB:
                    case CF_BITMAP:

                    case CF_METAFILEPICT:
                    case CF_DSPMETAFILEPICT:
                    case CF_ENHMETAFILE:
                    case CF_DSPENHMETAFILE:

                    case CF_OWNERDISPLAY:
                    case CF_PALETTE:
                    case CF_HDROP:
                    case CF_LOCALE:

                        /* can display all of these, put them on menu */

                        // Check if the current format is the one selected by the user
                        if (GETMDIINFO(hwndMDI)->CurSelFormat == fmt)
                            {
                            bAutoSelect = FALSE;
                            wFlags |= MF_CHECKED;
                            }

                        GetClipboardMenuName(fmt, szName, sizeof(szName));
                        AppendMenu (hDispMenu, wFlags, fmt, (LPTSTR)szName);

                        break;

                    default:        /* all the rest... later */
                        break;
                    }
                }
            }



        for (fmt=VEnumClipboardFormats (GETMDIINFO(hwndMDI)->pVClpbrd, 0);
             fmt;
             fmt=VEnumClipboardFormats (GETMDIINFO(hwndMDI)->pVClpbrd, fmt))
            if ( fmt != cf_preview )
                switch (fmt)
                    {
                    case CF_TEXT:
                    case CF_OEMTEXT:
                    case CF_DSPTEXT:
                    case CF_UNICODETEXT:
                    case CF_DSPBITMAP:
                    case CF_DIB:
                    case CF_BITMAP:
                    case CF_METAFILEPICT:
                    case CF_DSPMETAFILEPICT:
                    case CF_ENHMETAFILE:
                    case CF_DSPENHMETAFILE:
                    case CF_OWNERDISPLAY:
                    case CF_PALETTE:
                    case CF_HDROP:
                    case CF_LOCALE:
                        break;

                    default:
                        /* can't display this, put it on menu and gray it */

                        GetClipboardName(fmt, szName, sizeof(szName));
                        AppendMenu (hDispMenu, MF_GRAYED, fmt, (LPTSTR)szName);

                        break;
                    }




        VCloseClipboard( GETMDIINFO(hwndMDI)->pVClpbrd );

        if (bAutoSelect)
            {
            GETMDIINFO(hwndMDI)->CurSelFormat = CBM_AUTO;
            CheckMenuItem(hDispMenu, CBM_AUTO, MF_BYCOMMAND | MF_CHECKED);
            }

        // Enable the menu items in the top level menu.
        // EnableMenuItem(hMainMenu, 2, MF_BYPOSITION | MF_ENABLED);
        // EnableMenuItem(hMainMenu, CBM_CLEAR, MF_BYCOMMAND | MF_ENABLED);
        // EnableMenuItem(hMainMenu, CBM_SAVEAS, MF_BYCOMMAND | MF_ENABLED);
        }
    else
        {
        PERROR(TEXT("UpdateCBMenu:couldn't open clip, or no data on clip\r\n"));
        }

    DrawMenuBar(hwnd);

}








/*
 *      ClearClipboard
 *
 *  This is called to clear the clipboard.  If the clipboard is not
 *  empty the user is asked if it should be cleared.
 */

BOOL ClearClipboard (
    register HWND   hwnd)

{
register int    RetVal;

    if (CountClipboardFormats() <= 0)
       return(TRUE);

    if ( MessageBoxID( hInst, hwnd, IDS_CONFIRMCLEAR, IDS_CLEARTITLE,
          MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
        {
        if (RetVal = SyncOpenClipboard(hwnd))
            {
            // PINFO("ClearClipboard: emptied clipboard\r\n");
            RetVal &= EmptyClipboard();
            RetVal &= SyncCloseClipboard();
            }
        else
            {
            // PERROR("ClearClipboard: could not open\r\n");

            MessageBoxID (hInst,
                          hwnd,
                          IDS_CLEAR,
                          IDS_ERROR,
                          MB_OK | MB_SYSTEMMODAL | MB_ICONHAND);
            }

        InvalidateRect(hwnd, NULL, TRUE);
        return RetVal;
        }

    return(FALSE);

}
