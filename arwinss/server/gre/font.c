/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         LGPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/gre/font.c
 * PURPOSE:         ReactOS font support routines
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

static void SharpGlyphMono(PDC physDev, INT x, INT y,
                           void *bitmap, GlyphInfo *gi, EBRUSHOBJ *pTextBrush)
{
#if 1
    unsigned char   *srcLine = bitmap, *src;
    unsigned char   bits, bitsMask;
    int             width = gi->width;
    int             stride = ((width + 31) & ~31) >> 3;
    int             height = gi->height;
    int             w;
    int             xspan, lenspan;
    RECTL           rcBounds;

    x -= gi->x;
    y -= gi->y;
    while (height--)
    {
        src = srcLine;
        srcLine += stride;
        w = width;
        
        bitsMask = 0x80;    /* FreeType is always MSB first */
        bits = *src++;
        
        xspan = x;
        while (w)
        {
            if (bits & bitsMask)
            {
                lenspan = 0;
                do
                {
                    lenspan++;
                    if (lenspan == w)
                        break;
                    bitsMask = bitsMask >> 1;
                    if (!bitsMask)
                    {
                        bits = *src++;
                        bitsMask = 0x80;
                    }
                } while (bits & bitsMask);
                rcBounds.left = xspan; rcBounds.top = y;
                rcBounds.right = xspan+lenspan; rcBounds.bottom = y+1;
                GreLineTo(&physDev->dclevel.pSurface->SurfObj,
                    physDev->CombinedClip,
                    &pTextBrush->BrushObject,
                    xspan,
                    y,
                    xspan + lenspan,
                    y,
                    &rcBounds,
                    0);
                xspan += lenspan;
                w -= lenspan;
            }
            else
            {
                do
                {
                    w--;
                    xspan++;
                    if (!w)
                        break;
                    bitsMask = bitsMask >> 1;
                    if (!bitsMask)
                    {
                        bits = *src++;
                        bitsMask = 0x80;
                    }
                } while (!(bits & bitsMask));
            }
        }
        y++;
    }
#else
    HBITMAP hBmp;
    SIZEL szSize;
    SURFOBJ *pSurfObj;
    POINT ptBrush, ptSrc;
    RECTL rcTarget;

    szSize.cx = gi->width;
    szSize.cy = gi->height;

    hBmp = GreCreateBitmap(szSize, ((gi->width + 31) & ~31) >> 3, BMF_1BPP, 0, bitmap);

    pSurfObj = EngLockSurface((HSURF)hBmp);

    ptBrush.x = 0; ptBrush.y = 0;
    ptSrc.x = 0; ptSrc.y = 0;
    rcTarget.left = x; rcTarget.top = y;
    rcTarget.right = x + szSize.cx; rcTarget.bottom = y + szSize.cy;

    GrepBitBltEx(
        &physDev->dclevel.pSurface->SurfObj,
        pSurfObj,
        NULL,
        NULL,
        NULL,
        &rcTarget,
        &ptSrc,
        NULL,
        &physDev->dclevel.pbrLine->BrushObject,
        &ptBrush,
        ROP3_TO_ROP4(SRCCOPY),
        TRUE);

    EngUnlockSurface(pSurfObj);

    GreDeleteObject(hBmp);
#endif
}

void SharpGlyphGray(PDC physDev, INT x, INT y,
                    void *bitmap, GlyphInfo *gi, EBRUSHOBJ *pTextBrush)
{
    unsigned char   *srcLine = bitmap, *src, bits;
    int             width = gi->width;
    int             stride = ((width + 3) & ~3);
    int             height = gi->height;
    int             w;
    int             xspan, lenspan;
    RECTL           rcBounds;

    x -= gi->x;
    y -= gi->y;
    while (height--)
    {
        src = srcLine;
        srcLine += stride;
        w = width;
        
        bits = *src++;
        xspan = x;
        while (w)
        {
            if (bits >= 0x80)
            {
                lenspan = 0;
                do
                {
                    lenspan++;
                    if (lenspan == w)
                        break;
                    bits = *src++;
                } while (bits >= 0x80);
                rcBounds.left = xspan; rcBounds.top = y;
                rcBounds.right = xspan+lenspan; rcBounds.bottom = y+1;
                GreLineTo(&physDev->dclevel.pSurface->SurfObj,
                    physDev->CombinedClip,
                    &pTextBrush->BrushObject,
                    xspan,
                    y,
                    xspan + lenspan,
                    y,
                    &rcBounds,
                    0);
                xspan += lenspan;
                w -= lenspan;
            }
            else
            {
                do
                {
                    w--;
                    xspan++;
                    if (!w)
                        break;
                    bits = *src++;
                } while (bits < 0x80);
            }
        }
        y++;
    }
}

