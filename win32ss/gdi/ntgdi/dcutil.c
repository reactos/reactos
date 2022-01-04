#include <win32k.h>

#define NDEBUG
#include <debug.h>

BOOL FASTCALL
GreDPtoLP(HDC hdc, LPPOINT lpPoints, INT nCount)
{
   PDC dc;
   if (!(dc = DC_LockDc(hdc)))
   {
      EngSetLastError(ERROR_INVALID_HANDLE);
      return FALSE;
   }
   IntDPtoLP(dc, lpPoints, nCount);
   DC_UnlockDc(dc);
   return TRUE;
}

BOOL FASTCALL
GreLPtoDP(HDC hdc, LPPOINT lpPoints, INT nCount)
{
   PDC dc;
   if (!(dc = DC_LockDc(hdc)))
   {
      EngSetLastError(ERROR_INVALID_HANDLE);
      return FALSE;
   }
   IntLPtoDP(dc, lpPoints, nCount);
   DC_UnlockDc(dc);
   return TRUE;
}

int FASTCALL
GreGetBkMode(HDC hdc)
{
   PDC dc;
   LONG lBkMode;
   if (!(dc = DC_LockDc(hdc)))
   {
      EngSetLastError(ERROR_INVALID_HANDLE);
      return CLR_INVALID;
   }
   lBkMode = dc->pdcattr->lBkMode;
   DC_UnlockDc(dc);
   return lBkMode;
}

COLORREF FASTCALL
GreGetBkColor(HDC hdc)
{
   PDC dc;
   COLORREF crBk;
   if (!(dc = DC_LockDc(hdc)))
   {
      EngSetLastError(ERROR_INVALID_HANDLE);
      return CLR_INVALID;
   }
   crBk = dc->pdcattr->ulBackgroundClr;
   DC_UnlockDc(dc);
   return crBk;
}

int FASTCALL
GreGetMapMode(HDC hdc)
{
   PDC dc;
   INT iMapMode;
   if (!(dc = DC_LockDc(hdc)))
   {
      EngSetLastError(ERROR_INVALID_HANDLE);
      return CLR_INVALID;
   }
   iMapMode = dc->pdcattr->iMapMode;
   DC_UnlockDc(dc);
   return iMapMode;
}

COLORREF FASTCALL
GreGetTextColor(HDC hdc)
{
   PDC dc;
   ULONG ulForegroundClr;
   if (!(dc = DC_LockDc(hdc)))
   {
      EngSetLastError(ERROR_INVALID_HANDLE);
      return CLR_INVALID;
   }
   ulForegroundClr = dc->pdcattr->ulForegroundClr;
   DC_UnlockDc(dc);
   return ulForegroundClr;
}

COLORREF FASTCALL
IntGdiSetBkColor(HDC hDC, COLORREF color)
{
    COLORREF oldColor;
    PDC dc;
    PDC_ATTR pdcattr;
    HBRUSH hBrush;

    if (!(dc = DC_LockDc(hDC)))
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return CLR_INVALID;
    }
    pdcattr = dc->pdcattr;

    oldColor = pdcattr->ulBackgroundClr;
    pdcattr->ulBackgroundClr = color;

    if (pdcattr->crBackgroundClr != color)
    {
        pdcattr->ulDirty_ |= (DIRTY_BACKGROUND|DIRTY_LINE|DIRTY_FILL); // Clear Flag if set.
        pdcattr->crBackgroundClr = color;
    }
    hBrush = pdcattr->hbrush;
    DC_UnlockDc(dc);
    NtGdiSelectBrush(hDC, hBrush);
    return oldColor;
}

INT FASTCALL
IntGdiSetBkMode(HDC hDC, INT Mode)
{
    COLORREF oldMode;
    PDC dc;
    PDC_ATTR pdcattr;

    if (!(dc = DC_LockDc(hDC)))
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return CLR_INVALID;
    }
    pdcattr = dc->pdcattr;
    oldMode = pdcattr->lBkMode;
    pdcattr->jBkMode = Mode;
    pdcattr->lBkMode = Mode;
    DC_UnlockDc(dc);
    return oldMode;
}

