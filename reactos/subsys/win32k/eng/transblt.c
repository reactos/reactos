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
/* $Id: transblt.c,v 1.16 2004/04/07 22:17:36 weiden Exp $
 * 
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          GDI TransparentBlt Function
 * FILE:             subsys/win32k/eng/transblt.c
 * PROGRAMER:        Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * REVISION HISTORY:
 *        4/6/2004: Created
 */

#include <ddk/winddi.h>
#include <ddk/ntddk.h>
#include <ddk/ntddmou.h>
#include <ntos/minmax.h>
#include <include/dib.h>
#include <include/eng.h>
#include <include/object.h>
#include <include/surface.h>
#include <include/mouse.h>
#include <include/inteng.h>

#include "brush.h"
#include "clip.h"
#include "objects.h"

#define NDEBUG
#include <win32k/debug1.h>


BOOL STDCALL
EngTransparentBlt(PSURFOBJ Dest,
		  PSURFOBJ Source,
		  PCLIPOBJ Clip,
		  PXLATEOBJ ColorTranslation,
		  PRECTL DestRect,
		  PRECTL SourceRect,
		  ULONG iTransColor,
		  ULONG Reserved)
{
  BOOL Ret;
  BYTE ClippingType;
  INTENG_ENTER_LEAVE EnterLeaveSource, EnterLeaveDest;
  SURFOBJ *InputObj, *OutputObj;
  SURFGDI *InputGDI, *OutputGDI;
  RECTL OutputRect, InputRect;
  POINTL Translate, InputPoint;
  
  InputRect.left = 0;
  InputRect.right = DestRect->right - DestRect->left;
  InputRect.top = 0;
  InputRect.bottom = DestRect->bottom - DestRect->top;
  
  if(!IntEngEnter(&EnterLeaveSource, Source, &InputRect, TRUE, &Translate, &InputObj))
  {
    return FALSE;
  }
  
  InputPoint.x = SourceRect->left + Translate.x;
  InputPoint.y = SourceRect->top + Translate.y;
  
  InputGDI = (InputObj ? (SURFGDI*)AccessInternalObjectFromUserObject(InputObj) : NULL);
  ASSERT(InputGDI);
  
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
  
  if(!IntEngEnter(&EnterLeaveDest, Dest, &OutputRect, FALSE, &Translate, &OutputObj))
  {
    IntEngLeave(&EnterLeaveSource);
    return FALSE;
  }
  
  OutputRect.left = DestRect->left + Translate.x;
  OutputRect.right = DestRect->right + Translate.x;
  OutputRect.top = DestRect->top + Translate.y;
  OutputRect.bottom = DestRect->bottom + Translate.y;
  
  OutputGDI = (OutputObj ? (SURFGDI*)AccessInternalObjectFromUserObject(OutputObj) : NULL);
  ASSERT(OutputGDI);
  
  ClippingType = (Clip ? Clip->iDComplexity : DC_TRIVIAL);
  
  switch(ClippingType)
  {
    case DC_TRIVIAL:
    {
      Ret = OutputGDI->DIB_TransparentBlt(OutputObj, InputObj, OutputGDI, InputGDI, &OutputRect, 
                                         &InputPoint, ColorTranslation, iTransColor);
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
      Ret = OutputGDI->DIB_TransparentBlt(OutputObj, InputObj, OutputGDI, InputGDI, &CombinedRect, 
                                          &Pt, ColorTranslation, iTransColor);
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
          Ret = OutputGDI->DIB_TransparentBlt(OutputObj, InputObj, OutputGDI, InputGDI, &CombinedRect, 
                                              &Pt, ColorTranslation, iTransColor);
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
IntEngTransparentBlt(PSURFOBJ Dest,
                     PSURFOBJ Source,
                     PCLIPOBJ Clip,
                     PXLATEOBJ ColorTranslation,
                     PRECTL DestRect,
                     PRECTL SourceRect,
                     ULONG iTransColor,
                     ULONG Reserved)
{
  BOOL Ret;
  RECTL OutputRect, InputClippedRect;
  PSURFGDI SurfGDIDest, SurfGDISrc;
  
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
  
  if(Source != Dest)
  {
    SurfGDISrc = (PSURFGDI)AccessInternalObjectFromUserObject(Source);
    ASSERT(SurfGDISrc);
    MouseSafetyOnDrawStart(Source, SurfGDISrc, SourceRect->left, SourceRect->top, 
                           (SourceRect->left + abs(SourceRect->right - SourceRect->left)),
                           (SourceRect->top + abs(SourceRect->bottom - SourceRect->top)));
  }
  SurfGDIDest = (PSURFGDI)AccessInternalObjectFromUserObject(Dest);
  ASSERT(SurfGDIDest);
  MouseSafetyOnDrawStart(Dest, SurfGDIDest, OutputRect.left, OutputRect.top,
                         OutputRect.right, OutputRect.bottom);
  
  if(SurfGDIDest->TransparentBlt)
  {
    IntLockGDIDriver(SurfGDIDest);
    Ret = SurfGDIDest->TransparentBlt(Dest, Source, Clip, ColorTranslation, &OutputRect, 
                                      SourceRect, iTransColor, Reserved);
    IntUnLockGDIDriver(SurfGDIDest);
  }
  else
    Ret = FALSE;
  
  if(!Ret)
  {
    Ret = EngTransparentBlt(Dest, Source, Clip, ColorTranslation, &OutputRect, 
                            SourceRect, iTransColor, Reserved);
  }
  
  if(Ret)
  {
    /* Dummy BitBlt to let driver know that something has changed.
       0x00AA0029 is the Rop for D (no-op) */
    if(SurfGDIDest->BitBlt)
    {
      IntLockGDIDriver(SurfGDIDest);
      SurfGDIDest->BitBlt(Dest, NULL, NULL, Clip, ColorTranslation,
                          &OutputRect, NULL, NULL, NULL, NULL, ROP_NOOP);
      IntUnLockGDIDriver(SurfGDIDest);
    }
    else
      EngBitBlt(Dest, NULL, NULL, Clip, ColorTranslation,
                &OutputRect, NULL, NULL, NULL, NULL, ROP_NOOP);
  }
  
  MouseSafetyOnDrawEnd(Dest, SurfGDIDest);
  if(Source != Dest)
  {
    MouseSafetyOnDrawEnd(Source, SurfGDISrc);
  }

  return Ret;
}

/* EOF */
