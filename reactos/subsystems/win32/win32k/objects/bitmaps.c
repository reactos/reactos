/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$ */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

HBITMAP APIENTRY
IntGdiCreateBitmap(
    INT Width,
    INT Height,
    UINT Planes,
    UINT BitsPixel,
    IN OPTIONAL LPBYTE pBits)
{
    HBITMAP hBitmap;
    SIZEL Size;
    LONG WidthBytes;
    PSURFACE psurfBmp;

    /* NOTE: Windows also doesn't store nr. of planes separately! */
    BitsPixel = BITMAP_GetRealBitsPixel(BitsPixel * Planes);

    /* Check parameters */
    if (BitsPixel == 0 || Width <= 0 || Width >= 0x8000000 || Height == 0)
    {
        DPRINT1("Width = %d, Height = %d BitsPixel = %d\n",
                Width, Height, BitsPixel);
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return 0;
    }

    WidthBytes = BITMAP_GetWidthBytes(Width, BitsPixel);

    Size.cx = Width;
    Size.cy = abs(Height);

    /* Make sure that cjBits will not overflow */
    if ((ULONGLONG)WidthBytes * Size.cy >= 0x100000000ULL)
    {
        DPRINT1("Width = %d, Height = %d BitsPixel = %d\n",
                Width, Height, BitsPixel);
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return 0;
    }

    /* Create the bitmap object. */
    hBitmap = IntCreateBitmap(Size, WidthBytes,
                              BitmapFormat(BitsPixel, BI_RGB),
                              (Height < 0 ? BMF_TOPDOWN : 0) |
                              (NULL == pBits ? 0 : BMF_NOZEROINIT), NULL);
    if (!hBitmap)
    {
        DPRINT("IntGdiCreateBitmap: returned 0\n");
        return 0;
    }

    psurfBmp = SURFACE_LockSurface(hBitmap);
    if (psurfBmp == NULL)
    {
        GreDeleteObject(hBitmap);
        return NULL;
    }

    psurfBmp->flFlags = BITMAPOBJ_IS_APIBITMAP;
    psurfBmp->hDC = NULL; // Fixme

    if (NULL != pBits)
    {
        IntSetBitmapBits(psurfBmp, psurfBmp->SurfObj.cjBits, pBits);
    }

    SURFACE_UnlockSurface(psurfBmp);

    DPRINT("IntGdiCreateBitmap : %dx%d, %d BPP colors, topdown %d, returning %08x\n",
           Size.cx, Size.cy, BitsPixel, (Height < 0 ? 1 : 0), hBitmap);

    return hBitmap;
}


HBITMAP APIENTRY
NtGdiCreateBitmap(
    INT Width,
    INT Height,
    UINT Planes,
    UINT BitsPixel,
    IN OPTIONAL LPBYTE pUnsafeBits)
{
    if (pUnsafeBits)
    {
        BOOL Hit = FALSE;
        UINT cjBits = BITMAP_GetWidthBytes(Width, BitsPixel) * abs(Height);

        // FIXME: Use MmSecureVirtualMemory
        _SEH2_TRY
        {
            ProbeForRead(pUnsafeBits, cjBits, 1);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Hit = TRUE;
        }
        _SEH2_END

        if (Hit) return 0;
    }

    return IntGdiCreateBitmap(Width, Height, Planes, BitsPixel, pUnsafeBits);
}

