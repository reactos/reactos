/* 
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          GDI alpha blending functions
 * FILE:             subsystems/win32/win32k/eng/alphablend.c
 * PROGRAMER:        Jason Filby
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>


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
            if (RECTL_bIntersectRect(&CombinedRect, &OutputRect, &ClipRect))
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
                    if (RECTL_bIntersectRect(&CombinedRect, &OutputRect, &ClipRect))
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

