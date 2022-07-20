/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS Win32k subsystem
 * PURPOSE:           GDI Palette Functions
 * FILE:              win32ss/gdi/ntgdi/palette.c
 * PROGRAMERS:        Jason Filby
 *                    Timo Kreuzer
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

#define PAL_SETPOWNER 0x8000
#define MAX_PALCOLORS 65536

static UINT SystemPaletteUse = SYSPAL_NOSTATIC;  /* The program need save the pallete and restore it */

PALETTE gpalRGB, gpalBGR, gpalRGB555, gpalRGB565, *gppalMono, *gppalDefault;
PPALETTE appalSurfaceDefault[11];

const PALETTEENTRY g_sysPalTemplate[NB_RESERVED_COLORS] =
{
  // First 10 entries in the system palette
  // Red  Green Blue  Flags
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
  { 0xff, 0xff, 0xff, PC_SYS_USED }     // Last 10
};

unsigned short GetNumberOfBits(unsigned int dwMask)
{
   unsigned short wBits;
   for (wBits = 0; dwMask; dwMask = dwMask & (dwMask - 1))
      wBits++;
   return wBits;
}

// Create the system palette
CODE_SEG("INIT")
NTSTATUS
NTAPI
InitPaletteImpl(VOID)
{
    // Create default palette (20 system colors)
    gppalDefault = PALETTE_AllocPalWithHandle(PAL_INDEXED,
                                              20,
                                              g_sysPalTemplate,
                                              0, 0, 0);
    GDIOBJ_vReferenceObjectByPointer(&gppalDefault->BaseObject);
    PALETTE_UnlockPalette(gppalDefault);

    /*  palette_size = visual->map_entries; */

    gpalRGB.flFlags = PAL_RGB;
    gpalRGB.RedMask = RGB(0xFF, 0x00, 0x00);
    gpalRGB.GreenMask = RGB(0x00, 0xFF, 0x00);
    gpalRGB.BlueMask = RGB(0x00, 0x00, 0xFF);
    gpalRGB.BaseObject.ulShareCount = 1;
    gpalRGB.BaseObject.BaseFlags = 0 ;

    gpalBGR.flFlags = PAL_BGR;
    gpalBGR.RedMask = RGB(0x00, 0x00, 0xFF);
    gpalBGR.GreenMask = RGB(0x00, 0xFF, 0x00);
    gpalBGR.BlueMask = RGB(0xFF, 0x00, 0x00);
    gpalBGR.BaseObject.ulShareCount = 1;
    gpalBGR.BaseObject.BaseFlags = 0 ;

    gpalRGB555.flFlags = PAL_RGB16_555 | PAL_BITFIELDS;
    gpalRGB555.RedMask = 0x7C00;
    gpalRGB555.GreenMask = 0x3E0;
    gpalRGB555.BlueMask = 0x1F;
    gpalRGB555.BaseObject.ulShareCount = 1;
    gpalRGB555.BaseObject.BaseFlags = 0 ;

    gpalRGB565.flFlags = PAL_RGB16_565 | PAL_BITFIELDS;
    gpalRGB565.RedMask = 0xF800;
    gpalRGB565.GreenMask = 0x7E0;
    gpalRGB565.BlueMask = 0x1F;
    gpalRGB565.BaseObject.ulShareCount = 1;
    gpalRGB565.BaseObject.BaseFlags = 0 ;

    gppalMono = PALETTE_AllocPalette(PAL_MONOCHROME|PAL_INDEXED, 2, NULL, 0, 0, 0);
    PALETTE_vSetRGBColorForIndex(gppalMono, 0, 0x000000);
    PALETTE_vSetRGBColorForIndex(gppalMono, 1, 0xffffff);

    /* Initialize default surface palettes */
    appalSurfaceDefault[BMF_1BPP] = gppalMono;
    appalSurfaceDefault[BMF_4BPP] = gppalDefault;
    appalSurfaceDefault[BMF_8BPP] = gppalDefault;
    appalSurfaceDefault[BMF_16BPP] = &gpalRGB565;
    appalSurfaceDefault[BMF_24BPP] = &gpalBGR;
    appalSurfaceDefault[BMF_32BPP] = &gpalBGR;
    appalSurfaceDefault[BMF_4RLE] = gppalDefault;
    appalSurfaceDefault[BMF_8RLE] = gppalDefault;
    appalSurfaceDefault[BMF_JPEG] = &gpalRGB;
    appalSurfaceDefault[BMF_PNG] = &gpalRGB;

    return STATUS_SUCCESS;
}

