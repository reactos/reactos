/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          GDI BitBlt Functions
 * FILE:             subsys/win32k/eng/bitblt.c
 * PROGRAMER:        Jason Filby
 * REVISION HISTORY:
 *        2/10/1999: Created
 */

#include <ddk/winddi.h>
#include <ddk/ntddk.h>
#include <ntos/minmax.h>
#include "brush.h"
#include "enum.h"
#include "objects.h"
#include <include/mouse.h>
#include <include/object.h>
#include <include/dib.h>
#include <include/surface.h>
#include <include/copybits.h>

BOOL EngIntersectRect(PRECTL prcDst, PRECTL prcSrc1, PRECTL prcSrc2)
{
  static const RECTL rclEmpty = { 0, 0, 0, 0 };

  prcDst->left  = max(prcSrc1->left, prcSrc2->left);
  prcDst->right = min(prcSrc1->right, prcSrc2->right);

  if (prcDst->left < prcDst->right)
  {
    prcDst->top    = max(prcSrc1->top, prcSrc2->top);
    prcDst->bottom = min(prcSrc1->bottom, prcSrc2->bottom);

    if (prcDst->top < prcDst->bottom) return(TRUE);
  }

  *prcDst = rclEmpty;

  return(FALSE);
}

INT abs(INT nm);