UINT
FASTCALL
IntGdiSetTextAlign(HDC  hDC,
                   UINT  Mode)
{
    UINT prevAlign;
    DC *dc;
    PDC_ATTR pdcattr;

    dc = DC_LockDc(hDC);
    if (!dc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return GDI_ERROR;
    }
    pdcattr = dc->pdcattr;
    prevAlign = pdcattr->lTextAlign;
    pdcattr->lTextAlign = Mode;
    if (pdcattr->dwLayout & LAYOUT_RTL)
    {
        if ((Mode & TA_CENTER) != TA_CENTER) Mode ^= TA_RIGHT;
    }
    pdcattr->flTextAlign = Mode & TA_MASK;
    DC_UnlockDc(dc);
    return  prevAlign;
}

COLORREF
FASTCALL
IntGdiSetTextColor(HDC hDC,
                   COLORREF color)
{
    COLORREF crOldColor;
    PDC pdc;
    PDC_ATTR pdcattr;

    pdc = DC_LockDc(hDC);
    if (!pdc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return CLR_INVALID;
    }
    pdcattr = pdc->pdcattr;

    crOldColor = (COLORREF) pdcattr->ulForegroundClr;
    pdcattr->ulForegroundClr = (ULONG)color;

    if (pdcattr->crForegroundClr != color)
    {
        pdcattr->ulDirty_ |= (DIRTY_TEXT|DIRTY_LINE|DIRTY_FILL);
        pdcattr->crForegroundClr = color;
    }

    DC_vUpdateTextBrush(pdc);
    DC_vUpdateLineBrush(pdc);
    DC_vUpdateFillBrush(pdc);

    DC_UnlockDc(pdc);

    return  crOldColor;
}

COLORREF FASTCALL
IntSetDCBrushColor(HDC hdc, COLORREF crColor)
{
   COLORREF OldColor = CLR_INVALID;
   PDC dc;
   if (!(dc = DC_LockDc(hdc)))
   {
      EngSetLastError(ERROR_INVALID_HANDLE);
      return CLR_INVALID;
   }
   else
   {
      OldColor = (COLORREF) dc->pdcattr->ulBrushClr;
      dc->pdcattr->ulBrushClr = (ULONG) crColor;

      if ( dc->pdcattr->crBrushClr != crColor )
      {
         dc->pdcattr->ulDirty_ |= DIRTY_FILL;
         dc->pdcattr->crBrushClr = crColor;
      }
   }
   DC_UnlockDc(dc);
   return OldColor;
}

BOOL FASTCALL
GreSetBrushOrg(
    HDC hdc,
    INT x,
    INT y,
    LPPOINT pptOut)
{
    PDC pdc = DC_LockDc(hdc);
    if (pdc == NULL)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pptOut != NULL)
    {
        *pptOut = pdc->pdcattr->ptlBrushOrigin;
    }

    pdc->pdcattr->ptlBrushOrigin.x = x;
    pdc->pdcattr->ptlBrushOrigin.y = y;

    DC_vSetBrushOrigin(pdc, x, y);

    DC_UnlockDc(pdc);
    return TRUE;
}

COLORREF FASTCALL
IntSetDCPenColor(HDC hdc, COLORREF crColor)
{
   COLORREF OldColor;
   PDC dc;
   if (!(dc = DC_LockDc(hdc)))
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
      return CLR_INVALID;
   }

   OldColor = (COLORREF)dc->pdcattr->ulPenClr;
   dc->pdcattr->ulPenClr = (ULONG)crColor;

   if (dc->pdcattr->crPenClr != crColor)
   {
      dc->pdcattr->ulDirty_ |= DIRTY_LINE;
      dc->pdcattr->crPenClr = crColor;
   }
   DC_UnlockDc(dc);
   return OldColor;
}

