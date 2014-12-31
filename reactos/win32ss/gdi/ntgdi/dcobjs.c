/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS Win32k subsystem
 * PURPOSE:           Functions for creation and destruction of DCs
 * FILE:              subsystems/win32/win32k/objects/dcobjs.c
 * PROGRAMER:         Timo Kreuzer (timo.kreuzer@rectos.org)
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

VOID
FASTCALL
DC_vUpdateFillBrush(PDC pdc)
{
    PDC_ATTR pdcattr = pdc->pdcattr;
    PBRUSH pbrFill;

    /* Check if the brush handle has changed */
    if (pdcattr->hbrush != pdc->dclevel.pbrFill->BaseObject.hHmgr)
    {
        /* Try to lock the new brush */
        pbrFill = BRUSH_ShareLockBrush(pdcattr->hbrush);
        if (pbrFill)
        {
            /* Unlock old brush, set new brush */
            BRUSH_ShareUnlockBrush(pdc->dclevel.pbrFill);
            pdc->dclevel.pbrFill = pbrFill;

            /* Mark eboFill as dirty */
            pdcattr->ulDirty_ |= DIRTY_FILL;
        }
        else
        {
            /* Invalid brush handle, restore old one */
            pdcattr->hbrush = pdc->dclevel.pbrFill->BaseObject.hHmgr;
        }
    }

    /* Check if the EBRUSHOBJ needs update */
    if (pdcattr->ulDirty_ & DIRTY_FILL)
    {
        /* Update eboFill */
        EBRUSHOBJ_vUpdateFromDC(&pdc->eboFill, pdc->dclevel.pbrFill, pdc);
    }

    /* Check for DC brush */
    if (pdcattr->hbrush == StockObjects[DC_BRUSH])
    {
        /* ROS HACK, should use surf xlate */
        /* Update the eboFill's solid color */
        EBRUSHOBJ_vSetSolidRGBColor(&pdc->eboFill, pdcattr->crPenClr);
    }

    /* Clear flags */
    pdcattr->ulDirty_ &= ~(DIRTY_FILL | DC_BRUSH_DIRTY);
}

VOID
FASTCALL
DC_vUpdateLineBrush(PDC pdc)
{
    PDC_ATTR pdcattr = pdc->pdcattr;
    PBRUSH pbrLine;

    /* Check if the pen handle has changed */
    if (pdcattr->hpen != pdc->dclevel.pbrLine->BaseObject.hHmgr)
    {
        /* Try to lock the new pen */
        pbrLine = PEN_ShareLockPen(pdcattr->hpen);
        if (pbrLine)
        {
            /* Unlock old brush, set new brush */
            BRUSH_ShareUnlockBrush(pdc->dclevel.pbrLine);
            pdc->dclevel.pbrLine = pbrLine;

            /* Mark eboLine as dirty */
            pdcattr->ulDirty_ |= DIRTY_LINE;
        }
        else
        {
            /* Invalid pen handle, restore old one */
            pdcattr->hpen = pdc->dclevel.pbrLine->BaseObject.hHmgr;
        }
    }

    /* Check if the EBRUSHOBJ needs update */
    if (pdcattr->ulDirty_ & DIRTY_LINE)
    {
        /* Update eboLine */
        EBRUSHOBJ_vUpdateFromDC(&pdc->eboLine, pdc->dclevel.pbrLine, pdc);
    }

    /* Check for DC pen */
    if (pdcattr->hpen == StockObjects[DC_PEN])
    {
        /* Update the eboLine's solid color */
        EBRUSHOBJ_vSetSolidRGBColor(&pdc->eboLine, pdcattr->crPenClr);
    }

    /* Clear flags */
    pdcattr->ulDirty_ &= ~(DIRTY_LINE | DC_PEN_DIRTY);
}