HBITMAP FASTCALL
IntCreateCompatibleBitmap(
    PDC Dc,
    INT Width,
    INT Height)
{
    HBITMAP Bmp = NULL;

    /* MS doc says if width or height is 0, return 1-by-1 pixel, monochrome bitmap */
    if (0 == Width || 0 == Height)
    {
        Bmp = NtGdiGetStockObject(DEFAULT_BITMAP);
    }
    else
    {
        if (Dc->dctype != DC_TYPE_MEMORY)
        {
            Bmp = IntGdiCreateBitmap(abs(Width),
                                     abs(Height),
                                     IntGdiGetDeviceCaps(Dc,PLANES),
                                     IntGdiGetDeviceCaps(Dc,BITSPIXEL),
                                     NULL);
        }
        else
        {
            DIBSECTION dibs;
            INT Count;
            PSURFACE psurf = SURFACE_LockSurface(Dc->rosdc.hBitmap);
            Count = BITMAP_GetObject(psurf, sizeof(dibs), &dibs);

            if (Count)
            {
                if (Count == sizeof(BITMAP))
                {
                    /* We have a bitmap bug!!! W/O the HACK, we have white icons.

                       MSDN Note: When a memory device context is created, it initially
                       has a 1-by-1 monochrome bitmap selected into it. If this memory
                       device context is used in CreateCompatibleBitmap, the bitmap that
                       is created is a monochrome bitmap. To create a color bitmap, use
                       the hDC that was used to create the memory device context, as
                       shown in the following code:

                           HDC memDC = CreateCompatibleDC(hDC);
                           HBITMAP memBM = CreateCompatibleBitmap(hDC, nWidth, nHeight);
                           SelectObject(memDC, memBM);
                     */
                    Bmp = IntGdiCreateBitmap(abs(Width),
                                             abs(Height),
                                             dibs.dsBm.bmPlanes,
                                             IntGdiGetDeviceCaps(Dc,BITSPIXEL),//<-- HACK! dibs.dsBm.bmBitsPixel, // <-- Correct!
                                             NULL);
                }
                else
                {
                    /* A DIB section is selected in the DC */
                    BITMAPINFO *bi;
                    PVOID Bits;

                    /* Allocate memory for a BITMAPINFOHEADER structure and a
                       color table. The maximum number of colors in a color table
                       is 256 which corresponds to a bitmap with depth 8.
                       Bitmaps with higher depths don't have color tables. */
                    bi = ExAllocatePoolWithTag(PagedPool,
                                               sizeof(BITMAPINFOHEADER) +
                                                   256 * sizeof(RGBQUAD),
                                               TAG_TEMP);

                    if (bi)
                    {
                        bi->bmiHeader.biSize          = sizeof(bi->bmiHeader);
                        bi->bmiHeader.biWidth         = Width;
                        bi->bmiHeader.biHeight        = Height;
                        bi->bmiHeader.biPlanes        = dibs.dsBmih.biPlanes;
                        bi->bmiHeader.biBitCount      = dibs.dsBmih.biBitCount;
                        bi->bmiHeader.biCompression   = dibs.dsBmih.biCompression;
                        bi->bmiHeader.biSizeImage     = 0;
                        bi->bmiHeader.biXPelsPerMeter = dibs.dsBmih.biXPelsPerMeter;
                        bi->bmiHeader.biYPelsPerMeter = dibs.dsBmih.biYPelsPerMeter;
                        bi->bmiHeader.biClrUsed       = dibs.dsBmih.biClrUsed;
                        bi->bmiHeader.biClrImportant  = dibs.dsBmih.biClrImportant;

                        if (bi->bmiHeader.biCompression == BI_BITFIELDS)
                        {
                            /* Copy the color masks */
                            RtlCopyMemory(bi->bmiColors, dibs.dsBitfields, 3 * sizeof(DWORD));
                        }
                        else if (bi->bmiHeader.biBitCount <= 8)
                        {
                            /* Copy the color table */
                            UINT Index;
                            PPALETTE PalGDI = PALETTE_LockPalette(psurf->hDIBPalette);

                            if (!PalGDI)
                            {
                                ExFreePoolWithTag(bi, TAG_TEMP);
                                SURFACE_UnlockSurface(psurf);
                                SetLastWin32Error(ERROR_INVALID_HANDLE);
                                return 0;
                            }

                            for (Index = 0;
                                    Index < 256 && Index < PalGDI->NumColors;
                                    Index++)
                            {
                                bi->bmiColors[Index].rgbRed   = PalGDI->IndexedColors[Index].peRed;
                                bi->bmiColors[Index].rgbGreen = PalGDI->IndexedColors[Index].peGreen;
                                bi->bmiColors[Index].rgbBlue  = PalGDI->IndexedColors[Index].peBlue;
                                bi->bmiColors[Index].rgbReserved = 0;
                            }
                            PALETTE_UnlockPalette(PalGDI);
                        }
                        SURFACE_UnlockSurface(psurf);

                        Bmp = DIB_CreateDIBSection(Dc,
                                                   bi,
                                                   DIB_RGB_COLORS,
                                                   &Bits,
                                                   NULL,
                                                   0,
                                                   0);

                        ExFreePoolWithTag(bi, TAG_TEMP);
                        return Bmp;
                    }
                }
            }
            SURFACE_UnlockSurface(psurf);
        }
    }
    return Bmp;
}

