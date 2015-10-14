/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Palette Functions
 * FILE:              subsys/win32k/eng/palette.c
 * PROGRAMERS:        Jason Filby
 *                    Timo Kreuzer
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

//static UINT SystemPaletteUse = SYSPAL_NOSTATIC;  /* the program need save the pallete and restore it */

HPALETTE APIENTRY
GreCreatePaletteInternal ( IN LPLOGPALETTE pLogPal, IN UINT cEntries );

PALETTE gpalRGB, gpalBGR, gpalMono;

const PALETTEENTRY g_sysPalTemplate[NB_RESERVED_COLORS] =
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

unsigned short GetNumberOfBits(unsigned int dwMask)
{
   unsigned short wBits;
   for (wBits = 0; dwMask; dwMask = dwMask & (dwMask - 1))
      wBits++;
   return wBits;
}

// Create the system palette
HPALETTE FASTCALL PALETTE_Init(VOID)
{
    int i;
    HPALETTE hpalette;
    PLOGPALETTE palPtr;

    // create default palette (20 system colors)
    palPtr = ExAllocatePoolWithTag(PagedPool,
                                   sizeof(LOGPALETTE) +
                                       (NB_RESERVED_COLORS * sizeof(PALETTEENTRY)),
                                   TAG_PALETTE);
    if (!palPtr) return FALSE;

    palPtr->palVersion = 0x300;
    palPtr->palNumEntries = NB_RESERVED_COLORS;
    for (i=0; i<NB_RESERVED_COLORS; i++)
    {
        palPtr->palPalEntry[i].peRed = g_sysPalTemplate[i].peRed;
        palPtr->palPalEntry[i].peGreen = g_sysPalTemplate[i].peGreen;
        palPtr->palPalEntry[i].peBlue = g_sysPalTemplate[i].peBlue;
        palPtr->palPalEntry[i].peFlags = 0;
    }

    hpalette = GreCreatePaletteInternal(palPtr,NB_RESERVED_COLORS);
    ExFreePoolWithTag(palPtr, TAG_PALETTE);

    /*  palette_size = visual->map_entries; */

    gpalRGB.Mode = PAL_RGB;
    gpalRGB.RedMask = RGB(0xFF, 0x00, 0x00);
    gpalRGB.GreenMask = RGB(0x00, 0xFF, 0x00);
    gpalRGB.BlueMask = RGB(0x00, 0x00, 0xFF);

    gpalBGR.Mode = PAL_BGR;
    gpalBGR.RedMask = RGB(0x00, 0x00, 0xFF);
    gpalBGR.GreenMask = RGB(0x00, 0xFF, 0x00);
    gpalBGR.BlueMask = RGB(0xFF, 0x00, 0x00);

    memset(&gpalMono, 0, sizeof(PALETTE));
    gpalMono.Mode = PAL_MONOCHROME;

    return hpalette;
}

