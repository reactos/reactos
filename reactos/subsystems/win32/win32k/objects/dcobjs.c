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

HANDLE
APIENTRY
NtGdiGetDCObject(HDC  hDC, INT  ObjectType)
{
  HGDIOBJ SelObject;
  DC *dc;
  PDC_ATTR pdcattr;

  /* From Wine: GetCurrentObject does not SetLastError() on a null object */
  if(!hDC) return NULL;

  if(!(dc = DC_LockDc(hDC)))
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return NULL;
  }
  pdcattr = dc->pdcattr;

  if (pdcattr->ulDirty_ & DC_BRUSH_DIRTY)
     IntGdiSelectBrush(dc,pdcattr->hbrush);

  if (pdcattr->ulDirty_ & DC_PEN_DIRTY)
     IntGdiSelectPen(dc,pdcattr->hpen);

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
      SelObject = dc->dclevel.hpal;
      break;
    case GDI_OBJECT_TYPE_FONT:
      SelObject = pdcattr->hlfntNew;
      break;
    case GDI_OBJECT_TYPE_BITMAP:
      SelObject = dc->rosdc.hBitmap;
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

  DC_UnlockDc(dc);
  return SelObject;
}

HPALETTE 
FASTCALL 
GdiSelectPalette(HDC  hDC,
           HPALETTE  hpal,
    BOOL  ForceBackground)
{
    PDC dc;
    HPALETTE oldPal = NULL;
    PPALGDI PalGDI;

    // FIXME: mark the palette as a [fore\back]ground pal
    dc = DC_LockDc(hDC);
    if (!dc)
    {
        return NULL;
    }

    /* Check if this is a valid palette handle */
    PalGDI = PALETTE_LockPalette(hpal);
    if (!PalGDI)
    {
        DC_UnlockDc(dc);
        return NULL;
    }

    /* Is this a valid palette for this depth? */
    if ((dc->rosdc.bitsPerPixel <= 8 && PalGDI->Mode == PAL_INDEXED) ||
        (dc->rosdc.bitsPerPixel > 8  && PalGDI->Mode != PAL_INDEXED))
    {
        oldPal = dc->dclevel.hpal;
        dc->dclevel.hpal = hpal;
    }
    else if (8 < dc->rosdc.bitsPerPixel && PAL_INDEXED == PalGDI->Mode)
    {
        oldPal = dc->dclevel.hpal;
        dc->dclevel.hpal = hpal;
    }

    PALETTE_UnlockPalette(PalGDI);
    DC_UnlockDc(dc);

    return oldPal;
}