int
FASTCALL
GreSetStretchBltMode(HDC hDC, int iStretchMode)
{
    PDC pdc;
    PDC_ATTR pdcattr;
    INT oSMode = 0;

    pdc = DC_LockDc(hDC);
    if (pdc)
    {
       pdcattr = pdc->pdcattr;
       oSMode = pdcattr->lStretchBltMode;
       pdcattr->lStretchBltMode = iStretchMode;

       // Wine returns an error here. We set the default.
       if ((iStretchMode <= 0) || (iStretchMode > MAXSTRETCHBLTMODE)) iStretchMode = WHITEONBLACK;

       pdcattr->jStretchBltMode = iStretchMode;
       DC_UnlockDc(pdc);
    }
    return oSMode;
}

int FASTCALL
GreGetGraphicsMode(HDC hdc)
{
   PDC dc;
   int GraphicsMode;
   if (!(dc = DC_LockDc(hdc)))
   {
      EngSetLastError(ERROR_INVALID_HANDLE);
      return CLR_INVALID;
   }
   GraphicsMode = dc->pdcattr->iGraphicsMode;
   DC_UnlockDc(dc);
   return GraphicsMode;
}

VOID
FASTCALL
DCU_SetDcUndeletable(HDC  hDC)
{
    PDC dc = DC_LockDc(hDC);
    if (!dc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return;
    }

    dc->fs |= DC_FLAG_PERMANENT;
    DC_UnlockDc(dc);
    return;
}

#if 0
BOOL FASTCALL
IntIsPrimarySurface(SURFOBJ *SurfObj)
{
    if (PrimarySurface.pSurface == NULL)
    {
        return FALSE;
    }
    return SurfObj->hsurf == PrimarySurface.pSurface; // <- FIXME: WTF?
}
#endif

BOOL
FASTCALL
IntSetDefaultRegion(PDC pdc)
{
    PSURFACE pSurface;
    PREGION prgn;
    RECTL rclWnd, rclClip;

    IntGdiReleaseRaoRgn(pdc);

    rclWnd.left   = 0;
    rclWnd.top    = 0;
    rclWnd.right  = pdc->dclevel.sizl.cx;
    rclWnd.bottom = pdc->dclevel.sizl.cy;
    rclClip = rclWnd;

    //EngAcquireSemaphoreShared(pdc->ppdev->hsemDevLock);
    if (pdc->ppdev->flFlags & PDEV_META_DEVICE)
    {
        pSurface = pdc->dclevel.pSurface;
        if (pSurface && pSurface->flags & PDEV_SURFACE)
        {
            rclClip.left   += pdc->ppdev->ptlOrigion.x;
            rclClip.top    += pdc->ppdev->ptlOrigion.y;
            rclClip.right  += pdc->ppdev->ptlOrigion.x;
            rclClip.bottom += pdc->ppdev->ptlOrigion.y;
        }
    }
    //EngReleaseSemaphore(pdc->ppdev->hsemDevLock);

    prgn = pdc->prgnVis;

    if (prgn && prgn != prgnDefault)
    {
        REGION_SetRectRgn( prgn,
                           rclClip.left,
                           rclClip.top,
                           rclClip.right ,
                           rclClip.bottom );
    }
    else
    {
        prgn = IntSysCreateRectpRgn( rclClip.left,
                                     rclClip.top,
                                     rclClip.right ,
                                     rclClip.bottom );
        pdc->prgnVis = prgn;
    }

    if (prgn)
    {
        pdc->ptlDCOrig.x = 0;
        pdc->ptlDCOrig.y = 0;
        pdc->erclWindow = rclWnd;
        pdc->erclClip = rclClip;
        /* Might be an InitDC or DCE... */
        pdc->ptlFillOrigin = pdc->dcattr.ptlBrushOrigin;
        return TRUE;
    }
    // No Vis use the Default System Region.
    pdc->prgnVis = prgnDefault;
    return FALSE;
}


