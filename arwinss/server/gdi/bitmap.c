/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         LGPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/gdi/bitmap.c
 * PURPOSE:         ReactOS GDI bitmap syscalls
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

BOOL APIENTRY RosGdiAlphaBlend(HDC physDevDst, INT xDst, INT yDst, INT widthDst, INT heightDst,
                             HDC physDevSrc, INT xSrc, INT ySrc, INT widthSrc, INT heightSrc,
                             BLENDFUNCTION blendfn)
{
    BOOLEAN bRes;
    PDC pSrc, pDst;

    DPRINT("AlphaBlend %x -> %x\n", physDevSrc, physDevDst);

    /* Get a pointer to the DCs */
    pSrc = DC_LockDc(physDevSrc);
    if (physDevSrc != physDevDst)
        pDst = DC_LockDc(physDevDst);
    else
        pDst = pSrc;

    /* Call the internal helper */
    bRes = GreAlphaBlend(pDst, xDst, yDst, widthDst, heightDst, pSrc, xSrc, ySrc, widthSrc, heightSrc, blendfn);

    /* Release DC objects */
    if (physDevSrc != physDevDst) DC_UnlockDc(pDst);
    DC_UnlockDc(pSrc);

    /* Return status */
    return bRes;
}

BOOL APIENTRY RosGdiBitBlt( HDC physDevDst, INT xDst, INT yDst,
                    INT width, INT height, HDC physDevSrc,
                    INT xSrc, INT ySrc, DWORD rop )
{
    BOOLEAN bRes;
    PDC pSrc, pDst;

    DPRINT("BitBlt %x -> %x\n", physDevSrc, physDevDst);

    /* Get a pointer to the DCs */
    pSrc = DC_LockDc(physDevSrc);
    if (physDevSrc != physDevDst)
        pDst = DC_LockDc(physDevDst);
    else
        pDst = pSrc;

    /* Call the internal helper */
    bRes = GreBitBlt(pDst, xDst, yDst, width, height, pSrc, xSrc, ySrc, rop);

    /* Release DC objects */
    if (physDevSrc != physDevDst) DC_UnlockDc(pDst);
    DC_UnlockDc(pSrc);

    /* Return status */
    return bRes;
}

HBITMAP APIENTRY RosGdiCreateBitmap( HDC physDev, BITMAP *pBitmap, LPVOID bmBits )
{
    HBITMAP hBitmap;
    SIZEL slSize;
    ULONG ulFormat;
    ULONG ulFlags = 0;
    PSURFACE pSurface;

    DPRINT("RosGdiCreateBitmap %dx%d %d bpp, bmBits %p\n", pBitmap->bmWidth, pBitmap->bmHeight, pBitmap->bmBitsPixel, bmBits);

    /* Convert dimensions into a SIZEL structure */
    slSize.cx = pBitmap->bmWidth;
    slSize.cy = abs(pBitmap->bmHeight);

    /* Convert format */
    ulFormat = GrepBitmapFormat(pBitmap->bmBitsPixel, BI_RGB);

    /* Set flags */
    if (bmBits) ulFlags |= BMF_NOZEROINIT;
    if (pBitmap->bmHeight < 0) ulFlags |= BMF_TOPDOWN;

    /* Call GRE to create the bitmap object */
    hBitmap = GreCreateBitmap(slSize,
                              pBitmap->bmWidthBytes,
                              ulFormat,
                              ulFlags,
                              NULL);

    /* Return failure if no bitmap was created */
    if (!hBitmap) return 0;

    /* Set its bits if any */
    if (bmBits)
    {
        /* Get the object pointer */
        pSurface = SURFACE_LockSurface(hBitmap);

        /* Copy bits */
        GreSetBitmapBits(pSurface, pSurface->SurfObj.cjBits, bmBits);

        /* Release the surface */
        SURFACE_UnlockSurface(pSurface);
    }

    DPRINT("Created bitmap %x \n", hBitmap);

    /* Indicate success */
    return hBitmap;
}

