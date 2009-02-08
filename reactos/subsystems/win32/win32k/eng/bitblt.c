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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$
 *
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
                                        BRUSHOBJ* Brush,
                                        POINTL* BrushOrigin,
                                        ROP4 Rop4);
typedef BOOLEAN (APIENTRY *PSTRETCHRECTFUNC)(SURFOBJ* OutputObj,
                                            SURFOBJ* InputObj,
                                            SURFOBJ* Mask,
                                            CLIPOBJ* ClipRegion,
                                            XLATEOBJ* ColorTranslation,
                                            RECTL* OutputRect,
                                            RECTL* InputRect,
                                            POINTL* MaskOrigin,
                                            POINTL* BrushOrigin,
                                            ULONG Mode);

BOOL APIENTRY EngIntersectRect(RECTL* prcDst, RECTL* prcSrc1, RECTL* prcSrc2)
{
    static const RECTL rclEmpty = { 0, 0, 0, 0 };

    prcDst->left  = max(prcSrc1->left, prcSrc2->left);
    prcDst->right = min(prcSrc1->right, prcSrc2->right);

    if (prcDst->left < prcDst->right)
    {
        prcDst->top    = max(prcSrc1->top, prcSrc2->top);
        prcDst->bottom = min(prcSrc1->bottom, prcSrc2->bottom);

        if (prcDst->top < prcDst->bottom)
        {
            return TRUE;
        }
    }

    *prcDst = rclEmpty;

    return FALSE;
}