VOID
FASTCALL
DC_vUpdateTextBrush(PDC pdc)
{
    PDC_ATTR pdcattr = pdc->pdcattr;

    /* Timo : The text brush should never be changed.
     * Jérôme : Yeah, but its palette must be updated anyway! */
    if(pdcattr->ulDirty_ & DIRTY_TEXT)
        EBRUSHOBJ_vUpdateFromDC(&pdc->eboText, pbrDefaultBrush, pdc);

    /* Update the eboText's solid color */
    EBRUSHOBJ_vSetSolidRGBColor(&pdc->eboText, pdcattr->crForegroundClr);

    /* Clear flag */
    pdcattr->ulDirty_ &= ~DIRTY_TEXT;
}

VOID
FASTCALL
DC_vUpdateBackgroundBrush(PDC pdc)
{
    PDC_ATTR pdcattr = pdc->pdcattr;

    if(pdcattr->ulDirty_ & DIRTY_BACKGROUND)
        EBRUSHOBJ_vUpdateFromDC(&pdc->eboBackground, pbrDefaultBrush, pdc);

    /* Update the eboBackground's solid color */
    EBRUSHOBJ_vSetSolidRGBColor(&pdc->eboBackground, pdcattr->crBackgroundClr);

    /* Clear flag */
    pdcattr->ulDirty_ &= ~DIRTY_BACKGROUND;
}

VOID
NTAPI
DC_vSetBrushOrigin(PDC pdc, LONG x, LONG y)
{
    /* Set the brush origin */
    pdc->dclevel.ptlBrushOrigin.x = x;
    pdc->dclevel.ptlBrushOrigin.y = y;

    /* Set the fill origin */
    pdc->ptlFillOrigin.x = x + pdc->ptlDCOrig.x;
    pdc->ptlFillOrigin.y = y + pdc->ptlDCOrig.y;
}

/**
 * \name NtGdiSetBrushOrg
 *
 * \brief Sets the brush origin that GDI uses when drawing with pattern
 *     brushes. The brush origin is relative to the DC origin.
 *
 * @implemented
 */
_Success_(return != FALSE)
BOOL
APIENTRY
NtGdiSetBrushOrg(
    _In_ HDC hdc,
    _In_ INT x,
    _In_ INT y,
    _Out_opt_ LPPOINT pptOut)
{
    PDC pdc;

    /* Lock the DC */
    pdc = DC_LockDc(hdc);
    if (pdc == NULL)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    /* Check if the old origin was requested */
    if (pptOut != NULL)
    {
        /* Enter SEH for buffer transfer */
        _SEH2_TRY
        {
            /* Probe and copy the old origin */
            ProbeForWrite(pptOut, sizeof(POINT), 1);
            *pptOut = pdc->pdcattr->ptlBrushOrigin;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            DC_UnlockDc(pdc);
            _SEH2_YIELD(return FALSE);
        }
        _SEH2_END;
    }

    /* Call the internal function */
    DC_vSetBrushOrigin(pdc, x, y);

    /* Unlock the DC and return success */
    DC_UnlockDc(pdc);
    return TRUE;
}

HPALETTE
NTAPI
GdiSelectPalette(
    HDC hDC,
    HPALETTE hpal,
    BOOL ForceBackground)
{
    PDC pdc;
    HPALETTE oldPal = NULL;
    PPALETTE ppal;

    // FIXME: Mark the palette as a [fore\back]ground pal
    pdc = DC_LockDc(hDC);
    if (!pdc)
    {
        return NULL;
    }

    /* Check if this is a valid palette handle */
    ppal = PALETTE_ShareLockPalette(hpal);
    if (!ppal)
    {
        DC_UnlockDc(pdc);
        return NULL;
    }

    /// FIXME: we shouldn't dereference pSurface when the PDEV is not locked
    /* Is this a valid palette for this depth? */
	if ((!pdc->dclevel.pSurface) ||
        (BitsPerFormat(pdc->dclevel.pSurface->SurfObj.iBitmapFormat) <= 8
            && (ppal->flFlags & PAL_INDEXED)) ||
        (BitsPerFormat(pdc->dclevel.pSurface->SurfObj.iBitmapFormat) > 8))
    {
        /* Get old palette, set new one */
        oldPal = pdc->dclevel.hpal;
        pdc->dclevel.hpal = hpal;
        DC_vSelectPalette(pdc, ppal);

        /* Mark the brushes invalid */
        pdc->pdcattr->ulDirty_ |= DIRTY_FILL | DIRTY_LINE |
                                  DIRTY_BACKGROUND | DIRTY_TEXT;
    }

    if(pdc->dctype == DCTYPE_MEMORY)
    {
        // This didn't work anyway
        //IntGdiRealizePalette(hDC);
    }

    PALETTE_ShareUnlockPalette(ppal);
    DC_UnlockDc(pdc);

    return oldPal;
}

 /*
 * @implemented
 */
