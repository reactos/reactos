/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          GDI Color Translation Functions
 * FILE:             subsystems/win32/win32k/eng/xlate.c
 * PROGRAMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <win32k.h>

#include <intrin.h>

#define NDEBUG
#include <debug.h>


/** Globals *******************************************************************/

ULONG giUniqueXlate = 0;
EXLATEOBJ gexloTrivial;
XLATEOBJ* gpxloTrivial = &gexloTrivial.xlo;

const BYTE gajXlate5to8[32] =
{  0,  8, 16, 25, 33, 41, 49, 58, 66, 74, 82, 90, 99,107,115,123,
 132,140,148,156,165,173,181,189,197,206,214,222,231,239,247,255};

const BYTE gajXlate6to8[64] =
{ 0,  4,  8, 12, 16, 20, 24, 28, 32, 36, 40, 45, 49, 52, 57, 61,
 65, 69, 73, 77, 81, 85, 89, 93, 97,101,105,109,113,117,121,125,
130,134,138,142,146,150,154,158,162,166,170,174,178,182,186,190,
194,198,202,207,210,215,219,223,227,231,235,239,243,247,251,255};


/** iXlate functions **********************************************************/

ULONG
FASTCALL
EXLATEOBJ_iXlateTrivial(PEXLATEOBJ pexlo, ULONG iColor)
{
    return iColor;
}

ULONG
FASTCALL
EXLATEOBJ_iXlateToMono(PEXLATEOBJ pexlo, ULONG iColor)
{
    return (iColor == pexlo->xlo.pulXlate[0]);
}

ULONG
FASTCALL
EXLATEOBJ_iXlateTable(PEXLATEOBJ pexlo, ULONG iColor)
{
    if (iColor >= pexlo->xlo.cEntries)
    {
        iColor %= pexlo->xlo.cEntries;
    }
    return pexlo->xlo.pulXlate[iColor];
}

ULONG
FASTCALL
EXLATEOBJ_iXlateRGBtoBGR(PEXLATEOBJ pxlo, ULONG iColor)
{
    ULONG iNewColor;

    /* Copy green */
    iNewColor = iColor & 0x00ff00;

    /* Mask red and blue */
    iColor &= 0xff00ff;

    /* Shift and copy red and blue */
    iNewColor |= iColor >> 16;
    iNewColor |= iColor << 16;

    return iNewColor;
}

ULONG
FASTCALL
EXLATEOBJ_iXlateRGBto555(PEXLATEOBJ pxlo, ULONG iColor)
{
    ULONG iNewColor;

    /* Copy red */
    iColor <<= 7;
    iNewColor = iColor & 0x7C00;

    /* Copy green */
    iColor >>= 13;
    iNewColor |= iColor & 0x3E0;

    /* Copy green */
    iColor >>= 13;
    iNewColor |= iColor & 0x1F;

    return iNewColor;
}

ULONG
FASTCALL
EXLATEOBJ_iXlateBGRto555(PEXLATEOBJ pxlo, ULONG iColor)
{
    ULONG iNewColor;

    /* Copy blue */
    iColor >>= 3;
    iNewColor = iColor & 0x1f;

    /* Copy green */
    iColor >>= 3;
    iNewColor |= (iColor & 0x3E0);

    /* Copy red */
    iColor >>= 3;
    iNewColor |= (iColor & 0x7C00);

    return iNewColor;
}

ULONG
FASTCALL
EXLATEOBJ_iXlateRGBto565(PEXLATEOBJ pxlo, ULONG iColor)
{
    ULONG iNewColor;

    /* Copy red */
    iColor <<= 8;
    iNewColor = iColor & 0xF800;

    /* Copy green */
    iColor >>= 13;
    iNewColor |= iColor & 0x7E0;

    /* Copy green */
    iColor >>= 14;
    iNewColor |= iColor & 0x1F;

    return iNewColor;
}

ULONG
FASTCALL
EXLATEOBJ_iXlateBGRto565(PEXLATEOBJ pxlo, ULONG iColor)
{
    ULONG iNewColor;

    /* Copy blue */
    iColor >>= 3;
    iNewColor = iColor & 0x1f;

    /* Copy green */
    iColor >>= 2;
    iNewColor |= (iColor & 0x7E0);

    /* Copy red */
    iColor >>= 3;
    iNewColor |= (iColor & 0xF800);

    return iNewColor;
}