BOOL APIENTRY
NtGdiCancelDC(HDC  hDC)
{
    UNIMPLEMENTED;
    return FALSE;
}


WORD APIENTRY
IntGdiSetHookFlags(HDC hDC, WORD Flags)
{
    WORD wRet;
    DC *dc = DC_LockDc(hDC);

    if (NULL == dc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    wRet = dc->fs & DC_FLAG_DIRTY_RAO; // FIXME: Wrong flag!

    /* Info in "Undocumented Windows" is slightly confusing. */
    DPRINT("DC %p, Flags %04x\n", hDC, Flags);

    if (Flags & DCHF_INVALIDATEVISRGN)
    {
        /* hVisRgn has to be updated */
        dc->fs |= DC_FLAG_DIRTY_RAO;
    }
    else if (Flags & DCHF_VALIDATEVISRGN || 0 == Flags)
    {
        //dc->fs &= ~DC_FLAG_DIRTY_RAO;
    }

    DC_UnlockDc(dc);

    return wRet;
}


BOOL
APIENTRY
NtGdiGetDCDword(
    HDC hDC,
    UINT u,
    DWORD *Result)
{
    BOOL Ret = TRUE;
    PDC pdc;
    PDC_ATTR pdcattr;

    DWORD SafeResult = 0;
    NTSTATUS Status = STATUS_SUCCESS;

    if (!Result)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pdc = DC_LockDc(hDC);
    if (!pdc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    pdcattr = pdc->pdcattr;

    switch (u)
    {
        case GdiGetJournal:
            break;

        case GdiGetRelAbs:
            SafeResult = pdcattr->lRelAbs;
            break;

        case GdiGetBreakExtra:
            SafeResult = pdcattr->lBreakExtra;
            break;

        case GdiGetCharBreak:
            SafeResult = pdcattr->cBreak;
            break;

        case GdiGetArcDirection:
            if (pdcattr->dwLayout & LAYOUT_RTL)
                SafeResult = AD_CLOCKWISE - ((pdc->dclevel.flPath & DCPATH_CLOCKWISE) != 0);
            else
                SafeResult = ((pdc->dclevel.flPath & DCPATH_CLOCKWISE) != 0) + AD_COUNTERCLOCKWISE;
            break;

        case GdiGetEMFRestorDc:
            SafeResult = pdc->dclevel.lSaveDepth;
            break;

        case GdiGetFontLanguageInfo:
            SafeResult = IntGetFontLanguageInfo(pdc);
            break;

        case GdiGetIsMemDc:
            SafeResult = pdc->dctype;
            break;

        case GdiGetMapMode:
            SafeResult = pdcattr->iMapMode;
            break;

        case GdiGetTextCharExtra:
            SafeResult = pdcattr->lTextExtra;
            break;

        default:
            EngSetLastError(ERROR_INVALID_PARAMETER);
            Ret = FALSE;
            break;
    }

    if (Ret)
    {
        _SEH2_TRY
        {
            ProbeForWrite(Result, sizeof(DWORD), 1);
            *Result = SafeResult;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        if (!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            Ret = FALSE;
        }
    }

    DC_UnlockDc(pdc);
    return Ret;
}

_Success_(return != FALSE)
BOOL
APIENTRY
NtGdiGetAndSetDCDword(
    _In_ HDC hdc,
    _In_ UINT u,
    _In_ DWORD dwIn,
    _Out_ DWORD *pdwResult)
{
    BOOL Ret = TRUE;
    PDC pdc;
    PDC_ATTR pdcattr;

    DWORD SafeResult = 0;
    NTSTATUS Status = STATUS_SUCCESS;

    if (!pdwResult)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pdc = DC_LockDc(hdc);
    if (!pdc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    pdcattr = pdc->pdcattr;

    switch (u)
    {
        case GdiGetSetEPSPrintingEscape:
            SafeResult = pdc->fs & DC_EPSPRINTINGESCAPE;
            pdc->fs &= ~DC_EPSPRINTINGESCAPE;
            break;

        case GdiGetSetCopyCount:
            SafeResult = pdc->ulCopyCount;
            pdc->ulCopyCount = dwIn;
            break;

        case GdiGetSetTextAlign:
            SafeResult = pdcattr->lTextAlign;
            pdcattr->lTextAlign = dwIn;
            // pdcattr->flTextAlign = dwIn; // Flags!
            break;

        case GdiGetSetRelAbs:
            SafeResult = pdcattr->lRelAbs;
            pdcattr->lRelAbs = dwIn;
            break;

        case GdiGetSetTextCharExtra:
            SafeResult = pdcattr->lTextExtra;
            pdcattr->lTextExtra = dwIn;
            break;

        case GdiGetSetSelectFont:
            break;

        case GdiGetSetMapperFlagsInternal:
            if (dwIn & ~1)
            {
                EngSetLastError(ERROR_INVALID_PARAMETER);
                Ret = FALSE;
                break;
            }
            SafeResult = pdcattr->flFontMapper;
            pdcattr->flFontMapper = dwIn;
            break;

        case GdiGetSetMapMode:
            SafeResult = IntGdiSetMapMode(pdc, dwIn);
            break;

        case GdiGetSetArcDirection:
            if (dwIn != AD_COUNTERCLOCKWISE && dwIn != AD_CLOCKWISE)
            {
                EngSetLastError(ERROR_INVALID_PARAMETER);
                Ret = FALSE;
                break;
            }
            if (pdcattr->dwLayout & LAYOUT_RTL) // Right to Left
            {
                SafeResult = AD_CLOCKWISE - ((pdc->dclevel.flPath & DCPATH_CLOCKWISE) != 0);
                if (dwIn == AD_CLOCKWISE)
                {
                    pdc->dclevel.flPath &= ~DCPATH_CLOCKWISE;
                    break;
                }
                pdc->dclevel.flPath |= DCPATH_CLOCKWISE;
            }
            else // Left to Right
            {
                SafeResult = ((pdc->dclevel.flPath & DCPATH_CLOCKWISE) != 0) +
                             AD_COUNTERCLOCKWISE;
                if (dwIn == AD_COUNTERCLOCKWISE)
                {
                    pdc->dclevel.flPath &= ~DCPATH_CLOCKWISE;
                    break;
                }
                pdc->dclevel.flPath |= DCPATH_CLOCKWISE;
            }
            break;

        default:
            EngSetLastError(ERROR_INVALID_PARAMETER);
            Ret = FALSE;
            break;
    }

    if (Ret)
    {
        _SEH2_TRY
        {
            ProbeForWrite(pdwResult, sizeof(DWORD), 1);
            *pdwResult = SafeResult;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        if (!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            Ret = FALSE;
        }
    }

    DC_UnlockDc(pdc);
    return Ret;
}

VOID
FASTCALL
IntUpdateBoundsRect(PDC pdc, PRECTL pRect)
{
   if (pdc->fs & DC_ACCUM_APP)
   {
      RECTL_bUnionRect(&pdc->erclBoundsApp, &pdc->erclBoundsApp, pRect);
   }
   if (pdc->fs & DC_ACCUM_WMGR)
   {
      RECTL_bUnionRect(&pdc->erclBounds, &pdc->erclBounds, pRect);
   }
}

DWORD
APIENTRY
NtGdiGetBoundsRect(
    IN HDC hdc,
    OUT LPRECT prc,
    IN DWORD flags)
{
    DWORD ret;
    PDC pdc;
    RECT rc;

    /* Lock the DC */
    if (!(pdc = DC_LockDc(hdc))) return 0;

    if (!(flags & DCB_WINDOWMGR))
    {
       rc = pdc->erclBoundsApp;

       if (RECTL_bIsEmptyRect(&rc))
       {
          rc.left = rc.top = rc.right = rc.bottom = 0;
          ret = DCB_RESET;
       }
       else
       {
          RECTL rcRgn;
          if (pdc->fs & DC_FLAG_DIRTY_RAO) CLIPPING_UpdateGCRegion(pdc);
          if(!REGION_GetRgnBox(pdc->prgnRao, &rcRgn))
          {
             REGION_GetRgnBox(pdc->prgnVis, &rcRgn);
          }
          rc.left   = max( rc.left, 0 );
          rc.top    = max( rc.top, 0 );
          rc.right  = min( rc.right,  rcRgn.right - rcRgn.left );
          rc.bottom = min( rc.bottom, rcRgn.bottom - rcRgn.top );
          DPRINT("Rao dc %p r %d b %d\n",pdc,rcRgn.right - rcRgn.left, rcRgn.bottom - rcRgn.top);
          DPRINT("rc  l %d t %d\n",rc.left,rc.top);
          DPRINT("    r %d b %d\n",rc.right,rc.bottom);
          ret = DCB_SET;
       }
       IntDPtoLP(pdc, (PPOINTL)&rc, 2);
       DPRINT("rc1 l %d t %d\n",rc.left,rc.top);
       DPRINT("    r %d b %d\n",rc.right,rc.bottom);
    }
    else
    {
       rc = pdc->erclBounds;
       ret = DCB_SET;
    }

    /* Copy the rect to the caller */
    _SEH2_TRY
    {
        ProbeForWrite(prc, sizeof(RECT), 1);
        *prc = rc;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = 0;
    }
    _SEH2_END;

    if (flags & DCB_RESET)
    {
        if (!(flags & DCB_WINDOWMGR))
        {
           pdc->erclBoundsApp.left = pdc->erclBoundsApp.top = INT_MAX;
           pdc->erclBoundsApp.right = pdc->erclBoundsApp.bottom = INT_MIN;
        }
        else
        {
           pdc->erclBounds.left = pdc->erclBounds.top = INT_MAX;
           pdc->erclBounds.right = pdc->erclBounds.bottom = INT_MIN;
        }
    }

    DC_UnlockDc(pdc);
    return ret;
}

DWORD
APIENTRY
NtGdiSetBoundsRect(
    IN HDC hdc,
    IN LPRECT prc,
    IN DWORD flags)
{
    DWORD ret;
    PDC pdc;
    RECTL rcl;

    /* Verify arguments */
    if ((flags & DCB_ENABLE) && (flags & DCB_DISABLE)) return 0;

    /* Lock the DC */
    if (!(pdc = DC_LockDc(hdc))) return 0;

    /* Get the return value */
    ret = DCB_RESET; /* we don't have device-specific bounds */
    ret = (pdc->fs & (DC_ACCUM_APP|DC_ACCUM_WMGR) ? DCB_ENABLE : DCB_DISABLE) |
          (RECTL_bIsEmptyRect(&pdc->erclBoundsApp) ? ret & DCB_SET : DCB_SET );
    ret |= (flags & DCB_WINDOWMGR);

    if (flags & DCB_RESET)
    {
        if (!(flags & DCB_WINDOWMGR))
        {
           pdc->erclBoundsApp.left = pdc->erclBoundsApp.top = INT_MAX;
           pdc->erclBoundsApp.right = pdc->erclBoundsApp.bottom = INT_MIN;
        }
        else
        {
           pdc->erclBounds.left = pdc->erclBounds.top = INT_MAX;
           pdc->erclBounds.right = pdc->erclBounds.bottom = INT_MIN;
        }
    }

    if (flags & DCB_ACCUMULATE && prc != NULL)
    {
        /* Capture the rect */
        _SEH2_TRY
        {
            ProbeForRead(prc, sizeof(RECT), 1);
            rcl = *prc;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            DC_UnlockDc(pdc);
            _SEH2_YIELD(return 0;)
        }
        _SEH2_END;

        RECTL_vMakeWellOrdered(&rcl);

        if (!(flags & DCB_WINDOWMGR))
        {
           IntLPtoDP( pdc, (POINT *)&rcl, 2 );
           RECTL_bUnionRect(&pdc->erclBoundsApp, &pdc->erclBoundsApp, &rcl);
        }
        else
           RECTL_bUnionRect(&pdc->erclBounds, &pdc->erclBounds, &rcl);
    }

    if (flags & DCB_ENABLE)
    {
       if (!(flags & DCB_WINDOWMGR))
          pdc->fs |= DC_ACCUM_APP;
       else
          pdc->fs |= DC_ACCUM_WMGR;
    }
    if (flags & DCB_DISABLE)
    {
       if (!(flags & DCB_WINDOWMGR))
          pdc->fs &= ~DC_ACCUM_APP;
       else
          pdc->fs &= ~DC_ACCUM_WMGR;
    }
    DC_UnlockDc(pdc);
    return ret;
}

/* Translates a COLORREF to the right color in the specified DC color space */
ULONG
TranslateCOLORREF(PDC pdc, COLORREF crColor)
{
    PSURFACE psurfDC;
    PPALETTE ppalDC;
    ULONG index, ulColor, iBitmapFormat;
    EXLATEOBJ exlo;

    /* Get the DC surface */
    psurfDC = pdc->dclevel.pSurface;

    /* If no surface is selected, use the default bitmap */
    if (!psurfDC)
        psurfDC = psurfDefaultBitmap;

    /* Check what color type this is */
    switch (crColor >> 24)
    {
        case 0x00: /* RGB color */
            break;

        case 0x01: /* PALETTEINDEX */
            index = crColor & 0xFFFFFF;
            ppalDC = pdc->dclevel.ppal;
            if (index >= ppalDC->NumColors) index = 0;

            /* Get the RGB value */
            crColor = PALETTE_ulGetRGBColorFromIndex(ppalDC, index);
            break;

        case 0x02: /* PALETTERGB */

            if (pdc->dclevel.hpal != StockObjects[DEFAULT_PALETTE])
            {
                /* First find the nearest index in the dc palette */
                ppalDC = pdc->dclevel.ppal;
                index = PALETTE_ulGetNearestIndex(ppalDC, crColor & 0xFFFFFF);

                /* Get the RGB value */
                crColor = PALETTE_ulGetRGBColorFromIndex(ppalDC, index);
            }
            else
            {
                /* Use the pure color */
                crColor = crColor & 0x00FFFFFF;
            }
            break;

        case 0x10: /* DIBINDEX */
            /* Mask the value to match the target bpp */
            iBitmapFormat = psurfDC->SurfObj.iBitmapFormat;
            if (iBitmapFormat == BMF_1BPP) index = crColor & 0x1;
            else if (iBitmapFormat == BMF_4BPP) index = crColor & 0xf;
            else if (iBitmapFormat == BMF_8BPP) index = crColor & 0xFF;
            else if (iBitmapFormat == BMF_16BPP) index = crColor & 0xFFFF;
            else index = crColor & 0xFFFFFF;
            return index;

        default:
            DPRINT("Unsupported color type %u passed\n", crColor >> 24);
            crColor &= 0xFFFFFF;
    }

    /* Initialize an XLATEOBJ from RGB to the target surface */
    EXLATEOBJ_vInitialize(&exlo, &gpalRGB, psurfDC->ppal, 0xFFFFFF, 0, 0);

    /* Translate the color to the target format */
    ulColor = XLATEOBJ_iXlate(&exlo.xlo, crColor);

    /* Cleanup the XLATEOBJ */
    EXLATEOBJ_vCleanup(&exlo);

    return ulColor;
}