static void SmoothGlyphGray(PDC physDev, INT x, INT y,
                            void *bitmap, GlyphInfo *gi, EBRUSHOBJ *pTextBrush)
{
    unsigned char   *srcLine = bitmap, *src, bits;
    int             width = gi->width;
    int             stride = ((width + 3) & ~3);
    int             height = gi->height;
    int             w,h;
    RECTL           rcBounds;
    COLORREF        srcColor;
    BYTE            rVal, gVal, bVal;
    PFN_DIB_PutPixel DibPutPixel;
    PFN_DIB_GetPixel DibGetPixel;
    SIZEL           slSize;
    HBITMAP         charBitmap;
    PSURFACE        pCharSurf;
    POINTL          BrushOrigin = {0,0};
    POINTL          SourcePoint;
    BOOLEAN         bRet;
    HPALETTE        hDestPalette;
    PPALETTE        ppalDst;
    EXLATEOBJ       exloRGB2Dst, exloDst2RGB;
    ULONG           xlBrushColor;

    /* Create a 24bpp bitmap for the glyph + background */
    slSize.cx = gi->width;
    slSize.cy = gi->height;
    if (height == 0) return;
    charBitmap = GreCreateBitmap(slSize, 0, BMF_24BPP, 0, NULL);

    /* Get the object pointer */
    pCharSurf = SURFACE_LockSurface(charBitmap);

    /* Create XLATE objects */
    /* (the following 3 lines is the old way of doing that,
       until yarotows is merged) */
    hDestPalette = physDev->dclevel.pSurface->hDIBPalette;
    if (!hDestPalette) hDestPalette = pPrimarySurface->devinfo.hpalDefault;
    ppalDst = PALETTE_LockPalette(hDestPalette);

    EXLATEOBJ_vInitialize(&exloRGB2Dst, &gpalRGB, /*psurf->ppal*/ppalDst, 0, 0, 0);
    EXLATEOBJ_vInitialize(&exloDst2RGB, /*psurf->ppal*/ppalDst, &gpalRGB, 0, 0, 0);

    PALETTE_UnlockPalette(ppalDst);

    /* Translate the brush color */
    xlBrushColor = XLATEOBJ_iXlate(&exloDst2RGB.xlo, pTextBrush->BrushObject.iSolidColor);

    /* Get pixel routines address */
    DibPutPixel = DibFunctionsForBitmapFormat[pCharSurf->SurfObj.iBitmapFormat].DIB_PutPixel;
    DibGetPixel = DibFunctionsForBitmapFormat[pCharSurf->SurfObj.iBitmapFormat].DIB_GetPixel;

    /* Blit background into this bitmap */
    rcBounds.left = 0; rcBounds.top = 0;
    rcBounds.right = width; rcBounds.bottom = height;

    x -= gi->x;
    y -= gi->y;

    SourcePoint.x = x; SourcePoint.y = y;

    bRet = GrepBitBltEx(
        &pCharSurf->SurfObj,
        &physDev->dclevel.pSurface->SurfObj,
        NULL,
        NULL,
        &exloDst2RGB.xlo,
        &rcBounds,
        &SourcePoint,
        NULL,
        &physDev->eboFill.BrushObject,
        &BrushOrigin,
        ROP3_TO_ROP4(SRCCOPY),
        TRUE);

    for (h=0; h<height; h++)
    {
        src = srcLine;
        srcLine += stride;

        for (w=0;w<width;w++)
        {
            bits = *src++;

            if (bits == 0xff)
            {
                DibPutPixel(&pCharSurf->SurfObj, w, h, xlBrushColor);
            }
            else
            {
                srcColor = DibGetPixel(&pCharSurf->SurfObj, w, h);
                rVal = ((UCHAR)~bits * (USHORT)GetRValue(srcColor) + bits * (USHORT)GetRValue(xlBrushColor)) >> 8;
                gVal = ((UCHAR)~bits * (USHORT)GetGValue(srcColor) + bits * (USHORT)GetGValue(xlBrushColor)) >> 8;
                bVal = ((UCHAR)~bits * (USHORT)GetBValue(srcColor) + bits * (USHORT)GetBValue(xlBrushColor)) >> 8;
                DibPutPixel(&pCharSurf->SurfObj, w, h, RGB(rVal, gVal, bVal));
            }
        }
    }

    /* Blit the modified bitmap back */
    rcBounds.left = x; rcBounds.top = y;
    rcBounds.right = rcBounds.left + width; rcBounds.bottom = rcBounds.top + height;

    SourcePoint.x = 0; SourcePoint.y = 0;

    bRet = GrepBitBltEx(
        &physDev->dclevel.pSurface->SurfObj,
        &pCharSurf->SurfObj,
        NULL,
        physDev->CombinedClip,
        &exloRGB2Dst.xlo,
        &rcBounds,
        &SourcePoint,
        NULL,
        &physDev->eboFill.BrushObject,
        &BrushOrigin,
        ROP3_TO_ROP4(SRCCOPY),
        TRUE);

    SURFACE_UnlockSurface(pCharSurf);

    /* Cleanup exlate objects */
    EXLATEOBJ_vCleanup(&exloRGB2Dst);
    EXLATEOBJ_vCleanup(&exloDst2RGB);

    /* Release the surface and delete the bitmap */
    GreDeleteObject(charBitmap);

    UNREFERENCED_LOCAL_VARIABLE(bRet);
}

