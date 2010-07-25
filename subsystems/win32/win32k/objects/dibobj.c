/*
 * ReactOS W32 Subsystem
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

static const RGBQUAD EGAColorsQuads[16] = {
/* rgbBlue, rgbGreen, rgbRed, rgbReserved */
    { 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x80, 0x00 },
    { 0x00, 0x80, 0x00, 0x00 },
    { 0x00, 0x80, 0x80, 0x00 },
    { 0x80, 0x00, 0x00, 0x00 },
    { 0x80, 0x00, 0x80, 0x00 },
    { 0x80, 0x80, 0x00, 0x00 },
    { 0x80, 0x80, 0x80, 0x00 },
    { 0xc0, 0xc0, 0xc0, 0x00 },
    { 0x00, 0x00, 0xff, 0x00 },
    { 0x00, 0xff, 0x00, 0x00 },
    { 0x00, 0xff, 0xff, 0x00 },
    { 0xff, 0x00, 0x00, 0x00 },
    { 0xff, 0x00, 0xff, 0x00 },
    { 0xff, 0xff, 0x00, 0x00 },
    { 0xff, 0xff, 0xff, 0x00 }
};

static const RGBQUAD DefLogPaletteQuads[20] = { /* Copy of Default Logical Palette */
/* rgbBlue, rgbGreen, rgbRed, rgbReserved */
    { 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x80, 0x00 },
    { 0x00, 0x80, 0x00, 0x00 },
    { 0x00, 0x80, 0x80, 0x00 },
    { 0x80, 0x00, 0x00, 0x00 },
    { 0x80, 0x00, 0x80, 0x00 },
    { 0x80, 0x80, 0x00, 0x00 },
    { 0xc0, 0xc0, 0xc0, 0x00 },
    { 0xc0, 0xdc, 0xc0, 0x00 },
    { 0xf0, 0xca, 0xa6, 0x00 },
    { 0xf0, 0xfb, 0xff, 0x00 },
    { 0xa4, 0xa0, 0xa0, 0x00 },
    { 0x80, 0x80, 0x80, 0x00 },
    { 0x00, 0x00, 0xf0, 0x00 },
    { 0x00, 0xff, 0x00, 0x00 },
    { 0x00, 0xff, 0xff, 0x00 },
    { 0xff, 0x00, 0x00, 0x00 },
    { 0xff, 0x00, 0xff, 0x00 },
    { 0xff, 0xff, 0x00, 0x00 },
    { 0xff, 0xff, 0xff, 0x00 }
};


