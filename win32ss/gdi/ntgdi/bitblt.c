/*
 * COPYRIGHT:        GNU GPL, See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Bit blit functions
 * FILE:             win32ss/gdi/ntgdi/bitblt.c
 * PROGRAMER:        Unknown
 */

#include <win32k.h>
#define NDEBUG
#include <debug.h>
DBG_DEFAULT_CHANNEL(GdiBlt);

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

    if (DCDest->fs & (DC_ACCUM_APP|DC_ACCUM_WMGR))
    {
       IntUpdateBoundsRect(DCDest, &DestRect);
    }

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
                               (CLIPOBJ *)&DCDest->co,
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
    DWORD dwRop,
    IN DWORD crBackColor,
    IN FLONG fl)
{

    if (dwRop & CAPTUREBLT)
    {
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
                              dwRop,
                              crBackColor);
    }

    dwRop = dwRop & ~(NOMIRRORBITMAP|CAPTUREBLT);

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
                        MAKEROP4(dwRop, dwRop),
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

    if (DCDest->fs & (DC_ACCUM_APP|DC_ACCUM_WMGR))
    {
       IntUpdateBoundsRect(DCDest, &rcDest);
    }

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
        (CLIPOBJ *)&DCDest->co, &exlo.xlo, &rcDest, &rcSrc,
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
    DWORD dwRop4,
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
    BOOL UsesSource;
    ROP4 rop4;

    rop4 = WIN32_ROP4_TO_ENG_ROP4(dwRop4);

    UsesSource = ROP4_USES_SOURCE(rop4);
    if (!hdcDest || (UsesSource && !hdcSrc))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Check if we need a mask and have a mask bitmap */
    if (ROP4_USES_MASK(rop4) && (hbmMask != NULL))
    {
        /* Reference the mask bitmap */
        psurfMask = SURFACE_ShareLockSurface(hbmMask);
        if (psurfMask == NULL)
        {
            EngSetLastError(ERROR_INVALID_HANDLE);
            return FALSE;
        }

        /* Make sure the mask bitmap is 1 BPP */
        if (gajBitsPerFormat[psurfMask->SurfObj.iBitmapFormat] != 1)
        {
            EngSetLastError(ERROR_INVALID_PARAMETER);
            SURFACE_ShareUnlockSurface(psurfMask);
            return FALSE;
        }
    }
    else
    {
        /* We use NULL, if we need a mask, the Eng function will take care of
           that and use the brushobject to get a mask */
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
        WARN("Invalid dc handle (dest=0x%p, src=0x%p) passed to NtGdiMaskBlt\n", hdcDest, hdcSrc);
        if(psurfMask) SURFACE_ShareUnlockSurface(psurfMask);
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    DCDest = apObj[0];
    DCSrc = apObj[1];

    ASSERT(DCDest);
    if (NULL == DCDest)
    {
        if(DCSrc) DC_UnlockDc(DCSrc);
        WARN("Invalid destination dc handle (0x%p) passed to NtGdiMaskBlt\n", hdcDest);
        if(psurfMask) SURFACE_ShareUnlockSurface(psurfMask);
        return FALSE;
    }

    if (DCDest->dctype == DC_TYPE_INFO)
    {
        if(DCSrc) DC_UnlockDc(DCSrc);
        DC_UnlockDc(DCDest);
        /* Yes, Windows really returns TRUE in this case */
        if(psurfMask) SURFACE_ShareUnlockSurface(psurfMask);
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
            if(psurfMask) SURFACE_ShareUnlockSurface(psurfMask);
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

    if (DCDest->fs & (DC_ACCUM_APP|DC_ACCUM_WMGR))
    {
       IntUpdateBoundsRect(DCDest, &DestRect);
    }

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
        BitmapSrc = DCSrc->dclevel.pSurface;
        if (!BitmapSrc)
            goto cleanup;

        /* Create the XLATEOBJ. */
        EXLATEOBJ_vInitXlateFromDCs(&exlo, DCSrc, DCDest);
        XlateObj = &exlo.xlo;
    }

    DPRINT("DestRect: (%d,%d)-(%d,%d) and SourcePoint is (%d,%d)\n",
        DestRect.left, DestRect.top, DestRect.right, DestRect.bottom,
        SourcePoint.x, SourcePoint.y);

    DPRINT("nWidth is '%d' and nHeight is '%d'.\n", nWidth, nHeight);

    /* Fix BitBlt so that it will not flip left to right */
    if ((DestRect.left > DestRect.right) && (nWidth < 0))
    {
        SourcePoint.x += nWidth;
        nWidth = -nWidth;
    }

    /* Fix BitBlt so that it will not flip top to bottom */
    if ((DestRect.top > DestRect.bottom) && (nHeight < 0))
    {
        SourcePoint.y += nHeight;
        nHeight = -nHeight;
    }

    /* Make Well Ordered so that we don't flip either way */
    RECTL_vMakeWellOrdered(&DestRect);

    /* Perform the bitblt operation */
    Status = IntEngBitBlt(&BitmapDest->SurfObj,
                          BitmapSrc ? &BitmapSrc->SurfObj : NULL,
                          psurfMask ? &psurfMask->SurfObj : NULL,
                          (CLIPOBJ *)&DCDest->co,
                          XlateObj,
                          &DestRect,
                          &SourcePoint,
                          &MaskPoint,
                          &DCDest->eboFill.BrushObject,
                          &DCDest->dclevel.pbrFill->ptOrigin,
                          rop4);

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

BOOL
NTAPI
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
    DWORD dwRop4,
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
    ROP4 rop4;
    BOOL Case0000, Case0101, Case1010, CaseExcept;

    rop4 = WIN32_ROP4_TO_ENG_ROP4(dwRop4);

    UsesSource = ROP4_USES_SOURCE(rop4);
    UsesMask = ROP4_USES_MASK(rop4);

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


    Case0000 = ((WidthDest < 0) && (HeightDest < 0) && (WidthSrc < 0) && (HeightSrc < 0));
    Case0101 = ((WidthDest < 0) && (HeightDest > 0) && (WidthSrc < 0) && (HeightSrc > 0));
    Case1010 = ((WidthDest > 0) && (HeightDest < 0) && (WidthSrc > 0) && (HeightSrc < 0));
    CaseExcept = (Case0000 || Case0101 || Case1010);

    pdcattr = DCDest->pdcattr;

    DestRect.left   = XOriginDest;
    DestRect.top    = YOriginDest;
    DestRect.right  = XOriginDest+WidthDest;
    DestRect.bottom = YOriginDest+HeightDest;

    /* Account for possible negative span values */
    if ((WidthDest < 0) && !CaseExcept)
    {
        DestRect.left++;
        DestRect.right++;
    }
    if ((HeightDest < 0) && !CaseExcept)
    {
        DestRect.top++;
        DestRect.bottom++;
    }

    IntLPtoDP(DCDest, (LPPOINT)&DestRect, 2);

    DestRect.left   += DCDest->ptlDCOrig.x;
    DestRect.top    += DCDest->ptlDCOrig.y;
    DestRect.right  += DCDest->ptlDCOrig.x;
    DestRect.bottom += DCDest->ptlDCOrig.y;

    if (DCDest->fs & (DC_ACCUM_APP|DC_ACCUM_WMGR))
    {
       IntUpdateBoundsRect(DCDest, &DestRect);
    }

    SourceRect.left   = XOriginSrc;
    SourceRect.top    = YOriginSrc;
    SourceRect.right  = XOriginSrc+WidthSrc;
    SourceRect.bottom = YOriginSrc+HeightSrc;

    /* Account for possible negative span values */
    if ((WidthSrc < 0) && !CaseExcept)
    {
        SourceRect.left++;
        SourceRect.right++;
    }
    if ((HeightSrc < 0) && !CaseExcept)
    {
        SourceRect.top++;
        SourceRect.bottom++;
    }

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

    DPRINT("Calling IntEngStrethBlt SourceRect: (%d,%d)-(%d,%d) and DestRect: (%d,%d)-(%d,%d).\n",
           SourceRect.left, SourceRect.top, SourceRect.right, SourceRect.bottom,
           DestRect.left, DestRect.top, DestRect.right, DestRect.bottom);

    /* Perform the bitblt operation */
    Status = IntEngStretchBlt(&BitmapDest->SurfObj,
                              BitmapSrc ? &BitmapSrc->SurfObj : NULL,
                              BitmapMask ? &BitmapMask->SurfObj : NULL,
                              (CLIPOBJ *)&DCDest->co,
                              XlateObj,
                              &DCDest->dclevel.ca,
                              &DestRect,
                              &SourceRect,
                              BitmapMask ? &MaskPoint : NULL,
                              &DCDest->eboFill.BrushObject,
                              &BrushOrigin,
                              rop4);
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
    DWORD dwRop3,
    IN DWORD dwBackColor)
{
    dwRop3 = dwRop3 & ~(NOMIRRORBITMAP|CAPTUREBLT);

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
                MAKEROP4(dwRop3 & 0xFF0000, dwRop3),
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
    DWORD dwRop3,
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

    if (pdc->fs & (DC_ACCUM_APP|DC_ACCUM_WMGR))
    {
       IntUpdateBoundsRect(pdc, &DestRect);
    }

#ifdef _USE_DIBLIB_
    BrushOrigin.x = pbrush->ptOrigin.x + pdc->ptlDCOrig.x + XLeft;
    BrushOrigin.y = pbrush->ptOrigin.y + pdc->ptlDCOrig.y + YLeft;
#else
    BrushOrigin.x = pbrush->ptOrigin.x + pdc->ptlDCOrig.x;
    BrushOrigin.y = pbrush->ptOrigin.y + pdc->ptlDCOrig.y;
#endif

    DC_vPrepareDCsForBlit(pdc, &DestRect, NULL, NULL);

    psurf = pdc->dclevel.pSurface;

    ret = IntEngBitBlt(&psurf->SurfObj,
                       NULL,
                       NULL,
                       (CLIPOBJ *)&pdc->co,
                       NULL,
                       &DestRect,
                       NULL,
                       NULL,
                       &pebo->BrushObject,
                       &BrushOrigin,
                       WIN32_ROP3_TO_ENG_ROP4(dwRop3));

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

    /* Convert the ROP3 to a ROP4 */
    dwRop = MAKEROP4(dwRop & 0xFF0000, dwRop);

    /* Check if the rop uses a source */
    if (WIN32_ROP4_USES_SOURCE(dwRop))
    {
        /* This is not possible */
        return FALSE;
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

    /* Update the fill brush, if necessary */
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

static
BOOL
FASTCALL
REGION_LPTODP(
    _In_ PDC pdc,
    _Inout_ PREGION prgnDest,
    _In_ PREGION prgnSrc)
{
    if (IntGdiCombineRgn(prgnDest, prgnSrc, NULL, RGN_COPY) == ERROR)
        return FALSE;

    return REGION_bXformRgn(prgnDest, DC_pmxWorldToDevice(pdc));
}

BOOL
APIENTRY
IntGdiBitBltRgn(
    _In_ PDC pdc,
    _In_ PREGION prgn,
    _In_opt_ BRUSHOBJ *pbo,
    _In_opt_ POINTL *pptlBrush,
    _In_ ROP4 rop4)
{
    PREGION prgnClip;
    XCLIPOBJ xcoClip;
    BOOL bResult;
    NT_ASSERT((pdc != NULL) && (prgn != NULL));

    /* Check if we have a surface */
    if (pdc->dclevel.pSurface == NULL)
    {
        return TRUE;
    }

    /* Create an empty clip region */
    prgnClip = IntSysCreateRectpRgn(0, 0, 0, 0);
    if (prgnClip == NULL)
    {
        return FALSE;
    }

    /* Transform given region into device coordinates */
    if (!REGION_LPTODP(pdc, prgnClip, prgn))
    {
        REGION_Delete(prgnClip);
        return FALSE;
    }

    /* Intersect with the system or RAO region (these are (atm) without DC-origin) */
    if (pdc->prgnRao)
        IntGdiCombineRgn(prgnClip, prgnClip, pdc->prgnRao, RGN_AND);
    else
        IntGdiCombineRgn(prgnClip, prgnClip, pdc->prgnVis, RGN_AND);

    /* Now account for the DC-origin */
    if (!REGION_bOffsetRgn(prgnClip, pdc->ptlDCOrig.x, pdc->ptlDCOrig.y))
    {
        REGION_Delete(prgnClip);
        return FALSE;
    }

    if (pdc->fs & (DC_ACCUM_APP|DC_ACCUM_WMGR))
    {
        RECTL rcrgn;
        REGION_GetRgnBox(prgnClip, &rcrgn);
        IntUpdateBoundsRect(pdc, &rcrgn);
    }

    /* Prepare the DC */
    DC_vPrepareDCsForBlit(pdc, &prgnClip->rdh.rcBound, NULL, NULL);

    /* Initialize a clip object */
    IntEngInitClipObj(&xcoClip);
    IntEngUpdateClipRegion(&xcoClip,
                           prgnClip->rdh.nCount,
                           prgnClip->Buffer,
                           &prgnClip->rdh.rcBound);

    /* Call the Eng or Drv function */
    bResult = IntEngBitBlt(&pdc->dclevel.pSurface->SurfObj,
                           NULL,
                           NULL,
                           (CLIPOBJ *)&xcoClip,
                           NULL,
                           &prgnClip->rdh.rcBound,
                           NULL,
                           NULL,
                           pbo,
                           pptlBrush,
                           rop4);

    /* Cleanup */
    DC_vFinishBlit(pdc, NULL);
    REGION_Delete(prgnClip);
    IntEngFreeClipResources(&xcoClip);

    /* Return the result */
    return bResult;
}

BOOL
IntGdiFillRgn(
    _In_ PDC pdc,
    _In_ PREGION prgn,
    _In_opt_ PBRUSH pbrFill)
{
    PREGION prgnClip;
    XCLIPOBJ xcoClip;
    EBRUSHOBJ eboFill;
    BRUSHOBJ *pbo;
    BOOL bRet;
    DWORD rop2Fg;
    MIX mix;
    NT_ASSERT((pdc != NULL) && (prgn != NULL));

    if (pdc->dclevel.pSurface == NULL)
    {
        return TRUE;
    }

    prgnClip = IntSysCreateRectpRgn(0, 0, 0, 0);
    if (prgnClip == NULL)
    {
        return FALSE;
    }

    /* Transform region into device coordinates */
    if (!REGION_LPTODP(pdc, prgnClip, prgn))
    {
        REGION_Delete(prgnClip);
        return FALSE;
    }

    /* Intersect with the system or RAO region (these are (atm) without DC-origin) */
    if (pdc->prgnRao)
        IntGdiCombineRgn(prgnClip, prgnClip, pdc->prgnRao, RGN_AND);
    else
        IntGdiCombineRgn(prgnClip, prgnClip, pdc->prgnVis, RGN_AND);

    /* Now account for the DC-origin */
    if (!REGION_bOffsetRgn(prgnClip, pdc->ptlDCOrig.x, pdc->ptlDCOrig.y))
    {
        REGION_Delete(prgnClip);
        return FALSE;
    }

    if (pdc->fs & (DC_ACCUM_APP|DC_ACCUM_WMGR))
    {
        RECTL rcrgn;
        REGION_GetRgnBox(prgnClip, &rcrgn);
        IntUpdateBoundsRect(pdc, &rcrgn);
    }

    IntEngInitClipObj(&xcoClip);
    IntEngUpdateClipRegion(&xcoClip,
                           prgnClip->rdh.nCount,
                           prgnClip->Buffer,
                           &prgnClip->rdh.rcBound );

    /* Get the FG rop and create a MIX based on the BK mode */
    rop2Fg = FIXUP_ROP2(pdc->pdcattr->jROP2);
    mix = rop2Fg | (pdc->pdcattr->jBkMode == OPAQUE ? rop2Fg : R2_NOP) << 8;

    /* Prepare DC for blit */
    DC_vPrepareDCsForBlit(pdc, &prgnClip->rdh.rcBound, NULL, NULL);

    /* Check if we have a fill brush */
    if (pbrFill != NULL)
    {
        /* Initialize the brush object */
        /// \todo Check parameters
        EBRUSHOBJ_vInit(&eboFill, pbrFill, pdc->dclevel.pSurface, 0x00FFFFFF, 0, NULL);
        pbo = &eboFill.BrushObject;
    }
    else
    {
        /* Update the fill brush if needed */
        if (pdc->pdcattr->ulDirty_ & (DIRTY_FILL | DC_BRUSH_DIRTY))
            DC_vUpdateFillBrush(pdc);

        /* Use the DC brush object */
        pbo = &pdc->eboFill.BrushObject;
    }

    /* Call the internal function */
    bRet = IntEngPaint(&pdc->dclevel.pSurface->SurfObj,
                       (CLIPOBJ *)&xcoClip,
                       pbo,
                       &pdc->pdcattr->ptlBrushOrigin,
                       mix);

    DC_vFinishBlit(pdc, NULL);
    REGION_Delete(prgnClip);
    IntEngFreeClipResources(&xcoClip);

    // Fill the region
    return bRet;
}

BOOL
FASTCALL
IntGdiPaintRgn(
    _In_ PDC pdc,
    _In_ PREGION prgn)
{
    return IntGdiFillRgn(pdc, prgn, NULL);
}

BOOL
APIENTRY
NtGdiFillRgn(
    _In_ HDC hdc,
    _In_ HRGN hrgn,
    _In_ HBRUSH hbrush)
{
    PDC pdc;
    PREGION prgn;
    PBRUSH pbrFill;
    BOOL bResult;

    /* Lock the DC */
    pdc = DC_LockDc(hdc);
    if (pdc == NULL)
    {
        ERR("Failed to lock hdc %p\n", hdc);
        return FALSE;
    }

    /* Check if the DC has no surface (empty mem or info DC) */
    if (pdc->dclevel.pSurface == NULL)
    {
        DC_UnlockDc(pdc);
        return TRUE;
    }

    /* Lock the region */
    prgn = REGION_LockRgn(hrgn);
    if (prgn == NULL)
    {
        ERR("Failed to lock hrgn %p\n", hrgn);
        DC_UnlockDc(pdc);
        return FALSE;
    }

    /* Lock the brush */
    pbrFill = BRUSH_ShareLockBrush(hbrush);
    if (pbrFill == NULL)
    {
        ERR("Failed to lock hbrush %p\n", hbrush);
        REGION_UnlockRgn(prgn);
        DC_UnlockDc(pdc);
        return FALSE;
    }

    /* Call the internal function */
    bResult = IntGdiFillRgn(pdc, prgn, pbrFill);

    /* Cleanup locks */
    BRUSH_ShareUnlockBrush(pbrFill);
    REGION_UnlockRgn(prgn);
    DC_UnlockDc(pdc);

    return bResult;
}

BOOL
APIENTRY
NtGdiFrameRgn(
    _In_ HDC hdc,
    _In_ HRGN hrgn,
    _In_ HBRUSH hbrush,
    _In_ INT xWidth,
    _In_ INT yHeight)
{
    HRGN hrgnFrame;
    BOOL bResult;

    hrgnFrame = GreCreateFrameRgn(hrgn, xWidth, yHeight);
    if (hrgnFrame == NULL)
    {
        return FALSE;
    }

    bResult = NtGdiFillRgn(hdc, hrgnFrame, hbrush);

    GreDeleteObject(hrgnFrame);
    return bResult;
}

BOOL
APIENTRY
NtGdiInvertRgn(
    _In_ HDC hdc,
    _In_ HRGN hrgn)
{
    BOOL bResult;
    PDC pdc;
    PREGION prgn;

    /* Lock the DC */
    pdc = DC_LockDc(hdc);
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

    /* Lock the region */
    prgn = REGION_LockRgn(hrgn);
    if (prgn == NULL)
    {
        DC_UnlockDc(pdc);
        return FALSE;
    }

    /* Call the internal function */
    bResult = IntGdiBitBltRgn(pdc,
                              prgn,
                              NULL, // pbo
                              NULL, // pptlBrush,
                              ROP4_DSTINVERT);

    /* Unlock the region and DC and return the result */
    REGION_UnlockRgn(prgn);
    DC_UnlockDc(pdc);
    return bResult;
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

    if (pdc->fs & (DC_ACCUM_APP|DC_ACCUM_WMGR))
    {
       RECTL rcDst;

       RECTL_vSetRect(&rcDst, x, y, x+1, y+1);

       IntLPtoDP(pdc, (LPPOINT)&rcDst, 2);

       rcDst.left   += pdc->ptlDCOrig.x;
       rcDst.top    += pdc->ptlDCOrig.y;
       rcDst.right  += pdc->ptlDCOrig.x;
       rcDst.bottom += pdc->ptlDCOrig.y;

       IntUpdateBoundsRect(pdc, &rcDst);
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

    /// FIXME: we shouldn't dereference pSurface while the PDEV is not locked!
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
    RECT rcDest;
    PSURFACE psurfSrc, psurfDest;

    /* Lock the DC */
    pdc = DC_LockDc(hdc);
    if (!pdc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return CLR_INVALID;
    }

    /* Check if the DC has no surface (empty mem or info DC) */
    if (pdc->dclevel.pSurface == NULL)
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

    rcDest.left = x;
    rcDest.top = y;
    rcDest.right = x + 1;
    rcDest.bottom = y + 1;

    /* Prepare DC for blit */
    DC_vPrepareDCsForBlit(pdc, &rcDest, NULL, NULL);

    /* Check if the pixel is outside the surface */
    psurfSrc = pdc->dclevel.pSurface;
    if ((ptlSrc.x >= psurfSrc->SurfObj.sizlBitmap.cx) ||
        (ptlSrc.y >= psurfSrc->SurfObj.sizlBitmap.cy) ||
        (ptlSrc.x < 0) ||
        (ptlSrc.y < 0))
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
    DC_vFinishBlit(pdc, NULL);
    DC_UnlockDc(pdc);

    /* Return the new RGB color or -1 on failure */
    return ulRGBColor;
}