HBRUSH
APIENTRY
NtGdiSelectBrush(
    IN HDC hDC,
    IN HBRUSH hBrush)
{
    PDC pDC;
    HBRUSH hOrgBrush;

    if (hDC == NULL || hBrush == NULL) return NULL;

    pDC = DC_LockDc(hDC);
    if (!pDC)
    {
        return NULL;
    }

    /* Simply return the user mode value, without checking */
    hOrgBrush = pDC->pdcattr->hbrush;
    pDC->pdcattr->hbrush = hBrush;
    DC_vUpdateFillBrush(pDC);

    DC_UnlockDc(pDC);

    return hOrgBrush;
}

 /*
 * @implemented
 */
HPEN
APIENTRY
NtGdiSelectPen(
    IN HDC hDC,
    IN HPEN hPen)
{
    PDC pDC;
    HPEN hOrgPen;

    if (hDC == NULL || hPen == NULL) return NULL;

    pDC = DC_LockDc(hDC);
    if (!pDC)
    {
        return NULL;
    }

    /* Simply return the user mode value, without checking */
    hOrgPen = pDC->pdcattr->hpen;
    pDC->pdcattr->hpen = hPen;
    DC_vUpdateLineBrush(pDC);

    DC_UnlockDc(pDC);

    return hOrgPen;
}

BOOL
NTAPI
DC_bIsBitmapCompatible(PDC pdc, PSURFACE psurf)
{
    ULONG cBitsPixel;

    /* Must be an API bitmap */
    if (!(psurf->flags & API_BITMAP)) return FALSE;

    /* DIB sections are always compatible */
    if (psurf->hSecure != NULL) return TRUE;

    /* See if this is the same PDEV */
    if (psurf->SurfObj.hdev == (HDEV)pdc->ppdev)
        return TRUE;

    /* Get the bit depth of the bitmap */
    cBitsPixel = gajBitsPerFormat[psurf->SurfObj.iBitmapFormat];

    /* 1 BPP is compatible */
    if ((cBitsPixel == 1) || (cBitsPixel == pdc->ppdev->gdiinfo.cBitsPixel))
        return TRUE;

    return FALSE;
}

/*
 * @implemented
 */