UINT
APIENTRY
IntSetDIBColorTable(
    HDC hDC,
    UINT StartIndex,
    UINT Entries,
    CONST RGBQUAD *Colors)
{
    PDC dc;
    PSURFACE psurf;
    PPALETTE PalGDI;
    UINT Index;
    ULONG biBitCount;

    if (!(dc = DC_LockDc(hDC))) return 0;
    if (dc->dctype == DC_TYPE_INFO)
    {
        DC_UnlockDc(dc);
        return 0;
    }

    psurf = dc->dclevel.pSurface;
    if (psurf == NULL)
    {
        DC_UnlockDc(dc);
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (psurf->hSecure == NULL)
    {
        DC_UnlockDc(dc);
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return 0;
    }

    biBitCount = BitsPerFormat(psurf->SurfObj.iBitmapFormat);
    if (biBitCount <= 8 && StartIndex < (1 << biBitCount))
    {
        if (StartIndex + Entries > (1 << biBitCount))
            Entries = (1 << biBitCount) - StartIndex;

        if (psurf->ppal == NULL)
        {
            DC_UnlockDc(dc);
            SetLastWin32Error(ERROR_INVALID_HANDLE);
            return 0;
        }

        PalGDI = PALETTE_LockPalette(psurf->ppal->BaseObject.hHmgr);

        for (Index = StartIndex;
             Index < StartIndex + Entries && Index < PalGDI->NumColors;
             Index++)
        {
            PalGDI->IndexedColors[Index].peRed = Colors[Index - StartIndex].rgbRed;
            PalGDI->IndexedColors[Index].peGreen = Colors[Index - StartIndex].rgbGreen;
            PalGDI->IndexedColors[Index].peBlue = Colors[Index - StartIndex].rgbBlue;
        }
        PALETTE_UnlockPalette(PalGDI);
    }
    else
        Entries = 0;

    /* Mark the brushes invalid */
    dc->pdcattr->ulDirty_ |= DIRTY_FILL|DIRTY_LINE|DIRTY_BACKGROUND|DIRTY_TEXT;

    DC_UnlockDc(dc);

    return Entries;
}

UINT
APIENTRY
IntGetDIBColorTable(
    HDC hDC,
    UINT StartIndex,
    UINT Entries,
    RGBQUAD *Colors)
{
    PDC dc;
    PSURFACE psurf;
    PPALETTE PalGDI;
    UINT Index, Count = 0;
    ULONG biBitCount;

    if (!(dc = DC_LockDc(hDC))) return 0;
    if (dc->dctype == DC_TYPE_INFO)
    {
        DC_UnlockDc(dc);
        return 0;
    }

    psurf = dc->dclevel.pSurface;
    if (psurf == NULL)
    {
        DC_UnlockDc(dc);
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (psurf->hSecure == NULL)
    {
        DC_UnlockDc(dc);
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return 0;
    }

    biBitCount = BitsPerFormat(psurf->SurfObj.iBitmapFormat);
    if (biBitCount <= 8 &&
            StartIndex < (1 << biBitCount))
    {
        if (StartIndex + Entries > (1 << biBitCount))
            Entries = (1 << biBitCount) - StartIndex;

        if (psurf->ppal == NULL)
        {
            DC_UnlockDc(dc);
            SetLastWin32Error(ERROR_INVALID_HANDLE);
            return 0;
        }

        PalGDI = PALETTE_LockPalette(psurf->ppal->BaseObject.hHmgr);

        for (Index = StartIndex;
             Index < StartIndex + Entries && Index < PalGDI->NumColors;
             Index++)
        {
            Colors[Index - StartIndex].rgbRed = PalGDI->IndexedColors[Index].peRed;
            Colors[Index - StartIndex].rgbGreen = PalGDI->IndexedColors[Index].peGreen;
            Colors[Index - StartIndex].rgbBlue = PalGDI->IndexedColors[Index].peBlue;
            Colors[Index - StartIndex].rgbReserved = 0;
            Count++;
        }
        PALETTE_UnlockPalette(PalGDI);
    }

    DC_UnlockDc(dc);

    return Count;
}

// Converts a DIB to a device-dependent bitmap
static INT
FASTCALL
IntSetDIBits(
    PDC   DC,
    HBITMAP  hBitmap,
    UINT  StartScan,
    UINT  ScanLines,
    CONST VOID  *Bits,
    CONST BITMAPV5INFO  *bmi,
    UINT  ColorUse)
{
    SURFACE     *bitmap;
    HBITMAP     SourceBitmap;
    INT         result = 0;
    BOOL        copyBitsResult;
    SURFOBJ     *DestSurf, *SourceSurf;
    SIZEL       SourceSize;
    POINTL      ZeroPoint;
    RECTL       DestRect;
    EXLATEOBJ   exlo;
    PPALETTE    ppalDIB;
    //RGBQUAD    *lpRGB;
    HPALETTE    DIB_Palette;
    ULONG       DIB_Palette_Type;
    INT         DIBWidth;

    // Check parameters
    if (!(bitmap = SURFACE_LockSurface(hBitmap)))
    {
        return 0;
    }

    // Get RGB values
    //if (ColorUse == DIB_PAL_COLORS)
    //  lpRGB = DIB_MapPaletteColors(hDC, bmi);
    //else
    //  lpRGB = &bmi->bmiColors;

    DestSurf = &bitmap->SurfObj;

    // Create source surface
    SourceSize.cx = bmi->bmiHeader.bV5Width;
    SourceSize.cy = ScanLines;

    // Determine width of DIB
    DIBWidth = DIB_GetDIBWidthBytes(SourceSize.cx, bmi->bmiHeader.bV5BitCount);

    SourceBitmap = EngCreateBitmap(SourceSize,
                                   DIBWidth,
                                   BitmapFormat(bmi->bmiHeader.bV5BitCount, bmi->bmiHeader.bV5Compression),
                                   bmi->bmiHeader.bV5Height < 0 ? BMF_TOPDOWN : 0,
                                   (PVOID) Bits);
    if (0 == SourceBitmap)
    {
        SURFACE_UnlockSurface(bitmap);
        SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
        return 0;
    }

    SourceSurf = EngLockSurface((HSURF)SourceBitmap);
    if (NULL == SourceSurf)
    {
        EngDeleteSurface((HSURF)SourceBitmap);
        SURFACE_UnlockSurface(bitmap);
        SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
        return 0;
    }

    ASSERT(bitmap->ppal);

    // Source palette obtained from the BITMAPINFO
    DIB_Palette = BuildDIBPalette((BITMAPINFO*)bmi, (PINT)&DIB_Palette_Type);
    if (NULL == DIB_Palette)
    {
        EngUnlockSurface(SourceSurf);
        EngDeleteSurface((HSURF)SourceBitmap);
        SURFACE_UnlockSurface(bitmap);
        SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
        return 0;
    }

    ppalDIB = PALETTE_LockPalette(DIB_Palette);

    /* Initialize XLATEOBJ for color translation */
    EXLATEOBJ_vInitialize(&exlo, ppalDIB, bitmap->ppal, 0, 0, 0);

    // Zero point
    ZeroPoint.x = 0;
    ZeroPoint.y = 0;

    // Determine destination rectangle
    DestRect.left	= 0;
    DestRect.top	= abs(bmi->bmiHeader.bV5Height) - StartScan - ScanLines;
    DestRect.right	= SourceSize.cx;
    DestRect.bottom	= DestRect.top + ScanLines;

    copyBitsResult = IntEngCopyBits(DestSurf, SourceSurf, NULL, &exlo.xlo, &DestRect, &ZeroPoint);

    // If it succeeded, return number of scanlines copies
    if (copyBitsResult == TRUE)
    {
        result = SourceSize.cy;
// or
//        result = abs(bmi->bmiHeader.biHeight) - StartScan;
    }

    // Clean up
    EXLATEOBJ_vCleanup(&exlo);
    PALETTE_UnlockPalette(ppalDIB);
    PALETTE_FreePaletteByHandle(DIB_Palette);
    EngUnlockSurface(SourceSurf);
    EngDeleteSurface((HSURF)SourceBitmap);

//    if (ColorUse == DIB_PAL_COLORS)
//        WinFree((LPSTR)lpRGB);

    SURFACE_UnlockSurface(bitmap);

    return result;
}

// FIXME by Removing NtGdiSetDIBits!!!
// This is a victim of the Win32k Initialization BUG!!!!!
// Converts a DIB to a device-dependent bitmap
INT
APIENTRY
NtGdiSetDIBits(
    HDC  hDC,
    HBITMAP  hBitmap,
    UINT  StartScan,
    UINT  ScanLines,
    CONST VOID  *Bits,
    CONST BITMAPINFO  *bmi,
    UINT  ColorUse)
{
    PDC Dc;
    INT Ret;
    NTSTATUS Status = STATUS_SUCCESS;
    BITMAPV5INFO bmiLocal;

    if (!Bits) return 0;

    _SEH2_TRY
    {
        Status = ProbeAndConvertToBitmapV5Info(&bmiLocal, bmi, ColorUse, 0);
        ProbeForRead(Bits, bmiLocal.bmiHeader.bV5SizeImage, 1);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END

    if (!NT_SUCCESS(Status))
    {
        return 0;
    }

    Dc = DC_LockDc(hDC);
    if (NULL == Dc)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return 0;
    }
    if (Dc->dctype == DC_TYPE_INFO)
    {
        DC_UnlockDc(Dc);
        return 0;
    }

    Ret = IntSetDIBits(Dc, hBitmap, StartScan, ScanLines, Bits, &bmiLocal, ColorUse);

    DC_UnlockDc(Dc);

    return Ret;
}

W32KAPI
INT
APIENTRY
NtGdiSetDIBitsToDeviceInternal(
    IN HDC hDC,
    IN INT XDest,
    IN INT YDest,
    IN DWORD Width,
    IN DWORD Height,
    IN INT XSrc,
    IN INT YSrc,
    IN DWORD StartScan,
    IN DWORD ScanLines,
    IN LPBYTE Bits,
    IN LPBITMAPINFO bmi,
    IN DWORD ColorUse,
    IN UINT cjMaxBits,
    IN UINT cjMaxInfo,
    IN BOOL bTransformCoordinates,
    IN OPTIONAL HANDLE hcmXform)
{
    INT ret = 0;
    NTSTATUS Status = STATUS_SUCCESS;
    PDC pDC;
    HBITMAP hSourceBitmap = NULL;
    SURFOBJ *pDestSurf, *pSourceSurf = NULL;
    SURFACE *pSurf;
    RECTL rcDest;
    POINTL ptSource;
    INT DIBWidth;
    SIZEL SourceSize;
    EXLATEOBJ exlo;
    PPALETTE ppalDIB = NULL;
    HPALETTE hpalDIB = NULL;
    ULONG DIBPaletteType;
    BITMAPV5INFO bmiLocal ;

    if (!Bits) return 0;

    _SEH2_TRY
    {
        Status = ProbeAndConvertToBitmapV5Info(&bmiLocal, bmi, ColorUse, cjMaxInfo);
        ProbeForRead(Bits, cjMaxBits, 1);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END

    if (!NT_SUCCESS(Status))
    {
        return 0;
    }

    pDC = DC_LockDc(hDC);
    if (!pDC)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return 0;
    }
    if (pDC->dctype == DC_TYPE_INFO)
    {
        DC_UnlockDc(pDC);
        return 0;
    }

    pSurf = pDC->dclevel.pSurface;

    pDestSurf = pSurf ? &pSurf->SurfObj : NULL;

    ScanLines = min(ScanLines, abs(bmiLocal.bmiHeader.bV5Height) - StartScan);

    rcDest.left = XDest;
    rcDest.top = YDest;
    if (bTransformCoordinates)
    {
        CoordLPtoDP(pDC, (LPPOINT)&rcDest);
    }
    rcDest.left += pDC->ptlDCOrig.x;
    rcDest.top += pDC->ptlDCOrig.y;
    rcDest.right = rcDest.left + Width;
    rcDest.bottom = rcDest.top + Height;
    rcDest.top += StartScan;

    ptSource.x = XSrc;
    ptSource.y = YSrc;

    SourceSize.cx = bmiLocal.bmiHeader.bV5Width;
    SourceSize.cy = ScanLines;

    DIBWidth = DIB_GetDIBWidthBytes(SourceSize.cx, bmiLocal.bmiHeader.bV5BitCount);

    hSourceBitmap = EngCreateBitmap(SourceSize,
                                    DIBWidth,
                                    BitmapFormat(bmiLocal.bmiHeader.bV5BitCount,
                                                 bmiLocal.bmiHeader.bV5Compression),
                                    bmiLocal.bmiHeader.bV5Height < 0 ? BMF_TOPDOWN : 0,
                                    (PVOID) Bits);
    if (!hSourceBitmap)
    {
        SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
        Status = STATUS_NO_MEMORY;
        goto Exit;
    }

    pSourceSurf = EngLockSurface((HSURF)hSourceBitmap);
    if (!pSourceSurf)
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    ASSERT(pSurf->ppal);

    /* Create a palette for the DIB */
    hpalDIB = BuildDIBPalette((PBITMAPINFO)&bmiLocal, (PINT)&DIBPaletteType);
    if (!hpalDIB)
    {
        SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
        Status = STATUS_NO_MEMORY;
        goto Exit;
    }

    /* Lock the DIB palette */
    ppalDIB = PALETTE_LockPalette(hpalDIB);
    if (!ppalDIB)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    /* Initialize EXLATEOBJ */
    EXLATEOBJ_vInitialize(&exlo, ppalDIB, pSurf->ppal, 0, 0, 0);

    /* Copy the bits */
    DPRINT("BitsToDev with dstsurf=(%d|%d) (%d|%d), src=(%d|%d) w=%d h=%d\n",
        rcDest.left, rcDest.top, rcDest.right, rcDest.bottom,
        ptSource.x, ptSource.y, SourceSize.cx, SourceSize.cy);
    Status = IntEngBitBlt(pDestSurf,
                          pSourceSurf,
                          NULL,
                          pDC->rosdc.CombinedClip,
                          &exlo.xlo,
                          &rcDest,
                          &ptSource,
                          NULL,
                          NULL,
                          NULL,
                          ROP3_TO_ROP4(SRCCOPY));

    /* Cleanup EXLATEOBJ */
    EXLATEOBJ_vCleanup(&exlo);

Exit:
    if (NT_SUCCESS(Status))
    {
        ret = ScanLines;
    }

    if (ppalDIB) PALETTE_UnlockPalette(ppalDIB);

    if (pSourceSurf) EngUnlockSurface(pSourceSurf);
    if (hSourceBitmap) EngDeleteSurface((HSURF)hSourceBitmap);
    if (hpalDIB) PALETTE_FreePaletteByHandle(hpalDIB);
    DC_UnlockDc(pDC);

    return ret;
}


/* Converts a device-dependent bitmap to a DIB */
INT
APIENTRY
NtGdiGetDIBitsInternal(
    HDC hDC,
    HBITMAP hBitmap,
    UINT StartScan,
    UINT ScanLines,
    LPBYTE Bits,
    LPBITMAPINFO Info,
    UINT Usage,
    UINT MaxBits,
    UINT MaxInfo)
{
    PDC Dc;
    SURFACE *psurf = NULL;
    HBITMAP hDestBitmap = NULL;
    HPALETTE hDestPalette = NULL;
    BITMAPV5INFO bmiLocal ;
    PPALETTE ppalDst = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Result = 0;
    BOOL bPaletteMatch = FALSE;
    PBYTE ChkBits = Bits;
    ULONG DestPaletteType;
    ULONG Index;

    DPRINT("Entered NtGdiGetDIBitsInternal()\n");

    if ((Usage && Usage != DIB_PAL_COLORS) || !Info || !hBitmap)
        return 0;

    // if ScanLines == 0, no need to copy Bits.
    if (!ScanLines)
        ChkBits = NULL;

    _SEH2_TRY
    {
        Status = ProbeAndConvertToBitmapV5Info(&bmiLocal, Info, Usage, MaxInfo);
        if (ChkBits) ProbeForWrite(ChkBits, MaxBits, 1);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END

    if (!NT_SUCCESS(Status))
    {
        return 0;
    }

    Dc = DC_LockDc(hDC);
    if (Dc == NULL) return 0;
    if (Dc->dctype == DC_TYPE_INFO)
    {
        DC_UnlockDc(Dc);
        return 0;
    }

    /* Get a pointer to the source bitmap object */
    psurf = SURFACE_LockSurface(hBitmap);
    if (psurf == NULL)
    {
        DC_UnlockDc(Dc);
        return 0;
    }

    ASSERT(psurf->ppal);

    DC_UnlockDc(Dc);

    /* Copy palette information
     * Always create a palette for 15 & 16 bit. */
    if ((bmiLocal.bmiHeader.bV5BitCount == BitsPerFormat(psurf->SurfObj.iBitmapFormat) &&
         bmiLocal.bmiHeader.bV5BitCount != 15 && bmiLocal.bmiHeader.bV5BitCount != 16) ||
         !ChkBits)
    {
        ppalDst = psurf->ppal;
        bPaletteMatch = TRUE;
    }
    else
    {
        hDestPalette = BuildDIBPalette((PBITMAPINFO)&bmiLocal, (PINT)&DestPaletteType);
        ppalDst = PALETTE_LockPalette(hDestPalette);
    }

    DPRINT("ppalDst : %p\n", ppalDst);
    ASSERT(ppalDst);

    /* Copy palette. */
    switch (bmiLocal.bmiHeader.bV5BitCount)
    {
        case 1:
        case 4:
        case 8:
            bmiLocal.bmiHeader.bV5ClrUsed = 0;
            if (psurf->hSecure &&
                BitsPerFormat(psurf->SurfObj.iBitmapFormat) == bmiLocal.bmiHeader.bV5BitCount)
            {
                if (Usage == DIB_RGB_COLORS)
                {
                    if (ppalDst->NumColors != 1 << bmiLocal.bmiHeader.bV5BitCount)
                        bmiLocal.bmiHeader.bV5ClrUsed = ppalDst->NumColors;
                    for (Index = 0;
                         Index < (1 << bmiLocal.bmiHeader.bV5BitCount) && Index < ppalDst->NumColors;
                         Index++)
                    {
                        bmiLocal.bmiColors[Index].rgbRed   = ppalDst->IndexedColors[Index].peRed;
                        bmiLocal.bmiColors[Index].rgbGreen = ppalDst->IndexedColors[Index].peGreen;
                        bmiLocal.bmiColors[Index].rgbBlue  = ppalDst->IndexedColors[Index].peBlue;
                        bmiLocal.bmiColors[Index].rgbReserved = 0;
                    }
                }
                else
                {
                    for (Index = 0;
                         Index < (1 << bmiLocal.bmiHeader.bV5BitCount);
                         Index++)
                    {
                        ((WORD*)bmiLocal.bmiColors)[Index] = Index;
                    }
                }
            }
            else
            {
                if (Usage == DIB_PAL_COLORS)
                {
                    for (Index = 0;
                         Index < (1 << bmiLocal.bmiHeader.bV5BitCount);
                         Index++)
                    {
                        ((WORD*)bmiLocal.bmiColors)[Index] = (WORD)Index;
                    }
                }
                else if (bmiLocal.bmiHeader.bV5BitCount > 1  && bPaletteMatch)
                {
                    for (Index = 0;
                         Index < (1 << bmiLocal.bmiHeader.bV5BitCount) && Index < ppalDst->NumColors;
                         Index++)
                    {
                        bmiLocal.bmiColors[Index].rgbRed   = ppalDst->IndexedColors[Index].peRed;
                        bmiLocal.bmiColors[Index].rgbGreen = ppalDst->IndexedColors[Index].peGreen;
                        bmiLocal.bmiColors[Index].rgbBlue  = ppalDst->IndexedColors[Index].peBlue;
                        bmiLocal.bmiColors[Index].rgbReserved = 0;
                    }
                }
                else
                {
                    switch (bmiLocal.bmiHeader.bV5BitCount)
                    {
                        case 1:
                            bmiLocal.bmiColors[0].rgbRed =0 ;
                            bmiLocal.bmiColors[0].rgbGreen = 0;
                            bmiLocal.bmiColors[0].rgbBlue = 0;
                            bmiLocal.bmiColors[0].rgbReserved = 0;
                            bmiLocal.bmiColors[1].rgbRed =0xFF ;
                            bmiLocal.bmiColors[1].rgbGreen = 0xFF;
                            bmiLocal.bmiColors[1].rgbBlue = 0xFF;
                            bmiLocal.bmiColors[1].rgbReserved = 0;
                            break;
                        case 4:
                            RtlCopyMemory(bmiLocal.bmiColors, EGAColorsQuads, sizeof(EGAColorsQuads));
                            break;
                        case 8:
                        {
                            INT r, g, b;
                            RGBQUAD *color;

                            RtlCopyMemory(bmiLocal.bmiColors, DefLogPaletteQuads, 10 * sizeof(RGBQUAD));
                            RtlCopyMemory(bmiLocal.bmiColors + 246, DefLogPaletteQuads + 10, 10 * sizeof(RGBQUAD));
                            color = bmiLocal.bmiColors + 10;
                            for (r = 0; r <= 5; r++) /* FIXME */
                                for (g = 0; g <= 5; g++)
                                    for (b = 0; b <= 5; b++)
                                    {
                                        color->rgbRed = (r * 0xff) / 5;
                                        color->rgbGreen = (g * 0xff) / 5;
                                        color->rgbBlue = (b * 0xff) / 5;
                                        color->rgbReserved = 0;
                                        color++;
                                    }
                        }
                        break;
                    }
                }
            }

        case 15:
            if (bmiLocal.bmiHeader.bV5Compression == BI_BITFIELDS)
            {
                bmiLocal.bmiHeader.bV5RedMask = 0x7c00;
                bmiLocal.bmiHeader.bV5GreenMask = 0x03e0;
                bmiLocal.bmiHeader.bV5BlueMask = 0x001f;
            }
            break;

        case 16:
            if (bmiLocal.bmiHeader.bV5Compression == BI_BITFIELDS)
            {
                bmiLocal.bmiHeader.bV5RedMask = 0xf800;
                bmiLocal.bmiHeader.bV5GreenMask = 0x07e0;
                bmiLocal.bmiHeader.bV5BlueMask = 0x001f;
            }
            break;

        case 24:
        case 32:
            if (bmiLocal.bmiHeader.bV5Compression == BI_BITFIELDS)
            {
                bmiLocal.bmiHeader.bV5RedMask = 0xff0000;
                bmiLocal.bmiHeader.bV5GreenMask = 0x00ff00;
                bmiLocal.bmiHeader.bV5BlueMask = 0x0000ff;
            }
            break;
    }

    /* fill out the BITMAPINFO struct */
    if (!ChkBits)
    {
        bmiLocal.bmiHeader.bV5Width = psurf->SurfObj.sizlBitmap.cx;
        /* Report negative height for top-down bitmaps. */
        if (psurf->SurfObj.fjBitmap & BMF_TOPDOWN)
            bmiLocal.bmiHeader.bV5Height = - psurf->SurfObj.sizlBitmap.cy;
        else
            bmiLocal.bmiHeader.bV5Height = psurf->SurfObj.sizlBitmap.cy;
        bmiLocal.bmiHeader.bV5Planes = 1;
        bmiLocal.bmiHeader.bV5BitCount = BitsPerFormat(psurf->SurfObj.iBitmapFormat);
        switch (psurf->SurfObj.iBitmapFormat)
        {
                /* FIXME: What about BI_BITFIELDS? */
            case BMF_1BPP:
            case BMF_4BPP:
            case BMF_8BPP:
            case BMF_16BPP:
            case BMF_24BPP:
            case BMF_32BPP:
                bmiLocal.bmiHeader.bV5Compression = BI_RGB;
                break;
            case BMF_4RLE:
                bmiLocal.bmiHeader.bV5Compression = BI_RLE4;
                break;
            case BMF_8RLE:
                bmiLocal.bmiHeader.bV5Compression = BI_RLE8;
                break;
            case BMF_JPEG:
                bmiLocal.bmiHeader.bV5Compression = BI_JPEG;
                break;
            case BMF_PNG:
                bmiLocal.bmiHeader.bV5Compression = BI_PNG;
                break;
        }

        bmiLocal.bmiHeader.bV5SizeImage = psurf->SurfObj.cjBits;
        bmiLocal.bmiHeader.bV5XPelsPerMeter = psurf->sizlDim.cx; /* FIXME */
        bmiLocal.bmiHeader.bV5YPelsPerMeter = psurf->sizlDim.cy; /* FIXME */
        bmiLocal.bmiHeader.bV5ClrUsed = 0;
        bmiLocal.bmiHeader.bV5ClrImportant = 1 << bmiLocal.bmiHeader.bV5BitCount; /* FIXME */
        Result = psurf->SurfObj.sizlBitmap.cy;
    }
    else
    {
        SIZEL DestSize;
        POINTL SourcePoint;

//
// If we have a good dib pointer, why not just copy bits from there w/o XLATE'ing them.
//
        /* Create the destination bitmap too for the copy operation */
        if (StartScan > psurf->SurfObj.sizlBitmap.cy)
        {
            goto cleanup;
        }
        else
        {
            ScanLines = min(ScanLines, psurf->SurfObj.sizlBitmap.cy - StartScan);
            DestSize.cx = psurf->SurfObj.sizlBitmap.cx;
            DestSize.cy = ScanLines;

            hDestBitmap = NULL;

            bmiLocal.bmiHeader.bV5SizeImage = DIB_GetDIBWidthBytes(DestSize.cx,
                                              bmiLocal.bmiHeader.bV5BitCount) * DestSize.cy;

            hDestBitmap = EngCreateBitmap(DestSize,
                                          DIB_GetDIBWidthBytes(DestSize.cx,
                                                               bmiLocal.bmiHeader.bV5BitCount),
                                          BitmapFormat(bmiLocal.bmiHeader.bV5BitCount,
                                                       bmiLocal.bmiHeader.bV5Compression),
                                          bmiLocal.bmiHeader.bV5Height > 0 ? 0 : BMF_TOPDOWN,
                                          Bits);

            if (hDestBitmap == NULL)
                goto cleanup;
        }

        if (NT_SUCCESS(Status))
        {
            EXLATEOBJ exlo;
            SURFOBJ *DestSurfObj;
            RECTL DestRect;

            EXLATEOBJ_vInitialize(&exlo, psurf->ppal, ppalDst, 0, 0, 0);

            SourcePoint.x = 0;
            SourcePoint.y = psurf->SurfObj.sizlBitmap.cy - (StartScan + ScanLines);

            /* Determine destination rectangle */
            DestRect.top = 0;
            DestRect.left = 0;
            DestRect.right = DestSize.cx;
            DestRect.bottom = DestSize.cy;

            DestSurfObj = EngLockSurface((HSURF)hDestBitmap);

            if (IntEngCopyBits(DestSurfObj,
                               &psurf->SurfObj,
                               NULL,
                               &exlo.xlo,
                               &DestRect,
                               &SourcePoint))
            {
                DPRINT("GetDIBits %d \n",abs(bmiLocal.bmiHeader.bV5Height) - StartScan);
                Result = ScanLines;
            }

            EXLATEOBJ_vCleanup(&exlo);
            EngUnlockSurface(DestSurfObj);
        }
    }

    /* Now that everything is over, get back the information to caller */
    _SEH2_TRY
    {
        /* Note : Info has already been probed */
        GetBMIFromBitmapV5Info(&bmiLocal, Info, Usage);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* FIXME: fail or something */
    }
    _SEH2_END

cleanup:

    if (hDestBitmap != NULL)
        EngDeleteSurface((HSURF)hDestBitmap);

    if (hDestPalette != NULL && !bPaletteMatch)
    {
        PALETTE_UnlockPalette(ppalDst);
        PALETTE_FreePaletteByHandle(hDestPalette);
    }

    SURFACE_UnlockSurface(psurf);

    DPRINT("leaving NtGdiGetDIBitsInternal\n");

    return Result;
}

INT
APIENTRY
NtGdiStretchDIBitsInternal(
    HDC  hDC,
    INT  XDest,
    INT  YDest,
    INT  DestWidth,
    INT  DestHeight,
    INT  XSrc,
    INT  YSrc,
    INT  SrcWidth,
    INT  SrcHeight,
    LPBYTE Bits,
    LPBITMAPINFO BitsInfo,
    DWORD  Usage,
    DWORD  ROP,
    UINT cjMaxInfo,
    UINT cjMaxBits,
    HANDLE hcmXform)
{
    HBITMAP hBitmap, hOldBitmap = NULL;
    HDC hdcMem;
    HPALETTE hPal = NULL;
    PDC pDC;
    NTSTATUS Status;
    BITMAPV5INFO bmiLocal ;

    if (!Bits || !BitsInfo)
    {
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return 0;
    }

    _SEH2_TRY
    {
        Status = ProbeAndConvertToBitmapV5Info(&bmiLocal, BitsInfo, Usage, cjMaxInfo);
        ProbeForRead(Bits, cjMaxBits, 1);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtGdiStretchDIBitsInternal fail to read BitMapInfo: %x or Bits: %x\n",BitsInfo,Bits);
        return 0;
    }

    hdcMem = NtGdiCreateCompatibleDC(hDC);
    if (hdcMem == NULL)
    {
        DPRINT1("NtGdiCreateCompatibleDC fail create hdc\n");
        return 0;
    }

    hBitmap = NtGdiCreateCompatibleBitmap(hDC,
                                          abs(bmiLocal.bmiHeader.bV5Width),
                                          abs(bmiLocal.bmiHeader.bV5Height));
    if (hBitmap == NULL)
    {
        DPRINT1("NtGdiCreateCompatibleBitmap fail create bitmap\n");
        DPRINT1("hDC : 0x%08x \n", hDC);
        DPRINT1("BitsInfo->bmiHeader.biWidth : 0x%08x \n", BitsInfo->bmiHeader.biWidth);
        DPRINT1("BitsInfo->bmiHeader.biHeight : 0x%08x \n", BitsInfo->bmiHeader.biHeight);
        return 0;
    }

    /* Select the bitmap into hdcMem, and save a handle to the old bitmap */
    hOldBitmap = NtGdiSelectBitmap(hdcMem, hBitmap);

    if (Usage == DIB_PAL_COLORS)
    {
        hPal = NtGdiGetDCObject(hDC, GDI_OBJECT_TYPE_PALETTE);
        hPal = GdiSelectPalette(hdcMem, hPal, FALSE);
    }

    if (bmiLocal.bmiHeader.bV5Compression == BI_RLE4 ||
            bmiLocal.bmiHeader.bV5Compression == BI_RLE8)
    {
        /* copy existing bitmap from destination dc */
        if (SrcWidth == DestWidth && SrcHeight == DestHeight)
            NtGdiBitBlt(hdcMem, XSrc, abs(bmiLocal.bmiHeader.bV5Height) - SrcHeight - YSrc,
                        SrcWidth, SrcHeight, hDC, XDest, YDest, ROP, 0, 0);
        else
            NtGdiStretchBlt(hdcMem, XSrc, abs(bmiLocal.bmiHeader.bV5Height) - SrcHeight - YSrc,
                            SrcWidth, SrcHeight, hDC, XDest, YDest, DestWidth, DestHeight,
                            ROP, 0);
    }

    pDC = DC_LockDc(hdcMem);
    if (pDC != NULL)
    {
        /* Note BitsInfo->bmiHeader.biHeight is the number of scanline,
         * if it negitve we getting to many scanline for scanline is UINT not
         * a INT, so we need make the negtive value to positve and that make the
         * count correct for negtive bitmap, TODO : we need testcase for this api */
        IntSetDIBits(pDC, hBitmap, 0, abs(bmiLocal.bmiHeader.bV5Height), Bits,
                     &bmiLocal, Usage);

        DC_UnlockDc(pDC);
    }


    /* Origin for DIBitmap may be bottom left (positive biHeight) or top
       left (negative biHeight) */
    if (SrcWidth == DestWidth && SrcHeight == DestHeight)
        NtGdiBitBlt(hDC, XDest, YDest, DestWidth, DestHeight,
                    hdcMem, XSrc, abs(bmiLocal.bmiHeader.bV5Height) - SrcHeight - YSrc,
                    ROP, 0, 0);
    else
        NtGdiStretchBlt(hDC, XDest, YDest, DestWidth, DestHeight,
                        hdcMem, XSrc, abs(bmiLocal.bmiHeader.bV5Height) - SrcHeight - YSrc,
                        SrcWidth, SrcHeight, ROP, 0);

    /* cleanup */
    if (hPal)
        GdiSelectPalette(hdcMem, hPal, FALSE);

    if (hOldBitmap)
        NtGdiSelectBitmap(hdcMem, hOldBitmap);

    NtGdiDeleteObjectApp(hdcMem);

    GreDeleteObject(hBitmap);

    return SrcHeight;
}


HBITMAP
FASTCALL
IntCreateDIBitmap(
    PDC Dc,
    INT width,
    INT height,
    UINT bpp,
    DWORD init,
    LPBYTE bits,
    PBITMAPV5INFO data,
    DWORD coloruse)
{
    HBITMAP handle;
    BOOL fColor;

    // Check if we should create a monochrome or color bitmap. We create a monochrome bitmap only if it has exactly 2
    // colors, which are black followed by white, nothing else. In all other cases, we create a color bitmap.

    if (bpp != 1) fColor = TRUE;
    else if ((coloruse != DIB_RGB_COLORS) || (init != CBM_INIT) || !data) fColor = FALSE;
    else
    {
        const RGBQUAD *rgb = data->bmiColors;
        DWORD col = RGB(rgb->rgbRed, rgb->rgbGreen, rgb->rgbBlue);

        // Check if the first color of the colormap is black
        if ((col == RGB(0, 0, 0)))
        {
            rgb++;
            col = RGB(rgb->rgbRed, rgb->rgbGreen, rgb->rgbBlue);

            // If the second color is white, create a monochrome bitmap
            fColor = (col != RGB(0xff,0xff,0xff));
        }
        else fColor = TRUE;
    }

    // Now create the bitmap
    if (fColor)
    {
        handle = IntCreateCompatibleBitmap(Dc, width, height);
    }
    else
    {
        handle = GreCreateBitmap(width,
                                 height,
                                 1,
                                 1,
                                 NULL);
    }

    if (height < 0)
        height = -height;

    if (NULL != handle && CBM_INIT == init)
    {
        IntSetDIBits(Dc, handle, 0, height, bits, data, coloruse);
    }

    return handle;
}

// The CreateDIBitmap function creates a device-dependent bitmap (DDB) from a DIB and, optionally, sets the bitmap bits
// The DDB that is created will be whatever bit depth your reference DC is
HBITMAP
APIENTRY
NtGdiCreateDIBitmapInternal(
    IN HDC hDc,
    IN INT cx,
    IN INT cy,
    IN DWORD fInit,
    IN OPTIONAL LPBYTE pjInit,
    IN OPTIONAL LPBITMAPINFO pbmi,
    IN DWORD iUsage,
    IN UINT cjMaxInitInfo,
    IN UINT cjMaxBits,
    IN FLONG fl,
    IN HANDLE hcmXform)
{
    BITMAPV5INFO bmiLocal ;
    NTSTATUS Status = STATUS_SUCCESS;

    _SEH2_TRY
    {
        if(pbmi) Status = ProbeAndConvertToBitmapV5Info(&bmiLocal, pbmi, iUsage, cjMaxInitInfo);
        if(pjInit && (fInit == CBM_INIT)) ProbeForRead(pjInit, cjMaxBits, 1);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END

    if(!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return NULL;
    }

    return GreCreateDIBitmapInternal(hDc,
                                     cx,
                                     cy,
                                     fInit,
                                     pjInit,
                                     pbmi ? &bmiLocal : NULL,
                                     iUsage,
                                     fl,
                                     hcmXform);
}

HBITMAP
FASTCALL
GreCreateDIBitmapInternal(
    IN HDC hDc,
    IN INT cx,
    IN INT cy,
    IN DWORD fInit,
    IN OPTIONAL LPBYTE pjInit,
    IN OPTIONAL PBITMAPV5INFO pbmi,
    IN DWORD iUsage,
    IN FLONG fl,
    IN HANDLE hcmXform)
{
    PDC Dc;
    HBITMAP Bmp;
    WORD bpp;
    HDC hdcDest;

    if (!hDc) /* 1bpp monochrome bitmap */
    {  // Should use System Bitmap DC hSystemBM, with CreateCompatibleDC for this.
        hdcDest = IntGdiCreateDC(NULL, NULL, NULL, NULL,FALSE);
        if(!hdcDest)
        {
            return NULL;
        }
    }
    else
    {
        hdcDest = hDc;
    }

    Dc = DC_LockDc(hdcDest);
    if (!Dc)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return NULL;
    }
    /* It's OK to set bpp=0 here, as IntCreateDIBitmap will create a compatible Bitmap
     * if bpp != 1 and ignore the real value that was passed */
    if (pbmi)
        bpp = pbmi->bmiHeader.bV5BitCount;
    else
        bpp = 0;
    Bmp = IntCreateDIBitmap(Dc, cx, cy, bpp, fInit, pjInit, pbmi, iUsage);
    DC_UnlockDc(Dc);

    if(!hDc)
    {
        NtGdiDeleteObjectApp(hdcDest);
    }
    return Bmp;
}


HBITMAP
APIENTRY
NtGdiCreateDIBSection(
    IN HDC hDC,
    IN OPTIONAL HANDLE hSection,
    IN DWORD dwOffset,
    IN LPBITMAPINFO bmi,
    IN DWORD Usage,
    IN UINT cjHeader,
    IN FLONG fl,
    IN ULONG_PTR dwColorSpace,
    OUT PVOID *Bits)
{
    HBITMAP hbitmap = 0;
    DC *dc;
    BOOL bDesktopDC = FALSE;
	BITMAPV5INFO bmiLocal;
	NTSTATUS Status;

    if (!bmi) return hbitmap; // Make sure.

	_SEH2_TRY
    {
        Status = ProbeAndConvertToBitmapV5Info(&bmiLocal, bmi, Usage, 0);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END

    if(!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return NULL;
    }

    // If the reference hdc is null, take the desktop dc
    if (hDC == 0)
    {
        hDC = NtGdiCreateCompatibleDC(0);
        bDesktopDC = TRUE;
    }

    if ((dc = DC_LockDc(hDC)))
    {
        hbitmap = DIB_CreateDIBSection(dc,
                                       &bmiLocal,
                                       Usage,
                                       Bits,
                                       hSection,
                                       dwOffset,
                                       0);
        DC_UnlockDc(dc);
    }
    else
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
    }

    if (bDesktopDC)
        NtGdiDeleteObjectApp(hDC);

    return hbitmap;
}

HBITMAP
APIENTRY
DIB_CreateDIBSection(
    PDC dc,
    CONST BITMAPV5INFO *bmi,
    UINT usage,
    LPVOID *bits,
    HANDLE section,
    DWORD offset,
    DWORD ovr_pitch)
{
    HBITMAP res = 0;
    SURFACE *bmp = NULL;
    void *mapBits = NULL;
    HPALETTE hpal ;
	ULONG palMode = PAL_INDEXED;

    // Fill BITMAP32 structure with DIB data
    CONST BITMAPV5HEADER *bi = &bmi->bmiHeader;
    INT effHeight;
    ULONG totalSize;
    BITMAP bm;
    SIZEL Size;
    CONST RGBQUAD *lpRGB = NULL;
    HANDLE hSecure;
    DWORD dsBitfields[3] = {0};
    ULONG ColorCount;

    DPRINT("format (%ld,%ld), planes %d, bpp %d, size %ld, colors %ld (%s)\n",
           bi->bV5Width, bi->bV5Height, bi->bV5Planes, bi->bV5BitCount,
           bi->bV5SizeImage, bi->bV5ClrUsed, usage == DIB_PAL_COLORS? "PAL" : "RGB");

    /* CreateDIBSection should fail for compressed formats */
    if (bi->bV5Compression == BI_RLE4 || bi->bV5Compression == BI_RLE8)
    {
        return (HBITMAP)NULL;
    }

    effHeight = bi->bV5Height >= 0 ? bi->bV5Height : -bi->bV5Height;
    bm.bmType = 0;
    bm.bmWidth = bi->bV5Width;
    bm.bmHeight = effHeight;
    bm.bmWidthBytes = ovr_pitch ? ovr_pitch : (ULONG) DIB_GetDIBWidthBytes(bm.bmWidth, bi->bV5BitCount);

    bm.bmPlanes = bi->bV5Planes;
    bm.bmBitsPixel = bi->bV5BitCount;
    bm.bmBits = NULL;

    // Get storage location for DIB bits.  Only use biSizeImage if it's valid and
    // we're dealing with a compressed bitmap.  Otherwise, use width * height.
    totalSize = bi->bV5SizeImage && bi->bV5Compression != BI_RGB
                ? bi->bV5SizeImage : (ULONG)(bm.bmWidthBytes * effHeight);

    if (section)
    {
        SYSTEM_BASIC_INFORMATION Sbi;
        NTSTATUS Status;
        DWORD mapOffset;
        LARGE_INTEGER SectionOffset;
        SIZE_T mapSize;

        Status = ZwQuerySystemInformation(SystemBasicInformation,
                                          &Sbi,
                                          sizeof Sbi,
                                          0);
        if (!NT_SUCCESS(Status))
        {
            return NULL;
        }

        mapOffset = offset - (offset % Sbi.AllocationGranularity);
        mapSize = bi->bV5SizeImage + (offset - mapOffset);

        SectionOffset.LowPart  = mapOffset;
        SectionOffset.HighPart = 0;

        Status = ZwMapViewOfSection(section,
                                    NtCurrentProcess(),
                                    &mapBits,
                                    0,
                                    0,
                                    &SectionOffset,
                                    &mapSize,
                                    ViewShare,
                                    0,
                                    PAGE_READWRITE);
        if (!NT_SUCCESS(Status))
        {
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            return NULL;
        }

        if (mapBits) bm.bmBits = (char *)mapBits + (offset - mapOffset);
    }
    else if (ovr_pitch && offset)
        bm.bmBits = (LPVOID) offset;
    else
    {
        offset = 0;
        bm.bmBits = EngAllocUserMem(totalSize, 0);
		if(!bm.bmBits) goto cleanup;
    }

//  hSecure = MmSecureVirtualMemory(bm.bmBits, totalSize, PAGE_READWRITE);
    hSecure = (HANDLE)0x1; // HACK OF UNIMPLEMENTED KERNEL STUFF !!!!

    if (usage == DIB_PAL_COLORS)
    {
        lpRGB = DIB_MapPaletteColors(dc, bmi);
        ColorCount = bi->bV5ClrUsed;
        if (ColorCount == 0)
        {
            ColorCount = max(1 << bi->bV5BitCount, 256);
        }
    }
    else if(bi->bV5BitCount <= 8)
    {
        lpRGB = bmi->bmiColors;
        ColorCount = 1 << bi->bV5BitCount;
    }
	else
	{
		lpRGB = NULL;
		ColorCount = 0;
	}

    /* Set dsBitfields values */
    if (usage == DIB_PAL_COLORS || bi->bV5BitCount <= 8)
    {
        dsBitfields[0] = dsBitfields[1] = dsBitfields[2] = 0;
		palMode = PAL_INDEXED;
    }
    else if (bi->bV5Compression == BI_RGB)
    {
		dsBitfields[0] = dsBitfields[1] = dsBitfields[2] = 0;
        switch (bi->bV5BitCount)
        {
            case 15:
                palMode = PAL_RGB16_555;
				break;

            case 16:
                palMode = PAL_RGB16_565;
                break;

            case 24:
            case 32:
                palMode = PAL_RGB;
                break;
        }
    }
    else
    {
        dsBitfields[0] = bi->bV5RedMask;
        dsBitfields[1] = bi->bV5GreenMask;
        dsBitfields[2] = bi->bV5BlueMask;
		palMode = PAL_BITFIELDS;
    }

    // Create Device Dependent Bitmap and add DIB pointer
    Size.cx = bm.bmWidth;
    Size.cy = abs(bm.bmHeight);
    res = GreCreateBitmapEx(bm.bmWidth,
                            abs(bm.bmHeight),
                            bm.bmWidthBytes,
                            BitmapFormat(bi->bV5BitCount * bi->bV5Planes, bi->bV5Compression),
                            BMF_DONTCACHE | BMF_USERMEM | BMF_NOZEROINIT |
                              (bi->bV5Height < 0 ? BMF_TOPDOWN : 0),
                            bi->bV5SizeImage,
                            bm.bmBits);
    if (!res)
    {
        SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
        goto cleanup;
    }
    bmp = SURFACE_LockSurface(res);
    if (NULL == bmp)
    {
		SetLastWin32Error(ERROR_INVALID_HANDLE);
        goto cleanup;
    }

    /* WINE NOTE: WINE makes use of a colormap, which is a color translation
                  table between the DIB and the X physical device. Obviously,
                  this is left out of the ReactOS implementation. Instead,
                  we call NtGdiSetDIBColorTable. */
    bmp->hDIBSection = section;
    bmp->hSecure = hSecure;
    bmp->dwOffset = offset;
    bmp->flags = API_BITMAP;
    bmp->dsBitfields[0] = dsBitfields[0];
    bmp->dsBitfields[1] = dsBitfields[1];
    bmp->dsBitfields[2] = dsBitfields[2];
    bmp->biClrUsed = ColorCount;
    bmp->biClrImportant = bi->bV5ClrImportant;

    hpal = PALETTE_AllocPalette(palMode, ColorCount, (ULONG*)lpRGB,
                                                dsBitfields[0],
                                                dsBitfields[1],
                                                dsBitfields[2]);

    bmp->ppal = PALETTE_ShareLockPalette(hpal);
    /* Lazy delete hpal, it will be freed at surface release */
    GreDeleteObject(hpal);

    // Clean up in case of errors
cleanup:
    if (!res || !bmp || !bm.bmBits)
    {
        DPRINT("got an error res=%08x, bmp=%p, bm.bmBits=%p\n", res, bmp, bm.bmBits);
        if (bm.bmBits)
        {
            // MmUnsecureVirtualMemory(hSecure); // FIXME: Implement this!
            if (section)
            {
                ZwUnmapViewOfSection(NtCurrentProcess(), mapBits);
                bm.bmBits = NULL;
            }
            else
                if (!offset)
                    EngFreeUserMem(bm.bmBits), bm.bmBits = NULL;
        }

        if (bmp)
            bmp = NULL;

        if (res)
        {
            SURFACE_FreeSurfaceByHandle(res);
            res = 0;
        }
    }

    if (lpRGB != bmi->bmiColors && lpRGB)
    {
        ExFreePoolWithTag((PVOID)lpRGB, TAG_COLORMAP);
    }

    if (bmp)
    {
        SURFACE_UnlockSurface(bmp);
    }

    // Return BITMAP handle and storage location
    if (NULL != bm.bmBits && NULL != bits)
    {
        *bits = bm.bmBits;
    }

    return res;
}

/***********************************************************************
 *           DIB_GetDIBWidthBytes
 *
 * Return the width of a DIB bitmap in bytes. DIB bitmap data is 32-bit aligned.
 * http://www.microsoft.com/msdn/sdk/platforms/doc/sdk/win32/struc/src/str01.htm
 * 11/16/1999 (RJJ) lifted from wine
 */
INT FASTCALL DIB_GetDIBWidthBytes(INT width, INT depth)
{
    return ((width * depth + 31) & ~31) >> 3;
}

/***********************************************************************
 *           DIB_GetDIBImageBytes
 *
 * Return the number of bytes used to hold the image in a DIB bitmap.
 * 11/16/1999 (RJJ) lifted from wine
 */

INT APIENTRY DIB_GetDIBImageBytes(INT  width, INT height, INT depth)
{
    return DIB_GetDIBWidthBytes(width, depth) * (height < 0 ? -height : height);
}

/***********************************************************************
 *           DIB_BitmapInfoSize
 *
 * Return the size of the bitmap info structure including color table.
 * 11/16/1999 (RJJ) lifted from wine
 */

INT FASTCALL DIB_BitmapInfoSize(const BITMAPINFO * info, WORD coloruse)
{
    int colors;

    if (info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        BITMAPCOREHEADER *core = (BITMAPCOREHEADER *)info;
        colors = (core->bcBitCount <= 8) ? 1 << core->bcBitCount : 0;
        return sizeof(BITMAPCOREHEADER) + colors * ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBTRIPLE) : sizeof(WORD));
    }
    else  /* assume BITMAPINFOHEADER */
    {
        colors = info->bmiHeader.biClrUsed;
        if (!colors && (info->bmiHeader.biBitCount <= 8)) colors = 1 << info->bmiHeader.biBitCount;
        return info->bmiHeader.biSize + colors * ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBQUAD) : sizeof(WORD));
    }
}

