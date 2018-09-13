/****************************************************************************
 *
 *  MODULE  : DIB.C
 *
 *  DESCRIPTION : Routines for dealing with Device Independent Bitmaps.
 *
 *  FUNCTION   :bmfNumDIBColors(HANDLE hDib)
 *
 *  FUNCTION   :bmfCreateDIBPalette(HANDLE hDib)
 *
 *  FUNCTION   :bmfDIBSize(HANDLE hDIB)
 *
 *  FUNCTION   :bmfDIBFromBitmap(HBITMAP hBmp, DWORD biStyle, WORD biBits,
 *                               HPALETTE hPal)
 *
 *  FUNCTION   :bmfBitmapFromDIB(HANDLE hDib, HPALETTE hPal)
 *
 *  FUNCTION   :bmfBitmapFromIcon (HICON hIcon, DWORD dwColor)
 *
 *  FUNCTION   :bmfDrawBitmap(HDC hdc, int xpos, int ypos, HBITMAP hBmp,
 *                            DWORD rop)
 *
 *  FUNCTION   :bmfDrawBitmapSize(HDC hdc, int x, int y, int xSize,
 *                                int ySize, HBITMAP hBmp, DWORD rop)
 *
 *  FUNCTION   :DIBInfo(HANDLE hbi,LPBITMAPINFOHEADER lpbi)
 *
 *  FUNCTION   :CreateBIPalette(LPBITMAPINFOHEADER lpbi)
 *
 *  FUNCTION   :PaletteSize(VOID FAR * pv)
 *
 *  FUNCTION   :NumDIBColors(VOID FAR * pv)
 *
 *  FUNCTION   :LoadUIBitmap(HANDLE hInstance, LPCTSTR szName, 
 *							 COLORREF rgbText,
 *                           COLORREF rgbFace, COLORREF rgbShadow,
 *                           COLORREF rgbHighlight, COLORREF rgbWindow,
 *                           COLORREF rgbFrame)
 *
 ****************************************************************************/

#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include <stdlib.h>
#include <shellapi.h>
#include "draw.h"

/* global variables */
extern      HANDLE        ghInstance;    //instance handle of DIB.DLL

/***************************************************************************
 *
 *  FUNCTION   :bmfNumDIBColors(HANDLE hDib)
 *
 *  PURPOSE    :Returns the number of colors required to display the DIB
 *              indicated by hDib.
 *
 *  RETURNS    :The number of colors in the DIB. Possibilities are
 *              2, 16, 256, and 0 (0 indicates a 24-bit DIB).  In
 *              the case of an error, -1 is returned.
 *
 ****************************************************************************/

WORD WINAPI bmfNumDIBColors (HANDLE hDib)
{
    WORD                bits;
    LPBITMAPINFOHEADER  lpbi;

    lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib);
    if (!lpbi)
	return ((WORD)-1);

    /* The function NumDIBColors will return the number of colors in the
     * DIB by looking at the BitCount field in the info block
     */
    bits = NumDIBColors(lpbi);
    GlobalUnlock(hDib);
    return(bits);
}


/***************************************************************************
 *
 *  FUNCTION   :bmfCreateDIBPalette(HANDLE hDib)
 *
 *  PURPOSE    :Creates a palette suitable for displaying hDib.
 *
 *  RETURNS    :A handle to the palette if successful, NULL otherwise.
 *
 ****************************************************************************/

HPALETTE WINAPI bmfCreateDIBPalette (HANDLE hDib)
{
    HPALETTE            hPal;
    LPBITMAPINFOHEADER  lpbi;

    if (!hDib)
	return NULL;    //bail out if handle is invalid

    lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib);
    if (!lpbi)
	return NULL;

    hPal = CreateBIPalette(lpbi);
    GlobalUnlock(hDib);
    return hPal;
}


