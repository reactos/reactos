/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          GDI stretch blt functions
 * FILE:             win32ss/gdi/eng/stretchblt.c
 * PROGRAMERS:       Jason Filby
 *                   Doug Lyons
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

/***************************************************************************************************************************
  We want to receive and send the flip state along to existing functions without changing their parameter lists.
  So a way that we can do this is to use the DestRect to carry this information along with it.
  Since there are four values, we can use their relative positions (coordinates) to indicate the four flip conditions.
  if delta-x == 0 then there can be no Left-to-Right flip. If delta-y = 0 there can be no Top-to-Bottom flip.
  So we can set the four flip conditions based on BOOLEAN flags as follows:

  We will use internal bits and BltInfo->bTopToBottom and BltInfo->bLeftToRight as follows:

  !bTopToBottom && !bLeftToRight means no flips      therefore left < right and top < bottom (normal well-formed rect)
  bTopToBottom means there is a Top-To-Bottom flip   therefore left < right and top > bottom
  bLeftToRight means there is a Left-To-Right flip   therefore left > right and top < bottom
  bLeftToRight && bTopToBottom means both flips      therefore left > right and top > bottom
****************************************************************************************************************************/


typedef BOOLEAN (APIENTRY *PSTRETCHRECTFUNC)(SURFOBJ* OutputObj,
                                            SURFOBJ* InputObj,
                                            SURFOBJ* Mask,
                                            XLATEOBJ* ColorTranslation,
                                            RECTL* OutputRect,
                                            RECTL* InputRect,
                                            POINTL* MaskOrigin,
                                            BRUSHOBJ* pbo,
                                            POINTL* BrushOrigin,
                                            ROP4 Rop4);

static BOOLEAN APIENTRY
CallDibStretchBlt(SURFOBJ* psoDest,
                  SURFOBJ* psoSource,
                  SURFOBJ* Mask,
                  XLATEOBJ* ColorTranslation,
                  RECTL* OutputRect,
                  RECTL* InputRect,
                  POINTL* MaskOrigin,
                  BRUSHOBJ* pbo,
                  POINTL* BrushOrigin,
                  ROP4 Rop4)
{
    POINTL RealBrushOrigin;
    SURFOBJ* psoPattern;
    BOOL bResult;

    DPRINT("Entering CallDibStretchBlt: psoSource cx/cy (%d/%d), psoDest cx/cy (%d/%d) OutputRect: (%d,%d)-(%d,%d)\n",
        psoSource->sizlBitmap.cx, psoSource->sizlBitmap.cy,
        psoDest->sizlBitmap.cx, psoDest->sizlBitmap.cy,
        OutputRect->left, OutputRect->top, OutputRect->right, OutputRect->bottom);

    if (BrushOrigin == NULL)
    {
        RealBrushOrigin.x = RealBrushOrigin.y = 0;
    }
    else
    {
        RealBrushOrigin = *BrushOrigin;
    }

    /* Pattern brush */
    if (ROP4_USES_PATTERN(Rop4) && pbo && pbo->iSolidColor == 0xFFFFFFFF)
    {
        psoPattern = BRUSHOBJ_psoPattern(pbo);

        if (!psoPattern) return FALSE;
    }
    else
    {
        psoPattern = NULL;
    }

    bResult = DibFunctionsForBitmapFormat[psoDest->iBitmapFormat].DIB_StretchBlt(
               psoDest, psoSource, Mask, psoPattern,
               OutputRect, InputRect, MaskOrigin, pbo, &RealBrushOrigin,
               ColorTranslation, Rop4);

    return bResult;
}



/*
 * @implemented
 */