HBITMAP
APIENTRY
NtGdiSelectBitmap(
    IN HDC hdc,
    IN HBITMAP hbmp)
{
    PDC pdc;
    HBITMAP hbmpOld;
    PSURFACE psurfNew, psurfOld;
    HDC hdcOld;
    ASSERT_NOGDILOCKS();

    /* Verify parameters */
    if (hdc == NULL || hbmp == NULL) return NULL;

    /* First lock the DC */
    pdc = DC_LockDc(hdc);
    if (!pdc)
    {
        return NULL;
    }

    /* Must be a memory dc to select a bitmap */
    if (pdc->dctype != DC_TYPE_MEMORY)
    {
        DC_UnlockDc(pdc);
        return NULL;
    }

    /* Save the old bitmap */
    psurfOld = pdc->dclevel.pSurface;

    /* Check if there is a bitmap selected */
    if (psurfOld)
    {
        /* Get the old bitmap's handle */
        hbmpOld = psurfOld->BaseObject.hHmgr;
    }
    else
    {
        /* Use the default bitmap */
        hbmpOld = StockObjects[DEFAULT_BITMAP];
    }

    /* Check if the new bitmap is already selected */
    if (hbmp == hbmpOld)
    {
        /* Unlock the DC and return the old bitmap */
        DC_UnlockDc(pdc);
        return hbmpOld;
    }

    /* Check if the default bitmap was passed */
    if (hbmp == StockObjects[DEFAULT_BITMAP])
    {
        psurfNew = NULL;

        /* Default bitmap is 1x1 pixel */
        pdc->dclevel.sizl.cx = 1;
        pdc->dclevel.sizl.cy = 1;
    }
    else
    {
        /* Reference the new bitmap and check if it's valid */
        psurfNew = SURFACE_ShareLockSurface(hbmp);
        if (!psurfNew)
        {
            DC_UnlockDc(pdc);
            return NULL;
        }

        /* Check if the bitmap is compatible with the dc */
        if (!DC_bIsBitmapCompatible(pdc, psurfNew))
        {
            /* Dereference the bitmap, unlock the DC and fail. */
            SURFACE_ShareUnlockSurface(psurfNew);
            DC_UnlockDc(pdc);
            return NULL;
        }

        /* Set the bitmap's hdc and check if it was set before */
        hdcOld = InterlockedCompareExchangePointer((PVOID*)&psurfNew->hdc, hdc, 0);
        if (hdcOld != NULL)
        {
            /* The bitmap is already selected into a different DC */
            ASSERT(hdcOld != hdc);

            /* Dereference the bitmap, unlock the DC and fail. */
            SURFACE_ShareUnlockSurface(psurfNew);
            DC_UnlockDc(pdc);
            return NULL;
        }

        /* Copy the bitmap size */
        pdc->dclevel.sizl = psurfNew->SurfObj.sizlBitmap;

        /* Check if the bitmap is a dibsection */
        if (psurfNew->hSecure)
        {
            /* Set DIBSECTION attribute */
            pdc->pdcattr->ulDirty_ |= DC_DIBSECTION;
        }
        else
        {
            /* Remove DIBSECTION attribute */
            pdc->pdcattr->ulDirty_ &= ~DC_DIBSECTION;
        }
    }

    /* Select the new bitmap */
    pdc->dclevel.pSurface = psurfNew;

    /* Check if there was a bitmap selected before */
    if (psurfOld)
    {
        /* Reset hdc of the old bitmap, it isn't selected anymore */
        psurfOld->hdc = NULL;

        /* Dereference the old bitmap */
        SURFACE_ShareUnlockSurface(psurfOld);
    }

    /* Mark the DC brushes invalid */
    pdc->pdcattr->ulDirty_ |= DIRTY_FILL | DIRTY_LINE | DC_FLAG_DIRTY_RAO;

    /* Update the system region */
    REGION_SetRectRgn(pdc->prgnVis,
                      0,
                      0,
                      pdc->dclevel.sizl.cx,
                      pdc->dclevel.sizl.cy);

    /* Unlock the DC */
    DC_UnlockDc(pdc);

    /* Return the old bitmap handle */
    return hbmpOld;
}


