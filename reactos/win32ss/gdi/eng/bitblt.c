/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          GDI BitBlt Functions
 * FILE:             subsystems/win32/win32k/eng/bitblt.c
 * PROGRAMER:        Jason Filby
 *                   Timo Kreuzer
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

XCLIPOBJ gxcoTrivial =
{
    /* CLIPOBJ */
    {
        {
            0, /* iUniq */
            {LONG_MIN, LONG_MIN, LONG_MAX, LONG_MAX}, /* rclBounds */
            DC_TRIVIAL,    /* idCOmplexity */
            FC_RECT,       /* iFComplexity */
            TC_RECTANGLES, /* iMode */
            0              /* fjOptions */
        },
    },
    0, 0, 0
};

typedef BOOLEAN (APIENTRY *PBLTRECTFUNC)(SURFOBJ* OutputObj,
                                        SURFOBJ* InputObj,
                                        SURFOBJ* Mask,
                                        XLATEOBJ* ColorTranslation,
                                        RECTL* OutputRect,
                                        POINTL* InputPoint,
                                        POINTL* MaskOrigin,
                                        BRUSHOBJ* pbo,
                                        POINTL* BrushOrigin,
                                        ROP4 Rop4);

static BOOLEAN APIENTRY
BltMask(SURFOBJ* psoDest,
        SURFOBJ* psoSource,
        SURFOBJ* psoMask,
        XLATEOBJ* ColorTranslation,
        RECTL* prclDest,
        POINTL* pptlSource,
        POINTL* pptlMask,
        BRUSHOBJ* pbo,
        POINTL* pptlBrush,
        ROP4 Rop4)
{
    LONG x, y;
    BYTE *pjMskLine, *pjMskCurrent;
    BYTE fjMaskBit0, fjMaskBit;
    /* Pattern brushes */
    SURFOBJ *psoPattern;
    ULONG PatternWidth = 0, PatternHeight = 0;
    LONG PatternX0 = 0, PatternX = 0, PatternY = 0;
    LONG SrcX = 0, SrcY = 0;
    PFN_DIB_PutPixel fnDest_PutPixel = NULL;
    PFN_DIB_GetPixel fnPattern_GetPixel = NULL, fnSrc_GetPixel = NULL, fnDest_GetPixel;
    ULONG Pattern = 0, Source = 0, Dest = 0;
    DWORD fgndRop, bkgndRop;

    ASSERT(IS_VALID_ROP4(Rop4));
    ASSERT(psoMask->iBitmapFormat == BMF_1BPP);

    if (!psoMask) return FALSE;

    fgndRop = ROP4_FGND(Rop4);
    bkgndRop = ROP4_BKGND(Rop4);

    //DPRINT1("Rop4 : 0x%08x\n", Rop4);

    /* Determine pattern */
    if (pbo && pbo->iSolidColor == 0xFFFFFFFF)
    {
        psoPattern = BRUSHOBJ_psoPattern(pbo);
        if (psoPattern)
        {
            PatternWidth = psoPattern->sizlBitmap.cx;
            PatternHeight = psoPattern->sizlBitmap.cy;
            fnPattern_GetPixel = DibFunctionsForBitmapFormat[psoPattern->iBitmapFormat].DIB_GetPixel;
        }
    }
    else
        psoPattern = NULL;

    pjMskLine = (PBYTE)psoMask->pvScan0 + pptlMask->y * psoMask->lDelta + (pptlMask->x >> 3);
    fjMaskBit0 = 0x80 >> (pptlMask->x & 0x07);

    fnDest_PutPixel = DibFunctionsForBitmapFormat[psoDest->iBitmapFormat].DIB_PutPixel;
    fnDest_GetPixel = DibFunctionsForBitmapFormat[psoDest->iBitmapFormat].DIB_GetPixel;

    /* Do we have a source */
    if(psoSource)
    {
        /* Sanity check */
        ASSERT(ROP4_USES_SOURCE(Rop4));
        fnSrc_GetPixel = DibFunctionsForBitmapFormat[psoSource->iBitmapFormat].DIB_GetPixel;
        SrcY = pptlSource->y;
        SrcX = pptlSource->x;
    }

    if (psoPattern)
    {
        PatternY = (prclDest->top - pptlBrush->y) % PatternHeight;
        if (PatternY < 0)
        {
            PatternY += PatternHeight;
        }
        PatternX0 = (prclDest->left - pptlBrush->x) % PatternWidth;
        if (PatternX0 < 0)
        {
            PatternX0 += PatternWidth;
        }
        PatternX = PatternX0;
    }
    else
    {
        Pattern = pbo ? pbo->iSolidColor : 0;
    }

    for (y = prclDest->top; y < prclDest->bottom; y++)
    {
        pjMskCurrent = pjMskLine;
        fjMaskBit = fjMaskBit0;

        for (x = prclDest->left; x < prclDest->right; x++)
        {
            Rop4 = (*pjMskCurrent & fjMaskBit) ? fgndRop : bkgndRop;

            if(psoPattern)
            {
                if(ROP4_USES_PATTERN(Rop4))
                    Pattern = fnPattern_GetPixel(psoPattern, PatternX, PatternY);
                PatternX++;
                PatternX %= PatternWidth;
            }

            if(psoSource)
            {
                if(ROP4_USES_SOURCE(Rop4))
                {
                    Source = XLATEOBJ_iXlate(ColorTranslation,
                                             fnSrc_GetPixel(psoSource, SrcX, SrcY));
                }
                SrcX++;
            }

            if(ROP4_USES_DEST(Rop4))
                Dest = fnDest_GetPixel(psoDest, x, y);

            fnDest_PutPixel(psoDest,
                            x,
                            y,
                            DIB_DoRop(Rop4,
                                      Dest,
                                      Source,
                                      Pattern));
            fjMaskBit = _rotr8(fjMaskBit, 1);
            pjMskCurrent += (fjMaskBit >> 7);
        }
        pjMskLine += psoMask->lDelta;
        if(psoPattern)
        {
            PatternY++;
            PatternY %= PatternHeight;
            PatternX = PatternX0;
        }
        if(psoSource)
        {
            SrcY++;
            SrcX = pptlSource->x;
        }
    }

    return TRUE;
}

