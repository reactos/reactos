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


VOID
FASTCALL
DC_vUpdateFillBrush(PDC pdc)
{
    PDC_ATTR pdcattr = pdc->pdcattr;
    PBRUSH pbrFill;
    XLATEOBJ *pxlo;
    ULONG iSolidColor;

    /* Check if update of eboFill is needed */
    if (pdcattr->ulDirty_ & DIRTY_FILL)
    {
        /* ROS HACK, should use surf xlate */
        pxlo = pdc->rosdc.XlatePen;

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

                /* Update eboFill, realizing it, if needed */
                EBRUSHOBJ_vUpdate(&pdc->eboFill, pbrFill, pxlo);
            }
            else
            {
                /* Invalid brush handle, restore old one */
                pdcattr->hbrush = pdc->dclevel.pbrFill->BaseObject.hHmgr;
            }
        }

        /* Check for DC brush */
        if (pdcattr->hbrush == StockObjects[DC_BRUSH])
        {
            /* Translate the color to the target format */
            iSolidColor = XLATEOBJ_iXlate(pxlo, pdcattr->crPenClr);

            /* Update the eboFill's solid color */
            EBRUSHOBJ_vSetSolidBrushColor(&pdc->eboFill, iSolidColor);
        }

        /* Clear flag */
        pdcattr->ulDirty_ &= ~DIRTY_FILL;
    }
}

VOID
FASTCALL
DC_vUpdateLineBrush(PDC pdc)
{
    PDC_ATTR pdcattr = pdc->pdcattr;
    PBRUSH pbrLine;
    XLATEOBJ *pxlo;
    ULONG iSolidColor;

    /* Check if update of eboLine is needed */
    if (pdcattr->ulDirty_ & DIRTY_LINE)
    {
        /* ROS HACK, should use surf xlate */
        pxlo = pdc->rosdc.XlatePen;

        /* Check if the pen handle has changed */
        if (pdcattr->hpen != pdc->dclevel.pbrLine->BaseObject.hHmgr)
        {
            /* Try to lock the new pen */
            pbrLine = BRUSH_ShareLockBrush(pdcattr->hpen);
            if (pbrLine)
            {
                /* Unlock old brush, set new brush */
                BRUSH_ShareUnlockBrush(pdc->dclevel.pbrLine);
                pdc->dclevel.pbrLine = pbrLine;

                /* Update eboLine, realizing it, if needed */
                EBRUSHOBJ_vUpdate(&pdc->eboLine, pbrLine, pxlo);
            }
            else
            {
                /* Invalid pen handle, restore old one */
                pdcattr->hpen = pdc->dclevel.pbrLine->BaseObject.hHmgr;
            }
        }

        /* Check for DC pen */
        if (pdcattr->hpen == StockObjects[DC_PEN])
        {
            /* Translate the color to the target format */
            iSolidColor = XLATEOBJ_iXlate(pxlo, pdcattr->crPenClr);

            /* Update the eboLine's solid color */
            EBRUSHOBJ_vSetSolidBrushColor(&pdc->eboLine, iSolidColor);
        }

        /* Clear flag */
        pdcattr->ulDirty_ &= ~DIRTY_LINE;
    }
}

VOID
FASTCALL
DC_vUpdateTextBrush(PDC pdc)
{
    PDC_ATTR pdcattr = pdc->pdcattr;
    XLATEOBJ *pxlo;
    ULONG iSolidColor;

    /* Check if update of eboText is needed */
    if (pdcattr->ulDirty_ & DIRTY_TEXT)
    {
        /* ROS HACK, should use surf xlate */
        pxlo = pdc->rosdc.XlatePen;

        /* Translate the color to the target format */
        iSolidColor = XLATEOBJ_iXlate(pxlo, pdcattr->crForegroundClr);

        /* Update the eboText's solid color */
        EBRUSHOBJ_vSetSolidBrushColor(&pdc->eboText, iSolidColor);

        /* Clear flag */
        pdcattr->ulDirty_ &= ~DIRTY_TEXT;
    }
}