RGBQUAD *
FASTCALL
DIB_MapPaletteColors(PDC dc, CONST BITMAPV5INFO* lpbmi)
{
    RGBQUAD *lpRGB;
    ULONG nNumColors,i;
    USHORT *lpIndex;
    PPALETTE palGDI;

    palGDI = PALETTE_LockPalette(dc->dclevel.hpal);

    if (NULL == palGDI)
    {
        return NULL;
    }

    if (palGDI->Mode != PAL_INDEXED)
    {
        PALETTE_UnlockPalette(palGDI);
        return NULL;
    }

    nNumColors = 1 << lpbmi->bmiHeader.bV5BitCount;
    if (lpbmi->bmiHeader.bV5ClrUsed)
    {
        nNumColors = min(nNumColors, lpbmi->bmiHeader.bV5ClrUsed);
    }

    lpRGB = (RGBQUAD *)ExAllocatePoolWithTag(PagedPool, sizeof(RGBQUAD) * nNumColors, TAG_COLORMAP);
    if (lpRGB == NULL)
    {
        PALETTE_UnlockPalette(palGDI);
        return NULL;
    }

    lpIndex = (USHORT *)&lpbmi->bmiColors[0];

    for (i = 0; i < nNumColors; i++)
    {
        if (*lpIndex < palGDI->NumColors)
        {
            lpRGB[i].rgbRed = palGDI->IndexedColors[*lpIndex].peRed;
            lpRGB[i].rgbGreen = palGDI->IndexedColors[*lpIndex].peGreen;
            lpRGB[i].rgbBlue = palGDI->IndexedColors[*lpIndex].peBlue;
        }
        else
        {
            lpRGB[i].rgbRed = 0;
            lpRGB[i].rgbGreen = 0;
            lpRGB[i].rgbBlue = 0;
        }
        lpRGB[i].rgbReserved = 0;
        lpIndex++;
    }
    PALETTE_UnlockPalette(palGDI);

    return lpRGB;
}

