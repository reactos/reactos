/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          GDI BitBlt Functions
 * FILE:             subsys/win32k/eng/bitblt.c
 * PROGRAMER:        Jason Filby
 * REVISION HISTORY:
 *        2/10/1999: Created
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

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
        SURFOBJ* psoSource, // unused
        SURFOBJ* psoMask,
        XLATEOBJ* ColorTranslation,  // unused
        RECTL* prclDest,
        POINTL* pptlSource, // unused
        POINTL* pptlMask,
        BRUSHOBJ* pbo,
        POINTL* pptlBrush,
        ROP4 Rop4)
{
    LONG x, y;
    BYTE *pjMskLine, *pjMskCurrent;
    BYTE fjMaskBit0, fjMaskBit;
    /* Pattern brushes */
    PEBRUSHOBJ pebo = NULL;
    SURFOBJ *psoPattern = NULL;
    PSURFACE psurfPattern;
    ULONG PatternWidth = 0, PatternHeight = 0;
    LONG PatternX0 = 0, PatternX = 0, PatternY = 0;
    PFN_DIB_PutPixel fnDest_PutPixel = NULL;
    PFN_DIB_GetPixel fnPattern_GetPixel = NULL;
    XLATEOBJ *XlateObj;
    ULONG Pattern = 0;

    if (psoMask == NULL)
    {
        return FALSE;
    }

    if (pbo && pbo->iSolidColor == 0xFFFFFFFF)
    {
        pebo = CONTAINING_RECORD(pbo, EBRUSHOBJ, BrushObject);

        psurfPattern = SURFACE_LockSurface(pebo->pbrush->hbmPattern);
        if (psurfPattern != NULL)
        {
            psoPattern = &psurfPattern->SurfObj;
            PatternWidth = psoPattern->sizlBitmap.cx;
            PatternHeight = psoPattern->sizlBitmap.cy;
        }
        fnPattern_GetPixel = DibFunctionsForBitmapFormat[psoPattern->iBitmapFormat].DIB_GetPixel;
    }
    else
        psurfPattern = NULL;

    pjMskLine = (PBYTE)psoMask->pvScan0 + pptlMask->y * psoMask->lDelta + (pptlMask->x >> 3);
    fjMaskBit0 = 0x80 >> (pptlMask->x & 0x07);

    fnDest_PutPixel = DibFunctionsForBitmapFormat[psoDest->iBitmapFormat].DIB_PutPixel;
    if (psurfPattern)
    {
        XlateObj = pebo ? pebo->XlateObject : NULL;
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

        for (y = prclDest->top; y < prclDest->bottom; y++)
        {
            pjMskCurrent = pjMskLine;
            fjMaskBit = fjMaskBit0;
            PatternX = PatternX0;

            for (x = prclDest->left; x < prclDest->right; x++)
            {
                if (*pjMskCurrent & fjMaskBit)
                {
                    fnDest_PutPixel(psoDest, x, y,
                        XLATEOBJ_iXlate(XlateObj, 
                            fnPattern_GetPixel(psoPattern, PatternX, PatternY)));
                }
                fjMaskBit = _rotr8(fjMaskBit, 1);
                pjMskCurrent += (fjMaskBit >> 7);
                PatternX++;
                PatternX %= PatternWidth;
            }
            pjMskLine += psoMask->lDelta;
            PatternY++;
            PatternY %= PatternHeight;
        }
    }
    else
    {
        Pattern = pbo ? pbo->iSolidColor : 0;
        for (y = prclDest->top; y < prclDest->bottom; y++)
        {
            pjMskCurrent = pjMskLine;
            fjMaskBit = fjMaskBit0;

            for (x = prclDest->left; x < prclDest->right; x++)
            {
                if (*pjMskCurrent & fjMaskBit)
                {
                     fnDest_PutPixel(psoDest, x, y, Pattern);
                }
                fjMaskBit = _rotr8(fjMaskBit, 1);
                pjMskCurrent += (fjMaskBit >> 7);
            }
            pjMskLine += psoMask->lDelta;
        }
    }

    if (psurfPattern)
        SURFACE_UnlockSurface(psurfPattern);

    return TRUE;
}

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
           ROP4 Rop4)
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
    PEBRUSHOBJ GdiBrush = NULL;
    SURFACE *psurfPattern;
    BOOLEAN Result;

    BltInfo.DestSurface = OutputObj;
    BltInfo.SourceSurface = InputObj;
    BltInfo.PatternSurface = NULL;
    BltInfo.XlateSourceToDest = ColorTranslation;
    BltInfo.DestRect = *OutputRect;
    BltInfo.SourcePoint = *InputPoint;

    if (ROP3_TO_ROP4(SRCCOPY) == Rop4)
        return DibFunctionsForBitmapFormat[OutputObj->iBitmapFormat].DIB_BitBltSrcCopy(&BltInfo);

    BltInfo.XlatePatternToDest = NULL;
    BltInfo.Brush = pbo;
    BltInfo.BrushOrigin = *BrushOrigin;
    BltInfo.Rop4 = Rop4;

    /* Pattern brush */
    if (ROP4_USES_PATTERN(Rop4) && pbo && pbo->iSolidColor == 0xFFFFFFFF)
    {
        GdiBrush = CONTAINING_RECORD(pbo, EBRUSHOBJ, BrushObject);
        if ((psurfPattern = SURFACE_LockSurface(GdiBrush->pbrush->hbmPattern)))
        {
            BltInfo.PatternSurface = &psurfPattern->SurfObj;
        }
        else
        {
            /* FIXME - What to do here? */
        }
        BltInfo.XlatePatternToDest = GdiBrush->XlateObject;
    }
    else
    {
        psurfPattern = NULL;
    }

    Result = DibFunctionsForBitmapFormat[OutputObj->iBitmapFormat].DIB_BitBlt(&BltInfo);

    /* Pattern brush */
    if (psurfPattern != NULL)
    {
        SURFACE_UnlockSurface(psurfPattern);
    }

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
                IN ROP4  rop4    )
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

    return  EngBitBlt(psoTrg, psoSrc, psoMask, pco, pxlo, &rclTrg, &ptlSrc, &ptlMask, pbo, &ptlBrush, rop4);
}