BOOL EngBitBlt(SURFOBJ *Dest, SURFOBJ *Source,
               SURFOBJ *Mask, CLIPOBJ *ClipRegion,
               XLATEOBJ *ColorTranslation, RECTL *DestRect,
               POINTL *SourcePoint, POINTL *MaskRect,
               BRUSHOBJ *Brush, POINTL *BrushOrigin, ROP4 rop4)
{
  BOOLEAN   ret;
  BYTE      clippingType;
  RECTL     rclTmp;
  POINTL    ptlTmp;
  RECT_ENUM RectEnum;
  BOOL      EnumMore;
  PSURFGDI  DestGDI, SourceGDI;
  HSURF     hTemp;
  PSURFOBJ  TempSurf = NULL;
  BOOLEAN   canCopyBits;
  POINTL    TempPoint;
  RECTL     TempRect;
  SIZEL     TempSize;

  if(Source != NULL) SourceGDI = (PSURFGDI)AccessInternalObjectFromUserObject(Source);
  if(Dest   != NULL) DestGDI   = (PSURFGDI)AccessInternalObjectFromUserObject(Dest);

  MouseSafetyOnDrawStart(Source, SourceGDI, SourcePoint->x, SourcePoint->y,
                         (SourcePoint->x + abs(DestRect->right - DestRect->left)),
                         (SourcePoint->y + abs(DestRect->bottom - DestRect->top)));
  MouseSafetyOnDrawStart(Dest, DestGDI, DestRect->left, DestRect->top, DestRect->right, DestRect->bottom);

  // If we don't have to do anything special, we can punt to DrvCopyBits
  // if it exists
  if( (Mask == NULL)        && (MaskRect == NULL) && (Brush == NULL) &&
      (BrushOrigin == NULL) && (rop4 == 0) )
  {
    canCopyBits = TRUE;
  } else
    canCopyBits = FALSE;

  // Check for CopyBits or BitBlt hooks if one is not a GDI managed bitmap, IF:
  // * The destination bitmap is not managed by the GDI OR
  if(Dest->iType != STYPE_BITMAP)
  {
    // Destination surface is device managed
    if (DestGDI->BitBlt!=NULL)
    {
      if (Source!=NULL)
      {
        // Get the source into a format compatible surface
        TempPoint.x = 0;
        TempPoint.y = 0;
        TempRect.top    = 0;
        TempRect.left   = 0;
        TempRect.bottom = DestRect->bottom - DestRect->top;
        TempRect.right  = DestRect->right - DestRect->left;
        TempSize.cx = TempRect.right;
        TempSize.cy = TempRect.bottom;

        hTemp = EngCreateBitmap(TempSize,
                     DIB_GetDIBWidthBytes(DestRect->right - DestRect->left, BitsPerFormat(Dest->iBitmapFormat)),
                     Dest->iBitmapFormat, 0, NULL);
        TempSurf = (PSURFOBJ)AccessUserObject((ULONG)hTemp);

        // FIXME: Skip creating a TempSurf if we have the same BPP and palette
        EngBitBlt(TempSurf, Source, NULL, NULL, ColorTranslation, &TempRect, SourcePoint, NULL, NULL, NULL, 0);
      }

      ret = DestGDI->BitBlt(Dest, TempSurf, Mask, ClipRegion,
                            NULL, DestRect, &TempPoint,
                            MaskRect, Brush, BrushOrigin, rop4);

      MouseSafetyOnDrawEnd(Source, SourceGDI);
      MouseSafetyOnDrawEnd(Dest, DestGDI);

      return ret;
    }
  }

  // * The source bitmap is not managed by the GDI and we didn't already obtain it using EngCopyBits from the device
  if(Source->iType != STYPE_BITMAP && SourceGDI->CopyBits == NULL)
  {
    if (SourceGDI->BitBlt!=NULL)
    {
      // Request the device driver to return the bitmap in a format compatible with the device
      ret = SourceGDI->BitBlt(Dest, Source, Mask, ClipRegion,
                              NULL, DestRect, SourcePoint,
                              MaskRect, Brush, BrushOrigin, rop4);

      MouseSafetyOnDrawEnd(Source, SourceGDI);
      MouseSafetyOnDrawEnd(Dest, DestGDI);

      return ret;

      // Convert the surface from the driver into the required destination surface
    }
  }

  // Determine clipping type
  if (ClipRegion == (CLIPOBJ *) NULL)
  {
    clippingType = DC_TRIVIAL;
  } else {
    clippingType = ClipRegion->iDComplexity;
  }

  // We don't handle color translation just yet [we dont have to.. REMOVE REMOVE REMOVE]
  switch(clippingType)
  {
    case DC_TRIVIAL:
      CopyBitsCopy(Dest, Source, DestGDI, SourceGDI, DestRect, SourcePoint, Source->lDelta, ColorTranslation);

      MouseSafetyOnDrawEnd(Source, SourceGDI);
      MouseSafetyOnDrawEnd(Dest, DestGDI);

      return(TRUE);

    case DC_RECT:

      // Clip the blt to the clip rectangle
      EngIntersectRect(&rclTmp, DestRect, &ClipRegion->rclBounds);

      ptlTmp.x = SourcePoint->x + rclTmp.left - DestRect->left;
      ptlTmp.y = SourcePoint->y + rclTmp.top  - DestRect->top;

      MouseSafetyOnDrawEnd(Source, SourceGDI);
      MouseSafetyOnDrawEnd(Dest, DestGDI);

      return(TRUE);

    case DC_COMPLEX:

      CLIPOBJ_cEnumStart(ClipRegion, FALSE, CT_RECTANGLES, CD_ANY, ENUM_RECT_LIMIT);

      do {
        EnumMore = CLIPOBJ_bEnum(ClipRegion,(ULONG) sizeof(RectEnum), (PVOID) &RectEnum);

        if (RectEnum.c > 0)
        {
          RECTL* prclEnd = &RectEnum.arcl[RectEnum.c];
          RECTL* prcl    = &RectEnum.arcl[0];

          do {
            EngIntersectRect(prcl, prcl, DestRect);

            ptlTmp.x = SourcePoint->x + prcl->left - DestRect->left;
            ptlTmp.y = SourcePoint->y + prcl->top - DestRect->top;

            prcl++;

          } while (prcl < prclEnd);
        }

      } while(EnumMore);

    MouseSafetyOnDrawEnd(Source, SourceGDI);
    MouseSafetyOnDrawEnd(Dest, DestGDI);

    return(TRUE);
  }

  MouseSafetyOnDrawEnd(Source, SourceGDI);
  MouseSafetyOnDrawEnd(Dest, DestGDI);

  return(FALSE);
}