HBITMAP APIENTRY
NtGdiCreateCompatibleBitmap(
    HDC hDC,
    INT Width,
    INT Height)
{
    HBITMAP Bmp;
    PDC Dc;

    if (Width <= 0 || Height <= 0 || (Width * Height) > 0x3FFFFFFF)
    {
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    if (!hDC)
        return IntGdiCreateBitmap(Width, Height, 1, 1, 0);

    Dc = DC_LockDc(hDC);

    DPRINT("NtGdiCreateCompatibleBitmap(%04x,%d,%d, bpp:%d) = \n",
           hDC, Width, Height, Dc->ppdev->GDIInfo.cBitsPixel);

    if (NULL == Dc)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return NULL;
    }

    Bmp = IntCreateCompatibleBitmap(Dc, Width, Height);

    DPRINT("\t\t%04x\n", Bmp);
    DC_UnlockDc(Dc);
    return Bmp;
}

BOOL APIENTRY
NtGdiGetBitmapDimension(
    HBITMAP hBitmap,
    LPSIZE Dimension)
{
    PSURFACE psurfBmp;
    BOOL Ret = TRUE;

    if (hBitmap == NULL)
        return FALSE;

    psurfBmp = SURFACE_LockSurface(hBitmap);
    if (psurfBmp == NULL)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    _SEH2_TRY
    {
        ProbeForWrite(Dimension, sizeof(SIZE), 1);
        *Dimension = psurfBmp->dimension;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Ret = FALSE;
    }
    _SEH2_END

    SURFACE_UnlockSurface(psurfBmp);

    return Ret;
}