VOID FASTCALL PALETTE_ValidateFlags(PALETTEENTRY* lpPalE, INT size)
{
    int i = 0;
    for (; i<size ; i++)
        lpPalE[i].peFlags = PC_SYS_USED | (lpPalE[i].peFlags & 0x07);
}

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
    if (!PalGDI)
    {
        return NULL;
    }

    NewPalette = PalGDI->BaseObject.hHmgr;

    PalGDI->Self = NewPalette;
    PalGDI->Mode = Mode;

    if (NULL != Colors)
    {
        PalGDI->IndexedColors = ExAllocatePoolWithTag(PagedPool,
                                                      sizeof(PALETTEENTRY) * NumColors,
                                                      TAG_PALETTE);
        if (NULL == PalGDI->IndexedColors)
        {
            PALETTE_UnlockPalette(PalGDI);
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
        
        if (Red == 0x7c00 && Green == 0x3E0 && Blue == 0x1F)
            PalGDI->Mode |= PAL_RGB16_555;
        else if (Red == 0xF800 && Green == 0x7E0 && Blue == 0x1F)
            PalGDI->Mode |= PAL_RGB16_565;
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
    if (!PalGDI)
    {
        return NULL;
    }

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

BOOL INTERNAL_CALL
PALETTE_Cleanup(PVOID ObjectBody)
{
    PPALETTE pPal = (PPALETTE)ObjectBody;
    if (NULL != pPal->IndexedColors)
    {
        ExFreePoolWithTag(pPal->IndexedColors, TAG_PALETTE);
    }

    return TRUE;
}

INT FASTCALL
PALETTE_GetObject(PPALETTE ppal, INT cbCount, LPLOGBRUSH lpBuffer)
{
    if (!lpBuffer)
    {
        return sizeof(WORD);
    }

    if ((UINT)cbCount < sizeof(WORD)) return 0;
    *((WORD*)lpBuffer) = (WORD)ppal->NumColors;
    return sizeof(WORD);
}

ULONG
NTAPI
PALETTE_ulGetNearestPaletteIndex(PALETTE* ppal, ULONG iColor)
{
    ULONG ulDiff, ulColorDiff, ulMinimalDiff = 0xFFFFFF;
    ULONG i, ulBestIndex = 0;
    PALETTEENTRY peColor = *(PPALETTEENTRY)&iColor;

    /* Loop all palette entries, break on exact match */
    for (i = 0; i < ppal->NumColors && ulMinimalDiff != 0; i++)
    {
        /* Calculate distance in the color cube */
        ulDiff = peColor.peRed - ppal->IndexedColors[i].peRed;
        ulColorDiff = ulDiff * ulDiff;
        ulDiff = peColor.peGreen - ppal->IndexedColors[i].peGreen;
        ulColorDiff += ulDiff * ulDiff;
        ulDiff = peColor.peBlue - ppal->IndexedColors[i].peBlue;
        ulColorDiff += ulDiff * ulDiff;

        /* Check for a better match */
        if (ulColorDiff < ulMinimalDiff)
        {
            ulBestIndex = i;
            ulMinimalDiff = ulColorDiff;
        }
    }

    return ulBestIndex;
}

ULONG
NTAPI
PALETTE_ulGetNearestBitFieldsIndex(PALETTE* ppal, ULONG ulColor)
{
    ULONG ulNewColor;

    // FIXME: HACK, should be stored already
    ppal->ulRedShift = CalculateShift(RGB(0xff,0,0), ppal->RedMask);
    ppal->ulGreenShift = CalculateShift(RGB(0,0xff,0), ppal->GreenMask);
    ppal->ulBlueShift = CalculateShift(RGB(0,0,0xff), ppal->BlueMask);

    ulNewColor = _rotl(ulColor, ppal->ulRedShift) & ppal->RedMask;
    ulNewColor |= _rotl(ulColor, ppal->ulGreenShift) & ppal->GreenMask;
    ulNewColor |= _rotl(ulColor, ppal->ulBlueShift) & ppal->BlueMask;

   return ulNewColor;
}

ULONG
NTAPI
PALETTE_ulGetNearestIndex(PALETTE* ppal, ULONG ulColor)
{
    if (ppal->Mode & PAL_INDEXED) // use fl & PALINDEXED
        return PALETTE_ulGetNearestPaletteIndex(ppal, ulColor);
    else
        return PALETTE_ulGetNearestBitFieldsIndex(ppal, ulColor);
}

VOID
NTAPI
PALETTE_vGetBitMasks(PPALETTE ppal, PULONG pulColors)
{
    ASSERT(pulColors);

    if (ppal->Mode & PAL_INDEXED || ppal->Mode & PAL_RGB)
    {
        pulColors[0] = RGB(0xFF, 0x00, 0x00);
        pulColors[1] = RGB(0x00, 0xFF, 0x00);
        pulColors[2] = RGB(0x00, 0x00, 0xFF);
    }
    else if (ppal->Mode & PAL_BGR)
    {
        pulColors[0] = RGB(0x00, 0x00, 0xFF);
        pulColors[1] = RGB(0x00, 0xFF, 0x00);
        pulColors[2] = RGB(0xFF, 0x00, 0x00);
    }
    else if (ppal->Mode & PAL_BITFIELDS)
    {
        pulColors[0] = ppal->RedMask;
        pulColors[1] = ppal->GreenMask;
        pulColors[2] = ppal->BlueMask;
    }
}

VOID
FASTCALL
ColorCorrection(PPALETTE PalGDI, PPALETTEENTRY PaletteEntry, ULONG Colors)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)PalGDI->hPDev;

    if (!ppdev) return;
#if 0
    if (ppdev->flFlags & PDEV_GAMMARAMP_TABLE)
    {
        INT i;
        PGAMMARAMP GammaRamp = (PGAMMARAMP)ppdev->pvGammaRamp;
        for ( i = 0; i < Colors; i++)
        {
            PaletteEntry[i].peRed   += GammaRamp->Red[i];
            PaletteEntry[i].peGreen += GammaRamp->Green[i];
            PaletteEntry[i].peBlue  += GammaRamp->Blue[i];
        }
    }
#endif
    return;
}

/** Display Driver Interface **************************************************/

/*
 * @implemented
 */
HPALETTE
APIENTRY
EngCreatePalette(
    ULONG Mode,
    ULONG NumColors,
    ULONG *Colors,
    ULONG Red,
    ULONG Green,
    ULONG Blue)
{
    HPALETTE Palette;

    Palette = PALETTE_AllocPalette(Mode, NumColors, Colors, Red, Green, Blue);
    if (Palette != NULL)
    {
        GDIOBJ_SetOwnership(Palette, NULL);
    }

    return Palette;
}

