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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

LONG APIENTRY
IntSetBitmapBits(
    PSURFACE psurf,
    DWORD Bytes,
    IN PBYTE Bits)
{
    /* Don't copy more bytes than the buffer has */
    Bytes = min(Bytes, psurf->SurfObj.cjBits);

    RtlCopyMemory(psurf->SurfObj.pvBits, Bits, Bytes);

    return Bytes;
}

void
NTAPI
UnsafeSetBitmapBits(
    PSURFACE psurf,
    IN ULONG cjBits,
    IN PVOID pvBits)
{
    PUCHAR pjDst, pjSrc;
    LONG lDeltaDst, lDeltaSrc;
    ULONG nWidth, nHeight, cBitsPixel;

    nWidth = psurf->SurfObj.sizlBitmap.cx;
    nHeight = psurf->SurfObj.sizlBitmap.cy;
    cBitsPixel = BitsPerFormat(psurf->SurfObj.iBitmapFormat);

    /* Get pointers */
    pjDst = psurf->SurfObj.pvScan0;
    pjSrc = pvBits;
    lDeltaDst = psurf->SurfObj.lDelta;
    lDeltaSrc = WIDTH_BYTES_ALIGN16(nWidth, cBitsPixel);

    while (nHeight--)
    {
        /* Copy one line */
        memcpy(pjDst, pjSrc, lDeltaSrc);
        pjSrc += lDeltaSrc;
        pjDst += lDeltaDst;
    }

}

HBITMAP
APIENTRY
GreCreateBitmapEx(
    IN INT nWidth,
    IN INT nHeight,
    IN ULONG cjWidthBytes,
    IN ULONG iFormat,
    IN USHORT fjBitmap,
    IN ULONG cjSizeImage,
    IN OPTIONAL PVOID pvBits,
	IN FLONG flags)
{
    PSURFACE psurf;
    SURFOBJ *pso;
    HBITMAP hbmp;
    PVOID pvCompressedBits;
    SIZEL sizl;

    /* Verify format */
    if (iFormat < BMF_1BPP || iFormat > BMF_PNG) return NULL;

    /* Allocate a surface */
    psurf = SURFACE_AllocSurface(STYPE_BITMAP, nWidth, nHeight, iFormat);
    if (!psurf)
    {
        DPRINT1("SURFACE_AllocSurface failed.\n");
        return NULL;
    }

    /* Get the handle for the bitmap and the surfobj */
    hbmp = (HBITMAP)psurf->SurfObj.hsurf;
    pso = &psurf->SurfObj;

    /* The infamous RLE hack */
    if (iFormat == BMF_4RLE || iFormat == BMF_8RLE)
    {
        sizl.cx = nWidth;
        sizl.cy = nHeight;
        pvCompressedBits = pvBits;
        pvBits = EngAllocMem(FL_ZERO_MEMORY, pso->cjBits, TAG_DIB);
        if (!pvBits)
        {
            EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
            SURFACE_FreeSurfaceByHandle(hbmp);
            return NULL;
        }
        DecompressBitmap(sizl, pvCompressedBits, pvBits, pso->lDelta, iFormat);
        fjBitmap |= BMF_RLE_HACK;
    }

	/* Mark as API bitmap */
	psurf->flags |= (flags | API_BITMAP);

    /* Set the bitmap bits */
    if (!SURFACE_bSetBitmapBits(psurf, fjBitmap, cjWidthBytes, pvBits))
    {
        /* Bail out if that failed */
        DPRINT1("SURFACE_bSetBitmapBits failed.\n");
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        SURFACE_FreeSurfaceByHandle(hbmp);
        return NULL;
    }

    /* Unlock the surface and return */
    SURFACE_UnlockSurface(psurf);
    return hbmp;
}

/* Creates a DDB surface,
 * as in CreateCompatibleBitmap or CreateBitmap.
 */
HBITMAP
APIENTRY
GreCreateBitmap(
    IN INT nWidth,
    IN INT nHeight,
    IN UINT cPlanes,
    IN UINT cBitsPixel,
    IN OPTIONAL PVOID pvBits)
{
    /* Call the extended function */
    return GreCreateBitmapEx(nWidth,
                             nHeight,
                             0, /* auto width */
                             BitmapFormat(cBitsPixel * cPlanes, BI_RGB),
                             0, /* no bitmap flags */
                             0, /* auto size */
                             pvBits,
							 DDB_SURFACE /* DDB */);
}