COLORREF APIENTRY
NtGdiGetPixel(HDC hDC, INT XPos, INT YPos)
{
    PDC dc = NULL;
    COLORREF Result = (COLORREF)CLR_INVALID; // default to failure
    BOOL bInRect = FALSE;
    SURFACE *psurf;
    SURFOBJ *pso;
    HPALETTE Pal = 0;
    XLATEOBJ *XlateObj;
    HBITMAP hBmpTmp;

    dc = DC_LockDc(hDC);

    if (!dc)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return Result;
    }

    if (dc->dctype == DC_TYPE_INFO)
    {
        DC_UnlockDc(dc);
        return Result;
    }

    XPos += dc->ptlDCOrig.x;
    YPos += dc->ptlDCOrig.y;
    if (RECTL_bPointInRect(&dc->rosdc.CombinedClip->rclBounds, XPos, YPos))
    {
        bInRect = TRUE;
        psurf = SURFACE_LockSurface(dc->rosdc.hBitmap);
        pso = &psurf->SurfObj;
        if (psurf)
        {
            Pal = psurf->hDIBPalette;
            if (!Pal) Pal = pPrimarySurface->DevInfo.hpalDefault;

            /* FIXME: Verify if it shouldn't be PAL_BGR! */
            XlateObj = (XLATEOBJ*)IntEngCreateXlate(PAL_RGB, 0, NULL, Pal);
            if (XlateObj)
            {
                // check if this DC has a DIB behind it...
                if (pso->pvScan0) // STYPE_BITMAP == pso->iType
                {
                    ASSERT(pso->lDelta);
                    Result = XLATEOBJ_iXlate(XlateObj,
                                             DibFunctionsForBitmapFormat[pso->iBitmapFormat].DIB_GetPixel(pso, XPos, YPos));
                }
                EngDeleteXlate(XlateObj);
            }
            SURFACE_UnlockSurface(psurf);
        }
    }
    DC_UnlockDc(dc);

    // if Result is still CLR_INVALID, then the "quick" method above didn't work
    if (bInRect && Result == CLR_INVALID)
    {
        // FIXME: create a 1x1 32BPP DIB, and blit to it
        HDC hDCTmp = NtGdiCreateCompatibleDC(hDC);
        if (hDCTmp)
        {
            static const BITMAPINFOHEADER bih = { sizeof(BITMAPINFOHEADER), 1, 1, 1, 32, BI_RGB, 0, 0, 0, 0, 0 };
            BITMAPINFO bi;
            RtlMoveMemory(&(bi.bmiHeader), &bih, sizeof(bih));
            hBmpTmp = NtGdiCreateDIBitmapInternal(hDC,
                                                  bi.bmiHeader.biWidth,
                                                  bi.bmiHeader.biHeight,
                                                  0,
                                                  NULL,
                                                  &bi,
                                                  DIB_RGB_COLORS,
                                                  bi.bmiHeader.biBitCount,
                                                  bi.bmiHeader.biSizeImage,
                                                  0,
                                                  0);

            //HBITMAP hBmpTmp = IntGdiCreateBitmap(1, 1, 1, 32, NULL);
            if (hBmpTmp)
            {
                HBITMAP hBmpOld = (HBITMAP)NtGdiSelectBitmap(hDCTmp, hBmpTmp);
                if (hBmpOld)
                {
                    PSURFACE psurf;

                    NtGdiBitBlt(hDCTmp, 0, 0, 1, 1, hDC, XPos, YPos, SRCCOPY, 0, 0);
                    NtGdiSelectBitmap(hDCTmp, hBmpOld);

                    // our bitmap is no longer selected, so we can access it's stuff...
                    psurf = SURFACE_LockSurface(hBmpTmp);
                    if (psurf)
                    {
                        // Dont you need to convert something here?
                        Result = *(COLORREF*)psurf->SurfObj.pvScan0;
                        SURFACE_UnlockSurface(psurf);
                    }
                }
                GreDeleteObject(hBmpTmp);
            }
            NtGdiDeleteObjectApp(hDCTmp);
        }
    }

    return Result;
}


LONG APIENTRY
IntGetBitmapBits(
    PSURFACE psurf,
    DWORD Bytes,
    OUT PBYTE Bits)
{
    LONG ret;

    ASSERT(Bits);

    /* Don't copy more bytes than the buffer has */
    Bytes = min(Bytes, psurf->SurfObj.cjBits);

#if 0
    /* FIXME: Call DDI CopyBits here if available  */
    if (psurf->DDBitmap)
    {
        DPRINT("Calling device specific BitmapBits\n");
        if (psurf->DDBitmap->funcs->pBitmapBits)
        {
            ret = psurf->DDBitmap->funcs->pBitmapBits(hbitmap,
                                                      bits,
                                                      count,
                                                      DDB_GET);
        }
        else
        {
            ERR_(bitmap)("BitmapBits == NULL??\n");
            ret = 0;
        }
    }
    else
#endif
    {
        RtlCopyMemory(Bits, psurf->SurfObj.pvBits, Bytes);
        ret = Bytes;
    }
    return ret;
}