VOID
FASTCALL
DC_vUpdateBackgroundBrush(PDC pdc)
{
    PDC_ATTR pdcattr = pdc->pdcattr;
    XLATEOBJ *pxlo;
    ULONG iSolidColor;

    /* Check if update of eboBackground is needed */
    if (pdcattr->ulDirty_ & DIRTY_BACKGROUND)
    {
        /* ROS HACK, should use surf xlate */
        pxlo = pdc->rosdc.XlatePen;

        /* Translate the color to the target format */
        iSolidColor = XLATEOBJ_iXlate(pxlo, pdcattr->crBackgroundClr);

        /* Update the eboBackground's solid color */
        EBRUSHOBJ_vSetSolidBrushColor(&pdc->eboBackground, iSolidColor);

        /* Clear flag */
        pdcattr->ulDirty_ &= ~DIRTY_BACKGROUND;
    }
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
    PPALGDI ppal;

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

    /* Is this a valid palette for this depth? */
    if ((pdc->rosdc.bitsPerPixel <= 8 && ppal->Mode == PAL_INDEXED) ||
        (pdc->rosdc.bitsPerPixel > 8  && ppal->Mode != PAL_INDEXED))
    {
        oldPal = pdc->dclevel.hpal;
        pdc->dclevel.hpal = hpal;
    }
    else if (pdc->rosdc.bitsPerPixel > 8 && ppal->Mode == PAL_INDEXED)
    {
        oldPal = pdc->dclevel.hpal;
        pdc->dclevel.hpal = hpal;
    }

    PALETTE_UnlockPalette(ppal);
    DC_UnlockDc(pdc);

    return oldPal;
}

HBRUSH
FASTCALL
IntGdiSelectBrush(
    PDC pDC,
    HBRUSH hBrush)
{
    PDC_ATTR pdcattr;
    HBRUSH hOrgBrush;
    PBRUSH pbrush;
    XLATEOBJ *XlateObj;
    BOOLEAN bFailed;

    if (pDC == NULL || hBrush == NULL) return NULL;

    pdcattr = pDC->pdcattr;

    pbrush = BRUSH_LockBrush(hBrush);
    if (pbrush == NULL)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return NULL;
    }

    XlateObj = IntGdiCreateBrushXlate(pDC, pbrush, &bFailed);
    BRUSH_UnlockBrush(pbrush);
    if(bFailed)
    {
        return NULL;
    }

    hOrgBrush = pdcattr->hbrush;
    pdcattr->hbrush = hBrush;

    if (pDC->rosdc.XlateBrush != NULL)
    {
        EngDeleteXlate(pDC->rosdc.XlateBrush);
    }
    pDC->rosdc.XlateBrush = XlateObj;

    pdcattr->ulDirty_ &= ~DC_BRUSH_DIRTY;

    return hOrgBrush;
}

HPEN
FASTCALL
IntGdiSelectPen(
    PDC pDC,
    HPEN hPen)
{
    PDC_ATTR pdcattr;
    HPEN hOrgPen = NULL;
    PBRUSH pbrushPen;
    XLATEOBJ *XlateObj;
    BOOLEAN bFailed;

    if (pDC == NULL || hPen == NULL) return NULL;

    pdcattr = pDC->pdcattr;

    pbrushPen = PEN_LockPen(hPen);
    if (pbrushPen == NULL)
    {
        return NULL;
    }

    XlateObj = IntGdiCreateBrushXlate(pDC, pbrushPen, &bFailed);
    PEN_UnlockPen(pbrushPen);
    if (bFailed)
    {
        SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
        return NULL;
    }

    hOrgPen = pdcattr->hpen;
    pdcattr->hpen = hPen;

    if (pDC->rosdc.XlatePen != NULL)
    {
        EngDeleteXlate(pDC->rosdc.XlatePen);
    }
    pdcattr->ulDirty_ &= ~DC_PEN_DIRTY;

    pDC->rosdc.XlatePen = XlateObj;

    return hOrgPen;
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

    hOrgBrush = IntGdiSelectBrush(pDC,hBrush);

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

    hOrgPen = IntGdiSelectPen(pDC, hPen);

    DC_UnlockDc(pDC);

    return hOrgPen;
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

    if (pdcattr->ulDirty_ & DC_BRUSH_DIRTY)
        IntGdiSelectBrush(pdc, pdcattr->hbrush);

    if (pdcattr->ulDirty_ & DC_PEN_DIRTY)
        IntGdiSelectPen(pdc, pdcattr->hpen);

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

