/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: bitblt.c,v 1.22 2003/07/09 07:00:00 gvg Exp $
 *
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
#include <ddk/ntddmou.h>
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

typedef BOOLEAN STDCALL (*PBLTRECTFUNC)(PSURFOBJ OutputObj,
                                        PSURFGDI OutputGDI,
                                        PSURFOBJ InputObj,
                                        PSURFGDI InputGDI,
                                        PSURFOBJ Mask,
                                        PXLATEOBJ ColorTranslation,
                                        PRECTL OutputRect,
                                        PPOINTL InputPoint,
                                        PPOINTL MaskOrigin,
                                        PBRUSHOBJ Brush,
                                        PPOINTL BrushOrigin,
                                        ROP4 Rop4);

BOOL STDCALL EngIntersectRect(PRECTL prcDst, PRECTL prcSrc1, PRECTL prcSrc2)
{
  static const RECTL rclEmpty = { 0, 0, 0, 0 };

  prcDst->left  = max(prcSrc1->left, prcSrc2->left);
  prcDst->right = min(prcSrc1->right, prcSrc2->right);

  if (prcDst->left < prcDst->right)
    {
      prcDst->top    = max(prcSrc1->top, prcSrc2->top);
      prcDst->bottom = min(prcSrc1->bottom, prcSrc2->bottom);

      if (prcDst->top < prcDst->bottom)
	{
	  return TRUE;
	}
    }

  *prcDst = rclEmpty;

  return FALSE;
}

static BOOLEAN STDCALL
BltMask(PSURFOBJ Dest,
        PSURFGDI DestGDI,
        PSURFOBJ Source,
        PSURFGDI SourceGDI,
        PSURFOBJ Mask, 
        PXLATEOBJ ColorTranslation,
	PRECTL DestRect,
        PPOINTL SourcePoint,
        PPOINTL MaskPoint,
        PBRUSHOBJ Brush,
	PPOINTL BrushPoint,
        ROP4 Rop4)
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

static BOOLEAN STDCALL
BltPatCopy(PSURFOBJ Dest,
           PSURFGDI DestGDI,
           PSURFOBJ Source,
           PSURFGDI SourceGDI,
           PSURFOBJ Mask, 
           PXLATEOBJ ColorTranslation,
	   PRECTL DestRect,
           PPOINTL SourcePoint,
           PPOINTL MaskPoint,
           PBRUSHOBJ Brush,
	   PPOINTL BrushPoint,
           ROP4 Rop4)
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