BOOL
APIENTRY
EngStretchBltROP(
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
    IN ULONG  Mode,
    IN BRUSHOBJ *pbo,
    IN ROP4 Rop4)
{
    RECTL              InputRect;
    RECTL              OutputRect;
    POINTL             Translate;
    INTENG_ENTER_LEAVE EnterLeaveSource;
    INTENG_ENTER_LEAVE EnterLeaveDest;
    SURFOBJ*           psoInput;
    SURFOBJ*           psoOutput;
    PSTRETCHRECTFUNC   BltRectFunc;
    BOOLEAN            Ret = TRUE;
    POINTL             AdjustedBrushOrigin;
    BOOL               UsesSource = ROP4_USES_SOURCE(Rop4);

    BYTE               clippingType;
    RECTL              ClipRect;
    RECT_ENUM          RectEnum;
    BOOL               EnumMore;
    ULONG              Direction;
    RECTL              CombinedRect;
    RECTL              InputToCombinedRect;
    unsigned           i;

    LONG DstHeight;
    LONG DstWidth;
    LONG SrcHeight;
    LONG SrcWidth;

    LONG cxSrc, cySrc, cxDest, cyDest;
    BOOLEAN bLeftToRight, bTopToBottom;
    LONG lTmp;

    DPRINT("Entering EngStretchBltROP: SrcSurf cx/cy (%d/%d), DestSuft cx/cy (%d/%d) dstRect: (%d,%d)-(%d,%d)\n",
        psoSource->sizlBitmap.cx, psoSource->sizlBitmap.cy, //SourceRect->right, SourceRect->bottom,
        psoDest->sizlBitmap.cx, psoDest->sizlBitmap.cy,
        prclDest->left, prclDest->top, prclDest->right, prclDest->bottom);

    cxSrc = prclSrc->right - prclSrc->left;
    cySrc = prclSrc->bottom - prclSrc->top;
    cxDest = prclDest->right - prclDest->left;
    cyDest = prclDest->bottom - prclDest->top;

    /* Here we do the tests and set our conditions */
    if (((cxSrc < 0) && (cxDest < 0)) || ((cxSrc >= 0) && (cxDest >= 0)))
        bLeftToRight = FALSE;
    else
        bLeftToRight = TRUE;

    if (((cySrc < 0) && (cyDest < 0)) || ((cySrc >= 0) && (cyDest >= 0)))
        bTopToBottom = FALSE;
    else
        bTopToBottom = TRUE;

    /* Make Well Ordered to start */
    OutputRect = *prclDest;
    RECTL_vMakeWellOrdered(&OutputRect);
    *prclDest = OutputRect;

    if (Rop4 == ROP4_NOOP)
    {
        /* Copy destination onto itself: nop */
        return TRUE;
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

    OutputRect = *prclDest;
    if (OutputRect.right < OutputRect.left)
    {
        OutputRect.left = prclDest->right;
        OutputRect.right = prclDest->left;
    }
    if (OutputRect.bottom < OutputRect.top)
    {
        OutputRect.top = prclDest->bottom;
        OutputRect.bottom = prclDest->top;
    }

    if (UsesSource)
    {
        if (NULL == prclSrc)
        {
            return FALSE;
        }
        InputRect = *prclSrc;

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
    else
    {
        InputRect.left = 0;
        InputRect.right = OutputRect.right - OutputRect.left;
        InputRect.top = 0;
        InputRect.bottom = OutputRect.bottom - OutputRect.top;
        psoInput = NULL;
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

    BltRectFunc = CallDibStretchBlt;

    DstHeight = OutputRect.bottom - OutputRect.top;
    DstWidth = OutputRect.right - OutputRect.left;
    SrcHeight = InputRect.bottom - InputRect.top;
    SrcWidth = InputRect.right - InputRect.left;

    DPRINT("bLeftToRight is '%d' and bTopToBottom is '%d'.\n", bLeftToRight, bTopToBottom);

    switch (clippingType)
    {
        case DC_TRIVIAL:
            if (bLeftToRight)
            {
               lTmp = OutputRect.left;
                OutputRect.left = OutputRect.right;
                OutputRect.right = lTmp;
            }

            if (bTopToBottom)
            {
                lTmp = OutputRect.top;
                OutputRect.top = OutputRect.bottom;
                OutputRect.bottom = lTmp;
            }

            DPRINT("About to call CallDibStretchBlt?: OutputRect: (%d,%d)-(%d,%d)\n",
                   OutputRect.left, OutputRect.top, OutputRect.right, OutputRect.bottom);

            Ret = (*BltRectFunc)(psoOutput, psoInput, Mask,
                         ColorTranslation, &OutputRect, &InputRect, MaskOrigin,
                         pbo, &AdjustedBrushOrigin, Rop4);
            break;
        case DC_RECT:
            // Clip the blt to the clip rectangle
            ClipRect.left = ClipRegion->rclBounds.left + Translate.x;
            ClipRect.right = ClipRegion->rclBounds.right + Translate.x;
            ClipRect.top = ClipRegion->rclBounds.top + Translate.y;
            ClipRect.bottom = ClipRegion->rclBounds.bottom + Translate.y;
            if (RECTL_bIntersectRect(&CombinedRect, &OutputRect, &ClipRect))
            {
                InputToCombinedRect.top = InputRect.top + (CombinedRect.top - OutputRect.top) * SrcHeight / DstHeight;
                InputToCombinedRect.bottom = InputRect.top + (CombinedRect.bottom - OutputRect.top) * SrcHeight / DstHeight;
                InputToCombinedRect.left = InputRect.left + (CombinedRect.left - OutputRect.left) * SrcWidth / DstWidth;
                InputToCombinedRect.right = InputRect.left + (CombinedRect.right - OutputRect.left) * SrcWidth / DstWidth;

                if (bLeftToRight)
                {
                   lTmp = CombinedRect.left;
                    CombinedRect.left = CombinedRect.right;
                    CombinedRect.right = lTmp;
                }

                if (bTopToBottom)
                {
                    lTmp = CombinedRect.top;
                    CombinedRect.top = CombinedRect.bottom;
                    CombinedRect.bottom = lTmp;
                }

                DPRINT("About to call CallDibStretchBlt?: CombinedRect: (%d,%d)-(%d,%d)\n",
                       CombinedRect.left, CombinedRect.top, CombinedRect.right, CombinedRect.bottom);

                Ret = (*BltRectFunc)(psoOutput, psoInput, Mask,
                           ColorTranslation,
                           &CombinedRect,
                           &InputToCombinedRect,
                           MaskOrigin,
                           pbo,
                           &AdjustedBrushOrigin,
                           Rop4);
            }
            break;
        case DC_COMPLEX:
            if (psoOutput == psoInput)
            {
                if (OutputRect.top < InputRect.top)
                {
                    Direction = OutputRect.left < InputRect.left ?
                                CD_RIGHTDOWN : CD_LEFTDOWN;
                }
                else
                {
                    Direction = OutputRect.left < InputRect.left ?
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
                        InputToCombinedRect.top = InputRect.top + (CombinedRect.top - OutputRect.top) * SrcHeight / DstHeight;
                        InputToCombinedRect.bottom = InputRect.top + (CombinedRect.bottom - OutputRect.top) * SrcHeight / DstHeight;
                        InputToCombinedRect.left = InputRect.left + (CombinedRect.left - OutputRect.left) * SrcWidth / DstWidth;
                        InputToCombinedRect.right = InputRect.left + (CombinedRect.right - OutputRect.left) * SrcWidth / DstWidth;

                        if (bLeftToRight)
                        {
                           lTmp = OutputRect.left;
                            OutputRect.left = OutputRect.right;
                            OutputRect.right = lTmp;
                        }

                        if (bTopToBottom)
                        {
                            lTmp = OutputRect.top;
                            OutputRect.top = OutputRect.bottom;
                            OutputRect.bottom = lTmp;
                        }

                        DPRINT("About to call CallDibStretchBlt?: CombinedRect: (%d,%d)-(%d,%d)\n",
                               CombinedRect.left, CombinedRect.top, CombinedRect.right, CombinedRect.bottom);

                        Ret = (*BltRectFunc)(psoOutput, psoInput, Mask,
                           ColorTranslation,
                           &CombinedRect,
                           &InputToCombinedRect,
                           MaskOrigin,
                           pbo,
                           &AdjustedBrushOrigin,
                           Rop4);
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

/*
 * @implemented
 */
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
    IN ULONG  Mode)
{
    return EngStretchBltROP(
        psoDest,
        psoSource,
        Mask,
        ClipRegion,
        ColorTranslation,
        pca,
        BrushOrigin,
        prclDest,
        prclSrc,
        MaskOrigin,
        Mode,
        NULL,
        ROP4_FROM_INDEX(R3_OPINDEX_SRCCOPY));
}

BOOL APIENTRY
IntEngStretchBlt(SURFOBJ *psoDest,
                 SURFOBJ *psoSource,
                 SURFOBJ *MaskSurf,
                 CLIPOBJ *ClipRegion,
                 XLATEOBJ *ColorTranslation,
                 COLORADJUSTMENT *pca,
                 RECTL *DestRect,
                 RECTL *SourceRect,
                 POINTL *pMaskOrigin,
                 BRUSHOBJ *pbo,
                 POINTL *BrushOrigin,
                 DWORD Rop4)
{
    BOOLEAN ret;
    POINTL MaskOrigin = {0, 0};
    SURFACE *psurfDest;
    //SURFACE *psurfSource = NULL;
    RECTL InputClippedRect;
    RECTL InputRect;
    RECTL OutputRect;
    BOOL UsesSource = ROP4_USES_SOURCE(Rop4);
    LONG InputClWidth, InputClHeight, InputWidth, InputHeight;
    LONG lTmp, cxSrc, cySrc, cxDest, cyDest;
    BOOLEAN bTopToBottom, bLeftToRight;

    ASSERT(psoDest);
    //ASSERT(psoSource); // FIXME!
    ASSERT(DestRect);
    ASSERT(SourceRect);
    //ASSERT(!RECTL_bIsEmptyRect(SourceRect)); // FIXME!

    /* If no clip object is given, use trivial one */
    if (!ClipRegion) ClipRegion = (CLIPOBJ *)&gxcoTrivial;

    psurfDest = CONTAINING_RECORD(psoDest, SURFACE, SurfObj);

    /* Sanity check */
    ASSERT(IS_VALID_ROP4(Rop4));

    cxSrc = SourceRect->right - SourceRect->left;
    cySrc = SourceRect->bottom - SourceRect->top;
    cxDest = DestRect->right - DestRect->left;
    cyDest = DestRect->bottom - DestRect->top;

    DPRINT("SourceRect is (%d,%d)-(%d,%d) and DestRect is (%d,%d)-(%d,%d)\n",
           DestRect->left, DestRect->top, DestRect->right, DestRect->bottom,
           SourceRect->left, SourceRect->top, SourceRect->right, SourceRect->bottom);

    DPRINT("cxDest(%d) cyDest(%d) cxSrc(%d) cySrc(%d)\n",
           cxDest, cyDest, cxSrc, cySrc);

    /* Check if source and dest size are equal in size */
    if ((abs(cxDest) == abs(cxSrc)) && (abs(cyDest) == abs(cySrc)))
    {
        /* if cxSrc < 0 and cxDest <0 (both negative, we change both their signs  */
        /* because the outcome is the same as if they were both positive and we   */
        /* reverse both their top and bottom values                               */
        if ((cxSrc < 0) && (cxDest < 0))
        {
            lTmp = DestRect->left;
            DestRect->left = DestRect->right;
            DestRect->right = lTmp;
            lTmp = SourceRect->left;
            SourceRect->left = SourceRect->right;
            SourceRect->right = lTmp;
            cxSrc = abs(cxSrc);
            cxDest = abs(cxDest);
        }

        /* if cySrc < 0 and cyDest <0 (both negative, we change both their signs  */
        /* because the outcome is the same as if they were both positive and we   */
        /* reverse both their top and bottom values                               */
        if ((cySrc < 0) && (cyDest < 0))
        {
            lTmp = DestRect->top;
            DestRect->top = DestRect->bottom;
            DestRect->bottom = lTmp;
            lTmp = SourceRect->top;
            SourceRect->top = SourceRect->bottom;
            SourceRect->bottom = lTmp;
            cySrc = -cySrc;
            cyDest = -cyDest;
        }

        /* if the cxDest >= 0 and cxSrc < 0 then we reverse  */
        /* the signs of both cxDest and cxSrc                */
        if ((cxDest >= 0) && (cxSrc < 0))
        {
            cxDest = -cxDest;
            cxSrc = -cxSrc;
        }

        /* if the cxDest < 0 and cxSrc >= 0 then we reverse */
        /* the left and right values                        */
        else if ((cxDest < 0) && (cxSrc >= 0))
        {
            lTmp = SourceRect->left;
            SourceRect->left = SourceRect->right;
            SourceRect->right = lTmp;
        }

        /* if the cyDest >= 0 and cySrc < 0 then we reverse  */
        /* the signs of both cxDest and cxSrc                */
        if ((cyDest >= 0) && (cySrc < 0))
        {
            cyDest = -cyDest;
            cySrc = -cySrc;
        }

        /* if the cyDest < 0 and cySrc >= 0 then we reverse */
        /* the top and bottom values                        */
        else if ((cyDest <= 0) && (cySrc > 0))
        {
            lTmp = SourceRect->top;
            SourceRect->top = SourceRect->bottom;
            SourceRect->bottom = lTmp;
        }
    }

    /* Make Well Ordered to start */
    RECTL_vMakeWellOrdered(DestRect);
    OutputRect = *DestRect;

    /* Here we do the tests and set our conditions */
    if (((cxSrc < 0) && (cxDest < 0)) || ((cxSrc >= 0) && (cxDest >= 0)))
        bLeftToRight = FALSE;
    else
        bLeftToRight = TRUE;

    if (((cySrc < 0) && (cyDest < 0)) || ((cySrc >= 0) && (cyDest >= 0)))
        bTopToBottom = FALSE;
    else
        bTopToBottom = TRUE;

    /* Check if source and dest size are equal */
    if ((abs(DestRect->right - DestRect->left) == abs(SourceRect->right - SourceRect->left)) &&
        (abs(DestRect->bottom - DestRect->top) == abs(SourceRect->bottom - SourceRect->top)))
    {
        DPRINT("source and dest size are equal.\n");

        DPRINT("xDest(%d) yDest(%d) cxDest(%d) cyDest(%d) xSrc(%d) ySrc(%d) cxSrc(%d) cySrc(%d)\n",
            DestRect->left, DestRect->top, cxDest, cyDest,
            SourceRect->left, SourceRect->top, cxSrc, cySrc);

        if ((cxSrc < 0) != (cxDest < 0)) // Not same sign
        {
            DPRINT("x Signs are not equal.\n");
            lTmp = DestRect->left;
            DestRect->left = DestRect->right;
            DestRect->right = lTmp;
        }

        if ((cySrc >= 0) != (cyDest >= 0)) // Not same sign
        {
            DPRINT("y Signs are not equal.\n");
            lTmp = DestRect->top;
            DestRect->top = DestRect->bottom;
            DestRect->bottom = lTmp;
        }

        /* Pass the request to IntEngBitBlt */
        return IntEngBitBlt(psoDest,
                            psoSource,
                            MaskSurf,
                            ClipRegion,
                            ColorTranslation,
                            DestRect,
                            (PPOINTL)SourceRect,
                            pMaskOrigin,
                            pbo,
                            BrushOrigin,
                            Rop4);
    }
    else
    {
        DPRINT("source and dest size are NOT equal.\n");
    }

    DPRINT("bLeftToRight is '%d' and bTopToBottom is '%d'.\n", bLeftToRight, bTopToBottom);

    /* if cxSrc < 0 then we change the signs for both cxSrc and cxDest and */
    /* we reverse their coordinates, because these outcomes are the same.  */
    if (cxSrc < 0)
    {
        lTmp = SourceRect->left;
        SourceRect->left = SourceRect->right;
        SourceRect->right = lTmp;
        lTmp = DestRect->left;
        DestRect->left = DestRect->right;
        DestRect->right = lTmp;
        cxSrc = -cxSrc;
        cxDest = -cxDest;
    }

    /* if cySrc < 0 then we change the signs for both cySrc and cyDest and */
    /* we reverse their coordinates, because these outcomes are the same.  */
    if (cySrc < 0)
    {
        lTmp = DestRect->top;
        DestRect->top = DestRect->bottom;
        DestRect->bottom = lTmp;
        lTmp = SourceRect->top;
        SourceRect->top = SourceRect->bottom;
        SourceRect->bottom = lTmp;
        cySrc = -cySrc;
        cyDest = -cyDest;
    }

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

    if (NULL == psoSource)
    {
        return FALSE;
    }
    InputRect = *SourceRect;

    if (InputRect.right < InputRect.left ||
            InputRect.bottom < InputRect.top)
    {
        /* Everything clipped away, nothing to do */
        return TRUE;
    }

    if (ClipRegion->iDComplexity != DC_TRIVIAL)
    {
        if (!RECTL_bIntersectRect(&OutputRect, &InputClippedRect,
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
        MaskOrigin.x = pMaskOrigin->x;
        MaskOrigin.y = pMaskOrigin->y;
    }

    /* No success yet */
    ret = FALSE;

    if (UsesSource)
    {
        //psurfSource = CONTAINING_RECORD(psoSource, SURFACE, SurfObj);
    }

    /* Call the driver's DrvStretchBlt if available */
    if (psurfDest->flags & HOOK_STRETCHBLTROP)
    {
        /* Drv->StretchBltROP (look at http://www.osronline.com/ddkx/graphics/ddifncs_0z3b.htm ) */
        ret = GDIDEVFUNCS(psoDest).StretchBltROP(psoDest,
                                                 psoSource,
                                                 MaskSurf,
                                                 ClipRegion,
                                                 ColorTranslation,
                                                 pca,
                                                 BrushOrigin,
                                                 &OutputRect,
                                                 &InputRect,
                                                 &MaskOrigin,
                                                 COLORONCOLOR,
                                                 pbo,
                                                 Rop4);
    }

    if (! ret)
    {
        DPRINT("bLeftToRight is '%d' and bTopToBottom is '%d'.\n", bLeftToRight, bTopToBottom);
        /* set OutputRect to follow flip */
        /* Above we converted cxSrc and cySrc to   */
        /* positive so no need to check these here */
        if (bLeftToRight)
        {
            lTmp = OutputRect.left;
            OutputRect.left = OutputRect.right;
            OutputRect.right = lTmp;
        }

        if (bTopToBottom)
        {
            lTmp = OutputRect.top;
            OutputRect.top = OutputRect.bottom;
            OutputRect.bottom = lTmp;
        }

        DPRINT("About to call EngStretchBltROP: SrcSurf cx/cy (%d/%d), DestSuft cx/cy (%d/%d) dstRect: (%d,%d)-(%d,%d)\n",
            psoSource->sizlBitmap.cx, psoSource->sizlBitmap.cy, //SourceRect->right, SourceRect->bottom,
            psoDest->sizlBitmap.cx, psoDest->sizlBitmap.cy,
            OutputRect.left, OutputRect.top, OutputRect.right, OutputRect.bottom);

        ret = EngStretchBltROP(psoDest,
                               psoSource,
                               MaskSurf,
                               ClipRegion,
                               ColorTranslation,
                               pca,
                               BrushOrigin,
                               &OutputRect,
                               &InputRect,
                               &MaskOrigin,
                               COLORONCOLOR,
                               pbo,
                               Rop4);
    }

    return ret;
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
    IN ULONG  Mode)
{
    COLORADJUSTMENT  ca;
    POINTL  lBrushOrigin;
    RECTL rclDest;
    RECTL rclSrc;
    POINTL lMaskOrigin;

    _SEH2_TRY
    {
        if (pca)
        {
            ProbeForRead(pca, sizeof(COLORADJUSTMENT), 1);
            RtlCopyMemory(&ca,pca, sizeof(COLORADJUSTMENT));
            pca = &ca;
        }

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

    return EngStretchBlt(psoDest, psoSource, Mask, ClipRegion, ColorTranslation, pca, &lBrushOrigin, &rclDest, &rclSrc, &lMaskOrigin, Mode);
}

/* EOF */