ULONG
FASTCALL
EXLATEOBJ_iXlateRGBtoPal(PEXLATEOBJ pexlo, ULONG iColor)
{
    return PALETTE_ulGetNearestPaletteIndex(pexlo->ppalDst, iColor);
}

ULONG
FASTCALL
EXLATEOBJ_iXlate555toRGB(PEXLATEOBJ pxlo, ULONG iColor)
{
    ULONG iNewColor;

    /* Copy blue */
    iNewColor = gajXlate5to8[iColor & 0x1F] << 16;

    /* Copy green */
    iColor >>= 5;
    iNewColor |= gajXlate5to8[iColor & 0x1F] << 8;

    /* Copy red */
    iColor >>= 5;
    iNewColor |= gajXlate5to8[iColor & 0x1F];

    return iNewColor;
}

ULONG
FASTCALL
EXLATEOBJ_iXlate555toBGR(PEXLATEOBJ pxlo, ULONG iColor)
{
    ULONG iNewColor;

    /* Copy blue */
    iNewColor = gajXlate5to8[iColor & 0x1F];

    /* Copy green */
    iColor >>= 5;
    iNewColor |= gajXlate5to8[iColor & 0x1F] << 8;

    /* Copy red */
    iColor >>= 5;
    iNewColor |= gajXlate5to8[iColor & 0x1F] << 16;

    return iNewColor;
}

ULONG
FASTCALL
EXLATEOBJ_iXlate555to565(PEXLATEOBJ pxlo, ULONG iColor)
{
    ULONG iNewColor;

    /* Copy blue */
    iNewColor = iColor & 0x1f;

    /* Copy red and green */
    iColor <<= 1;
    iNewColor |= iColor & 0xFFC0;

    /* Duplicate highest green bit */
    iColor >>= 5;
    iNewColor |= (iColor & 0x20);

    return iNewColor;
}

ULONG
FASTCALL
EXLATEOBJ_iXlate555toPal(PEXLATEOBJ pexlo, ULONG iColor)
{
    iColor = EXLATEOBJ_iXlate555toRGB(pexlo, iColor);

    return PALETTE_ulGetNearestPaletteIndex(pexlo->ppalDst, iColor);
}

ULONG
FASTCALL
EXLATEOBJ_iXlate565to555(PEXLATEOBJ pxlo, ULONG iColor)
{
    ULONG iNewColor;

    /* Copy blue */
    iNewColor = iColor & 0x1f;

    /* Copy red and green */
    iColor >>= 1;
    iNewColor |= iColor & 0x7FE0;

    return iNewColor;
}

ULONG
FASTCALL
EXLATEOBJ_iXlate565toRGB(PEXLATEOBJ pexlo, ULONG iColor)
{
    ULONG iNewColor;

    /* Copy blue */
    iNewColor = gajXlate5to8[iColor & 0x1F] << 16;

    /* Copy green */
    iColor >>= 5;
    iNewColor |= gajXlate6to8[iColor & 0x3F] << 8;

    /* Copy red */
    iColor >>= 6;
    iNewColor |= gajXlate5to8[iColor & 0x1F];

    return iNewColor;
}

ULONG
FASTCALL
EXLATEOBJ_iXlate565toBGR(PEXLATEOBJ pexlo, ULONG iColor)
{
    ULONG iNewColor;

    /* Copy blue */
    iNewColor = gajXlate5to8[iColor & 0x1F];

    /* Copy green */
    iColor >>= 5;
    iNewColor |= gajXlate6to8[iColor & 0x3F] << 8;

    /* Copy blue */
    iColor >>= 6;
    iNewColor |= gajXlate5to8[iColor & 0x1F] << 16;

    return iNewColor;
}

ULONG
FASTCALL
EXLATEOBJ_iXlate565toPal(EXLATEOBJ *pexlo, ULONG iColor)
{
    iColor = EXLATEOBJ_iXlate565toRGB(pexlo, iColor);

    return PALETTE_ulGetNearestPaletteIndex(pexlo->ppalDst, iColor);
}

ULONG
FASTCALL
EXLATEOBJ_iXlateShiftAndMask(PEXLATEOBJ pexlo, ULONG iColor)
{
    ULONG iNewColor;

    iNewColor = _rotl(iColor, pexlo->ulRedShift) & pexlo->ulRedMask;
    iNewColor |= _rotl(iColor, pexlo->ulGreenShift) & pexlo->ulGreenMask;
    iNewColor |= _rotl(iColor, pexlo->ulBlueShift) & pexlo->ulBlueMask;

    return iNewColor;
}