#ifndef _USE_DIBLIB_

static BOOLEAN APIENTRY
BltPatCopy(SURFOBJ* Dest,
           SURFOBJ* Source,
           SURFOBJ* Mask,
           XLATEOBJ* ColorTranslation,
           RECTL* DestRect,
           POINTL* SourcePoint,
           POINTL* MaskPoint,
           BRUSHOBJ* pbo,
           POINTL* BrushPoint,
           DWORD Rop4)
{
    // These functions are assigned if we're working with a DIB
    // The assigned functions depend on the bitsPerPixel of the DIB

    DibFunctionsForBitmapFormat[Dest->iBitmapFormat].DIB_ColorFill(Dest, DestRect, pbo ? pbo->iSolidColor : 0);

    return TRUE;
}

static BOOLEAN APIENTRY
CallDibBitBlt(SURFOBJ* OutputObj,
              SURFOBJ* InputObj,
              SURFOBJ* Mask,
              XLATEOBJ* ColorTranslation,
              RECTL* OutputRect,
              POINTL* InputPoint,
              POINTL* MaskOrigin,
              BRUSHOBJ* pbo,
              POINTL* BrushOrigin,
              ROP4 Rop4)
{
    BLTINFO BltInfo;
    SURFOBJ *psoPattern;
    BOOLEAN Result;

    BltInfo.DestSurface = OutputObj;
    BltInfo.SourceSurface = InputObj;
    BltInfo.PatternSurface = NULL;
    BltInfo.XlateSourceToDest = ColorTranslation;
    BltInfo.DestRect = *OutputRect;
    BltInfo.SourcePoint = *InputPoint;

    if ((Rop4 & 0xFF) == R3_OPINDEX_SRCCOPY)
        return DibFunctionsForBitmapFormat[OutputObj->iBitmapFormat].DIB_BitBltSrcCopy(&BltInfo);

    BltInfo.Brush = pbo;
    BltInfo.BrushOrigin = *BrushOrigin;
    BltInfo.Rop4 = Rop4;

    /* Pattern brush */
    if (ROP4_USES_PATTERN(Rop4) && pbo && pbo->iSolidColor == 0xFFFFFFFF)
    {
        psoPattern = BRUSHOBJ_psoPattern(pbo);
        if (psoPattern)
        {
            BltInfo.PatternSurface = psoPattern;
        }
        else
        {
            /* FIXME: What to do here? */
        }
    }
    else
    {
        psoPattern = NULL;
    }

    Result = DibFunctionsForBitmapFormat[OutputObj->iBitmapFormat].DIB_BitBlt(&BltInfo);

    return Result;
}

INT __cdecl abs(INT nm);


/*
 * @implemented
 */