/***************************************************************************
 *
 *  FUNCTION   :bmfDIBSize(HANDLE hDIB)
 *
 *  PURPOSE    :Return the size of a DIB.
 *
 *  RETURNS    :DWORD with size of DIB, include BITMAPINFOHEADER and
 *              palette.  Returns 0 if failed.
 *
 *  HISTORY:
 *  92/08/13 -  BUG 1642: (w-markd)
 *              Added this function so Quick Recorder could find out the
 *              size of a DIB.
 *  92/08/29 -  BUG 2123: (w-markd)
 *              If the biSizeImage field of the structure we get is zero,
 *              then we have to calculate the size of the image ourselves.
 *              Also, after size is calculated, we bail out if the 
 *              size we calculated is larger than the size of the global
 *              object, since this indicates that the structure data
 *              we used to calculate the size was invalid.
 *
 ****************************************************************************/

DWORD WINAPI bmfDIBSize(HANDLE hDIB)
{
    LPBITMAPINFOHEADER  lpbi;
    DWORD               dwSize;

    /* Lock down the handle, and cast to a LPBITMAPINFOHEADER
    ** so we can read the fields we need
    */
    lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
    if (!lpbi)
	return 0;

    /* BUG 2123: (w-markd)
    ** Since the biSizeImage could be zero, we may have to calculate
    ** the size ourselves.
    */
    dwSize = lpbi->biSizeImage;
    if (dwSize == 0)
	dwSize = WIDTHBYTES((WORD)(lpbi->biWidth) * lpbi->biBitCount) *
	    lpbi->biHeight;


    /* The size of the DIB is the size of the BITMAPINFOHEADER
    ** structure (lpbi->biSize) plus the size of our palette plus
    ** the size of the actual data (calculated above).
    */
    dwSize += lpbi->biSize + (DWORD)PaletteSize(lpbi);

    /* BUG 2123: (w-markd)
    ** Check to see if the size is greater than the size
    ** of the global object.  If it is, the hDIB is corrupt.
    */
    GlobalUnlock(hDIB);
    if (dwSize > GlobalSize(hDIB))
	return 0;
    else
	return(dwSize);
}


/***************************************************************************
 *
 *  FUNCTION   :bmfDIBFromBitmap(HBITMAP hBmp, DWORD biStyle, WORD biBits,
 *                               HPALETTE hPal)
 *
 *  PURPOSE    :Converts the device-dependent BITMAP indicated by hBmp into
 *              a DIB.  biStyle indicates whether the palette contains
 *              DIB_RGB_COLORS or DIB_PAL_COLORS.  biBits indicates the
 *              desired number of bits in the destination DIB.  If biBits
 *              is zero, the destination DIB will be created with the
 *              minimum required bits.  hPal is a handle to the palette to be
 *              stored with the DIB data.  If hPal is NULL, the default
 *              system palette is used.
 *
 *  RETURNS    :A global handle to a memory block containing the DIB
 *              information in CF_DIB format.  NULL is returned if errors
 *              are encountered.
 *
 *  HISTORY:
 *
 *  92/08/12 -  BUG 1642: (angeld)
 *              Check the return value from GetObject, it will tell us
 *              if the handle hBmp was valid. bail out right away if it isn't
 *  92/08/29 -  BUG 2123: (w-markd)
 *              Use temporary variable to store old palette, then
 *              reselect the old palette when we are done.
 *
 ****************************************************************************/

