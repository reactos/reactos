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
/* $Id: bitblt.c,v 1.52 2004/04/25 11:34:13 weiden Exp $
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
#include <include/eng.h>
#include <include/inteng.h>

#define NDEBUG
#include <win32k/debug1.h>

typedef BOOLEAN STDCALL (*PBLTRECTFUNC)(SURFOBJ* OutputObj,
                                        SURFGDI* OutputGDI,
                                        SURFOBJ* InputObj,
                                        SURFGDI* InputGDI,
                                        SURFOBJ* Mask,
                                        XLATEOBJ* ColorTranslation,
                                        RECTL* OutputRect,
                                        POINTL* InputPoint,
                                        POINTL* MaskOrigin,
                                        BRUSHOBJ* Brush,
                                        POINTL* BrushOrigin,
                                        ROP4 Rop4);
typedef BOOLEAN STDCALL (*PSTRETCHRECTFUNC)(SURFOBJ* OutputObj,
                                            SURFGDI* OutputGDI,
                                            SURFOBJ* InputObj,
                                            SURFGDI* InputGDI,
                                            SURFOBJ* Mask,
                                            XLATEOBJ* ColorTranslation,
                                            RECTL* OutputRect,
                                            RECTL* InputRect,
                                            POINTL* MaskOrigin,
                                            POINTL* BrushOrigin,
                                            ULONG Mode);

BOOL STDCALL EngIntersectRect(RECTL* prcDst, RECTL* prcSrc1, RECTL* prcSrc2)
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
BltMask(SURFOBJ* Dest,
	SURFGDI* DestGDI,
	SURFOBJ* Source,
	SURFGDI* SourceGDI,
	SURFOBJ* Mask, 
	XLATEOBJ* ColorTranslation,
	RECTL* DestRect,
	POINTL* SourcePoint,
	POINTL* MaskPoint,
	BRUSHOBJ* Brush,
	POINTL* BrushPoint,
	ROP4 Rop4)
{
   LONG i, j, dx, dy, c8;
   BYTE *tMask, *lMask;
   static BYTE maskbit[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
   /* Pattern brushes */
   PGDIBRUSHOBJ GdiBrush;
   HBITMAP PatternSurface = NULL;
   SURFOBJ *PatternObj;
   ULONG PatternWidth, PatternHeight, PatternY;
  
   if (Mask == NULL)
   {
      return FALSE;
   }

   dx = DestRect->right  - DestRect->left;
   dy = DestRect->bottom - DestRect->top;

   if (Brush->iSolidColor == 0xFFFFFFFF)
   {
      PBITMAPOBJ PatternBitmap;

      GdiBrush = CONTAINING_RECORD(
         Brush,
         GDIBRUSHOBJ,
         BrushObject);

      PatternBitmap = BITMAPOBJ_LockBitmap(GdiBrush->hbmPattern);
      PatternSurface = BitmapToSurf(PatternBitmap, Dest->hdev);
      BITMAPOBJ_UnlockBitmap(GdiBrush->hbmPattern);

      PatternObj = (SURFOBJ*)AccessUserObject((ULONG)PatternSurface);
      PatternWidth = PatternObj->sizlBitmap.cx;
      PatternHeight = PatternObj->sizlBitmap.cy;
   }

   tMask = Mask->pvScan0 + SourcePoint->y * Mask->lDelta + (SourcePoint->x >> 3);
   for (j = 0; j < dy; j++)
   {
      lMask = tMask;
      c8 = SourcePoint->x & 0x07;
      
      if(PatternSurface)
         PatternY = (DestRect->top + j) % PatternHeight;
      
      for (i = 0; i < dx; i++)
      {
         if (0 != (*lMask & maskbit[c8]))
         {
            if (PatternSurface == NULL)
            {
               DestGDI->DIB_PutPixel(Dest, DestRect->left + i, DestRect->top + j, Brush->iSolidColor);
            }
            else
            {
               DestGDI->DIB_PutPixel(Dest, DestRect->left + i, DestRect->top + j,
                  DIB_1BPP_GetPixel(PatternObj, (DestRect->left + i) % PatternWidth, PatternY) ? GdiBrush->crFore : GdiBrush->crBack);
            }
         }
         c8++;
         if (8 == c8)
         {
            lMask++;
            c8 = 0;
         }
      }
      tMask += Mask->lDelta;
   }

   if (PatternSurface != NULL)
      EngDeleteSurface((HSURF)PatternSurface);

   return TRUE;
}

