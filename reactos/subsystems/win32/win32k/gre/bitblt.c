/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         LGPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/gre/bitblt.c
 * PURPOSE:         Bitblitting
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

static const RGBQUAD EGAColorsQuads[16] = {
/* rgbBlue, rgbGreen, rgbRed, rgbReserved */
    { 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x80, 0x00 },
    { 0x00, 0x80, 0x00, 0x00 },
    { 0x00, 0x80, 0x80, 0x00 },
    { 0x80, 0x00, 0x00, 0x00 },
    { 0x80, 0x00, 0x80, 0x00 },
    { 0x80, 0x80, 0x00, 0x00 },
    { 0x80, 0x80, 0x80, 0x00 },
    { 0xc0, 0xc0, 0xc0, 0x00 },
    { 0x00, 0x00, 0xff, 0x00 },
    { 0x00, 0xff, 0x00, 0x00 },
    { 0x00, 0xff, 0xff, 0x00 },
    { 0xff, 0x00, 0x00, 0x00 },
    { 0xff, 0x00, 0xff, 0x00 },
    { 0xff, 0xff, 0x00, 0x00 },
    { 0xff, 0xff, 0xff, 0x00 }
};

static const RGBQUAD DefLogPaletteQuads[20] = { /* Copy of Default Logical Palette */
/* rgbBlue, rgbGreen, rgbRed, rgbReserved */
    { 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x80, 0x00 },
    { 0x00, 0x80, 0x00, 0x00 },
    { 0x00, 0x80, 0x80, 0x00 },
    { 0x80, 0x00, 0x00, 0x00 },
    { 0x80, 0x00, 0x80, 0x00 },
    { 0x80, 0x80, 0x00, 0x00 },
    { 0xc0, 0xc0, 0xc0, 0x00 },
    { 0xc0, 0xdc, 0xc0, 0x00 },
    { 0xf0, 0xca, 0xa6, 0x00 },
    { 0xf0, 0xfb, 0xff, 0x00 },
    { 0xa4, 0xa0, 0xa0, 0x00 },
    { 0x80, 0x80, 0x80, 0x00 },
    { 0x00, 0x00, 0xf0, 0x00 },
    { 0x00, 0xff, 0x00, 0x00 },
    { 0x00, 0xff, 0xff, 0x00 },
    { 0xff, 0x00, 0x00, 0x00 },
    { 0xff, 0x00, 0xff, 0x00 },
    { 0xff, 0xff, 0x00, 0x00 },
    { 0xff, 0xff, 0xff, 0x00 }
};

INT NTAPI DIB_GetDIBWidthBytes(INT width, INT depth)
{
    return ((width * depth + 31) & ~31) >> 3;
}

BOOL APIENTRY
GrepAlphaBlend(IN SURFOBJ *psoDest,
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

BOOLEAN
NTAPI
GrepBitBltEx(
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
    ROP4 rop4,
    BOOL bRemoveMouse)
{
    SURFACE *psurfTrg;
    SURFACE *psurfSrc = NULL;
    BOOL bResult;
    RECTL rclClipped;
    RECTL rclSrc;
    PFN_DrvBitBlt pfnBitBlt;

    ASSERT(psoTrg);
    psurfTrg = CONTAINING_RECORD(psoTrg, SURFACE, SurfObj);

    /* FIXME: Should we really allow to pass non-well-ordered rects? */
    rclClipped = *prclTrg;
    //RECTL_vMakeWellOrdered(&rclClipped);

    /* Clip target rect against the bounds of the clipping region */
    if (pco)
    {
        if (!RECTL_bIntersectRect(&rclClipped, &rclClipped, &pco->rclBounds))
        {
            /* Nothing left */
            return TRUE;
        }

        /* Don't pass a clipobj with only a single rect */
        if (pco->iDComplexity == DC_RECT)
            pco = NULL;
    }

    if (ROP4_USES_SOURCE(rop4))
    {
        ASSERT(psoSrc);
        psurfSrc = CONTAINING_RECORD(psoSrc, SURFACE, SurfObj);

        /* Calculate source rect */
        rclSrc.left = pptlSrc->x + rclClipped.left - prclTrg->left;
        rclSrc.top = pptlSrc->y + rclClipped.top - prclTrg->top;
        rclSrc.right = rclSrc.left + rclClipped.right - rclClipped.left;
        rclSrc.bottom = rclSrc.top + rclClipped.bottom - rclClipped.top;
    }
    else
    {
        psoSrc = NULL;
        psurfSrc = NULL;
    }

    if (bRemoveMouse)
    {
        SURFACE_LockBitmapBits(psurfTrg);

        if (psoSrc)
        {
            if (psoSrc != psoTrg)
            {
                SURFACE_LockBitmapBits(psurfSrc);
            }
            MouseSafetyOnDrawStart(psoSrc, rclSrc.left, rclSrc.top,
                                   rclSrc.right, rclSrc.bottom);
        }
        MouseSafetyOnDrawStart(psoTrg, rclClipped.left, rclClipped.top,
                               rclClipped.right, rclClipped.bottom);
    }

    /* Is the target surface device managed? */
    if (psurfTrg->flHooks & HOOK_BITBLT)
    {
        /* Is the source a different device managed surface? */
        if (psoSrc && psoSrc->hdev != psoTrg->hdev && psurfSrc->flHooks & HOOK_BITBLT)
        {
            DPRINT1("Need to copy to standard bitmap format!\n");
            ASSERT(FALSE);
        }

        pfnBitBlt = GDIDEVFUNCS(psoTrg).BitBlt;
    }

    /* Is the source surface device managed? */
    else if (psoSrc && psurfSrc->flHooks & HOOK_BITBLT)
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
                        (POINTL*)&rclSrc,
                        pptlMask,
                        pbo,
                        pptlBrush,
                        rop4);

    if (bRemoveMouse)
    {
        MouseSafetyOnDrawEnd(psoTrg);
        if (psoSrc)
        {
            MouseSafetyOnDrawEnd(psoSrc);
            if (psoSrc != psoTrg)
            {
                SURFACE_UnlockBitmapBits(psurfSrc);
            }
        }

        SURFACE_UnlockBitmapBits(psurfTrg);
    }

    return bResult;
}

