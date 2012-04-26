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
        EBRUSHOBJ_vUpdate(&pdc->eboFill, pdc->dclevel.pbrFill, pdc);
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
        EBRUSHOBJ_vUpdate(&pdc->eboLine, pdc->dclevel.pbrLine, pdc);
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
        EBRUSHOBJ_vUpdate(&pdc->eboText, pbrDefaultBrush, pdc);

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
        EBRUSHOBJ_vUpdate(&pdc->eboBackground, pbrDefaultBrush, pdc);

    /* Update the eboBackground's solid color */
    EBRUSHOBJ_vSetSolidRGBColor(&pdc->eboBackground, pdcattr->crBackgroundClr);

    /* Clear flag */
    pdcattr->ulDirty_ &= ~DIRTY_BACKGROUND;
}

HPALETTE
FASTCALL
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

    /* Is this a valid palette for this depth? */
	if ((BitsPerFormat(pdc->dclevel.pSurface->SurfObj.iBitmapFormat) <= 8
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
    HRGN hVisRgn;
    HDC hdcOld;
    ULONG cBitsPixel;
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

        // HACK
        psurfNew = SURFACE_ShareLockSurface(hbmp);
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

        /* Check if the bitmap is compatile with the dc */
        cBitsPixel = gajBitsPerFormat[psurfNew->SurfObj.iBitmapFormat];
        if ((cBitsPixel != 1) &&
            (cBitsPixel != pdc->ppdev->gdiinfo.cBitsPixel) &&
            (psurfNew->hSecure == NULL))
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

    /* Mark the dc brushes invalid */
    pdc->pdcattr->ulDirty_ |= DIRTY_FILL | DIRTY_LINE;

    /* FIXME: Improve by using a region without a handle and selecting it */
    hVisRgn = IntSysCreateRectRgn( 0,
                                   0,
                                   pdc->dclevel.sizl.cx,
                                   pdc->dclevel.sizl.cy);

    if (hVisRgn)
    {
        GdiSelectVisRgn(hdc, hVisRgn);
        GreDeleteObject(hVisRgn);
    }

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
    HRGN  hrgnPath;
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
    else if (PATH_PathToRegion(pPath, pdcattr->jFillMode, &hrgnPath))
    {
        success = GdiExtSelectClipRgn(pdc, hrgnPath, Mode) != ERROR;
        GreDeleteObject( hrgnPath );

        /* Empty the path */
        if (success)
            PATH_EmptyPath(pPath);

        /* FIXME: Should this function delete the path even if it failed? */
    }

    PATH_UnlockPath(pPath);
    DC_UnlockDc(pdc);

    return success;
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
            SelObject = psurf ? psurf->BaseObject.hHmgr : NULL;
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
    HRGN hrgnSrc = NULL;
    POINTL ptlOrg;

    pdc = DC_LockDc(hdc);
    if (!pdc)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    switch (iCode)
    {
        case CLIPRGN:
            hrgnSrc = pdc->rosdc.hClipRgn;
//            if (pdc->dclevel.prgnClip) hrgnSrc = pdc->dclevel.prgnClip->BaseObject.hHmgr;
            break;
        case METARGN:
            if (pdc->dclevel.prgnMeta)
                hrgnSrc = pdc->dclevel.prgnMeta->BaseObject.hHmgr;
            break;
        case APIRGN:
            if (pdc->prgnAPI) hrgnSrc = pdc->prgnAPI->BaseObject.hHmgr;
//            else if (pdc->dclevel.prgnClip) hrgnSrc = pdc->dclevel.prgnClip->BaseObject.hHmgr;
            else if (pdc->rosdc.hClipRgn) hrgnSrc = pdc->rosdc.hClipRgn;
            else if (pdc->dclevel.prgnMeta) hrgnSrc = pdc->dclevel.prgnMeta->BaseObject.hHmgr;
            break;
        case SYSRGN:
            if (pdc->prgnVis)
            {
                PREGION prgnDest = REGION_LockRgn(hrgnDest);
                ret = IntGdiCombineRgn(prgnDest, pdc->prgnVis, 0, RGN_COPY) == ERROR ? -1 : 1;
                REGION_UnlockRgn(prgnDest);
            }
            break;
        default:
            hrgnSrc = NULL;
    }

    if (hrgnSrc)
    {
        ret = NtGdiCombineRgn(hrgnDest, hrgnSrc, 0, RGN_COPY) == ERROR ? -1 : 1;
    }

    if (iCode == SYSRGN)
    {
        ptlOrg = pdc->ptlDCOrig;
        NtGdiOffsetRgn(hrgnDest, ptlOrg.x, ptlOrg.y );
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
