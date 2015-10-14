/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engblt.c
 * PURPOSE:         Bit-Block Transfer Support Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
                    Jason Filby
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
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

/* PRIVATE FUNCTIONS **********************************************************/

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
    ULONG Pattern = 0;
    HBITMAP hbmPattern;

    if (psoMask == NULL)
    {
        return FALSE;
    }

    if (pbo && pbo->iSolidColor == 0xFFFFFFFF)
    {
        pebo = CONTAINING_RECORD(pbo, EBRUSHOBJ, BrushObject);

        hbmPattern = EBRUSHOBJ_pvGetEngBrush(pebo);
        psurfPattern = SURFACE_LockSurface(hbmPattern);
        if (psurfPattern != NULL)
        {
            psoPattern = &psurfPattern->SurfObj;
            PatternWidth = psoPattern->sizlBitmap.cx;
            PatternHeight = psoPattern->sizlBitmap.cy;
            fnPattern_GetPixel = DibFunctionsForBitmapFormat[psoPattern->iBitmapFormat].DIB_GetPixel;
        }
    }
    else
        psurfPattern = NULL;

    pjMskLine = (PBYTE)psoMask->pvScan0 + pptlMask->y * psoMask->lDelta + (pptlMask->x >> 3);
    fjMaskBit0 = 0x80 >> (pptlMask->x & 0x07);

    fnDest_PutPixel = DibFunctionsForBitmapFormat[psoDest->iBitmapFormat].DIB_PutPixel;
    if (psurfPattern)
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
                        fnPattern_GetPixel(psoPattern, PatternX, PatternY));
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
    HBITMAP hbmPattern;

    BltInfo.DestSurface = OutputObj;
    BltInfo.SourceSurface = InputObj;
    BltInfo.PatternSurface = NULL;
    BltInfo.XlateSourceToDest = ColorTranslation;
    BltInfo.DestRect = *OutputRect;
    BltInfo.SourcePoint = *InputPoint;

    if (ROP3_TO_ROP4(SRCCOPY) == Rop4)
        return DibFunctionsForBitmapFormat[OutputObj->iBitmapFormat].DIB_BitBltSrcCopy(&BltInfo);

    BltInfo.Brush = pbo;
    BltInfo.BrushOrigin = *BrushOrigin;
    BltInfo.Rop4 = Rop4;

    /* Pattern brush */
    if (ROP4_USES_PATTERN(Rop4) && pbo && pbo->iSolidColor == 0xFFFFFFFF)
    {
        GdiBrush = CONTAINING_RECORD(pbo, EBRUSHOBJ, BrushObject);
        hbmPattern = EBRUSHOBJ_pvGetEngBrush(GdiBrush);
        psurfPattern = SURFACE_LockSurface(hbmPattern);
        if (psurfPattern)
        {
            BltInfo.PatternSurface = &psurfPattern->SurfObj;
        }
        else
        {
            /* FIXME - What to do here? */
        }
    }
    else
    {
        psurfPattern = NULL;
    }

    psurfPattern = NULL;


    Result = DibFunctionsForBitmapFormat[OutputObj->iBitmapFormat].DIB_BitBlt(&BltInfo);

    /* Pattern brush */
    if (psurfPattern)
    {
        SURFACE_UnlockSurface(psurfPattern);
    }

    return Result;
}

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
    SURFACE* psurfPattern;
    PEBRUSHOBJ GdiBrush = NULL;
    SURFOBJ* PatternSurface = NULL;
    BOOL bResult;
    HBITMAP hbmPattern;

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
        GdiBrush = CONTAINING_RECORD(pbo, EBRUSHOBJ, BrushObject);
        hbmPattern = EBRUSHOBJ_pvGetEngBrush(GdiBrush);
        psurfPattern = SURFACE_LockSurface(hbmPattern);
        if (psurfPattern)
        {
            PatternSurface = &psurfPattern->SurfObj;
        }
        else
        {
            /* FIXME - What to do here? */
        }
    }
    else
    {
        psurfPattern = NULL;
    }

    bResult = DibFunctionsForBitmapFormat[psoDest->iBitmapFormat].DIB_StretchBlt(
               psoDest, psoSource, Mask, PatternSurface,
               OutputRect, InputRect, MaskOrigin, pbo, &RealBrushOrigin,
               ColorTranslation, Rop4);

    /* Pattern brush */
    if (psurfPattern)
    {
        SURFACE_UnlockSurface(psurfPattern);
    }

    return bResult;
}