VOID FASTCALL PALETTE_ValidateFlags(PALETTEENTRY* lpPalE, INT size)
{
    int i = 0;
    for (; i<size ; i++)
        lpPalE[i].peFlags = PC_SYS_USED | (lpPalE[i].peFlags & 0x07);
}


PPALETTE
NTAPI
PALETTE_AllocPalette(
    _In_ ULONG iMode,
    _In_ ULONG cColors,
    _In_opt_ const PALETTEENTRY* pEntries,
    _In_ FLONG flRed,
    _In_ FLONG flGreen,
    _In_ FLONG flBlue)
{
    PPALETTE ppal;
    ULONG fl = 0, cjSize = sizeof(PALETTE);

    /* Check if the palette has entries */
    if (iMode & PAL_INDEXED)
    {
        /* Check color count */
        if ((cColors == 0) || (cColors > 1024)) return NULL;

        /* Allocate enough space for the palete entries */
        cjSize += cColors * sizeof(PALETTEENTRY);
    }
    else
    {
        /* There are no palette entries */
        cColors = 0;

        /* We can use the lookaside list */
        fl |= BASEFLAG_LOOKASIDE;
    }

    /* Allocate the object (without a handle!) */
    ppal = (PPALETTE)GDIOBJ_AllocateObject(GDIObjType_PAL_TYPE, cjSize, fl);
    if (!ppal)
    {
        return NULL;
    }

    /* Set mode, color count and entry pointer */
    ppal->flFlags = iMode;
    ppal->NumColors = cColors;
    ppal->IndexedColors = ppal->apalColors;

    /* Check what kind of palette this is */
    if (iMode & PAL_INDEXED)
    {
        /* Check if we got a color array */
        if (pEntries)
        {
            /* Copy the entries */
            RtlCopyMemory(ppal->IndexedColors, pEntries, cColors * sizeof(pEntries[0]));
        }
    }
    else if (iMode & PAL_BITFIELDS)
    {
        /* Copy the color masks */
        ppal->RedMask = flRed;
        ppal->GreenMask = flGreen;
        ppal->BlueMask = flBlue;

        /* Check what masks we have and set optimization flags */
        if ((flRed == 0x7c00) && (flGreen == 0x3E0) && (flBlue == 0x1F))
            ppal->flFlags |= PAL_RGB16_555;
        else if ((flRed == 0xF800) && (flGreen == 0x7E0) && (flBlue == 0x1F))
            ppal->flFlags |= PAL_RGB16_565;
        else if ((flRed == 0xFF0000) && (flGreen == 0xFF00) && (flBlue == 0xFF))
            ppal->flFlags |= PAL_BGR;
        else if ((flRed == 0xFF) && (flGreen == 0xFF00) && (flBlue == 0xFF0000))
            ppal->flFlags |= PAL_RGB;
    }

    return ppal;
}

PPALETTE
NTAPI
PALETTE_AllocPalWithHandle(
    _In_ ULONG iMode,
    _In_ ULONG cColors,
    _In_opt_ const PALETTEENTRY* pEntries,
    _In_ FLONG flRed,
    _In_ FLONG flGreen,
    _In_ FLONG flBlue)
{
    PPALETTE ppal;

    /* Allocate the palette without a handle */
    ppal = PALETTE_AllocPalette(iMode, cColors, pEntries, flRed, flGreen, flBlue);
    if (!ppal) return NULL;

    /* Insert the palette into the handle table */
    if (!GDIOBJ_hInsertObject(&ppal->BaseObject, GDI_OBJ_HMGR_POWNED))
    {
        DPRINT1("Could not insert palette into handle table.\n");
        GDIOBJ_vFreeObject(&ppal->BaseObject);
        return NULL;
    }

    return ppal;
}

VOID
NTAPI
PALETTE_vCleanup(PVOID ObjectBody)
{
    PPALETTE pPal = (PPALETTE)ObjectBody;
    if (pPal->IndexedColors && pPal->IndexedColors != pPal->apalColors)
    {
        ExFreePoolWithTag(pPal->IndexedColors, TAG_PALETTE);
    }
}