HBITMAP
APIENTRY
NtGdiCreateBitmap(
    IN INT nWidth,
    IN INT nHeight,
    IN UINT cPlanes,
    IN UINT cBitsPixel,
    IN OPTIONAL LPBYTE pUnsafeBits)
{
    HBITMAP hbmp;
    ULONG cRealBpp, cjWidthBytes, iFormat;
    ULONGLONG cjSize;

    /* Calculate bitmap format and real bits per pixel. */
    iFormat = BitmapFormat(cBitsPixel * cPlanes, BI_RGB);
    cRealBpp = gajBitsPerFormat[iFormat];

    /* Calculate width and image size in bytes */
    cjWidthBytes = WIDTH_BYTES_ALIGN16(nWidth, cRealBpp);
    cjSize = cjWidthBytes * nHeight;

    /* Check parameters (possible overflow of cjWidthBytes!) */
    if (iFormat == 0 || nWidth <= 0 || nWidth >= 0x8000000 || nHeight <= 0 ||
        cBitsPixel > 32 || cPlanes > 32 || cjSize >= 0x100000000ULL)
    {
        DPRINT1("Invalid bitmap format! Width=%d, Height=%d, Bpp=%d, Planes=%d\n",
                nWidth, nHeight, cBitsPixel, cPlanes);
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    /* Call internal function. */
    hbmp = GreCreateBitmapEx(nWidth, nHeight, 0, iFormat, 0, 0, NULL, DDB_SURFACE);

    if (pUnsafeBits && hbmp)
    {
        PSURFACE psurf = SURFACE_LockSurface(hbmp);
        _SEH2_TRY
        {
            ProbeForRead(pUnsafeBits, cjSize, 1);
            UnsafeSetBitmapBits(psurf, 0, pUnsafeBits);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            SURFACE_UnlockSurface(psurf);
            SURFACE_FreeSurfaceByHandle(hbmp);
            _SEH2_YIELD(return NULL;)
        }
        _SEH2_END

        SURFACE_UnlockSurface(psurf);
    }

    return hbmp;
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
            PSURFACE psurf;

            Bmp = GreCreateBitmap(abs(Width),
                                  abs(Height),
                                  1,
                                  Dc->ppdev->gdiinfo.cBitsPixel,
                                  NULL);
            psurf = SURFACE_LockSurface(Bmp);
            ASSERT(psurf);
            /* Set palette */
            psurf->ppal = PALETTE_ShareLockPalette(Dc->ppdev->devinfo.hpalDefault);
            /* Set flags */
            psurf->flags = API_BITMAP;
            psurf->hdc = NULL; // Fixme
            SURFACE_UnlockSurface(psurf);
        }
        else
        {
            DIBSECTION dibs;
            INT Count;
            PSURFACE psurf = Dc->dclevel.pSurface;
            Count = BITMAP_GetObject(psurf, sizeof(dibs), &dibs);

            if (Count)
            {
                if (Count == sizeof(BITMAP))
                {
                    PSURFACE psurfBmp;

                    Bmp = GreCreateBitmap(abs(Width),
                                  abs(Height),
                                  1,
                                  dibs.dsBm.bmBitsPixel,
                                  NULL);
                    psurfBmp = SURFACE_LockSurface(Bmp);
                    ASSERT(psurfBmp);
                    /* Assign palette */
                    psurfBmp->ppal = psurf->ppal;
                    GDIOBJ_IncrementShareCount((POBJ)psurf->ppal);
                    /* Set flags */
                    psurfBmp->flags = API_BITMAP;
                    psurfBmp->hdc = NULL; // Fixme
                    SURFACE_UnlockSurface(psurfBmp);
                }
                else
                {
                    /* A DIB section is selected in the DC */
					BYTE buf[sizeof(BITMAPINFOHEADER) + 256*sizeof(RGBQUAD)] = {0};
                    PVOID Bits;
					BITMAPINFO* bi = (BITMAPINFO*)buf;

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
                        RtlCopyMemory(bi->bmiColors, dibs.dsBitfields, 3*sizeof(RGBQUAD));
                    }
                    else if (bi->bmiHeader.biBitCount <= 8)
                    {
                        /* Copy the color table */
                        UINT Index;
                        PPALETTE PalGDI;

                        if (!psurf->ppal)
                        {
                            EngSetLastError(ERROR_INVALID_HANDLE);
                            return 0;
                        }

                        PalGDI = PALETTE_LockPalette(psurf->ppal->BaseObject.hHmgr);

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

                        Bmp = DIB_CreateDIBSection(Dc,
                                                   bi,
                                                   DIB_RGB_COLORS,
                                                   &Bits,
                                                   NULL,
                                                   0,
                                                   0);
                        return Bmp;
                    }
                }
            }
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
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    if (!hDC)
        return GreCreateBitmap(Width, Height, 1, 1, 0);

    Dc = DC_LockDc(hDC);

    DPRINT("NtGdiCreateCompatibleBitmap(%04x,%d,%d, bpp:%d) = \n",
           hDC, Width, Height, Dc->ppdev->gdiinfo.cBitsPixel);

    if (NULL == Dc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
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
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    _SEH2_TRY
    {
        ProbeForWrite(Dimension, sizeof(SIZE), 1);
        *Dimension = psurfBmp->sizlDim;
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
    EXLATEOBJ exlo;
    HBITMAP hBmpTmp;

    dc = DC_LockDc(hDC);

    if (!dc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return Result;
    }

    if (dc->dctype == DC_TYPE_INFO)
    {
        DC_UnlockDc(dc);
        return Result;
    }

    XPos += dc->ptlDCOrig.x;
    YPos += dc->ptlDCOrig.y;
    if ((dc->rosdc.CombinedClip == NULL) ||
        (RECTL_bPointInRect(&dc->rosdc.CombinedClip->rclBounds, XPos, YPos)))
    {
        bInRect = TRUE;
        psurf = dc->dclevel.pSurface;
        if (psurf)
        {
			pso = &psurf->SurfObj;
            EXLATEOBJ_vInitialize(&exlo, psurf->ppal, &gpalRGB, 0, 0xffffff, 0);
            // check if this DC has a DIB behind it...
            if (pso->pvScan0) // STYPE_BITMAP == pso->iType
            {
                ASSERT(pso->lDelta);
                Result = XLATEOBJ_iXlate(&exlo.xlo,
                                         DibFunctionsForBitmapFormat[pso->iBitmapFormat].DIB_GetPixel(pso, XPos, YPos));
            }

            EXLATEOBJ_vCleanup(&exlo);
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

            //HBITMAP hBmpTmp = GreCreateBitmap(1, 1, 1, 32, NULL);
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

VOID
FASTCALL
UnsafeGetBitmapBits(
    PSURFACE psurf,
	DWORD Bytes,
	OUT PBYTE pvBits)
{
	PUCHAR pjDst, pjSrc;
    LONG lDeltaDst, lDeltaSrc;
    ULONG nWidth, nHeight, cBitsPixel;

    nWidth = psurf->SurfObj.sizlBitmap.cx;
    nHeight = psurf->SurfObj.sizlBitmap.cy;
    cBitsPixel = BitsPerFormat(psurf->SurfObj.iBitmapFormat);

    /* Get pointers */
    pjSrc = psurf->SurfObj.pvScan0;
    pjDst = pvBits;
    lDeltaSrc = psurf->SurfObj.lDelta;
    lDeltaDst = WIDTH_BYTES_ALIGN16(nWidth, cBitsPixel);

    while (nHeight--)
    {
        /* Copy one line */
        RtlCopyMemory(pjDst, pjSrc, lDeltaDst);
        pjSrc += lDeltaSrc;
        pjDst += lDeltaDst;
    }
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
        EngSetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    bmSize = WIDTH_BYTES_ALIGN16(psurf->SurfObj.sizlBitmap.cx,
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
        UnsafeGetBitmapBits(psurf, Bytes, pUnsafeBits);
		ret = Bytes;
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
        EngSetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    _SEH2_TRY
    {
        ProbeForRead(pUnsafeBits, Bytes, 1);
        UnsafeSetBitmapBits(psurf, Bytes, pUnsafeBits);
        ret = 1;
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
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (Size)
    {
        _SEH2_TRY
        {
            ProbeForWrite(Size, sizeof(SIZE), 1);
            *Size = psurf->sizlDim;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Ret = FALSE;
        }
        _SEH2_END
    }

    /* The dimension is changed even if writing the old value failed */
    psurf->sizlDim.cx = Width;
    psurf->sizlDim.cy = Height;

    SURFACE_UnlockSurface(psurf);

    return Ret;
}

VOID IntHandleSpecialColorType(HDC hDC, COLORREF* Color)
{
    PDC pdc = NULL;
    RGBQUAD quad;
    PALETTEENTRY palEntry;
    UINT index;

    switch (*Color >> 24)
    {
        case 0x10: /* DIBINDEX */
            if (IntGetDIBColorTable(hDC, LOWORD(*Color), 1, &quad) == 1)
            {
                *Color = RGB(quad.rgbRed, quad.rgbGreen, quad.rgbBlue);
            }
            else
            {
                /* Out of color table bounds - use black */
                *Color = RGB(0, 0, 0);
            }
            break;
        case 0x02: /* PALETTERGB */
            pdc = DC_LockDc(hDC);
            if (pdc->dclevel.hpal != NtGdiGetStockObject(DEFAULT_PALETTE))
            {
                index = NtGdiGetNearestPaletteIndex(pdc->dclevel.hpal, *Color);
                IntGetPaletteEntries(pdc->dclevel.hpal, index, 1, &palEntry);
                *Color = RGB(palEntry.peRed, palEntry.peGreen, palEntry.peBlue);
            }
            else
            {
                /* Use the pure color */
                *Color = *Color & 0x00FFFFFF;
            }
            DC_UnlockDc(pdc);
            break;
        case 0x01: /* PALETTEINDEX */
            pdc = DC_LockDc(hDC);
            if (IntGetPaletteEntries(pdc->dclevel.hpal, LOWORD(*Color), 1, &palEntry) == 1)
            {
                *Color = RGB(palEntry.peRed, palEntry.peGreen, palEntry.peBlue);
            }
            else
            {
                /* Index does not exist, use zero index */
                IntGetPaletteEntries(pdc->dclevel.hpal, 0, 1, &palEntry);
                *Color = RGB(palEntry.peRed, palEntry.peGreen, palEntry.peBlue);
            }
            DC_UnlockDc(pdc);
            break;
        default:
            DPRINT("Unsupported color type %d passed\n", *Color >> 24);
            break;
    }
}

BOOL APIENTRY
GdiSetPixelV(
    HDC hDC,
    INT X,
    INT Y,
    COLORREF Color)
{
    HBRUSH hBrush;
    HGDIOBJ OldBrush;

    if ((Color & 0xFF000000) != 0)
    {
        IntHandleSpecialColorType(hDC, &Color);
    }

    hBrush = NtGdiCreateSolidBrush(Color, NULL);
    if (hBrush == NULL)
        return FALSE;

    OldBrush = NtGdiSelectBrush(hDC, hBrush);
    if (OldBrush == NULL)
    {
        GreDeleteObject(hBrush);
        return FALSE;
    }

    NtGdiPatBlt(hDC, X, Y, 1, 1, PATCOPY);
    NtGdiSelectBrush(hDC, OldBrush);
    GreDeleteObject(hBrush);

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

    Bitmap = SURFACE_LockSurface(hBitmap);
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
    res = GreCreateBitmapEx(Size.cx,
							Size.cy,
							bm.bmWidthBytes,
							Bitmap->SurfObj.iBitmapFormat,
							Bitmap->SurfObj.fjBitmap,
							Bitmap->SurfObj.cjBits,
							NULL,
							Bitmap->flags);


    if (res)
    {
        resBitmap = SURFACE_LockSurface(res);
        if (resBitmap)
        {
            IntSetBitmapBits(resBitmap, Bitmap->SurfObj.cjBits, Bitmap->SurfObj.pvBits);
			SURFACE_UnlockSurface(resBitmap);
        }
        else
        {
            GreDeleteObject(res);
            res = NULL;
        }
    }

    SURFACE_UnlockSurface(Bitmap);

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
    pBitmap->bmPlanes = 1;
    pBitmap->bmBitsPixel = BitsPerFormat(psurf->SurfObj.iBitmapFormat);
	pBitmap->bmWidthBytes = WIDTH_BYTES_ALIGN16(pBitmap->bmWidth, pBitmap->bmBitsPixel);

    /* Check for DIB section */
    if (psurf->hSecure)
    {
        /* Set bmBits in this case */
        pBitmap->bmBits = psurf->SurfObj.pvBits;
		/* DIBs data are 32 bits aligned */
		pBitmap->bmWidthBytes = WIDTH_BYTES_ALIGN32(pBitmap->bmWidth, pBitmap->bmBitsPixel);

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
                case BMF_1BPP:
                case BMF_4BPP:
                case BMF_8BPP:
                case BMF_24BPP:
                   pds->dsBmih.biCompression = BI_RGB;
                   break;

                case BMF_16BPP:
                    if (psurf->ppal->flFlags & PAL_RGB16_555)
                        pds->dsBmih.biCompression = BI_RGB;
                    else
                        pds->dsBmih.biCompression = BI_BITFIELDS;
                    break;

                case BMF_32BPP:
                    if (psurf->ppal->flFlags & PAL_RGB)
                        pds->dsBmih.biCompression = BI_RGB;
                    else
                        pds->dsBmih.biCompression = BI_BITFIELDS;
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
                default:
                    ASSERT(FALSE); /* this shouldn't happen */
            }

            pds->dsBmih.biSizeImage = psurf->SurfObj.cjBits;
            pds->dsBmih.biXPelsPerMeter = 0;
            pds->dsBmih.biYPelsPerMeter = 0;
            pds->dsBmih.biClrUsed = psurf->ppal->NumColors;
            pds->dsBmih.biClrImportant = psurf->biClrImportant;
            pds->dsBitfields[0] = psurf->ppal->RedMask;
            pds->dsBitfields[1] = psurf->ppal->GreenMask;
            pds->dsBitfields[2] = psurf->ppal->BlueMask;
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
    HDC hdc = NULL;
    PSURFACE psurf = SURFACE_LockSurface(hsurf);
    if (psurf)
    {
        hdc = psurf->hdc;
        SURFACE_UnlockSurface(psurf);
    }
    return hdc;
}


/* EOF */