HANDLE WINAPI bmfDIBFromBitmap (HBITMAP hBmp, DWORD biStyle, WORD biBits,
				    HPALETTE hPal)
{
    BITMAP                  bm;
    BITMAPINFOHEADER        bi;
    BITMAPINFOHEADER FAR    *lpbi;
    DWORD                   dwLen;
    HANDLE                  hDib;
    HANDLE                  hMem;
    HDC                     hdc;
    HPALETTE                hOldPal;

    if (!hBmp || !(GetObject(hBmp,sizeof(bm),(LPSTR)&bm)))
	 {
#if DEBUG
	  OutputDebugString(TEXT("bmfDIBFromBitmap:   INVALID HBITMAP!!!\n\r"));
#endif          
	return NULL;    //bail out if handle is invalid
	 }

    /* get default system palette if hPal is invalid */
    if (hPal == NULL)
	hPal = GetStockObject(DEFAULT_PALETTE);

    if (biBits == 0)
	biBits =  bm.bmPlanes * bm.bmBitsPixel;

    /* set up BITMAPINFOHEADER structure */
    bi.biSize               = sizeof(BITMAPINFOHEADER);
    bi.biWidth              = bm.bmWidth;
    bi.biHeight             = bm.bmHeight;
    bi.biPlanes             = 1;
    bi.biBitCount           = biBits;
    bi.biCompression        = biStyle;
    bi.biSizeImage          = 0;
    bi.biXPelsPerMeter      = 0;
    bi.biYPelsPerMeter      = 0;
    bi.biClrUsed            = 0;
    bi.biClrImportant       = 0;

    dwLen  = bi.biSize + PaletteSize(&bi);

    hdc = GetDC(NULL);
    /* BUG 2123: (w-markd)
    ** Store the previous palette in hOldPal, restore it on exit.
    */
    hOldPal = SelectPalette(hdc,hPal,FALSE);
    RealizePalette(hdc);

    /* get global memory for DIB */
    hDib = GlobalAlloc(GHND,dwLen);

    if (!hDib)
    {
	/* could not allocate memory; clean up and exit */
	SelectPalette(hdc,hOldPal,FALSE);
	ReleaseDC(NULL,hdc);
	return NULL;
    }

    lpbi = (VOID FAR *)GlobalLock(hDib);
    if (!lpbi)
    {
	/* could not lock memory; clean up and exit */
	SelectPalette(hdc,hOldPal,FALSE);
	ReleaseDC(NULL,hdc);
	GlobalFree(hDib);
	return NULL;
    }

    *lpbi = bi;

    /*  call GetDIBits with a NULL lpBits param, so it will calculate the
     *  biSizeImage field for us
     */
    GetDIBits(hdc, hBmp, 0, (WORD)bi.biHeight,
	NULL, (LPBITMAPINFO)lpbi, DIB_RGB_COLORS);

    bi = *lpbi;
    GlobalUnlock(hDib);

    /* If the driver did not fill in the biSizeImage field, make one up */
    if (bi.biSizeImage == 0)
    {
	bi.biSizeImage = WIDTHBYTES((DWORD)bm.bmWidth * biBits) * bm.bmHeight;

	if (biStyle != BI_RGB)
	    bi.biSizeImage = (bi.biSizeImage * 3) / 2;
    }

    /*  realloc the buffer big enough to hold all the bits */
    dwLen = bi.biSize + PaletteSize(&bi) + bi.biSizeImage;
    hMem = GlobalReAlloc(hDib,dwLen,GMEM_MOVEABLE);
    if (!hMem)
    {
	/* could not allocate memory; clean up and exit */
	GlobalFree(hDib);
	SelectPalette(hdc,hOldPal,FALSE);
	ReleaseDC(NULL,hdc);
	return NULL;
    }
    else
	hDib = hMem;

    /*  call GetDIBits with a NON-NULL lpBits param, and actualy get the
     *  bits this time
     */
    lpbi = (VOID FAR *)GlobalLock(hDib);
    if (!lpbi)
    {
	/* could not lock memory; clean up and exit */
	GlobalFree(hDib);
	SelectPalette(hdc,hOldPal,FALSE);
	ReleaseDC(NULL,hdc);
	return NULL;
    }

    if (GetDIBits( hdc, hBmp, 0, (WORD)bi.biHeight,
	   (LPSTR)lpbi + (WORD)lpbi->biSize + PaletteSize(lpbi),
	   (LPBITMAPINFO)lpbi, DIB_RGB_COLORS) == 0)
    {
	/* clean up and exit */
	GlobalUnlock(hDib);
	GlobalFree(hDib);
	SelectPalette(hdc,hOldPal,FALSE);
	ReleaseDC(NULL,hdc);
	return NULL;
    }

    bi = *lpbi;

    /* clean up and exit */
    GlobalUnlock(hDib);
    SelectPalette(hdc,hOldPal,FALSE);
    ReleaseDC(NULL,hdc);
    return hDib;
}