BOOL APIENTRY
NtGdiEngBitBlt(
                IN SURFOBJ  *psoTrg,
                IN SURFOBJ  *psoSrc,
                IN SURFOBJ  *psoMask,
                IN CLIPOBJ  *pco,
                IN XLATEOBJ  *pxlo,
                IN RECTL  *prclTrg,
                IN POINTL  *pptlSrc,
                IN POINTL  *pptlMask,
                IN BRUSHOBJ  *pbo,
                IN POINTL  *pptlBrush,
                IN ROP4 Rop4)
{
    RECTL  rclTrg;
    POINTL ptlSrc;
    POINTL ptlMask;
    POINTL ptlBrush;

    _SEH2_TRY
    {
        ProbeForRead(prclTrg, sizeof(RECTL), 1);
        RtlCopyMemory(&rclTrg,prclTrg, sizeof(RECTL));

        ProbeForRead(pptlSrc, sizeof(POINTL), 1);
        RtlCopyMemory(&ptlSrc, pptlSrc, sizeof(POINTL));

        ProbeForRead(pptlMask, sizeof(POINTL), 1);
        RtlCopyMemory(&ptlMask, pptlMask, sizeof(POINTL));

        ProbeForRead(pptlBrush, sizeof(POINTL), 1);
        RtlCopyMemory(&ptlBrush, pptlBrush, sizeof(POINTL));

    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return FALSE);
    }
    _SEH2_END;

    return  EngBitBlt(psoTrg, psoSrc, psoMask, pco, pxlo, &rclTrg, &ptlSrc, &ptlMask, pbo, &ptlBrush, Rop4);
}

/*
 * @implemented
 */