BOOL
APIENTRY
NtGdiSelectClipPath(
    HDC hDC,
    int Mode)
{
    PREGION  RgnPath;
    PPATH pPath;
    BOOL  success = FALSE;
    PDC_ATTR pdcattr;
    PDC pdc;

    pdc = DC_LockDc(hDC);
    if (!pdc)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    pdcattr = pdc->pdcattr;

    pPath = PATH_LockPath(pdc->dclevel.hPath);
    if (!pPath)
    {
        DC_UnlockDc(pdc);
        return FALSE;
    }

    /* Check that path is closed */
    if (pPath->state != PATH_Closed)
    {
        EngSetLastError(ERROR_CAN_NOT_COMPLETE);
        DC_UnlockDc(pdc);
        return FALSE;
    }

    /* Construct a region from the path */
    RgnPath = IntSysCreateRectpRgn(0, 0, 0, 0);
    if (!RgnPath)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        DC_UnlockDc(pdc);
        return FALSE;
    }

    if (!PATH_PathToRegion(pPath, pdcattr->jFillMode, RgnPath))
    {
        EngSetLastError(ERROR_CAN_NOT_COMPLETE);
        REGION_Delete(RgnPath);
        DC_UnlockDc(pdc);
        return FALSE;
    }

    success = IntGdiExtSelectClipRgn(pdc, RgnPath, Mode) != ERROR;
    REGION_Delete(RgnPath);

    /* Empty the path */
    if (success)
        PATH_EmptyPath(pPath);

    /* FIXME: Should this function delete the path even if it failed? */

    PATH_UnlockPath(pPath);
    DC_UnlockDc(pdc);

    return success;
}

HFONT
NTAPI
DC_hSelectFont(
    _In_ PDC pdc,
    _In_ HFONT hlfntNew)
{
    PLFONT plfntNew;
    HFONT hlfntOld;

    // Legacy crap that will die with font engine rewrite
    if (!NT_SUCCESS(TextIntRealizeFont(hlfntNew, NULL)))
    {
        return NULL;
    }

    /* Get the current selected font */
    hlfntOld = pdc->dclevel.plfnt->BaseObject.hHmgr;

    /* Check if a new font should be selected */
    if (hlfntNew != hlfntOld)
    {
        /* Lock the new font */
        plfntNew = LFONT_ShareLockFont(hlfntNew);
        if (plfntNew)
        {
            /* Success, dereference the old font */
            LFONT_ShareUnlockFont(pdc->dclevel.plfnt);

            /* Select the new font */
            pdc->dclevel.plfnt = plfntNew;
            pdc->pdcattr->hlfntNew = hlfntNew;

            /* Update dirty flags */
            pdc->pdcattr->ulDirty_ |= DIRTY_CHARSET;
            pdc->pdcattr->ulDirty_ &= ~SLOW_WIDTHS;
        }
        else
        {
            /* Failed, restore old, return NULL */
            pdc->pdcattr->hlfntNew = hlfntOld;
            hlfntOld = NULL;
        }
    }

    return hlfntOld;
}

HFONT
APIENTRY
NtGdiSelectFont(
    _In_ HDC hdc,
    _In_ HFONT hfont)
{
    HFONT hfontOld;
    PDC pdc;

    /* Check parameters */
    if ((hdc == NULL) || (hfont == NULL))
    {
        return NULL;
    }

    /* Lock the DC */
    pdc = DC_LockDc(hdc);
    if (!pdc)
    {
        return NULL;
    }

    /* Call the internal function */
    hfontOld = DC_hSelectFont(pdc, hfont);

    /* Unlock the DC */
    DC_UnlockDc(pdc);

    /* Return the previously selected font */
    return hfontOld;
}