LONG APIENTRY
NtGdiGetBitmapBits(
    HBITMAP hBitmap,
    ULONG Bytes,
    OUT OPTIONAL PBYTE pUnsafeBits)
{
    PSURFACE psurf;
    LONG bmSize, ret;

    if (pUnsafeBits != NULL && Bytes == 0)
    {
        return 0;
    }

    psurf = SURFACE_LockSurface(hBitmap);
    if (!psurf)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return 0;
    }

    bmSize = BITMAP_GetWidthBytes(psurf->SurfObj.sizlBitmap.cx, 
             BitsPerFormat(psurf->SurfObj.iBitmapFormat)) * 
             abs(psurf->SurfObj.sizlBitmap.cy);
    
    /* If the bits vector is null, the function should return the read size */
    if (pUnsafeBits == NULL)
    {
        SURFACE_UnlockSurface(psurf);
        return bmSize;
    }

    /* Don't copy more bytes than the buffer has */
    Bytes = min(Bytes, bmSize);

    // FIXME: use MmSecureVirtualMemory
    _SEH2_TRY
    {
        ProbeForWrite(pUnsafeBits, Bytes, 1);
        ret = IntGetBitmapBits(psurf, Bytes, pUnsafeBits);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = 0;
    }
    _SEH2_END

    SURFACE_UnlockSurface(psurf);

    return ret;
}


LONG APIENTRY
IntSetBitmapBits(
    PSURFACE psurf,
    DWORD Bytes,
    IN PBYTE Bits)
{
    LONG ret;

    /* Don't copy more bytes than the buffer has */
    Bytes = min(Bytes, psurf->SurfObj.cjBits);

#if 0
    /* FIXME: call DDI specific function here if available  */
    if (psurf->DDBitmap)
    {
        DPRINT("Calling device specific BitmapBits\n");
        if (psurf->DDBitmap->funcs->pBitmapBits)
        {
            ret = psurf->DDBitmap->funcs->pBitmapBits(hBitmap, 
                                                      (void *)Bits,
                                                      Bytes,
                                                      DDB_SET);
        }
        else
        {
            DPRINT("BitmapBits == NULL??\n");
            ret = 0;
        }
    }
    else
#endif
    {
        RtlCopyMemory(psurf->SurfObj.pvBits, Bits, Bytes);
        ret = Bytes;
    }

    return ret;
}


LONG APIENTRY
NtGdiSetBitmapBits(
    HBITMAP hBitmap,
    DWORD Bytes,
    IN PBYTE pUnsafeBits)
{
    LONG ret;
    PSURFACE psurf;

    if (pUnsafeBits == NULL || Bytes == 0)
    {
        return 0;
    }

    psurf = SURFACE_LockSurface(hBitmap);
    if (psurf == NULL)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return 0;
    }

    _SEH2_TRY
    {
        ProbeForRead(pUnsafeBits, Bytes, 1);
        ret = IntSetBitmapBits(psurf, Bytes, pUnsafeBits);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = 0;
    }
    _SEH2_END

    SURFACE_UnlockSurface(psurf);

    return ret;
}

BOOL APIENTRY
NtGdiSetBitmapDimension(
    HBITMAP hBitmap,
    INT Width,
    INT Height,
    LPSIZE Size)
{
    PSURFACE psurf;
    BOOL Ret = TRUE;

    if (hBitmap == NULL)
        return FALSE;

    psurf = SURFACE_LockSurface(hBitmap);
    if (psurf == NULL)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (Size)
    {
        _SEH2_TRY
        {
            ProbeForWrite(Size, sizeof(SIZE), 1);
            *Size = psurf->dimension;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Ret = FALSE;
        }
        _SEH2_END
    }

    /* The dimension is changed even if writing the old value failed */
    psurf->dimension.cx = Width;
    psurf->dimension.cy = Height;

    SURFACE_UnlockSurface(psurf);

    return Ret;
}

BOOL APIENTRY
GdiSetPixelV(
    HDC hDC,
    INT X,
    INT Y,
    COLORREF Color)
{
    HBRUSH hbrush = NtGdiCreateSolidBrush(Color, NULL);
    HGDIOBJ OldBrush;

    if (hbrush == NULL)
        return(FALSE);

    OldBrush = NtGdiSelectBrush(hDC, hbrush);
    if (OldBrush == NULL)
    {
        GreDeleteObject(hbrush);
        return(FALSE);
    }

    NtGdiPatBlt(hDC, X, Y, 1, 1, PATCOPY);
    NtGdiSelectBrush(hDC, OldBrush);
    GreDeleteObject(hbrush);

    return TRUE;
}