INT
FASTCALL
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

    /* Loop all palette entries */
    for (i = 0; i < ppal->NumColors; i++)
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

            /* Break on exact match */
            if (ulMinimalDiff == 0) break;
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
    if (ppal->flFlags & PAL_INDEXED) // Use fl & PALINDEXED
        return PALETTE_ulGetNearestPaletteIndex(ppal, ulColor);
    else
        return PALETTE_ulGetNearestBitFieldsIndex(ppal, ulColor);
}

VOID
NTAPI
PALETTE_vGetBitMasks(PPALETTE ppal, PULONG pulColors)
{
    ASSERT(pulColors);

    if (ppal->flFlags & PAL_INDEXED || ppal->flFlags & PAL_RGB)
    {
        pulColors[0] = RGB(0xFF, 0x00, 0x00);
        pulColors[1] = RGB(0x00, 0xFF, 0x00);
        pulColors[2] = RGB(0x00, 0x00, 0xFF);
    }
    else if (ppal->flFlags & PAL_BGR)
    {
        pulColors[0] = RGB(0x00, 0x00, 0xFF);
        pulColors[1] = RGB(0x00, 0xFF, 0x00);
        pulColors[2] = RGB(0xFF, 0x00, 0x00);
    }
    else if (ppal->flFlags & PAL_BITFIELDS)
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

    if (ppdev->flFlags & PDEV_GAMMARAMP_TABLE)
    {
        ULONG i;
        PGAMMARAMP GammaRamp = (PGAMMARAMP)ppdev->pvGammaRamp;
        for ( i = 0; i < Colors; i++)
        {
            PaletteEntry[i].peRed   += GammaRamp->Red[i];
            PaletteEntry[i].peGreen += GammaRamp->Green[i];
            PaletteEntry[i].peBlue  += GammaRamp->Blue[i];
        }
    }
    return;
}

/** Display Driver Interface **************************************************/

/*
 * @implemented
 */
HPALETTE
APIENTRY
EngCreatePalette(
    ULONG iMode,
    ULONG cColors,
    ULONG *pulColors,
    ULONG flRed,
    ULONG flGreen,
    ULONG flBlue)
{
    PPALETTE ppal;
    HPALETTE hpal;

    ppal = PALETTE_AllocPalette(iMode, cColors, (PPALETTEENTRY)pulColors, flRed, flGreen, flBlue);
    if (!ppal) return NULL;

    hpal = GDIOBJ_hInsertObject(&ppal->BaseObject, GDI_OBJ_HMGR_PUBLIC);
    if (!hpal)
    {
        DPRINT1("Could not insert palette into handle table.\n");
        GDIOBJ_vFreeObject(&ppal->BaseObject);
        return NULL;
    }

    PALETTE_UnlockPalette(ppal);
    return hpal;
}

/*
 * @implemented
 */
BOOL
APIENTRY
EngDeletePalette(IN HPALETTE hpal)
{
    PPALETTE ppal;

    ppal = PALETTE_ShareLockPalette(hpal);
    if (!ppal) return FALSE;

    GDIOBJ_vDeleteObject(&ppal->BaseObject);

    return TRUE;
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

    if (Start >= PalGDI->NumColors)
        return 0;

    Colors = min(Colors, PalGDI->NumColors - Start);

    /* NOTE: PaletteEntry ULONGs are in the same order as PALETTEENTRY. */
    RtlCopyMemory(PaletteEntry, PalGDI->IndexedColors + Start, sizeof(ULONG) * Colors);

    if (PalGDI->flFlags & PAL_GAMMACORRECTION)
        ColorCorrection(PalGDI, (PPALETTEENTRY)PaletteEntry, Colors);

    return Colors;
}


/** Systemcall Interface ******************************************************/

HPALETTE
NTAPI
GreCreatePaletteInternal(
    IN LPLOGPALETTE pLogPal,
    IN UINT cEntries)
{
    HPALETTE hpal = NULL;
    PPALETTE ppal;

    pLogPal->palNumEntries = cEntries;
    ppal = PALETTE_AllocPalWithHandle(PAL_INDEXED,
                                      cEntries,
                                      pLogPal->palPalEntry,
                                      0, 0, 0);

    if (ppal != NULL)
    {
        PALETTE_ValidateFlags(ppal->IndexedColors, ppal->NumColors);

        hpal = ppal->BaseObject.hHmgr;
        PALETTE_UnlockPalette(ppal);
    }

    return hpal;
}

/*
 * @implemented
 */
