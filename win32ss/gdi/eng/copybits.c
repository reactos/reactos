/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          GDI EngCopyBits Function
 * FILE:             win32ss/gdi/eng/copybits.c
 * PROGRAMERS:       Jason Filby
 *                   Doug Lyons
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

/*
 * @implemented
 */
BOOL APIENTRY
EngCopyBits(
    _In_ SURFOBJ *psoDest,
    _In_ SURFOBJ *psoSource,
    _In_opt_ CLIPOBJ *Clip,
    _In_opt_ XLATEOBJ *ColorTranslation,
    _In_ RECTL *DestRect,
    _In_ POINTL *SourcePoint)
{
    BOOL      ret;
    BYTE      clippingType;
    RECT_ENUM RectEnum;
    BOOL      EnumMore;
    BLTINFO   BltInfo;
    SURFACE *psurfDest;
    SURFACE *psurfSource;
    RECTL rclDest = *DestRect;
    POINTL ptlSrc = *SourcePoint;
    LONG      lTmp;
    BOOL      bTopToBottom;

    DPRINT("Entering EngCopyBits with SourcePoint (%d,%d) and DestRect (%d,%d)-(%d,%d).\n",
           SourcePoint->x, SourcePoint->y, DestRect->left, DestRect->top, DestRect->right, DestRect->bottom);

    DPRINT("psoSource cx/cy is %d/%d and psoDest cx/cy is %d/%d.\n",
           psoSource->sizlBitmap.cx, psoSource->sizlBitmap.cy, psoDest->sizlBitmap.cx, psoDest->sizlBitmap.cy);

    /* Retrieve Top Down/flip here and then make Well-Ordered again */
    if (DestRect->top > DestRect->bottom)
    {
        bTopToBottom = TRUE;
        lTmp = DestRect->top;
        DestRect->top = DestRect->bottom;
        DestRect->bottom = lTmp;
        rclDest = *DestRect;
    }
    else
    {
        bTopToBottom = FALSE;
    }

    DPRINT("bTopToBottom is '%d'.\n", bTopToBottom);

    ASSERT(psoDest != NULL && psoSource != NULL && DestRect != NULL && SourcePoint != NULL);

    psurfSource = CONTAINING_RECORD(psoSource, SURFACE, SurfObj);
    psurfDest = CONTAINING_RECORD(psoDest, SURFACE, SurfObj);

    /* Clip dest rect against source surface size / source point */
    if (psoSource->sizlBitmap.cx - ptlSrc.x < rclDest.right - rclDest.left)
        rclDest.right = rclDest.left + psoSource->sizlBitmap.cx - ptlSrc.x;
    if (psoSource->sizlBitmap.cy - ptlSrc.y < rclDest.bottom - rclDest.top)
        rclDest.bottom = rclDest.top + psoSource->sizlBitmap.cy - ptlSrc.y;

    /* Clip dest rect against target surface size */
    if (rclDest.right > psoDest->sizlBitmap.cx)
        rclDest.right = psoDest->sizlBitmap.cx;
    if (rclDest.bottom > psoDest->sizlBitmap.cy)
        rclDest.bottom = psoDest->sizlBitmap.cy;
    if (RECTL_bIsEmptyRect(&rclDest)) return TRUE;
    DestRect = &rclDest;

    // FIXME: Don't punt to the driver's DrvCopyBits immediately. Instead,
    //        mark the copy block function to be DrvCopyBits instead of the
    //        GDI's copy bit function so as to remove clipping from the
    //        driver's responsibility

    // If one of the surfaces isn't managed by the GDI
    if ((psoDest->iType!=STYPE_BITMAP) || (psoSource->iType!=STYPE_BITMAP))
    {
        // Destination surface is device managed
        if (psoDest->iType!=STYPE_BITMAP)
        {
            /* FIXME: Eng* functions shouldn't call Drv* functions. ? */
            if (psurfDest->flags & HOOK_COPYBITS)
            {
                ret = GDIDEVFUNCS(psoDest).CopyBits(
                          psoDest, psoSource, Clip, ColorTranslation, DestRect, SourcePoint);

                goto cleanup;
            }
        }

        // Source surface is device managed
        if (psoSource->iType!=STYPE_BITMAP)
        {
            /* FIXME: Eng* functions shouldn't call Drv* functions. ? */
            if (psurfSource->flags & HOOK_COPYBITS)
            {
                ret = GDIDEVFUNCS(psoSource).CopyBits(
                          psoDest, psoSource, Clip, ColorTranslation, DestRect, SourcePoint);

                goto cleanup;
            }
        }

        // If CopyBits wasn't hooked, BitBlt must be
        ret = IntEngBitBlt(psoDest, psoSource,
                           NULL, Clip, ColorTranslation, DestRect, SourcePoint,
                           NULL, NULL, NULL, ROP4_FROM_INDEX(R3_OPINDEX_SRCCOPY));

        goto cleanup;
    }

    // Determine clipping type
    if (!Clip)
    {
        clippingType = DC_TRIVIAL;
    }
    else
    {
        clippingType = Clip->iDComplexity;
    }

    BltInfo.DestSurface = psoDest;
    BltInfo.SourceSurface = psoSource;
    BltInfo.PatternSurface = NULL;
    BltInfo.XlateSourceToDest = ColorTranslation;
    BltInfo.Rop4 = ROP4_FROM_INDEX(R3_OPINDEX_SRCCOPY);

    switch (clippingType)
    {
        case DC_TRIVIAL:
            DPRINT("DC_TRIVIAL.\n");
            BltInfo.DestRect = *DestRect;
            BltInfo.SourcePoint = *SourcePoint;

            /* Now we set the Dest Rect top and bottom based on Top Down/flip */
            if (bTopToBottom)
            {
                lTmp = BltInfo.DestRect.top;
                BltInfo.DestRect.top = BltInfo.DestRect.bottom;
                BltInfo.DestRect.bottom = lTmp;
            }

            ret = DibFunctionsForBitmapFormat[psoDest->iBitmapFormat].DIB_BitBltSrcCopy(&BltInfo);
            break;

        case DC_RECT:
            DPRINT("DC_RECT.\n");
            // Clip the blt to the clip rectangle
            RECTL_bIntersectRect(&BltInfo.DestRect, DestRect, &Clip->rclBounds);

            BltInfo.SourcePoint.x = SourcePoint->x + BltInfo.DestRect.left - DestRect->left;
            BltInfo.SourcePoint.y = SourcePoint->y + BltInfo.DestRect.top  - DestRect->top;

            /* Now we set the Dest Rect top and bottom based on Top Down/flip */
            if (bTopToBottom)
            {
                lTmp = BltInfo.DestRect.top;
                BltInfo.DestRect.top = BltInfo.DestRect.bottom;
                BltInfo.DestRect.bottom = lTmp;
            }

            ret = DibFunctionsForBitmapFormat[psoDest->iBitmapFormat].DIB_BitBltSrcCopy(&BltInfo);
            break;

        case DC_COMPLEX:
            DPRINT("DC_COMPLEX.\n");
            CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES, CD_ANY, 0);

            do
            {
                EnumMore = CLIPOBJ_bEnum(Clip,(ULONG) sizeof(RectEnum), (PVOID) &RectEnum);

                if (RectEnum.c > 0)
                {
                    RECTL* prclEnd = &RectEnum.arcl[RectEnum.c];
                    RECTL* prcl    = &RectEnum.arcl[0];

                    do
                    {
                        RECTL_bIntersectRect(&BltInfo.DestRect, prcl, DestRect);

                        BltInfo.SourcePoint.x = SourcePoint->x + BltInfo.DestRect.left - DestRect->left;
                        BltInfo.SourcePoint.y = SourcePoint->y + BltInfo.DestRect.top - DestRect->top;

                        /* Now we set the Dest Rect top and bottom based on Top Down/flip */
                        if (bTopToBottom)
                        {
                            lTmp = BltInfo.DestRect.top;
                            BltInfo.DestRect.top = BltInfo.DestRect.bottom;
                            BltInfo.DestRect.bottom = lTmp;
                        }

                        if (!DibFunctionsForBitmapFormat[psoDest->iBitmapFormat].DIB_BitBltSrcCopy(&BltInfo))
                        {
                            ret = FALSE;
                            goto cleanup;
                        }

                        prcl++;

                    } while (prcl < prclEnd);
                }

            } while (EnumMore);
            ret = TRUE;
            break;

        default:
            ASSERT(FALSE);
            ret = FALSE;
            break;
    }

cleanup:
    return ret;
}

BOOL APIENTRY
IntEngCopyBits(
    SURFOBJ *psoDest,
    SURFOBJ *psoSource,
    CLIPOBJ *pco,
    XLATEOBJ *pxlo,
    RECTL *prclDest,
    POINTL *ptlSource)
{
    return EngCopyBits(psoDest, psoSource, pco, pxlo, prclDest, ptlSource);
}


/* EOF */
