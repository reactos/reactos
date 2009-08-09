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
/* $Id$
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          GDI TransparentBlt Function
 * FILE:             subsys/win32k/eng/transblt.c
 * PROGRAMER:        Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * REVISION HISTORY:
 *        4/6/2004: Created
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

BOOL APIENTRY
EngTransparentBlt(SURFOBJ *psoDest,
		  SURFOBJ *psoSource,
		  CLIPOBJ *Clip,
		  XLATEOBJ *ColorTranslation,
		  PRECTL DestRect,
		  PRECTL SourceRect,
		  ULONG iTransColor,
		  ULONG Reserved)
{
  BOOL Ret = TRUE;
  BYTE ClippingType;
  INTENG_ENTER_LEAVE EnterLeaveSource, EnterLeaveDest;
  SURFOBJ *InputObj, *OutputObj;
  RECTL OutputRect, InputRect;
  POINTL Translate, InputPoint;

  InputRect.left = 0;
  InputRect.right = DestRect->right - DestRect->left;
  InputRect.top = 0;
  InputRect.bottom = DestRect->bottom - DestRect->top;

  if(!IntEngEnter(&EnterLeaveSource, psoSource, &InputRect, TRUE, &Translate, &InputObj))
  {
    return FALSE;
  }

  InputPoint.x = SourceRect->left + Translate.x;
  InputPoint.y = SourceRect->top + Translate.y;

  OutputRect = *DestRect;
  if(Clip)
  {
    if(OutputRect.left < Clip->rclBounds.left)
    {
      InputRect.left += Clip->rclBounds.left - OutputRect.left;
      InputPoint.x += Clip->rclBounds.left - OutputRect.left;
      OutputRect.left = Clip->rclBounds.left;
    }
    if(Clip->rclBounds.right < OutputRect.right)
    {
      InputRect.right -=  OutputRect.right - Clip->rclBounds.right;
      OutputRect.right = Clip->rclBounds.right;
    }
    if(OutputRect.top < Clip->rclBounds.top)
    {
      InputRect.top += Clip->rclBounds.top - OutputRect.top;
      InputPoint.y += Clip->rclBounds.top - OutputRect.top;
      OutputRect.top = Clip->rclBounds.top;
    }
    if(Clip->rclBounds.bottom < OutputRect.bottom)
    {
      InputRect.bottom -=  OutputRect.bottom - Clip->rclBounds.bottom;
      OutputRect.bottom = Clip->rclBounds.bottom;
    }
  }

  /* Check for degenerate case: if height or width of OutputRect is 0 pixels there's
     nothing to do */
  if(OutputRect.right <= OutputRect.left || OutputRect.bottom <= OutputRect.top)
  {
    IntEngLeave(&EnterLeaveSource);
    return TRUE;
  }

  if(!IntEngEnter(&EnterLeaveDest, psoDest, &OutputRect, FALSE, &Translate, &OutputObj))
  {
    IntEngLeave(&EnterLeaveSource);
    return FALSE;
  }

  OutputRect.left = DestRect->left + Translate.x;
  OutputRect.right = DestRect->right + Translate.x;
  OutputRect.top = DestRect->top + Translate.y;
  OutputRect.bottom = DestRect->bottom + Translate.y;

  ClippingType = (Clip ? Clip->iDComplexity : DC_TRIVIAL);

  switch(ClippingType)
  {
    case DC_TRIVIAL:
    {
      Ret = DibFunctionsForBitmapFormat[psoDest->iBitmapFormat].DIB_TransparentBlt(
        OutputObj, InputObj, &OutputRect, &InputPoint, ColorTranslation, iTransColor);
      break;
    }
    case DC_RECT:
    {
      RECTL ClipRect, CombinedRect;
      POINTL Pt;

      ClipRect.left = Clip->rclBounds.left + Translate.x;
      ClipRect.right = Clip->rclBounds.right + Translate.x;
      ClipRect.top = Clip->rclBounds.top + Translate.y;
      ClipRect.bottom = Clip->rclBounds.bottom + Translate.y;
      EngIntersectRect(&CombinedRect, &OutputRect, &ClipRect);
      Pt.x = InputPoint.x + CombinedRect.left - OutputRect.left;
      Pt.y = InputPoint.y + CombinedRect.top - OutputRect.top;
      Ret = DibFunctionsForBitmapFormat[psoDest->iBitmapFormat].DIB_TransparentBlt(
        OutputObj, InputObj, &CombinedRect, &Pt, ColorTranslation, iTransColor);
      break;
    }
    case DC_COMPLEX:
    {
      ULONG Direction, i;
      RECT_ENUM RectEnum;
      BOOL EnumMore;
      POINTL Pt;

      if(OutputObj == InputObj)
      {
        if(OutputRect.top < InputPoint.y)
        {
          Direction = OutputRect.left < (InputPoint.x ? CD_RIGHTDOWN : CD_LEFTDOWN);
        }
        else
        {
          Direction = OutputRect.left < (InputPoint.x ? CD_RIGHTUP : CD_LEFTUP);
        }
      }
      else
      {
        Direction = CD_ANY;
      }

      CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES, Direction, 0);
      do
      {
        EnumMore = CLIPOBJ_bEnum(Clip, sizeof(RectEnum), (PVOID)&RectEnum);
        for (i = 0; i < RectEnum.c; i++)
        {
          RECTL ClipRect, CombinedRect;

          ClipRect.left = RectEnum.arcl[i].left + Translate.x;
          ClipRect.right = RectEnum.arcl[i].right + Translate.x;
          ClipRect.top = RectEnum.arcl[i].top + Translate.y;
          ClipRect.bottom = RectEnum.arcl[i].bottom + Translate.y;
          EngIntersectRect(&CombinedRect, &OutputRect, &ClipRect);
          Pt.x = InputPoint.x + CombinedRect.left - OutputRect.left;
          Pt.y = InputPoint.y + CombinedRect.top - OutputRect.top;
          Ret = DibFunctionsForBitmapFormat[psoDest->iBitmapFormat].DIB_TransparentBlt(
            OutputObj, InputObj, &CombinedRect, &Pt, ColorTranslation, iTransColor);
          if(!Ret)
          {
            break;
          }
        }
      } while(EnumMore && Ret);
      break;
    }
    default:
    {
      Ret = FALSE;
      break;
    }
  }

  IntEngLeave(&EnterLeaveDest);
  IntEngLeave(&EnterLeaveSource);

  return Ret;
}