HPALETTE
APIENTRY
NtGdiCreatePaletteInternal(
    IN LPLOGPALETTE plogpalUser,
    IN UINT cEntries)
{
    HPALETTE hpal = NULL;
    PPALETTE ppal;
    ULONG i, cjSize;

    ppal = PALETTE_AllocPalWithHandle(PAL_INDEXED, cEntries, NULL, 0, 0, 0);
    if (ppal == NULL)
    {
        return NULL;
    }

    cjSize = FIELD_OFFSET(LOGPALETTE, palPalEntry[cEntries]);

    _SEH2_TRY
    {
        ProbeForRead(plogpalUser, cjSize, 1);

        for (i = 0; i < cEntries; i++)
        {
            ppal->IndexedColors[i] = plogpalUser->palPalEntry[i];
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        GDIOBJ_vDeleteObject(&ppal->BaseObject);
        _SEH2_YIELD(return NULL);
    }
    _SEH2_END;

    PALETTE_ValidateFlags(ppal->IndexedColors, cEntries);
    hpal = ppal->BaseObject.hHmgr;
    PALETTE_UnlockPalette(ppal);

    return hpal;
}

HPALETTE
APIENTRY
NtGdiCreateHalftonePalette(HDC  hDC)
{
    int i, r, g, b;
    PALETTEENTRY PalEntries[256];
    PPALETTE ppal;
    PDC pdc;
    HPALETTE hpal = NULL;

    pdc = DC_LockDc(hDC);
    if (!pdc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }

    RtlZeroMemory(PalEntries, sizeof(PalEntries));

    /* First and last ten entries are default ones */
    for (i = 0; i < 10; i++)
    {
        PalEntries[i].peRed = g_sysPalTemplate[i].peRed;
        PalEntries[i].peGreen = g_sysPalTemplate[i].peGreen;
        PalEntries[i].peBlue = g_sysPalTemplate[i].peBlue;

        PalEntries[246 + i].peRed = g_sysPalTemplate[10 + i].peRed;
        PalEntries[246 + i].peGreen = g_sysPalTemplate[10 + i].peGreen;
        PalEntries[246 + i].peBlue = g_sysPalTemplate[10 + i].peBlue;
    }

    ppal = PALETTE_ShareLockPalette(pdc->dclevel.hpal);
    if (ppal && (ppal->flFlags & PAL_INDEXED))
    {
        /* FIXME: optimize the palette for the current palette */
        UNIMPLEMENTED;
    }
    else
    {
        for (r = 0; r < 6; r++)
        {
            for (g = 0; g < 6; g++)
            {
                for (b = 0; b < 6; b++)
                {
                    i = r + g*6 + b*36 + 10;
                    PalEntries[i].peRed = r * 51;
                    PalEntries[i].peGreen = g * 51;
                    PalEntries[i].peBlue = b * 51;
                }
            }
        }

        for (i = 216; i < 246; i++)
        {
            int v = (i - 216) << 3;
            PalEntries[i].peRed = v;
            PalEntries[i].peGreen = v;
            PalEntries[i].peBlue = v;
        }
    }

    if (ppal)
        PALETTE_ShareUnlockPalette(ppal);

    DC_UnlockDc(pdc);

    ppal = PALETTE_AllocPalWithHandle(PAL_INDEXED, 256, PalEntries, 0, 0, 0);
    if (ppal)
    {
        hpal = ppal->BaseObject.hHmgr;
        PALETTE_UnlockPalette(ppal);
    }

    return hpal;
}

BOOL
APIENTRY
NtGdiResizePalette(
    HPALETTE hpal,
    UINT Entries)
{
/*  PALOBJ *palPtr = (PALOBJ*)AccessUserObject(hPal);
  UINT cPrevEnt, prevVer;
  INT prevsize, size = sizeof(LOGPALETTE) + (cEntries - 1) * sizeof(PALETTEENTRY);
  XLATEOBJ *XlateObj = NULL;

  if(!palPtr) return FALSE;
  cPrevEnt = palPtr->logpalette->palNumEntries;
  prevVer = palPtr->logpalette->palVersion;
  prevsize = sizeof(LOGPALETTE) + (cPrevEnt - 1) * sizeof(PALETTEENTRY) + sizeof(int*) + sizeof(GDIOBJHDR);
  size += sizeof(int*) + sizeof(GDIOBJHDR);
  XlateObj = palPtr->logicalToSystem;

  if (!(palPtr = GDI_ReallocObject(size, hPal, palPtr))) return FALSE;

  if(XlateObj)
  {
    XLATEOBJ *NewXlateObj = (int*) HeapReAlloc(GetProcessHeap(), 0, XlateObj, cEntries * sizeof(int));
    if(NewXlateObj == NULL)
    {
      ERR("Can not resize logicalToSystem -- out of memory!\n");
      GDI_ReleaseObj( hPal );
      return FALSE;
    }
    palPtr->logicalToSystem = NewXlateObj;
  }

  if(cEntries > cPrevEnt)
  {
    if(XlateObj) memset(palPtr->logicalToSystem + cPrevEnt, 0, (cEntries - cPrevEnt)*sizeof(int));
    memset( (BYTE*)palPtr + prevsize, 0, size - prevsize );
    PALETTE_ValidateFlags((PALETTEENTRY*)((BYTE*)palPtr + prevsize), cEntries - cPrevEnt );
  }
  palPtr->logpalette->palNumEntries = cEntries;
  palPtr->logpalette->palVersion = prevVer;
//    GDI_ReleaseObj( hPal );
  return TRUE; */

  UNIMPLEMENTED;
  return FALSE;
}

BOOL
APIENTRY
NtGdiGetColorAdjustment(
    HDC hdc,
    LPCOLORADJUSTMENT pca)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiSetColorAdjustment(
    HDC hdc,
    LPCOLORADJUSTMENT pca)
{
    UNIMPLEMENTED;
    return FALSE;
}

COLORREF
APIENTRY
NtGdiGetNearestColor(
    _In_ HDC hDC,
    _In_ COLORREF Color)
{
    COLORREF nearest = CLR_INVALID;
    PDC dc;
    EXLATEOBJ exlo;
    PPALETTE ppal;

    dc = DC_LockDc(hDC);

    if(dc == NULL)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return CLR_INVALID;
    }

    /// FIXME: shouldn't dereference pSurface while the PDEV is not locked
    if(dc->dclevel.pSurface == NULL)
        ppal = gppalMono;
    else
        ppal = dc->dclevel.pSurface->ppal;

    /* Translate the color to the DC format */
    Color = TranslateCOLORREF(dc, Color);

    /* XLATE it back to RGB color space */
    EXLATEOBJ_vInitialize(&exlo,
        ppal,
        &gpalRGB,
        0,
        RGB(0xff, 0xff, 0xff),
        RGB(0, 0, 0));

    nearest = XLATEOBJ_iXlate(&exlo.xlo, Color);

    EXLATEOBJ_vCleanup(&exlo);

    /* We're done */
    DC_UnlockDc(dc);

   return nearest;
}