BOOL APIENTRY
EngpStretchBlt(SURFOBJ *psoDest,
               SURFOBJ *psoSource,
               SURFOBJ *MaskSurf,
               CLIPOBJ *ClipRegion,
               XLATEOBJ *ColorTranslation,
               RECTL *DestRect,
               RECTL *SourceRect,
               POINTL *pMaskOrigin,
               BRUSHOBJ *pbo,
               POINTL *BrushOrigin,
               ROP4 ROP)
{
    BOOLEAN ret;
    COLORADJUSTMENT ca;
    //POINT MaskOrigin;
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
 
        if (InputRect.right < InputRect.left ||
                InputRect.bottom < InputRect.top)
        {
            /* Everything clipped away, nothing to do */
            return TRUE;
        }
    }    

    if (ClipRegion)
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
    
    /*if (pMaskOrigin != NULL)
    {
        MaskOrigin.x = pMaskOrigin->x; MaskOrigin.y = pMaskOrigin->y;
    }*/

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
    if (psurfDest->flHooks & HOOK_STRETCHBLTROP)
    {
        /* Drv->StretchBltROP (look at http://www.osronline.com/ddkx/graphics/ddifncs_0z3b.htm ) */
        // FIXME: MaskOrigin is always NULL !
        ret = GDIDEVFUNCS(psoDest).StretchBltROP(psoDest, (UsesSource) ? psoSource : NULL, MaskSurf, ClipRegion, ColorTranslation,
                  &ca, BrushOrigin, &OutputRect, &InputRect, NULL, COLORONCOLOR, pbo, ROP);
    }

    if (! ret)
    {
        // FIXME: see previous fixme
        ret = EngStretchBltROP(psoDest, (UsesSource) ? psoSource : NULL, MaskSurf, ClipRegion, ColorTranslation,
                            &ca, BrushOrigin, &OutputRect, &InputRect, NULL, COLORONCOLOR, pbo, ROP);
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

/* PUBLIC FUNCTIONS **********************************************************/

BOOL
APIENTRY
EngAlphaBlend(IN SURFOBJ* psoDest,
              IN SURFOBJ* psoSource,
              IN CLIPOBJ* ClipRegion,
              IN XLATEOBJ* ColorTranslation,
              IN PRECTL DestRect,
              IN PRECTL SourceRect,
              IN BLENDOBJ* BlendObj)
{
    RECTL              SourceStretchedRect;
    SIZEL              SourceStretchedSize;
    HSURF              SourceStretchedBitmap = 0;
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
        SourceStretchedBitmap = (HSURF)EngCreateBitmap(SourceStretchedSize, Width, psoSource->iBitmapFormat,
                                                BMF_TOPDOWN | BMF_NOZEROINIT, NULL);

        if (SourceStretchedBitmap == 0)
        {
            DPRINT1("EngCreateBitmap failed!\n");
            return FALSE;
        }
        SourceStretchedObj = EngLockSurface(SourceStretchedBitmap);
        if (SourceStretchedObj == NULL)
        {
            DPRINT1("EngLockSurface failed!\n");
            EngDeleteSurface(SourceStretchedBitmap);
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
        EngUnlockSurface(SourceStretchedObj);
        EngDeleteSurface(SourceStretchedBitmap);
        return FALSE;
    }

    InputRect.left +=  Translate.x;
    InputRect.right +=  Translate.x;
    InputRect.top +=  Translate.y;
    InputRect.bottom +=  Translate.y;

    if (!IntEngEnter(&EnterLeaveDest, psoDest, &OutputRect, FALSE, &Translate, &OutputObj))
    {
        IntEngLeave(&EnterLeaveSource);
        EngUnlockSurface(SourceStretchedObj);
        EngDeleteSurface(SourceStretchedBitmap);
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

    if(SourceStretchedObj != NULL)
        EngUnlockSurface(SourceStretchedObj);
    if(SourceStretchedBitmap != 0)
        EngDeleteSurface(SourceStretchedBitmap);

    return Ret;
}

BOOL
APIENTRY
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
          ROP4 rop4)
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
    unsigned           i;
    POINTL             Pt;
    ULONG              Direction;
    BOOL               UsesSource;
    //BOOL               UsesPattern;
    POINTL             AdjustedBrushOrigin;

    UsesSource = ROP4_USES_SOURCE(rop4);
    //UsesPattern = ROP4_USES_PATTERN(rop4);
    if (R4_NOOP == rop4)
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

        InputObj = SourceObj;
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
        return TRUE;
    }

    OutputObj = DestObj;

    if (BrushOrigin)
    {
        AdjustedBrushOrigin.x = BrushOrigin->x;
        AdjustedBrushOrigin.y = BrushOrigin->y;
    }
    else
    {
        AdjustedBrushOrigin.x = 0;
        AdjustedBrushOrigin.y = 0;
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

    if (R4_MASK == rop4)
    {
        BltRectFunc = BltMask;
    }
    else if (ROP3_TO_ROP4(PATCOPY) == rop4)
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
                                 &AdjustedBrushOrigin, rop4);
            break;
        case DC_RECT:
            /* Clip the blt to the clip rectangle */
            ClipRect.left = ClipRegion->rclBounds.left;
            ClipRect.right = ClipRegion->rclBounds.right;
            ClipRect.top = ClipRegion->rclBounds.top;
            ClipRect.bottom = ClipRegion->rclBounds.bottom;
            if (RECTL_bIntersectRect(&CombinedRect, &OutputRect, &ClipRect))
            {
                Pt.x = InputPoint.x + CombinedRect.left - OutputRect.left;
                Pt.y = InputPoint.y + CombinedRect.top - OutputRect.top;
                Ret = (*BltRectFunc)(OutputObj, InputObj, Mask, ColorTranslation,
                                     &CombinedRect, &Pt, MaskOrigin, pbo,
                                     &AdjustedBrushOrigin, rop4);
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
                    ClipRect.left = RectEnum.arcl[i].left;
                    ClipRect.right = RectEnum.arcl[i].right;
                    ClipRect.top = RectEnum.arcl[i].top;
                    ClipRect.bottom = RectEnum.arcl[i].bottom;
                    if (RECTL_bIntersectRect(&CombinedRect, &OutputRect, &ClipRect))
                    {
                        Pt.x = InputPoint.x + CombinedRect.left - OutputRect.left;
                        Pt.y = InputPoint.y + CombinedRect.top - OutputRect.top;
                        Ret = (*BltRectFunc)(OutputObj, InputObj, Mask,
                                             ColorTranslation, &CombinedRect, &Pt,
                                             MaskOrigin, pbo, &AdjustedBrushOrigin,
                                             rop4) && Ret;
                    }
                }
            }
            while (EnumMore);
            break;
    }

    return Ret;
}

