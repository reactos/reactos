/*
 * COPYRIGHT:        GNU GPL, See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Bit blit functions
 * FILE:             subsys/win32k/objects/bitblt.c
 * PROGRAMER:        Unknown
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(GdiBlt);

#define ROP_USES_SOURCE(Rop)  (((((Rop) & 0xCC0000) >> 2) != ((Rop) & 0x330000)) || ((((Rop) & 0xCC000000) >> 2) != ((Rop) & 0x33000000)))
#define ROP_USES_MASK(Rop)    (((Rop) & 0xFF000000) != (((Rop) & 0xff0000) << 8))

#define FIXUP_ROP(Rop) if(((Rop) & 0xFF000000) == 0) Rop = MAKEROP4((Rop), (Rop))
#define ROP_TO_ROP4(Rop) ((Rop) >> 16)

BOOL APIENTRY
NtGdiAlphaBlend(
    HDC hDCDest,
    LONG XOriginDest,
    LONG YOriginDest,
    LONG WidthDest,
    LONG HeightDest,
    HDC hDCSrc,
    LONG XOriginSrc,
    LONG YOriginSrc,
    LONG WidthSrc,
    LONG HeightSrc,
    BLENDFUNCTION BlendFunc,
    HANDLE hcmXform)
{
    PDC DCDest;
    PDC DCSrc;
    HDC ahDC[2];
    PGDIOBJ apObj[2];
    SURFACE *BitmapDest, *BitmapSrc;
    RECTL DestRect, SourceRect;
    BOOL bResult;
    EXLATEOBJ exlo;
    BLENDOBJ BlendObj;
    BlendObj.BlendFunction = BlendFunc;

    if (WidthDest < 0 || HeightDest < 0 || WidthSrc < 0 || HeightSrc < 0)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ((hDCDest == NULL) || (hDCSrc == NULL))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    TRACE("Locking DCs\n");
    ahDC[0] = hDCDest;
    ahDC[1] = hDCSrc ;
    if (!GDIOBJ_bLockMultipleObjects(2, (HGDIOBJ*)ahDC, apObj, GDIObjType_DC_TYPE))
    {
        WARN("Invalid dc handle (dest=0x%p, src=0x%p) passed to NtGdiAlphaBlend\n", hDCDest, hDCSrc);
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    DCDest = apObj[0];
    DCSrc = apObj[1];

    if (DCDest->dctype == DC_TYPE_INFO || DCDest->dctype == DCTYPE_INFO)
    {
        GDIOBJ_vUnlockObject(&DCSrc->BaseObject);
        GDIOBJ_vUnlockObject(&DCDest->BaseObject);
        /* Yes, Windows really returns TRUE in this case */
        return TRUE;
    }

    DestRect.left   = XOriginDest;
    DestRect.top    = YOriginDest;
    DestRect.right  = XOriginDest + WidthDest;
    DestRect.bottom = YOriginDest + HeightDest;
    IntLPtoDP(DCDest, (LPPOINT)&DestRect, 2);

    DestRect.left   += DCDest->ptlDCOrig.x;
    DestRect.top    += DCDest->ptlDCOrig.y;
    DestRect.right  += DCDest->ptlDCOrig.x;
    DestRect.bottom += DCDest->ptlDCOrig.y;

    SourceRect.left   = XOriginSrc;
    SourceRect.top    = YOriginSrc;
    SourceRect.right  = XOriginSrc + WidthSrc;
    SourceRect.bottom = YOriginSrc + HeightSrc;
    IntLPtoDP(DCSrc, (LPPOINT)&SourceRect, 2);

    SourceRect.left   += DCSrc->ptlDCOrig.x;
    SourceRect.top    += DCSrc->ptlDCOrig.y;
    SourceRect.right  += DCSrc->ptlDCOrig.x;
    SourceRect.bottom += DCSrc->ptlDCOrig.y;

    if (!DestRect.right ||
        !DestRect.bottom ||
        !SourceRect.right ||
        !SourceRect.bottom)
    {
        GDIOBJ_vUnlockObject(&DCSrc->BaseObject);
        GDIOBJ_vUnlockObject(&DCDest->BaseObject);
        return TRUE;
    }

    /* Prepare DCs for blit */
    TRACE("Preparing DCs for blit\n");
    DC_vPrepareDCsForBlit(DCDest, &DestRect, DCSrc, &SourceRect);

    /* Determine surfaces to be used in the bitblt */
    BitmapDest = DCDest->dclevel.pSurface;
    if (!BitmapDest)
    {
        bResult = FALSE ;
        goto leave ;
    }

    BitmapSrc = DCSrc->dclevel.pSurface;
    if (!BitmapSrc)
    {
        bResult = FALSE;
        goto leave;
    }

    /* Create the XLATEOBJ. */
    EXLATEOBJ_vInitXlateFromDCs(&exlo, DCSrc, DCDest);

    /* Perform the alpha blend operation */
    TRACE("Performing the alpha blend\n");
    bResult = IntEngAlphaBlend(&BitmapDest->SurfObj,
                               &BitmapSrc->SurfObj,
                               &DCDest->co.ClipObj,
                               &exlo.xlo,
                               &DestRect,
                               &SourceRect,
                               &BlendObj);

    EXLATEOBJ_vCleanup(&exlo);