static void SmoothGlyphColor(PDC physDev, INT x, INT y,
                            void *bitmap, GlyphInfo *gi, EBRUSHOBJ *pTextBrush)
{
    unsigned char   *srcLine = bitmap, *src, bits_r, bits_g, bits_b;
    int             width = gi->width;
    int             stride = width * 4;
    int             height = gi->height;
    int             w,h;
    RECTL           rcBounds;
    COLORREF        srcColor;
    BYTE            rVal, gVal, bVal;
    PFN_DIB_PutPixel DibPutPixel;
    PFN_DIB_GetPixel DibGetPixel;
    SIZEL           slSize;
    HBITMAP         charBitmap;
    PSURFACE        pCharSurf;
    POINTL          BrushOrigin = {0,0};
    POINTL          SourcePoint;
    BOOLEAN         bRet;
    HPALETTE        hDestPalette;
    PPALETTE        ppalDst;
    EXLATEOBJ       exloRGB2Dst, exloDst2RGB;
    ULONG           xlBrushColor;

    /* Create a 24bpp bitmap for the glyph + background */
    slSize.cx = gi->width;
    slSize.cy = gi->height;
    if (height == 0) return;
    charBitmap = GreCreateBitmap(slSize, 0, BMF_24BPP, 0, NULL);

    /* Get the object pointer */
    pCharSurf = SURFACE_LockSurface(charBitmap);

    /* Create XLATE objects */
    /* (the following 3 lines is the old way of doing that,
       until yarotows is merged) */
    hDestPalette = physDev->dclevel.pSurface->hDIBPalette;
    if (!hDestPalette) hDestPalette = pPrimarySurface->devinfo.hpalDefault;
    ppalDst = PALETTE_LockPalette(hDestPalette);

    EXLATEOBJ_vInitialize(&exloRGB2Dst, &gpalRGB, /*psurf->ppal*/ppalDst, 0, 0, 0);
    EXLATEOBJ_vInitialize(&exloDst2RGB, /*psurf->ppal*/ppalDst, &gpalRGB, 0, 0, 0);

    PALETTE_UnlockPalette(ppalDst);

    /* Translate the brush color */
    xlBrushColor = XLATEOBJ_iXlate(&exloDst2RGB.xlo, pTextBrush->BrushObject.iSolidColor);

    /* Get pixel routines address */
    DibPutPixel = DibFunctionsForBitmapFormat[pCharSurf->SurfObj.iBitmapFormat].DIB_PutPixel;
    DibGetPixel = DibFunctionsForBitmapFormat[pCharSurf->SurfObj.iBitmapFormat].DIB_GetPixel;

    /* Blit background into this bitmap */
    rcBounds.left = 0; rcBounds.top = 0;
    rcBounds.right = width; rcBounds.bottom = height;

    x -= gi->x;
    y -= gi->y;

    SourcePoint.x = x; SourcePoint.y = y;

    bRet = GrepBitBltEx(
        &pCharSurf->SurfObj,
        &physDev->dclevel.pSurface->SurfObj,
        NULL,
        NULL,
        &exloDst2RGB.xlo,
        &rcBounds,
        &SourcePoint,
        NULL,
        &physDev->eboFill.BrushObject,
        &BrushOrigin,
        ROP3_TO_ROP4(SRCCOPY),
        TRUE);

    for (h=0; h<height; h++)
    {
        src = srcLine;
        srcLine += stride;
        bits_r = *src++;
        bits_g = *src++;
        bits_b = *src++;
        src++;

        for (w=0;w<width;w++)
        {
            if (bits_r == 0xff &&
                bits_g == 0xff &&
                bits_b == 0xff)
            {
                DibPutPixel(&pCharSurf->SurfObj, w, h, xlBrushColor);
            }
            else
            {
                srcColor = DibGetPixel(&pCharSurf->SurfObj, w, h);
                rVal = ((UCHAR)~bits_r * (USHORT)GetRValue(srcColor) + bits_r * (USHORT)GetRValue(xlBrushColor)) >> 8;
                gVal = ((UCHAR)~bits_g * (USHORT)GetGValue(srcColor) + bits_g * (USHORT)GetGValue(xlBrushColor)) >> 8;
                bVal = ((UCHAR)~bits_b * (USHORT)GetBValue(srcColor) + bits_b * (USHORT)GetBValue(xlBrushColor)) >> 8;
                DibPutPixel(&pCharSurf->SurfObj, w, h, RGB(rVal, gVal, bVal));
            }

            bits_r = *src++;
            bits_g = *src++;
            bits_b = *src++;
            src++;
        }
    }

    /* Blit the modified bitmap back */
    rcBounds.left = x; rcBounds.top = y;
    rcBounds.right = rcBounds.left + width; rcBounds.bottom = rcBounds.top + height;

    SourcePoint.x = 0; SourcePoint.y = 0;

    bRet = GrepBitBltEx(
        &physDev->dclevel.pSurface->SurfObj,
        &pCharSurf->SurfObj,
        NULL,
        physDev->CombinedClip,
        &exloRGB2Dst.xlo,
        &rcBounds,
        &SourcePoint,
        NULL,
        &physDev->eboFill.BrushObject,
        &BrushOrigin,
        ROP3_TO_ROP4(SRCCOPY),
        TRUE);

    SURFACE_UnlockSurface(pCharSurf);

    /* Cleanup exlate objects */
    EXLATEOBJ_vCleanup(&exloRGB2Dst);
    EXLATEOBJ_vCleanup(&exloDst2RGB);

    /* Release the surface and delete the bitmap */
    GreDeleteObject(charBitmap);

    UNREFERENCED_LOCAL_VARIABLE(bRet);
}

