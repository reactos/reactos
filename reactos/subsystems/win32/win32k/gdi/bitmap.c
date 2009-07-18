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

BOOL APIENTRY RosGdiAlphaBlend(HDC devDst, INT xDst, INT yDst, INT widthDst, INT heightDst,
                             HDC devSrc, INT xSrc, INT ySrc, INT widthSrc, INT heightSrc,
                             BLENDFUNCTION blendfn)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL APIENTRY RosGdiBitBlt( HDC physDevDst, INT xDst, INT yDst,
                    INT width, INT height, HDC physDevSrc,
                    INT xSrc, INT ySrc, DWORD rop )
{
    BOOLEAN bRes;
    PDC pSrc, pDst;

    DPRINT1("BitBlt %x -> %x\n", physDevSrc, physDevDst);

    /* Get a pointer to the DCs */
    pSrc = GDI_GetObjPtr(physDevSrc, (SHORT)GDI_OBJECT_TYPE_DC);
    pDst = GDI_GetObjPtr(physDevDst, (SHORT)GDI_OBJECT_TYPE_DC);

    /* Call the internal helper */
    bRes = GreBitBlt(pDst, xDst, yDst, width, height, pSrc, xSrc, ySrc, rop);

    /* Release DC objects */
    GDI_ReleaseObj(physDevDst);
    GDI_ReleaseObj(physDevSrc);

    /* Return status */
    return bRes;
}

BOOL APIENTRY RosGdiCreateBitmap( HDC physDev, HBITMAP hUserBitmap, BITMAP *pBitmap, LPVOID bmBits )
{
    HBITMAP hBitmap;
    SIZEL slSize;
    ULONG ulFormat;

    DPRINT("RosGdiCreateBitmap %dx%d %d bpp, bmBits %p\n", pBitmap->bmWidth, pBitmap->bmHeight, pBitmap->bmBitsPixel, bmBits);

    /* Convert dimensions into a SIZEL structure */
    slSize.cx = pBitmap->bmWidth;
    slSize.cy = abs(pBitmap->bmHeight);

    /* Convert format */
    ulFormat = GrepBitmapFormat(pBitmap->bmBitsPixel, BI_RGB);

    /* Call GRE to create the bitmap object */
    hBitmap = GreCreateBitmap(slSize,
                              pBitmap->bmWidthBytes,
                              ulFormat,
                              (pBitmap->bmHeight < 0 ? BMF_TOPDOWN : 0),
                              NULL);

    /* Return failure if no bitmap was created */
    if (!hBitmap) return FALSE;

    /* Map handles */
    GDI_AddHandleMapping(hBitmap, hUserBitmap);

    DPRINT("Created bitmap %x (user handle %x)\n", hBitmap, hUserBitmap);

    /* Indicate success */
    return TRUE;
}

HBITMAP APIENTRY RosGdiCreateDIBSection( HDC physDev, HBITMAP hbitmap,
                                       const BITMAPINFO *bmi, UINT usage )
{
    UNIMPLEMENTED;
    return 0;
}

BOOL APIENTRY RosGdiDeleteBitmap( HBITMAP hbitmap )
{
    HGDIOBJ hKernel = GDI_MapUserHandle(hbitmap);

    /* Fail if this object doesn't exist */
    if (!hKernel) return FALSE;

    /* Delete U->K mapping */
    GDI_RemoveHandleMapping(hbitmap);

    /* Delete the bitmap */
    GreDeleteBitmap(hKernel);

    /* Indicate success */
    return TRUE;
}