/* PUBLIC FUNCTIONS **********************************************************/

BOOLEAN
NTAPI
GreAlphaBlend(PDC pDest, INT XDest, INT YDest,
              INT WidthDst, INT HeightDst, PDC pSrc,
              INT XSrc, INT YSrc, INT WidthSrc, INT HeightSrc,
              BLENDFUNCTION blendfn)
{
    RECTL DestRect, SrcRect;
    BOOL Status;
    XLATEOBJ *XlateObj;
    BLENDOBJ BlendObj;
    BlendObj.BlendFunction = blendfn;

    DestRect.left   = XDest;
    DestRect.top    = YDest;
    DestRect.right  = XDest+WidthDst;
    DestRect.bottom = YDest+HeightDst;

    DestRect.left += pDest->rcVport.left + pDest->rcDcRect.left;
    DestRect.top += pDest->rcVport.top + pDest->rcDcRect.top;
    DestRect.right += pDest->rcVport.left + pDest->rcDcRect.left;
    DestRect.bottom += pDest->rcVport.top + pDest->rcDcRect.top;

    SrcRect.left   = XSrc;
    SrcRect.top    = YSrc;
    SrcRect.right  = XSrc+WidthSrc;
    SrcRect.bottom = YSrc+HeightSrc;

    SrcRect.left += pSrc->rcVport.left + pSrc->rcDcRect.left;
    SrcRect.top += pSrc->rcVport.top + pSrc->rcDcRect.top;
    SrcRect.right += pSrc->rcVport.left + pSrc->rcDcRect.left;
    SrcRect.bottom += pSrc->rcVport.top + pSrc->rcDcRect.top;

    /* Create the XLATEOBJ. */
    XlateObj = IntCreateXlateForBlt(pDest, pSrc, pDest->pBitmap, pSrc->pBitmap);

    if (XlateObj == (XLATEOBJ*)-1)
    {
        DPRINT1("couldn't create XlateObj\n");
        SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
        XlateObj = NULL;
        Status = FALSE;
    }
    else
    {
        /* Perform the alpha blend operation */
        Status = GrepAlphaBlend(&pDest->pBitmap->SurfObj,
                                &pSrc->pBitmap->SurfObj,
                                pDest->CombinedClip,
                                XlateObj,
                                &DestRect,
                                &SrcRect,
                                &BlendObj);
    }

    if (XlateObj != NULL)
        EngDeleteXlate(XlateObj);

    return Status;
}