UINT
APIENTRY
NtGdiGetNearestPaletteIndex(
    HPALETTE hpal,
    COLORREF crColor)
{
    PPALETTE ppal = PALETTE_ShareLockPalette(hpal);
    UINT index  = 0;

    if (ppal)
    {
        if (ppal->flFlags & PAL_INDEXED)
        {
            /* Return closest match for the given RGB color */
            index = PALETTE_ulGetNearestPaletteIndex(ppal, crColor);
        }
        // else SetLastError ?
        PALETTE_ShareUnlockPalette(ppal);
    }

    return index;
}

UINT
FASTCALL
IntGdiRealizePalette(HDC hDC)
{
    UINT realize = 0;
    PDC pdc;
    PALETTE *ppalSurf, *ppalDC;

    pdc = DC_LockDc(hDC);
    if (!pdc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    if (!pdc->dclevel.pSurface)
    {
        goto cleanup;
    }

    if (pdc->dctype == DCTYPE_DIRECT)
    {
        static BOOL g_WarnedOnce = FALSE;
        if (!g_WarnedOnce)
        {
            g_WarnedOnce = TRUE;
            UNIMPLEMENTED;
        }
        goto cleanup;
    }

    /// FIXME: shouldn't dereference pSurface while the PDEV is not locked
    ppalSurf = pdc->dclevel.pSurface->ppal;
    ppalDC = pdc->dclevel.ppal;

    if (!(ppalSurf->flFlags & PAL_INDEXED))
    {
        // FIXME: Set error?
        goto cleanup;
    }

    ASSERT(ppalDC->flFlags & PAL_INDEXED);

    DPRINT1("RealizePalette unimplemented for %s\n", 
            (pdc->dctype == DCTYPE_MEMORY ? "memory managed DCs" : "device DCs"));

cleanup:
    DC_UnlockDc(pdc);
    return realize;
}

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
        const PALETTEENTRY *pptr = PaletteColors;

        palPtr = PALETTE_ShareLockPalette(hPal);
        if (!palPtr) return FALSE;

        pal_entries = palPtr->NumColors;
        if (StartIndex >= pal_entries)
        {
            PALETTE_ShareUnlockPalette(palPtr);
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

        PALETTE_ShareUnlockPalette(palPtr);

#if 0
/* FIXME: This is completely broken! We cannot call UserGetDesktopWindow
   without first acquiring the USER lock. But the whole process here is
   screwed anyway. Instead of messing with the desktop DC, we need to
   check, whether the palette is associated with a PDEV and whether that
   PDEV supports palette operations. Then we need to call pfnDrvSetPalette.
   But since IntGdiRealizePalette() is not even implemented for direct DCs,
   we can as well just do nothing, that will at least not ASSERT!
   I leave the whole thing here, to scare people away, who want to "fix" it. */

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
#endif // 0
    }
    return ret;
}