ULONG
FASTCALL
EXLATEOBJ_iXlateBitfieldsToPal(PEXLATEOBJ pexlo, ULONG iColor)
{
    /* Convert bitfields to RGB */
    iColor = EXLATEOBJ_iXlateShiftAndMask(pexlo, iColor);

    /* Return nearest index */
    return PALETTE_ulGetNearestPaletteIndex(pexlo->ppalDst, iColor);
}


/** Private Functions *********************************************************/

VOID
NTAPI
EXLATEOBJ_vInitTrivial(PEXLATEOBJ pexlo)
{
    pexlo->xlo.iUniq = InterlockedIncrement((LONG*)&giUniqueXlate);
    pexlo->xlo.flXlate = XO_TRIVIAL;
    pexlo->xlo.iSrcType = PAL_RGB;
    pexlo->xlo.iDstType = PAL_RGB;
    pexlo->xlo.cEntries = 0;
    pexlo->xlo.pulXlate = pexlo->aulXlate;
    pexlo->pfnXlate = EXLATEOBJ_iXlateTrivial;
    pexlo->ppalSrc = &gpalRGB;
    pexlo->ppalDst = &gpalRGB;
    pexlo->ppalDstDc = &gpalRGB;
    pexlo->hColorTransform = NULL;
}

VOID
NTAPI
EXLATEOBJ_vInitialize(
    PEXLATEOBJ pexlo,
    PALETTE *ppalSrc,
    PALETTE *ppalDst,
    COLORREF crSrcBackColor,
    COLORREF crDstBackColor,
    COLORREF crDstForeColor)
{
    ULONG cEntries;
    ULONG i, ulColor;

    EXLATEOBJ_vInitTrivial(pexlo);

    if (ppalDst == ppalSrc || !ppalSrc || !ppalDst ||
        ((ppalDst->Mode == PAL_RGB || ppalDst->Mode == PAL_BGR) &&
         ppalDst->Mode == ppalSrc->Mode))
    {
        return;
    }

    pexlo->ppalSrc = ppalSrc;
    pexlo->ppalDst = ppalDst;
    pexlo->xlo.iSrcType = ppalSrc->Mode;
    pexlo->xlo.iDstType = ppalDst->Mode;

    /* Chack if both of the pallettes are indexed */
    if (!(ppalSrc->Mode & PAL_INDEXED) || !(ppalDst->Mode & PAL_INDEXED))
    {
        /* At least one palette is not indexed, calculate shifts/masks */
        ULONG aulMasksSrc[3], aulMasksDst[3];

        PALETTE_vGetBitMasks(ppalSrc, aulMasksSrc);
        PALETTE_vGetBitMasks(ppalDst, aulMasksDst);

        pexlo->ulRedMask = aulMasksDst[0];
        pexlo->ulGreenMask = aulMasksDst[1];
        pexlo->ulBlueMask = aulMasksDst[2];

        pexlo->ulRedShift = CalculateShift(aulMasksSrc[0], aulMasksDst[0]);
        pexlo->ulGreenShift = CalculateShift(aulMasksSrc[1], aulMasksDst[1]);
        pexlo->ulBlueShift = CalculateShift(aulMasksSrc[2], aulMasksDst[2]);
    }

    if (ppalSrc->Mode & PAL_MONOCHROME)
    {
        /* This is a monochrome palette */
        if (!(ppalDst->Mode & PAL_MONOCHROME))
        {
            /* Mono to color, use the dest DC's fore and back color */
            pexlo->pfnXlate = EXLATEOBJ_iXlateTable;
            pexlo->xlo.flXlate |= XO_TABLE;
            pexlo->xlo.cEntries = 2;
            pexlo->xlo.pulXlate[0] =
                PALETTE_ulGetNearestIndex(ppalDst, crDstForeColor);
            pexlo->xlo.pulXlate[1] =
                PALETTE_ulGetNearestIndex(ppalDst, crDstBackColor);
        }
    }
    else if (ppalDst->Mode & PAL_MONOCHROME)
    {
        pexlo->pfnXlate = EXLATEOBJ_iXlateToMono;
        pexlo->xlo.flXlate |= XO_TO_MONO;
        pexlo->xlo.cEntries = 1;

        if (ppalSrc->Mode & PAL_INDEXED)
        {
            pexlo->aulXlate[0] =
                PALETTE_ulGetNearestPaletteIndex(ppalSrc, crSrcBackColor);
        }
        else if (ppalSrc->Mode & PAL_BGR)
        {
            pexlo->aulXlate[0] = crSrcBackColor;
        }
        else if (ppalSrc->Mode & PAL_RGB)
        {
            pexlo->aulXlate[0] = RGB(GetBValue(crSrcBackColor),
                                     GetGValue(crSrcBackColor),
                                     GetRValue(crSrcBackColor));
        }
        else if (ppalSrc->Mode & PAL_BITFIELDS)
        {
            PALETTE_vGetBitMasks(ppalSrc, &pexlo->ulRedMask);
            pexlo->ulRedShift = CalculateShift(0xFF, pexlo->ulRedMask);
            pexlo->ulGreenShift = CalculateShift(0xFF00, pexlo->ulGreenMask);
            pexlo->ulBlueShift = CalculateShift(0xFF0000, pexlo->ulBlueMask);

            pexlo->aulXlate[0] = EXLATEOBJ_iXlateShiftAndMask(pexlo, crSrcBackColor);
        }
    }
    else if (ppalSrc->Mode & PAL_INDEXED)
    {
        cEntries = ppalSrc->NumColors;

        /* Allocate buffer if needed */
        if (cEntries > 6)
        {
            pexlo->xlo.pulXlate = EngAllocMem(0,
                                              cEntries * sizeof(ULONG),
                                              TAG_XLATEOBJ);
            if (!pexlo->xlo.pulXlate)
            {
                DPRINT1("Could not allocate pulXlate buffer.\n");
                pexlo->pfnXlate = EXLATEOBJ_iXlateTrivial;
                pexlo->xlo.flXlate = XO_TRIVIAL;
                return;
            }
        }
        pexlo->xlo.cEntries = cEntries;

        pexlo->pfnXlate = EXLATEOBJ_iXlateTable;
        if (ppalDst->Mode & PAL_INDEXED)
        {
            pexlo->xlo.flXlate |= XO_TABLE;

            for (i = 0; i < cEntries; i++)
            {
                ulColor = RGB(ppalSrc->IndexedColors[i].peRed,
                              ppalSrc->IndexedColors[i].peGreen,
                              ppalSrc->IndexedColors[i].peBlue);

                pexlo->xlo.pulXlate[i] =
                    PALETTE_ulGetNearestPaletteIndex(ppalDst, ulColor);

                if (pexlo->xlo.pulXlate[i] != i)
                    pexlo->xlo.flXlate &= ~XO_TRIVIAL;
            }

            if (pexlo->xlo.flXlate & XO_TRIVIAL)
            {
                if (pexlo->xlo.pulXlate != pexlo->aulXlate)
                {
                    EngFreeMem(pexlo->xlo.pulXlate);
                    pexlo->xlo.pulXlate = pexlo->aulXlate;
                }
                pexlo->pfnXlate = EXLATEOBJ_iXlateTrivial;
                pexlo->xlo.flXlate = XO_TRIVIAL;
                return;
            }
        }
        else
        {
            // FIXME: use PALETTE_ulGetNearest
            EXLATEOBJ exloTmp = *pexlo;
            exloTmp.xlo.pulXlate = exloTmp.aulXlate;

            pexlo->xlo.flXlate |= XO_TABLE;
            for (i = 0; i < pexlo->xlo.cEntries; i++)
            {
                ulColor = RGB(ppalSrc->IndexedColors[i].peRed,
                              ppalSrc->IndexedColors[i].peGreen,
                              ppalSrc->IndexedColors[i].peBlue);
                pexlo->xlo.pulXlate[i] =
                    EXLATEOBJ_iXlateShiftAndMask(&exloTmp, ulColor);
            }
        }
    }
    else if (ppalSrc->Mode & PAL_RGB)
    {
        if (ppalDst->Mode & PAL_INDEXED)
            pexlo->pfnXlate = EXLATEOBJ_iXlateRGBtoPal;

        else if (ppalDst->Mode & PAL_BGR)
            pexlo->pfnXlate = EXLATEOBJ_iXlateRGBtoBGR;

        else if (ppalDst->Mode & PAL_RGB16_555)
            pexlo->pfnXlate = EXLATEOBJ_iXlateRGBto555;

        else if (ppalDst->Mode & PAL_RGB16_565)
            pexlo->pfnXlate = EXLATEOBJ_iXlateRGBto565;

        else if (ppalDst->Mode & PAL_BITFIELDS)
            pexlo->pfnXlate = EXLATEOBJ_iXlateShiftAndMask;
    }
    else if (ppalSrc->Mode & PAL_BGR)
    {
        if (ppalDst->Mode & PAL_INDEXED)
            pexlo->pfnXlate = EXLATEOBJ_iXlateBitfieldsToPal;

        else if (ppalDst->Mode & PAL_RGB)
            /* The inverse function works the same */
            pexlo->pfnXlate = EXLATEOBJ_iXlateRGBtoBGR;

        else if (ppalDst->Mode & PAL_RGB16_555)
            pexlo->pfnXlate = EXLATEOBJ_iXlateBGRto555;

        else if (ppalDst->Mode & PAL_RGB16_565)
            pexlo->pfnXlate = EXLATEOBJ_iXlateBGRto565;

        else if (ppalDst->Mode & PAL_BITFIELDS)
            pexlo->pfnXlate = EXLATEOBJ_iXlateShiftAndMask;
    }
    else if (ppalSrc->Mode & PAL_RGB16_555)
    {
        if (ppalDst->Mode & PAL_INDEXED)
            pexlo->pfnXlate = EXLATEOBJ_iXlate555toPal;

        else if (ppalDst->Mode & PAL_RGB)
            pexlo->pfnXlate = EXLATEOBJ_iXlate555toRGB;

        else if (ppalDst->Mode & PAL_BGR)
            pexlo->pfnXlate = EXLATEOBJ_iXlate555toBGR;

        else if (ppalDst->Mode & PAL_RGB16_565)
            pexlo->pfnXlate = EXLATEOBJ_iXlate555to565;

        else if (ppalDst->Mode & PAL_BITFIELDS)
            pexlo->pfnXlate = EXLATEOBJ_iXlateShiftAndMask;
    }
    else if (ppalSrc->Mode & PAL_RGB16_565)
    {
        if (ppalDst->Mode & PAL_INDEXED)
            pexlo->pfnXlate = EXLATEOBJ_iXlate565toPal;

        else if (ppalDst->Mode & PAL_RGB)
            pexlo->pfnXlate = EXLATEOBJ_iXlate565toRGB;

        else if (ppalDst->Mode & PAL_BGR)
            pexlo->pfnXlate = EXLATEOBJ_iXlate565toBGR;

        else if (ppalDst->Mode & PAL_RGB16_555)
            pexlo->pfnXlate = EXLATEOBJ_iXlate565to555;

        else if (ppalDst->Mode & PAL_BITFIELDS)
            pexlo->pfnXlate = EXLATEOBJ_iXlateShiftAndMask;
    }
    else if (ppalSrc->Mode & PAL_BITFIELDS)
    {
        if (ppalDst->Mode & PAL_INDEXED)
            pexlo->pfnXlate = EXLATEOBJ_iXlateBitfieldsToPal;
        else
            pexlo->pfnXlate = EXLATEOBJ_iXlateShiftAndMask;
    }

    /* Check for a trivial shift and mask operation */
    if (pexlo->pfnXlate == EXLATEOBJ_iXlateShiftAndMask &&
        !pexlo->ulRedShift && !pexlo->ulGreenShift && !pexlo->ulBlueShift)
    {
            pexlo->pfnXlate = EXLATEOBJ_iXlateTrivial;
    }

    /* Check for trivial xlate */
    if (pexlo->pfnXlate == EXLATEOBJ_iXlateTrivial)
        pexlo->xlo.flXlate = XO_TRIVIAL;
    else
        pexlo->xlo.flXlate &= ~XO_TRIVIAL;
}

