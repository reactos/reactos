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

INT NTAPI DIB_GetDIBWidthBytes(INT width, INT depth)
{
    return ((width * depth + 31) & ~31) >> 3;
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
        DPRINT1("psoSrc %p\n", psoSrc);
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
GreBitBlt(PDC pDest, INT XDest, INT YDest,
          INT Width, INT Height, PDC pSrc,
          INT XSrc, INT YSrc, DWORD rop)
{
    BOOLEAN bRet;
    POINT BrushOrigin = {0,0};
    POINT SourcePoint;
    RECTL DestRect;
    XLATEOBJ *XlateObj = NULL;
    BOOL UsesSource = ROP3_USES_SOURCE(rop);

    DestRect.left   = XDest;
    DestRect.top    = YDest;
    DestRect.right  = XDest+Width;
    DestRect.bottom = YDest+Height;
    // FIXME: LP->DP missing!
    //IntLPtoDP(pDest, (LPPOINT)&DestRect, 2);

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

    /* Perform the bitblt operation */
    bRet = GrepBitBltEx(
        &pDest->pBitmap->SurfObj,
        pSrc ? &pSrc->pBitmap->SurfObj : NULL,
        NULL,
        NULL,//dc->rosdc.CombinedClip,
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

        //BrushOrigin.x = BrushObj->ptOrigin.x + pDC->ptVportOrg.x;
        //BrushOrigin.y = BrushObj->ptOrigin.y + pDC->ptVportOrg.y;

        bRet = GrepBitBltEx(
            &pDC->pBitmap->SurfObj,
            NULL,
            NULL,
            NULL,//dc->rosdc.CombinedClip,
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
    IntLPtoDP(DCDest, (LPPOINT)&DestRect, 2); // FIXME: Why?

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
        IntLPtoDP(DCSrc, (LPPOINT)&SourceRect, 2);

        SourceRect.left   += DCSrc->rcVport.left + DCSrc->rcDcRect.left;
        SourceRect.top    += DCSrc->rcVport.top + DCSrc->rcDcRect.top;
        SourceRect.right  += DCSrc->rcVport.left + DCSrc->rcDcRect.left;
        SourceRect.bottom += DCSrc->rcVport.top + DCSrc->rcDcRect.top;
    }

    BrushOrigin.x = 0;
    BrushOrigin.y = 0;

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
        XlateObj = NULL;/*IntCreateXlateForBlt(DCDest, DCSrc, BitmapDest, BitmapSrc);
        if (XlateObj == (XLATEOBJ*)-1)
        {
            SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
            XlateObj = NULL;
            goto failed;
        }*/
    }

    /* Offset the brush */
    BrushOrigin.x += DCDest->rcVport.left + DCDest->rcDcRect.left;
    BrushOrigin.y += DCDest->rcVport.top + DCDest->rcDcRect.top;

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
                            NULL,//DCDest->CombinedClip,
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

    copyBitsResult = EngCopyBits(DestSurf, SourceSurf, NULL, XlateObj, &DestRect, &ZeroPoint);

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

/* EOF */
