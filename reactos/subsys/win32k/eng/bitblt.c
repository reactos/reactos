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
#include "clip.h"
#include "objects.h"
#include "../dib/dib.h"
#include <include/mouse.h>
#include <include/object.h>
#include <include/dib.h>
#include <include/surface.h>
#include <include/copybits.h>

#define NDEBUG
#include <win32k/debug1.h>

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

static BOOL STDCALL
BltMask(SURFOBJ *Dest, PSURFGDI DestGDI, SURFOBJ *Mask, 
	RECTL *DestRect, POINTL *MaskPoint, BRUSHOBJ* Brush,
	POINTL* BrushPoint)
{
  LONG i, j, dx, dy, c8;
  BYTE *tMask, *lMask;
  PFN_DIB_PutPixel DIB_PutPixel;
  static BYTE maskbit[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

  // Assign DIB functions according to bytes per pixel
  switch(BitsPerFormat(Dest->iBitmapFormat))
  {
    case 1:
      DIB_PutPixel = (PFN_DIB_PutPixel)DIB_1BPP_PutPixel;
      break;

    case 4:
      DIB_PutPixel = (PFN_DIB_PutPixel)DIB_4BPP_PutPixel;
      break;

    case 16:
      DIB_PutPixel = (PFN_DIB_PutPixel)DIB_16BPP_PutPixel;
      break;

    case 24:
      DIB_PutPixel = (PFN_DIB_PutPixel)DIB_24BPP_PutPixel;
      break;

    default:
      DbgPrint("BltMask: unsupported DIB format %u (bitsPerPixel:%u)\n", Dest->iBitmapFormat,
               BitsPerFormat(Dest->iBitmapFormat));
      return FALSE;
  }

  dx = DestRect->right  - DestRect->left;
  dy = DestRect->bottom - DestRect->top;

  if (Mask != NULL)
    {
      tMask = Mask->pvBits;
      for (j = 0; j < dy; j++)
	{
	  lMask = tMask;
	  c8 = 0;
	  for (i = 0; i < dx; i++)
	    {
	      if (0 != (*lMask & maskbit[c8]))
		{
		  DIB_PutPixel(Dest, DestRect->left + i, DestRect->top + j, Brush->iSolidColor);
		}
	      c8++;
	      if (8 == c8)
		{
		  lMask++;
		  c8=0;
		}
	    }
	  tMask += Mask->lDelta;
	}
      return TRUE;
    }
  else
    {
    return FALSE;
    }
}

static BOOL STDCALL
BltPatCopy(SURFOBJ *Dest, PSURFGDI DestGDI, SURFOBJ *Mask, 
	   RECTL *DestRect, POINTL *MaskPoint, BRUSHOBJ* Brush,
	   POINTL* BrushPoint)
{
  // These functions are assigned if we're working with a DIB
  // The assigned functions depend on the bitsPerPixel of the DIB
  PFN_DIB_HLine    DIB_HLine;
  LONG y;
  ULONG LineWidth;

  MouseSafetyOnDrawStart(Dest, DestGDI, DestRect->left, DestRect->top, DestRect->right, DestRect->bottom);
  // Assign DIB functions according to bytes per pixel
  DPRINT("BPF: %d\n", BitsPerFormat(Dest->iBitmapFormat));
  switch(BitsPerFormat(Dest->iBitmapFormat))
  {
    case 4:
      DIB_HLine    = (PFN_DIB_HLine)DIB_4BPP_HLine;
      break;

    case 16:
      DIB_HLine    = (PFN_DIB_HLine)DIB_16BPP_HLine;
      break;

    case 24:
      DIB_HLine    = (PFN_DIB_HLine)DIB_24BPP_HLine;
      break;

    default:
      DbgPrint("BltPatCopy: unsupported DIB format %u (bitsPerPixel:%u)\n", Dest->iBitmapFormat,
               BitsPerFormat(Dest->iBitmapFormat));

      MouseSafetyOnDrawEnd(Dest, DestGDI);
      return FALSE;
  }

  LineWidth  = DestRect->right - DestRect->left;
  for (y = DestRect->top; y < DestRect->bottom; y++)
  {
    DIB_HLine(Dest, DestRect->left, DestRect->right, y,  Brush->iSolidColor);
  }
  MouseSafetyOnDrawEnd(Dest, DestGDI);

  return TRUE;
}

INT abs(INT nm);

BOOL STDCALL
EngBitBlt(SURFOBJ *Dest,
	  SURFOBJ *Source,
	  SURFOBJ *Mask,
	  CLIPOBJ *ClipRegion,
	  XLATEOBJ *ColorTranslation,
	  RECTL *DestRect,
	  POINTL *SourcePoint,
	  POINTL *MaskOrigin,
	  BRUSHOBJ *Brush,
	  POINTL *BrushOrigin,
	  ROP4 rop4)
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

  if (Source != NULL)
    {
      MouseSafetyOnDrawStart(Source, SourceGDI, SourcePoint->x, SourcePoint->y,
			     (SourcePoint->x + abs(DestRect->right - DestRect->left)),
			     (SourcePoint->y + abs(DestRect->bottom - DestRect->top)));
    }
  MouseSafetyOnDrawStart(Dest, DestGDI, DestRect->left, DestRect->top, DestRect->right, DestRect->bottom);

  // If we don't have to do anything special, we can punt to DrvCopyBits
  // if it exists
  if( (Mask == NULL)        && (MaskOrigin == NULL) && (Brush == NULL) &&
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
                            MaskOrigin, Brush, BrushOrigin, rop4);

      MouseSafetyOnDrawEnd(Source, SourceGDI);
      MouseSafetyOnDrawEnd(Dest, DestGDI);

      return ret;
    }
  }

  /* The code currently assumes there will be a source bitmap. This is not true when, for example, using this function to
   * paint a brush pattern on the destination. */
  if(!Source && 0xaacc != rop4 && PATCOPY != rop4)
  {
    DbgPrint("EngBitBlt: A source is currently required, even though not all operations require one (FIXME)\n");
    return FALSE;
  }

  // * The source bitmap is not managed by the GDI and we didn't already obtain it using EngCopyBits from the device
  if(NULL != Source && STYPE_BITMAP != Source->iType && NULL == SourceGDI->CopyBits)
  {
    if (SourceGDI->BitBlt!=NULL)
    {
      // Request the device driver to return the bitmap in a format compatible with the device
      ret = SourceGDI->BitBlt(Dest, Source, Mask, ClipRegion,
                              NULL, DestRect, SourcePoint,
                              MaskOrigin, Brush, BrushOrigin, rop4);

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

  if (0xaacc == rop4)
  {
    return BltMask(Dest, DestGDI, Mask, DestRect, MaskOrigin, Brush, BrushOrigin);
  } else if (PATCOPY == rop4) {
    return BltPatCopy(Dest, DestGDI, Mask, DestRect, MaskOrigin, Brush, BrushOrigin);
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
