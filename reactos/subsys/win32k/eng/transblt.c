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
/* $Id: transblt.c,v 1.13 2004/04/03 21:25:20 weiden Exp $
 * 
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          GDI TransparentBlt Function
 * FILE:             subsys/win32k/eng/transblt.c
 * PROGRAMER:        Jason Filby
 * REVISION HISTORY:
 *        4/6/2001: Created
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
		  ULONG TransparentColor,
		  ULONG Reserved)
{
  DPRINT1("EngTransparentBlt() unimplemented!\n");
  return FALSE;
}

BOOL FASTCALL
IntTransparentBlt(PSURFOBJ Dest,
                  PSURFOBJ Source,
                  PCLIPOBJ Clip,
                  PXLATEOBJ ColorTranslation,
                  PRECTL DestRect,
                  PRECTL SourceRect,
                  ULONG TransparentColor,
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
    Ret = SurfGDIDest->TransparentBlt(Dest, Source, Clip, ColorTranslation, &OutputRect, 
                                      SourceRect, TransparentColor, Reserved);
  }
  else
    Ret = FALSE;
  
  if(!Ret)
  {
    Ret = EngTransparentBlt(Dest, Source, Clip, ColorTranslation, &OutputRect, 
                            SourceRect, TransparentColor, Reserved);
  }
  
  MouseSafetyOnDrawEnd(Dest, SurfGDIDest);
  if(Source != Dest)
  {
    MouseSafetyOnDrawEnd(Source, SurfGDISrc);
  }

  return Ret;
}

/* EOF */