static BOOLEAN APIENTRY
BltMask(SURFOBJ* Dest,
        SURFOBJ* Source,
        SURFOBJ* Mask,
        XLATEOBJ* ColorTranslation,
        RECTL* DestRect,
        POINTL* SourcePoint,
        POINTL* MaskPoint,
        BRUSHOBJ* Brush,
        POINTL* BrushPoint,
        ROP4 Rop4)
{
    LONG i, j, dx, dy, c8;
    BYTE *tMask, *lMask;
    static BYTE maskbit[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
    /* Pattern brushes */
    PGDIBRUSHINST GdiBrush = NULL;
    SURFOBJ *psoPattern = NULL;
    PSURFACE psurfPattern;
    ULONG PatternWidth = 0, PatternHeight = 0, PatternY = 0;

    if (Mask == NULL)
    {
        return FALSE;
    }

    dx = DestRect->right  - DestRect->left;
    dy = DestRect->bottom - DestRect->top;

    if (Brush->iSolidColor == 0xFFFFFFFF)
    {
        GdiBrush = CONTAINING_RECORD(
                       Brush,
                       GDIBRUSHINST,
                       BrushObject);

        psurfPattern = SURFACE_LockSurface(GdiBrush->GdiBrushObject->hbmPattern);
        if (psurfPattern != NULL)
        {
            psoPattern = &psurfPattern->SurfObj;
            PatternWidth = psoPattern->sizlBitmap.cx;
            PatternHeight = psoPattern->sizlBitmap.cy;
        }
    }
    else
        psurfPattern = NULL;

    tMask = (PBYTE)Mask->pvScan0 + SourcePoint->y * Mask->lDelta + (SourcePoint->x >> 3);
    for (j = 0; j < dy; j++)
    {
        lMask = tMask;
        c8 = SourcePoint->x & 0x07;

        if (psurfPattern != NULL)
            PatternY = (DestRect->top + j) % PatternHeight;

        for (i = 0; i < dx; i++)
        {
            if (0 != (*lMask & maskbit[c8]))
            {
                if (psurfPattern == NULL)
                {
                    DibFunctionsForBitmapFormat[Dest->iBitmapFormat].DIB_PutPixel(
                        Dest, DestRect->left + i, DestRect->top + j, Brush->iSolidColor);
                }
                else
                {
                    DibFunctionsForBitmapFormat[Dest->iBitmapFormat].DIB_PutPixel(
                        Dest, DestRect->left + i, DestRect->top + j,
                        DIB_GetSource(psoPattern, (DestRect->left + i) % PatternWidth, PatternY, GdiBrush->XlateObject));
                }
            }
            c8++;
            if (8 == c8)
            {
                lMask++;
                c8 = 0;
            }
        }
        tMask += Mask->lDelta;
    }

    if (psurfPattern != NULL)
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
           BRUSHOBJ* Brush,
           POINTL* BrushPoint,
           ROP4 Rop4)
{
    // These functions are assigned if we're working with a DIB
    // The assigned functions depend on the bitsPerPixel of the DIB

    DibFunctionsForBitmapFormat[Dest->iBitmapFormat].DIB_ColorFill(Dest, DestRect, Brush->iSolidColor);

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
              BRUSHOBJ* Brush,
              POINTL* BrushOrigin,
              ROP4 Rop4)
{
    BLTINFO BltInfo;
    PGDIBRUSHINST GdiBrush = NULL;
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
    BltInfo.Brush = Brush;
    BltInfo.BrushOrigin = *BrushOrigin;
    BltInfo.Rop4 = Rop4;

    /* Pattern brush */
    if (ROP4_USES_PATTERN(Rop4) && Brush->iSolidColor == 0xFFFFFFFF)
    {
        GdiBrush = CONTAINING_RECORD(Brush, GDIBRUSHINST, BrushObject);
        if ((psurfPattern = SURFACE_LockSurface(GdiBrush->GdiBrushObject->hbmPattern)))
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
          BRUSHOBJ *Brush,
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
        if (Brush->iSolidColor == 0xFFFFFFFF)
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
                                 &OutputRect, &InputPoint, MaskOrigin, Brush,
                                 &AdjustedBrushOrigin, Rop4);
            break;
        case DC_RECT:
            /* Clip the blt to the clip rectangle */
            ClipRect.left = ClipRegion->rclBounds.left + Translate.x;
            ClipRect.right = ClipRegion->rclBounds.right + Translate.x;
            ClipRect.top = ClipRegion->rclBounds.top + Translate.y;
            ClipRect.bottom = ClipRegion->rclBounds.bottom + Translate.y;
            if (EngIntersectRect(&CombinedRect, &OutputRect, &ClipRect))
            {
                Pt.x = InputPoint.x + CombinedRect.left - OutputRect.left;
                Pt.y = InputPoint.y + CombinedRect.top - OutputRect.top;
                Ret = (*BltRectFunc)(OutputObj, InputObj, Mask, ColorTranslation,
                                     &CombinedRect, &Pt, MaskOrigin, Brush,
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
                    if (EngIntersectRect(&CombinedRect, &OutputRect, &ClipRect))
                    {
                        Pt.x = InputPoint.x + CombinedRect.left - OutputRect.left;
                        Pt.y = InputPoint.y + CombinedRect.top - OutputRect.top;
                        Ret = (*BltRectFunc)(OutputObj, InputObj, Mask,
                                             ColorTranslation, &CombinedRect, &Pt,
                                             MaskOrigin, Brush, &AdjustedBrushOrigin,
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
               BRUSHOBJ *Brush,
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
        if (! EngIntersectRect(&OutputRect, &InputClippedRect,
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
                  &OutputRect, &InputPoint, MaskOrigin, Brush, BrushOrigin,
                  Rop4);
    }

    if (! ret)
    {
        ret = EngBitBlt(psoDest, psoSource, MaskSurf, ClipRegion, ColorTranslation,
                        &OutputRect, &InputPoint, MaskOrigin, Brush, BrushOrigin,
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

static BOOLEAN APIENTRY
CallDibStretchBlt(SURFOBJ* psoDest,
                  SURFOBJ* psoSource,
                  SURFOBJ* Mask,
                  CLIPOBJ* ClipRegion,
                  XLATEOBJ* ColorTranslation,
                  RECTL* OutputRect,
                  RECTL* InputRect,
                  POINTL* MaskOrigin,
                  POINTL* BrushOrigin,
                  ULONG Mode)
{
    POINTL RealBrushOrigin;
    if (BrushOrigin == NULL)
    {
        RealBrushOrigin.x = RealBrushOrigin.y = 0;
    }
    else
    {
        RealBrushOrigin = *BrushOrigin;
    }
    return DibFunctionsForBitmapFormat[psoDest->iBitmapFormat].DIB_StretchBlt(
               psoDest, psoSource, OutputRect, InputRect, MaskOrigin, RealBrushOrigin, ClipRegion, ColorTranslation, Mode);
}


BOOL
APIENTRY
NtGdiEngStretchBlt(
    IN SURFOBJ  *psoDest,
    IN SURFOBJ  *psoSource,
    IN SURFOBJ  *Mask,
    IN CLIPOBJ  *ClipRegion,
    IN XLATEOBJ  *ColorTranslation,
    IN COLORADJUSTMENT  *pca,
    IN POINTL  *BrushOrigin,
    IN RECTL  *prclDest,
    IN RECTL  *prclSrc,
    IN POINTL  *MaskOrigin,
    IN ULONG  Mode
)
{
    COLORADJUSTMENT  ca;
    POINTL  lBrushOrigin;
    RECTL rclDest;
    RECTL rclSrc;
    POINTL lMaskOrigin;

    _SEH2_TRY
    {
        ProbeForRead(pca, sizeof(COLORADJUSTMENT), 1);
        RtlCopyMemory(&ca,pca, sizeof(COLORADJUSTMENT));

        ProbeForRead(BrushOrigin, sizeof(POINTL), 1);
        RtlCopyMemory(&lBrushOrigin, BrushOrigin, sizeof(POINTL));

        ProbeForRead(prclDest, sizeof(RECTL), 1);
        RtlCopyMemory(&rclDest, prclDest, sizeof(RECTL));

        ProbeForRead(prclSrc, sizeof(RECTL), 1);
        RtlCopyMemory(&rclSrc, prclSrc, sizeof(RECTL));

        ProbeForRead(MaskOrigin, sizeof(POINTL), 1);
        RtlCopyMemory(&lMaskOrigin, MaskOrigin, sizeof(POINTL));

    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return FALSE);
    }
    _SEH2_END;

    return EngStretchBlt(psoDest, psoSource, Mask, ClipRegion, ColorTranslation, &ca, &lBrushOrigin, &rclDest, &rclSrc, &lMaskOrigin, Mode);
}

BOOL
APIENTRY
EngStretchBlt(
    IN SURFOBJ  *psoDest,
    IN SURFOBJ  *psoSource,
    IN SURFOBJ  *Mask,
    IN CLIPOBJ  *ClipRegion,
    IN XLATEOBJ  *ColorTranslation,
    IN COLORADJUSTMENT  *pca,
    IN POINTL  *BrushOrigin,
    IN RECTL  *prclDest,
    IN RECTL  *prclSrc,
    IN POINTL  *MaskOrigin,
    IN ULONG  Mode
)
{
    // www.osr.com/ddk/graphics/gdifncs_0bs7.htm

    RECTL              InputRect;
    RECTL              OutputRect;
    POINTL             Translate;
    INTENG_ENTER_LEAVE EnterLeaveSource;
    INTENG_ENTER_LEAVE EnterLeaveDest;
    SURFOBJ*           psoInput;
    SURFOBJ*           psoOutput;
    PSTRETCHRECTFUNC   BltRectFunc;
    BOOLEAN            Ret;
    POINTL             AdjustedBrushOrigin;
    BOOL               UsesSource = ROP4_USES_SOURCE(Mode);

    if (Mode == R4_NOOP)
    {
        /* Copy destination onto itself: nop */
        return TRUE;
    }
   
    OutputRect = *prclDest;
    if (OutputRect.right < OutputRect.left)
    {
        OutputRect.left = prclDest->right;
        OutputRect.right = prclDest->left;
    }
    if (OutputRect.bottom < OutputRect.top)
    {
        OutputRect.left = prclDest->right;
        OutputRect.right = prclDest->left;
    }
    
    InputRect = *prclSrc;
    if (UsesSource)
    {
        if (NULL == prclSrc)
        {
            return FALSE;
        }

        /* Make sure we don't try to copy anything outside the valid source region */
        if (InputRect.left < 0)
        {
            OutputRect.left -= InputRect.left;
            InputRect.left = 0;
        }
        if (InputRect.top < 0)
        {
            OutputRect.top -= InputRect.top;
            InputRect.top = 0;
        }

        if (! IntEngEnter(&EnterLeaveSource, psoSource, &InputRect, TRUE,
                          &Translate, &psoInput))
        {
            return FALSE;
        }

        InputRect.left += Translate.x;
        InputRect.right += Translate.x;
        InputRect.top += Translate.y;
        InputRect.bottom += Translate.y;
    }

    if (NULL != ClipRegion)
    {
        if (OutputRect.left < ClipRegion->rclBounds.left)
        {
            InputRect.left += ClipRegion->rclBounds.left - OutputRect.left;
            OutputRect.left = ClipRegion->rclBounds.left;
        }
        if (ClipRegion->rclBounds.right < OutputRect.right)
        {
            InputRect.right -= OutputRect.right - ClipRegion->rclBounds.right;
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

    /* Check for degenerate case: if height or width of OutputRect is 0 pixels there's
       nothing to do */
    if (OutputRect.right <= OutputRect.left || OutputRect.bottom <= OutputRect.top)
    {
        if (UsesSource)
        {
            IntEngLeave(&EnterLeaveSource);
        }
        return TRUE;
    }

    if (! IntEngEnter(&EnterLeaveDest, psoDest, &OutputRect, FALSE, &Translate, &psoOutput))
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

    if (Mask != NULL)
    {
        //BltRectFunc = BltMask;
        DPRINT("EngStretchBlt isn't capable of handling mask yet.\n");
        IntEngLeave(&EnterLeaveDest);
        if (UsesSource)
        {
            IntEngLeave(&EnterLeaveSource);
        }
        return FALSE;
    }
    else
    {
        BltRectFunc = CallDibStretchBlt;
    }

    Ret = (*BltRectFunc)(psoOutput, psoInput, Mask, ClipRegion,
                         ColorTranslation, &OutputRect, &InputRect, MaskOrigin,
                         &AdjustedBrushOrigin, Mode);

    IntEngLeave(&EnterLeaveDest);
    if (UsesSource)
    {
        IntEngLeave(&EnterLeaveSource);
    }

    return Ret;
}

BOOL APIENTRY
IntEngStretchBlt(SURFOBJ *psoDest,
                 SURFOBJ *psoSource,
                 SURFOBJ *MaskSurf,
                 CLIPOBJ *ClipRegion,
                 XLATEOBJ *ColorTranslation,
                 RECTL *DestRect,
                 RECTL *SourceRect,
                 POINTL *pMaskOrigin,
                 BRUSHOBJ *Brush,
                 POINTL *BrushOrigin,
                 ROP4 ROP)
{
    BOOLEAN ret;
    COLORADJUSTMENT ca;
    POINT MaskOrigin;
    SURFACE *psurfDest;
    SURFACE *psurfSource = NULL;
    RECTL InputClippedRect;
    RECTL InputRect;
    RECTL OutputRect;
    BOOL UsesSource = ROP4_USES_SOURCE(ROP);
    LONG InputClWidth, InputClHeight, InputWidth, InputHeight;

    ASSERT(psoDest);
    psurfDest = CONTAINING_RECORD(psoDest, SURFACE, SurfObj);
    ASSERT(psurfDest);
    ASSERT(DestRect);

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
    
    if (UsesSource)
    {
        if (NULL == SourceRect || NULL == psoSource)
        {
            return FALSE;
        }
        InputRect = *SourceRect;
 
        /* Make sure we don't try to copy anything outside the valid source region */
        if (InputRect.left < 0)
        {
            InputClippedRect.left -= InputRect.left;
            InputRect.left = 0;
        }
        if (InputRect.top < 0)
        {
            InputClippedRect.top -= InputRect.top;
            InputRect.top = 0;
        }

        if (InputClippedRect.right < InputClippedRect.left ||
                InputClippedRect.bottom < InputClippedRect.top)
        {
            /* Everything clipped away, nothing to do */
            return TRUE;
        }
    }    

    if (ClipRegion)
    {
        if (! EngIntersectRect(&OutputRect, &InputClippedRect,
                               &ClipRegion->rclBounds))
        {
            return TRUE;
        }
        /* Update source rect */
        InputClWidth = InputClippedRect.right - InputClippedRect.left;
        InputClHeight = InputClippedRect.bottom - InputClippedRect.top;
        InputWidth = InputRect.right - InputRect.left;
        InputHeight = InputRect.bottom - InputRect.top;

        InputRect.left += (InputWidth * (OutputRect.left - InputClippedRect.left)) / InputClWidth;
        InputRect.right -= (InputWidth * (InputClippedRect.right - OutputRect.right)) / InputClWidth;
        InputRect.top += (InputHeight * (OutputRect.top - InputClippedRect.top)) / InputClHeight;
        InputRect.bottom -= (InputHeight * (InputClippedRect.bottom - OutputRect.bottom)) / InputClHeight;
    }
    else
    {
        OutputRect = InputClippedRect;
    }
    
    if (pMaskOrigin != NULL)
    {
        MaskOrigin.x = pMaskOrigin->x; MaskOrigin.y = pMaskOrigin->y;
    }

    /* No success yet */
    ret = FALSE;
    SURFACE_LockBitmapBits(psurfDest);
    MouseSafetyOnDrawStart(psoDest, OutputRect.left, OutputRect.top,
                           OutputRect.right, OutputRect.bottom);

    if (UsesSource)
    {
        psurfSource = CONTAINING_RECORD(psoSource, SURFACE, SurfObj);
        if (psoSource != psoDest)
        {
            SURFACE_LockBitmapBits(psurfSource);
        }
        MouseSafetyOnDrawStart(psoSource, InputRect.left, InputRect.top,
                               InputRect.right, InputRect.bottom);
    }

    /* Prepare color adjustment */

    /* Call the driver's DrvStretchBlt if available */
    if (psurfDest->flHooks & HOOK_STRETCHBLT)
    {
        /* Drv->StretchBlt (look at http://www.osr.com/ddk/graphics/ddifncs_3ew7.htm ) */
        // FIXME: MaskOrigin is always NULL !
        ret = GDIDEVFUNCS(psoDest).StretchBlt(
                  psoDest, (UsesSource) ? psoSource : NULL, MaskSurf, ClipRegion, ColorTranslation,
                  &ca, BrushOrigin, &OutputRect, &InputRect, NULL, ROP);
    }

    if (! ret)
    {
        // FIXME: see previous fixme
        ret = EngStretchBlt(psoDest, psoSource, MaskSurf, ClipRegion, ColorTranslation,
                            &ca, BrushOrigin, &OutputRect, &InputRect, NULL, ROP);
    }

    if (UsesSource)
    {
        MouseSafetyOnDrawEnd(psoSource);
        if (psoSource != psoDest)
        {
            SURFACE_UnlockBitmapBits(psurfSource);
        }
    }
    MouseSafetyOnDrawEnd(psoDest);
    SURFACE_UnlockBitmapBits(psurfDest);

    return ret;
}


/*
 * @implemented
 */
BOOL
APIENTRY
NtGdiEngAlphaBlend(IN SURFOBJ *psoDest,
                   IN SURFOBJ *psoSource,
                   IN CLIPOBJ *ClipRegion,
                   IN XLATEOBJ *ColorTranslation,
                   IN PRECTL upDestRect,
                   IN PRECTL upSourceRect,
                   IN BLENDOBJ *BlendObj)
{
    RECTL DestRect;
    RECTL SourceRect;

    _SEH2_TRY
    {
        ProbeForRead(upDestRect, sizeof(RECTL), 1);
        RtlCopyMemory(&DestRect,upDestRect, sizeof(RECTL));

        ProbeForRead(upSourceRect, sizeof(RECTL), 1);
        RtlCopyMemory(&SourceRect, upSourceRect, sizeof(RECTL));

    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return FALSE);
    }
    _SEH2_END;

    return EngAlphaBlend(psoDest, psoSource, ClipRegion, ColorTranslation, &DestRect, &SourceRect, BlendObj);
}

/*
 * @implemented
 */
BOOL
APIENTRY
EngAlphaBlend(IN SURFOBJ *psoDest,
              IN SURFOBJ *psoSource,
              IN CLIPOBJ *ClipRegion,
              IN XLATEOBJ *ColorTranslation,
              IN PRECTL DestRect,
              IN PRECTL SourceRect,
              IN BLENDOBJ *BlendObj)
{
    RECTL              SourceStretchedRect;
    SIZEL              SourceStretchedSize;
    HBITMAP            SourceStretchedBitmap = 0;
    SURFOBJ*           SourceStretchedObj = NULL;
    RECTL              InputRect;
    RECTL              OutputRect;
    RECTL              ClipRect;
    RECTL              CombinedRect;
    RECTL              Rect;
    POINTL             Translate;
    INTENG_ENTER_LEAVE EnterLeaveSource;
    INTENG_ENTER_LEAVE EnterLeaveDest;
    SURFOBJ*           InputObj;
    SURFOBJ*           OutputObj;
    LONG               Width;
    LONG               ClippingType;
    RECT_ENUM          RectEnum;
    BOOL               EnumMore;
    INT                i;
    BOOLEAN            Ret;

    DPRINT("EngAlphaBlend(psoDest:0x%p, psoSource:0x%p, ClipRegion:0x%p, ColorTranslation:0x%p,\n", psoDest, psoSource, ClipRegion, ColorTranslation);
    DPRINT("              DestRect:{0x%x, 0x%x, 0x%x, 0x%x}, SourceRect:{0x%x, 0x%x, 0x%x, 0x%x},\n",
           DestRect->left, DestRect->top, DestRect->right, DestRect->bottom,
           SourceRect->left, SourceRect->top, SourceRect->right, SourceRect->bottom);
    DPRINT("              BlendObj:{0x%x, 0x%x, 0x%x, 0x%x}\n", BlendObj->BlendFunction.BlendOp,
           BlendObj->BlendFunction.BlendFlags, BlendObj->BlendFunction.SourceConstantAlpha,
           BlendObj->BlendFunction.AlphaFormat);


    /* Validate output */
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


    /* Validate input */

    /* FIXME when WindowOrg.x or .y are negitve this check are not vaild, 
     * we need convert the inputRect to the windows org and do it right */
    InputRect = *SourceRect;
    if ( (InputRect.top < 0) || (InputRect.bottom < 0) ||
         (InputRect.left < 0) || (InputRect.right < 0) )
    {
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (psoDest == psoSource &&
            !(OutputRect.left >= SourceRect->right || InputRect.left >= OutputRect.right ||
              OutputRect.top >= SourceRect->bottom || InputRect.top >= OutputRect.bottom))
    {
        DPRINT1("Source and destination rectangles overlap!\n");
        return FALSE;
    }

    if (BlendObj->BlendFunction.BlendOp != AC_SRC_OVER)
    {
        DPRINT1("BlendOp != AC_SRC_OVER (0x%x)\n", BlendObj->BlendFunction.BlendOp);
        return FALSE;
    }
    if (BlendObj->BlendFunction.BlendFlags != 0)
    {
        DPRINT1("BlendFlags != 0 (0x%x)\n", BlendObj->BlendFunction.BlendFlags);
        return FALSE;
    }
    if ((BlendObj->BlendFunction.AlphaFormat & ~AC_SRC_ALPHA) != 0)
    {
        DPRINT1("Unsupported AlphaFormat (0x%x)\n", BlendObj->BlendFunction.AlphaFormat);
        return FALSE;
    }

    /* Check if there is anything to draw */
    if (ClipRegion != NULL &&
            (ClipRegion->rclBounds.left >= ClipRegion->rclBounds.right ||
             ClipRegion->rclBounds.top >= ClipRegion->rclBounds.bottom))
    {
        /* Nothing to do */
        return TRUE;
    }

    /* Stretch source if needed */
    if (OutputRect.right - OutputRect.left != InputRect.right - InputRect.left ||
            OutputRect.bottom - OutputRect.top != InputRect.bottom - InputRect.top)
    {
        SourceStretchedSize.cx = OutputRect.right - OutputRect.left;
        SourceStretchedSize.cy = OutputRect.bottom - OutputRect.top;
        Width = DIB_GetDIBWidthBytes(SourceStretchedSize.cx, BitsPerFormat(psoSource->iBitmapFormat));
        /* FIXME: Maybe it is a good idea to use EngCreateDeviceBitmap and IntEngStretchBlt
                  if possible to get a HW accelerated stretch. */
        SourceStretchedBitmap = EngCreateBitmap(SourceStretchedSize, Width, psoSource->iBitmapFormat,
                                                BMF_TOPDOWN | BMF_NOZEROINIT, NULL);
        if (SourceStretchedBitmap == 0)
        {
            DPRINT1("EngCreateBitmap failed!\n");
            return FALSE;
        }
        SourceStretchedObj = EngLockSurface((HSURF)SourceStretchedBitmap);
        if (SourceStretchedObj == NULL)
        {
            DPRINT1("EngLockSurface failed!\n");
            EngDeleteSurface((HSURF)SourceStretchedBitmap);
            return FALSE;
        }

        SourceStretchedRect.left = 0;
        SourceStretchedRect.right = SourceStretchedSize.cx;
        SourceStretchedRect.top = 0;
        SourceStretchedRect.bottom = SourceStretchedSize.cy;
        /* FIXME: IntEngStretchBlt isn't used here atm because it results in a
                  try to acquire an already acquired mutex (lock the already locked source surface) */
        /*if (!IntEngStretchBlt(SourceStretchedObj, psoSource, NULL, NULL,
                              NULL, &SourceStretchedRect, SourceRect, NULL,
                              NULL, NULL, COLORONCOLOR))*/
        if (!EngStretchBlt(SourceStretchedObj, psoSource, NULL, NULL, NULL,
                           NULL, NULL, &SourceStretchedRect, &InputRect,
                           NULL, COLORONCOLOR))
        {
            DPRINT1("EngStretchBlt failed!\n");
            EngFreeMem(SourceStretchedObj->pvBits);
            EngUnlockSurface(SourceStretchedObj);
            EngDeleteSurface((HSURF)SourceStretchedBitmap);
            return FALSE;
        }
        InputRect.top = SourceStretchedRect.top;
        InputRect.bottom = SourceStretchedRect.bottom;
        InputRect.left = SourceStretchedRect.left;
        InputRect.right = SourceStretchedRect.right;
        psoSource = SourceStretchedObj;
    }

    /* Now call the DIB function */
    if (!IntEngEnter(&EnterLeaveSource, psoSource, &InputRect, TRUE, &Translate, &InputObj))
    {
        if (SourceStretchedObj != NULL)
        {
            EngFreeMem(SourceStretchedObj->pvBits);
            EngUnlockSurface(SourceStretchedObj);
        }
        if (SourceStretchedBitmap != 0)
        {
            EngDeleteSurface((HSURF)SourceStretchedBitmap);
        }
        return FALSE;
    }
    InputRect.left +=  Translate.x;
    InputRect.right +=  Translate.x;
    InputRect.top +=  Translate.y;
    InputRect.bottom +=  Translate.y;

    if (!IntEngEnter(&EnterLeaveDest, psoDest, &OutputRect, FALSE, &Translate, &OutputObj))
    {
        IntEngLeave(&EnterLeaveSource);
        if (SourceStretchedObj != NULL)
        {
            EngFreeMem(SourceStretchedObj->pvBits);
            EngUnlockSurface(SourceStretchedObj);
        }
        if (SourceStretchedBitmap != 0)
        {
            EngDeleteSurface((HSURF)SourceStretchedBitmap);
        }
        return FALSE;
    }
    OutputRect.left += Translate.x;
    OutputRect.right += Translate.x;
    OutputRect.top += Translate.y;
    OutputRect.bottom += Translate.y;

    Ret = FALSE;
    ClippingType = (ClipRegion == NULL) ? DC_TRIVIAL : ClipRegion->iDComplexity;
    switch (ClippingType)
    {
        case DC_TRIVIAL:
            Ret = DibFunctionsForBitmapFormat[OutputObj->iBitmapFormat].DIB_AlphaBlend(
                      OutputObj, InputObj, &OutputRect, &InputRect, ClipRegion, ColorTranslation, BlendObj);
            break;

        case DC_RECT:
            ClipRect.left = ClipRegion->rclBounds.left + Translate.x;
            ClipRect.right = ClipRegion->rclBounds.right + Translate.x;
            ClipRect.top = ClipRegion->rclBounds.top + Translate.y;
            ClipRect.bottom = ClipRegion->rclBounds.bottom + Translate.y;
            if (EngIntersectRect(&CombinedRect, &OutputRect, &ClipRect))
            {
                Rect.left = InputRect.left + CombinedRect.left - OutputRect.left;
                Rect.right = InputRect.right + CombinedRect.right - OutputRect.right;
                Rect.top = InputRect.top + CombinedRect.top - OutputRect.top;
                Rect.bottom = InputRect.bottom + CombinedRect.bottom - OutputRect.bottom;
                Ret = DibFunctionsForBitmapFormat[OutputObj->iBitmapFormat].DIB_AlphaBlend(
                          OutputObj, InputObj, &CombinedRect, &Rect, ClipRegion, ColorTranslation, BlendObj);
            }
            break;

        case DC_COMPLEX:
            Ret = TRUE;
            CLIPOBJ_cEnumStart(ClipRegion, FALSE, CT_RECTANGLES, CD_ANY, 0);
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
                    if (EngIntersectRect(&CombinedRect, &OutputRect, &ClipRect))
                    {
                        Rect.left = InputRect.left + CombinedRect.left - OutputRect.left;
                        Rect.right = InputRect.right + CombinedRect.right - OutputRect.right;
                        Rect.top = InputRect.top + CombinedRect.top - OutputRect.top;
                        Rect.bottom = InputRect.bottom + CombinedRect.bottom - OutputRect.bottom;
                        Ret = DibFunctionsForBitmapFormat[OutputObj->iBitmapFormat].DIB_AlphaBlend(
                                  OutputObj, InputObj, &CombinedRect, &Rect, ClipRegion, ColorTranslation, BlendObj) && Ret;
                    }
                }
            }
            while (EnumMore);
            break;

        default:
            UNIMPLEMENTED;
            ASSERT(FALSE);
            break;
    }

    IntEngLeave(&EnterLeaveDest);
    IntEngLeave(&EnterLeaveSource);

    if (SourceStretchedObj != NULL)
    {
        EngFreeMem(SourceStretchedObj->pvBits);
        EngUnlockSurface(SourceStretchedObj);
    }
    if (SourceStretchedBitmap != 0)
    {
        EngDeleteSurface((HSURF)SourceStretchedBitmap);
    }

    return Ret;
}

BOOL APIENTRY
IntEngAlphaBlend(IN SURFOBJ *psoDest,
                 IN SURFOBJ *psoSource,
                 IN CLIPOBJ *ClipRegion,
                 IN XLATEOBJ *ColorTranslation,
                 IN PRECTL DestRect,
                 IN PRECTL SourceRect,
                 IN BLENDOBJ *BlendObj)
{
    BOOL ret = FALSE;
    SURFACE *psurfDest;
    SURFACE *psurfSource;

    ASSERT(psoDest);
    psurfDest = CONTAINING_RECORD(psoDest, SURFACE, SurfObj);

    ASSERT(psoSource);
    psurfSource = CONTAINING_RECORD(psoSource, SURFACE, SurfObj);

    ASSERT(DestRect);
    ASSERT(SourceRect);

    /* Check if there is anything to draw */
    if (ClipRegion != NULL &&
            (ClipRegion->rclBounds.left >= ClipRegion->rclBounds.right ||
             ClipRegion->rclBounds.top >= ClipRegion->rclBounds.bottom))
    {
        /* Nothing to do */
        return TRUE;
    }

    SURFACE_LockBitmapBits(psurfDest);
    MouseSafetyOnDrawStart(psoDest, DestRect->left, DestRect->top,
                           DestRect->right, DestRect->bottom);

    if (psoSource != psoDest)
        SURFACE_LockBitmapBits(psurfSource);
    MouseSafetyOnDrawStart(psoSource, SourceRect->left, SourceRect->top,
                           SourceRect->right, SourceRect->bottom);

    /* Call the driver's DrvAlphaBlend if available */
    if (psurfDest->flHooks & HOOK_ALPHABLEND)
    {
        ret = GDIDEVFUNCS(psoDest).AlphaBlend(
                  psoDest, psoSource, ClipRegion, ColorTranslation,
                  DestRect, SourceRect, BlendObj);
    }

    if (! ret)
    {
        ret = EngAlphaBlend(psoDest, psoSource, ClipRegion, ColorTranslation,
                            DestRect, SourceRect, BlendObj);
    }

    MouseSafetyOnDrawEnd(psoSource);
    if (psoSource != psoDest)
        SURFACE_UnlockBitmapBits(psurfSource);
    MouseSafetyOnDrawEnd(psoDest);
    SURFACE_UnlockBitmapBits(psurfDest);

    return ret;
}

/**** REACTOS FONT RENDERING CODE *********************************************/

/* renders the alpha mask bitmap */
static BOOLEAN APIENTRY
AlphaBltMask(SURFOBJ* psoDest,
             SURFOBJ* psoSource,
             SURFOBJ* Mask,
             XLATEOBJ* ColorTranslation,
             XLATEOBJ* SrcColorTranslation,
             RECTL* DestRect,
             POINTL* SourcePoint,
             POINTL* MaskPoint,
             BRUSHOBJ* Brush,
             POINTL* BrushPoint)
{
    LONG i, j, dx, dy;
    int r, g, b;
    ULONG Background, BrushColor, NewColor;
    BYTE *tMask, *lMask;

    dx = DestRect->right  - DestRect->left;
    dy = DestRect->bottom - DestRect->top;

    if (Mask != NULL)
    {
        BrushColor = XLATEOBJ_iXlate(SrcColorTranslation, Brush->iSolidColor);
        r = (int)GetRValue(BrushColor);
        g = (int)GetGValue(BrushColor);
        b = (int)GetBValue(BrushColor);

        tMask = (PBYTE)Mask->pvScan0 + (SourcePoint->y * Mask->lDelta) + SourcePoint->x;
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
                            psoDest, DestRect->left + i, DestRect->top + j, Brush->iSolidColor);
                    }
                    else
                    {
                        Background = DIB_GetSource(psoDest, DestRect->left + i, DestRect->top + j,
                                                   SrcColorTranslation);

                        NewColor =
                            RGB((*lMask * (r - GetRValue(Background)) >> 8) + GetRValue(Background),
                                (*lMask * (g - GetGValue(Background)) >> 8) + GetGValue(Background),
                                (*lMask * (b - GetBValue(Background)) >> 8) + GetBValue(Background));

                        Background = XLATEOBJ_iXlate(ColorTranslation, NewColor);
                        DibFunctionsForBitmapFormat[psoDest->iBitmapFormat].DIB_PutPixel(
                            psoDest, DestRect->left + i, DestRect->top + j, Background);
                    }
                }
                lMask++;
            }
            tMask += Mask->lDelta;
        }
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL APIENTRY
EngMaskBitBlt(SURFOBJ *psoDest,
              SURFOBJ *psoMask,
              CLIPOBJ *ClipRegion,
              XLATEOBJ *DestColorTranslation,
              XLATEOBJ *SourceColorTranslation,
              RECTL *DestRect,
              POINTL *SourcePoint,
              POINTL *MaskOrigin,
              BRUSHOBJ *Brush,
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

    if (NULL != SourcePoint)
    {
        InputRect.left = SourcePoint->x;
        InputRect.right = SourcePoint->x + (DestRect->right - DestRect->left);
        InputRect.top = SourcePoint->y;
        InputRect.bottom = SourcePoint->y + (DestRect->bottom - DestRect->top);
    }
    else
    {
        InputRect.left = 0;
        InputRect.right = DestRect->right - DestRect->left;
        InputRect.top = 0;
        InputRect.bottom = DestRect->bottom - DestRect->top;
    }

    if (! IntEngEnter(&EnterLeaveSource, psoDest, &InputRect, TRUE, &Translate, &psoInput))
    {
        return FALSE;
    }

    if (NULL != SourcePoint)
    {
        InputPoint.x = SourcePoint->x + Translate.x;
        InputPoint.y = SourcePoint->y + Translate.y;
    }
    else
    {
        InputPoint.x = 0;
        InputPoint.y = 0;
    }

    OutputRect = *DestRect;
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
                Ret = AlphaBltMask(psoOutput, psoInput, psoMask, DestColorTranslation, SourceColorTranslation,
                                   &OutputRect, &InputPoint, MaskOrigin, Brush, &AdjustedBrushOrigin);
            else
                Ret = BltMask(psoOutput, psoInput, psoMask, DestColorTranslation,
                              &OutputRect, &InputPoint, MaskOrigin, Brush, &AdjustedBrushOrigin,
                              R4_MASK);
            break;
        case DC_RECT:
            // Clip the blt to the clip rectangle
            ClipRect.left = ClipRegion->rclBounds.left + Translate.x;
            ClipRect.right = ClipRegion->rclBounds.right + Translate.x;
            ClipRect.top = ClipRegion->rclBounds.top + Translate.y;
            ClipRect.bottom = ClipRegion->rclBounds.bottom + Translate.y;
            if (EngIntersectRect(&CombinedRect, &OutputRect, &ClipRect))
            {
                Pt.x = InputPoint.x + CombinedRect.left - OutputRect.left;
                Pt.y = InputPoint.y + CombinedRect.top - OutputRect.top;
                if (psoMask->iBitmapFormat == BMF_8BPP)
                {
                    Ret = AlphaBltMask(psoOutput, psoInput, psoMask, DestColorTranslation, SourceColorTranslation,
                                       &CombinedRect, &Pt, MaskOrigin, Brush, &AdjustedBrushOrigin);
                }
                else
                {
                    Ret = BltMask(psoOutput, psoInput, psoMask, DestColorTranslation,
                                  &CombinedRect, &Pt, MaskOrigin, Brush, &AdjustedBrushOrigin, R4_MASK);
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
                    if (EngIntersectRect(&CombinedRect, &OutputRect, &ClipRect))
                    {
                        Pt.x = InputPoint.x + CombinedRect.left - OutputRect.left;
                        Pt.y = InputPoint.y + CombinedRect.top - OutputRect.top;
                        if (psoMask->iBitmapFormat == BMF_8BPP)
                        {
                            Ret = AlphaBltMask(psoOutput, psoInput, psoMask,
                                               DestColorTranslation,
                                               SourceColorTranslation,
                                               &CombinedRect, &Pt, MaskOrigin, Brush,
                                               &AdjustedBrushOrigin) && Ret;
                        }
                        else
                        {
                            Ret = BltMask(psoOutput, psoInput, psoMask,
                                          DestColorTranslation, &CombinedRect, &Pt,
                                          MaskOrigin, Brush, &AdjustedBrushOrigin,
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
              POINTL *SourcePoint,
              POINTL *MaskOrigin,
              BRUSHOBJ *Brush,
              POINTL *BrushOrigin)
{
    BOOLEAN ret;
    RECTL OutputRect;
    POINTL InputPoint;
    SURFACE *psurfDest;

    ASSERT(psoMask);

    if (NULL != SourcePoint)
    {
        InputPoint = *SourcePoint;
    }

    /* Clip against the bounds of the clipping region so we won't try to write
     * outside the surface */
    if (NULL != ClipRegion)
    {
        if (! EngIntersectRect(&OutputRect, DestRect, &ClipRegion->rclBounds))
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
                   DestRect, SourcePoint, MaskOrigin, Brush, BrushOrigin,
                   R4_NOOP, FALSE);

    ret = EngMaskBitBlt(psoDest, psoMask, ClipRegion, DestColorTranslation, SourceColorTranslation,
                        &OutputRect, &InputPoint, MaskOrigin, Brush, BrushOrigin);

    /* Dummy BitBlt to let driver know that something has changed. */
    IntEngBitBltEx(psoDest, NULL, psoMask, ClipRegion, DestColorTranslation,
                   DestRect, SourcePoint, MaskOrigin, Brush, BrushOrigin,
                   R4_NOOP, FALSE);

    MouseSafetyOnDrawEnd(psoDest);
    SURFACE_UnlockBitmapBits(psurfDest);

    return ret;
}
/* EOF */