leave :
    TRACE("Finishing blit\n");
    DC_vFinishBlit(DCDest, DCSrc);
    GDIOBJ_vUnlockObject(&DCSrc->BaseObject);
    GDIOBJ_vUnlockObject(&DCDest->BaseObject);

    return bResult;
}

BOOL APIENTRY
NtGdiBitBlt(
    HDC hDCDest,
    INT XDest,
    INT YDest,
    INT Width,
    INT Height,
    HDC hDCSrc,
    INT XSrc,
    INT YSrc,
    DWORD ROP,
    IN DWORD crBackColor,
    IN FLONG fl)
{
    DWORD dwTRop;

    if (ROP & CAPTUREBLT)
       return NtGdiStretchBlt(hDCDest,
                              XDest,
                              YDest,
                              Width,
                              Height,
                              hDCSrc,
                              XSrc,
                              YSrc,
                              Width,
                              Height,
                              ROP,
                              crBackColor);

    dwTRop = ROP & ~(NOMIRRORBITMAP|CAPTUREBLT);

    /* Forward to NtGdiMaskBlt */
    // TODO: What's fl for? LOL not to send this to MaskBit!
    return NtGdiMaskBlt(hDCDest,
                        XDest,
                        YDest,
                        Width,
                        Height,
                        hDCSrc,
                        XSrc,
                        YSrc,
                        NULL,
                        0,
                        0,
                        dwTRop,
                        crBackColor);
}

