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
#include "misc.h"
#include <include/mouse.h>
#include <include/object.h>
#include <include/dib.h>
#include <include/surface.h>
#include <include/copybits.h>
#include <include/inteng.h>

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
  static BYTE maskbit[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

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
		  DestGDI->DIB_PutPixel(Dest, DestRect->left + i, DestRect->top + j, Brush->iSolidColor);
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
  LONG y;
  ULONG LineWidth;

  LineWidth  = DestRect->right - DestRect->left;
  for (y = DestRect->top; y < DestRect->bottom; y++)
  {
    DestGDI->DIB_HLine(Dest, DestRect->left, DestRect->right, y,  Brush->iSolidColor);
  }

  return TRUE;
}

INT abs(INT nm);

BOOL STDCALL
EngBitBlt(SURFOBJ *DestObj,
	  SURFOBJ *SourceObj,
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
  BOOLEAN            ret;
  BYTE               clippingType;
  RECTL              rclTmp;
  POINTL             ptlTmp;
  RECT_ENUM          RectEnum;
  BOOL               EnumMore;
  PSURFGDI           OutputGDI, InputGDI;
  POINTL             InputPoint;
  RECTL              InputRect;
  RECTL              OutputRect;
  POINTL             Translate;
  INTENG_ENTER_LEAVE EnterLeaveSource;
  INTENG_ENTER_LEAVE EnterLeaveDest;
  PSURFOBJ           InputObj;
  PSURFOBJ           OutputObj;

  /* Check for degenerate case: if height or width of DestRect is 0 pixels there's
     nothing to do */
  if (DestRect->right == DestRect->left || DestRect->bottom == DestRect->top)
    {
    return TRUE;
    }

  if (NULL != SourcePoint)
    {
    InputRect.left = SourcePoint->x;
    InputRect.right = SourcePoint->x + (DestRect->right - DestRect->left);
    InputRect.top = SourcePoint->y;
    InputRect.bottom = SourcePoint->y + (DestRect->bottom - DestRect->top);
    }
  else
    {
    InputRect.left = 0;
    InputRect.right = DestRect->right - DestRect->left;
    InputRect.top = 0;
    InputRect.bottom = DestRect->bottom - DestRect->top;
    }

  if (! IntEngEnter(&EnterLeaveSource, SourceObj, &InputRect, TRUE, &Translate, &InputObj))
    {
    return FALSE;
    }

  if (NULL != SourcePoint)
    {
    InputPoint.x = SourcePoint->x + Translate.x;
    InputPoint.y = SourcePoint->y + Translate.y;
    }
  else
    {
    InputPoint.x = 0;
    InputPoint.y = 0;
    }

  if (NULL != InputObj)
    {
    InputGDI = (PSURFGDI) AccessInternalObjectFromUserObject(InputObj);
    }

  OutputRect = *DestRect;

  if (! IntEngEnter(&EnterLeaveDest, DestObj, &OutputRect, FALSE, &Translate, &OutputObj))
    {
    IntEngLeave(&EnterLeaveSource);
    return FALSE;
    }

  OutputRect.left = DestRect->left + Translate.x;
  OutputRect.right = DestRect->right + Translate.x;
  OutputRect.top = DestRect->top + Translate.y;
  OutputRect.bottom = DestRect->bottom + Translate.y;


  if (NULL != OutputObj)
    {
    OutputGDI = (PSURFGDI)AccessInternalObjectFromUserObject(OutputObj);
    }

  /* The code currently assumes there will be a source bitmap. This is not true when, for example, using this function to
   * paint a brush pattern on the destination. */
  if (NULL == InputObj && 0xaacc != rop4 && PATCOPY != rop4)
  {
    DbgPrint("EngBitBlt: A source is currently required, even though not all operations require one (FIXME)\n");
    return FALSE;
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
    ret = BltMask(OutputObj, OutputGDI, Mask, &OutputRect, MaskOrigin, Brush, BrushOrigin);
    IntEngLeave(&EnterLeaveDest);
    IntEngLeave(&EnterLeaveSource);
    return ret;
  } else if (PATCOPY == rop4) {
    ret = BltPatCopy(OutputObj, OutputGDI, Mask, &OutputRect, MaskOrigin, Brush, BrushOrigin);
    IntEngLeave(&EnterLeaveDest);
    IntEngLeave(&EnterLeaveSource);
    return ret;
  }


  // We don't handle color translation just yet [we dont have to.. REMOVE REMOVE REMOVE]
  switch(clippingType)
  {
    case DC_TRIVIAL:
      OutputGDI->DIB_BitBlt(OutputObj, InputObj, OutputGDI, InputGDI, &OutputRect, &InputPoint, ColorTranslation);

      IntEngLeave(&EnterLeaveDest);
      IntEngLeave(&EnterLeaveSource);

      return(TRUE);

    case DC_RECT:

      // Clip the blt to the clip rectangle
      EngIntersectRect(&rclTmp, &OutputRect, &ClipRegion->rclBounds);

      ptlTmp.x = InputPoint.x + rclTmp.left - OutputRect.left;
      ptlTmp.y = InputPoint.y + rclTmp.top  - OutputRect.top;

      IntEngLeave(&EnterLeaveDest);
      IntEngLeave(&EnterLeaveSource);

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
            EngIntersectRect(prcl, prcl, &OutputRect);

            ptlTmp.x = InputPoint.x + prcl->left - OutputRect.left;
            ptlTmp.y = InputPoint.y + prcl->top - OutputRect.top;

            prcl++;

          } while (prcl < prclEnd);
        }

      } while(EnumMore);

    IntEngLeave(&EnterLeaveDest);
    IntEngLeave(&EnterLeaveSource);

    return(TRUE);
  }

  IntEngLeave(&EnterLeaveDest);
  IntEngLeave(&EnterLeaveSource);

  return(FALSE);
}

BOOL STDCALL
IntEngBitBlt(SURFOBJ *DestObj,
             SURFOBJ *SourceObj,
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
  BOOLEAN ret;
  SURFGDI *DestGDI;
  SURFGDI *SourceGDI;

  if (NULL != SourceObj)
    {
    SourceGDI = (PSURFGDI) AccessInternalObjectFromUserObject(SourceObj);
    MouseSafetyOnDrawStart(SourceObj, SourceGDI, SourcePoint->x, SourcePoint->y,
                           (SourcePoint->x + abs(DestRect->right - DestRect->left)),
			   (SourcePoint->y + abs(DestRect->bottom - DestRect->top)));
    }

  /* No success yet */
  ret = FALSE;
  DestGDI = (SURFGDI*)AccessInternalObjectFromUserObject(DestObj);
  MouseSafetyOnDrawStart(DestObj, DestGDI, DestRect->left, DestRect->top,
                         DestRect->right, DestRect->bottom);

  /* Call the driver's DrvBitBlt if available */
  if (NULL != DestGDI->BitBlt) {
    ret = DestGDI->BitBlt(DestObj, SourceObj, Mask, ClipRegion, ColorTranslation,
                          DestRect, SourcePoint, MaskOrigin, Brush, BrushOrigin, rop4);
  }

  if (! ret) {
    ret = EngBitBlt(DestObj, SourceObj, Mask, ClipRegion, ColorTranslation,
                    DestRect, SourcePoint, MaskOrigin, Brush, BrushOrigin, rop4);
  }

  MouseSafetyOnDrawEnd(DestObj, DestGDI);
  if (NULL != SourceObj)
    {
    MouseSafetyOnDrawEnd(SourceObj, SourceGDI);
    }

  return ret;
}