HANDLE
APIENTRY
NtGdiGetDCObject(HDC hDC, INT ObjectType)
{
    HGDIOBJ SelObject;
    DC *pdc;
    PDC_ATTR pdcattr;

    /* From Wine: GetCurrentObject does not SetLastError() on a null object */
    if(!hDC) return NULL;

    if(!(pdc = DC_LockDc(hDC)))
    {
        return NULL;
    }
    pdcattr = pdc->pdcattr;

    if (pdcattr->ulDirty_ & (DIRTY_FILL | DC_BRUSH_DIRTY))
        DC_vUpdateFillBrush(pdc);

    if (pdcattr->ulDirty_ & (DIRTY_LINE | DC_PEN_DIRTY))
        DC_vUpdateLineBrush(pdc);

    switch(ObjectType)
    {
        case GDI_OBJECT_TYPE_EXTPEN:
        case GDI_OBJECT_TYPE_PEN:
            SelObject = pdcattr->hpen;
            break;

        case GDI_OBJECT_TYPE_BRUSH:
            SelObject = pdcattr->hbrush;
            break;

        case GDI_OBJECT_TYPE_PALETTE:
            SelObject = pdc->dclevel.hpal;
            break;

        case GDI_OBJECT_TYPE_FONT:
            SelObject = pdcattr->hlfntNew;
            break;

        case GDI_OBJECT_TYPE_BITMAP:
        {
            SURFACE *psurf = pdc->dclevel.pSurface;
            SelObject = psurf ? psurf->BaseObject.hHmgr : StockObjects[DEFAULT_BITMAP];
            break;
        }

        case GDI_OBJECT_TYPE_COLORSPACE:
            DPRINT1("FIXME: NtGdiGetCurrentObject() ObjectType OBJ_COLORSPACE not supported yet!\n");
            // SelObject = dc->dclevel.pColorSpace.BaseObject.hHmgr; ?
            SelObject = NULL;
            break;

        default:
            SelObject = NULL;
            EngSetLastError(ERROR_INVALID_PARAMETER);
            break;
    }

    DC_UnlockDc(pdc);
    return SelObject;
}

/* See WINE, MSDN, OSR and Feng Yuan - Windows Graphics Programming Win32 GDI and DirectDraw
 *
 * 1st: http://www.codeproject.com/gdi/cliprgnguide.asp is wrong!
 *
 * The intersection of the clip with the meta region is not Rao it's API!
 * Go back and read 7.2 Clipping pages 418-19:
 * Rao = API & Vis:
 * 1) The Rao region is the intersection of the API region and the system region,
 *    named after the Microsoft engineer who initially proposed it.
 * 2) The Rao region can be calculated from the API region and the system region.
 *
 * API:
 *    API region is the intersection of the meta region and the clipping region,
 *    clearly named after the fact that it is controlled by GDI API calls.
 */
INT
APIENTRY
NtGdiGetRandomRgn(
    HDC hdc,
    HRGN hrgnDest,
    INT iCode)
{
    INT ret = 0;
    PDC pdc;
    PREGION prgnSrc = NULL;

    pdc = DC_LockDc(hdc);
    if (!pdc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return -1;
    }

    switch (iCode)
    {
        case CLIPRGN:
            prgnSrc = pdc->dclevel.prgnClip;
            break;

        case METARGN:
            prgnSrc = pdc->dclevel.prgnMeta;
            break;

        case APIRGN:
            if (pdc->fs & DC_FLAG_DIRTY_RAO)
                CLIPPING_UpdateGCRegion(pdc);
            if (pdc->prgnAPI)
            {
                prgnSrc = pdc->prgnAPI;
            }
            else if (pdc->dclevel.prgnClip)
            {
                prgnSrc = pdc->dclevel.prgnClip;
            }
            else if (pdc->dclevel.prgnMeta)
            {
                prgnSrc = pdc->dclevel.prgnMeta;
            }
            break;

        case SYSRGN:
            prgnSrc = pdc->prgnVis;
            break;

        default:
            break;
    }

    if (prgnSrc)
    {
        PREGION prgnDest = REGION_LockRgn(hrgnDest);
        if (prgnDest)
        {
            ret = IntGdiCombineRgn(prgnDest, prgnSrc, 0, RGN_COPY) == ERROR ? -1 : 1;
            if ((ret == 1) && (iCode == SYSRGN))
            {
                /// \todo FIXME This is not really correct, since we already modified the region
                ret = REGION_bOffsetRgn(prgnDest, pdc->ptlDCOrig.x, pdc->ptlDCOrig.y);
            }
            REGION_UnlockRgn(prgnDest);
        }
        else
            ret = -1;
    }

    DC_UnlockDc(pdc);

    return ret;
}

ULONG
APIENTRY
NtGdiEnumObjects(
    IN HDC hdc,
    IN INT iObjectType,
    IN ULONG cjBuf,
    OUT OPTIONAL PVOID pvBuf)
{
    UNIMPLEMENTED;
    return 0;
}

/* EOF */