/***************************************************************************
 *
 *  FUNCTION   :bmfBitmapFromDIB(HANDLE hDib, HPALETTE hPal)
 *
 *  PURPOSE    :Converts DIB information into a device-dependent BITMAP
 *              suitable for display on the current display device.  hDib is
 *              a global handle to a memory block containing the DIB
 *              information in CF_DIB format.  hPal is a handle to a palette
 *              to be used for displaying the bitmap.  If hPal is NULL, the
 *              default system palette is used during the conversion.
 *
 *  RETURNS    :Returns a handle to a bitmap is successful, NULL otherwise.
 *
 *  HISTORY:
 *  92/08/29 -  BUG 2123: (w-markd)
 *              Check if DIB is has a valid size, and bail out if not.
 *              If no palette is passed in, try to create one.  If we
 *              create one, we must destroy it before we exit.
 *
 ****************************************************************************/

HBITMAP WINAPI bmfBitmapFromDIB(HANDLE hDib, HPALETTE hPal)
{
    LPBITMAPINFOHEADER  lpbi;
    HPALETTE            hPalT;
    HDC                 hdc;
    HBITMAP             hBmp;
    DWORD               dwSize;
    BOOL                bMadePalette = FALSE;

    if (!hDib)
	return NULL;    //bail out if handle is invalid
    
    /* BUG 2123: (w-markd)
    ** Check to see if we can get the size of the DIB.  If this call
    ** fails, bail out.
    */
    dwSize = bmfDIBSize(hDib);
    if (!dwSize)
	return NULL;

    lpbi = (VOID FAR *)GlobalLock(hDib);
    if (!lpbi)
	return NULL;

    /* prepare palette */
    /* BUG 2123: (w-markd)
    ** If the palette is NULL, we create one suitable for displaying
    ** the dib.
    */
    if (!hPal)
    {
	hPal = bmfCreateDIBPalette(hDib);
	if (!hPal)
	{
	    GlobalUnlock(hDib);
	    #ifdef V101
	    #else
	    bMadePalette = TRUE;
	    #endif
	    return NULL;
	}
	#ifdef V101
	/* BUGFIX: mikeroz 2123 - this flag was in the wrong place */
	bMadePalette = TRUE;
	#endif
    }
    hdc = GetDC(NULL);
    hPalT = SelectPalette(hdc,hPal,FALSE);
    RealizePalette(hdc);     // GDI Bug...????

    /* Create the bitmap.  Note that a return value of NULL is ok here */
    hBmp = CreateDIBitmap(hdc, (LPBITMAPINFOHEADER)lpbi, (LONG)CBM_INIT,
			  (LPSTR)lpbi + lpbi->biSize + PaletteSize(lpbi),
			  (LPBITMAPINFO)lpbi, DIB_RGB_COLORS );

    /* clean up and exit */
    /* BUG 2123: (w-markd)
    ** If we made the palette, we need to delete it.
    */
    if (bMadePalette)
	DeleteObject(SelectPalette(hdc,hPalT,FALSE));
    else
	SelectPalette(hdc,hPalT,FALSE);
    ReleaseDC(NULL,hdc);
    GlobalUnlock(hDib);
    return hBmp;
}


/***************************************************************************
 *
 *  FUNCTION   :bmfBitmapFromIcon (HICON hIcon, DWORD dwColor)
 *
 *  PURPOSE    :Converts an icon into a bitmap.  hIcon is a handle to a
 *              windows ICON object.  dwColor sets the background color for
 *              the bitmap.
 *
 *  RETURNS    :A handle to the bitmap is successful, NULL otherwise.
 *
 ****************************************************************************/


