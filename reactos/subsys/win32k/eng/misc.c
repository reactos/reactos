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
/* $Id: misc.c,v 1.2 2003/05/18 17:16:17 ea Exp $ */
#include <ddk/winddi.h>
#include <include/dib.h>
#include <include/object.h>
#include <include/surface.h>
#include "misc.h"
#include "objects.h"

BOOL STDCALL
IntEngEnter(PINTENG_ENTER_LEAVE EnterLeave,
            SURFOBJ *DestObj,
            RECTL *DestRect,
            BOOL ReadOnly,
            POINTL *Translate,
            SURFOBJ **OutputObj)
{
  LONG Exchange;
  SIZEL BitmapSize;
  POINTL SrcPoint;
  LONG Width;

  /* Normalize */
  if (DestRect->right < DestRect->left)
    {
    Exchange = DestRect->left;
    DestRect->left = DestRect->right;
    DestRect->right = Exchange;
    }
  if (DestRect->bottom < DestRect->top)
    {
    Exchange = DestRect->top;
    DestRect->top = DestRect->bottom;
    DestRect->bottom = Exchange;
    }

  if (NULL != DestObj && STYPE_BITMAP != DestObj->iType &&
      (NULL == DestObj->pvScan0 || 0 == DestObj->lDelta))
    {
    EnterLeave->DestGDI = (SURFGDI*)AccessInternalObjectFromUserObject(DestObj);
    /* Driver needs to support DrvCopyBits, else we can't do anything */
    if (NULL == EnterLeave->DestGDI->CopyBits)
      {
      return FALSE;
      }

    /* Allocate a temporary bitmap */
    BitmapSize.cx = DestRect->right - DestRect->left;
    BitmapSize.cy = DestRect->bottom - DestRect->top;
    Width = DIB_GetDIBWidthBytes(BitmapSize.cx, BitsPerFormat(DestObj->iBitmapFormat));
    EnterLeave->OutputBitmap = EngCreateBitmap(BitmapSize, Width,
                                               DestObj->iBitmapFormat,
                                               BMF_NOZEROINIT, NULL);
    *OutputObj = (SURFOBJ *) AccessUserObject((ULONG) EnterLeave->OutputBitmap);

    EnterLeave->DestRect.left = 0;
    EnterLeave->DestRect.top = 0;
    EnterLeave->DestRect.right = BitmapSize.cx;
    EnterLeave->DestRect.bottom = BitmapSize.cy;
    SrcPoint.x = DestRect->left;
    SrcPoint.y = DestRect->top;
    EnterLeave->TrivialClipObj = EngCreateClip();
    EnterLeave->TrivialClipObj->iDComplexity = DC_TRIVIAL;
    if (! EnterLeave->DestGDI->CopyBits(*OutputObj, DestObj,
                                        EnterLeave->TrivialClipObj, NULL,
                                        &EnterLeave->DestRect, &SrcPoint))
      {
      EngDeleteClip(EnterLeave->TrivialClipObj);
      EngFreeMem((*OutputObj)->pvBits);
      EngDeleteSurface(EnterLeave->OutputBitmap);
      return FALSE;
      }
    EnterLeave->DestRect.left = DestRect->left;
    EnterLeave->DestRect.top = DestRect->top;
    EnterLeave->DestRect.right = DestRect->right;
    EnterLeave->DestRect.bottom = DestRect->bottom;
    Translate->x = - DestRect->left;
    Translate->y = - DestRect->top;
    }
  else
    {
    Translate->x = 0;
    Translate->y = 0;
    *OutputObj = DestObj;
    }

  EnterLeave->DestObj = DestObj;
  EnterLeave->OutputObj = *OutputObj;
  EnterLeave->ReadOnly = ReadOnly;

  return TRUE;
}

BOOL STDCALL
IntEngLeave(PINTENG_ENTER_LEAVE EnterLeave)
{
  POINTL SrcPoint;
  BOOL Result;

  if (EnterLeave->OutputObj != EnterLeave->DestObj && NULL != EnterLeave->OutputObj)
    {
    if (! EnterLeave->ReadOnly)
      {
      SrcPoint.x = 0;
      SrcPoint.y = 0;
      Result = EnterLeave->DestGDI->CopyBits(EnterLeave->DestObj,
                                             EnterLeave->OutputObj,
                                             EnterLeave->TrivialClipObj, NULL,
                                             &EnterLeave->DestRect, &SrcPoint);
      }
    EngFreeMem(EnterLeave->OutputObj->pvBits);
    EngDeleteSurface(EnterLeave->OutputBitmap);
    EngDeleteClip(EnterLeave->TrivialClipObj);
    }
  else
    {
    Result = TRUE;
    }

  return Result;
}
/* EOF */