static BOOLEAN STDCALL
CallDibBitBlt(PSURFOBJ OutputObj,
              PSURFGDI OutputGDI,
              PSURFOBJ InputObj,
              PSURFGDI InputGDI,
              PSURFOBJ Mask,
              PXLATEOBJ ColorTranslation,
              PRECTL OutputRect,
              PPOINTL InputPoint,
              PPOINTL MaskOrigin,
              PBRUSHOBJ Brush,
              PPOINTL BrushOrigin,
              ROP4 Rop4)
{
  return OutputGDI->DIB_BitBlt(OutputObj, InputObj, OutputGDI, InputGDI, OutputRect, InputPoint, ColorTranslation);
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
	  ROP4 Rop4)
{
  BYTE               clippingType;
  RECTL              CombinedRect;
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
  PBLTRECTFUNC       BltRectFunc;
  BOOLEAN            Ret;
  RECTL              ClipRect;
  unsigned           i;

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
  if (NULL != ClipRegion)
    {
      if (OutputRect.left < ClipRegion->rclBounds.left)
	{
	  InputRect.left += ClipRegion->rclBounds.left - OutputRect.left;
	  InputPoint.x += ClipRegion->rclBounds.left - OutputRect.left;
	  OutputRect.left = ClipRegion->rclBounds.left;
	}
      if (ClipRegion->rclBounds.right < OutputRect.right)
	{
	  InputRect.right -=  OutputRect.right - ClipRegion->rclBounds.right;
	  OutputRect.right = ClipRegion->rclBounds.right;
	}
      if (OutputRect.top < ClipRegion->rclBounds.top)
	{
	  InputRect.top += ClipRegion->rclBounds.top - OutputRect.top;
	  InputPoint.y += ClipRegion->rclBounds.top - OutputRect.top;
	  OutputRect.top = ClipRegion->rclBounds.top;
	}
      if (ClipRegion->rclBounds.bottom < OutputRect.bottom)
	{
	  InputRect.bottom -=  OutputRect.bottom - ClipRegion->rclBounds.bottom;
	  OutputRect.bottom = ClipRegion->rclBounds.bottom;
	}
    }

  /* Check for degenerate case: if height or width of OutputRect is 0 pixels there's
     nothing to do */
  if (OutputRect.right <= OutputRect.left || OutputRect.bottom <= OutputRect.top)
    {
    IntEngLeave(&EnterLeaveSource);
    return TRUE;
    }

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
  if (NULL == InputObj && 0xaacc != Rop4 && PATCOPY != Rop4)
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

  if (0xaacc == Rop4)
    {
      BltRectFunc = BltMask;
    }
  else if (PATCOPY == Rop4)
    {
      BltRectFunc = BltPatCopy;
    }
  else
    {
      BltRectFunc = CallDibBitBlt;
    }


  switch(clippingType)
  {
    case DC_TRIVIAL:
      Ret = (*BltRectFunc)(OutputObj, OutputGDI, InputObj, InputGDI, Mask, ColorTranslation,
                           &OutputRect, &InputPoint, MaskOrigin, Brush, BrushOrigin, Rop4);
      break;
    case DC_RECT:
      // Clip the blt to the clip rectangle
      ClipRect.left = ClipRegion->rclBounds.left + Translate.x;
      ClipRect.right = ClipRegion->rclBounds.right + Translate.x;
      ClipRect.top = ClipRegion->rclBounds.top + Translate.y;
      ClipRect.bottom = ClipRegion->rclBounds.bottom + Translate.y;
      EngIntersectRect(&CombinedRect, &OutputRect, &ClipRect);
      Ret = (*BltRectFunc)(OutputObj, OutputGDI, InputObj, InputGDI, Mask, ColorTranslation,
                           &CombinedRect, &InputPoint, MaskOrigin, Brush, BrushOrigin, Rop4);
      break;
    case DC_COMPLEX:
      Ret = TRUE;
      CLIPOBJ_cEnumStart(ClipRegion, FALSE, CT_RECTANGLES, CD_ANY, ENUM_RECT_LIMIT);
      do
	{
	  EnumMore = CLIPOBJ_bEnum(ClipRegion,(ULONG) sizeof(RectEnum), (PVOID) &RectEnum);

	  for (i = 0; i < RectEnum.c; i++)
	    {
	      ClipRect.left = RectEnum.arcl[i].left + Translate.x;
	      ClipRect.right = RectEnum.arcl[i].right + Translate.x;
	      ClipRect.top = RectEnum.arcl[i].top + Translate.y;
	      ClipRect.bottom = RectEnum.arcl[i].bottom + Translate.y;
	      EngIntersectRect(&CombinedRect, &OutputRect, &ClipRect);
	      Ret = (*BltRectFunc)(OutputObj, OutputGDI, InputObj, InputGDI, Mask, ColorTranslation,
	                           &CombinedRect, &InputPoint, MaskOrigin, Brush, BrushOrigin, Rop4) &&
	            Ret;
	    }
	}
      while(EnumMore);
      break;
  }


  IntEngLeave(&EnterLeaveDest);
  IntEngLeave(&EnterLeaveSource);

  return Ret;
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
             ROP4 Rop4)
{
  BOOLEAN ret;
  SURFGDI *DestGDI;
  SURFGDI *SourceGDI;
  RECTL OutputRect;
  POINTL InputPoint;

  if (NULL != SourcePoint)
    {
      InputPoint = *SourcePoint;
    }

  /* Clip against the bounds of the clipping region so we won't try to write
   * outside the surface */
  if (NULL != ClipRegion)
    {
      if (! EngIntersectRect(&OutputRect, DestRect, &ClipRegion->rclBounds))
	{
	  return TRUE;
	}
      InputPoint.x += OutputRect.left - DestRect->left;
      InputPoint.y += OutputRect.top - DestRect->top;
    }
  else
    {
      OutputRect = *DestRect;
    }

  if (NULL != SourceObj)
    {
    SourceGDI = (PSURFGDI) AccessInternalObjectFromUserObject(SourceObj);
    MouseSafetyOnDrawStart(SourceObj, SourceGDI, InputPoint.x, InputPoint.y,
                           (InputPoint.x + abs(DestRect->right - DestRect->left)),
			   (InputPoint.y + abs(DestRect->bottom - DestRect->top)));
    }

  /* No success yet */
  ret = FALSE;
  DestGDI = (SURFGDI*)AccessInternalObjectFromUserObject(DestObj);
  MouseSafetyOnDrawStart(DestObj, DestGDI, OutputRect.left, OutputRect.top,
                         OutputRect.right, OutputRect.bottom);

  /* Call the driver's DrvBitBlt if available */
  if (NULL != DestGDI->BitBlt)
    {
      ret = DestGDI->BitBlt(DestObj, SourceObj, Mask, ClipRegion, ColorTranslation,
                            &OutputRect, &InputPoint, MaskOrigin, Brush, BrushOrigin,
                            Rop4);
    }

  if (! ret)
    {
      ret = EngBitBlt(DestObj, SourceObj, Mask, ClipRegion, ColorTranslation,
                      &OutputRect, &InputPoint, MaskOrigin, Brush, BrushOrigin,
                      Rop4);
    }

  MouseSafetyOnDrawEnd(DestObj, DestGDI);
  if (NULL != SourceObj)
    {
    MouseSafetyOnDrawEnd(SourceObj, SourceGDI);
    }

  return ret;
}
/* EOF */