BOOLEAN
NTAPI
GreBitBlt(PDC pDest, INT XDest, INT YDest,
          INT Width, INT Height, PDC pSrc,
          INT XSrc, INT YSrc, DWORD rop)
{
    BOOLEAN bRet;
    POINT SourcePoint, BrushOrigin;
    RECTL DestRect;
    XLATEOBJ *XlateObj = NULL;
    BOOL UsesSource = ROP3_USES_SOURCE(rop);

    DestRect.left   = XDest;
    DestRect.top    = YDest;
    DestRect.right  = XDest+Width;
    DestRect.bottom = YDest+Height;

    DestRect.left += pDest->rcVport.left + pDest->rcDcRect.left;
    DestRect.top += pDest->rcVport.top + pDest->rcDcRect.top;
    DestRect.right += pDest->rcVport.left + pDest->rcDcRect.left;
    DestRect.bottom += pDest->rcVport.top + pDest->rcDcRect.top;

    SourcePoint.x = XSrc;
    SourcePoint.y = YSrc;

    if (pSrc)
    {
        SourcePoint.x += pSrc->rcDcRect.left + pSrc->rcVport.left;
        SourcePoint.y += pSrc->rcDcRect.top + pSrc->rcVport.left;
    }

    /* Create the XLATEOBJ */
    if (UsesSource)
    {
        XlateObj = IntCreateXlateForBlt(pDest, pSrc, pDest->pBitmap, pSrc->pBitmap);

        if (XlateObj == (XLATEOBJ*)-1)
        {
            DPRINT1("couldn't create XlateObj\n");
            SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
            XlateObj = NULL;
            return FALSE;
        }
    }

    BrushOrigin.x = pDest->ptBrushOrg.x + pDest->rcDcRect.left;
    BrushOrigin.y = pDest->ptBrushOrg.y + pDest->rcDcRect.top;

    /* Perform the bitblt operation */
    bRet = GrepBitBltEx(
        &pDest->pBitmap->SurfObj,
        pSrc ? &pSrc->pBitmap->SurfObj : NULL,
        NULL,
        pDest->CombinedClip,
        XlateObj,
        &DestRect,
        &SourcePoint,
        NULL,
        &pDest->pFillBrush->BrushObj,
        &BrushOrigin,
        ROP3_TO_ROP4(rop),
        TRUE);

    /* Free XlateObj if it was created */
    if (UsesSource && XlateObj)
        EngDeleteXlate(XlateObj);

    return bRet;
}

BOOLEAN
NTAPI
GrePatBlt(PDC pDC, INT XLeft, INT YLeft,
          INT Width, INT Height, DWORD ROP,
          PBRUSHGDI BrushObj)
{
    RECTL DestRect;
    POINTL BrushOrigin = {0, 0};
    BOOLEAN bRet = FALSE;

    DPRINT("GrePatBlt %p %d %d %d %d %x %p\n", pDC, XLeft, YLeft, Width, Height, ROP, BrushObj);

    if (!(BrushObj->flAttrs & GDIBRUSH_IS_NULL))
    {
        if (Width > 0)
        {
            DestRect.left = XLeft;
            DestRect.right = XLeft + Width;
        }
        else
        {
            DestRect.left = XLeft + Width + 1;
            DestRect.right = XLeft + 1;
        }

        if (Height > 0)
        {
            DestRect.top = YLeft;
            DestRect.bottom = YLeft + Height;
        }
        else
        {
            DestRect.top = YLeft + Height + 1;
            DestRect.bottom = YLeft + 1;
        }

        DestRect.left += pDC->rcVport.left + pDC->rcDcRect.left;
        DestRect.top += pDC->rcVport.top + pDC->rcDcRect.top;
        DestRect.right += pDC->rcVport.left + pDC->rcDcRect.left;
        DestRect.bottom += pDC->rcVport.top + pDC->rcDcRect.top;

        BrushOrigin.x = pDC->ptBrushOrg.x + pDC->rcDcRect.left;
        BrushOrigin.y = pDC->ptBrushOrg.y + pDC->rcDcRect.top;

        GreUpdateBrush(pDC->pFillBrush, pDC);

        bRet = GrepBitBltEx(
            &pDC->pBitmap->SurfObj,
            NULL,
            NULL,
            pDC->CombinedClip,
            NULL,
            &DestRect,
            NULL,
            NULL,
            &pDC->pFillBrush->BrushObj,
            &BrushOrigin,
            ROP3_TO_ROP4(ROP),
            TRUE);
    }

    return bRet;
}