HBITMAP APIENTRY RosGdiCreateDIBSection( HDC physDev, 
                                       const BITMAPINFO *bmi, UINT usage, DIBSECTION *dib )
{
    SIZEL szSize;
    ULONG ulFormat;
    HBITMAP hbmDIB;
    RGBQUAD *lpRGB = NULL;
    PDC pDC;
    SURFACE *dibSurf;

    /* Get DIB section size */
    szSize.cx = dib->dsBm.bmWidth;
    szSize.cy = dib->dsBm.bmHeight;

    /* Get its format */
    ulFormat = GrepBitmapFormat(dib->dsBmih.biBitCount * dib->dsBmih.biPlanes,
                                dib->dsBmih.biCompression);

    /* Create the bitmap */
    hbmDIB = GreCreateBitmap(szSize,
                             dib->dsBm.bmWidthBytes, ulFormat,
                             BMF_DONTCACHE | BMF_USERMEM | BMF_NOZEROINIT |
                             (dib->dsBmih.biHeight < 0 ? BMF_TOPDOWN : 0),
                             dib->dsBm.bmBits);

    dib->dsBmih.biClrUsed = 0;
    /* set number of entries in bmi.bmiColors table */
    if (dib->dsBmih.biBitCount == 1)
    {
        dib->dsBmih.biClrUsed = 2;
    }
    else if (dib->dsBmih.biBitCount == 4)
    {
        dib->dsBmih.biClrUsed = 16;
    }
    else if (dib->dsBmih.biBitCount == 8)
    {
        dib->dsBmih.biClrUsed = 256;
    }

    dibSurf = SURFACE_LockSurface(hbmDIB);

    if (dib->dsBmih.biClrUsed != 0)
    {
        if (usage == DIB_PAL_COLORS)
        {
            pDC = DC_LockDc(physDev);
            lpRGB = DIB_MapPaletteColors(pDC, bmi);
            DC_UnlockDc(pDC);
            dibSurf->hDIBPalette = PALETTE_AllocPaletteIndexedRGB(dib->dsBmih.biClrUsed, lpRGB);
        }
        else
        {
            dibSurf->hDIBPalette = PALETTE_AllocPaletteIndexedRGB(dib->dsBmih.biClrUsed, bmi->bmiColors);
        }
    }
    else
    {
        dibSurf->hDIBPalette = PALETTE_AllocPalette(PAL_BITFIELDS, 0, NULL,
                                                    dib->dsBitfields[0],
                                                    dib->dsBitfields[1],
                                                    dib->dsBitfields[2]);
    }

    SURFACE_UnlockSurface(dibSurf);

    if (lpRGB)
    {
        ExFreePoolWithTag(lpRGB, TAG_COLORMAP);
    }

    DPRINT("Created bitmap %x for DIB section\n", hbmDIB);

    /* Return success */
    return hbmDIB;
}

BOOL APIENTRY RosGdiDeleteBitmap( HBITMAP hbitmap )
{
    /* Delete the bitmap */
    GreDeleteObject(hbitmap);

    /* Indicate success */
    return TRUE;
}

LONG APIENTRY RosGdiGetBitmapBits( HBITMAP hbitmap, void *buffer, LONG Bytes )
{
    PSURFACE psurf;
    LONG bmSize, ret;

    if (buffer != NULL && Bytes == 0)
    {
        return 0;
    }

    psurf = SURFACE_LockSurface(hbitmap);
    if (!psurf) return 0;

    bmSize = BITMAP_GetWidthBytes(psurf->SurfObj.sizlBitmap.cx,
             BitsPerFormat(psurf->SurfObj.iBitmapFormat)) *
             abs(psurf->SurfObj.sizlBitmap.cy);

    /* If the bits vector is null, the function should return the read size */
    if (buffer == NULL)
    {
        SURFACE_UnlockSurface(psurf);
        return bmSize;
    }

    /* Don't copy more bytes than the buffer has */
    Bytes = min(Bytes, bmSize);

    /* Get actual bitmap bits */
    ret = GreGetBitmapBits(psurf, Bytes, buffer);

    /* Release bitmap pointer */
    SURFACE_UnlockSurface(psurf);

    return ret;
}

INT APIENTRY RosGdiGetDIBits( HDC physDev, HBITMAP hBitmap, UINT StartScan, UINT ScanLines,
                            LPVOID Bits, BITMAPINFO *bmi, UINT ColorUse, DIBSECTION *dib )
{
    PDC pDC;

    /* Get a pointer to the DCs */
    pDC = DC_LockDc(physDev);

    DPRINT("RosGdiGetDIBits for bitmap %x , StartScan %d, ScanLines %d, height %d\n",
        hBitmap, StartScan, ScanLines, dib->dsBm.bmHeight);

    /* Set the bits */
    GreGetDIBits(pDC,
                 hBitmap,
                 StartScan,
                 ScanLines,
                 Bits,
                 bmi,
                 ColorUse);

    /* Release DC objects */
    DC_UnlockDc(pDC);

    /* Return amount of lines set */
    return ScanLines;
}

COLORREF APIENTRY RosGdiGetPixel( HDC physDev, INT x, INT y )
{
    PDC pDC;
    COLORREF crPixel;

    /* Get a pointer to the DC */
    pDC = DC_LockDc(physDev);

    crPixel = GreGetPixel(pDC, x, y);

    /* Release DC */
    DC_UnlockDc(pDC);

    return crPixel;
}

