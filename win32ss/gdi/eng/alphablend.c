/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          GDI alpha blending functions
 * FILE:             win32ss/gdi/eng/alphablend.c
 * PROGRAMER:        Jason Filby
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>


/*
 * @implemented
 */
BOOL
APIENTRY
EngAlphaBlend(
    _Inout_ SURFOBJ *psoDest,
    _In_ SURFOBJ *psoSource,
    _In_opt_ CLIPOBJ *ClipRegion,
    _In_opt_ XLATEOBJ *ColorTranslation,
    _In_ RECTL *DestRect,
    _In_ RECTL *SourceRect,
    _In_ BLENDOBJ *BlendObj)
{
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
    LONG               ClippingType;
    RECT_ENUM          RectEnum;
    BOOL               EnumMore;
    ULONG              i;
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
    RECTL_vMakeWellOrdered(&OutputRect);

    /* Validate input */
    InputRect = *SourceRect;
    RECTL_vMakeWellOrdered(&InputRect);
    if ( (InputRect.top < 0) || (InputRect.bottom < 0) ||
         (InputRect.left < 0) || (InputRect.right < 0) ||
         InputRect.right > psoSource->sizlBitmap.cx ||
         InputRect.bottom > psoSource->sizlBitmap.cy )
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
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

    /* Now call the DIB function */
    if (!IntEngEnter(&EnterLeaveSource, psoSource, &InputRect, TRUE, &Translate, &InputObj))
    {
        return FALSE;
    }
    InputRect.left +=  Translate.x;
    InputRect.right +=  Translate.x;
    InputRect.top +=  Translate.y;
    InputRect.bottom +=  Translate.y;

    if (!IntEngEnter(&EnterLeaveDest, psoDest, &OutputRect, FALSE, &Translate, &OutputObj))
    {
        IntEngLeave(&EnterLeaveSource);
        return FALSE;
    }
    OutputRect.left += Translate.x;
    OutputRect.right += Translate.x;
    OutputRect.top += Translate.y;
    OutputRect.bottom += Translate.y;

    ASSERT(InputRect.left <= InputRect.right && InputRect.top <= InputRect.bottom);

    Ret = FALSE;
    ClippingType = (ClipRegion == NULL) ? DC_TRIVIAL : ClipRegion->iDComplexity;
    switch (ClippingType)
    {
        case DC_TRIVIAL:
            Ret = DibFunctionsForBitmapFormat[OutputObj->iBitmapFormat].DIB_AlphaBlend(
                      OutputObj, InputObj, &OutputRect, &InputRect, ClipRegion, ColorTranslation, BlendObj);
            break;

        case DC_RECT:
            Ret = TRUE;
            ClipRect.left = ClipRegion->rclBounds.left + Translate.x;
            ClipRect.right = ClipRegion->rclBounds.right + Translate.x;
            ClipRect.top = ClipRegion->rclBounds.top + Translate.y;
            ClipRect.bottom = ClipRegion->rclBounds.bottom + Translate.y;
            if (RECTL_bIntersectRect(&CombinedRect, &OutputRect, &ClipRect))
            {
                /* take into acount clipping results when calculating new input rect (scaled to input rect size) */
                Rect.left = InputRect.left + (CombinedRect.left - OutputRect.left) * (InputRect.right - InputRect.left) / (OutputRect.right - OutputRect.left);
                Rect.right = InputRect.right + (CombinedRect.right - OutputRect.right) * (InputRect.right - InputRect.left) / (OutputRect.right - OutputRect.left);
                Rect.top = InputRect.top + (CombinedRect.top - OutputRect.top) * (InputRect.bottom - InputRect.top) / (OutputRect.bottom - OutputRect.top);
                Rect.bottom = InputRect.bottom + (CombinedRect.bottom - OutputRect.bottom) * (InputRect.bottom - InputRect.top) / (OutputRect.bottom - OutputRect.top);

                /* Aplha blend one rect */
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
                    if (RECTL_bIntersectRect(&CombinedRect, &OutputRect, &ClipRect))
                    {
                        /* take into acount clipping results when calculating new input rect (scaled to input rect size) */
                        Rect.left = InputRect.left + (CombinedRect.left - OutputRect.left) * (InputRect.right - InputRect.left) / (OutputRect.right - OutputRect.left);
                        Rect.right = InputRect.right + (CombinedRect.right - OutputRect.right) * (InputRect.right - InputRect.left) / (OutputRect.right - OutputRect.left);
                        Rect.top = InputRect.top + (CombinedRect.top - OutputRect.top) * (InputRect.bottom - InputRect.top) / (OutputRect.bottom - OutputRect.top);
                        Rect.bottom = InputRect.bottom + (CombinedRect.bottom - OutputRect.bottom) * (InputRect.bottom - InputRect.top) / (OutputRect.bottom - OutputRect.top);

                        /* Alpha blend one rect */
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

    return Ret;
}

BOOL
APIENTRY
IntEngAlphaBlend(
    _Inout_ SURFOBJ *psoDest,
    _In_ SURFOBJ *psoSource,
    _In_opt_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_ RECTL *prclDest,
    _In_ RECTL *prclSrc,
    _In_ BLENDOBJ *pBlendObj)
{
    BOOL ret = FALSE;
    SURFACE *psurfDest;

    ASSERT(psoDest);
    ASSERT(psoSource);
    ASSERT(prclDest);
    ASSERT(prclSrc);
    //ASSERT(pBlendObj);

    /* If no clip object is given, use trivial one */
    if (!pco) pco = (CLIPOBJ *)&gxcoTrivial;

    /* Check if there is anything to draw */
    if ((pco->rclBounds.left >= pco->rclBounds.right) ||
        (pco->rclBounds.top >= pco->rclBounds.bottom))
    {
        /* Nothing to do */
        return TRUE;
    }

    psurfDest = CONTAINING_RECORD(psoDest, SURFACE, SurfObj);

    /* Call the driver's DrvAlphaBlend if available */
    if (psurfDest->flags & HOOK_ALPHABLEND)
    {
        ret = GDIDEVFUNCS(psoDest).AlphaBlend(
                  psoDest, psoSource, pco, pxlo,
                  prclDest, prclSrc, pBlendObj);
    }

    if (!ret)
    {
        ret = EngAlphaBlend(psoDest, psoSource, pco, pxlo,
                            prclDest, prclSrc, pBlendObj);
    }

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