LONG APIENTRY RosGdiGetBitmapBits( HBITMAP hbitmap, void *buffer, LONG Bytes )
{
    PSURFACE psurf;
    LONG bmSize, ret;
    HGDIOBJ hKernel;

    if (buffer != NULL && Bytes == 0)
    {
        return 0;
    }

    /* Get kernelmode bitmap handle */
    hKernel = GDI_MapUserHandle(hbitmap);

    if (!hKernel)
    {
        DPRINT1("Trying to GetBitmapBits of an unkown bitmap (uhandle %x)\n", hbitmap);
        return 0;
    }

    psurf = GDI_GetObjPtr(hKernel, (SHORT)GDI_OBJECT_TYPE_BITMAP);
    if (!psurf)
    {
        return 0;
    }

    bmSize = BITMAP_GetWidthBytes(psurf->SurfObj.sizlBitmap.cx,
             BitsPerFormat(psurf->SurfObj.iBitmapFormat)) *
             abs(psurf->SurfObj.sizlBitmap.cy);

    /* If the bits vector is null, the function should return the read size */
    if (buffer == NULL)
    {
        GDI_ReleaseObj(psurf);
        return bmSize;
    }

    /* Don't copy more bytes than the buffer has */
    Bytes = min(Bytes, bmSize);

    /* Get actual bitmap bits */
    ret = GreGetBitmapBits(psurf, Bytes, buffer);

    /* Release bitmap pointer */
    GDI_ReleaseObj(psurf);

    return ret;
}

INT APIENTRY RosGdiGetDIBits( HDC physDev, HBITMAP hbitmap, UINT startscan, UINT lines,
                            LPVOID bits, BITMAPINFO *info, UINT coloruse )
{
    UNIMPLEMENTED;
    return 0;
}

COLORREF APIENTRY RosGdiGetPixel( HDC physDev, INT x, INT y )
{
    UNIMPLEMENTED;
    return 0;
}

BOOL APIENTRY RosGdiPatBlt( HDC physDev, INT left, INT top, INT width, INT height, DWORD rop )
{
    BOOLEAN bRet;
    PDC pDst;

    /* Get a pointer to the DCs */
    pDst = GDI_GetObjPtr(physDev, (SHORT)GDI_OBJECT_TYPE_DC);

    /* Call the internal helper */
    bRet = GrePatBlt(pDst, left, top, width, height, rop, pDst->pLineBrush);

    /* Release DC objects */
    GDI_ReleaseObj(physDev);

    /* Return status */
    return bRet;
}

LONG APIENTRY RosGdiSetBitmapBits( HBITMAP hbitmap, const void *bits, LONG count )
{
    UNIMPLEMENTED;
    return 0;
}

UINT APIENTRY RosGdiSetDIBColorTable( HDC physDev, UINT start, UINT count, const RGBQUAD *colors )
{
    UNIMPLEMENTED;
    return 0;
}

INT APIENTRY RosGdiSetDIBits(HDC physDev, HBITMAP hUserBitmap, UINT StartScan,
                            UINT ScanLines, LPCVOID Bits, const BITMAPINFO *bmi, UINT ColorUse)
{
    PDC pDC;

    HGDIOBJ hBitmap = GDI_MapUserHandle(hUserBitmap);

    /* Get a pointer to the DCs */
    pDC = GDI_GetObjPtr(physDev, (SHORT)GDI_OBJECT_TYPE_DC);

    DPRINT1("RosGdiSetDIBits for bitmap %x (user handle %x), StartScan %d, ScanLines %d\n",
        hBitmap, hUserBitmap, StartScan, ScanLines);

    /* Set the bits */
    GreSetDIBits(pDC,
                 hBitmap,
                 StartScan,
                 ScanLines,
                 Bits,
                 bmi,
                 ColorUse);

    /* Release DC objects */
    GDI_ReleaseObj(physDev);

    /* Return amount of lines set */
    return ScanLines;
}

INT APIENTRY RosGdiSetDIBitsToDevice( HDC physDev, INT xDest, INT yDest, DWORD cx,
                                    DWORD cy, INT xSrc, INT ySrc,
                                    UINT startscan, UINT lines, LPCVOID bits,
                                    const BITMAPINFO *info, UINT coloruse )
{
    UNIMPLEMENTED;
    return 0;
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
    pDcDest = GDI_GetObjPtr(physDevDst, (SHORT)GDI_OBJECT_TYPE_DC);

    if (physDevSrc)
        pDcSrc = GDI_GetObjPtr(physDevSrc, (SHORT)GDI_OBJECT_TYPE_DC);


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
    GDI_ReleaseObj(physDevDst);
    if (physDevSrc) GDI_ReleaseObj(physDevSrc);

    /* Return result */
    return bRet;
}


/* EOF */