/*
 * @implemented
 */
BOOL APIENTRY
EngBitBlt(SURFOBJ *DestObj,
          SURFOBJ *SourceObj,
          SURFOBJ *Mask,
          CLIPOBJ *ClipRegion,
          XLATEOBJ *ColorTranslation,
          RECTL *DestRect,
          POINTL *SourcePoint,
          POINTL *MaskOrigin,
          BRUSHOBJ *pbo,
          POINTL *BrushOrigin,
          ROP4 Rop4)
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
    SURFOBJ*           InputObj;
    SURFOBJ*           OutputObj;
    PBLTRECTFUNC       BltRectFunc;
    BOOLEAN            Ret = TRUE;
    RECTL              ClipRect;
    unsigned           i;
    POINTL             Pt;
    ULONG              Direction;
    BOOL               UsesSource;
    BOOL               UsesPattern;
    POINTL             AdjustedBrushOrigin;

    UsesSource = ROP4_USES_SOURCE(Rop4);
    UsesPattern = ROP4_USES_PATTERN(Rop4);
    if (R4_NOOP == Rop4)
    {
        /* Copy destination onto itself: nop */
        return TRUE;
    }

    OutputRect = *DestRect;
    if (OutputRect.right < OutputRect.left)
    {
        OutputRect.left = DestRect->right;
        OutputRect.right = DestRect->left;
    }
    if (OutputRect.bottom < OutputRect.top)
    {
        OutputRect.left = DestRect->right;
        OutputRect.right = DestRect->left;
    }

    if (UsesSource)
    {
        if (NULL == SourcePoint)
        {
            return FALSE;
        }

        /* Make sure we don't try to copy anything outside the valid source
           region */
        InputPoint = *SourcePoint;
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
        if (SourceObj->sizlBitmap.cx < InputPoint.x +
                OutputRect.right - OutputRect.left)
        {
            OutputRect.right = OutputRect.left +
                               SourceObj->sizlBitmap.cx - InputPoint.x;
        }
        if (SourceObj->sizlBitmap.cy < InputPoint.y +
                OutputRect.bottom - OutputRect.top)
        {
            OutputRect.bottom = OutputRect.top +
                                SourceObj->sizlBitmap.cy - InputPoint.y;
        }

        InputRect.left = InputPoint.x;
        InputRect.right = InputPoint.x + (OutputRect.right - OutputRect.left);
        InputRect.top = InputPoint.y;
        InputRect.bottom = InputPoint.y + (OutputRect.bottom - OutputRect.top);

        if (! IntEngEnter(&EnterLeaveSource, SourceObj, &InputRect, TRUE,
                          &Translate, &InputObj))
        {
            return FALSE;
        }

        InputPoint.x += Translate.x;
        InputPoint.y += Translate.y;
    }
    else
    {
        InputRect.left = 0;
        InputRect.right = DestRect->right - DestRect->left;
        InputRect.top = 0;
        InputRect.bottom = DestRect->bottom - DestRect->top;
    }

    if (NULL != ClipRegion)
    {
        if (OutputRect.left < ClipRegion->rclBounds.left)
        {
            InputRect.left += ClipRegion->rclBounds.left - OutputRect.left;
            InputPoint.x += ClipRegion->rclBounds.left - OutputRect.left;
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
            InputPoint.y += ClipRegion->rclBounds.top - OutputRect.top;
            OutputRect.top = ClipRegion->rclBounds.top;
        }
        if (ClipRegion->rclBounds.bottom < OutputRect.bottom)
        {
            InputRect.bottom -=  OutputRect.bottom - ClipRegion->rclBounds.bottom;
            OutputRect.bottom = ClipRegion->rclBounds.bottom;
        }
    }

    /* Check for degenerate case: if height or width of OutputRect is 0 pixels
       there's nothing to do */
    if (OutputRect.right <= OutputRect.left ||
            OutputRect.bottom <= OutputRect.top)
    {
        if (UsesSource)
        {
            IntEngLeave(&EnterLeaveSource);
        }
        return TRUE;
    }

    if (! IntEngEnter(&EnterLeaveDest, DestObj, &OutputRect, FALSE, &Translate,
                      &OutputObj))
    {
        if (UsesSource)
        {
            IntEngLeave(&EnterLeaveSource);
        }
        return FALSE;
    }

    OutputRect.left += Translate.x;
    OutputRect.right += Translate.x;
    OutputRect.top += Translate.y;
    OutputRect.bottom += Translate.y;

    if (BrushOrigin)
    {
        AdjustedBrushOrigin.x = BrushOrigin->x + Translate.x;
        AdjustedBrushOrigin.y = BrushOrigin->y + Translate.y;
    }
    else
    {
        AdjustedBrushOrigin = Translate;
    }

    /* Determine clipping type */
    if (ClipRegion == (CLIPOBJ *) NULL)
    {
        clippingType = DC_TRIVIAL;
    }
    else
    {
        clippingType = ClipRegion->iDComplexity;
    }

    if (R4_MASK == Rop4)
    {
        BltRectFunc = BltMask;
    }
    else if (ROP3_TO_ROP4(PATCOPY) == Rop4)
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
            Ret = (*BltRectFunc)(OutputObj, InputObj, Mask, ColorTranslation,
                                 &OutputRect, &InputPoint, MaskOrigin, pbo,
                                 &AdjustedBrushOrigin, Rop4);
            break;
        case DC_RECT:
            /* Clip the blt to the clip rectangle */
            ClipRect.left = ClipRegion->rclBounds.left + Translate.x;
            ClipRect.right = ClipRegion->rclBounds.right + Translate.x;
            ClipRect.top = ClipRegion->rclBounds.top + Translate.y;
            ClipRect.bottom = ClipRegion->rclBounds.bottom + Translate.y;
            if (RECTL_bIntersectRect(&CombinedRect, &OutputRect, &ClipRect))
            {
                Pt.x = InputPoint.x + CombinedRect.left - OutputRect.left;
                Pt.y = InputPoint.y + CombinedRect.top - OutputRect.top;
                Ret = (*BltRectFunc)(OutputObj, InputObj, Mask, ColorTranslation,
                                     &CombinedRect, &Pt, MaskOrigin, pbo,
                                     &AdjustedBrushOrigin, Rop4);
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
            CLIPOBJ_cEnumStart(ClipRegion, FALSE, CT_RECTANGLES, Direction, 0);
            do
            {
                EnumMore = CLIPOBJ_bEnum(ClipRegion,(ULONG) sizeof(RectEnum),
                                         (PVOID) &RectEnum);

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
                        Ret = (*BltRectFunc)(OutputObj, InputObj, Mask,
                                             ColorTranslation, &CombinedRect, &Pt,
                                             MaskOrigin, pbo, &AdjustedBrushOrigin,
                                             Rop4) && Ret;
                    }
                }
            }
            while (EnumMore);
            break;
    }


    IntEngLeave(&EnterLeaveDest);
    if (UsesSource)
    {
        IntEngLeave(&EnterLeaveSource);
    }

    return Ret;
}

