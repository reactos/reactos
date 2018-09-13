
/*****************************************************************************

                                    D I B

    Name:       dib.c
    Date:       19-Apr-1994
    Creator:    Unknown

    Description:
        Handles Device Independent Bitmap.

        PaletteSize()       - Calculates the palette size in bytes
                              of given DIB

        DibNumColors()      - Determines the number of colors in DIB

        BitmapFromDib()     - Creates a DDB given a global handle to
                              a block in CF_DIB format.

        DibFromBitmap()     - Creates a DIB repr. the DDB passed in.


    History:
        16-Jun-1994 John Fu     Fix DibFromBitmap() to use correct biBits.

        08-Jul-1994 John Fu     Fix DibFromBitmap() and BitmapFromDib() to
                                use palette when available.

        15-Mar-1995 John Fu     Changed to use DIB_RGB_COLORS only.

*****************************************************************************/




#include <windows.h>
#include "dib.h"


static   HCURSOR hcurSave;







/*
 *
 *  FUNCTION   :  PaletteSize(VOID FAR * pv)
 *
 *  PURPOSE    :  Calculates the palette size in bytes. If the info. block
 *                is of the BITMAPCOREHEADER type, the number of colors is
 *                multiplied by 3 to give the palette size, otherwise the
 *                number of colors is multiplied by 4.                                                         *
 *
 *  RETURNS    :  Palette size in number of bytes.
 *
 */

WORD PaletteSize (
    VOID FAR * pv)
{
LPBITMAPINFOHEADER  lpbi;
WORD                NumColors;

    lpbi      = (LPBITMAPINFOHEADER)pv;
    NumColors = DibNumColors(lpbi);

    if (lpbi->biSize == sizeof(BITMAPCOREHEADER))
        return NumColors * sizeof(RGBTRIPLE);
    else
        return NumColors * sizeof(RGBQUAD);

}





/*
 *
 *  FUNCTION   : DibNumColors(VOID FAR * pv)
 *
 *  PURPOSE    : Determines the number of colors in the DIB by looking at
 *               the BitCount filed in the info block.
 *
 *  RETURNS    : The number of colors in the DIB.
 *
 */

WORD DibNumColors (
    VOID FAR * pv)
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






/*
 *
 *  FUNCTION   : DibFromBitmap()
 *
 *  PURPOSE    : Will create a global memory block in DIB format that
 *               represents the Device-dependent bitmap (DDB) passed in.
 *
 *  RETURNS    : A handle to the DIB
 *
 */

