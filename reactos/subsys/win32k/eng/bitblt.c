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
/* $Id: bitblt.c,v 1.56 2004/07/03 13:55:35 navaraf Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          GDI BitBlt Functions
 * FILE:             subsys/win32k/eng/bitblt.c
 * PROGRAMER:        Jason Filby
 * REVISION HISTORY:
 *        2/10/1999: Created
 */
#include <w32k.h>

typedef BOOLEAN STDCALL (*PBLTRECTFUNC)(SURFOBJ* OutputObj,
                                        SURFOBJ* InputObj,
                                        SURFOBJ* Mask,
                                        XLATEOBJ* ColorTranslation,
                                        RECTL* OutputRect,
                                        POINTL* InputPoint,
                                        POINTL* MaskOrigin,
                                        BRUSHOBJ* Brush,
                                        POINTL* BrushOrigin,
                                        ROP4 Rop4);
typedef BOOLEAN STDCALL (*PSTRETCHRECTFUNC)(SURFOBJ* OutputObj,
                                            SURFOBJ* InputObj,
                                            SURFOBJ* Mask,
                                            CLIPOBJ* ClipRegion,
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
	SURFOBJ* Source,
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

      PatternSurface = GdiBrush->hbmPattern;
      PatternBitmap = BITMAPOBJ_LockBitmap(GdiBrush->hbmPattern);

      PatternObj = &PatternBitmap->SurfObj;
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
               DibFunctionsForBitmapFormat[Dest->iBitmapFormat].DIB_PutPixel(
                  Dest, DestRect->left + i, DestRect->top + j, Brush->iSolidColor);
            }
            else
            {
               DibFunctionsForBitmapFormat[Dest->iBitmapFormat].DIB_PutPixel(
                  Dest, DestRect->left + i, DestRect->top + j,
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
      BITMAPOBJ_UnlockBitmap(PatternSurface);

   return TRUE;
}

static BOOLEAN STDCALL
BltPatCopy(SURFOBJ* Dest,
	   SURFOBJ* Source,
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
    DibFunctionsForBitmapFormat[Dest->iBitmapFormat].DIB_HLine(
      Dest, DestRect->left, DestRect->right, y,  Brush->iSolidColor);
  }

  return TRUE;
}