UINT APIENTRY
IntGetPaletteEntries(
    HPALETTE hpal,
    UINT StartIndex,
    UINT  Entries,
    LPPALETTEENTRY  pe)
{
    PPALETTE palGDI;
    UINT numEntries;

    palGDI = (PPALETTE) PALETTE_ShareLockPalette(hpal);
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
            PALETTE_ShareUnlockPalette(palGDI);
            return 0;
        }
        memcpy(pe, palGDI->IndexedColors + StartIndex, Entries * sizeof(PALETTEENTRY));
    }
    else
    {
        Entries = numEntries;
    }

    PALETTE_ShareUnlockPalette(palGDI);
    return Entries;
}

UINT APIENTRY
IntGetSystemPaletteEntries(HDC  hDC,
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
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (pe != NULL)
    {
        EntriesSize = Entries * sizeof(pe[0]);
        if (Entries != EntriesSize / sizeof(pe[0]))
        {
            /* Integer overflow! */
            EngSetLastError(ERROR_INVALID_PARAMETER);
            return 0;
        }
    }

    if (!(dc = DC_LockDc(hDC)))
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    palGDI = PALETTE_ShareLockPalette(dc->dclevel.hpal);
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
        PALETTE_ShareUnlockPalette(palGDI);

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
    ULONG numEntries;

    if ((UINT_PTR)hpal & GDI_HANDLE_STOCK_MASK)
    {
    	return 0;
    }

    palGDI = PALETTE_ShareLockPalette(hpal);
    if (!palGDI) return 0;

    numEntries = palGDI->NumColors;
    if (Start >= numEntries)
    {
        PALETTE_ShareUnlockPalette(palGDI);
        return 0;
    }
    if (numEntries < Start + Entries)
    {
        Entries = numEntries - Start;
    }
    memcpy(palGDI->IndexedColors + Start, pe, Entries * sizeof(PALETTEENTRY));
    PALETTE_ShareUnlockPalette(palGDI);

    return Entries;
}