BOOL
APIENTRY
EngBitBlt(
    _Inout_ SURFOBJ *psoTrg,
    _In_opt_ SURFOBJ *psoSrc,
    _In_opt_ SURFOBJ *psoMask,
    _In_opt_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_ RECTL *prclTrg,
    _In_opt_ POINTL *pptlSrc,
    _In_opt_ POINTL *pptlMask,
    _In_opt_ BRUSHOBJ *pbo,
    _In_opt_ POINTL *pptlBrush,
    _In_ ROP4 rop4)
{
    BYTE               clippingType;
    RECTL              CombinedRect;
    RECT_ENUM          RectEnum;
    BOOL               EnumMore;
    POINTL             InputPoint;
    RECTL              InputRect;
    RECTL              OutputRect;
    SURFOBJ*           InputObj = 0;
    SURFOBJ*           OutputObj;
    PBLTRECTFUNC       BltRectFunc;
    BOOLEAN            Ret = TRUE;
    RECTL              ClipRect;
    ULONG              i;
    POINTL             Pt;
    ULONG              Direction;
    BOOL               UsesSource, UsesMask;
    POINTL             AdjustedBrushOrigin;

    UsesSource = ROP4_USES_SOURCE(rop4);
    UsesMask = ROP4_USES_MASK(rop4);

    if (rop4 == ROP4_NOOP)
    {
        /* Copy destination onto itself: nop */
        return TRUE;
    }

    //DPRINT1("rop4 : 0x%08x\n", rop4);

    OutputRect = *prclTrg;
    RECTL_vMakeWellOrdered(&OutputRect);

    if (UsesSource)
    {
        if (!psoSrc || !pptlSrc)
        {
            return FALSE;
        }

        /* Make sure we don't try to copy anything outside the valid source
           region */
        InputPoint = *pptlSrc;
        if (InputPoint.x < 0)
        {
            OutputRect.left -= InputPoint.x;
            InputPoint.x = 0;
        }
        if (InputPoint.y < 0)
        {
            OutputRect.top -= InputPoint.y;
            InputPoint.y = 0;
        }
        if (psoSrc->sizlBitmap.cx < InputPoint.x +
                OutputRect.right - OutputRect.left)
        {
            OutputRect.right = OutputRect.left +
                               psoSrc->sizlBitmap.cx - InputPoint.x;
        }
        if (psoSrc->sizlBitmap.cy < InputPoint.y +
                OutputRect.bottom - OutputRect.top)
        {
            OutputRect.bottom = OutputRect.top +
                                psoSrc->sizlBitmap.cy - InputPoint.y;
        }

        InputRect.left = InputPoint.x;
        InputRect.right = InputPoint.x + (OutputRect.right - OutputRect.left);
        InputRect.top = InputPoint.y;
        InputRect.bottom = InputPoint.y + (OutputRect.bottom - OutputRect.top);

        InputObj = psoSrc;
    }
    else
    {
        InputPoint.x = InputPoint.y = 0;
        InputRect.left = 0;
        InputRect.right = prclTrg->right - prclTrg->left;
        InputRect.top = 0;
        InputRect.bottom = prclTrg->bottom - prclTrg->top;
    }

    if (NULL != pco)
    {
        if (OutputRect.left < pco->rclBounds.left)
        {
            InputRect.left += pco->rclBounds.left - OutputRect.left;
            InputPoint.x += pco->rclBounds.left - OutputRect.left;
            OutputRect.left = pco->rclBounds.left;
        }
        if (pco->rclBounds.right < OutputRect.right)
        {
            InputRect.right -=  OutputRect.right - pco->rclBounds.right;
            OutputRect.right = pco->rclBounds.right;
        }
        if (OutputRect.top < pco->rclBounds.top)
        {
            InputRect.top += pco->rclBounds.top - OutputRect.top;
            InputPoint.y += pco->rclBounds.top - OutputRect.top;
            OutputRect.top = pco->rclBounds.top;
        }
        if (pco->rclBounds.bottom < OutputRect.bottom)
        {
            InputRect.bottom -=  OutputRect.bottom - pco->rclBounds.bottom;
            OutputRect.bottom = pco->rclBounds.bottom;
        }
    }

    /* Check for degenerate case: if height or width of OutputRect is 0 pixels
       there's nothing to do */
    if (OutputRect.right <= OutputRect.left ||
            OutputRect.bottom <= OutputRect.top)
    {
        return TRUE;
    }

    OutputObj = psoTrg;

    if (pptlBrush)
    {
        AdjustedBrushOrigin.x = pptlBrush->x;
        AdjustedBrushOrigin.y = pptlBrush->y;
    }
    else
    {
        AdjustedBrushOrigin.x = 0;
        AdjustedBrushOrigin.y = 0;
    }

    /* Determine clipping type */
    if (pco == (CLIPOBJ *) NULL)
    {
        clippingType = DC_TRIVIAL;
    }
    else
    {
        clippingType = pco->iDComplexity;
    }

    if (UsesMask)
    {
        BltRectFunc = BltMask;
    }
    else if ((rop4 & 0xFF) == R3_OPINDEX_PATCOPY)
    {
        if (pbo && pbo->iSolidColor == 0xFFFFFFFF)
            BltRectFunc = CallDibBitBlt;
        else
            BltRectFunc = BltPatCopy;
    }
    else
    {
        BltRectFunc = CallDibBitBlt;
    }


    switch (clippingType)
    {
        case DC_TRIVIAL:
            Ret = (*BltRectFunc)(OutputObj,
                                 InputObj,
                                 psoMask,
                                 pxlo,
                                 &OutputRect,
                                 &InputPoint,
                                 pptlMask,
                                 pbo,
                                 &AdjustedBrushOrigin,
                                 rop4);
            break;
        case DC_RECT:
            /* Clip the blt to the clip rectangle */
            ClipRect.left = pco->rclBounds.left;
            ClipRect.right = pco->rclBounds.right;
            ClipRect.top = pco->rclBounds.top;
            ClipRect.bottom = pco->rclBounds.bottom;
            if (RECTL_bIntersectRect(&CombinedRect, &OutputRect, &ClipRect))
            {
#ifdef _USE_DIBLIB_
                if (BrushOrigin)
                {
                    AdjustedBrushOrigin.x = BrushOrigin->x + CombinedRect.left - OutputRect.left;
                    AdjustedBrushOrigin.y = BrushOrigin->y + CombinedRect.top - OutputRect.top;
                }
#endif
                Pt.x = InputPoint.x + CombinedRect.left - OutputRect.left;
                Pt.y = InputPoint.y + CombinedRect.top - OutputRect.top;
                Ret = (*BltRectFunc)(OutputObj,
                                     InputObj,
                                     psoMask,
                                     pxlo,
                                     &CombinedRect,
                                     &Pt,
                                     pptlMask,
                                     pbo,
                                     &AdjustedBrushOrigin,
                                     rop4);
            }
            break;
        case DC_COMPLEX:
            Ret = TRUE;
            if (OutputObj == InputObj)
            {
                if (OutputRect.top < InputPoint.y)
                {
                    Direction = OutputRect.left < InputPoint.x ?
                                CD_RIGHTDOWN : CD_LEFTDOWN;
                }
                else
                {
                    Direction = OutputRect.left < InputPoint.x ?
                                CD_RIGHTUP : CD_LEFTUP;
                }
            }
            else
            {
                Direction = CD_ANY;
            }
            CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, Direction, 0);
            do
            {
                EnumMore = CLIPOBJ_bEnum(pco, sizeof(RectEnum),
                                         (PVOID) &RectEnum);

                for (i = 0; i < RectEnum.c; i++)
                {
                    ClipRect.left = RectEnum.arcl[i].left;
                    ClipRect.right = RectEnum.arcl[i].right;
                    ClipRect.top = RectEnum.arcl[i].top;
                    ClipRect.bottom = RectEnum.arcl[i].bottom;
                    if (RECTL_bIntersectRect(&CombinedRect, &OutputRect, &ClipRect))
                    {
#ifdef _USE_DIBLIB_
                        if (BrushOrigin)
                        {
                            AdjustedBrushOrigin.x = BrushOrigin->x + CombinedRect.left - OutputRect.left;
                            AdjustedBrushOrigin.y = BrushOrigin->y + CombinedRect.top - OutputRect.top;
                        }
#endif
                        Pt.x = InputPoint.x + CombinedRect.left - OutputRect.left;
                        Pt.y = InputPoint.y + CombinedRect.top - OutputRect.top;
                        Ret = (*BltRectFunc)(OutputObj,
                                             InputObj,
                                             psoMask,
                                             pxlo,
                                             &CombinedRect,
                                             &Pt,
                                             pptlMask,
                                             pbo,
                                             &AdjustedBrushOrigin,
                                             rop4) && Ret;
                    }
                }
            }
            while (EnumMore);
            break;
    }

    return Ret;
}