HBITMAP WINAPI bmfBitmapFromIcon (HICON hIcon, DWORD dwColor)
{
    HDC     hDC;
    HDC     hMemDC = 0;
    HBITMAP hBitmap = 0;
    HBITMAP hOldBitmap;
    HBRUSH  hBrush = 0;
    HBRUSH  hOldBrush;
    int     xIcon, yIcon;

    hDC = GetDC(NULL);
    hMemDC = CreateCompatibleDC( hDC );
    if (hMemDC)
    {
	/* get the size for the destination bitmap */
	xIcon = GetSystemMetrics(SM_CXICON);
	yIcon = GetSystemMetrics(SM_CYICON);
	hBitmap = CreateCompatibleBitmap(hDC, xIcon, yIcon);
	if (hBitmap)
	{
	    hBrush = CreateSolidBrush(dwColor);
	    if (hBrush)
	    {
		hOldBitmap = SelectObject (hMemDC, hBitmap);
		hOldBrush  = SelectObject (hMemDC, hBrush);

		/* draw the icon on the memory device context */
		PatBlt   (hMemDC, 0, 0, xIcon, yIcon, PATCOPY);
		DrawIcon (hMemDC, 0, 0, hIcon);

		/* clean up and exit */
		DeleteObject(SelectObject(hMemDC, hOldBrush));
		SelectObject(hMemDC, hOldBitmap);
		DeleteDC(hMemDC);
		ReleaseDC(NULL, hDC);
		return hBitmap;
	    }
	}
    }

    /* clean up resources and exit */
    if (hBitmap)
	DeleteObject(hBitmap);
    if (hMemDC)
	DeleteDC(hMemDC);
    ReleaseDC (NULL, hDC);
    return NULL;
}


/***************************************************************************
 *
 *  FUNCTION   :bmfDrawBitmap(HDC hdc, int xpos, int ypos, HBITMAP hBmp,
 *                            DWORD rop)
 *
 *  PURPOSE    :Draws bitmap hBmp at the specifed position in DC hdc
 *
 *  RETURNS    :Return value of BitBlt() or FALSE if in an error is
 *              encountered.  Note that BitBlt returns true if successful.
 *
 ****************************************************************************/

BOOL WINAPI bmfDrawBitmap (HDC hdc, int xpos, int ypos, HBITMAP hBmp,
			       DWORD rop)
{
    HDC       hdcBits;
    BITMAP    bm;
    BOOL      bResult;

    if (!hdc || !hBmp)
	return FALSE;

    hdcBits = CreateCompatibleDC(hdc);
    if (!hdcBits)
	return FALSE;
    GetObject(hBmp,sizeof(BITMAP),(LPSTR)&bm);
    SelectObject(hdcBits,hBmp);
    bResult = BitBlt(hdc,xpos,ypos,bm.bmWidth,bm.bmHeight,hdcBits,0,0,rop);
    DeleteDC(hdcBits);

    return bResult;
}


/***************************************************************************
 *
 *  FUNCTION   :DIBInfo(HANDLE hbi,LPBITMAPINFOHEADER lpbi)
 *
 *  PURPOSE    :Retrieves the DIB info associated with a CF_DIB
 *              format memory block.
 *
 *  RETURNS    :TRUE (non-zero) if successful;  FALSE (zero) otherwise.
 *
 ****************************************************************************/

BOOL DIBInfo (HANDLE hbi, LPBITMAPINFOHEADER lpbi)
{
    if (!hbi)
	return FALSE;

    *lpbi = *(LPBITMAPINFOHEADER)GlobalLock (hbi);
    if (!lpbi)
	return FALSE;
    /* fill in the default fields */
    if (lpbi->biSize != sizeof (BITMAPCOREHEADER))
    {
	if (lpbi->biSizeImage == 0L)
	    lpbi->biSizeImage =
	    WIDTHBYTES(lpbi->biWidth*lpbi->biBitCount) * lpbi->biHeight;
	if (lpbi->biClrUsed == 0L)
	    lpbi->biClrUsed = NumDIBColors (lpbi);
    }
    GlobalUnlock (hbi);
    return TRUE;
}

