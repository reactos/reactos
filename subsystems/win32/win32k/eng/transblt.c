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
  POINTL Translate;

  LONG DstHeight;
  LONG DstWidth;
  LONG SrcHeight;
  LONG SrcWidth;

  InputRect = *SourceRect;

  if(!IntEngEnter(&EnterLeaveSource, psoSource, &InputRect, TRUE, &Translate, &InputObj))
  {
    return FALSE;
  }
  InputRect.left += Translate.x;
  InputRect.right += Translate.x;
  InputRect.top += Translate.y;
  InputRect.bottom += Translate.y;

  OutputRect = *DestRect;
  if (OutputRect.right < OutputRect.left)
  {
    OutputRect.left = DestRect->right;
    OutputRect.right = DestRect->left;
  }
  if (OutputRect.bottom < OutputRect.top)
  {
    OutputRect.top = DestRect->bottom;
    OutputRect.bottom = DestRect->top;
  }
    
  if(Clip)
  {
    if(OutputRect.left < Clip->rclBounds.left)
    {
      InputRect.left += Clip->rclBounds.left - OutputRect.left;
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

  DstHeight = OutputRect.bottom - OutputRect.top;
  DstWidth = OutputRect.right - OutputRect.left;
  SrcHeight = InputRect.bottom - InputRect.top;
  SrcWidth = InputRect.right - InputRect.left;
  switch(ClippingType)
  {
    case DC_TRIVIAL:
    {
      Ret = DibFunctionsForBitmapFormat[psoDest->iBitmapFormat].DIB_TransparentBlt(
        OutputObj, InputObj, &OutputRect, &InputRect, ColorTranslation, iTransColor);
      break;
    }
    case DC_RECT:
    {
      RECTL ClipRect, CombinedRect;
      RECTL InputToCombinedRect;

      ClipRect.left = Clip->rclBounds.left + Translate.x;
      ClipRect.right = Clip->rclBounds.right + Translate.x;
      ClipRect.top = Clip->rclBounds.top + Translate.y;
      ClipRect.bottom = Clip->rclBounds.bottom + Translate.y;
      if (RECTL_bIntersectRect(&CombinedRect, &OutputRect, &ClipRect))
      {
        InputToCombinedRect.top = InputRect.top + (CombinedRect.top - OutputRect.top) * SrcHeight / DstHeight;
        InputToCombinedRect.bottom = InputRect.top + (CombinedRect.bottom - OutputRect.top) * SrcHeight / DstHeight;
        InputToCombinedRect.left = InputRect.left + (CombinedRect.left - OutputRect.left) * SrcWidth / DstWidth;
        InputToCombinedRect.right = InputRect.left + (CombinedRect.right - OutputRect.left) * SrcWidth / DstWidth;
        Ret = DibFunctionsForBitmapFormat[psoDest->iBitmapFormat].DIB_TransparentBlt(
          OutputObj, InputObj, &CombinedRect, &InputToCombinedRect, ColorTranslation, iTransColor);
      }
      break;
    }
    case DC_COMPLEX:
    {
      ULONG Direction, i;
      RECT_ENUM RectEnum;
      BOOL EnumMore;

      if(OutputObj == InputObj)
      {
        if(OutputRect.top < InputRect.top)
        {
          Direction = OutputRect.left < (InputRect.left ? CD_RIGHTDOWN : CD_LEFTDOWN);
        }
        else
        {
          Direction = OutputRect.left < (InputRect.left ? CD_RIGHTUP : CD_LEFTUP);
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
          RECTL InputToCombinedRect;

          ClipRect.left = RectEnum.arcl[i].left + Translate.x;
          ClipRect.right = RectEnum.arcl[i].right + Translate.x;
          ClipRect.top = RectEnum.arcl[i].top + Translate.y;
          ClipRect.bottom = RectEnum.arcl[i].bottom + Translate.y;
          if (RECTL_bIntersectRect(&CombinedRect, &OutputRect, &ClipRect))
          {
            InputToCombinedRect.top = InputRect.top + (CombinedRect.top - OutputRect.top) * SrcHeight / DstHeight;
            InputToCombinedRect.bottom = InputRect.top + (CombinedRect.bottom - OutputRect.top) * SrcHeight / DstHeight;
            InputToCombinedRect.left = InputRect.left + (CombinedRect.left - OutputRect.left) * SrcWidth / DstWidth;
            InputToCombinedRect.right = InputRect.left + (CombinedRect.right - OutputRect.left) * SrcWidth / DstWidth;

            Ret = DibFunctionsForBitmapFormat[psoDest->iBitmapFormat].DIB_TransparentBlt(
              OutputObj, InputObj, &CombinedRect, &InputToCombinedRect, ColorTranslation, iTransColor);
            if(!Ret)
            {
              break;
            }
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
  RECTL InputRect;
  LONG InputClWidth, InputClHeight, InputWidth, InputHeight;

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

  InputRect = *SourceRect;
  /* Clip against the bounds of the clipping region so we won't try to write
   * outside the surface */
  if(Clip)
  {
    if(!RECTL_bIntersectRect(&OutputRect, &InputClippedRect, &Clip->rclBounds))
    {
      return TRUE;
    }
    /* Update source rect */
    InputClWidth = InputClippedRect.right - InputClippedRect.left;
    InputClHeight = InputClippedRect.bottom - InputClippedRect.top;
    InputWidth = InputRect.right - InputRect.left;
    InputHeight = InputRect.bottom - InputRect.top;

    InputRect.left += (InputWidth * (OutputRect.left - InputClippedRect.left)) / InputClWidth;
    InputRect.right -= (InputWidth * (InputClippedRect.right - OutputRect.right)) / InputClWidth;
    InputRect.top += (InputHeight * (OutputRect.top - InputClippedRect.top)) / InputClHeight;
    InputRect.bottom -= (InputHeight * (InputClippedRect.bottom - OutputRect.bottom)) / InputClHeight;
  }
  else
  {
    OutputRect = InputClippedRect;
  }

  if(psoSource != psoDest)
  {
    SURFACE_LockBitmapBits(psurfSource);
    MouseSafetyOnDrawStart(psoSource, InputRect.left, InputRect.top,
                           InputRect.right, InputRect.bottom);
  }
  SURFACE_LockBitmapBits(psurfDest);
  MouseSafetyOnDrawStart(psoDest, OutputRect.left, OutputRect.top,
                         OutputRect.right, OutputRect.bottom);

  if(psurfDest->flHooks & HOOK_TRANSPARENTBLT)
  {
    Ret = GDIDEVFUNCS(psoDest).TransparentBlt(
      psoDest, psoSource, Clip, ColorTranslation, &OutputRect,
      &InputRect, iTransColor, Reserved);
  }
  else
    Ret = FALSE;

  if(!Ret)
  {
    Ret = EngTransparentBlt(psoDest, psoSource, Clip, ColorTranslation,
                            &OutputRect, &InputRect, iTransColor, Reserved);
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