static BOOLEAN STDCALL
CallDibBitBlt(SURFOBJ* OutputObj,
              SURFOBJ* InputObj,
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

  return DibFunctionsForBitmapFormat[OutputObj->iBitmapFormat].DIB_BitBlt(
    OutputObj, InputObj, OutputRect, InputPoint, Brush, RealBrushOrigin, ColorTranslation, Rop4);
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
  
  if(BrushOrigin)
  {
    AdjustedBrushOrigin.x = BrushOrigin->x + Translate.x;
    AdjustedBrushOrigin.y = BrushOrigin->y + Translate.y;
  }
  else
    AdjustedBrushOrigin = Translate;

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
      Ret = (*BltRectFunc)(OutputObj, InputObj, Mask, ColorTranslation,
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
      Ret = (*BltRectFunc)(OutputObj, InputObj, Mask, ColorTranslation,
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
	      Ret = (*BltRectFunc)(OutputObj, InputObj, Mask, ColorTranslation,
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
IntEngBitBlt(BITMAPOBJ *DestObj,
             BITMAPOBJ *SourceObj,
             BITMAPOBJ *MaskObj,
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
  RECTL InputClippedRect;
  RECTL OutputRect;
  POINTL InputPoint;
  BOOLEAN UsesSource;
  SURFOBJ *DestSurf = &DestObj->SurfObj;
  SURFOBJ *SourceSurf = SourceObj ? &SourceObj->SurfObj : NULL;
  SURFOBJ *MaskSurf = MaskObj ? &MaskObj->SurfObj : NULL;

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
      if (SourceSurf->sizlBitmap.cx < InputPoint.x + InputClippedRect.right - InputClippedRect.left)
        {
          InputClippedRect.right = InputClippedRect.left + SourceSurf->sizlBitmap.cx - InputPoint.x;
        }
      if (SourceSurf->sizlBitmap.cy < InputPoint.y + InputClippedRect.bottom - InputClippedRect.top)
        {
          InputClippedRect.bottom = InputClippedRect.top + SourceSurf->sizlBitmap.cy - InputPoint.y;
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
    MouseSafetyOnDrawStart(SourceSurf, InputPoint.x, InputPoint.y,
                           (InputPoint.x + abs(DestRect->right - DestRect->left)),
			   (InputPoint.y + abs(DestRect->bottom - DestRect->top)));
    }

  /* No success yet */
  ret = FALSE;
  MouseSafetyOnDrawStart(DestSurf, OutputRect.left, OutputRect.top,
                         OutputRect.right, OutputRect.bottom);

  /* Call the driver's DrvBitBlt if available */
  if (DestObj->flHooks & HOOK_BITBLT)
    {
      ret = GDIDEVFUNCS(DestSurf).BitBlt(
                            DestSurf, SourceSurf, MaskSurf, ClipRegion, ColorTranslation,
                            &OutputRect, &InputPoint, MaskOrigin, Brush, BrushOrigin,
                            Rop4);
    }

  if (! ret)
    {
      ret = EngBitBlt(DestSurf, SourceSurf, MaskSurf, ClipRegion, ColorTranslation,
                      &OutputRect, &InputPoint, MaskOrigin, Brush, BrushOrigin,
                      Rop4);
    }

  MouseSafetyOnDrawEnd(DestSurf);
  if (UsesSource)
    {
    MouseSafetyOnDrawEnd(SourceSurf);
    }

  return ret;
}

static BOOLEAN STDCALL
CallDibStretchBlt(SURFOBJ* OutputObj,
                  SURFOBJ* InputObj,
                  SURFOBJ* Mask,
	          CLIPOBJ* ClipRegion,
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
  return DibFunctionsForBitmapFormat[OutputObj->iBitmapFormat].DIB_StretchBlt(
    OutputObj, InputObj, OutputRect, InputRect, MaskOrigin, RealBrushOrigin, ClipRegion, ColorTranslation, Mode);
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
  
  POINTL             InputPoint;
  RECTL              InputRect;
  RECTL              OutputRect;
  POINTL             Translate;
  INTENG_ENTER_LEAVE EnterLeaveSource;
  INTENG_ENTER_LEAVE EnterLeaveDest;
  SURFOBJ*           InputObj;
  SURFOBJ*           OutputObj;
  PSTRETCHRECTFUNC   BltRectFunc;
  BOOLEAN            Ret;
  POINTL             AdjustedBrushOrigin;

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
 
  OutputRect = *prclDest;

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
  
  if (NULL != BrushOrigin)
    {
      AdjustedBrushOrigin.x = BrushOrigin->x + Translate.x;
      AdjustedBrushOrigin.y = BrushOrigin->y + Translate.y;
    }
  else
    {
      AdjustedBrushOrigin = Translate;
    }

  if (Mask != NULL)
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


  Ret = (*BltRectFunc)(OutputObj, InputObj, Mask, ClipRegion,
                       ColorTranslation, &OutputRect, &InputRect, MaskOrigin,
                       &AdjustedBrushOrigin, Mode);

  IntEngLeave(&EnterLeaveDest);
  IntEngLeave(&EnterLeaveSource);

  return Ret;
}

BOOL STDCALL
IntEngStretchBlt(BITMAPOBJ *DestObj,
                 BITMAPOBJ *SourceObj,
                 BITMAPOBJ *MaskObj,
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
  COLORADJUSTMENT ca;
  POINT MaskOrigin;
  SURFOBJ *DestSurf = &DestObj->SurfObj;
  SURFOBJ *SourceSurf = SourceObj ? &SourceObj->SurfObj : NULL;
  SURFOBJ *MaskSurf = MaskObj ? &MaskObj->SurfObj : NULL;

  if (pMaskOrigin != NULL)
    {
      MaskOrigin.x = pMaskOrigin->x; MaskOrigin.y = pMaskOrigin->y;
    }

  if (NULL != SourceObj)
    {
    MouseSafetyOnDrawStart(SourceSurf, SourceRect->left, SourceRect->top,
                           (SourceRect->left + abs(SourceRect->right - SourceRect->left)),
			   (SourceRect->top + abs(SourceRect->bottom - SourceRect->top)));
    }

  /* No success yet */
  ret = FALSE;
  MouseSafetyOnDrawStart(DestSurf, DestRect->left, DestRect->top,
                         DestRect->right, DestRect->bottom);

  /* Prepare color adjustment */

  /* Call the driver's DrvStretchBlt if available */
  if (DestObj->flHooks & HOOK_STRETCHBLT)
    {
      /* Drv->StretchBlt (look at http://www.osr.com/ddk/graphics/ddifncs_3ew7.htm )
      SURFOBJ *psoMask // optional, if it exists, then rop4=0xCCAA, otherwise rop4=0xCCCC */
      // FIXME: MaskOrigin is always NULL !
      ret = GDIDEVFUNCS(DestSurf).StretchBlt(
                            DestSurf, SourceSurf, MaskSurf, ClipRegion, ColorTranslation,
                            &ca, BrushOrigin, DestRect, SourceRect, NULL, Mode);
    }

  if (! ret)
    {
      // FIXME: see previous fixme
      ret = EngStretchBlt(DestSurf, SourceSurf, MaskSurf, ClipRegion, ColorTranslation,
                          &ca, BrushOrigin, DestRect, SourceRect, NULL, Mode);
    }

  MouseSafetyOnDrawEnd(DestSurf);
  if (NULL != SourceSurf)
    {
    MouseSafetyOnDrawEnd(SourceSurf);
    }

  return ret;
}

/**** REACTOS FONT RENDERING CODE *********************************************/

/* renders the alpha mask bitmap */
static BOOLEAN STDCALL
AlphaBltMask(SURFOBJ* Dest,
	SURFOBJ* Source,
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
				DibFunctionsForBitmapFormat[Dest->iBitmapFormat].DIB_PutPixel(
					Dest, DestRect->left + i, DestRect->top + j, Brush->iSolidColor);
			}
			else
			{
				Background = DIB_GetSource(Dest, DestRect->left + i, DestRect->top + j, SrcColorTranslation);

				NewColor = 
				     RGB((*lMask * (r - GetRValue(Background)) >> 8) + GetRValue(Background),
				         (*lMask * (g - GetGValue(Background)) >> 8) + GetGValue(Background),
				         (*lMask * (b - GetBValue(Background)) >> 8) + GetBValue(Background));
				
				Background = XLATEOBJ_iXlate(ColorTranslation, NewColor);
				DibFunctionsForBitmapFormat[Dest->iBitmapFormat].DIB_PutPixel(
					Dest, DestRect->left + i, DestRect->top + j, Background);
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

  if (! IntEngEnter(&EnterLeaveSource, NULL, &InputRect, TRUE, &Translate, &InputObj))
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
  
  if(BrushOrigin)
  {
    AdjustedBrushOrigin.x = BrushOrigin->x + Translate.x;
    AdjustedBrushOrigin.y = BrushOrigin->y + Translate.y;
  }
  else
    AdjustedBrushOrigin = Translate;

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
        Ret = AlphaBltMask(OutputObj, InputObj, Mask, DestColorTranslation, SourceColorTranslation,
                           &OutputRect, &InputPoint, MaskOrigin, Brush, &AdjustedBrushOrigin);
      else
        Ret = BltMask(OutputObj, InputObj, Mask, DestColorTranslation,
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
        Ret = AlphaBltMask(OutputObj, InputObj, Mask, DestColorTranslation, SourceColorTranslation,
                           &CombinedRect, &Pt, MaskOrigin, Brush, &AdjustedBrushOrigin);
      else
        Ret = BltMask(OutputObj, InputObj, Mask, DestColorTranslation,
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
	        Ret = AlphaBltMask(OutputObj, InputObj, Mask, DestColorTranslation, SourceColorTranslation,
	                           &CombinedRect, &Pt, MaskOrigin, Brush, &AdjustedBrushOrigin) && Ret;
              else
                Ret = BltMask(OutputObj, InputObj, Mask, DestColorTranslation,
                                   &CombinedRect, &Pt, MaskOrigin, Brush, &AdjustedBrushOrigin, 0xAACC) && Ret;
	    }
	}
      while(EnumMore);
      break;
  }


  IntEngLeave(&EnterLeaveDest);
  IntEngLeave(&EnterLeaveSource);

  /* Dummy BitBlt to let driver know that something has changed.
     0x00AA0029 is the Rop for D (no-op) */
  /* FIXME: Remove the typecast! */
  IntEngBitBlt((BITMAPOBJ*)DestObj, NULL, (BITMAPOBJ*)Mask, ClipRegion, DestColorTranslation,
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
  MouseSafetyOnDrawStart(DestObj, OutputRect.left, OutputRect.top,
                         OutputRect.right, OutputRect.bottom);

  ret = EngMaskBitBlt(DestObj, Mask, ClipRegion, DestColorTranslation, SourceColorTranslation,
                      &OutputRect, &InputPoint, MaskOrigin, Brush, BrushOrigin);

  MouseSafetyOnDrawEnd(DestObj);

  return ret;
}
/* EOF */