COLORREF APIENTRY
NtGdiSetPixel(
    HDC hDC,
    INT X,
    INT Y,
    COLORREF Color)
{
    DPRINT("0 NtGdiSetPixel X %ld Y %ld C %ld\n", X, Y, Color);

    if (GdiSetPixelV(hDC,X,Y,Color))
    {
        Color = NtGdiGetPixel(hDC,X,Y);
        DPRINT("1 NtGdiSetPixel X %ld Y %ld C %ld\n", X, Y, Color);
        return Color;
    }

    Color = (COLORREF)CLR_INVALID;
    DPRINT("2 NtGdiSetPixel X %ld Y %ld C %ld\n", X, Y, Color);
    return Color;
}


/*  Internal Functions  */

UINT FASTCALL
BITMAP_GetRealBitsPixel(UINT nBitsPixel)
{
    if (nBitsPixel <= 1)
        return 1;
    if (nBitsPixel <= 4)
        return 4;
    if (nBitsPixel <= 8)
        return 8;
    if (nBitsPixel <= 16)
        return 16;
    if (nBitsPixel <= 24)
        return 24;
    if (nBitsPixel <= 32)
        return 32;

    return 0;
}

INT FASTCALL
BITMAP_GetWidthBytes(INT bmWidth, INT bpp)
{
#if 0
    switch (bpp)
    {
    case 1:
        return 2 * ((bmWidth+15) >> 4);

    case 24:
        bmWidth *= 3; /* fall through */
    case 8:
        return bmWidth + (bmWidth & 1);

    case 32:
        return bmWidth * 4;

    case 16:
    case 15:
        return bmWidth * 2;

    case 4:
        return 2 * ((bmWidth+3) >> 2);

    default:
        DPRINT ("stub");
    }

    return -1;
#endif

    return ((bmWidth * bpp + 15) & ~15) >> 3;
}

HBITMAP FASTCALL
BITMAP_CopyBitmap(HBITMAP hBitmap)
{
    HBITMAP res;
    BITMAP bm;
    SURFACE *Bitmap, *resBitmap;
    SIZEL Size;

    if (hBitmap == NULL)
    {
        return 0;
    }

    Bitmap = GDIOBJ_LockObj(hBitmap, GDI_OBJECT_TYPE_BITMAP);
    if (Bitmap == NULL)
    {
        return 0;
    }

    BITMAP_GetObject(Bitmap, sizeof(BITMAP), (PVOID)&bm);
    bm.bmBits = NULL;
    if (Bitmap->SurfObj.lDelta >= 0)
        bm.bmHeight = -bm.bmHeight;

    Size.cx = abs(bm.bmWidth);
    Size.cy = abs(bm.bmHeight);
    res = IntCreateBitmap(Size,
                          bm.bmWidthBytes,
                          BitmapFormat(bm.bmBitsPixel * bm.bmPlanes, BI_RGB),
                          (bm.bmHeight < 0 ? BMF_TOPDOWN : 0) | BMF_NOZEROINIT,
                          NULL);

    if (res)
    {
        PBYTE buf;

        resBitmap = GDIOBJ_LockObj(res, GDI_OBJECT_TYPE_BITMAP);
        if (resBitmap)
        {
            buf = ExAllocatePoolWithTag(PagedPool,
                                        bm.bmWidthBytes * abs(bm.bmHeight),
                                        TAG_BITMAP);
            if (buf == NULL)
            {
                GDIOBJ_UnlockObjByPtr((POBJ)resBitmap);
                GDIOBJ_UnlockObjByPtr((POBJ)Bitmap);
                GreDeleteObject(res);
                return 0;
            }
            IntGetBitmapBits(Bitmap, bm.bmWidthBytes * abs(bm.bmHeight), buf);
            IntSetBitmapBits(resBitmap, bm.bmWidthBytes * abs(bm.bmHeight), buf);
            ExFreePoolWithTag(buf,TAG_BITMAP);
            resBitmap->flFlags = Bitmap->flFlags;
            GDIOBJ_UnlockObjByPtr((POBJ)resBitmap);
        }
        else
        {
            GreDeleteObject(res);
            res = NULL;
        }
    }

    GDIOBJ_UnlockObjByPtr((POBJ)Bitmap);

    return  res;
}