COLORREF APIENTRY RosGdiSetPixel( HDC physDev, INT x, INT y, COLORREF color )
{
    HBRUSH old_brush, new_brush;

    new_brush = GreCreateSolidBrush(color);
    old_brush = GreSelectBrush(physDev, new_brush);

    RosGdiPatBlt(physDev, x, y, 1, 1, PATCOPY);

    new_brush = GreSelectBrush(physDev, old_brush);

    GreDeleteObject(new_brush);

    return color;
}

BOOL APIENTRY RosGdiPatBlt( HDC physDev, INT left, INT top, INT width, INT height, DWORD rop )
{
    BOOLEAN bRet;
    PDC pDst;

    DPRINT("PatBlt hdc %x, offs (%d,%d), w %d, h %d\n", physDev, left, top, width, height);

    /* Get a pointer to the DCs */
    pDst = DC_LockDc(physDev);

    /* Call the internal helper */
    bRet = GrePatBlt(pDst, left, top, width, height, rop, pDst->dclevel.pbrFill);

    /* Release DC objects */
    DC_UnlockDc(pDst);

    /* Return status */
    return bRet;
}

LONG APIENTRY RosGdiSetBitmapBits( HBITMAP hbitmap, const void *bits, LONG count )
{
    UNIMPLEMENTED;
    return 0;
}

UINT APIENTRY RosGdiSetDIBColorTable( HDC physDev, UINT StartIndex, UINT Entries, const RGBQUAD *Colors )
{
    PDC pDC;

    /* Get a pointer to the DCs */
    pDC = DC_LockDc(physDev);

    Entries = GreSetDIBColorTable(pDC, StartIndex, Entries, Colors);

    /* Release DC objects */
    DC_UnlockDc(pDC);

    /* Return amount of lines set */
    return Entries;
}

INT APIENTRY RosGdiSetDIBits(HDC physDev, HBITMAP hBitmap, UINT StartScan,
                            UINT ScanLines, LPCVOID Bits, const BITMAPINFO *bmi, UINT ColorUse)
{
    PDC pDC;

    /* Get a pointer to the DCs */
    pDC = DC_LockDc(physDev);

    DPRINT("RosGdiSetDIBits for bitmap %x, StartScan %d, ScanLines %d\n",
        hBitmap, StartScan, ScanLines);

    /* Set the bits */
    ScanLines = GreSetDIBits(pDC,
                             hBitmap,
                             StartScan,
                             ScanLines,
                             Bits,
                             bmi,
                             ColorUse);

    /* Release DC objects */
    DC_UnlockDc(pDC);

    /* Return amount of lines set */
    return ScanLines;
}

INT APIENTRY RosGdiSetDIBitsToDevice( HDC physDev, INT xDest, INT yDest, DWORD cx,
                                    DWORD cy, INT xSrc, INT ySrc,
                                    UINT StartScan, UINT ScanLines, LPCVOID Bits,
                                    const BITMAPINFO *bmi, UINT ColorUse )
{
    PDC pDC;

    /* Get a pointer to the DCs */
    pDC = DC_LockDc(physDev);

    /* Set the bits */
    ScanLines = GreSetDIBitsToDevice(pDC,
                                     xDest,
                                     yDest,
                                     cx,
                                     cy,
                                     xSrc,
                                     ySrc,
                                     StartScan,
                                     ScanLines,
                                     Bits,
                                     bmi,
                                     ColorUse);

    /* Release DC objects */
    DC_UnlockDc(pDC);

    /* Return amount of lines set */
    return ScanLines;
}

BOOL APIENTRY RosGdiStretchBlt( HDC physDevDst, INT xDst, INT yDst,
                              INT widthDst, INT heightDst,
                              HDC physDevSrc, INT xSrc, INT ySrc,
                              INT widthSrc, INT heightSrc, DWORD rop )
{
    PDC pDcDest, pDcSrc = NULL;
    BOOL bRet;

    /* Check parameters */
    if (!widthDst || !heightDst || !widthSrc || !heightSrc)
    {
        return FALSE;
    }

    /* Get a pointer to the DCs */
    pDcDest = DC_LockDc(physDevDst);

    if (physDevSrc)
        pDcSrc = DC_LockDc(physDevSrc);

    bRet = GreStretchBltMask(
                pDcDest,
                xDst,
                yDst,
                widthDst,
                heightDst,
                pDcSrc,
                xSrc,
                ySrc,
                widthSrc,
                heightSrc,
                rop,
                0,
                NULL);

    /* Release DC objects */
    DC_UnlockDc(pDcDest);
    if (pDcSrc) DC_UnlockDc(pDcSrc);

    /* Return result */
    return bRet;
}

/* EOF */