/***************************************************************************
 *
 *  FUNCTION   :CreateBIPalette(LPBITMAPINFOHEADER lpbi)
 *
 *  PURPOSE    :Given a Pointer to a BITMAPINFO struct will create a
 *              a GDI palette object from the color table.
 *
 *  RETURNS    :A handle to the palette if successful, NULL otherwise.
 *
 ****************************************************************************/

HPALETTE CreateBIPalette (LPBITMAPINFOHEADER lpbi)
{
    LPLOGPALETTE        pPal;
    HPALETTE            hPal = NULL;
    WORD                nNumColors;
    BYTE                red;
    BYTE                green;
    BYTE                blue;
    int                 i;
    RGBQUAD             FAR *pRgb;
    HANDLE hMem;

    if (!lpbi)
	return NULL;

    if (lpbi->biSize != sizeof(BITMAPINFOHEADER))
	return NULL;

    /* Get a pointer to the color table and the number of colors in it */
    pRgb = (RGBQUAD FAR *)((LPSTR)lpbi + (WORD)lpbi->biSize);
    nNumColors = NumDIBColors(lpbi);

    if (nNumColors)
    {
	/* Allocate for the logical palette structure */
	hMem = GlobalAlloc(GMEM_MOVEABLE,
	sizeof(LOGPALETTE) + nNumColors * sizeof(PALETTEENTRY));
	if (!hMem)
	    return NULL;
	pPal = (LPLOGPALETTE)GlobalLock(hMem);
	if (!pPal)
	{
	    GlobalFree(hMem);
	    return NULL;
	}

	pPal->palNumEntries = nNumColors;
	pPal->palVersion    = PALVERSION;

	/* Fill in the palette entries from the DIB color table and
	 * create a logical color palette.
	 */
	for (i = 0; (unsigned)i < nNumColors; i++)
	{
	    pPal->palPalEntry[i].peRed   = pRgb[i].rgbRed;
	    pPal->palPalEntry[i].peGreen = pRgb[i].rgbGreen;
	    pPal->palPalEntry[i].peBlue  = pRgb[i].rgbBlue;
	    pPal->palPalEntry[i].peFlags = (BYTE)0;
	}
	hPal = CreatePalette(pPal);
	/* note that a NULL return value for the above CreatePalette call
	 * is acceptable, since this value will be returned, and is not
	 * used again here
	 */
	GlobalUnlock(hMem);
	GlobalFree(hMem);
    }
    else if (lpbi->biBitCount == 24)
    {
	/* A 24 bitcount DIB has no color table entries so, set the number of
	 * to the maximum value (256).
	 */
	nNumColors = MAXPALETTE;
	hMem =GlobalAlloc(GMEM_MOVEABLE,
	sizeof(LOGPALETTE) + nNumColors * sizeof(PALETTEENTRY));
	if (!hMem)
	    return NULL;
	pPal = (LPLOGPALETTE)GlobalLock(hMem);
	if (!pPal)
	{
	    GlobalFree(hMem);
	    return NULL;
	}

	pPal->palNumEntries = nNumColors;
	pPal->palVersion    = PALVERSION;

	red = green = blue = 0;

	/* Generate 256 (= 8*8*4) RGB combinations to fill the palette
	 * entries.
	 */
	for (i = 0; (unsigned)i < pPal->palNumEntries; i++)
	{
	    pPal->palPalEntry[i].peRed   = red;
	    pPal->palPalEntry[i].peGreen = green;
	    pPal->palPalEntry[i].peBlue  = blue;
	    pPal->palPalEntry[i].peFlags = (BYTE)0;

	    if (!(red += 32))
	    if (!(green += 32))
		blue += 64;
	}
	hPal = CreatePalette(pPal);
	/* note that a NULL return value for the above CreatePalette call
	 * is acceptable, since this value will be returned, and is not
	 * used again here
	 */
	GlobalUnlock(hMem);
	GlobalFree(hMem);
    }
    return hPal;
}
/***************************************************************************
 *
 *  FUNCTION   :PaletteSize(VOID FAR * pv)
 *
 *  PURPOSE    :Calculates the palette size in bytes. If the info. block
 *              is of the BITMAPCOREHEADER type, the number of colors is
 *              multiplied by 3 to give the palette size, otherwise the
 *              number of colors is multiplied by 4.
 *
 *  RETURNS    :Palette size in number of bytes.
 *
 ****************************************************************************/