/*
 * @implemented
 */
BOOL
APIENTRY
EngDeletePalette(IN HPALETTE Palette)
{
    GDIOBJ_SetOwnership(Palette, PsGetCurrentProcess());

    return PALETTE_FreePaletteByHandle(Palette);
}

/*
 * @implemented
 */
ULONG
APIENTRY
PALOBJ_cGetColors(PALOBJ *PalObj, ULONG Start, ULONG Colors, ULONG *PaletteEntry)
{
    PALETTE *PalGDI;

    PalGDI = (PALETTE*)PalObj;
   /* PalGDI = (PALETTE*)AccessInternalObjectFromUserObject(PalObj); */

    if (Start >= PalGDI->NumColors)
        return 0;

    Colors = min(Colors, PalGDI->NumColors - Start);

    /* NOTE: PaletteEntry ULONGs are in the same order as PALETTEENTRY. */
    RtlCopyMemory(PaletteEntry, PalGDI->IndexedColors + Start, sizeof(ULONG) * Colors);

    if (PalGDI->Mode & PAL_GAMMACORRECTION)
        ColorCorrection(PalGDI, (PPALETTEENTRY)PaletteEntry, Colors);

    return Colors;
}


/** Systemcall Interface ******************************************************/

/*
 * @implemented
 */
HPALETTE APIENTRY
GreCreatePaletteInternal ( IN LPLOGPALETTE pLogPal, IN UINT cEntries )
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
        PALETTE_UnlockPalette(PalGDI);
    }
    else
    {
        /* FIXME - Handle PalGDI == NULL!!!! */
        DPRINT1("waring PalGDI is NULL \n");
    }
  return NewPalette;
}


UINT
FASTCALL
IntGdiRealizePalette(HDC hDC)
{
  /*
   * This function doesn't do any real work now and there's plenty
   * of bugs in it.
   */

  PPALETTE palGDI, sysGDI;
  int realized = 0;
  PDC dc;
  HPALETTE systemPalette;
  //USHORT sysMode, palMode;

  dc = DC_LockDc(hDC);
  if (!dc) return 0;

  systemPalette = NtGdiGetStockObject(DEFAULT_PALETTE);
  palGDI = PALETTE_LockPalette(dc->dclevel.hpal);

  if (palGDI == NULL)
  {
    DPRINT1("IntGdiRealizePalette(): palGDI is NULL, exiting\n");
    DC_UnlockDc(dc);
    return 0;
  }

  sysGDI = PALETTE_LockPalette(systemPalette);

  if (sysGDI == NULL)
  {
    DPRINT1("IntGdiRealizePalette(): sysGDI is NULL, exiting\n");
    PALETTE_UnlockPalette(palGDI);
    DC_UnlockDc(dc);
    return 0;
  }

#if 0
  // The RealizePalette function modifies the palette for the device associated with the specified device context. If the
  // device context is a memory DC, the color table for the bitmap selected into the DC is modified. If the device
  // context is a display DC, the physical palette for that device is modified.
  if(dc->dctype == DC_TYPE_MEMORY)
  {
    // Memory managed DC
    DPRINT1("RealizePalette unimplemented for memory managed DCs\n");
  } else
  {
    DPRINT1("RealizePalette unimplemented for device DCs\n");
  }
#else
  DPRINT1("RealizePalette unimplemented\n");
#endif

  // need to pass this to IntEngCreateXlate with palettes unlocked
  //sysMode = sysGDI->Mode;
  //palMode = palGDI->Mode;
  PALETTE_UnlockPalette(sysGDI);
  PALETTE_UnlockPalette(palGDI);

  DC_UnlockDc(dc);

  return realized;
}

#if 0
UINT APIENTRY
IntAnimatePalette(HPALETTE hPal,
                  UINT StartIndex,
                  UINT NumEntries,
                  CONST PPALETTEENTRY PaletteColors)
{
    UINT ret = 0;

    if( hPal != NtGdiGetStockObject(DEFAULT_PALETTE) )
    {
        PPALETTE palPtr;
        UINT pal_entries;
        HDC hDC;
        PDC dc;
        PWINDOW_OBJECT Wnd;
        const PALETTEENTRY *pptr = PaletteColors;

        palPtr = (PPALETTE)PALETTE_LockPalette(hPal);
        if (!palPtr) return FALSE;

        pal_entries = palPtr->NumColors;
        if (StartIndex >= pal_entries)
        {
            PALETTE_UnlockPalette(palPtr);
            return FALSE;
        }
        if (StartIndex+NumEntries > pal_entries) NumEntries = pal_entries - StartIndex;

        for (NumEntries += StartIndex; StartIndex < NumEntries; StartIndex++, pptr++)
        {
            /* According to MSDN, only animate PC_RESERVED colours */
            if (palPtr->IndexedColors[StartIndex].peFlags & PC_RESERVED)
            {
                memcpy( &palPtr->IndexedColors[StartIndex], pptr,
                        sizeof(PALETTEENTRY) );
                ret++;
                PALETTE_ValidateFlags(&palPtr->IndexedColors[StartIndex], 1);
            }
        }

        PALETTE_UnlockPalette(palPtr);

        /* Immediately apply the new palette if current window uses it */
        Wnd = UserGetDesktopWindow();
        hDC =  UserGetWindowDC(Wnd);
        dc = DC_LockDc(hDC);
        if (NULL != dc)
        {
            if (dc->dclevel.hpal == hPal)
            {
                DC_UnlockDc(dc);
                IntGdiRealizePalette(hDC);
            }
            else
                DC_UnlockDc(dc);
        }
        UserReleaseDC(Wnd,hDC, FALSE);
    }
    return ret;
}
#endif