HANDLE DibFromBitmap (
    HBITMAP     hbm,
    DWORD       biStyle,
    WORD        biBits,
    HPALETTE    hpal)
{
BITMAP               bm;
BITMAPINFOHEADER     bi;
BITMAPINFOHEADER FAR *lpbi;
DWORD                dwLen;
HANDLE               hdib;
HANDLE               h;
HDC                  hdc;



    if (!hbm)
        return NULL;


    if (hpal == NULL)
        {
        hpal = GetStockObject(DEFAULT_PALETTE);
        }


    if (!GetObject(hbm,sizeof(bm),(LPSTR)&bm))
        return NULL;


    if (biBits == 0)
        biBits = bm.bmPlanes * bm.bmBitsPixel;



    // make sure we have the right # of bits

    if (biBits <= 1)
        biBits = 1;
    else if (biBits <= 4)
        biBits = 4;
    else if (biBits <= 8)
        biBits = 8;
    else
        biBits = 24;



    bi.biSize           = sizeof(BITMAPINFOHEADER);
    bi.biWidth          = bm.bmWidth;
    bi.biHeight         = bm.bmHeight;
    bi.biPlanes         = 1;
    bi.biBitCount       = biBits;
    bi.biCompression    = biStyle;
    bi.biSizeImage      = 0;
    bi.biXPelsPerMeter  = 0;
    bi.biYPelsPerMeter  = 0;
    bi.biClrUsed        = 0;
    bi.biClrImportant   = 0;

    dwLen  = bi.biSize + PaletteSize (&bi);




    hdc = GetDC(NULL);

    hpal = SelectPalette (hdc, hpal, FALSE);
    RealizePalette (hdc);



    hdib = GlobalAlloc (GHND, dwLen);

    if (!hdib)
        {
        SelectPalette (hdc,hpal,FALSE);
        ReleaseDC (NULL,hdc);
        return NULL;
        }




    lpbi = (VOID FAR *)GlobalLock(hdib);
    *lpbi = bi;




    /*  call GetDIBits with a NULL lpBits param, so it will calculate the
     *  biSizeImage field for us
     */

    GetDIBits (hdc,
               hbm,
               0,
               (WORD)bi.biHeight,
               NULL,
               (LPBITMAPINFO)lpbi,
               DIB_RGB_COLORS);


    bi = *lpbi;
    GlobalUnlock(hdib);





    // If the driver did not fill in the biSizeImage field, make one up

    if (bi.biSizeImage == 0)
        {
        bi.biSizeImage = WIDTHBYTES((DWORD)bm.bmWidth * biBits) * bm.bmHeight;

        if (biStyle != BI_RGB)
            bi.biSizeImage = (bi.biSizeImage * 3) / 2;
        }




    // realloc the buffer big enough to hold all the bits

    dwLen = bi.biSize + PaletteSize(&bi) + bi.biSizeImage;

    if (h = GlobalReAlloc (hdib,dwLen,0))
        hdib = h;
    else
        {
        GlobalFree(hdib);
        hdib = NULL;

        SelectPalette(hdc,hpal,FALSE);
        ReleaseDC(NULL,hdc);
        return hdib;
        }




    /*  call GetDIBits with a NON-NULL lpBits param, and actualy get the
     *  bits this time
     */

    lpbi = (VOID FAR *)GlobalLock(hdib);

    if (0 == GetDIBits (hdc,
                        hbm,
                        0,
                        (WORD)bi.biHeight,
                        (LPSTR)lpbi + (WORD)lpbi->biSize + PaletteSize(lpbi),
                        (LPBITMAPINFO)lpbi,
                        DIB_RGB_COLORS))
        {
        GlobalUnlock (hdib);
        hdib = NULL;
        SelectPalette (hdc, hpal, FALSE);
        ReleaseDC (NULL, hdc);
        return NULL;
        }


    bi = *lpbi;
    GlobalUnlock (hdib);


    SelectPalette (hdc, hpal, FALSE);

    ReleaseDC (NULL, hdc);

    return hdib;

}








/*
 *
 *  FUNCTION   : BitmapFromDib(HANDLE hdib, HPALETTE hpal)
 *
 *  PURPOSE    : Will create a DDB (Device Dependent Bitmap) given a global
 *               handle to a memory block in CF_DIB format
 *
 *  RETURNS    : A handle to the DDB.
 *
 */

HBITMAP BitmapFromDib (
    HANDLE      hdib,
    HPALETTE    hpal)
{
LPBITMAPINFOHEADER  lpbi;
HPALETTE            hpalT;
HDC                 hdc;
HBITMAP             hbm = NULL;



    StartWait();


    if (!hdib)
        goto done;


    lpbi = (VOID FAR *)GlobalLock (hdib);
    if (!lpbi)
        goto done;



    hdc = GetDC (NULL);

    if (hpal)
        {
        hpalT = SelectPalette (hdc, hpal, FALSE);
        RealizePalette (hdc);
        }

    hbm = CreateDIBitmap (hdc,
                          (LPBITMAPINFOHEADER)lpbi,
                          (LONG)CBM_INIT,
                          (LPSTR)lpbi + lpbi->biSize + PaletteSize(lpbi),
                          (LPBITMAPINFO)lpbi,
                          DIB_RGB_COLORS);


    if (hpal)
        SelectPalette (hdc, hpalT, FALSE);


    ReleaseDC (NULL, hdc);
    GlobalUnlock (hdib);


done:

    EndWait();

    return hbm;

}