WORD PaletteSize (VOID FAR *pv)
{
    LPBITMAPINFOHEADER  lpbi;
    WORD                NumColors;

    lpbi      = (LPBITMAPINFOHEADER)pv;
    NumColors = NumDIBColors(lpbi);

    if (lpbi->biSize == sizeof(BITMAPCOREHEADER))
	return (NumColors * sizeof(RGBTRIPLE));
    else
	return (NumColors * sizeof(RGBQUAD));
}


/***************************************************************************
 *
 *  FUNCTION   :NumDIBColors(VOID FAR * pv)
 *
 *  PURPOSE    :Determines the number of colors in the DIB by looking at
 *              the BitCount field in the info block.
 *              For use only internal to DLL.
 *
 *  RETURNS    :The number of colors in the DIB.
 *
 ****************************************************************************/

WORD NumDIBColors (VOID FAR * pv)
{
    int                 bits;
    LPBITMAPINFOHEADER  lpbi;
    LPBITMAPCOREHEADER  lpbc;

    lpbi = ((LPBITMAPINFOHEADER)pv);
    lpbc = ((LPBITMAPCOREHEADER)pv);

    /*  With the BITMAPINFO format headers, the size of the palette
     *  is in biClrUsed, whereas in the BITMAPCORE - style headers, it
     *  is dependent on the bits per pixel ( = 2 raised to the power of
     *  bits/pixel).
     */
    if (lpbi->biSize != sizeof(BITMAPCOREHEADER))
    {
	if (lpbi->biClrUsed != 0)
	    return (WORD)lpbi->biClrUsed;
	bits = lpbi->biBitCount;
    }
    else
	bits = lpbc->bcBitCount;

    switch (bits)
    {
    case 1:
	return 2;
    case 4:
	return 16;
    case 8:
	return 256;
    default:
	/* A 24 bitcount DIB has no color table */
	return 0;
    }
}


/***************************************************************************
 *
 *  FUNCTION   :bmfDrawBitmapSize(HDC hdc, int x, int y, int xSize,
 *              int ySize, HBITMAP hBmp, DWORD rop)
 *
 *  PURPOSE    :Draws bitmap <hBmp> at the specifed position in DC <hdc> with
 *              a specified size.
 *
 *  RETURNS    :Return value of BitBlt() or false in an error is
 *              encountered.  Note that BitBlt returns true if successful.
 *
 *  HISTORY:
 *  92/08/13 -  BUG 1642: (w-markd)
 *              Exported this function.
 *              Also stored object that was returned from SelectObject,
 *              and selected this back into the hdc before deleting.
 *
 ****************************************************************************/

BOOL WINAPI bmfDrawBitmapSize (HDC hdc, int xpos, int ypos, int xSize, int ySize, HBITMAP hBmp, DWORD rop)
{
    HDC         hdcBits;
    BOOL        bResult;
    HBITMAP     hOldBmp;

    if (!hdc || !hBmp)
	return FALSE;

    hdcBits = CreateCompatibleDC(hdc);
    if (!hdcBits)
	return FALSE;
    /* BUG 1642: (w-markd)
    ** Remeber old bmp and reselect into hdc before DeleteDC
    */
    hOldBmp = SelectObject(hdcBits,hBmp);
    bResult = BitBlt(hdc,xpos,ypos,xSize, ySize,hdcBits, 0,0,rop);
    SelectObject(hdcBits, hOldBmp);
    DeleteDC(hdcBits);

    return bResult;
}