/* PUBLIC FUNCTIONS **********************************************************/

VOID NTAPI
GreTextOut(PDC pDC, INT x, INT y, UINT flags,
           const RECT *lprect, LPCWSTR wstr, UINT count,
           const INT *lpDx, gsCacheEntryFormat *formatEntry,
           AA_Type aa_type)
{
    POINT offset = {0, 0};
    INT idx;
    EBRUSHOBJ pTextPen;
    PBRUSH ppen;
    HPEN hpen;
    void (* sharp_glyph_fn)(PDC, INT, INT, void *, GlyphInfo *, EBRUSHOBJ *);

    /* Create pen for text output */
    hpen  = GreCreatePen(PS_SOLID, 1, pDC->crForegroundClr, NULL);
    if(!hpen)
        return;
    ppen = PEN_ShareLockPen(hpen); 
    if(!ppen)
        return;
    EBRUSHOBJ_vInit(&pTextPen, ppen, pDC);

    if(aa_type == AA_None)
        sharp_glyph_fn = SharpGlyphMono;
    else if(aa_type == AA_Grey)
        sharp_glyph_fn = SmoothGlyphGray;
    else
        sharp_glyph_fn = SmoothGlyphColor;

    for(idx = 0; idx < count; idx++) {
        sharp_glyph_fn(pDC,
                       pDC->ptlDCOrig.x + x + offset.x,
                       pDC->ptlDCOrig.y + y + offset.y,
                       formatEntry->bitmaps[wstr[idx]],
                       &formatEntry->gis[wstr[idx]],
                       &pTextPen);
        if(lpDx)
        {
            if(flags & ETO_PDY)
            {
                offset.x += lpDx[idx * 2];
                offset.y += lpDx[idx * 2 + 1];
            }
            else
                offset.x += lpDx[idx];
        }
        else
        {
            offset.x += formatEntry->gis[wstr[idx]].xOff;
            offset.y += formatEntry->gis[wstr[idx]].yOff;
        }
    }

    //Cleanup the temporary pen
    EBRUSHOBJ_vCleanup(&pTextPen);
    BRUSH_ShareUnlockBrush(ppen);
    GreDeleteObject(hpen);
}

/* EOF */