BOOL
APIENTRY
EngStretchBlt(IN SURFOBJ* DestSurf,
              IN SURFOBJ* SourceSurf,
              IN SURFOBJ* Mask,
              IN CLIPOBJ* ClipRegion,
              IN XLATEOBJ* ColorTranslation,
              IN COLORADJUSTMENT* ColorAdjustment,
              IN PPOINTL BrushOrigin,
              IN PRECTL prclDest,
              IN PRECTL prclSource,
              IN PPOINTL MaskOrigin,
              IN ULONG Mode)
{
    return EngStretchBltROP(
        DestSurf,
        SourceSurf,
        Mask,
        ClipRegion,
        ColorTranslation,
        ColorAdjustment,
        BrushOrigin,
        prclDest,
        prclSource,
        MaskOrigin,
        Mode,
        NULL,
        ROP3_TO_ROP4(SRCCOPY));
}

BOOL
APIENTRY
EngPlgBlt(
   IN SURFOBJ *Dest,
   IN SURFOBJ *Source,
   IN SURFOBJ *Mask,
   IN CLIPOBJ *Clip,
   IN XLATEOBJ *Xlate,
   IN COLORADJUSTMENT *ColorAdjustment,
   IN POINTL *BrusOrigin,
   IN POINTFIX *DestPoints,
   IN RECTL *SourceRect,
   IN POINTL *MaskPoint,
   IN ULONG Mode)
{
   UNIMPLEMENTED;
   return FALSE;
}

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
    IN DWORD ROP4)
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
    BOOL               UsesSource = ROP4_USES_SOURCE(ROP4);

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

    /* Determine clipping type */
    if (ClipRegion == (CLIPOBJ *) NULL)
    {
        clippingType = DC_TRIVIAL;
    }
    else
    {
        clippingType = ClipRegion->iDComplexity;
    }

    if (ROP4 == R4_NOOP)
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
        OutputRect.top = prclDest->bottom;
        OutputRect.bottom = prclDest->top;
    }
    
    InputRect = *prclSrc;
    if (UsesSource)
    {
        if (NULL == prclSrc)
        {
            return FALSE;
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

    BltRectFunc = CallDibStretchBlt;

    DstHeight = OutputRect.bottom - OutputRect.top;
    DstWidth = OutputRect.right - OutputRect.left;
    SrcHeight = InputRect.bottom - InputRect.top;
    SrcWidth = InputRect.right - InputRect.left;
    switch (clippingType)
    {
        case DC_TRIVIAL:
            Ret = (*BltRectFunc)(psoOutput, psoInput, Mask, 
                         ColorTranslation, &OutputRect, &InputRect, MaskOrigin,
                         pbo, &AdjustedBrushOrigin, ROP4);
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
                Ret = (*BltRectFunc)(psoOutput, psoInput, Mask, 
                           ColorTranslation, 
                           &CombinedRect,
                           &InputToCombinedRect,
                           MaskOrigin,
                           pbo, 
                           &AdjustedBrushOrigin, 
                           ROP4);
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
                        Ret = (*BltRectFunc)(psoOutput, psoInput, Mask, 
                           ColorTranslation, 
                           &CombinedRect,
                           &InputToCombinedRect,
                           MaskOrigin,
                           pbo, 
                           &AdjustedBrushOrigin, 
                           ROP4);
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

BOOL
APIENTRY
EngTransparentBlt(IN SURFOBJ* DestSurf,
                  IN SURFOBJ* SourceSurf,
                  IN CLIPOBJ* ClipRegion,
                  IN XLATEOBJ* ColorTranslation,
                  IN PRECTL prclDest,
                  IN PRECTL prclSource,
                  IN ULONG iTransColor,
                  IN ULONG ulReserved)
{
    UNIMPLEMENTED;
    return FALSE;
}
