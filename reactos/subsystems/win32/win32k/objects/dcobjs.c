/* 
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Functions for creation and destruction of DCs
 * FILE:              subsystem/win32/win32k/objects/dcobjs.c
 * PROGRAMER:         Timo Kreuzer (timo.kreuzer@rectos.org)
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

// HACK!
static
BOOLEAN
IntUpdateBrushXlate(PDC pdc, XLATEOBJ **ppxlo, BRUSH *pbrush)
{
    SURFACE * psurf;
    XLATEOBJ *pxlo = NULL;
    HPALETTE hPalette = NULL;

    psurf = SURFACE_LockSurface(pdc->rosdc.hBitmap);
    if (psurf)
    {
        hPalette = psurf->hDIBPalette;
        SURFACE_UnlockSurface(psurf);
    }
    if (!hPalette) hPalette = pPrimarySurface->DevInfo.hpalDefault;

    if (pbrush->flAttrs & GDIBRUSH_IS_NULL)
    {
        pxlo = NULL;
    }
    else if (pbrush->flAttrs & GDIBRUSH_IS_SOLID)
    {
        pxlo = IntEngCreateXlate(0, PAL_RGB, hPalette, NULL);
    }
    else
    {
        SURFACE *psurfPattern = SURFACE_LockSurface(pbrush->hbmPattern);
        if (psurfPattern == NULL)
            return FALSE;

        /* Special case: 1bpp pattern */
        if (psurfPattern->SurfObj.iBitmapFormat == BMF_1BPP)
        {
            if (pdc->rosdc.bitsPerPixel != 1)
                pxlo = IntEngCreateSrcMonoXlate(hPalette,
                                                pdc->pdcattr->crBackgroundClr,
                                                pbrush->BrushAttr.lbColor);
        }
        else if (pbrush->flAttrs & GDIBRUSH_IS_DIB)
        {
            pxlo = IntEngCreateXlate(0, 0, hPalette, psurfPattern->hDIBPalette);
        }

        SURFACE_UnlockSurface(psurfPattern);
    }

    if (*ppxlo != NULL)
        EngDeleteXlate(*ppxlo);

    *ppxlo = pxlo;
    return TRUE;
}


VOID
FASTCALL
DC_vUpdateFillBrush(PDC pdc)
{
    PDC_ATTR pdcattr = pdc->pdcattr;
    PBRUSH pbrFill;
    XLATEOBJ *pxlo = NULL;

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
        pbrFill = pdc->dclevel.pbrFill;

        /* ROS HACK, should use surf xlate */
        IntUpdateBrushXlate(pdc, &pdc->rosdc.XlateBrush, pbrFill);

        /* Update eboFill, realizing it, if needed */
        EBRUSHOBJ_vUpdate(&pdc->eboFill, pbrFill, pdc->rosdc.XlateBrush);
    }

    /* Check for DC brush */
    if (pdcattr->hbrush == StockObjects[DC_BRUSH])
    {
        /* ROS HACK, should use surf xlate */
        pxlo = pdc->rosdc.XlateBrush;

        /* Update the eboFill's solid color */
        EBRUSHOBJ_vSetSolidBrushColor(&pdc->eboFill, pdcattr->crPenClr, pxlo);
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
    XLATEOBJ *pxlo;

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
        pbrLine = pdc->dclevel.pbrLine;

        /* ROS HACK, should use surf xlate */
        IntUpdateBrushXlate(pdc, &pdc->rosdc.XlatePen, pbrLine);

        /* Update eboLine, realizing it, if needed */
        EBRUSHOBJ_vUpdate(&pdc->eboLine, pbrLine, pdc->rosdc.XlatePen);
    }

    /* Check for DC pen */
    if (pdcattr->hpen == StockObjects[DC_PEN])
    {
        /* ROS HACK, should use surf xlate */
        pxlo = pdc->rosdc.XlatePen;

        /* Update the eboLine's solid color */
        EBRUSHOBJ_vSetSolidBrushColor(&pdc->eboLine, pdcattr->crPenClr, pxlo);
    }

    /* Clear flags */
    pdcattr->ulDirty_ &= ~(DIRTY_LINE | DC_PEN_DIRTY);
}

VOID
FASTCALL
DC_vUpdateTextBrush(PDC pdc)
{
    PDC_ATTR pdcattr = pdc->pdcattr;
    XLATEOBJ *pxlo = NULL;
    SURFACE *psurf;
    HPALETTE hpal;

//    psurf = pdc->dclevel.pSurface;
    psurf = SURFACE_LockSurface(pdc->rosdc.hBitmap);
    if (psurf)
    {
        hpal = psurf->hDIBPalette;
        if (!hpal) hpal = pPrimarySurface->DevInfo.hpalDefault;
        pxlo = IntEngCreateXlate(0, PAL_RGB, hpal, NULL);
        SURFACE_UnlockSurface(psurf);
    }

    /* Update the eboText's solid color */
    EBRUSHOBJ_vSetSolidBrushColor(&pdc->eboText, pdcattr->crForegroundClr, pxlo);

    if (pxlo)
    {
        EngDeleteXlate(pxlo);
    }

    /* Clear flag */
    pdcattr->ulDirty_ &= ~DIRTY_TEXT;
}