HPALETTE
FASTCALL
BuildDIBPalette(CONST BITMAPINFO *bmi, PINT paletteType)
{
    BYTE bits;
    ULONG ColorCount;
    HPALETTE hPal;
    ULONG RedMask, GreenMask, BlueMask;
    PDWORD pdwColors = (PDWORD)((PBYTE)bmi + bmi->bmiHeader.biSize);

    // Determine Bits Per Pixel
    bits = bmi->bmiHeader.biBitCount;

    // Determine paletteType from Bits Per Pixel
    if (bits <= 8)
    {
        *paletteType = PAL_INDEXED;
        RedMask = GreenMask = BlueMask = 0;
    }
    else if (bmi->bmiHeader.biCompression == BI_BITFIELDS)
    {
        *paletteType = PAL_BITFIELDS;
        RedMask = pdwColors[0];
        GreenMask = pdwColors[1];
        BlueMask = pdwColors[2];
    }
    else if (bits == 15)
    {
        *paletteType = PAL_BITFIELDS;
        RedMask = 0x7c00;
        GreenMask = 0x03e0;
        BlueMask = 0x001f;
    }
    else if (bits == 16)
    {
        *paletteType = PAL_BITFIELDS;
        RedMask = 0xF800;
        GreenMask = 0x07e0;
        BlueMask = 0x001f;
    }
    else
    {
        *paletteType = PAL_BGR;
        RedMask = 0xff0000;
        GreenMask = 0x00ff00;
        BlueMask = 0x0000ff;
    }

    if (bmi->bmiHeader.biClrUsed == 0)
    {
        ColorCount = 1 << bmi->bmiHeader.biBitCount;
    }
    else
    {
        ColorCount = bmi->bmiHeader.biClrUsed;
    }

    if (PAL_INDEXED == *paletteType)
    {
        hPal = PALETTE_AllocPaletteIndexedRGB(ColorCount, (RGBQUAD*)pdwColors);
    }
    else
    {
        hPal = PALETTE_AllocPalette(*paletteType, ColorCount,
                                    NULL,
                                    RedMask, GreenMask, BlueMask);
    }

    return hPal;
}