VOID
NTAPI
EXLATEOBJ_vInitXlateFromDCs(
    EXLATEOBJ* pexlo,
    PDC pdcSrc,
    PDC pdcDst)
{
    PSURFACE psurfDst, psurfSrc;
    HPALETTE hpalSrc, hpalDst;
    PPALETTE ppalSrc, ppalDst, ppalDstDc;

    DPRINT("Enter EXLATEOBJ_vInitXlateFromDCs\n");

    /* Do basic init */
    EXLATEOBJ_vInitTrivial(pexlo);

    psurfDst = pdcDst->dclevel.pSurface;
    psurfSrc = pdcSrc->dclevel.pSurface;

    if (psurfDst == psurfSrc)
    {
        return;
    }

    hpalSrc = psurfSrc->hDIBPalette;
    if (!hpalSrc) 
        hpalSrc = pPrimarySurface->devinfo.hpalDefault;

    ppalSrc = PALETTE_ShareLockPalette(hpalSrc);
    if (!ppalSrc)
        return;

    hpalDst = psurfDst->hDIBPalette;
    if (!hpalDst) hpalDst = pPrimarySurface->devinfo.hpalDefault;

    ppalDst = PALETTE_ShareLockPalette(hpalDst);
    if (!ppalDst)
    {
        PALETTE_ShareUnlockPalette(ppalSrc);
        return;
    }

    ppalDstDc = pdcDst->dclevel.ppal;
    ASSERT(ppalDstDc);

    /* KB41464 details how to convert between mono and color */
    if (psurfDst->SurfObj.iBitmapFormat == BMF_1BPP)
    {
        if (psurfSrc->SurfObj.iBitmapFormat != BMF_1BPP)
        {
            // HACK!! FIXME: 1bpp DDBs should have gpalMono already!
            EXLATEOBJ_vInitialize(pexlo,
                                  ppalSrc,
                                  &gpalMono,
                                  pdcSrc->pdcattr->crBackgroundClr,
                                  pdcDst->pdcattr->crBackgroundClr,
                                  pdcDst->pdcattr->crForegroundClr);
        }
    }
    else if (psurfSrc->SurfObj.iBitmapFormat == BMF_1BPP && !psurfSrc->hSecure)
    {
        // HACK!! FIXME: 1bpp DDBs should have gpalMono already!
        EXLATEOBJ_vInitialize(pexlo,
                              &gpalMono,
                              ppalDst,
                              0,
                              pdcDst->pdcattr->crBackgroundClr,
                              pdcDst->pdcattr->crForegroundClr);
    }
    else
    {
        EXLATEOBJ_vInitialize(pexlo, ppalSrc, ppalDst, 0, 0, 0);
    }

    PALETTE_ShareUnlockPalette(ppalDst);
    PALETTE_ShareUnlockPalette(ppalSrc);
}


