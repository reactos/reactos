/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         LGPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engmisc.c
 * PURPOSE:         Miscellaneous Support Routines
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 *                  Based on gdi/gdiobj.c from Wine:
 *                  Copyright 1993 Alexandre Julliard
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

HPALETTE
FASTCALL
PALETTE_AllocPalette(ULONG Mode,
                     ULONG NumColors,
                     ULONG *Colors,
                     ULONG Red,
                     ULONG Green,
                     ULONG Blue)
{
    HPALETTE NewPalette;
    PPALETTE PalGDI;

    PalGDI = (PPALETTE)GDIOBJ_AllocObjWithHandle(GDI_OBJECT_TYPE_PALETTE);
    if (!PalGDI) return NULL;

    NewPalette = PalGDI->BaseObject.hHmgr;

    PalGDI->Self = NewPalette;
    PalGDI->Mode = Mode;

    if (NULL != Colors)
    {
        PalGDI->IndexedColors = ExAllocatePoolWithTag(PagedPool, sizeof(PALETTEENTRY) * NumColors, TAG_PALETTE);
        if (NULL == PalGDI->IndexedColors)
        {
            PALETTE_FreePaletteByHandle(NewPalette);
            return NULL;
        }
        RtlCopyMemory(PalGDI->IndexedColors, Colors, sizeof(PALETTEENTRY) * NumColors);
    }

    if (PAL_INDEXED == Mode)
    {
        PalGDI->NumColors = NumColors;
    }
    else if (PAL_BITFIELDS == Mode)
    {
        PalGDI->RedMask = Red;
        PalGDI->GreenMask = Green;
        PalGDI->BlueMask = Blue;
    }

    PALETTE_UnlockPalette(PalGDI);

    return NewPalette;
}

HPALETTE
FASTCALL
PALETTE_AllocPaletteIndexedRGB(ULONG NumColors,
                               CONST RGBQUAD *Colors)
{
    HPALETTE NewPalette;
    PPALETTE PalGDI;
    UINT i;

    PalGDI = (PPALETTE)GDIOBJ_AllocObjWithHandle(GDI_OBJECT_TYPE_PALETTE);
    if (!PalGDI) return NULL;

    NewPalette = PalGDI->BaseObject.hHmgr;

    PalGDI->Self = NewPalette;
    PalGDI->Mode = PAL_INDEXED;

    PalGDI->IndexedColors = ExAllocatePoolWithTag(PagedPool,
                                                  sizeof(PALETTEENTRY) * NumColors,
                                                  TAG_PALETTE);
    if (NULL == PalGDI->IndexedColors)
    {
        PALETTE_UnlockPalette(PalGDI);
        PALETTE_FreePaletteByHandle(NewPalette);
        return NULL;
    }

    for (i = 0; i < NumColors; i++)
    {
        PalGDI->IndexedColors[i].peRed = Colors[i].rgbRed;
        PalGDI->IndexedColors[i].peGreen = Colors[i].rgbGreen;
        PalGDI->IndexedColors[i].peBlue = Colors[i].rgbBlue;
        PalGDI->IndexedColors[i].peFlags = 0;
    }

    PalGDI->NumColors = NumColors;

    PALETTE_UnlockPalette(PalGDI);

    return NewPalette;
}

RGBQUAD *
NTAPI
DIB_MapPaletteColors(PDC dc, CONST BITMAPINFO* lpbmi)
{
    RGBQUAD *lpRGB;
    ULONG nNumColors,i;
    USHORT *lpIndex;
    PPALETTE palGDI;

    palGDI = PALETTE_LockPalette(dc->hPalette);

    if (NULL == palGDI)
    {
        return NULL;
    }

    if (palGDI->Mode != PAL_INDEXED)
    {
        PALETTE_UnlockPalette(palGDI);
        return NULL;
    }

    nNumColors = 1 << lpbmi->bmiHeader.biBitCount;
    if (lpbmi->bmiHeader.biClrUsed)
    {
        nNumColors = min(nNumColors, lpbmi->bmiHeader.biClrUsed);
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
        lpRGB[i].rgbRed = palGDI->IndexedColors[*lpIndex].peRed;
        lpRGB[i].rgbGreen = palGDI->IndexedColors[*lpIndex].peGreen;
        lpRGB[i].rgbBlue = palGDI->IndexedColors[*lpIndex].peBlue;
        lpRGB[i].rgbReserved = 0;
        lpIndex++;
    }
    PALETTE_UnlockPalette(palGDI);

    return lpRGB;
}

HPALETTE
NTAPI
BuildDIBPalette(CONST BITMAPINFO *bmi, PINT paletteType)
{
    BYTE bits;
    ULONG ColorCount;
    PALETTEENTRY *palEntries = NULL;
    HPALETTE hPal;
    ULONG RedMask, GreenMask, BlueMask;

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
        RedMask = ((ULONG *)bmi->bmiColors)[0];
        GreenMask = ((ULONG *)bmi->bmiColors)[1];
        BlueMask = ((ULONG *)bmi->bmiColors)[2];
    }
    else if (bits < 24)
    {
        *paletteType = PAL_BITFIELDS;
        RedMask = 0x7c00;
        GreenMask = 0x03e0;
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
        hPal = PALETTE_AllocPaletteIndexedRGB(ColorCount, (RGBQUAD*)bmi->bmiColors);
    }
    else
    {
        hPal = PALETTE_AllocPalette(*paletteType, ColorCount,
                                    (ULONG*) palEntries,
                                    RedMask, GreenMask, BlueMask);
    }

    return hPal;
}


/* EOF */
