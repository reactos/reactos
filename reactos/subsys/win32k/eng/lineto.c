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
 *
 * $Id: lineto.c,v 1.31 2004/03/27 00:35:02 weiden Exp $
 */

#include <ddk/winddi.h>
#include <ddk/ntddmou.h>
#include <include/inteng.h>
#include <include/eng.h>
#include <include/dib.h>
#include "clip.h"
#include "objects.h"
#include "../dib/dib.h"
#include "misc.h"

#include <include/mouse.h>
#include <include/object.h>
#include <include/surface.h>

static void FASTCALL
TranslateRects(RECT_ENUM *RectEnum, POINTL* Translate)
{
  RECTL* CurrentRect;

  if (0 != Translate->x || 0 != Translate->y)
    {
      for (CurrentRect = RectEnum->arcl; CurrentRect < RectEnum->arcl + RectEnum->c; CurrentRect++)
	{
	  CurrentRect->left += Translate->x;
	  CurrentRect->right += Translate->x;
	  CurrentRect->top += Translate->y;
	  CurrentRect->bottom += Translate->y;
	}
    }
}

/*
 * Draw a line from top-left to bottom-right
 */
static void FASTCALL
NWtoSE(SURFOBJ* OutputObj, SURFGDI* OutputGDI, CLIPOBJ* Clip,
       BRUSHOBJ* Brush, LONG x, LONG y, LONG deltax, LONG deltay,
       POINTL* Translate)
{
  int i;
  int error;
  BOOLEAN EnumMore;
  RECTL* ClipRect;
  RECT_ENUM RectEnum;
  ULONG Pixel = Brush->iSolidColor;
  LONG delta;

  CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES, CD_RIGHTDOWN, 0);
  EnumMore = CLIPOBJ_bEnum(Clip, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
  TranslateRects(&RectEnum, Translate);
  ClipRect = RectEnum.arcl;
  delta = max(deltax, deltay);
  i = 0;
  error = delta >> 1;
  while (i < delta && (ClipRect < RectEnum.arcl + RectEnum.c || EnumMore))
    {
      while ((ClipRect < RectEnum.arcl + RectEnum.c /* there's still a current clip rect */
              && (ClipRect->bottom <= y             /* but it's above us */
                  || (ClipRect->top <= y && ClipRect->right <= x))) /* or to the left of us */
             || EnumMore)                           /* no current clip rect, but rects left */
	{
	  /* Skip to the next clip rect */
	  if (RectEnum.arcl + RectEnum.c <= ClipRect)
	    {
	      EnumMore = CLIPOBJ_bEnum(Clip, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
	      TranslateRects(&RectEnum, Translate);
	      ClipRect = RectEnum.arcl;
	    }
	  else
	    {
	      ClipRect++;
	    }
	}
      if (ClipRect < RectEnum.arcl + RectEnum.c) /* If there's no current clip rect we're done */
	{
	  if (ClipRect->left <= x && ClipRect->top <= y)
	    {
	      OutputGDI->DIB_PutPixel(OutputObj, x, y, Pixel);
	    }
	  if (deltax < deltay)
	    {
	      y++;
	      error = error + deltax;
	      if (deltay <= error)
		{
		  x++;
		  error = error - deltay;
		}
	    }
	  else
	    {
	      x++;
	      error = error + deltay;
	      if (deltax <= error)
		{
	  	  y++;
		  error = error - deltax;
		}
	    }
	  i++;
	}
    }
}

static void FASTCALL
SWtoNE(SURFOBJ* OutputObj, SURFGDI* OutputGDI, CLIPOBJ* Clip,
       BRUSHOBJ* Brush, LONG x, LONG y, LONG deltax, LONG deltay,
       POINTL* Translate)
{
  int i;
  int error;
  BOOLEAN EnumMore;
  RECTL* ClipRect;
  RECT_ENUM RectEnum;
  ULONG Pixel = Brush->iSolidColor;
  LONG delta;

  CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES, CD_RIGHTUP, 0);
  EnumMore = CLIPOBJ_bEnum(Clip, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
  TranslateRects(&RectEnum, Translate);
  ClipRect = RectEnum.arcl;
  delta = max(deltax, deltay);
  i = 0;
  error = delta >> 1;
  while (i < delta && (ClipRect < RectEnum.arcl + RectEnum.c || EnumMore))
    {
      while ((ClipRect < RectEnum.arcl + RectEnum.c
              && (y < ClipRect->top
                  || (y < ClipRect->bottom && ClipRect->right <= x)))
             || EnumMore)
	{
	  if (RectEnum.arcl + RectEnum.c <= ClipRect)
	    {
	      EnumMore = CLIPOBJ_bEnum(Clip, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
	      TranslateRects(&RectEnum, Translate);
	      ClipRect = RectEnum.arcl;
	    }
	  else
	    {
	      ClipRect++;
	    }
	}
      if (ClipRect < RectEnum.arcl + RectEnum.c)
	{
	  if (ClipRect->left <= x && y < ClipRect->bottom)
	    {
	      OutputGDI->DIB_PutPixel(OutputObj, x, y, Pixel);
	    }
	  if (deltax < deltay)
	    {
	      y--;
	      error = error + deltax;
	      if (deltay <= error)
		{
		  x++;
		  error = error - deltay;
		}
	    }
	  else
	    {
	      x++;
	      error = error + deltay;
	      if (deltax <= error)
		{
	  	  y--;
		  error = error - deltax;
		}
	    }
	  i++;
	}
    }
}

static void FASTCALL
NEtoSW(SURFOBJ* OutputObj, SURFGDI* OutputGDI, CLIPOBJ* Clip,
       BRUSHOBJ* Brush, LONG x, LONG y, LONG deltax, LONG deltay,
       POINTL* Translate)
{
  int i;
  int error;
  BOOLEAN EnumMore;
  RECTL* ClipRect;
  RECT_ENUM RectEnum;
  ULONG Pixel = Brush->iSolidColor;
  LONG delta;

  CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES, CD_LEFTDOWN, 0);
  EnumMore = CLIPOBJ_bEnum(Clip, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
  TranslateRects(&RectEnum, Translate);
  ClipRect = RectEnum.arcl;
  delta = max(deltax, deltay);
  i = 0;
  error = delta >> 1;
  while (i < delta && (ClipRect < RectEnum.arcl + RectEnum.c || EnumMore))
    {
      while ((ClipRect < RectEnum.arcl + RectEnum.c
              && (ClipRect->bottom <= y
                  || (ClipRect->top <= y && x < ClipRect->left)))
             || EnumMore)
	{
	  if (RectEnum.arcl + RectEnum.c <= ClipRect)
	    {
	      EnumMore = CLIPOBJ_bEnum(Clip, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
	      TranslateRects(&RectEnum, Translate);
	      ClipRect = RectEnum.arcl;
	    }
	  else
	    {
	      ClipRect++;
	    }
	}
      if (ClipRect < RectEnum.arcl + RectEnum.c)
	{
	  if (x < ClipRect->right && ClipRect->top <= y)
	    {
	      OutputGDI->DIB_PutPixel(OutputObj, x, y, Pixel);
	    }
	  if (deltax < deltay)
	    {
	      y++;
	      error = error + deltax;
	      if (deltay <= error)
		{
		  x--;
		  error = error - deltay;
		}
	    }
	  else
	    {
	      x--;
	      error = error + deltay;
	      if (deltax <= error)
		{
	  	  y++;
		  error = error - deltax;
		}
	    }
	  i++;
	}
    }
}

static void FASTCALL
SEtoNW(SURFOBJ* OutputObj, SURFGDI* OutputGDI, CLIPOBJ* Clip,
       BRUSHOBJ* Brush, LONG x, LONG y, LONG deltax, LONG deltay,
       POINTL* Translate)
{
  int i;
  int error;
  BOOLEAN EnumMore;
  RECTL* ClipRect;
  RECT_ENUM RectEnum;
  ULONG Pixel = Brush->iSolidColor;
  LONG delta;

  CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES, CD_LEFTUP, 0);
  EnumMore = CLIPOBJ_bEnum(Clip, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
  TranslateRects(&RectEnum, Translate);
  ClipRect = RectEnum.arcl;
  delta = max(deltax, deltay);
  i = 0;
  error = delta >> 1;
  while (i < delta && (ClipRect < RectEnum.arcl + RectEnum.c || EnumMore))
    {
      while ((ClipRect < RectEnum.arcl + RectEnum.c
              && (y < ClipRect->top
                  || (y < ClipRect->bottom && x < ClipRect->left)))
             || EnumMore)
	{
	  if (RectEnum.arcl + RectEnum.c <= ClipRect)
	    {
	      EnumMore = CLIPOBJ_bEnum(Clip, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
	      TranslateRects(&RectEnum, Translate);
	      ClipRect = RectEnum.arcl;
	    }
	  else
	    {
	      ClipRect++;
	    }
	}
      if (ClipRect < RectEnum.arcl + RectEnum.c)
	{
	  if (x < ClipRect->right && y < ClipRect->bottom)
	    {
	      OutputGDI->DIB_PutPixel(OutputObj, x, y, Pixel);
	    }
	  if (deltax < deltay)
	    {
	      y--;
	      error = error + deltax;
	      if (deltay <= error)
		{
		  x--;
		  error = error - deltay;
		}
	    }
	  else
	    {
	      x--;
	      error = error + deltay;
	      if (deltax <= error)
		{
	  	  y--;
		  error = error - deltax;
		}
	    }
	  i++;
	}
    }
}

/*
 * @implemented
 */
BOOL STDCALL
EngLineTo(SURFOBJ *DestObj,
	  CLIPOBJ *Clip,
	  BRUSHOBJ *Brush,
	  LONG x1,
	  LONG y1,
	  LONG x2,
	  LONG y2,
	  RECTL *RectBounds,
	  MIX mix)
{
  LONG x, y, deltax, deltay, xchange, ychange, hx, vy;
  ULONG i;
  ULONG Pixel = Brush->iSolidColor;
  SURFOBJ *OutputObj;
  SURFGDI *OutputGDI;
  RECTL DestRect;
  POINTL Translate;
  INTENG_ENTER_LEAVE EnterLeave;
  RECT_ENUM RectEnum;
  BOOL EnumMore;

  if (x1 < x2)
    {
      DestRect.left = x1;
      DestRect.right = x2;
    }
  else
    {
      DestRect.left = x2;
      DestRect.right = x1 + 1;
    }
  if (y1 < y2)
    {
      DestRect.top = y1;
      DestRect.bottom = y2;
    }
  else
    {
      DestRect.top = y2;
      DestRect.bottom = y1 + 1;
    }

  if (! IntEngEnter(&EnterLeave, DestObj, &DestRect, FALSE, &Translate, &OutputObj))
    {
      return FALSE;
    }

  x1 += Translate.x;
  x2 += Translate.x;
  y1 += Translate.y;
  y2 += Translate.y;

  OutputGDI = AccessInternalObjectFromUserObject(OutputObj);

  x = x1;
  y = y1;
  deltax = x2 - x1;
  deltay = y2 - y1;

  if (deltax < 0)
    {
      xchange = -1;
      deltax = - deltax;
      hx = x2 + 1;
    }
  else
    {
      xchange = 1;
      hx = x1;
    }

  if (deltay < 0)
    {
      ychange = -1;
      deltay = - deltay;
      vy = y2 + 1;
    }
  else
    {
      ychange = 1;
      vy = y1;
    }

  if (y1 == y2)
    {
      CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES, CD_RIGHTDOWN, 0);
      do
	{
	  EnumMore = CLIPOBJ_bEnum(Clip, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
	  for (i = 0; i < RectEnum.c && RectEnum.arcl[i].top + Translate.y <= y1; i++)
	    {
	      if (y1 < RectEnum.arcl[i].bottom + Translate.y &&
	          RectEnum.arcl[i].left + Translate.x <= hx + deltax &&
	          hx < RectEnum.arcl[i].right + Translate.x)
		{
		  OutputGDI->DIB_HLine(OutputObj,
		                       max(hx, RectEnum.arcl[i].left + Translate.x),
		                       min(hx + deltax, RectEnum.arcl[i].right + Translate.x),
		                       y1, Pixel);
		}
	    }
	}
      while (EnumMore);
    }
  else if (x1 == x2)
    {
      CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES, CD_RIGHTDOWN, 0);
      do
	{
	  EnumMore = CLIPOBJ_bEnum(Clip, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
	  for (i = 0; i < RectEnum.c; i++)
	    {
	      if (RectEnum.arcl[i].left + Translate.x <= x1 &&
                  x1 < RectEnum.arcl[i].right + Translate.x &&
	          RectEnum.arcl[i].top + Translate.y <= vy + deltay &&
	          vy < RectEnum.arcl[i].bottom + Translate.y)
		{
		  OutputGDI->DIB_VLine(OutputObj, x1,
		                       max(vy, RectEnum.arcl[i].top + Translate.y),
		                       min(vy + deltay, RectEnum.arcl[i].bottom + Translate.y),
		                       Pixel);
		}
	    }
	}
      while (EnumMore);
    }
  else
    {
      if (0 < xchange)
	{
	  if (0 < ychange)
	    {
	      NWtoSE(OutputObj, OutputGDI, Clip, Brush, x, y, deltax, deltay, &Translate);
	    }
	  else
	    {
	      SWtoNE(OutputObj, OutputGDI, Clip, Brush, x, y, deltax, deltay, &Translate);
	    }
	}
      else
	{
	  if (0 < ychange)
	    {
	      NEtoSW(OutputObj, OutputGDI, Clip, Brush, x, y, deltax, deltay, &Translate);
	    }
	  else
	    {
	      SEtoNW(OutputObj, OutputGDI, Clip, Brush, x, y, deltax, deltay, &Translate);
	    }
	}
    }

  return IntEngLeave(&EnterLeave);
}

BOOL STDCALL
IntEngLineTo(SURFOBJ *DestSurf,
	     CLIPOBJ *Clip,
	     BRUSHOBJ *Brush,
	     LONG x1,
	     LONG y1,
	     LONG x2,
	     LONG y2,
	     RECTL *RectBounds,
	     MIX mix)
{
  BOOLEAN ret;
  SURFGDI *SurfGDI;
  RECTL b;

  if (Brush->logbrush.lbStyle == BS_NULL)
    return TRUE;

  /* No success yet */
  ret = FALSE;
  SurfGDI = (SURFGDI*)AccessInternalObjectFromUserObject(DestSurf);

  b.left = min(x1, x2);
  b.right = max(x1, x2);
  b.top = min(y1, y2);
  b.bottom = max(y1, y2);
  if (b.left == b.right) b.right++;
  if (b.top == b.bottom) b.bottom++;
  MouseSafetyOnDrawStart(DestSurf, SurfGDI, x1, y1, x2, y2);

  if (NULL != SurfGDI->LineTo)
    {
    /* Call the driver's DrvLineTo */
    IntLockGDIDriver(SurfGDI);
    ret = SurfGDI->LineTo(DestSurf, Clip, Brush, x1, y1, x2, y2, /*RectBounds*/&b, mix);
    IntUnLockGDIDriver(SurfGDI);
    }

#if 0
  if (! ret && NULL != SurfGDI->StrokePath)
    {
      /* FIXME: Emulate LineTo using drivers DrvStrokePath and set ret on success */
    }
#endif

  if (! ret)
    {
      ret = EngLineTo(DestSurf, Clip, Brush, x1, y1, x2, y2, RectBounds, mix);
    }

  MouseSafetyOnDrawEnd(DestSurf, SurfGDI);

  return ret;
}

BOOL STDCALL
IntEngPolyline(SURFOBJ *DestSurf,
	       CLIPOBJ *Clip,
	       BRUSHOBJ *Brush,
	       CONST LPPOINT  pt,
               LONG dCount,
	       MIX mix)
{
  LONG i;
  RECTL rect;
  BOOL ret = FALSE;

  //Draw the Polyline with a call to IntEngLineTo for each segment.
  for (i = 1; i < dCount; i++)
    {
      rect.left = min(pt[i-1].x, pt[i].x);
      rect.top = min(pt[i-1].y, pt[i].y);
      rect.right = max(pt[i-1].x, pt[i].x);
      rect.bottom = max(pt[i-1].y, pt[i].y);
      ret = IntEngLineTo(DestSurf,
	                 Clip,
	                 Brush,
                         pt[i-1].x,
	                 pt[i-1].y,
	                 pt[i].x,
	                 pt[i].y,
	                 &rect,
	                 mix);
      if (!ret)
	{
	  break;
	}
    }

  return ret;
}

/* EOF */