BOOL FASTCALL
IntEngTransparentBlt(SURFOBJ *psoDest,
                     SURFOBJ *psoSource,
                     CLIPOBJ *Clip,
                     XLATEOBJ *ColorTranslation,
                     PRECTL DestRect,
                     PRECTL SourceRect,
                     ULONG iTransColor,
                     ULONG Reserved)
{
  BOOL Ret;
  RECTL OutputRect, InputClippedRect;
  SURFACE *psurfDest;
  SURFACE *psurfSource;

  ASSERT(psoDest);
  ASSERT(psoSource);
  ASSERT(DestRect);

  psurfDest = CONTAINING_RECORD(psoDest, SURFACE, SurfObj);
  psurfSource = CONTAINING_RECORD(psoSource, SURFACE, SurfObj);

  ASSERT(psurfDest);
  ASSERT(psurfSource);

  InputClippedRect = *DestRect;
  if(InputClippedRect.right < InputClippedRect.left)
  {
    InputClippedRect.left = DestRect->right;
    InputClippedRect.right = DestRect->left;
  }
  if(InputClippedRect.bottom < InputClippedRect.top)
  {
    InputClippedRect.top = DestRect->bottom;
    InputClippedRect.bottom = DestRect->top;
  }

  /* Clip against the bounds of the clipping region so we won't try to write
   * outside the surface */
  if(Clip)
  {
    if(!EngIntersectRect(&OutputRect, &InputClippedRect, &Clip->rclBounds))
    {
      return TRUE;
    }
    SourceRect->left += OutputRect.left - DestRect->left;
    SourceRect->top += OutputRect.top - DestRect->top;
    SourceRect->right += OutputRect.left - DestRect->left;
    SourceRect->bottom += OutputRect.top - DestRect->top;
  }
  else
  {
    OutputRect = *DestRect;
  }

  if(psoSource != psoDest)
  {
    SURFACE_LockBitmapBits(psurfSource);
    MouseSafetyOnDrawStart(psoSource, SourceRect->left, SourceRect->top,
                           SourceRect->right, SourceRect->bottom);
  }
  SURFACE_LockBitmapBits(psurfDest);
  MouseSafetyOnDrawStart(psoDest, OutputRect.left, OutputRect.top,
                         OutputRect.right, OutputRect.bottom);

  if(psurfDest->flHooks & HOOK_TRANSPARENTBLT)
  {
    Ret = GDIDEVFUNCS(psoDest).TransparentBlt(
      psoDest, psoSource, Clip, ColorTranslation, &OutputRect,
      SourceRect, iTransColor, Reserved);
  }
  else
    Ret = FALSE;

  if(!Ret)
  {
    Ret = EngTransparentBlt(psoDest, psoSource, Clip, ColorTranslation,
                            &OutputRect, SourceRect, iTransColor, Reserved);
  }

  MouseSafetyOnDrawEnd(psoDest);
  SURFACE_UnlockBitmapBits(psurfDest);
  if(psoSource != psoDest)
  {
    MouseSafetyOnDrawEnd(psoSource);
    SURFACE_UnlockBitmapBits(psurfSource);
  }

  return Ret;
}

/* EOF */