BOOL APIENTRY
IntEngBitBltEx(SURFOBJ *psoDest,
               SURFOBJ *psoSource,
               SURFOBJ *MaskSurf,
               CLIPOBJ *ClipRegion,
               XLATEOBJ *ColorTranslation,
               RECTL *DestRect,
               POINTL *SourcePoint,
               POINTL *MaskOrigin,
               BRUSHOBJ *pbo,
               POINTL *BrushOrigin,
               ROP4 Rop4,
               BOOL RemoveMouse)
{
    BOOLEAN ret;
    RECTL InputClippedRect;
    RECTL OutputRect;
    POINTL InputPoint;
    BOOLEAN UsesSource;
    SURFACE *psurfDest;
    SURFACE *psurfSource = NULL;

    if (psoDest == NULL)
        return FALSE;

    psurfDest = CONTAINING_RECORD(psoDest, SURFACE, SurfObj);
    ASSERT(psurfDest);

    InputClippedRect = *DestRect;
    if (InputClippedRect.right < InputClippedRect.left)
    {
        InputClippedRect.left = DestRect->right;
        InputClippedRect.right = DestRect->left;
    }
    if (InputClippedRect.bottom < InputClippedRect.top)
    {
        InputClippedRect.top = DestRect->bottom;
        InputClippedRect.bottom = DestRect->top;
    }
    UsesSource = ROP4_USES_SOURCE(Rop4);
    if (UsesSource)
    {
        if (NULL == SourcePoint || NULL == psoSource)
        {
            return FALSE;
        }
        InputPoint = *SourcePoint;

        /* Make sure we don't try to copy anything outside the valid source
           region */
        if (InputPoint.x < 0)
        {
            InputClippedRect.left -= InputPoint.x;
            InputPoint.x = 0;
        }
        if (InputPoint.y < 0)
        {
            InputClippedRect.top -= InputPoint.y;
            InputPoint.y = 0;
        }
        if (psoSource->sizlBitmap.cx < InputPoint.x +
                InputClippedRect.right -
                InputClippedRect.left)
        {
            InputClippedRect.right = InputClippedRect.left +
                                     psoSource->sizlBitmap.cx - InputPoint.x;
        }
        if (psoSource->sizlBitmap.cy < InputPoint.y +
                InputClippedRect.bottom -
                InputClippedRect.top)
        {
            InputClippedRect.bottom = InputClippedRect.top +
                                      psoSource->sizlBitmap.cy - InputPoint.y;
        }

        if (InputClippedRect.right < InputClippedRect.left ||
                InputClippedRect.bottom < InputClippedRect.top)
        {
            /* Everything clipped away, nothing to do */
            return TRUE;
        }
    }

    /* Clip against the bounds of the clipping region so we won't try to write
     * outside the surface */
    if (NULL != ClipRegion)
    {
        if (!RECTL_bIntersectRect(&OutputRect, &InputClippedRect,
                               &ClipRegion->rclBounds))
        {
            return TRUE;
        }
        InputPoint.x += OutputRect.left - InputClippedRect.left;
        InputPoint.y += OutputRect.top - InputClippedRect.top;
    }
    else
    {
        OutputRect = InputClippedRect;
    }

    if (RemoveMouse)
    {
        SURFACE_LockBitmapBits(psurfDest);

        if (UsesSource)
        {
            if (psoSource != psoDest)
            {
                psurfSource = CONTAINING_RECORD(psoSource, SURFACE, SurfObj);
                SURFACE_LockBitmapBits(psurfSource);
            }
            MouseSafetyOnDrawStart(psoSource, InputPoint.x, InputPoint.y,
                                   (InputPoint.x + abs(DestRect->right - DestRect->left)),
                                   (InputPoint.y + abs(DestRect->bottom - DestRect->top)));
        }
        MouseSafetyOnDrawStart(psoDest, OutputRect.left, OutputRect.top,
                               OutputRect.right, OutputRect.bottom);
    }

    /* No success yet */
    ret = FALSE;

    /* Call the driver's DrvBitBlt if available */
    if (psurfDest->flHooks & HOOK_BITBLT)
    {
        ret = GDIDEVFUNCS(psoDest).BitBlt(
                  psoDest, psoSource, MaskSurf, ClipRegion, ColorTranslation,
                  &OutputRect, &InputPoint, MaskOrigin, pbo, BrushOrigin,
                  Rop4);
    }

    if (! ret)
    {
        ret = EngBitBlt(psoDest, psoSource, MaskSurf, ClipRegion, ColorTranslation,
                        &OutputRect, &InputPoint, MaskOrigin, pbo, BrushOrigin,
                        Rop4);
    }

    if (RemoveMouse)
    {
        MouseSafetyOnDrawEnd(psoDest);
        if (UsesSource)
        {
            MouseSafetyOnDrawEnd(psoSource);
            if (psoSource != psoDest)
            {
                SURFACE_UnlockBitmapBits(psurfSource);
            }
        }

        SURFACE_UnlockBitmapBits(psurfDest);
    }

    return ret;
}