ULONG
APIENTRY
GreGetSetColorTable(
    HDC hdc,
    ULONG iStartIndex,
    ULONG cEntries,
    RGBQUAD *prgbColors,
    BOOL bSet)
{
    PDC pdc;
    PSURFACE psurf;
    PPALETTE ppal = NULL;
    ULONG i, iEndIndex, iResult = 0;

    /* Lock the DC */
    pdc = DC_LockDc(hdc);
    if (!pdc)
    {
        return 0;
    }

    /* Get the surface from the DC */
    psurf = pdc->dclevel.pSurface;

    /* Check if we have the default surface */
    if (psurf == NULL)
    {
        /* Use a mono palette */
        if (!bSet)
            ppal = gppalMono;
    }
    else if (psurf->SurfObj.iType == STYPE_BITMAP)
    {
        /* Get the palette of the surface */
        ppal = psurf->ppal;
    }

    /* Check if this is an indexed palette and the range is ok */
    if (ppal && (ppal->flFlags & PAL_INDEXED) &&
        (iStartIndex < ppal->NumColors))
    {
        /* Calculate the end of the operation */
        iEndIndex = min(iStartIndex + cEntries, ppal->NumColors);

        /* Check what operation to perform */
        if (bSet)
        {
            /* Loop all colors and set the palette entries */
            for (i = iStartIndex; i < iEndIndex; i++, prgbColors++)
            {
                ppal->IndexedColors[i].peRed = prgbColors->rgbRed;
                ppal->IndexedColors[i].peGreen = prgbColors->rgbGreen;
                ppal->IndexedColors[i].peBlue = prgbColors->rgbBlue;
            }

            /* Mark the dc brushes invalid */
            pdc->pdcattr->ulDirty_ |= DIRTY_FILL|DIRTY_LINE|
                                      DIRTY_BACKGROUND|DIRTY_TEXT;
        }
        else
        {
            /* Loop all colors and get the palette entries */
            for (i = iStartIndex; i < iEndIndex; i++, prgbColors++)
            {
                prgbColors->rgbRed = ppal->IndexedColors[i].peRed;
                prgbColors->rgbGreen = ppal->IndexedColors[i].peGreen;
                prgbColors->rgbBlue = ppal->IndexedColors[i].peBlue;
                prgbColors->rgbReserved = 0;
            }
        }

        /* Calculate how many entries were modified */
        iResult = iEndIndex - iStartIndex;
    }

    /* Unlock the DC */
    DC_UnlockDc(pdc);

    return iResult;
}

__kernel_entry
LONG
APIENTRY
NtGdiDoPalette(
    _In_ HGDIOBJ hObj,
    _In_ WORD iStart,
    _In_ WORD cEntries,
    _When_(bInbound!=0, _In_reads_bytes_(cEntries*sizeof(PALETTEENTRY)))
    _When_(bInbound==0, _Out_writes_bytes_(cEntries*sizeof(PALETTEENTRY))) LPVOID pUnsafeEntries,
    _In_ DWORD iFunc,
    _In_ BOOL bInbound)
{
	LONG ret;
	LPVOID pEntries = NULL;
	SIZE_T cjSize;

	if (pUnsafeEntries)
	{
		if (cEntries == 0)
			return 0;

		cjSize = cEntries * sizeof(PALETTEENTRY);
		pEntries = ExAllocatePoolWithTag(PagedPool, cjSize, TAG_PALETTE);
		if (!pEntries)
			return 0;

		if (bInbound)
		{
			_SEH2_TRY
			{
				ProbeForRead(pUnsafeEntries, cjSize, 1);
				memcpy(pEntries, pUnsafeEntries, cjSize);
			}
			_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
			{
				ExFreePoolWithTag(pEntries, TAG_PALETTE);
				_SEH2_YIELD(return 0);
			}
			_SEH2_END
		}
		else
		{
		    /* Zero it out, so we don't accidentally leak kernel data */
		    RtlZeroMemory(pEntries, cjSize);
		}
	}

	ret = 0;
	switch(iFunc)
	{
		case GdiPalAnimate:
			if (pEntries)
				ret = IntAnimatePalette((HPALETTE)hObj, iStart, cEntries, (CONST PPALETTEENTRY)pEntries);
			break;

		case GdiPalSetEntries:
			if (pEntries)
				ret = IntSetPaletteEntries((HPALETTE)hObj, iStart, cEntries, (CONST LPPALETTEENTRY)pEntries);
			break;

		case GdiPalGetEntries:
			ret = IntGetPaletteEntries((HPALETTE)hObj, iStart, cEntries, (LPPALETTEENTRY)pEntries);
			break;

		case GdiPalGetSystemEntries:
			ret = IntGetSystemPaletteEntries((HDC)hObj, iStart, cEntries, (LPPALETTEENTRY)pEntries);
			break;

		case GdiPalSetColorTable:
			if (pEntries)
				ret = GreGetSetColorTable((HDC)hObj, iStart, cEntries, (RGBQUAD*)pEntries, TRUE);
			break;

		case GdiPalGetColorTable:
			if (pEntries)
				ret = GreGetSetColorTable((HDC)hObj, iStart, cEntries, (RGBQUAD*)pEntries, FALSE);
			break;
	}

	if (pEntries)
	{
		if (!bInbound && (ret > 0))
		{
			cjSize = min(cEntries, ret) * sizeof(PALETTEENTRY);
			_SEH2_TRY
			{
				ProbeForWrite(pUnsafeEntries, cjSize, 1);
				memcpy(pUnsafeEntries, pEntries, cjSize);
			}
			_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
			{
				ret = 0;
			}
			_SEH2_END
		}
		ExFreePoolWithTag(pEntries, TAG_PALETTE);
	}

	return ret;
}