INT APIENTRY
BITMAP_GetObject(SURFACE *psurf, INT Count, LPVOID buffer)
{
    PBITMAP pBitmap;

    if (!buffer) return sizeof(BITMAP);
    if ((UINT)Count < sizeof(BITMAP)) return 0;

    /* always fill a basic BITMAP structure */
    pBitmap = buffer;
    pBitmap->bmType = 0;
    pBitmap->bmWidth = psurf->SurfObj.sizlBitmap.cx;
    pBitmap->bmHeight = psurf->SurfObj.sizlBitmap.cy;
    pBitmap->bmWidthBytes = abs(psurf->SurfObj.lDelta);
    pBitmap->bmPlanes = 1;
    pBitmap->bmBitsPixel = BitsPerFormat(psurf->SurfObj.iBitmapFormat);

    /* Check for DIB section */
    if (psurf->hSecure)
    {
        /* Set bmBits in this case */
        pBitmap->bmBits = psurf->SurfObj.pvBits;

        if (Count >= sizeof(DIBSECTION))
        {
            /* Fill rest of DIBSECTION */
            PDIBSECTION pds = buffer;

            pds->dsBmih.biSize = sizeof(BITMAPINFOHEADER);
            pds->dsBmih.biWidth = pds->dsBm.bmWidth;
            pds->dsBmih.biHeight = pds->dsBm.bmHeight;
            pds->dsBmih.biPlanes = pds->dsBm.bmPlanes;
            pds->dsBmih.biBitCount = pds->dsBm.bmBitsPixel;
            switch (psurf->SurfObj.iBitmapFormat)
            {
                /* FIXME: What about BI_BITFIELDS? */
                case BMF_1BPP:
                case BMF_4BPP:
                case BMF_8BPP:
                case BMF_16BPP:
                case BMF_24BPP:
                case BMF_32BPP:
                   pds->dsBmih.biCompression = BI_RGB;
                   break;
                case BMF_4RLE:
                   pds->dsBmih.biCompression = BI_RLE4;
                   break;
                case BMF_8RLE:
                   pds->dsBmih.biCompression = BI_RLE8;
                   break;
                case BMF_JPEG:
                   pds->dsBmih.biCompression = BI_JPEG;
                   break;
                case BMF_PNG:
                   pds->dsBmih.biCompression = BI_PNG;
                   break;
            }
            pds->dsBmih.biSizeImage = psurf->SurfObj.cjBits;
            pds->dsBmih.biXPelsPerMeter = 0;
            pds->dsBmih.biYPelsPerMeter = 0;
            pds->dsBmih.biClrUsed = psurf->biClrUsed;
            pds->dsBmih.biClrImportant = psurf->biClrImportant;
            pds->dsBitfields[0] = psurf->dsBitfields[0];
            pds->dsBitfields[1] = psurf->dsBitfields[1];
            pds->dsBitfields[2] = psurf->dsBitfields[2];
            pds->dshSection = psurf->hDIBSection;
            pds->dsOffset = psurf->dwOffset;

            return sizeof(DIBSECTION);
        }
    }
    else
    {
        /* not set according to wine test, confirmed in win2k */
        pBitmap->bmBits = NULL;
    }

    return sizeof(BITMAP);
}

/*
 * @implemented
 */
HDC
APIENTRY
NtGdiGetDCforBitmap(
    IN HBITMAP hsurf)
{
    HDC hDC = NULL;
    PSURFACE psurf = SURFACE_LockSurface(hsurf);
    if (psurf)
    {
        hDC = psurf->hDC;
        SURFACE_UnlockSurface(psurf);
    }
    return hDC;
}

/* EOF */