FORCEINLINE
DWORD
GetBMIColor(CONST BITMAPINFO* pbmi, INT i)
{
    DWORD dwRet = 0;
    INT size;
    if(pbmi->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        /* BITMAPCOREINFO holds RGBTRIPLE values */
        size = sizeof(RGBTRIPLE);
    }
    else
    {
        size = sizeof(RGBQUAD);
    }
    memcpy(&dwRet, (PBYTE)pbmi + pbmi->bmiHeader.biSize + i*size, size);
    return dwRet;
}

FORCEINLINE
VOID
SetBMIColor(CONST BITMAPINFO* pbmi, DWORD* color, INT i)
{
    PVOID pvColors = ((PBYTE)pbmi + pbmi->bmiHeader.biSize);
    if(pbmi->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        RGBTRIPLE *pColor = pvColors;
        pColor[i] = *(RGBTRIPLE*)color;
    }
    else
    {
        RGBQUAD *pColor = pvColors;
        pColor[i] = *(RGBQUAD*)color;
    }
}

NTSTATUS
FASTCALL
ProbeAndConvertToBitmapV5Info(
    OUT PBITMAPV5INFO pbmiDst,
    IN CONST BITMAPINFO* pbmiUnsafe,
    IN DWORD dwColorUse,
    IN UINT MaxSize)
{
    DWORD dwSize;
    ULONG ulWidthBytes;
    PBITMAPV5HEADER pbmhDst = &pbmiDst->bmiHeader;

    /* Get the size and probe */
    ProbeForRead(&pbmiUnsafe->bmiHeader.biSize, sizeof(DWORD), 1);
    dwSize = pbmiUnsafe->bmiHeader.biSize;
    /* At least dwSize bytes must be valids */
    ProbeForRead(pbmiUnsafe, max(dwSize, MaxSize), 1);
	if(!MaxSize)
		ProbeForRead(pbmiUnsafe, DIB_BitmapInfoSize(pbmiUnsafe, dwColorUse), 1);

    /* Check the size */
    // FIXME: are intermediate sizes allowed? As what are they interpreted?
    //        make sure we don't use a too big dwSize later
    if (dwSize != sizeof(BITMAPCOREHEADER) &&
        dwSize != sizeof(BITMAPINFOHEADER) &&
        dwSize != sizeof(BITMAPV4HEADER) &&
        dwSize != sizeof(BITMAPV5HEADER))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (dwSize == sizeof(BITMAPCOREHEADER))
    {
        PBITMAPCOREHEADER pbch = (PBITMAPCOREHEADER)pbmiUnsafe;

        /* Manually copy the fields that are present */
        pbmhDst->bV5Width = pbch->bcWidth;
        pbmhDst->bV5Height = pbch->bcHeight;
        pbmhDst->bV5Planes = pbch->bcPlanes;
        pbmhDst->bV5BitCount = pbch->bcBitCount;

        /* Set some default values */
        pbmhDst->bV5Compression = BI_RGB;
        pbmhDst->bV5SizeImage = DIB_GetDIBImageBytes(pbch->bcWidth,
                                                     pbch->bcHeight,
                                                     pbch->bcPlanes*pbch->bcBitCount) ;
        pbmhDst->bV5XPelsPerMeter = 72;
        pbmhDst->bV5YPelsPerMeter = 72;
        pbmhDst->bV5ClrUsed = 0;
        pbmhDst->bV5ClrImportant = 0;
    }
    else
    {
        /* Copy valid fields */
        memcpy(pbmiDst, pbmiUnsafe, dwSize);
        if(!pbmhDst->bV5SizeImage)
            pbmhDst->bV5SizeImage = DIB_GetDIBImageBytes(pbmhDst->bV5Width,
                                                         pbmhDst->bV5Height,
                                                         pbmhDst->bV5Planes*pbmhDst->bV5BitCount) ;

        if(dwSize < sizeof(BITMAPV5HEADER))
        {
            /* Zero out the rest of the V5 header */
            memset((char*)pbmiDst + dwSize, 0, sizeof(BITMAPV5HEADER) - dwSize);
        }
    }
    pbmhDst->bV5Size = sizeof(BITMAPV5HEADER);


    if (dwSize < sizeof(BITMAPV4HEADER))
    {
        if (pbmhDst->bV5Compression == BI_BITFIELDS)
        {
            pbmhDst->bV5RedMask = GetBMIColor(pbmiUnsafe, 0);
            pbmhDst->bV5GreenMask = GetBMIColor(pbmiUnsafe, 1);
            pbmhDst->bV5BlueMask = GetBMIColor(pbmiUnsafe, 2);
            pbmhDst->bV5AlphaMask = 0;
            pbmhDst->bV5ClrUsed = 0;
        }

//        pbmhDst->bV5CSType;
//        pbmhDst->bV5Endpoints;
//        pbmhDst->bV5GammaRed;
//        pbmhDst->bV5GammaGreen;
//        pbmhDst->bV5GammaBlue;
    }

    if (dwSize < sizeof(BITMAPV5HEADER))
    {
//        pbmhDst->bV5Intent;
//        pbmhDst->bV5ProfileData;
//        pbmhDst->bV5ProfileSize;
//        pbmhDst->bV5Reserved;
    }

    ulWidthBytes = ((pbmhDst->bV5Width * pbmhDst->bV5Planes *
                     pbmhDst->bV5BitCount + 31) & ~31) / 8;

    if (pbmhDst->bV5SizeImage == 0)
        pbmhDst->bV5SizeImage = abs(ulWidthBytes * pbmhDst->bV5Height);

    if (pbmhDst->bV5ClrUsed == 0)
    {
        switch(pbmhDst->bV5BitCount)
        {
            case 1:
                pbmhDst->bV5ClrUsed = 2;
                break;
            case 4:
                pbmhDst->bV5ClrUsed = 16;
                break;
            case 8:
                pbmhDst->bV5ClrUsed = 256;
                break;
            default:
                pbmhDst->bV5ClrUsed = 0;
                break;
        }
    }

    if (pbmhDst->bV5Planes != 1)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (pbmhDst->bV5BitCount != 0 && pbmhDst->bV5BitCount != 1 &&
        pbmhDst->bV5BitCount != 4 && pbmhDst->bV5BitCount != 8 &&
        pbmhDst->bV5BitCount != 16 && pbmhDst->bV5BitCount != 24 &&
        pbmhDst->bV5BitCount != 32)
    {
        DPRINT("Invalid bit count: %d\n", pbmhDst->bV5BitCount);
        return STATUS_INVALID_PARAMETER;
    }

    if ((pbmhDst->bV5BitCount == 0 &&
         pbmhDst->bV5Compression != BI_JPEG && pbmhDst->bV5Compression != BI_PNG))
    {
        DPRINT("Bit count 0 is invalid for compression %d.\n", pbmhDst->bV5Compression);
        return STATUS_INVALID_PARAMETER;
    }

    if (pbmhDst->bV5Compression == BI_BITFIELDS &&
        pbmhDst->bV5BitCount != 16 && pbmhDst->bV5BitCount != 32)
    {
        DPRINT("Bit count %d is invalid for compression BI_BITFIELDS.\n", pbmhDst->bV5BitCount);
        return STATUS_INVALID_PARAMETER;
    }

    /* Copy Colors */
    if(pbmhDst->bV5ClrUsed)
    {
        INT i;
        if(dwColorUse == DIB_PAL_COLORS)
        {
            RtlCopyMemory(pbmiDst->bmiColors,
                          pbmiUnsafe->bmiColors,
                          pbmhDst->bV5ClrUsed * sizeof(WORD));
        }
        else
        {
            for(i = 0; i < pbmhDst->bV5ClrUsed; i++)
            {
                ((DWORD*)pbmiDst->bmiColors)[i] = GetBMIColor(pbmiUnsafe, i);
            }
        }
    }

    return STATUS_SUCCESS;
}