BOOL NTAPI
GreStretchBltMask(
                PDC DCDest,
                INT  XOriginDest,
                INT  YOriginDest,
                INT  WidthDest,
                INT  HeightDest,
                PDC DCSrc,
                INT  XOriginSrc,
                INT  YOriginSrc,
                INT  WidthSrc,
                INT  HeightSrc,
                DWORD  ROP,
                IN DWORD  dwBackColor,
                PDC DCMask)
{
    SURFACE *BitmapDest, *BitmapSrc = NULL;
    SURFACE *BitmapMask = NULL;
    RECTL DestRect;
    RECTL SourceRect;
    BOOL Status = FALSE;
    XLATEOBJ *XlateObj = NULL;
    POINTL BrushOrigin;
    BOOL UsesSource = ROP3_USES_SOURCE(ROP);

    if (0 == WidthDest || 0 == HeightDest || 0 == WidthSrc || 0 == HeightSrc)
    {
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    DestRect.left   = XOriginDest;
    DestRect.top    = YOriginDest;
    DestRect.right  = XOriginDest+WidthDest;
    DestRect.bottom = YOriginDest+HeightDest;

    DestRect.left   += DCDest->rcVport.left + DCDest->rcDcRect.left;
    DestRect.top    += DCDest->rcVport.top + DCDest->rcDcRect.top;
    DestRect.right  += DCDest->rcVport.left + DCDest->rcDcRect.left;
    DestRect.bottom += DCDest->rcVport.top + DCDest->rcDcRect.top;

    SourceRect.left   = XOriginSrc;
    SourceRect.top    = YOriginSrc;
    SourceRect.right  = XOriginSrc+WidthSrc;
    SourceRect.bottom = YOriginSrc+HeightSrc;

    if (UsesSource)
    {
        SourceRect.left   += DCSrc->rcVport.left + DCSrc->rcDcRect.left;
        SourceRect.top    += DCSrc->rcVport.top + DCSrc->rcDcRect.top;
        SourceRect.right  += DCSrc->rcVport.left + DCSrc->rcDcRect.left;
        SourceRect.bottom += DCSrc->rcVport.top + DCSrc->rcDcRect.top;
    }

    BrushOrigin.x = DCDest->ptBrushOrg.x + DCDest->rcDcRect.left;
    BrushOrigin.y = DCDest->ptBrushOrg.y + DCDest->rcDcRect.top;

    /* Determine surfaces to be used in the bitblt */
    BitmapDest = DCDest->pBitmap;
    if (BitmapDest == NULL)
        goto failed;
    if (UsesSource)
    {
        {
            BitmapSrc = DCSrc->pBitmap;
            if (BitmapSrc == NULL)
                goto failed;
        }

        /* Create the XLATEOBJ. */
        XlateObj = IntCreateXlateForBlt(DCDest, DCSrc, BitmapDest, BitmapSrc);
        if (XlateObj == (XLATEOBJ*)-1)
        {
            SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
            XlateObj = NULL;
            goto failed;
        }
    }

    /* Make mask surface for source surface */
    if (BitmapSrc && DCMask)
    {
            BitmapMask = DCMask->pBitmap;
            if (BitmapMask && 
                (BitmapMask->SurfObj.sizlBitmap.cx != WidthSrc ||
                 BitmapMask->SurfObj.sizlBitmap.cy != HeightSrc))
            {
                DPRINT1("Mask and bitmap sizes don't match!\n");
                goto failed;
            }
    }

    /* Perform the bitblt operation */
    Status = EngpStretchBlt(&BitmapDest->SurfObj,
                            &BitmapSrc->SurfObj,
                            BitmapMask ? &BitmapMask->SurfObj : NULL,
                            DCDest->CombinedClip,
                            XlateObj,
                            &DestRect,
                            &SourceRect,
                            NULL, 
                            &DCDest->pFillBrush->BrushObj,
                            &BrushOrigin,
                            ROP3_TO_ROP4(ROP));

failed:
    if (XlateObj) EngDeleteXlate(XlateObj);

    return Status;
}

// Converts a DIB to a device-dependent bitmap
INT
NTAPI
GreSetDIBits(
    PDC   DC,
    HBITMAP  hBitmap,
    UINT  StartScan,
    UINT  ScanLines,
    CONST VOID  *Bits,
    CONST BITMAPINFO  *bmi,
    UINT  ColorUse)
{
    SURFACE  *bitmap;
    HBITMAP     SourceBitmap;
    INT         result = 0;
    BOOL        copyBitsResult;
    SURFOBJ    *DestSurf, *SourceSurf;
    SIZEL       SourceSize;
    POINTL      ZeroPoint;
    RECTL       DestRect;
    XLATEOBJ   *XlateObj;
    PPALETTE     hDCPalette;
    RGBQUAD    *lpRGB;
    HPALETTE    DDB_Palette, DIB_Palette;
    ULONG       DDB_Palette_Type, DIB_Palette_Type;
    INT         DIBWidth;

    // Check parameters
    if (!(bitmap = SURFACE_Lock(hBitmap)))
    {
        return 0;
    }

    // Get RGB values
    if (ColorUse == DIB_PAL_COLORS)
      lpRGB = DIB_MapPaletteColors(DC, bmi);
    else
      lpRGB = (RGBQUAD *)&bmi->bmiColors;

    DestSurf = &bitmap->SurfObj;

    // Create source surface
    SourceSize.cx = bmi->bmiHeader.biWidth;
    SourceSize.cy = ScanLines;

    // Determine width of DIB
    DIBWidth = DIB_GetDIBWidthBytes(SourceSize.cx, bmi->bmiHeader.biBitCount);

    SourceBitmap = EngCreateBitmap(SourceSize,
                                   DIBWidth,
                                   GrepBitmapFormat(bmi->bmiHeader.biBitCount, bmi->bmiHeader.biCompression),
                                   bmi->bmiHeader.biHeight < 0 ? BMF_TOPDOWN : 0,
                                   (PVOID) Bits);
    if (0 == SourceBitmap)
    {
        SURFACE_Unlock(bitmap);
        SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
        return 0;
    }

    SourceSurf = EngLockSurface((HSURF)SourceBitmap);
    if (NULL == SourceSurf)
    {
        EngDeleteSurface((HSURF)SourceBitmap);
        SURFACE_Unlock(bitmap);
        SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
        return 0;
    }

    // Use hDIBPalette if it exists
    if (bitmap->hDIBPalette)
    {
        DDB_Palette = bitmap->hDIBPalette;
    }
    else
    {
        // Destination palette obtained from the hDC
        DDB_Palette = DC->pPDevice->DevInfo.hpalDefault;
    }

    hDCPalette = PALETTE_LockPalette(DDB_Palette);
    if (NULL == hDCPalette)
    {
        EngUnlockSurface(SourceSurf);
        EngDeleteSurface((HSURF)SourceBitmap);
        SURFACE_Unlock(bitmap);
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return 0;
    }
    DDB_Palette_Type = hDCPalette->Mode;
    PALETTE_UnlockPalette(hDCPalette);

    // Source palette obtained from the BITMAPINFO
    DIB_Palette = BuildDIBPalette((PBITMAPINFO)bmi, (PINT)&DIB_Palette_Type);
    if (NULL == DIB_Palette)
    {
        EngUnlockSurface(SourceSurf);
        EngDeleteSurface((HSURF)SourceBitmap);
        SURFACE_Unlock(bitmap);
        SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
        return 0;
    }

    // Determine XLATEOBJ for color translation
    XlateObj = IntEngCreateXlate(DDB_Palette_Type, DIB_Palette_Type, DDB_Palette, DIB_Palette);
    if (NULL == XlateObj)
    {
        PALETTE_FreePaletteByHandle(DIB_Palette);
        EngUnlockSurface(SourceSurf);
        EngDeleteSurface((HSURF)SourceBitmap);
        SURFACE_Unlock(bitmap);
        SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
        return 0;
    }

    // Zero point
    ZeroPoint.x = 0;
    ZeroPoint.y = 0;

    // Determine destination rectangle
    DestRect.left	= 0;
    DestRect.top	= abs(bmi->bmiHeader.biHeight) - StartScan - ScanLines;
    DestRect.right	= SourceSize.cx;
    DestRect.bottom	= DestRect.top + ScanLines;

    copyBitsResult = GreCopyBits(DestSurf, SourceSurf, NULL, XlateObj, &DestRect, &ZeroPoint);

    // If it succeeded, return number of scanlines copies
    if (copyBitsResult == TRUE)
    {
        result = SourceSize.cy;
    }

    // Clean up
    EngDeleteXlate(XlateObj);
    PALETTE_FreePaletteByHandle(DIB_Palette);
    EngUnlockSurface(SourceSurf);
    EngDeleteSurface((HSURF)SourceBitmap);

    SURFACE_Unlock(bitmap);

    return result;
}

// Converts a device-dependent bitmap to a DIB
INT
NTAPI
GreGetDIBits(
    PDC   Dc,
    HBITMAP  hBitmap,
    UINT  StartScan,
    UINT  ScanLines,
    VOID  *Bits,
    BITMAPINFO  *Info,
    UINT  Usage)
{
    SURFACE *psurf = NULL;
    HBITMAP hDestBitmap = NULL;
    HPALETTE hSourcePalette = NULL;
    HPALETTE hDestPalette = NULL;
    PPALETTE SourcePalette = NULL;
    PPALETTE DestPalette = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Result = 0;
    BOOL bPaletteMatch = FALSE;
    PBYTE ChkBits = Bits;
    PVOID ColorPtr;
    RGBQUAD *rgbQuads;
    ULONG SourcePaletteType = 0;
    ULONG DestPaletteType;
    ULONG Index;

    DPRINT("Entered NtGdiGetDIBitsInternal()\n");

    if ((Usage && Usage != DIB_PAL_COLORS) || !Info || !hBitmap)
        return 0;

    // if ScanLines == 0, no need to copy Bits.
    if (!ScanLines)
        ChkBits = NULL;

    _SEH2_TRY
    {
        ProbeForWrite(Info, Info->bmiHeader.biSize, 1); // Comp for Core.
        //if (ChkBits) ProbeForWrite(ChkBits, MaxBits, 1);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END

    if (!NT_SUCCESS(Status))
    {
        return 0;
    }

    /* Get a pointer to the source bitmap object */
    psurf = SURFACE_Lock(hBitmap);
    if (psurf == NULL)
        return 0;

    hSourcePalette = psurf->hDIBPalette;
    if (!hSourcePalette)
    {
        hSourcePalette = pPrimarySurface->DevInfo.hpalDefault;
    }

    ColorPtr = ((PBYTE)Info + Info->bmiHeader.biSize);
    rgbQuads = (RGBQUAD *)ColorPtr;

    /* Copy palette information
     * Always create a palette for 15 & 16 bit. */
    if (Info->bmiHeader.biBitCount == BitsPerFormat(psurf->SurfObj.iBitmapFormat) &&
        Info->bmiHeader.biBitCount != 15 && Info->bmiHeader.biBitCount != 16)
    {
        hDestPalette = hSourcePalette;
        bPaletteMatch = TRUE;
    }
    else
        hDestPalette = BuildDIBPalette(Info, (PINT)&DestPaletteType); //hDestPalette = Dc->DevInfo->hpalDefault;

    SourcePalette = PALETTE_LockPalette(hSourcePalette);
    /* FIXME - SourcePalette can be NULL!!! Don't assert here! */
    ASSERT(SourcePalette);
    SourcePaletteType = SourcePalette->Mode;
    PALETTE_UnlockPalette(SourcePalette);

    if (bPaletteMatch)
    {
        DestPalette = PALETTE_LockPalette(hDestPalette);
        /* FIXME - DestPalette can be NULL!!!! Don't assert here!!! */
        DPRINT("DestPalette : %p\n", DestPalette);
        ASSERT(DestPalette);
        DestPaletteType = DestPalette->Mode;
    }
    else
    {
        DestPalette = SourcePalette;
    }

    /* Copy palette. */
    /* FIXME: This is largely incomplete. ATM no Core!*/
    switch (Info->bmiHeader.biBitCount)
    {
        case 1:
        case 4:
        case 8:
            Info->bmiHeader.biClrUsed = 0;
            if (/*psurf->hSecure &&*/
                BitsPerFormat(psurf->SurfObj.iBitmapFormat) == Info->bmiHeader.biBitCount)
            {
                if (Usage == DIB_RGB_COLORS)
                {
                    if (DestPalette->NumColors != 1 << Info->bmiHeader.biBitCount)
                        Info->bmiHeader.biClrUsed = DestPalette->NumColors;
                    for (Index = 0;
                         Index < (1 << Info->bmiHeader.biBitCount) && Index < DestPalette->NumColors;
                         Index++)
                    {
                        rgbQuads[Index].rgbRed   = DestPalette->IndexedColors[Index].peRed;
                        rgbQuads[Index].rgbGreen = DestPalette->IndexedColors[Index].peGreen;
                        rgbQuads[Index].rgbBlue  = DestPalette->IndexedColors[Index].peBlue;
                        rgbQuads[Index].rgbReserved = 0;
                    }
                }
                else
                {
                    PWORD Ptr = ColorPtr;
                    for (Index = 0;
                         Index < (1 << Info->bmiHeader.biBitCount);
                         Index++)
                    {
                        Ptr[Index] = (WORD)Index;
                    }
                }
            }
            else
            {
                if (Usage == DIB_PAL_COLORS)
                {
                    PWORD Ptr = ColorPtr;
                    for (Index = 0;
                         Index < (1 << Info->bmiHeader.biBitCount);
                         Index++)
                    {
                        Ptr[Index] = (WORD)Index;
                    }
                }
                else if (Info->bmiHeader.biBitCount > 1  && bPaletteMatch)
                {
                    for (Index = 0;
                         Index < (1 << Info->bmiHeader.biBitCount) && Index < DestPalette->NumColors;
                         Index++)
                    {
                        Info->bmiColors[Index].rgbRed   = DestPalette->IndexedColors[Index].peRed;
                        Info->bmiColors[Index].rgbGreen = DestPalette->IndexedColors[Index].peGreen;
                        Info->bmiColors[Index].rgbBlue  = DestPalette->IndexedColors[Index].peBlue;
                        Info->bmiColors[Index].rgbReserved = 0;
                    }
                }
                else
                {
                    switch (Info->bmiHeader.biBitCount)
                    {
                        case 1:
                            rgbQuads[0].rgbRed = rgbQuads[0].rgbGreen = rgbQuads[0].rgbBlue = 0;
                            rgbQuads[0].rgbReserved = 0;
                            rgbQuads[1].rgbRed = rgbQuads[1].rgbGreen = rgbQuads[1].rgbBlue = 0xff;
                            rgbQuads[1].rgbReserved = 0;
                            break;
                        case 4:
                            RtlCopyMemory(ColorPtr, EGAColorsQuads, sizeof(EGAColorsQuads));
                            break;
                        case 8:
                        {
                            INT r, g, b;
                            RGBQUAD *color;

                            RtlCopyMemory(rgbQuads, DefLogPaletteQuads, 10 * sizeof(RGBQUAD));
                            RtlCopyMemory(rgbQuads + 246, DefLogPaletteQuads + 10, 10 * sizeof(RGBQUAD));
                            color = rgbQuads + 10;
                            for (r = 0; r <= 5; r++) /* FIXME */
                                for (g = 0; g <= 5; g++)
                                    for (b = 0; b <= 5; b++)
                                    {
                                        color->rgbRed = (r * 0xff) / 5;
                                        color->rgbGreen = (g * 0xff) / 5;
                                        color->rgbBlue = (b * 0xff) / 5;
                                        color->rgbReserved = 0;
                                        color++;
                                    }
                        }
                        break;
                    }
                }
            }

        case 15:
            if (Info->bmiHeader.biCompression == BI_BITFIELDS)
            {
                ((PDWORD)Info->bmiColors)[0] = 0x7c00;
                ((PDWORD)Info->bmiColors)[1] = 0x03e0;
                ((PDWORD)Info->bmiColors)[2] = 0x001f;
            }
            break;

        case 16:
            if (Info->bmiHeader.biCompression == BI_BITFIELDS)
            {
                ((PDWORD)Info->bmiColors)[0] = 0xf800;
                ((PDWORD)Info->bmiColors)[1] = 0x07e0;
                ((PDWORD)Info->bmiColors)[2] = 0x001f;
            }
            break;

        case 24:
        case 32:
            if (Info->bmiHeader.biCompression == BI_BITFIELDS)
            {
                ((PDWORD)Info->bmiColors)[0] = 0xff0000;
                ((PDWORD)Info->bmiColors)[1] = 0x00ff00;
                ((PDWORD)Info->bmiColors)[2] = 0x0000ff;
            }
            break;
    }

    if (bPaletteMatch)
        PALETTE_UnlockPalette(DestPalette);

    /* fill out the BITMAPINFO struct */
    if (!ChkBits)
    {  // Core or not to Core? We have converted from Core in Gdi~ so?
        if (Info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
        {
            BITMAPCOREHEADER* coreheader = (BITMAPCOREHEADER*) Info;
            coreheader->bcWidth = psurf->SurfObj.sizlBitmap.cx;
            coreheader->bcPlanes = 1;
            coreheader->bcBitCount = BitsPerFormat(psurf->SurfObj.iBitmapFormat);
            coreheader->bcHeight = psurf->SurfObj.sizlBitmap.cy;
            if (psurf->SurfObj.lDelta > 0)
                coreheader->bcHeight = -coreheader->bcHeight;
        }

        if (Info->bmiHeader.biSize >= sizeof(BITMAPINFOHEADER))
        {
            Info->bmiHeader.biWidth = psurf->SurfObj.sizlBitmap.cx;
            Info->bmiHeader.biHeight = psurf->SurfObj.sizlBitmap.cy;
            Info->bmiHeader.biPlanes = 1;
            Info->bmiHeader.biBitCount = BitsPerFormat(psurf->SurfObj.iBitmapFormat);
            switch (psurf->SurfObj.iBitmapFormat)
            {
                    /* FIXME: What about BI_BITFIELDS? */
                case BMF_1BPP:
                case BMF_4BPP:
                case BMF_8BPP:
                case BMF_16BPP:
                case BMF_24BPP:
                case BMF_32BPP:
                    Info->bmiHeader.biCompression = BI_RGB;
                    break;
                case BMF_4RLE:
                    Info->bmiHeader.biCompression = BI_RLE4;
                    break;
                case BMF_8RLE:
                    Info->bmiHeader.biCompression = BI_RLE8;
                    break;
                case BMF_JPEG:
                    Info->bmiHeader.biCompression = BI_JPEG;
                    break;
                case BMF_PNG:
                    Info->bmiHeader.biCompression = BI_PNG;
                    break;
            }
            /* Image size has to be calculated */
            Info->bmiHeader.biSizeImage = DIB_GetDIBWidthBytes(Info->bmiHeader.biWidth,
                                          Info->bmiHeader.biBitCount) * Info->bmiHeader.biHeight;
            Info->bmiHeader.biXPelsPerMeter = 0; /* FIXME */
            Info->bmiHeader.biYPelsPerMeter = 0; /* FIXME */
            Info->bmiHeader.biClrUsed = 0;
            Info->bmiHeader.biClrImportant = 1 << Info->bmiHeader.biBitCount; /* FIXME */
            /* Report negtive height for top-down bitmaps. */
            if (psurf->SurfObj.lDelta > 0)
                Info->bmiHeader.biHeight = -Info->bmiHeader.biHeight;
        }
        Result = psurf->SurfObj.sizlBitmap.cy;
    }
    else
    {
        SIZEL DestSize;
        POINTL SourcePoint;

//
// If we have a good dib pointer, why not just copy bits from there w/o XLATE'ing them.
//
        /* Create the destination bitmap too for the copy operation */
        if (StartScan > psurf->SurfObj.sizlBitmap.cy)
        {
            goto cleanup;
        }
        else
        {
            ScanLines = min(ScanLines, psurf->SurfObj.sizlBitmap.cy - StartScan);
            DestSize.cx = psurf->SurfObj.sizlBitmap.cx;
            DestSize.cy = ScanLines;

            hDestBitmap = NULL;

            if (Info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
            {
                BITMAPCOREHEADER* coreheader = (BITMAPCOREHEADER*) Info;
                hDestBitmap = EngCreateBitmap(DestSize,
                                              DIB_GetDIBWidthBytes(DestSize.cx, coreheader->bcBitCount),
                                              GrepBitmapFormat(coreheader->bcBitCount, BI_RGB),
                                              0 < coreheader->bcHeight ? 0 : BMF_TOPDOWN,
                                              Bits);
            }

            if (Info->bmiHeader.biSize >= sizeof(BITMAPINFOHEADER))
            {
                Info->bmiHeader.biSizeImage = DIB_GetDIBWidthBytes(DestSize.cx,
                                              Info->bmiHeader.biBitCount) * DestSize.cy;

                hDestBitmap = EngCreateBitmap(DestSize,
                                              DIB_GetDIBWidthBytes(DestSize.cx, Info->bmiHeader.biBitCount),
                                              GrepBitmapFormat(Info->bmiHeader.biBitCount, Info->bmiHeader.biCompression),
                                              0 < Info->bmiHeader.biHeight ? 0 : BMF_TOPDOWN,
                                              Bits);
            }

            if (hDestBitmap == NULL)
                goto cleanup;
        }

        if (NT_SUCCESS(Status))
        {
            XLATEOBJ *XlateObj;
            SURFOBJ *DestSurfObj;
            RECTL DestRect;

            XlateObj = IntEngCreateXlate(DestPaletteType,
                                         SourcePaletteType,
                                         hDestPalette,
                                         hSourcePalette);

            SourcePoint.x = 0;
            SourcePoint.y = psurf->SurfObj.sizlBitmap.cy - (StartScan + ScanLines);

            /* Determine destination rectangle */
            DestRect.top = 0;
            DestRect.left = 0;
            DestRect.right = DestSize.cx;
            DestRect.bottom = DestSize.cy;

            DestSurfObj = EngLockSurface((HSURF)hDestBitmap);

            if (GreCopyBits(DestSurfObj,
                            &psurf->SurfObj,
                            NULL,
                            XlateObj,
                            &DestRect,
                            &SourcePoint))
            {
                DPRINT("GetDIBits %d \n",abs(Info->bmiHeader.biHeight) - StartScan);
                Result = ScanLines;
            }

            EngDeleteXlate(XlateObj);
            EngUnlockSurface(DestSurfObj);
        }
    }
cleanup:
    if (hDestBitmap != NULL)
        EngDeleteSurface((HSURF)hDestBitmap);

    if (hDestPalette != NULL && bPaletteMatch == FALSE)
        PALETTE_FreePaletteByHandle(hDestPalette);

    SURFACE_Unlock(psurf);

    DPRINT("leaving GreGetDIBits\n");

    return Result;
}

/* EOF */