static BOOLEAN STDCALL
BltPatCopy(SURFOBJ* Dest,
	   SURFGDI* DestGDI,
	   SURFOBJ* Source,
	   SURFGDI* SourceGDI,
	   SURFOBJ* Mask, 
	   XLATEOBJ* ColorTranslation,
	   RECTL* DestRect,
	   POINTL* SourcePoint,
	   POINTL* MaskPoint,
	   BRUSHOBJ* Brush,
	   POINTL* BrushPoint,
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
CallDibBitBlt(SURFOBJ* OutputObj,
              SURFGDI* OutputGDI,
              SURFOBJ* InputObj,
              SURFGDI* InputGDI,
              SURFOBJ* Mask,
              XLATEOBJ* ColorTranslation,
              RECTL* OutputRect,
              POINTL* InputPoint,
              POINTL* MaskOrigin,
              BRUSHOBJ* Brush,
              POINTL* BrushOrigin,
              ROP4 Rop4)
{
  POINTL RealBrushOrigin;
  if (BrushOrigin == NULL)
    {
      RealBrushOrigin.x = RealBrushOrigin.y = 0;
    }
  else
    {
      RealBrushOrigin = *BrushOrigin;
    }
  return OutputGDI->DIB_BitBlt(OutputObj, InputObj, OutputGDI, InputGDI, OutputRect, InputPoint, Brush, RealBrushOrigin, ColorTranslation, Rop4);
}

INT abs(INT nm);

/*
 * @implemented
 */
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
  SURFGDI*           OutputGDI;
  SURFGDI*           InputGDI;
  POINTL             InputPoint;
  RECTL              InputRect;
  RECTL              OutputRect;
  POINTL             Translate;
  INTENG_ENTER_LEAVE EnterLeaveSource;
  INTENG_ENTER_LEAVE EnterLeaveDest;
  SURFOBJ*           InputObj;
  SURFOBJ*           OutputObj;
  PBLTRECTFUNC       BltRectFunc;
  BOOLEAN            Ret;
  RECTL              ClipRect;
  unsigned           i;
  POINTL             Pt;
  ULONG              Direction;
  BOOL               UsesSource;
  BOOL               UsesPattern;
  POINTL             AdjustedBrushOrigin;

  UsesSource = ((Rop4 & 0xCC0000) >> 2) != (Rop4 & 0x330000);
  UsesPattern = ((Rop4 & 0xF00000) >> 4) != (Rop4 & 0x0F0000);
  if (ROP_NOOP == Rop4)
    {
    /* Copy destination onto itself: nop */
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
    InputGDI = (SURFGDI*) AccessInternalObjectFromUserObject(InputObj);
    }
  else
    {
      InputGDI = NULL;
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
  
  #if 0
  if(BrushOrigin)
  {
    AdjustedBrushOrigin.x = BrushOrigin->x + Translate.x;
    AdjustedBrushOrigin.y = BrushOrigin->y + Translate.y;
  }
  else
    AdjustedBrushOrigin = Translate;
  #else
  AdjustedBrushOrigin = (BrushOrigin ? *BrushOrigin : Translate);
  #endif

  if (NULL != OutputObj)
    {
    OutputGDI = (SURFGDI*)AccessInternalObjectFromUserObject(OutputObj);
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
#if 0
      BltRectFunc = BltPatCopy;
#else
      if (Brush->iSolidColor == 0xFFFFFFFF)
        BltRectFunc = CallDibBitBlt;
      else
        BltRectFunc = BltPatCopy;
#endif
    }
  else
    {
      BltRectFunc = CallDibBitBlt;
    }


  switch(clippingType)
  {
    case DC_TRIVIAL:
      Ret = (*BltRectFunc)(OutputObj, OutputGDI, InputObj, InputGDI, Mask, ColorTranslation,
                           &OutputRect, &InputPoint, MaskOrigin, Brush, &AdjustedBrushOrigin, Rop4);
      break;
    case DC_RECT:
      // Clip the blt to the clip rectangle
      ClipRect.left = ClipRegion->rclBounds.left + Translate.x;
      ClipRect.right = ClipRegion->rclBounds.right + Translate.x;
      ClipRect.top = ClipRegion->rclBounds.top + Translate.y;
      ClipRect.bottom = ClipRegion->rclBounds.bottom + Translate.y;
      EngIntersectRect(&CombinedRect, &OutputRect, &ClipRect);
      Pt.x = InputPoint.x + CombinedRect.left - OutputRect.left;
      Pt.y = InputPoint.y + CombinedRect.top - OutputRect.top;
      Ret = (*BltRectFunc)(OutputObj, OutputGDI, InputObj, InputGDI, Mask, ColorTranslation,
                           &CombinedRect, &Pt, MaskOrigin, Brush, &AdjustedBrushOrigin, Rop4);
      break;
    case DC_COMPLEX:
      Ret = TRUE;
      if (OutputObj == InputObj)
	{
	  if (OutputRect.top < InputPoint.y)
	    {
	      Direction = OutputRect.left < InputPoint.x ? CD_RIGHTDOWN : CD_LEFTDOWN;
	    }
	  else
	    {
	      Direction = OutputRect.left < InputPoint.x ? CD_RIGHTUP : CD_LEFTUP;
	    }
	}
      else
	{
	  Direction = CD_ANY;
	}
      CLIPOBJ_cEnumStart(ClipRegion, FALSE, CT_RECTANGLES, Direction, 0);
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
	      Pt.x = InputPoint.x + CombinedRect.left - OutputRect.left;
	      Pt.y = InputPoint.y + CombinedRect.top - OutputRect.top;
	      Ret = (*BltRectFunc)(OutputObj, OutputGDI, InputObj, InputGDI, Mask, ColorTranslation,
	                           &CombinedRect, &Pt, MaskOrigin, Brush, &AdjustedBrushOrigin, Rop4) &&
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
  RECTL InputClippedRect;
  RECTL OutputRect;
  POINTL InputPoint;
  BOOLEAN UsesSource;

  InputClippedRect = *DestRect;
  if (InputClippedRect.right < InputClippedRect.left)
    {
      InputClippedRect.left = DestRect->right;
      InputClippedRect.right = DestRect->left;
    }
  if (InputClippedRect.bottom < InputClippedRect.top)
    {
      InputClippedRect.top = DestRect->bottom;
      InputClippedRect.bottom = DestRect->top;
    }
  UsesSource = ((Rop4 & 0xCC0000) >> 2) != (Rop4 & 0x330000);
  if (UsesSource)
    {
      if (NULL == SourcePoint || NULL == SourceObj)
        {
          return FALSE;
        }
      InputPoint = *SourcePoint;
      SourceGDI = (SURFGDI*) AccessInternalObjectFromUserObject(SourceObj);

      /* Make sure we don't try to copy anything outside the valid source region */
      if (InputPoint.x < 0)
        {
          InputClippedRect.left -= InputPoint.x;
          InputPoint.x = 0;
        }
      if (InputPoint.y < 0)
        {
          InputClippedRect.top -= InputPoint.y;
          InputPoint.y = 0;
        }
      if (SourceObj->sizlBitmap.cx < InputPoint.x + InputClippedRect.right - InputClippedRect.left)
        {
          InputClippedRect.right = InputClippedRect.left + SourceObj->sizlBitmap.cx - InputPoint.x;
        }
      if (SourceObj->sizlBitmap.cy < InputPoint.y + InputClippedRect.bottom - InputClippedRect.top)
        {
          InputClippedRect.bottom = InputClippedRect.top + SourceObj->sizlBitmap.cy - InputPoint.y;
        }

      if (InputClippedRect.right < InputClippedRect.left ||
          InputClippedRect.bottom < InputClippedRect.top)
        {
          /* Everything clipped away, nothing to do */
          return TRUE;
        }
    }

  /* Clip against the bounds of the clipping region so we won't try to write
   * outside the surface */
  if (NULL != ClipRegion)
    {
      if (! EngIntersectRect(&OutputRect, &InputClippedRect, &ClipRegion->rclBounds))
	{
	  return TRUE;
	}
      InputPoint.x += OutputRect.left - DestRect->left;
      InputPoint.y += OutputRect.top - DestRect->top;
    }
  else
    {
      OutputRect = InputClippedRect;
    }

  if (UsesSource)
    {
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
      IntLockGDIDriver(DestGDI);
      ret = DestGDI->BitBlt(DestObj, SourceObj, Mask, ClipRegion, ColorTranslation,
                            &OutputRect, &InputPoint, MaskOrigin, Brush, BrushOrigin,
                            Rop4);
      IntUnLockGDIDriver(DestGDI);
    }

  if (! ret)
    {
      IntLockGDIDriver(DestGDI);
      ret = EngBitBlt(DestObj, SourceObj, Mask, ClipRegion, ColorTranslation,
                      &OutputRect, &InputPoint, MaskOrigin, Brush, BrushOrigin,
                      Rop4);
      IntUnLockGDIDriver(DestGDI);
    }

  MouseSafetyOnDrawEnd(DestObj, DestGDI);
  if (UsesSource)
    {
    MouseSafetyOnDrawEnd(SourceObj, SourceGDI);
    }

  return ret;
}

static BOOLEAN STDCALL
CallDibStretchBlt(SURFOBJ* OutputObj,
                  SURFGDI* OutputGDI,
                  SURFOBJ* InputObj,
                  SURFGDI* InputGDI,
                  SURFOBJ* Mask,
                  XLATEOBJ* ColorTranslation,
                  RECTL* OutputRect,
                  RECTL* InputRect,
                  POINTL* MaskOrigin,
                  POINTL* BrushOrigin,
                  ULONG Mode)
{
  POINTL RealBrushOrigin;
  if (BrushOrigin == NULL)
    {
      RealBrushOrigin.x = RealBrushOrigin.y = 0;
    }
  else
    {
      RealBrushOrigin = *BrushOrigin;
    }
  return OutputGDI->DIB_StretchBlt(OutputObj, InputObj, OutputGDI, InputGDI, OutputRect, InputRect, MaskOrigin, RealBrushOrigin, ColorTranslation, Mode);
}


BOOL
STDCALL
EngStretchBlt(
	IN SURFOBJ  *DestObj,
	IN SURFOBJ  *SourceObj,
	IN SURFOBJ  *Mask,
	IN CLIPOBJ  *ClipRegion,
	IN XLATEOBJ  *ColorTranslation,
	IN COLORADJUSTMENT  *pca,
	IN POINTL  *BrushOrigin,
	IN RECTL  *prclDest,
	IN RECTL  *prclSrc,
	IN POINTL  *MaskOrigin,
	IN ULONG  Mode
	)
{
  // www.osr.com/ddk/graphics/gdifncs_0bs7.htm
  
  BYTE               clippingType;
  RECTL              CombinedRect;
//  RECT_ENUM          RectEnum;
//  BOOL               EnumMore;
  SURFGDI*           OutputGDI;
  SURFGDI*           InputGDI;
  POINTL             InputPoint;
  RECTL              InputRect;
  RECTL              OutputRect;
  POINTL             Translate;
  INTENG_ENTER_LEAVE EnterLeaveSource;
  INTENG_ENTER_LEAVE EnterLeaveDest;
  SURFOBJ*           InputObj;
  SURFOBJ*           OutputObj;
  PSTRETCHRECTFUNC       BltRectFunc;
  BOOLEAN            Ret;
  RECTL              ClipRect;
//  unsigned           i;
  POINTL             Pt;
//  ULONG              Direction;
  POINTL AdjustedBrushOrigin;

    InputRect.left = prclSrc->left;
    InputRect.right = prclSrc->right;
    InputRect.top = prclSrc->top;
    InputRect.bottom = prclSrc->bottom;

  if (! IntEngEnter(&EnterLeaveSource, SourceObj, &InputRect, TRUE, &Translate, &InputObj))
    {
    return FALSE;
    }

   InputPoint.x = InputRect.left + Translate.x;
   InputPoint.y = InputRect.top + Translate.y;
 
  if (NULL != InputObj)
    {
    InputGDI = (SURFGDI*) AccessInternalObjectFromUserObject(InputObj);
    }
  else
    {
      InputGDI = NULL;
    }

  OutputRect = *prclDest;
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

  OutputRect.left = prclDest->left + Translate.x;
  OutputRect.right = prclDest->right + Translate.x;
  OutputRect.top = prclDest->top + Translate.y;
  OutputRect.bottom = prclDest->bottom + Translate.y;
#if 0
  if(BrushOrigin)
  {
    AdjustedBrushOrigin.x = BrushOrigin->x + Translate.x;
    AdjustedBrushOrigin.y = BrushOrigin->y + Translate.y;
  }
  else
    AdjustedBrushOrigin = Translate;
#else
  AdjustedBrushOrigin = (BrushOrigin ? *BrushOrigin : Translate);
#endif

  if (NULL != OutputObj)
    {
    OutputGDI = (SURFGDI*)AccessInternalObjectFromUserObject(OutputObj);
    }

  // Determine clipping type
  if (ClipRegion == (CLIPOBJ *) NULL)
  {
    clippingType = DC_TRIVIAL;
  } else {
    clippingType = ClipRegion->iDComplexity;
  }

  if (Mask != NULL)//(0xaacc == Rop4)
    {
      //BltRectFunc = BltMask;
      DPRINT("EngStretchBlt isn't capable of handling mask yet.\n");
      IntEngLeave(&EnterLeaveDest);
      IntEngLeave(&EnterLeaveSource);
      
      return FALSE;      
    }
  else
    {
      BltRectFunc = CallDibStretchBlt;
    }


  switch(clippingType)
  {
    case DC_TRIVIAL:
      Ret = (*BltRectFunc)(OutputObj, OutputGDI, InputObj, InputGDI, Mask, ColorTranslation,
                          &OutputRect, &InputRect, MaskOrigin, &AdjustedBrushOrigin, Mode);
      break;
    case DC_RECT:
      // Clip the blt to the clip rectangle
      ClipRect.left = ClipRegion->rclBounds.left + Translate.x;
      ClipRect.right = ClipRegion->rclBounds.right + Translate.x;
      ClipRect.top = ClipRegion->rclBounds.top + Translate.y;
      ClipRect.bottom = ClipRegion->rclBounds.bottom + Translate.y;
      EngIntersectRect(&CombinedRect, &OutputRect, &ClipRect);
      Pt.x = InputPoint.x + CombinedRect.left - OutputRect.left;
      Pt.y = InputPoint.y + CombinedRect.top - OutputRect.top;
      Ret = (*BltRectFunc)(OutputObj, OutputGDI, InputObj, InputGDI, Mask, ColorTranslation,
                           &OutputRect, &InputRect, MaskOrigin, &AdjustedBrushOrigin, Mode);
      //Ret = (*BltRectFunc)(OutputObj, OutputGDI, InputObj, InputGDI, Mask, ColorTranslation,
      //                     &CombinedRect, &Pt, MaskOrigin, Brush, BrushOrigin, Rop4);
      DPRINT("EngStretchBlt() doesn't support DC_RECT clipping yet, so blitting w/o clip.\n");
      break;
      // TODO: Complex clipping
    /*
    case DC_COMPLEX:
      Ret = TRUE;
      if (OutputObj == InputObj)
	{
	  if (OutputRect.top < InputPoint.y)
	    {
	      Direction = OutputRect.left < InputPoint.x ? CD_RIGHTDOWN : CD_LEFTDOWN;
	    }
	  else
	    {
	      Direction = OutputRect.left < InputPoint.x ? CD_RIGHTUP : CD_LEFTUP;
	    }
	}
      else
	{
	  Direction = CD_ANY;
	}
      CLIPOBJ_cEnumStart(ClipRegion, FALSE, CT_RECTANGLES, Direction, 0);
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
	      Pt.x = InputPoint.x + CombinedRect.left - OutputRect.left;
	      Pt.y = InputPoint.y + CombinedRect.top - OutputRect.top;
	      Ret = (*BltRectFunc)(OutputObj, OutputGDI, InputObj, InputGDI, Mask, ColorTranslation,
	                           &CombinedRect, &Pt, MaskOrigin, Brush, &AdjustedBrushOrigin, Rop4) &&
	            Ret;
	    }
	}
      while(EnumMore);
      break;
      */
  }


  IntEngLeave(&EnterLeaveDest);
  IntEngLeave(&EnterLeaveSource);

  return Ret;
}

BOOL STDCALL
IntEngStretchBlt(SURFOBJ *DestObj,
             SURFOBJ *SourceObj,
             SURFOBJ *Mask,
             CLIPOBJ *ClipRegion,
             XLATEOBJ *ColorTranslation,
             RECTL *DestRect,
             RECTL *SourceRect,
             POINTL *pMaskOrigin,
             BRUSHOBJ *Brush,
             POINTL *BrushOrigin,
             ULONG Mode)
{
  BOOLEAN ret;
  SURFGDI *DestGDI;
  SURFGDI *SourceGDI;
  RECTL OutputRect;
  RECTL InputRect;
  COLORADJUSTMENT ca;
  POINT MaskOrigin;

  if (pMaskOrigin != NULL)
    {
      MaskOrigin.x = pMaskOrigin->x; MaskOrigin.y = pMaskOrigin->y;
    }

  if (NULL != SourceRect)
    {
      InputRect = *SourceRect;
    }

  // FIXME: Clipping is taken from IntEngBitBlt w/o modifications!
  
  /* Clip against the bounds of the clipping region so we won't try to write
   * outside the surface */
  if (NULL != ClipRegion)
    {
      if (! EngIntersectRect(&OutputRect, DestRect, &ClipRegion->rclBounds))
	{
	  return TRUE;
	}
	  DPRINT("Clipping isn't handled in IntEngStretchBlt() correctly yet\n");
      //InputPoint.x += OutputRect.left - DestRect->left;
      //InputPoint.y += OutputRect.top - DestRect->top;
    }
  else
    {
      OutputRect = *DestRect;
    }

  if (NULL != SourceObj)
    {
    SourceGDI = (SURFGDI*) AccessInternalObjectFromUserObject(SourceObj);
    MouseSafetyOnDrawStart(SourceObj, SourceGDI, InputRect.left, InputRect.top,
                           (InputRect.left + abs(InputRect.right - InputRect.left)),
			   (InputRect.top + abs(InputRect.bottom - InputRect.top)));
    }

  /* No success yet */
  ret = FALSE;
  DestGDI = (SURFGDI*)AccessInternalObjectFromUserObject(DestObj);
  MouseSafetyOnDrawStart(DestObj, DestGDI, OutputRect.left, OutputRect.top,
                         OutputRect.right, OutputRect.bottom);

  /* Prepare color adjustment */

  /* Call the driver's DrvStretchBlt if available */
  if (NULL != DestGDI->StretchBlt)
    {
      /* Drv->StretchBlt (look at http://www.osr.com/ddk/graphics/ddifncs_3ew7.htm )
      SURFOBJ *psoMask // optional, if it exists, then rop4=0xCCAA, otherwise rop4=0xCCCC */
      // FIXME: MaskOrigin is always NULL !
      IntLockGDIDriver(DestGDI);
      ret = DestGDI->StretchBlt(DestObj, SourceObj, Mask, ClipRegion, ColorTranslation,
                            &ca, BrushOrigin, &OutputRect, &InputRect, NULL, Mode);
      IntUnLockGDIDriver(DestGDI);
    }

  if (! ret)
    {
      // FIXME: see previous fixme
      ret = EngStretchBlt(DestObj, SourceObj, Mask, ClipRegion, ColorTranslation,
                          &ca, BrushOrigin, &OutputRect, &InputRect, NULL, Mode);
    }

  MouseSafetyOnDrawEnd(DestObj, DestGDI);
  if (NULL != SourceObj)
    {
    MouseSafetyOnDrawEnd(SourceObj, SourceGDI);
    }

  return ret;
}

/**** REACTOS FONT RENDERING CODE *********************************************/

/* renders the alpha mask bitmap */
static BOOLEAN STDCALL
AlphaBltMask(SURFOBJ* Dest,
	SURFGDI* DestGDI,
	SURFOBJ* Source,
	SURFGDI* SourceGDI,
	SURFOBJ* Mask, 
	XLATEOBJ* ColorTranslation,
	XLATEOBJ* SrcColorTranslation,
	RECTL* DestRect,
	POINTL* SourcePoint,
	POINTL* MaskPoint,
	BRUSHOBJ* Brush,
	POINTL* BrushPoint)
{
  LONG i, j, dx, dy;
  int r, g, b;
  ULONG Background, BrushColor, NewColor;
  BYTE *tMask, *lMask;

  dx = DestRect->right  - DestRect->left;
  dy = DestRect->bottom - DestRect->top;

  if (Mask != NULL)
    {
      BrushColor = XLATEOBJ_iXlate(SrcColorTranslation, Brush->iSolidColor);
      r = (int)GetRValue(BrushColor);
      g = (int)GetGValue(BrushColor);
      b = (int)GetBValue(BrushColor);
      
      tMask = Mask->pvBits + (SourcePoint->y * Mask->lDelta) + SourcePoint->x;
      for (j = 0; j < dy; j++)
	{
	  lMask = tMask;
	  for (i = 0; i < dx; i++)
	    {
	      if (*lMask > 0)
		{
			if(*lMask == 0xff)
			{
				DestGDI->DIB_PutPixel(Dest, DestRect->left + i, DestRect->top + j, Brush->iSolidColor);
			}
			else
			{
				Background = DIB_GetSource(Dest, DestGDI, DestRect->left + i, DestRect->top + j, SrcColorTranslation);

				NewColor = 
				     RGB((*lMask * (r - GetRValue(Background)) >> 8) + GetRValue(Background),
				         (*lMask * (g - GetGValue(Background)) >> 8) + GetGValue(Background),
				         (*lMask * (b - GetBValue(Background)) >> 8) + GetBValue(Background));
				
				Background = XLATEOBJ_iXlate(ColorTranslation, NewColor);
				DestGDI->DIB_PutPixel(Dest, DestRect->left + i, DestRect->top + j, Background);
			}
		}
		  lMask++;
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

BOOL STDCALL
EngMaskBitBlt(SURFOBJ *DestObj,
	  SURFOBJ *Mask,
	  CLIPOBJ *ClipRegion,
	  XLATEOBJ *DestColorTranslation,
	  XLATEOBJ *SourceColorTranslation,
	  RECTL *DestRect,
	  POINTL *SourcePoint,
	  POINTL *MaskOrigin,
	  BRUSHOBJ *Brush,
	  POINTL *BrushOrigin)
{
  BYTE               clippingType;
  RECTL              CombinedRect;
  RECT_ENUM          RectEnum;
  BOOL               EnumMore;
  SURFGDI*           OutputGDI;
  SURFGDI*           InputGDI;
  POINTL             InputPoint;
  RECTL              InputRect;
  RECTL              OutputRect;
  POINTL             Translate;
  INTENG_ENTER_LEAVE EnterLeaveSource;
  INTENG_ENTER_LEAVE EnterLeaveDest;
  SURFOBJ*           InputObj;
  SURFOBJ*           OutputObj;
  BOOLEAN            Ret;
  RECTL              ClipRect;
  unsigned           i;
  POINTL             Pt;
  ULONG              Direction;
  SURFGDI*           DestGDI;
  POINTL             AdjustedBrushOrigin;

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

  DestGDI = (SURFGDI*)AccessInternalObjectFromUserObject(DestObj);
  IntLockGDIDriver(DestGDI);
  if (! IntEngEnter(&EnterLeaveSource, NULL, &InputRect, TRUE, &Translate, &InputObj))
    {
    IntUnLockGDIDriver(DestGDI);
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
    InputGDI = (SURFGDI*) AccessInternalObjectFromUserObject(InputObj);
    }
  else
    {
      InputGDI = NULL;
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
    IntUnLockGDIDriver(DestGDI);
    return TRUE;
    }

  if (! IntEngEnter(&EnterLeaveDest, DestObj, &OutputRect, FALSE, &Translate, &OutputObj))
    {
    IntEngLeave(&EnterLeaveSource);
    IntUnLockGDIDriver(DestGDI);
    return FALSE;
    }

  OutputRect.left = DestRect->left + Translate.x;
  OutputRect.right = DestRect->right + Translate.x;
  OutputRect.top = DestRect->top + Translate.y;
  OutputRect.bottom = DestRect->bottom + Translate.y;

#if 0
  if(BrushOrigin)
  {
    AdjustedBrushOrigin.x = BrushOrigin->x + Translate.x;
    AdjustedBrushOrigin.y = BrushOrigin->y + Translate.y;
  }
  else
    AdjustedBrushOrigin = Translate;
#else
  AdjustedBrushOrigin = (BrushOrigin ? *BrushOrigin : Translate);
#endif

  if (NULL != OutputObj)
    {
    OutputGDI = (SURFGDI*)AccessInternalObjectFromUserObject(OutputObj);
    }

  // Determine clipping type
  if (ClipRegion == (CLIPOBJ *) NULL)
  {
    clippingType = DC_TRIVIAL;
  } else {
    clippingType = ClipRegion->iDComplexity;
  }

  switch(clippingType)
  {
    case DC_TRIVIAL:
      if(Mask->iBitmapFormat == BMF_8BPP)
        Ret = AlphaBltMask(OutputObj, OutputGDI, InputObj, InputGDI, Mask, DestColorTranslation, SourceColorTranslation,
                           &OutputRect, &InputPoint, MaskOrigin, Brush, &AdjustedBrushOrigin);
      else
        Ret = BltMask(OutputObj, OutputGDI, InputObj, InputGDI, Mask, DestColorTranslation,
                           &OutputRect, &InputPoint, MaskOrigin, Brush, &AdjustedBrushOrigin, 0xAACC);
      break;
    case DC_RECT:
      // Clip the blt to the clip rectangle
      ClipRect.left = ClipRegion->rclBounds.left + Translate.x;
      ClipRect.right = ClipRegion->rclBounds.right + Translate.x;
      ClipRect.top = ClipRegion->rclBounds.top + Translate.y;
      ClipRect.bottom = ClipRegion->rclBounds.bottom + Translate.y;
      EngIntersectRect(&CombinedRect, &OutputRect, &ClipRect);
      Pt.x = InputPoint.x + CombinedRect.left - OutputRect.left;
      Pt.y = InputPoint.y + CombinedRect.top - OutputRect.top;
      if(Mask->iBitmapFormat == BMF_8BPP)
        Ret = AlphaBltMask(OutputObj, OutputGDI, InputObj, InputGDI, Mask, DestColorTranslation, SourceColorTranslation,
                           &CombinedRect, &Pt, MaskOrigin, Brush, &AdjustedBrushOrigin);
      else
        Ret = BltMask(OutputObj, OutputGDI, InputObj, InputGDI, Mask, DestColorTranslation,
                           &CombinedRect, &Pt, MaskOrigin, Brush, &AdjustedBrushOrigin, 0xAACC);
      break;
    case DC_COMPLEX:
      Ret = TRUE;
      if (OutputObj == InputObj)
	{
	  if (OutputRect.top < InputPoint.y)
	    {
	      Direction = OutputRect.left < InputPoint.x ? CD_RIGHTDOWN : CD_LEFTDOWN;
	    }
	  else
	    {
	      Direction = OutputRect.left < InputPoint.x ? CD_RIGHTUP : CD_LEFTUP;
	    }
	}
      else
	{
	  Direction = CD_ANY;
	}
      CLIPOBJ_cEnumStart(ClipRegion, FALSE, CT_RECTANGLES, Direction, 0);
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
	      Pt.x = InputPoint.x + CombinedRect.left - OutputRect.left;
	      Pt.y = InputPoint.y + CombinedRect.top - OutputRect.top;
	      if(Mask->iBitmapFormat == BMF_8BPP)
	        Ret = AlphaBltMask(OutputObj, OutputGDI, InputObj, InputGDI, Mask, DestColorTranslation, SourceColorTranslation,
	                           &CombinedRect, &Pt, MaskOrigin, Brush, &AdjustedBrushOrigin) && Ret;
              else
                Ret = BltMask(OutputObj, OutputGDI, InputObj, InputGDI, Mask, DestColorTranslation,
                                   &CombinedRect, &Pt, MaskOrigin, Brush, &AdjustedBrushOrigin, 0xAACC) && Ret;
	    }
	}
      while(EnumMore);
      break;
  }


  IntEngLeave(&EnterLeaveDest);
  IntEngLeave(&EnterLeaveSource);

  IntUnLockGDIDriver(DestGDI);

  /* Dummy BitBlt to let driver know that something has changed.
     0x00AA0029 is the Rop for D (no-op) */
  IntEngBitBlt(DestObj, NULL, Mask, ClipRegion, DestColorTranslation,
               DestRect, SourcePoint, MaskOrigin, Brush, BrushOrigin, ROP_NOOP);

  return Ret;
}

BOOL STDCALL
IntEngMaskBlt(SURFOBJ *DestObj,
             SURFOBJ *Mask,
             CLIPOBJ *ClipRegion,
             XLATEOBJ *DestColorTranslation,
             XLATEOBJ *SourceColorTranslation,
             RECTL *DestRect,
             POINTL *SourcePoint,
             POINTL *MaskOrigin,
             BRUSHOBJ *Brush,
             POINTL *BrushOrigin)
{
  BOOLEAN ret;
  SURFGDI *DestGDI;
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

  /* No success yet */
  ret = FALSE;
  DestGDI = (SURFGDI*)AccessInternalObjectFromUserObject(DestObj);
  MouseSafetyOnDrawStart(DestObj, DestGDI, OutputRect.left, OutputRect.top,
                         OutputRect.right, OutputRect.bottom);

  ret = EngMaskBitBlt(DestObj, Mask, ClipRegion, DestColorTranslation, SourceColorTranslation,
                      &OutputRect, &InputPoint, MaskOrigin, Brush, BrushOrigin);

  MouseSafetyOnDrawEnd(DestObj, DestGDI);

  return ret;
}
/* EOF */