UINT APIENTRY
NtGdiSetSystemPaletteUse(HDC hDC, UINT Usage)
{
    UINT old = SystemPaletteUse;

    /* Device doesn't support colour palettes */
    if (!(NtGdiGetDeviceCaps(hDC, RASTERCAPS) & RC_PALETTE)) {
        return SYSPAL_ERROR;
    }

    switch (Usage)
	{
		case SYSPAL_NOSTATIC:
        case SYSPAL_NOSTATIC256:
        case SYSPAL_STATIC:
				SystemPaletteUse = Usage;
				break;

        default:
				old=SYSPAL_ERROR;
				break;
	}

 return old;
}

UINT
APIENTRY
NtGdiGetSystemPaletteUse(HDC hDC)
{
    return SystemPaletteUse;
}

BOOL
APIENTRY
NtGdiUpdateColors(HDC hDC)
{
   PWND Wnd;
   BOOL calledFromUser, ret;
   USER_REFERENCE_ENTRY Ref;

   calledFromUser = UserIsEntered();

   if (!calledFromUser){
      UserEnterExclusive();
   }

   Wnd = UserGetWindowObject(IntWindowFromDC(hDC));
   if (Wnd == NULL)
   {
      EngSetLastError(ERROR_INVALID_WINDOW_HANDLE);

      if (!calledFromUser){
         UserLeave();
      }

      return FALSE;
   }

   UserRefObjectCo(Wnd, &Ref);
   ret = co_UserRedrawWindow(Wnd, NULL, 0, RDW_INVALIDATE);
   UserDerefObjectCo(Wnd);

   if (!calledFromUser){
      UserLeave();
   }

   return ret;
}

BOOL
APIENTRY
NtGdiUnrealizeObject(HGDIOBJ hgdiobj)
{
   BOOL Ret = FALSE;
   PPALETTE palGDI;

   if ( !hgdiobj ||
        ((UINT_PTR)hgdiobj & GDI_HANDLE_STOCK_MASK) ||
        !GDI_HANDLE_IS_TYPE(hgdiobj, GDI_OBJECT_TYPE_PALETTE) )
      return Ret;

   palGDI = PALETTE_ShareLockPalette(hgdiobj);
   if (!palGDI) return FALSE;

   // FIXME!!
   // Need to do something!!!
   // Zero out Current and Old Translated pointers?
   //
   Ret = TRUE;
   PALETTE_ShareUnlockPalette(palGDI);
   return Ret;
}

__kernel_entry
HPALETTE
APIENTRY
NtGdiEngCreatePalette(
    _In_ ULONG iMode,
    _In_ ULONG cColors,
    _In_ ULONG *pulColors,
    _In_ FLONG flRed,
    _In_ FLONG flGreen,
    _In_ FLONG flBlue)
{
    HPALETTE hPal = NULL;
    ULONG *pulcSafe, ulColors[WINDDI_MAXSETPALETTECOLORS];

    if ( cColors > MAX_PALCOLORS ) return NULL;

    if ( cColors <= WINDDI_MAXSETPALETTECOLORS )
    {
        pulcSafe = ulColors;
    }
    else
    {
        pulcSafe = ExAllocatePoolWithTag(PagedPool, cColors * sizeof(ULONG), GDITAG_UMPD );
    }

        _SEH2_TRY
    {
        ProbeForRead( pulColors, cColors * sizeof(ULONG), 1);
        RtlCopyMemory( pulcSafe, pulColors, cColors * sizeof(ULONG) );
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
        if ( cColors > WINDDI_MAXSETPALETTECOLORS ) ExFreePoolWithTag( pulcSafe, GDITAG_UMPD );
        _SEH2_YIELD(return hPal);
    }
    _SEH2_END;

    hPal = EngCreatePalette( iMode/*|PAL_SETPOWNER*/, cColors, pulcSafe, flRed, flGreen, flBlue );

    if ( cColors > WINDDI_MAXSETPALETTECOLORS ) ExFreePoolWithTag( pulcSafe, GDITAG_UMPD );

    return hPal;
}

__kernel_entry
BOOL
APIENTRY
NtGdiEngDeletePalette(
    _In_ HPALETTE hPal)
{
    return EngDeletePalette(hPal);
}

/* EOF */