VOID
NTAPI
EXLATEOBJ_vInitBrushXlate(
    PEXLATEOBJ pexlo,
    BRUSH *pbrush,
    SURFACE *psurfDst,
    COLORREF crForegroundClr,
    COLORREF crBackgroundClr)
{
    HPALETTE hpalDst = NULL;
    PPALETTE ppalDst, ppalPattern;
    SURFACE *psurfPattern;

    ASSERT(pexlo);
    ASSERT(pbrush);
    ASSERT(psurfDst);
    ASSERT(!(pbrush->flAttrs & (GDIBRUSH_IS_SOLID | GDIBRUSH_IS_NULL)));

    EXLATEOBJ_vInitTrivial(pexlo);

    hpalDst = psurfDst->hDIBPalette;
    if (!hpalDst) hpalDst = pPrimarySurface->devinfo.hpalDefault;
    ppalDst = PALETTE_ShareLockPalette(hpalDst);
    if (!ppalDst)
    {
        DPRINT1("No ppalDst!\n");
        return;
    }

    psurfPattern = SURFACE_ShareLockSurface(pbrush->hbmPattern);
    if (!psurfPattern)
    {
        PALETTE_ShareUnlockPalette(ppalDst);
        return;
    }

#if 0
    if (psurfDst->SurfObj.iBitmapFormat == BMF_1BPP)
    {
        if (psurfSrc->SurfObj.iBitmapFormat != BMF_1BPP)
        {
            // HACK!! FIXME: 1bpp DDBs should have gpalMono already!
            EXLATEOBJ_vInitialize(pexlo,
                                  ppalSrc,
                                  &gpalMono,
                                  0,
                                  crBackgroundClr,
                                  crForegroundClr);
        }
    }
    else
#endif
    if (psurfPattern->SurfObj.iBitmapFormat == BMF_1BPP &&
        !(pbrush->flAttrs & GDIBRUSH_IS_DIB))
    {
        /* Special case: 1 bpp pattern, not a DIB brush. */
        if (psurfDst->SurfObj.iBitmapFormat != BMF_1BPP)
        {
            // HACK!! FIXME: 1bpp DDBs should have gpalMono already!
            EXLATEOBJ_vInitialize(pexlo,
                                  &gpalMono,
                                  ppalDst,
                                  0,
                                  crBackgroundClr,
                                  crForegroundClr);
        }
    }
    else
    {
        /* Default: use the patterns' palette */
        ppalPattern = PALETTE_LockPalette(psurfPattern->hDIBPalette);
        if (ppalPattern)
        {
            EXLATEOBJ_vInitialize(pexlo, &gpalRGB, ppalDst, 0, 0, 0);
            PALETTE_UnlockPalette(ppalPattern);
        }
    }

    PALETTE_ShareUnlockPalette(ppalDst);
    SURFACE_ShareUnlockSurface(psurfPattern);
}

