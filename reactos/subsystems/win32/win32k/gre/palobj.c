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

/* GLOBALS *******************************************************************/
HGDIOBJ hSystemPal;

#define NB_RESERVED_COLORS              20 /* number of fixed colors in system palette */
#define PC_SYS_USED     0x80		/* palentry is used (both system and logical) */

const PALETTEENTRY COLOR_sysPalTemplate[NB_RESERVED_COLORS] =
{
  // first 10 entries in the system palette
  // red  green blue  flags
  { 0x00, 0x00, 0x00, PC_SYS_USED },
  { 0x80, 0x00, 0x00, PC_SYS_USED },
  { 0x00, 0x80, 0x00, PC_SYS_USED },
  { 0x80, 0x80, 0x00, PC_SYS_USED },
  { 0x00, 0x00, 0x80, PC_SYS_USED },
  { 0x80, 0x00, 0x80, PC_SYS_USED },
  { 0x00, 0x80, 0x80, PC_SYS_USED },
  { 0xc0, 0xc0, 0xc0, PC_SYS_USED },
  { 0xc0, 0xdc, 0xc0, PC_SYS_USED },
  { 0xa6, 0xca, 0xf0, PC_SYS_USED },

  // ... c_min/2 dynamic colorcells
  // ... gap (for sparse palettes)
  // ... c_min/2 dynamic colorcells

  { 0xff, 0xfb, 0xf0, PC_SYS_USED },
  { 0xa0, 0xa0, 0xa4, PC_SYS_USED },
  { 0x80, 0x80, 0x80, PC_SYS_USED },
  { 0xff, 0x00, 0x00, PC_SYS_USED },
  { 0x00, 0xff, 0x00, PC_SYS_USED },
  { 0xff, 0xff, 0x00, PC_SYS_USED },
  { 0x00, 0x00, 0xff, PC_SYS_USED },
  { 0xff, 0x00, 0xff, PC_SYS_USED },
  { 0x00, 0xff, 0xff, PC_SYS_USED },
  { 0xff, 0xff, 0xff, PC_SYS_USED }     // last 10
};

const PALETTEENTRY* FASTCALL COLOR_GetSystemPaletteTemplate(void)
{
   return (const PALETTEENTRY*)&COLOR_sysPalTemplate;
}

/* PRIVATE FUNCTIONS *********************************************************/
/*
 * @implemented
 */
HPALETTE APIENTRY
GrepCreatePalette( IN LPLOGPALETTE pLogPal, IN UINT cEntries )
{
    PPALETTE PalGDI;
    HPALETTE NewPalette;

    pLogPal->palNumEntries = cEntries;
    NewPalette = PALETTE_AllocPalette( PAL_INDEXED,
                                       cEntries,
                                       (PULONG)pLogPal->palPalEntry,
                                       0, 0, 0);

    if (NewPalette == NULL)
    {
        return NULL;
    }

    PalGDI = (PPALETTE) PALETTE_LockPalette(NewPalette);
    if (PalGDI != NULL)
    {
        PALETTE_ValidateFlags(PalGDI->IndexedColors, PalGDI->NumColors);
        //PalGDI->logicalToSystem = NULL;
        PALETTE_UnlockPalette(PalGDI);
    }
    else
    {
        /* FIXME - Handle PalGDI == NULL!!!! */
        DPRINT1("Warning: PalGDI is NULL!\n");
    }
    return NewPalette;
}

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

// Create the system palette
VOID APIENTRY
PALETTE_Init(VOID)
{
    int i;
    PLOGPALETTE palPtr;

    const PALETTEENTRY* __sysPalTemplate = (const PALETTEENTRY*)COLOR_GetSystemPaletteTemplate();

    // create default palette (20 system colors)
    palPtr = ExAllocatePoolWithTag(PagedPool,
                                   sizeof(LOGPALETTE) +
                                       (NB_RESERVED_COLORS * sizeof(PALETTEENTRY)),
                                   TAG_PALETTE);
    if (!palPtr)
    {
        hSystemPal = 0;
        return;
    }

    palPtr->palVersion = 0x300;
    palPtr->palNumEntries = NB_RESERVED_COLORS;
    for (i=0; i<NB_RESERVED_COLORS; i++)
    {
        palPtr->palPalEntry[i].peRed = __sysPalTemplate[i].peRed;
        palPtr->palPalEntry[i].peGreen = __sysPalTemplate[i].peGreen;
        palPtr->palPalEntry[i].peBlue = __sysPalTemplate[i].peBlue;
        palPtr->palPalEntry[i].peFlags = 0;
    }

    hSystemPal = GrepCreatePalette(palPtr,NB_RESERVED_COLORS);
    ExFreePoolWithTag(palPtr, TAG_PALETTE);

    /* Convert it to a stock object */
    GDIOBJ_ConvertToStockObj(&hSystemPal);
}

UINT APIENTRY
GreGetSystemPaletteEntries(HDC  hDC,
                           UINT  StartIndex,
                           UINT  Entries,
                           LPPALETTEENTRY  pe)
{
    PPALETTE palGDI = NULL;
    PDC dc = NULL;
    UINT EntriesSize = 0;
    UINT Ret = 0;

    if (Entries == 0)
    {
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (pe != NULL)
    {
        EntriesSize = Entries * sizeof(pe[0]);
        if (Entries != EntriesSize / sizeof(pe[0]))
        {
            /* Integer overflow! */
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            return 0;
        }
    }

    if (!(dc = DC_Lock(hDC)))
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return 0;
    }

    palGDI = PALETTE_LockPalette(dc->hPalette);
    if (palGDI != NULL)
    {
        if (pe != NULL)
        {
            if (StartIndex >= palGDI->NumColors)
                Entries = 0;
            else if (Entries > palGDI->NumColors - StartIndex)
                Entries = palGDI->NumColors - StartIndex;

            memcpy(pe,
                   palGDI->IndexedColors + StartIndex,
                   Entries * sizeof(pe[0]));

            Ret = Entries;
        }
        else
        {
            Ret = dc->pPDevice->GDIInfo.ulNumPalReg;
        }
    }

    if (palGDI != NULL)
        PALETTE_UnlockPalette(palGDI);

    if (dc != NULL)
        DC_Unlock(dc);

    return Ret;
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
    UNIMPLEMENTED;
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

VOID FASTCALL PALETTE_ValidateFlags(PALETTEENTRY* lpPalE, INT size)
{
    int i = 0;
    for (; i<size ; i++)
        lpPalE[i].peFlags = PC_SYS_USED | (lpPalE[i].peFlags & 0x07);
}

/* EOF */