//----------------------------------------------------------------------------
//  LoadUIBitmap() - load a bitmap resource
//
//      load a bitmap resource from a resource file, converting all
//      the standard UI colors to the current user specifed ones.
//
//      this code is designed to load bitmaps used in "gray ui" or
//      "toolbar" code.
//
//      the bitmap must be a 4bpp windows 3.0 DIB, with the standard
//      VGA 16 colors.
//
//      the bitmap must be authored with the following colors
//
//          Button Text        Black        (index 0)
//          Button Face        lt gray      (index 7)
//          Button Shadow      gray         (index 8)
//          Button Highlight   white        (index 15)
//          Window Color       yellow       (index 11)
//          Window Frame       green        (index 10)
//
//      Example:
//
//          hbm = LoadUIBitmap(hInstance, "TestBmp",
//              GetSysColor(COLOR_BTNTEXT),
//              GetSysColor(COLOR_BTNFACE),
//              GetSysColor(COLOR_BTNSHADOW),
//              GetSysColor(COLOR_BTNHIGHLIGHT),
//              GetSysColor(COLOR_WINDOW),
//              GetSysColor(COLOR_WINDOWFRAME));
//
//      Author:     JimBov, ToddLa
//      History:    5/13/92 - added to dib.c in sndcntrl.dll, t-chrism
//
//----------------------------------------------------------------------------
HBITMAP WINAPI LoadUIBitmap(
    HANDLE      hInstance,          // EXE file to load resource from
    LPCTSTR      szName,             // name of bitmap resource
    COLORREF    rgbText,            // color to use for "Button Text"
    COLORREF    rgbFace,            // color to use for "Button Face"
    COLORREF    rgbShadow,          // color to use for "Button Shadow"
    COLORREF    rgbHighlight,       // color to use for "Button Hilight"
    COLORREF    rgbWindow,          // color to use for "Window Color"
    COLORREF    rgbFrame)           // color to use for "Window Frame"
{
    LPBYTE              lpb;
    HBITMAP             hbm;
    LPBITMAPINFOHEADER  lpbi;
    HANDLE              h;
    HDC                 hdc;
    LPDWORD             lprgb;

    // convert a RGB into a RGBQ
    #define RGBQ(dw) RGB(GetBValue(dw),GetGValue(dw),GetRValue(dw))

    h = LoadResource(hInstance,FindResource(hInstance, szName, RT_BITMAP));

    lpbi = (LPBITMAPINFOHEADER)LockResource(h);

    if (!lpbi)
	return(NULL);

    if (lpbi->biSize != sizeof(BITMAPINFOHEADER))
	return NULL;

    if (lpbi->biBitCount != 4)
	return NULL;

    lprgb = (LPDWORD)((LPBYTE)lpbi + (int)lpbi->biSize);
    lpb   = (LPBYTE)(lprgb + 16);

    lprgb[0]  = RGBQ(rgbText);          // Black
    lprgb[7]  = RGBQ(rgbFace);          // lt gray
    lprgb[8]  = RGBQ(rgbShadow);        // gray
    lprgb[15] = RGBQ(rgbHighlight);     // white
    lprgb[11] = RGBQ(rgbWindow);        // yellow
    lprgb[10] = RGBQ(rgbFrame);         // green

    hdc = GetDC(NULL);

    hbm = CreateDIBitmap(hdc, lpbi, CBM_INIT, (LPVOID)lpb,
	(LPBITMAPINFO)lpbi, DIB_RGB_COLORS);

    ReleaseDC(NULL, hdc);
    UnlockResource(h);
    FreeResource(h);
    return(hbm);
}// LoadUIBitmap