VOID
FASTCALL
GetBMIFromBitmapV5Info(IN PBITMAPV5INFO pbmiSrc,
                       OUT PBITMAPINFO pbmiDst,
                       IN DWORD dwColorUse)
{
    if(pbmiDst->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        /* Manually set value */
        BITMAPCOREHEADER* pbmhCore = (BITMAPCOREHEADER*)&pbmiDst->bmiHeader;
        pbmhCore->bcWidth = pbmiSrc->bmiHeader.bV5Width;
        pbmhCore->bcHeight = pbmiSrc->bmiHeader.bV5Height;
        pbmhCore->bcPlanes = pbmiSrc->bmiHeader.bV5Planes;
        pbmhCore->bcBitCount = pbmiSrc->bmiHeader.bV5BitCount;
    }
    else
    {
        /* Copy valid Fields, keep bmiHeader.biSize safe */
        RtlCopyMemory(&pbmiDst->bmiHeader.biWidth,
                      &pbmiSrc->bmiHeader.bV5Width,
                      pbmiDst->bmiHeader.biSize - sizeof(DWORD));
    }
    if((pbmiDst->bmiHeader.biSize < sizeof(BITMAPV4HEADER)) &&
        (pbmiSrc->bmiHeader.bV5Compression == BI_BITFIELDS))
    {
        /* Masks are already set in V4 and V5 headers */
        SetBMIColor(pbmiDst, &pbmiSrc->bmiHeader.bV5RedMask, 0);
        SetBMIColor(pbmiDst, &pbmiSrc->bmiHeader.bV5GreenMask, 1);
        SetBMIColor(pbmiDst, &pbmiSrc->bmiHeader.bV5BlueMask, 2);
    }
    else
    {
        INT i;
        ULONG cColorsUsed;

        cColorsUsed = pbmiSrc->bmiHeader.bV5ClrUsed;
        if (cColorsUsed == 0 && pbmiSrc->bmiHeader.bV5BitCount <= 8)
            cColorsUsed = (1 << pbmiSrc->bmiHeader.bV5BitCount);

        if(dwColorUse == DIB_PAL_COLORS)
        {
            RtlCopyMemory(pbmiDst->bmiColors,
                          pbmiSrc->bmiColors,
                          cColorsUsed * sizeof(WORD));
        }
        else
        {
            for(i = 0; i < cColorsUsed; i++)
            {
                SetBMIColor(pbmiDst, (DWORD*)pbmiSrc->bmiColors + i, i);
            }
        }
    }
}

/* EOF */