BOOL APIENTRY
IntEngBitBlt(
    SURFOBJ *psoTrg,
    SURFOBJ *psoSrc,
    SURFOBJ *psoMask,
    CLIPOBJ *pco,
    XLATEOBJ *pxlo,
    RECTL *prclTrg,
    POINTL *pptlSrc,
    POINTL *pptlMask,
    BRUSHOBJ *pbo,
    POINTL *pptlBrush,
    ROP4 Rop4)
{
    SURFACE *psurfTrg;
    SURFACE *psurfSrc = NULL;
    BOOL bResult;
    RECTL rclClipped;
    RECTL rclSrc;
    POINTL ptlBrush;
    PFN_DrvBitBlt pfnBitBlt;

    ASSERT(psoTrg);
    psurfTrg = CONTAINING_RECORD(psoTrg, SURFACE, SurfObj);

    /* FIXME: Should we really allow to pass non-well-ordered rects? */
    rclClipped = *prclTrg;
    RECTL_vMakeWellOrdered(&rclClipped);

    //DPRINT1("Rop4 : 0x%08x\n", Rop4);

    /* Sanity check */
    ASSERT(IS_VALID_ROP4(Rop4));

    if (pco)
    {
        /* Clip target rect against the bounds of the clipping region */
        if (!RECTL_bIntersectRect(&rclClipped, &rclClipped, &pco->rclBounds))
        {
            /* Nothing left */
            return TRUE;
        }

        /* Don't pass a clipobj with only a single rect */
        if (pco->iDComplexity == DC_RECT)
            pco = NULL;
    }
    else
        pco = &gxcoTrivial.ClipObj;

    if (ROP4_USES_SOURCE(Rop4))
    {
        ASSERT(psoSrc);
        psurfSrc = CONTAINING_RECORD(psoSrc, SURFACE, SurfObj);

        /* Calculate source rect */
        rclSrc.left = pptlSrc->x + rclClipped.left - prclTrg->left;
        rclSrc.top = pptlSrc->y + rclClipped.top - prclTrg->top;
        rclSrc.right = rclSrc.left + rclClipped.right - rclClipped.left;
        rclSrc.bottom = rclSrc.top + rclClipped.bottom - rclClipped.top;
        pptlSrc = (PPOINTL)&rclSrc;
    }
    else
    {
        psoSrc = NULL;
        psurfSrc = NULL;
    }

    if (pptlBrush)
    {
#ifdef _USE_DIBLIB_
        ptlBrush.x = pptlBrush->x + rclClipped.left - prclTrg->left;
        ptlBrush.y = pptlBrush->y + rclClipped.top - prclTrg->top;
#else
        ptlBrush = *pptlBrush;
#endif
    }

    /* Is the target surface device managed? */
    if (psurfTrg->flags & HOOK_BITBLT)
    {
        /* Is the source a different device managed surface? */
        if (psoSrc && psoSrc->hdev != psoTrg->hdev && psurfSrc->flags & HOOK_BITBLT)
        {
            DPRINT1("Need to copy to standard bitmap format!\n");
            ASSERT(FALSE);
        }

        pfnBitBlt = GDIDEVFUNCS(psoTrg).BitBlt;
    }

    /* Is the source surface device managed? */
    else if (psoSrc && psurfSrc->flags & HOOK_BITBLT)
    {
        pfnBitBlt = GDIDEVFUNCS(psoSrc).BitBlt;
    }
    else
    {
        pfnBitBlt = EngBitBlt;
    }

    bResult = pfnBitBlt(psoTrg,
                        psoSrc,
                        psoMask,
                        pco,
                        pxlo,
                        &rclClipped,
                        pptlSrc,
                        pptlMask,
                        pbo,
                        pptlBrush ? &ptlBrush : NULL,
                        Rop4);

    // FIXME: cleanup temp surface!

    return bResult;
}

#endif // !_USE_DIBLIB_

/**** REACTOS FONT RENDERING CODE *********************************************/