BOOL APIENTRY
NtGdiTransparentBlt(
    HDC hdcDst,
    INT xDst,
    INT yDst,
    INT cxDst,
    INT cyDst,
    HDC hdcSrc,
    INT xSrc,
    INT ySrc,
    INT cxSrc,
    INT cySrc,
    COLORREF TransColor)
{
    PDC DCDest, DCSrc;
    HDC ahDC[2];
    PGDIOBJ apObj[2];
    RECTL rcDest, rcSrc;
    SURFACE *BitmapDest, *BitmapSrc = NULL;
    ULONG TransparentColor = 0;
    BOOL Ret = FALSE;
    EXLATEOBJ exlo;

    if ((hdcDst == NULL) || (hdcSrc == NULL))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    TRACE("Locking DCs\n");
    ahDC[0] = hdcDst;
    ahDC[1] = hdcSrc ;
    if (!GDIOBJ_bLockMultipleObjects(2, (HGDIOBJ*)ahDC, apObj, GDIObjType_DC_TYPE))
    {
        WARN("Invalid dc handle (dest=0x%p, src=0x%p) passed to NtGdiAlphaBlend\n", hdcDst, hdcSrc);
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    DCDest = apObj[0];
    DCSrc = apObj[1];

    if (DCDest->dctype == DC_TYPE_INFO || DCDest->dctype == DCTYPE_INFO)
    {
        GDIOBJ_vUnlockObject(&DCSrc->BaseObject);
        GDIOBJ_vUnlockObject(&DCDest->BaseObject);
        /* Yes, Windows really returns TRUE in this case */
        return TRUE;
    }

    rcDest.left   = xDst;
    rcDest.top    = yDst;
    rcDest.right  = rcDest.left + cxDst;
    rcDest.bottom = rcDest.top + cyDst;
    IntLPtoDP(DCDest, (LPPOINT)&rcDest, 2);

    rcDest.left   += DCDest->ptlDCOrig.x;
    rcDest.top    += DCDest->ptlDCOrig.y;
    rcDest.right  += DCDest->ptlDCOrig.x;
    rcDest.bottom += DCDest->ptlDCOrig.y;

    rcSrc.left   = xSrc;
    rcSrc.top    = ySrc;
    rcSrc.right  = rcSrc.left + cxSrc;
    rcSrc.bottom = rcSrc.top + cySrc;
    IntLPtoDP(DCSrc, (LPPOINT)&rcSrc, 2);

    rcSrc.left   += DCSrc->ptlDCOrig.x;
    rcSrc.top    += DCSrc->ptlDCOrig.y;
    rcSrc.right  += DCSrc->ptlDCOrig.x;
    rcSrc.bottom += DCSrc->ptlDCOrig.y;

    /* Prepare for blit */
    DC_vPrepareDCsForBlit(DCDest, &rcDest, DCSrc, &rcSrc);

    BitmapDest = DCDest->dclevel.pSurface;
    if (!BitmapDest)
    {
        goto done;
    }

    BitmapSrc = DCSrc->dclevel.pSurface;
    if (!BitmapSrc)
    {
        goto done;
    }

    /* Translate Transparent (RGB) Color to the source palette */
    EXLATEOBJ_vInitialize(&exlo, &gpalRGB, BitmapSrc->ppal, 0, 0, 0);
    TransparentColor = XLATEOBJ_iXlate(&exlo.xlo, (ULONG)TransColor);
    EXLATEOBJ_vCleanup(&exlo);

    EXLATEOBJ_vInitXlateFromDCs(&exlo, DCSrc, DCDest);

    Ret = IntEngTransparentBlt(&BitmapDest->SurfObj, &BitmapSrc->SurfObj,
        &DCDest->co.ClipObj, &exlo.xlo, &rcDest, &rcSrc,
        TransparentColor, 0);

    EXLATEOBJ_vCleanup(&exlo);

done:
    DC_vFinishBlit(DCDest, DCSrc);
    GDIOBJ_vUnlockObject(&DCDest->BaseObject);
    GDIOBJ_vUnlockObject(&DCSrc->BaseObject);

    return Ret;
}

BOOL APIENTRY
NtGdiMaskBlt(
    HDC hdcDest,
    INT nXDest,
    INT nYDest,
    INT nWidth,
    INT nHeight,
    HDC hdcSrc,
    INT nXSrc,
    INT nYSrc,
    HBITMAP hbmMask,
    INT xMask,
    INT yMask,
    DWORD dwRop,
    IN DWORD crBackColor)
{
    PDC DCDest;
    PDC DCSrc = NULL;
    HDC ahDC[2];
    PGDIOBJ apObj[2];
    PDC_ATTR pdcattr = NULL;
    SURFACE *BitmapDest, *BitmapSrc = NULL, *psurfMask = NULL;
    RECTL DestRect, SourceRect;
    POINTL SourcePoint, MaskPoint;
    BOOL Status = FALSE;
    EXLATEOBJ exlo;
    XLATEOBJ *XlateObj = NULL;
    BOOL UsesSource = ROP_USES_SOURCE(dwRop);
    BOOL UsesMask;

    FIXUP_ROP(dwRop);

    UsesMask = ROP_USES_MASK(dwRop);

    //DPRINT1("dwRop : 0x%08x\n", dwRop);
    if (!hdcDest || (UsesSource && !hdcSrc))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Take care of mask bitmap */
    if(hbmMask)
    {
        psurfMask = SURFACE_ShareLockSurface(hbmMask);
        if(!psurfMask)
        {
            EngSetLastError(ERROR_INVALID_HANDLE);
            return FALSE;
        }
    }

    if(UsesMask)
    {
        if(!psurfMask)
        {
            EngSetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        if(gajBitsPerFormat[psurfMask->SurfObj.iBitmapFormat] != 1)
        {
            EngSetLastError(ERROR_INVALID_PARAMETER);
            SURFACE_ShareUnlockSurface(psurfMask);
            return FALSE;
        }
    }
    else if(psurfMask)
    {
        WARN("Getting Mask bitmap without needing it?\n");
        SURFACE_ShareUnlockSurface(psurfMask);
        psurfMask = NULL;
    }
    MaskPoint.x = xMask;
    MaskPoint.y = yMask;

    /* Take care of source and destination bitmap */
    TRACE("Locking DCs\n");
    ahDC[0] = hdcDest;
    ahDC[1] = UsesSource ? hdcSrc : NULL;
    if (!GDIOBJ_bLockMultipleObjects(2, (HGDIOBJ*)ahDC, apObj, GDIObjType_DC_TYPE))
    {
        WARN("Invalid dc handle (dest=0x%p, src=0x%p) passed to NtGdiAlphaBlend\n", hdcDest, hdcSrc);
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    DCDest = apObj[0];
    DCSrc = apObj[1];

    ASSERT(DCDest);
    if (NULL == DCDest)
    {
        if(DCSrc) DC_UnlockDc(DCSrc);
        WARN("Invalid destination dc handle (0x%p) passed to NtGdiBitBlt\n", hdcDest);
        return FALSE;
    }

    if (DCDest->dctype == DC_TYPE_INFO)
    {
        if(DCSrc) DC_UnlockDc(DCSrc);
        DC_UnlockDc(DCDest);
        /* Yes, Windows really returns TRUE in this case */
        return TRUE;
    }

    if (UsesSource)
    {
        ASSERT(DCSrc);
        if (DCSrc->dctype == DC_TYPE_INFO)
        {
            DC_UnlockDc(DCDest);
            DC_UnlockDc(DCSrc);
            /* Yes, Windows really returns TRUE in this case */
            return TRUE;
        }
    }

    pdcattr = DCDest->pdcattr;

    DestRect.left   = nXDest;
    DestRect.top    = nYDest;
    DestRect.right  = nXDest + nWidth;
    DestRect.bottom = nYDest + nHeight;
    IntLPtoDP(DCDest, (LPPOINT)&DestRect, 2);

    DestRect.left   += DCDest->ptlDCOrig.x;
    DestRect.top    += DCDest->ptlDCOrig.y;
    DestRect.right  += DCDest->ptlDCOrig.x;
    DestRect.bottom += DCDest->ptlDCOrig.y;

    SourcePoint.x = nXSrc;
    SourcePoint.y = nYSrc;

    if (UsesSource)
    {
        IntLPtoDP(DCSrc, (LPPOINT)&SourcePoint, 1);

        SourcePoint.x += DCSrc->ptlDCOrig.x;
        SourcePoint.y += DCSrc->ptlDCOrig.y;
        /* Calculate Source Rect */
        SourceRect.left = SourcePoint.x;
        SourceRect.top = SourcePoint.y;
        SourceRect.right = SourcePoint.x + DestRect.right - DestRect.left;
        SourceRect.bottom = SourcePoint.y + DestRect.bottom - DestRect.top ;
    }
    else
    {
        SourceRect.left = 0;
        SourceRect.top = 0;
        SourceRect.right = 0;
        SourceRect.bottom = 0;
    }

    /* Prepare blit */
    DC_vPrepareDCsForBlit(DCDest, &DestRect, DCSrc, &SourceRect);

    if (pdcattr->ulDirty_ & (DIRTY_FILL | DC_BRUSH_DIRTY))
        DC_vUpdateFillBrush(DCDest);

    /* Determine surfaces to be used in the bitblt */
    BitmapDest = DCDest->dclevel.pSurface;
    if (!BitmapDest)
        goto cleanup;

    if (UsesSource)
    {
        {
            BitmapSrc = DCSrc->dclevel.pSurface;
            if (!BitmapSrc)
                goto cleanup;
        }
    }

    /* Create the XLATEOBJ. */
    if (UsesSource)
    {
        EXLATEOBJ_vInitXlateFromDCs(&exlo, DCSrc, DCDest);
        XlateObj = &exlo.xlo;
    }


    /* Perform the bitblt operation */
    Status = IntEngBitBlt(&BitmapDest->SurfObj,
                          BitmapSrc ? &BitmapSrc->SurfObj : NULL,
                          psurfMask ? &psurfMask->SurfObj : NULL,
                          &DCDest->co.ClipObj,
                          XlateObj,
                          &DestRect,
                          &SourcePoint,
                          &MaskPoint,
                          &DCDest->eboFill.BrushObject,
                          &DCDest->dclevel.pbrFill->ptOrigin,
                          ROP_TO_ROP4(dwRop));

    if (UsesSource)
        EXLATEOBJ_vCleanup(&exlo);
cleanup:
    DC_vFinishBlit(DCDest, DCSrc);
    if (UsesSource)
    {
        DC_UnlockDc(DCSrc);
    }
    DC_UnlockDc(DCDest);
    if(psurfMask) SURFACE_ShareUnlockSurface(psurfMask);

    return Status;
}

BOOL
APIENTRY
NtGdiPlgBlt(
    IN HDC hdcTrg,
    IN LPPOINT pptlTrg,
    IN HDC hdcSrc,
    IN INT xSrc,
    IN INT ySrc,
    IN INT cxSrc,
    IN INT cySrc,
    IN HBITMAP hbmMask,
    IN INT xMask,
    IN INT yMask,
    IN DWORD crBackColor)
{
    FIXME("NtGdiPlgBlt: unimplemented.\n");
    return FALSE;
}

BOOL APIENTRY
GreStretchBltMask(
    HDC hDCDest,
    INT XOriginDest,
    INT YOriginDest,
    INT WidthDest,
    INT HeightDest,
    HDC hDCSrc,
    INT XOriginSrc,
    INT YOriginSrc,
    INT WidthSrc,
    INT HeightSrc,
    DWORD ROP,
    IN DWORD dwBackColor,
    HDC hDCMask,
    INT XOriginMask,
    INT YOriginMask)
{
    PDC DCDest;
    PDC DCSrc  = NULL;
    PDC DCMask = NULL;
    HDC ahDC[3];
    PGDIOBJ apObj[3];
    PDC_ATTR pdcattr;
    SURFACE *BitmapDest, *BitmapSrc = NULL;
    SURFACE *BitmapMask = NULL;
    RECTL DestRect;
    RECTL SourceRect;
    POINTL MaskPoint;
    BOOL Status = FALSE;
    EXLATEOBJ exlo;
    XLATEOBJ *XlateObj = NULL;
    POINTL BrushOrigin;
    BOOL UsesSource;
    BOOL UsesMask;

    FIXUP_ROP(ROP);
    UsesSource = ROP_USES_SOURCE(ROP);
    UsesMask = ROP_USES_MASK(ROP);

    if (0 == WidthDest || 0 == HeightDest || 0 == WidthSrc || 0 == HeightSrc)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return TRUE;
    }

    if (!hDCDest || (UsesSource && !hDCSrc) || (UsesMask && !hDCMask))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ahDC[0] = hDCDest;
    ahDC[1] = UsesSource ? hDCSrc : NULL;
    ahDC[2] = UsesMask ? hDCMask : NULL;
    if (!GDIOBJ_bLockMultipleObjects(3, (HGDIOBJ*)ahDC, apObj, GDIObjType_DC_TYPE))
    {
        WARN("Invalid dc handle (dest=0x%p, src=0x%p) passed to GreStretchBltMask\n", hDCDest, hDCSrc);
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    DCDest = apObj[0];
    DCSrc = apObj[1];
    DCMask = apObj[2];

    if (DCDest->dctype == DC_TYPE_INFO)
    {
        if(DCSrc) GDIOBJ_vUnlockObject(&DCSrc->BaseObject);
        if(DCMask) GDIOBJ_vUnlockObject(&DCMask->BaseObject);
        GDIOBJ_vUnlockObject(&DCDest->BaseObject);
        /* Yes, Windows really returns TRUE in this case */
        return TRUE;
    }

    if (UsesSource)
    {
        if (DCSrc->dctype == DC_TYPE_INFO)
        {
            GDIOBJ_vUnlockObject(&DCDest->BaseObject);
            GDIOBJ_vUnlockObject(&DCSrc->BaseObject);
            if(DCMask) GDIOBJ_vUnlockObject(&DCMask->BaseObject);
            /* Yes, Windows really returns TRUE in this case */
            return TRUE;
        }
    }

    pdcattr = DCDest->pdcattr;

    DestRect.left   = XOriginDest;
    DestRect.top    = YOriginDest;
    DestRect.right  = XOriginDest+WidthDest;
    DestRect.bottom = YOriginDest+HeightDest;
    IntLPtoDP(DCDest, (LPPOINT)&DestRect, 2);

    DestRect.left   += DCDest->ptlDCOrig.x;
    DestRect.top    += DCDest->ptlDCOrig.y;
    DestRect.right  += DCDest->ptlDCOrig.x;
    DestRect.bottom += DCDest->ptlDCOrig.y;

    SourceRect.left   = XOriginSrc;
    SourceRect.top    = YOriginSrc;
    SourceRect.right  = XOriginSrc+WidthSrc;
    SourceRect.bottom = YOriginSrc+HeightSrc;

    if (UsesSource)
    {
        IntLPtoDP(DCSrc, (LPPOINT)&SourceRect, 2);

        SourceRect.left   += DCSrc->ptlDCOrig.x;
        SourceRect.top    += DCSrc->ptlDCOrig.y;
        SourceRect.right  += DCSrc->ptlDCOrig.x;
        SourceRect.bottom += DCSrc->ptlDCOrig.y;
    }

    BrushOrigin.x = 0;
    BrushOrigin.y = 0;

    /* Only prepare Source and Dest, hdcMask represents a DIB */
    DC_vPrepareDCsForBlit(DCDest, &DestRect, DCSrc, &SourceRect);

    if (pdcattr->ulDirty_ & (DIRTY_FILL | DC_BRUSH_DIRTY))
        DC_vUpdateFillBrush(DCDest);

    /* Determine surfaces to be used in the bitblt */
    BitmapDest = DCDest->dclevel.pSurface;
    if (BitmapDest == NULL)
        goto failed;
    if (UsesSource)
    {
        BitmapSrc = DCSrc->dclevel.pSurface;
        if (BitmapSrc == NULL)
            goto failed;

        /* Create the XLATEOBJ. */
        EXLATEOBJ_vInitXlateFromDCs(&exlo, DCSrc, DCDest);
        XlateObj = &exlo.xlo;
    }

    /* Offset the brush */
    BrushOrigin.x += DCDest->ptlDCOrig.x;
    BrushOrigin.y += DCDest->ptlDCOrig.y;

    /* Make mask surface for source surface */
    if (BitmapSrc && DCMask)
    {
        BitmapMask = DCMask->dclevel.pSurface;
        if (BitmapMask &&
            (BitmapMask->SurfObj.sizlBitmap.cx < WidthSrc ||
             BitmapMask->SurfObj.sizlBitmap.cy < HeightSrc))
        {
            WARN("%dx%d mask is smaller than %dx%d bitmap\n",
                    BitmapMask->SurfObj.sizlBitmap.cx, BitmapMask->SurfObj.sizlBitmap.cy,
                    WidthSrc, HeightSrc);
            EXLATEOBJ_vCleanup(&exlo);
            goto failed;
        }
        /* Create mask offset point */
        MaskPoint.x = XOriginMask;
        MaskPoint.y = YOriginMask;
        IntLPtoDP(DCMask, &MaskPoint, 1);
        MaskPoint.x += DCMask->ptlDCOrig.x;
        MaskPoint.y += DCMask->ptlDCOrig.y;
    }

    /* Perform the bitblt operation */
    Status = IntEngStretchBlt(&BitmapDest->SurfObj,
                              BitmapSrc ? &BitmapSrc->SurfObj : NULL,
                              BitmapMask ? &BitmapMask->SurfObj : NULL,
                              &DCDest->co.ClipObj,
                              XlateObj,
                              &DCDest->dclevel.ca,
                              &DestRect,
                              &SourceRect,
                              BitmapMask ? &MaskPoint : NULL,
                              &DCDest->eboFill.BrushObject,
                              &BrushOrigin,
                              ROP_TO_ROP4(ROP));
    if (UsesSource)
    {
        EXLATEOBJ_vCleanup(&exlo);
    }

failed:
    DC_vFinishBlit(DCDest, DCSrc);
    if (UsesSource)
    {
        DC_UnlockDc(DCSrc);
    }
    if (DCMask)
    {
        DC_UnlockDc(DCMask);
    }
    DC_UnlockDc(DCDest);

    return Status;
}


BOOL APIENTRY
NtGdiStretchBlt(
    HDC hDCDest,
    INT XOriginDest,
    INT YOriginDest,
    INT WidthDest,
    INT HeightDest,
    HDC hDCSrc,
    INT XOriginSrc,
    INT YOriginSrc,
    INT WidthSrc,
    INT HeightSrc,
    DWORD ROP,
    IN DWORD dwBackColor)
{
    DWORD dwTRop = ROP & ~(NOMIRRORBITMAP|CAPTUREBLT);

    return GreStretchBltMask(
                hDCDest,
                XOriginDest,
                YOriginDest,
                WidthDest,
                HeightDest,
                hDCSrc,
                XOriginSrc,
                YOriginSrc,
                WidthSrc,
                HeightSrc,
                dwTRop,
                dwBackColor,
                NULL,
                0,
                0);
}


BOOL FASTCALL
IntPatBlt(
    PDC pdc,
    INT XLeft,
    INT YLeft,
    INT Width,
    INT Height,
    DWORD dwRop,
    PEBRUSHOBJ pebo)
{
    RECTL DestRect;
    SURFACE *psurf;
    POINTL BrushOrigin;
    BOOL ret;
    PBRUSH pbrush;

    ASSERT(pebo);
    pbrush = pebo->pbrush;
    ASSERT(pbrush);

    FIXUP_ROP(dwRop);

    if (pbrush->flAttrs & BR_IS_NULL)
    {
        return TRUE;
    }

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

    IntLPtoDP(pdc, (LPPOINT)&DestRect, 2);

    DestRect.left   += pdc->ptlDCOrig.x;
    DestRect.top    += pdc->ptlDCOrig.y;
    DestRect.right  += pdc->ptlDCOrig.x;
    DestRect.bottom += pdc->ptlDCOrig.y;
#ifdef _USE_DIBLIB_
    BrushOrigin.x = pbrush->ptOrigin.x + pdc->ptlDCOrig.x + XLeft;
    BrushOrigin.y = pbrush->ptOrigin.y + pdc->ptlDCOrig.y + YLeft;
#else
    BrushOrigin.x = pbrush->ptOrigin.x + pdc->ptlDCOrig.x;
    BrushOrigin.y = pbrush->ptOrigin.y + pdc->ptlDCOrig.y;
#endif

    DC_vPrepareDCsForBlit(pdc, &DestRect, NULL, NULL);

    psurf = pdc->dclevel.pSurface;

    ret = IntEngBitBlt(
        &psurf->SurfObj,
        NULL,
        NULL,
        &pdc->co.ClipObj,
        NULL,
        &DestRect,
        NULL,
        NULL,
        &pebo->BrushObject,
        &BrushOrigin,
        ROP_TO_ROP4(dwRop));

    DC_vFinishBlit(pdc, NULL);

    return ret;
}

BOOL FASTCALL
IntGdiPolyPatBlt(
    HDC hDC,
    DWORD dwRop,
    PPATRECT pRects,
    INT cRects,
    ULONG Reserved)
{
    INT i;
    PBRUSH pbrush;
    PDC pdc;
    EBRUSHOBJ eboFill;

    pdc = DC_LockDc(hDC);
    if (!pdc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pdc->dctype == DC_TYPE_INFO)
    {
        DC_UnlockDc(pdc);
        /* Yes, Windows really returns TRUE in this case */
        return TRUE;
    }

    for (i = 0; i < cRects; i++)
    {
        pbrush = BRUSH_ShareLockBrush(pRects->hBrush);

        /* Check if we could lock the brush */
        if (pbrush != NULL)
        {
            /* Initialize a brush object */
            EBRUSHOBJ_vInitFromDC(&eboFill, pbrush, pdc);

            IntPatBlt(
                pdc,
                pRects->r.left,
                pRects->r.top,
                pRects->r.right,
                pRects->r.bottom,
                dwRop,
                &eboFill);

            /* Cleanup the brush object and unlock the brush */
            EBRUSHOBJ_vCleanup(&eboFill);
            BRUSH_ShareUnlockBrush(pbrush);
        }
        pRects++;
    }

    DC_UnlockDc(pdc);

    return TRUE;
}

BOOL
APIENTRY
NtGdiPatBlt(
    _In_ HDC hdcDest,
    _In_ INT x,
    _In_ INT y,
    _In_ INT cx,
    _In_ INT cy,
    _In_ DWORD dwRop)
{
    BOOL bResult;
    PDC pdc;

    /* Mask away everything except foreground rop index */
    dwRop = dwRop & 0x00FF0000;
    dwRop |= dwRop << 8;

    /* Check if the rop uses a source */
    if (ROP_USES_SOURCE(dwRop))
    {
        /* This is not possible */
        return 0;
    }

    /* Lock the DC */
    pdc = DC_LockDc(hdcDest);
    if (pdc == NULL)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    /* Check if the DC has no surface (empty mem or info DC) */
    if (pdc->dclevel.pSurface == NULL)
    {
        /* Nothing to do, Windows returns TRUE! */
        DC_UnlockDc(pdc);
        return TRUE;
    }

    /* Update the fill brush, if neccessary */
    if (pdc->pdcattr->ulDirty_ & (DIRTY_FILL | DC_BRUSH_DIRTY))
        DC_vUpdateFillBrush(pdc);

    /* Call the internal function */
    bResult = IntPatBlt(pdc, x, y, cx, cy, dwRop, &pdc->eboFill);

    /* Unlock the DC and return the result */
    DC_UnlockDc(pdc);
    return bResult;
}

BOOL
APIENTRY
NtGdiPolyPatBlt(
    HDC hDC,
    DWORD dwRop,
    IN PPOLYPATBLT pRects,
    IN DWORD cRects,
    IN DWORD Mode)
{
    PPATRECT rb = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOL Ret;

    if (cRects > 0)
    {
        rb = ExAllocatePoolWithTag(PagedPool, sizeof(PATRECT) * cRects, GDITAG_PLGBLT_DATA);
        if (!rb)
        {
            EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
        _SEH2_TRY
        {
            ProbeForRead(pRects,
                cRects * sizeof(PATRECT),
                1);
            RtlCopyMemory(rb,
                pRects,
                cRects * sizeof(PATRECT));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        if (!NT_SUCCESS(Status))
        {
            ExFreePoolWithTag(rb, GDITAG_PLGBLT_DATA);
            SetLastNtError(Status);
            return FALSE;
        }
    }

    Ret = IntGdiPolyPatBlt(hDC, dwRop, rb, cRects, Mode);

    if (cRects > 0)
        ExFreePoolWithTag(rb, GDITAG_PLGBLT_DATA);

    return Ret;
}

BOOL
APIENTRY
NtGdiInvertRgn(
    HDC hDC,
    HRGN hRgn)
{
    PREGION RgnData;
    ULONG i;
    PRECTL rc;

    RgnData = RGNOBJAPI_Lock(hRgn, NULL);
    if (RgnData == NULL)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    rc = RgnData->Buffer;
    for (i = 0; i < RgnData->rdh.nCount; i++)
    {

        if (!NtGdiPatBlt(hDC, rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top, DSTINVERT))
        {
            RGNOBJAPI_Unlock(RgnData);
            return FALSE;
        }
        rc++;
    }

    RGNOBJAPI_Unlock(RgnData);
    return TRUE;
}

COLORREF
APIENTRY
NtGdiSetPixel(
    _In_ HDC hdc,
    _In_ INT x,
    _In_ INT y,
    _In_ COLORREF crColor)
{
    PDC pdc;
    ULONG iOldColor, iSolidColor;
    BOOL bResult;
    PEBRUSHOBJ pebo;
    ULONG ulDirty;
    EXLATEOBJ exlo;

    /* Lock the DC */
    pdc = DC_LockDc(hdc);
    if (!pdc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return -1;
    }

    /* Check if the DC has no surface (empty mem or info DC) */
    if (pdc->dclevel.pSurface == NULL)
    {
        /* Fail! */
        DC_UnlockDc(pdc);
        return -1;
    }

    /* Translate the color to the target format */
    iSolidColor = TranslateCOLORREF(pdc, crColor);

    /* Use the DC's text brush, which is always a solid brush */
    pebo = &pdc->eboText;

    /* Save the old solid color and set the one for the pixel */
    iOldColor = EBRUSHOBJ_iSetSolidColor(pebo, iSolidColor);

    /* Save dirty flags and reset dirty text brush flag */
    ulDirty = pdc->pdcattr->ulDirty_;
    pdc->pdcattr->ulDirty_ &= ~DIRTY_TEXT;

    /* Call the internal function */
    bResult = IntPatBlt(pdc, x, y, 1, 1, PATCOPY, pebo);

    /* Restore old text brush color and dirty flags */
    EBRUSHOBJ_iSetSolidColor(pebo, iOldColor);
    pdc->pdcattr->ulDirty_ = ulDirty;

    /* Initialize an XLATEOBJ from the target surface to RGB */
    EXLATEOBJ_vInitialize(&exlo,
                          pdc->dclevel.pSurface->ppal,
                          &gpalRGB,
                          0,
                          pdc->pdcattr->crBackgroundClr,
                          pdc->pdcattr->crForegroundClr);

    /* Translate the color back to RGB */
    crColor = XLATEOBJ_iXlate(&exlo.xlo, iSolidColor);

    /* Cleanup and return the target format color */
    EXLATEOBJ_vCleanup(&exlo);

    /* Unlock the DC */
    DC_UnlockDc(pdc);

    /* Return the new RGB color or -1 on failure */
    return bResult ? crColor : -1;
}

COLORREF
APIENTRY
NtGdiGetPixel(
    _In_ HDC hdc,
    _In_ INT x,
    _In_ INT y)
{
    PDC pdc;
    ULONG ulRGBColor = CLR_INVALID;
    POINTL ptlSrc;
    PSURFACE psurfSrc, psurfDest;

    /* Lock the DC */
    pdc = DC_LockDc(hdc);
    if (!pdc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return CLR_INVALID;
    }

    /* Check if the DC has no surface (empty mem or info DC) */
    psurfSrc = pdc->dclevel.pSurface;
    if (psurfSrc == NULL)
    {
        /* Fail! */
        goto leave;
    }

    /* Get the logical coordinates */
    ptlSrc.x = x;
    ptlSrc.y = y;

    /* Translate coordinates to device coordinates */
    IntLPtoDP(pdc, &ptlSrc, 1);
    ptlSrc.x += pdc->ptlDCOrig.x;
    ptlSrc.y += pdc->ptlDCOrig.y;

    /* Check if the pixel is outside the surface */
    if ((ptlSrc.x >= psurfSrc->SurfObj.sizlBitmap.cx) ||
        (ptlSrc.y >= psurfSrc->SurfObj.sizlBitmap.cy))
    {
        /* Fail! */
        goto leave;
    }

    /* Allocate a surface */
    psurfDest = SURFACE_AllocSurface(STYPE_BITMAP,
                                     1,
                                     1,
                                     BMF_32BPP,
                                     0,
                                     0,
                                     &ulRGBColor);
    if (psurfDest)
    {
        RECTL rclDest = {0, 0, 1, 1};
        EXLATEOBJ exlo;

        /* Translate from the source palette to RGB color */
        EXLATEOBJ_vInitialize(&exlo,
                              psurfSrc->ppal,
                              &gpalRGB,
                              0,
                              RGB(0xff,0xff,0xff),
                              RGB(0,0,0));

        /* Call the copy bits function */
        EngCopyBits(&psurfDest->SurfObj,
                    &psurfSrc->SurfObj,
                    NULL,
                    &exlo.xlo,
                    &rclDest,
                    &ptlSrc);

        /* Cleanup the XLATEOBJ */
        EXLATEOBJ_vCleanup(&exlo);

        /* Delete the surface */
        GDIOBJ_vDeleteObject(&psurfDest->BaseObject);
    }

leave:
    /* Unlock the DC */
    DC_UnlockDc(pdc);

    /* Return the new RGB color or -1 on failure */
    return ulRGBColor;
}