VOID
NTAPI
EXLATEOBJ_vCleanup(PEXLATEOBJ pexlo)
{
    if (pexlo->xlo.pulXlate != pexlo->aulXlate)
    {
        EngFreeMem(pexlo->xlo.pulXlate);
    }
    pexlo->xlo.pulXlate = pexlo->aulXlate;
}

VOID
InitXlateImpl(VOID)
{

    EXLATEOBJ_vInitTrivial(&gexloTrivial);
}


/** Public DDI Functions ******************************************************/

#undef XLATEOBJ_iXlate
ULONG
NTAPI
XLATEOBJ_iXlate(XLATEOBJ *pxlo, ULONG iColor)
{
    PEXLATEOBJ pexlo = (PEXLATEOBJ)pxlo;

    if (!pxlo)
        return iColor;

    /* Call the iXlate function */
    return pexlo->pfnXlate(pexlo, iColor);
}

ULONG
NTAPI
XLATEOBJ_cGetPalette(XLATEOBJ *pxlo, ULONG iPal, ULONG cPal, ULONG *pPalOut)
{
    PEXLATEOBJ pexlo = (PEXLATEOBJ)pxlo;
    PPALETTE ppal;
    INT i;

    if (!pxlo)
    {
        return 0;
    }

    if (iPal > 5)
    {
       DPRINT1("XLATEOBJ_cGetPalette called with wrong iPal: %d\n", iPal);
       return 0;
    }

    /* Get the requested palette */
    if (iPal == XO_DESTDCPALETTE)
    {
        ppal = pexlo->ppalDstDc;
    }
    else if (iPal == XO_SRCPALETTE || iPal == XO_SRCBITFIELDS)
    {
        ppal = pexlo->ppalSrc;
    }
    else
    {
        ppal = pexlo->ppalDst;
    }

    /* Verify palette type match */
    if (!ppal ||
        ((iPal == XO_SRCPALETTE || iPal == XO_DESTPALETTE)
            && !(ppal->Mode & PAL_INDEXED)) ||
        ((iPal == XO_SRCBITFIELDS || iPal == XO_DESTBITFIELDS)
            && !(ppal->Mode & PAL_BITFIELDS)))
    {
        return 0;
    }

    if(!pPalOut)
    {
        return ppal->NumColors;
    }

    /* Copy the values into the buffer */
    if (ppal->Mode & PAL_INDEXED)
    {
        cPal = min(cPal, ppal->NumColors);
        for (i = 0; i < cPal; i++)
        {
            pPalOut[i] = RGB(ppal->IndexedColors[i].peRed,
                             ppal->IndexedColors[i].peGreen,
                             ppal->IndexedColors[i].peBlue);
        }
    }
    else
    {
        // FIXME: should use the above code
        pPalOut[0] = ppal->RedMask;
        pPalOut[1] = ppal->GreenMask;
        pPalOut[2] = ppal->BlueMask;
    }

    return cPal;
}

HANDLE
NTAPI
XLATEOBJ_hGetColorTransform(XLATEOBJ *pxlo)
{
    PEXLATEOBJ pexlo = (PEXLATEOBJ)pxlo;
    return pexlo->hColorTransform;
}

PULONG
NTAPI
XLATEOBJ_piVector(XLATEOBJ *pxlo)
{
    if (pxlo->iSrcType == PAL_INDEXED)
    {
        return pxlo->pulXlate;
    }

    return NULL;
}

/* EOF */
