/******************************Module*Header*******************************\
* Module Name: fastdib.c
*
* CreateCompatibleDIB implementation.
*
* Created: 23-Jan-1996 21:08:18
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1996 Microsoft Corporation
*
\**************************************************************************/

#include <windows.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "sscommon.h"

BOOL APIENTRY GetDIBTranslationVector(HDC hdcMem, HPALETTE hpal, BYTE *pbVector);
static BOOL bFillBitmapInfo(HDC hdc, HPALETTE hpal, BITMAPINFO *pbmi);
static BOOL bFillColorTable(HDC hdc, HPALETTE hpal, BITMAPINFO *pbmi);
static UINT MyGetSystemPaletteEntries(HDC hdc, UINT iStartIndex, UINT nEntries,
                                      LPPALETTEENTRY lppe);
static BOOL bComputeLogicalToSurfaceMap(HDC hdc, HPALETTE hpal,
                                        BYTE *pajVector);

/******************************Public*Routine******************************\
* CreateCompatibleDIB
*
* Create a DIB section with an optimal format w.r.t. the specified hdc.
*
* If DIB <= 8bpp, then the DIB color table is initialized based on the
* specified palette.  If the palette handle is NULL, then the system
* palette is used.
*
* Note: The hdc must be a direct DC (not an info or memory DC).
*
* Note: On palettized displays, if the system palette changes the
*       UpdateDIBColorTable function should be called to maintain
*       the identity palette mapping between the DIB and the display.
*
* Returns:
*   Valid bitmap handle if successful, NULL if error.
*
* History:
*  23-Jan-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

//HBITMAP APIENTRY
HBITMAP
SSDIB_CreateCompatibleDIB(HDC hdc, HPALETTE hpal, ULONG ulWidth, ULONG ulHeight,
                    PVOID *ppvBits)
{
    HBITMAP hbmRet = (HBITMAP) NULL;
    BYTE aj[sizeof(BITMAPINFO) + (sizeof(RGBQUAD) * 255)];
    BITMAPINFO *pbmi = (BITMAPINFO *) aj;

    //
    // Validate hdc.
    //

    if ( GetObjectType(hdc) != OBJ_DC )
    {
        SS_DBGPRINT("CreateCompatibleDIB: not OBJ_DC\n");
        return hbmRet;
    }

    memset(aj, 0, sizeof(aj));
    if ( bFillBitmapInfo(hdc, hpal, pbmi) )
    {
        //
        // Change bitmap size to match specified dimensions.
        //

        pbmi->bmiHeader.biWidth = ulWidth;
        pbmi->bmiHeader.biHeight = ulHeight;
        if (pbmi->bmiHeader.biCompression == BI_RGB)
        {
            pbmi->bmiHeader.biSizeImage = 0;
        }
        else
        {
            if ( pbmi->bmiHeader.biBitCount == 16 )
                pbmi->bmiHeader.biSizeImage = ulWidth * ulHeight * 2;
            else if ( pbmi->bmiHeader.biBitCount == 32 )
                pbmi->bmiHeader.biSizeImage = ulWidth * ulHeight * 4;
            else
                pbmi->bmiHeader.biSizeImage = 0;
        }
        pbmi->bmiHeader.biClrUsed = 0;
        pbmi->bmiHeader.biClrImportant = 0;

        //
        // Create the DIB section.  Let Win32 allocate the memory and return
        // a pointer to the bitmap surface.
        //

        hbmRet = CreateDIBSection(hdc, pbmi, DIB_RGB_COLORS, ppvBits, NULL, 0);
        GdiFlush();

        if ( !hbmRet )
        {
            SS_DBGPRINT("CreateCompatibleDIB: CreateDIBSection failed\n");
        }
    }
    else
    {
        SS_DBGPRINT("CreateCompatibleDIB: bFillBitmapInfo failed\n");
    }

    return hbmRet;
}

/******************************Public*Routine******************************\
* UpdateDIBColorTable
*
* Synchronize the DIB color table to the specified palette hpal.
* If hpal is NULL, then use the system palette.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  23-Jan-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY
SSDIB_UpdateColorTable(HDC hdcMem, HDC hdc, HPALETTE hpal)
{
    BOOL bRet = FALSE;
    HBITMAP hbm;
    DIBSECTION ds;
    BYTE aj[(sizeof(RGBQUAD) + sizeof(PALETTEENTRY)) * 256];
    LPPALETTEENTRY lppe = (LPPALETTEENTRY) aj;
    LPRGBQUAD prgb = (LPRGBQUAD) (lppe + 256);
    ULONG i, cColors;

    //
    // Validate hdc.
    //

    if ( GetObjectType(hdc) != OBJ_DC )
    {
        SS_DBGPRINT("UpdateDIBColorTable: not OBJ_DC\n");
        return bRet;
    }
    if ( GetObjectType(hdcMem) != OBJ_MEMDC )
    {
        SS_DBGPRINT("UpdateDIBColorTable: not OBJ_MEMDC\n");
        return bRet;
    }

    //
    // Get the bitmap handle out of the memdc.
    //

    hbm = GetCurrentObject(hdcMem, OBJ_BITMAP);

    //
    // Validate bitmap (must be DIB section).
    //

    if ( (GetObject(hbm, sizeof(ds), &ds) == sizeof(ds)) &&
         ds.dsBm.bmBits )
    {
        //
        // Get palette entries from specified palette or system palette.
        //

        cColors = 1 << ds.dsBmih.biBitCount;


        if ( hpal ? GetPaletteEntries(hpal, 0, cColors, lppe)
                  : MyGetSystemPaletteEntries(hdc, 0, cColors, lppe)
           )
        {
            UINT i;

            //
            // Convert to RGBQUAD.
            //

            for (i = 0; i < cColors; i++)
            {
                prgb[i].rgbRed      = lppe[i].peRed;
                prgb[i].rgbGreen    = lppe[i].peGreen;
                prgb[i].rgbBlue     = lppe[i].peBlue;
                prgb[i].rgbReserved = 0;
            }

            //
            // Set the DIB color table.
            //

            bRet = (BOOL) SetDIBColorTable(hdcMem, 0, cColors, prgb);

            if (!bRet)
            {
                SS_DBGPRINT("UpdateDIBColorTable: SetDIBColorTable failed\n");
            }
        }
        else
        {
            SS_DBGPRINT("UpdateDIBColorTable: MyGetSystemPaletteEntries failed\n");
        }
    }
    else
    {
        SS_DBGPRINT("UpdateDIBColorTable: GetObject failed\n");
    }

    return bRet;
}

/******************************Public*Routine******************************\
* GetCompatibleDIBInfo
*
* Copies pointer to bitmap origin to ppvBase and bitmap stride to plStride.
* Win32 DIBs can be created bottom-up (the default) with the origin at the
* lower left corner or top-down with the origin at the upper left corner.
* If the bitmap is top-down, *plStride is positive; if bottom-up, *plStride
* us negative.
*
* Also, because of restrictions on the alignment of scan lines the width
* the bitmap is often not the same as the stride (stride is the number of
* bytes between vertically adjacent pixels).
*
* The ppvBase and plStride value returned will allow you to address any
* given pixel (x, y) in the bitmap as follows:
*
* PIXEL *ppix;
*
* ppix = (PIXEL *) (((BYTE *)*ppvBase) + (y * *plStride) + (x * sizeof(PIXEL)));
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  02-Feb-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY
GetCompatibleDIBInfo(HBITMAP hbm, PVOID *ppvBase, LONG *plStride)
{
    BOOL bRet = FALSE;
    DIBSECTION ds;

    //
    // Call GetObject to return a DIBSECTION.  If successful, the
    // bitmap is a DIB section and we can retrieve the pointer to
    // the bitmap bits and other parameters.
    //

    if ( (GetObject(hbm, sizeof(ds), &ds) == sizeof(ds))
         && ds.dsBm.bmBits )
    {
        // For backwards compatibility with Get/SetBitmapBits, GDI does
        // not accurately report the bitmap pitch in bmWidthBytes.  It
        // always computes bmWidthBytes assuming WORD-aligned scanlines
        // regardless of the platform.
        //
        // Therefore, if the platform is WinNT, which uses DWORD-aligned
        // scanlines, adjust the bmWidthBytes value.

        {
            OSVERSIONINFO osvi;

            osvi.dwOSVersionInfoSize = sizeof(osvi);
            if (GetVersionEx(&osvi))
            {
                if ( osvi.dwPlatformId == VER_PLATFORM_WIN32_NT )
                {
                    ds.dsBm.bmWidthBytes = (ds.dsBm.bmWidthBytes + 3) & ~3;
                }
            }
            else
            {
                SS_DBGPRINT1("GetCompatibleDIBInfo: GetVersionEx failed with %d\n", GetLastError());
                return bRet;
            }
        }

        //
        // If biHeight is positive, then the bitmap is a bottom-up DIB.
        // If biHeight is negative, then the bitmap is a top-down DIB.
        //

        if ( ds.dsBmih.biHeight > 0 )
        {
            *ppvBase  = (PVOID) (((INT_PTR) ds.dsBm.bmBits) + (ds.dsBm.bmWidthBytes * (ds.dsBm.bmHeight - 1)));
            *plStride = (ULONG) (-ds.dsBm.bmWidthBytes);
        }
        else
        {
            *ppvBase  = ds.dsBm.bmBits;
            *plStride = ds.dsBm.bmWidthBytes;
        }

        bRet = TRUE;
    }
    else
    {
        SS_DBGPRINT("GetCompatibleDIBInfo: cannot get pointer to DIBSECTION bmBits\n");
    }

    return bRet;
}

/******************************Public*Routine******************************\
* GetDIBTranslationVector
*
* Copies the translation vector that maps colors in the specified palette,
* hpal, to the DIB selected into the specified DC, hdcMem.
*
* Effects:
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  02-Feb-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY
GetDIBTranslationVector(HDC hdcMem, HPALETTE hpal, BYTE *pbVector)
{
    BOOL bRet = FALSE;
    HBITMAP hbm;
    DIBSECTION ds;

    //
    // Validate parameters.
    //

    if ( GetObjectType(hdcMem) != OBJ_MEMDC ||
         GetObjectType(hpal) != OBJ_PAL ||
         !pbVector )
    {
        SS_DBGPRINT("GetDIBTranslationVector: bad parameter\n");
        return bRet;
    }

    //
    // The function bComputeLogicalToSurfaceMap cannot handle palettes
    // greater than 256 entries.
    //

    if ( GetPaletteEntries(hpal, 0, 1, NULL) > 256 )
    {
        SS_DBGPRINT("GetDIBTranslationVector: palette too big\n");
        return bRet;
    }

    //
    // The DIB must have a color table.
    //

    hbm = GetCurrentObject(hdcMem, OBJ_BITMAP);
    if ( (GetObject(hbm, sizeof(ds), &ds) == sizeof(ds))
         && (ds.dsBmih.biBitCount <= 8) )
    {
        bRet = bComputeLogicalToSurfaceMap(hdcMem, hpal, pbVector);
    }
    else
    {
        SS_DBGPRINT("GetDIBTranslationVector: not a DIB section\n");
        return bRet;
    }

    return bRet;
}

//////////////////// Below here are internal-only routines ////////////////////

/******************************Public*Routine******************************\
* bFillBitmapInfo
*
* Fills in the fields of a BITMAPINFO so that we can create a bitmap
* that matches the format of the display.
*
* This is done by creating a compatible bitmap and calling GetDIBits
* to return the color masks.  This is done with two calls.  The first
* call passes in biBitCount = 0 to GetDIBits which will fill in the
* base BITMAPINFOHEADER data.  The second call to GetDIBits (passing
* in the BITMAPINFO filled in by the first call) will return the color
* table or bitmasks, as appropriate.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  07-Jun-1995 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

static BOOL
bFillBitmapInfo(HDC hdc, HPALETTE hpal, BITMAPINFO *pbmi)
{
    HBITMAP hbm;
    BOOL    bRet = FALSE;

    //
    // Create a dummy bitmap from which we can query color format info
    // about the device surface.
    //

    if ( (hbm = CreateCompatibleBitmap(hdc, 1, 1)) != NULL )
    {
        pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

        //
        // Call first time to fill in BITMAPINFO header.
        //

        GetDIBits(hdc, hbm, 0, 0, NULL, pbmi, DIB_RGB_COLORS);

        if ( pbmi->bmiHeader.biBitCount <= 8 )
        {
            bRet = bFillColorTable(hdc, hpal, pbmi);
        }
        else
        {
            if ( pbmi->bmiHeader.biCompression == BI_BITFIELDS )
            {
                //
                // Call a second time to get the color masks.
                // It's a GetDIBits Win32 "feature".
                //

                GetDIBits(hdc, hbm, 0, pbmi->bmiHeader.biHeight, NULL, pbmi,
                          DIB_RGB_COLORS);
            }

            bRet = TRUE;
        }

        DeleteObject(hbm);
    }
    else
    {
        SS_DBGPRINT("bFillBitmapInfo: CreateCompatibleBitmap failed\n");
    }

    return bRet;
}

/******************************Public*Routine******************************\
* bFillColorTable
*
* Initialize the color table of the BITMAPINFO pointed to by pbmi.  Colors
* are set to the current system palette.
*
* Note: call only valid for displays of 8bpp or less.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  23-Jan-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

static BOOL
bFillColorTable(HDC hdc, HPALETTE hpal, BITMAPINFO *pbmi)
{
    BOOL bRet = FALSE;
    BYTE aj[sizeof(PALETTEENTRY) * 256];
    LPPALETTEENTRY lppe = (LPPALETTEENTRY) aj;
    RGBQUAD *prgb = (RGBQUAD *) &pbmi->bmiColors[0];
    ULONG i, cColors;

    cColors = 1 << pbmi->bmiHeader.biBitCount;
    if ( cColors <= 256 )
    {
        if ( hpal ? GetPaletteEntries(hpal, 0, cColors, lppe)
                  : MyGetSystemPaletteEntries(hdc, 0, cColors, lppe) )
        {
            UINT i;

            for (i = 0; i < cColors; i++)
            {
                prgb[i].rgbRed      = lppe[i].peRed;
                prgb[i].rgbGreen    = lppe[i].peGreen;
                prgb[i].rgbBlue     = lppe[i].peBlue;
                prgb[i].rgbReserved = 0;
            }

            bRet = TRUE;
        }
        else
        {
            SS_DBGPRINT("bFillColorTable: MyGetSystemPaletteEntries failed\n");
        }
    }

    return bRet;
}

/******************************Public*Routine******************************\
* MyGetSystemPaletteEntries
*
* Internal version of GetSystemPaletteEntries.
*
* GetSystemPaletteEntries fails on some 4bpp devices.  This version
* will detect the 4bpp case and supply the hardcoded 16-color VGA palette.
* Otherwise, it will pass the call on to GDI's GetSystemPaletteEntries.
*
* It is expected that this call will only be called in the 4bpp and 8bpp
* cases as it is not necessary for OpenGL to query the system palette
* for > 8bpp devices.
*
* History:
*  17-Aug-1995 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

static PALETTEENTRY gapeVgaPalette[16] =
{
    { 0,   0,   0,    0 },
    { 0x80,0,   0,    0 },
    { 0,   0x80,0,    0 },
    { 0x80,0x80,0,    0 },
    { 0,   0,   0x80, 0 },
    { 0x80,0,   0x80, 0 },
    { 0,   0x80,0x80, 0 },
    { 0x80,0x80,0x80, 0 },
    { 0xC0,0xC0,0xC0, 0 },
    { 0xFF,0,   0,    0 },
    { 0,   0xFF,0,    0 },
    { 0xFF,0xFF,0,    0 },
    { 0,   0,   0xFF, 0 },
    { 0xFF,0,   0xFF, 0 },
    { 0,   0xFF,0xFF, 0 },
    { 0xFF,0xFF,0xFF, 0 }
};

static UINT
MyGetSystemPaletteEntries(HDC hdc, UINT iStartIndex, UINT nEntries,
                          LPPALETTEENTRY lppe)
{
    int nDeviceBits;

    nDeviceBits = GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES);

    //
    // Some 4bpp displays will fail the GetSystemPaletteEntries call.
    // So if detected, return the hardcoded table.
    //

    if ( nDeviceBits == 4 )
    {
        if ( lppe )
        {
            nEntries = min(nEntries, (16 - iStartIndex));

            memcpy(lppe, &gapeVgaPalette[iStartIndex],
                   nEntries * sizeof(PALETTEENTRY));
        }
        else
            nEntries = 16;

        return nEntries;
    }
    else
    {
        return GetSystemPaletteEntries(hdc, iStartIndex, nEntries, lppe);
    }
}

/******************************Public*Routine******************************\
* bComputeLogicalToSurfaceMap
*
* Copy logical palette to surface palette translation vector to the buffer
* pointed to by pajVector.  The logical palette is specified by hpal.  The
* surface is specified by hdc.
*
* Note: The hdc may identify either a direct (display) dc or a DIB memory dc.
* If hdc is a display dc, then the surface palette is the system palette.
* If hdc is a memory dc, then the surface palette is the DIB color table.
*
* History:
*  27-Jan-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

static BOOL bComputeLogicalToSurfaceMap(HDC hdc, HPALETTE hpal, BYTE *pajVector)
{
    BOOL bRet = FALSE;
    HPALETTE hpalSurf;
    ULONG cEntries, cSysEntries;
    DWORD dwDcType = GetObjectType(hdc);
    LPPALETTEENTRY lppeTmp, lppeEnd;

    BYTE aj[sizeof(LOGPALETTE) + (sizeof(PALETTEENTRY) * 512) + (sizeof(RGBQUAD) * 256)];
    LOGPALETTE *ppal = (LOGPALETTE *) aj;
    LPPALETTEENTRY lppeSurf = &ppal->palPalEntry[0];
    LPPALETTEENTRY lppe = lppeSurf + 256;
    RGBQUAD *prgb = (RGBQUAD *) (lppe + 256);

    //
    // Determine number of colors in each palette.
    //

    cEntries = GetPaletteEntries(hpal, 0, 1, NULL);
    if ( dwDcType == OBJ_DC )
        cSysEntries = MyGetSystemPaletteEntries(hdc, 0, 1, NULL);
    else
        cSysEntries = 256;

    //
    // Get the logical palette entries.
    //

    cEntries = GetPaletteEntries(hpal, 0, cEntries, lppe);

    //
    // Get the surface palette entries.
    //

    if ( dwDcType == OBJ_DC )
    {
        cSysEntries = MyGetSystemPaletteEntries(hdc, 0, cSysEntries, lppeSurf);

        lppeTmp = lppeSurf;
        lppeEnd = lppeSurf + cSysEntries;

        for (; lppeTmp < lppeEnd; lppeTmp++)
            lppeTmp->peFlags = 0;
    }
    else
    {
        RGBQUAD *prgbTmp;

        //
        // First get RGBQUADs from DIB color table...
        //

        cSysEntries = GetDIBColorTable(hdc, 0, cSysEntries, prgb);

        //
        // ...then convert RGBQUADs into PALETTEENTRIES.
        //

        prgbTmp = prgb;
        lppeTmp = lppeSurf;
        lppeEnd = lppeSurf + cSysEntries;

        while ( lppeTmp < lppeEnd )
        {
            lppeTmp->peRed   = prgbTmp->rgbRed;
            lppeTmp->peGreen = prgbTmp->rgbGreen;
            lppeTmp->peBlue  = prgbTmp->rgbBlue;
            lppeTmp->peFlags = 0;

            lppeTmp++;
            prgbTmp++;

        }
    }

    //
    // Construct a translation vector by using GetNearestPaletteIndex to
    // map each entry in the logical palette to the surface palette.
    //

    if ( cEntries && cSysEntries )
    {
        //
        // Create a temporary logical palette that matches the surface
        // palette retrieved above.
        //

        ppal->palVersion = 0x300;
        ppal->palNumEntries = (USHORT) cSysEntries;

        if ( hpalSurf = CreatePalette(ppal) )
        {
            //
            // Translate each logical palette entry into a surface palette
            // index.
            //

            lppeTmp = lppe;
            lppeEnd = lppe + cEntries;

            for ( ; lppeTmp < lppeEnd; lppeTmp++, pajVector++)
            {
                *pajVector = (BYTE) GetNearestPaletteIndex(
                                        hpalSurf,
                                        RGB(lppeTmp->peRed,
                                            lppeTmp->peGreen,
                                            lppeTmp->peBlue)
                                        );
            }

            bRet = TRUE;

            DeleteObject(hpalSurf);
        }
        else
        {
            SS_DBGPRINT("bComputeLogicalToSurfaceMap: CreatePalette failed\n");
        }
    }
    else
    {
        SS_DBGPRINT("bComputeLogicalToSurfaceMap: failed to get pal info\n");
    }

    return bRet;
}