UINT APIENTRY
IntGetPaletteEntries(
    HPALETTE hpal,
    UINT StartIndex,
    UINT  Entries,
    LPPALETTEENTRY  pe)
{
    PPALETTE palGDI;
    UINT numEntries;

    palGDI = (PPALETTE) PALETTE_LockPalette(hpal);
    if (NULL == palGDI)
    {
        return 0;
    }

    numEntries = palGDI->NumColors;
    if (NULL != pe)
    {
        if (numEntries < StartIndex + Entries)
        {
            Entries = numEntries - StartIndex;
        }
        if (numEntries <= StartIndex)
        {
            PALETTE_UnlockPalette(palGDI);
            return 0;
        }
        memcpy(pe, palGDI->IndexedColors + StartIndex, Entries * sizeof(PALETTEENTRY));
    }
    else
    {
        Entries = numEntries;
    }

    PALETTE_UnlockPalette(palGDI);
    return Entries;
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

    if (!(dc = DC_LockDc(hDC)))
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return 0;
    }

    palGDI = PALETTE_LockPalette(dc->dclevel.hpal);
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
            Ret = dc->ppdev->gdiinfo.ulNumPalReg;
        }
    }

    if (palGDI != NULL)
        PALETTE_UnlockPalette(palGDI);

    if (dc != NULL)
        DC_UnlockDc(dc);

    return Ret;
}

UINT
APIENTRY
IntSetPaletteEntries(
    HPALETTE  hpal,
    UINT  Start,
    UINT  Entries,
    CONST LPPALETTEENTRY pe)
{
    PPALETTE palGDI;
    WORD numEntries;

    if ((UINT)hpal & GDI_HANDLE_STOCK_MASK)
    {
    	return 0;
    }

    palGDI = PALETTE_LockPalette(hpal);
    if (!palGDI) return 0;

    numEntries = palGDI->NumColors;
    if (Start >= numEntries)
    {
        PALETTE_UnlockPalette(palGDI);
        return 0;
    }
    if (numEntries < Start + Entries)
    {
        Entries = numEntries - Start;
    }
    memcpy(palGDI->IndexedColors + Start, pe, Entries * sizeof(PALETTEENTRY));
    PALETTE_UnlockPalette(palGDI);

    return Entries;
}

ULONG
APIENTRY
EngQueryPalette(
	IN HPALETTE  hPal,
	OUT ULONG  *piMode,
	IN ULONG  cColors,
	OUT ULONG  *pulColors
	)
{
  // www.osr.com/ddk/graphics/gdifncs_21t3.htm
  UNIMPLEMENTED;
  return 0;
}


LONG
APIENTRY
HT_ComputeRGBGammaTable(
	IN USHORT  GammaTableEntries,
	IN USHORT  GammaTableType,
	IN USHORT  RedGamma,
	IN USHORT  GreenGamma,
	IN USHORT  BlueGamma,
	OUT LPBYTE  pGammaTable
	)
{
  // www.osr.com/ddk/graphics/gdifncs_9dpj.htm
  UNIMPLEMENTED;
  return 0;
}

LONG
APIENTRY
HT_Get8BPPFormatPalette(
	OUT LPPALETTEENTRY  pPaletteEntry,
	IN USHORT  RedGamma,
	IN USHORT  GreenGamma,
	IN USHORT  BlueGamma
	)
{
  // www.osr.com/ddk/graphics/gdifncs_8kvb.htm
  UNIMPLEMENTED;
  return 0;
}



/*
 * @unimplemented
 */
LONG APIENTRY
HT_Get8BPPMaskPalette(
   IN OUT LPPALETTEENTRY PaletteEntry,
   IN BOOL Use8BPPMaskPal,
   IN BYTE CMYMask,
   IN USHORT RedGamma,
   IN USHORT GreenGamma,
   IN USHORT BlueGamma)
{
   UNIMPLEMENTED;
   return 0;
}

/* EOF */