/**** REACTOS FONT RENDERING CODE *********************************************/

/* renders the alpha mask bitmap */
static BOOLEAN APIENTRY
AlphaBltMask(SURFOBJ* psoDest,
             SURFOBJ* psoSource, // unused
             SURFOBJ* psoMask,
             XLATEOBJ* ColorTranslation,
             XLATEOBJ* SrcColorTranslation,
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

    dx = prclDest->right  - prclDest->left;
    dy = prclDest->bottom - prclDest->top;

    if (psoMask != NULL)
    {
        BrushColor = XLATEOBJ_iXlate(SrcColorTranslation, pbo ? pbo->iSolidColor : 0);
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
                                                   SrcColorTranslation);

                        NewColor =
                            RGB((*lMask * (r - GetRValue(Background)) >> 8) + GetRValue(Background),
                                (*lMask * (g - GetGValue(Background)) >> 8) + GetGValue(Background),
                                (*lMask * (b - GetBValue(Background)) >> 8) + GetBValue(Background));

                        Background = XLATEOBJ_iXlate(ColorTranslation, NewColor);
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
                                   &OutputRect, &InputPoint, pptlMask, pbo, &AdjustedBrushOrigin);
            else
                Ret = BltMask(psoOutput, NULL, psoInput, DestColorTranslation,
                              &OutputRect, &InputPoint, pptlMask, pbo, &AdjustedBrushOrigin,
                              R4_MASK);
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
                    Ret = AlphaBltMask(psoOutput, psoInput, psoMask, DestColorTranslation, SourceColorTranslation,
                                       &CombinedRect, &Pt, pptlMask, pbo, &AdjustedBrushOrigin);
                }
                else
                {
                    Ret = BltMask(psoOutput, psoInput, psoMask, DestColorTranslation,
                                  &CombinedRect, &Pt, pptlMask, pbo, &AdjustedBrushOrigin, R4_MASK);
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
                            Ret = AlphaBltMask(psoOutput, psoInput, psoMask,
                                               DestColorTranslation,
                                               SourceColorTranslation,
                                               &CombinedRect, &Pt, pptlMask, pbo,
                                               &AdjustedBrushOrigin) && Ret;
                        }
                        else
                        {
                            Ret = BltMask(psoOutput, psoInput, psoMask,
                                          DestColorTranslation, &CombinedRect, &Pt,
                                          pptlMask, pbo, &AdjustedBrushOrigin,
                                          R4_MASK) && Ret;
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

BOOL APIENTRY
IntEngMaskBlt(SURFOBJ *psoDest,
              SURFOBJ *psoMask,
              CLIPOBJ *ClipRegion,
              XLATEOBJ *DestColorTranslation,
              XLATEOBJ *SourceColorTranslation,
              RECTL *DestRect,
              POINTL *pptlMask,
              BRUSHOBJ *pbo,
              POINTL *BrushOrigin)
{
    BOOLEAN ret;
    RECTL OutputRect;
    POINTL InputPoint;
    SURFACE *psurfDest;

    ASSERT(psoMask);

    if (pptlMask)
    {
        InputPoint = *pptlMask;
    }

    /* Clip against the bounds of the clipping region so we won't try to write
     * outside the surface */
    if (NULL != ClipRegion)
    {
        if (!RECTL_bIntersectRect(&OutputRect, DestRect, &ClipRegion->rclBounds))
        {
            return TRUE;
        }
        InputPoint.x += OutputRect.left - DestRect->left;
        InputPoint.y += OutputRect.top - DestRect->top;
    }
    else
    {
        OutputRect = *DestRect;
    }

    /* No success yet */
    ret = FALSE;
    ASSERT(psoDest);
    psurfDest = CONTAINING_RECORD(psoDest, SURFACE, SurfObj);

    SURFACE_LockBitmapBits(psurfDest);
    MouseSafetyOnDrawStart(psoDest, OutputRect.left, OutputRect.top,
                           OutputRect.right, OutputRect.bottom);

    /* Dummy BitBlt to let driver know that it should flush its changes.
       This should really be done using a call to DrvSynchronizeSurface,
       but the VMware driver doesn't hook that call. */
    IntEngBitBltEx(psoDest, NULL, psoMask, ClipRegion, DestColorTranslation,
                   DestRect, pptlMask, pptlMask, pbo, BrushOrigin,
                   R4_NOOP, FALSE);

    ret = EngMaskBitBlt(psoDest, psoMask, ClipRegion, DestColorTranslation, SourceColorTranslation,
                        &OutputRect, &InputPoint, pbo, BrushOrigin);

    /* Dummy BitBlt to let driver know that something has changed. */
    IntEngBitBltEx(psoDest, NULL, psoMask, ClipRegion, DestColorTranslation,
                   DestRect, pptlMask, pptlMask, pbo, BrushOrigin,
                   R4_NOOP, FALSE);

    MouseSafetyOnDrawEnd(psoDest);
    SURFACE_UnlockBitmapBits(psurfDest);

    return ret;
}
/* EOF */