VOID
FASTCALL
DC_vUpdateBackgroundBrush(PDC pdc)
{
    PDC_ATTR pdcattr = pdc->pdcattr;
    XLATEOBJ *pxlo = NULL;
    SURFACE *psurf;
    HPALETTE hpal;

//    psurf = pdc->dclevel.pSurface;
    psurf = SURFACE_LockSurface(pdc->rosdc.hBitmap);
    if (psurf)
    {
        hpal = psurf->hDIBPalette;
        if (!hpal) hpal = pPrimarySurface->DevInfo.hpalDefault;
        pxlo = IntEngCreateXlate(0, PAL_RGB, hpal, NULL);
        SURFACE_UnlockSurface(psurf);
    }

    /* Update the eboBackground's solid color */
    EBRUSHOBJ_vSetSolidBrushColor(&pdc->eboBackground, pdcattr->crBackgroundClr, pxlo);

    if (pxlo)
    {
        EngDeleteXlate(pxlo);
    }

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

    // FIXME: mark the palette as a [fore\back]ground pal
    pdc = DC_LockDc(hDC);
    if (!pdc)
    {
        return NULL;
    }

    /* Check if this is a valid palette handle */
    ppal = PALETTE_LockPalette(hpal);
    if (!ppal)
    {
        DC_UnlockDc(pdc);
        return NULL;
    }

    // FIXME: This looks wrong
    /* Is this a valid palette for this depth? */
    if ((pdc->rosdc.bitsPerPixel <= 8 && ppal->Mode == PAL_INDEXED) ||
        (pdc->rosdc.bitsPerPixel > 8))
    {
        /* Get old palette, set new one */
        oldPal = pdc->dclevel.hpal;
        pdc->dclevel.hpal = hpal;

        /* Mark the brushes invalid */
        pdc->pdcattr->ulDirty_ |= DIRTY_FILL | DIRTY_LINE |
                                  DIRTY_BACKGROUND | DIRTY_TEXT;
    }

    PALETTE_UnlockPalette(ppal);
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
    IN HDC hDC,
    IN HBITMAP hBmp)
{
    PDC pDC;
    PDC_ATTR pdcattr;
    HBITMAP hOrgBmp;
    PSURFACE psurfBmp, psurfOld;
    HRGN hVisRgn;

    if (hDC == NULL || hBmp == NULL) return NULL;

    pDC = DC_LockDc(hDC);
    if (!pDC)
    {
        return NULL;
    }
    pdcattr = pDC->pdcattr;

    /* must be memory dc to select bitmap */
    if (pDC->dctype != DC_TYPE_MEMORY)
    {
        DC_UnlockDc(pDC);
        return NULL;
    }

    psurfBmp = SURFACE_LockSurface(hBmp);
    if (!psurfBmp)
    {
        DC_UnlockDc(pDC);
        return NULL;
    }

    /* Get the handle for the old bitmap */
    psurfOld = pDC->dclevel.pSurface;
    hOrgBmp = psurfOld ? psurfOld->BaseObject.hHmgr : NULL;

    /* FIXME: ros hack */
    hOrgBmp = pDC->rosdc.hBitmap;

    pDC->rosdc.hBitmap = hBmp;

    /* Release the old bitmap, reference the new */
    DC_vSelectSurface(pDC, psurfBmp);

    // If Info DC this is zero and pSurface is moved to DC->pSurfInfo.
    psurfBmp->hDC = hDC;

    // if we're working with a DIB, get the palette 
    // [fixme: only create if the selected palette is null]
    if (psurfBmp->hSecure)
    {
//        pDC->rosdc.bitsPerPixel = psurfBmp->dib->dsBmih.biBitCount; ???
        pDC->rosdc.bitsPerPixel = BitsPerFormat(psurfBmp->SurfObj.iBitmapFormat);
    }
    else
    {
        pDC->rosdc.bitsPerPixel = BitsPerFormat(psurfBmp->SurfObj.iBitmapFormat);
    }

    /* FIXME; improve by using a region without a handle and selecting it */
    hVisRgn = NtGdiCreateRectRgn(0,
                                 0,
                                 psurfBmp->SurfObj.sizlBitmap.cx,
                                 psurfBmp->SurfObj.sizlBitmap.cy);

    /* Release the exclusive lock */
    SURFACE_UnlockSurface(psurfBmp);

    /* Mark the brushes invalid */
    pdcattr->ulDirty_ |= DIRTY_FILL | DIRTY_LINE;

    DC_UnlockDc(pDC);

    if (hVisRgn)
    {
        GdiSelectVisRgn(hDC, hVisRgn);
        GreDeleteObject(hVisRgn);
    }

    return hOrgBmp;
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
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
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
        SetLastWin32Error(ERROR_CAN_NOT_COMPLETE);
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
        SetLastWin32Error(ERROR_INVALID_HANDLE);
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
            SelObject = pdc->rosdc.hBitmap;
            break;

        case GDI_OBJECT_TYPE_COLORSPACE:
            DPRINT1("FIXME: NtGdiGetCurrentObject() ObjectType OBJ_COLORSPACE not supported yet!\n");
            // SelObject = dc->dclevel.pColorSpace.BaseObject.hHmgr; ?
            SelObject = NULL;
            break;

        default:
            SelObject = NULL;
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            break;
    }

    DC_UnlockDc(pdc);
    return SelObject;
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