/* renders the alpha mask bitmap */
static BOOLEAN APIENTRY
AlphaBltMask(SURFOBJ* psoDest,
             SURFOBJ* psoSource, // unused
             SURFOBJ* psoMask,
             XLATEOBJ* pxloRGB2Dest,
             XLATEOBJ* pxloBrush,
             RECTL* prclDest,
             POINTL* pptlSource, // unused
             POINTL* pptlMask,
             BRUSHOBJ* pbo,
             POINTL* pptlBrush)
{
    LONG i, j, dx, dy;
    int r, g, b;
    ULONG Background, BrushColor, NewColor;
    BYTE *tMask, *lMask;

    ASSERT(psoSource == NULL);
    ASSERT(pptlSource == NULL);

    dx = prclDest->right  - prclDest->left;
    dy = prclDest->bottom - prclDest->top;

    if (psoMask != NULL)
    {
        BrushColor = XLATEOBJ_iXlate(pxloBrush, pbo ? pbo->iSolidColor : 0);
        r = (int)GetRValue(BrushColor);
        g = (int)GetGValue(BrushColor);
        b = (int)GetBValue(BrushColor);

        tMask = (PBYTE)psoMask->pvScan0 + (pptlMask->y * psoMask->lDelta) + pptlMask->x;
        for (j = 0; j < dy; j++)
        {
            lMask = tMask;
            for (i = 0; i < dx; i++)
            {
                if (*lMask > 0)
                {
                    if (*lMask == 0xff)
                    {
                        DibFunctionsForBitmapFormat[psoDest->iBitmapFormat].DIB_PutPixel(
                            psoDest, prclDest->left + i, prclDest->top + j, pbo ? pbo->iSolidColor : 0);
                    }
                    else
                    {
                        Background = DIB_GetSource(psoDest, prclDest->left + i, prclDest->top + j,
                                                   pxloBrush);

                        NewColor =
                            RGB((*lMask * (r - GetRValue(Background)) >> 8) + GetRValue(Background),
                                (*lMask * (g - GetGValue(Background)) >> 8) + GetGValue(Background),
                                (*lMask * (b - GetBValue(Background)) >> 8) + GetBValue(Background));

                        Background = XLATEOBJ_iXlate(pxloRGB2Dest, NewColor);
                        DibFunctionsForBitmapFormat[psoDest->iBitmapFormat].DIB_PutPixel(
                            psoDest, prclDest->left + i, prclDest->top + j, Background);
                    }
                }
                lMask++;
            }
            tMask += psoMask->lDelta;
        }
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static
BOOL APIENTRY
EngMaskBitBlt(SURFOBJ *psoDest,
              SURFOBJ *psoMask,
              CLIPOBJ *ClipRegion,
              XLATEOBJ *DestColorTranslation,
              XLATEOBJ *SourceColorTranslation,
              RECTL *DestRect,
              POINTL *pptlMask,
              BRUSHOBJ *pbo,
              POINTL *BrushOrigin)
{
    BYTE               clippingType;
    RECTL              CombinedRect;
    RECT_ENUM          RectEnum;
    BOOL               EnumMore;
    POINTL             InputPoint;
    RECTL              InputRect;
    RECTL              OutputRect;
    POINTL             Translate;
    INTENG_ENTER_LEAVE EnterLeaveSource;
    INTENG_ENTER_LEAVE EnterLeaveDest;
    SURFOBJ*           psoInput;
    SURFOBJ*           psoOutput;
    BOOLEAN            Ret = TRUE;
    RECTL              ClipRect;
    unsigned           i;
    POINTL             Pt;
    ULONG              Direction;
    POINTL             AdjustedBrushOrigin;

    ASSERT(psoMask);

    if (pptlMask)
    {
        InputRect.left = pptlMask->x;
        InputRect.right = pptlMask->x + (DestRect->right - DestRect->left);
        InputRect.top = pptlMask->y;
        InputRect.bottom = pptlMask->y + (DestRect->bottom - DestRect->top);
    }
    else
    {
        InputRect.left = 0;
        InputRect.right = DestRect->right - DestRect->left;
        InputRect.top = 0;
        InputRect.bottom = DestRect->bottom - DestRect->top;
    }

    OutputRect = *DestRect;
    if (NULL != ClipRegion)
    {
        if (OutputRect.left < ClipRegion->rclBounds.left)
        {
            InputRect.left += ClipRegion->rclBounds.left - OutputRect.left;
            OutputRect.left = ClipRegion->rclBounds.left;
        }
        if (ClipRegion->rclBounds.right < OutputRect.right)
        {
            InputRect.right -=  OutputRect.right - ClipRegion->rclBounds.right;
            OutputRect.right = ClipRegion->rclBounds.right;
        }
        if (OutputRect.top < ClipRegion->rclBounds.top)
        {
            InputRect.top += ClipRegion->rclBounds.top - OutputRect.top;
            OutputRect.top = ClipRegion->rclBounds.top;
        }
        if (ClipRegion->rclBounds.bottom < OutputRect.bottom)
        {
            InputRect.bottom -=  OutputRect.bottom - ClipRegion->rclBounds.bottom;
            OutputRect.bottom = ClipRegion->rclBounds.bottom;
        }
    }

    if (! IntEngEnter(&EnterLeaveSource, psoMask, &InputRect, TRUE, &Translate, &psoInput))
    {
        return FALSE;
    }

    InputPoint.x = InputRect.left + Translate.x;
    InputPoint.y = InputRect.top + Translate.y;

    /* Check for degenerate case: if height or width of OutputRect is 0 pixels there's
       nothing to do */
    if (OutputRect.right <= OutputRect.left || OutputRect.bottom <= OutputRect.top)
    {
        IntEngLeave(&EnterLeaveSource);
        return TRUE;
    }

    if (! IntEngEnter(&EnterLeaveDest, psoDest, &OutputRect, FALSE, &Translate, &psoOutput))
    {
        IntEngLeave(&EnterLeaveSource);
        return FALSE;
    }

    OutputRect.left = DestRect->left + Translate.x;
    OutputRect.right = DestRect->right + Translate.x;
    OutputRect.top = DestRect->top + Translate.y;
    OutputRect.bottom = DestRect->bottom + Translate.y;

    if (BrushOrigin)
    {
        AdjustedBrushOrigin.x = BrushOrigin->x + Translate.x;
        AdjustedBrushOrigin.y = BrushOrigin->y + Translate.y;
    }
    else
        AdjustedBrushOrigin = Translate;

    // Determine clipping type
    if (ClipRegion == (CLIPOBJ *) NULL)
    {
        clippingType = DC_TRIVIAL;
    } else {
        clippingType = ClipRegion->iDComplexity;
    }

    switch (clippingType)
    {
        case DC_TRIVIAL:
            if (psoMask->iBitmapFormat == BMF_8BPP)
                Ret = AlphaBltMask(psoOutput, NULL , psoInput, DestColorTranslation, SourceColorTranslation,
                                   &OutputRect, NULL, &InputPoint, pbo, &AdjustedBrushOrigin);
            else
                Ret = BltMask(psoOutput, NULL, psoInput, DestColorTranslation,
                              &OutputRect, NULL, &InputPoint, pbo, &AdjustedBrushOrigin,
                              ROP4_MASK);
            break;
        case DC_RECT:
            // Clip the blt to the clip rectangle
            ClipRect.left = ClipRegion->rclBounds.left + Translate.x;
            ClipRect.right = ClipRegion->rclBounds.right + Translate.x;
            ClipRect.top = ClipRegion->rclBounds.top + Translate.y;
            ClipRect.bottom = ClipRegion->rclBounds.bottom + Translate.y;
            if (RECTL_bIntersectRect(&CombinedRect, &OutputRect, &ClipRect))
            {
                Pt.x = InputPoint.x + CombinedRect.left - OutputRect.left;
                Pt.y = InputPoint.y + CombinedRect.top - OutputRect.top;
                if (psoMask->iBitmapFormat == BMF_8BPP)
                {
                    Ret = AlphaBltMask(psoOutput, NULL, psoInput, DestColorTranslation, SourceColorTranslation,
                                       &CombinedRect, NULL, &Pt, pbo, &AdjustedBrushOrigin);
                }
                else
                {
                    Ret = BltMask(psoOutput, NULL, psoInput, DestColorTranslation,
                                  &CombinedRect, NULL, &Pt, pbo, &AdjustedBrushOrigin, ROP4_MASK);
                }
            }
            break;
        case DC_COMPLEX:
            Ret = TRUE;
            if (psoOutput == psoInput)
            {
                if (OutputRect.top < InputPoint.y)
                {
                    Direction = OutputRect.left < InputPoint.x ? CD_RIGHTDOWN : CD_LEFTDOWN;
                }
                else
                {
                    Direction = OutputRect.left < InputPoint.x ? CD_RIGHTUP : CD_LEFTUP;
                }
            }
            else
            {
                Direction = CD_ANY;
            }
            CLIPOBJ_cEnumStart(ClipRegion, FALSE, CT_RECTANGLES, Direction, 0);
            do
            {
                EnumMore = CLIPOBJ_bEnum(ClipRegion,(ULONG) sizeof(RectEnum), (PVOID) &RectEnum);

                for (i = 0; i < RectEnum.c; i++)
                {
                    ClipRect.left = RectEnum.arcl[i].left + Translate.x;
                    ClipRect.right = RectEnum.arcl[i].right + Translate.x;
                    ClipRect.top = RectEnum.arcl[i].top + Translate.y;
                    ClipRect.bottom = RectEnum.arcl[i].bottom + Translate.y;
                    if (RECTL_bIntersectRect(&CombinedRect, &OutputRect, &ClipRect))
                    {
                        Pt.x = InputPoint.x + CombinedRect.left - OutputRect.left;
                        Pt.y = InputPoint.y + CombinedRect.top - OutputRect.top;
                        if (psoMask->iBitmapFormat == BMF_8BPP)
                        {
                            Ret = AlphaBltMask(psoOutput, NULL, psoInput,
                                               DestColorTranslation,
                                               SourceColorTranslation,
                                               &CombinedRect, NULL, &Pt, pbo,
                                               &AdjustedBrushOrigin) && Ret;
                        }
                        else
                        {
                            Ret = BltMask(psoOutput, NULL, psoInput,
                                          DestColorTranslation, &CombinedRect, NULL,
                                          &Pt, pbo, &AdjustedBrushOrigin,
                                          ROP4_MASK) && Ret;
                        }
                    }
                }
            }
            while (EnumMore);
            break;
    }


    IntEngLeave(&EnterLeaveDest);
    IntEngLeave(&EnterLeaveSource);

    return Ret;
}

BOOL
APIENTRY
IntEngMaskBlt(
    _Inout_ SURFOBJ *psoDest,
    _In_ SURFOBJ *psoMask,
    _In_ CLIPOBJ *pco,
    _In_ XLATEOBJ *pxloDest,
    _In_ XLATEOBJ *pxloSource,
    _In_ RECTL *prclDest,
    _In_ POINTL *pptlMask,
    _In_ BRUSHOBJ *pbo,
    _In_ POINTL *pptlBrushOrg)
{
    BOOLEAN ret;
    RECTL rcDest;
    POINTL ptMask = {0,0};
    PSURFACE psurfTemp;
    RECTL rcTemp;

    ASSERT(psoDest);
    ASSERT(psoMask);

    /* Is this a 1 BPP mask? */
    if (psoMask->iBitmapFormat == BMF_1BPP)
    {
        /* Use IntEngBitBlt with an appropriate ROP4 */
        return IntEngBitBlt(psoDest,
                            NULL,
                            psoMask,
                            pco,
                            pxloDest,
                            prclDest,
                            NULL,
                            pptlMask,
                            pbo,
                            pptlBrushOrg,
                            ROP4_MASKPAINT);
    }

    ASSERT(psoMask->iBitmapFormat == BMF_8BPP);

    if (pptlMask)
    {
        ptMask = *pptlMask;
    }

    /* Clip against the bounds of the clipping region so we won't try to write
     * outside the surface */
    if (pco != NULL)
    {
        /* Intersect with the clip bounds and check if everything was clipped */
        if (!RECTL_bIntersectRect(&rcDest, prclDest, &pco->rclBounds))
        {
            return TRUE;
        }

        /* Adjust the mask point */
        ptMask.x += rcDest.left - prclDest->left;
        ptMask.y += rcDest.top - prclDest->top;
    }
    else
    {
        rcDest = *prclDest;
    }

    /* Check if the target surface is device managed */
    if (psoDest->iType != STYPE_BITMAP)
    {
        rcTemp.left = 0;
        rcTemp.top = 0;
        rcTemp.right = rcDest.right - rcDest.left;
        rcTemp.bottom = rcDest.bottom - rcDest.top;

        /* Allocate a temporary surface */
        psurfTemp = SURFACE_AllocSurface(STYPE_BITMAP,
                                         rcTemp.right,
                                         rcTemp.bottom,
                                         psoDest->iBitmapFormat,
                                         0,
                                         0,
                                         NULL);
        if (psurfTemp == NULL)
        {
            return FALSE;
        }

        /* Copy the current target surface bits to the temp surface */
        ret = EngCopyBits(&psurfTemp->SurfObj,
                          psoDest,
                          NULL, // pco
                          NULL, // pxlo
                          &rcTemp,
                          (PPOINTL)&rcDest);

        if (ret)
        {
            /* Do the operation on the temp surface */
            ret = EngMaskBitBlt(&psurfTemp->SurfObj,
                                psoMask,
                                NULL,
                                pxloDest,
                                pxloSource,
                                &rcTemp,
                                &ptMask,
                                pbo,
                                pptlBrushOrg);
        }

        if (ret)
        {
            /* Copy the result back to the dest surface */
            ret = EngCopyBits(psoDest,
                              &psurfTemp->SurfObj,
                              pco,
                              NULL,
                              &rcDest,
                              (PPOINTL)&rcTemp);
        }

        /* Delete the temp surface */
        GDIOBJ_vDeleteObject(&psurfTemp->BaseObject);
    }
    else
    {
        /* Do the operation on the target surface */
        ret = EngMaskBitBlt(psoDest,
                            psoMask,
                            pco,
                            pxloDest,
                            pxloSource,
                            &rcDest,
                            &ptMask,
                            pbo,
                            pptlBrushOrg);
    }

    return ret;
}

/* EOF */
