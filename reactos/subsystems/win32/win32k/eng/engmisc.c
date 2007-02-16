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
/* $Id$ */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

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
  RECTL ClippedDestRect;

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
    /* Driver needs to support DrvCopyBits, else we can't do anything */
    /* FIXME: Remove typecast! */
    if (!(((BITMAPOBJ*)DestObj)->flHooks & HOOK_COPYBITS))
      {
      return FALSE;
      }

    /* Allocate a temporary bitmap */
    BitmapSize.cx = DestRect->right - DestRect->left;
    BitmapSize.cy = DestRect->bottom - DestRect->top;
    Width = DIB_GetDIBWidthBytes(BitmapSize.cx, BitsPerFormat(DestObj->iBitmapFormat));
    EnterLeave->OutputBitmap = EngCreateBitmap(BitmapSize, Width,
                                               DestObj->iBitmapFormat,
                                               BMF_TOPDOWN | BMF_NOZEROINIT, NULL);
                                               
    if (!EnterLeave->OutputBitmap)
      {
      DPRINT1("EngCreateBitmap() failed\n");
      return FALSE;
      }
      
    *OutputObj = EngLockSurface((HSURF)EnterLeave->OutputBitmap);

    EnterLeave->DestRect.left = 0;
    EnterLeave->DestRect.top = 0;
    EnterLeave->DestRect.right = BitmapSize.cx;
    EnterLeave->DestRect.bottom = BitmapSize.cy;
    SrcPoint.x = DestRect->left;
    SrcPoint.y = DestRect->top;
    ClippedDestRect = EnterLeave->DestRect;
    if (SrcPoint.x < 0)
      {
        ClippedDestRect.left -= SrcPoint.x;
        SrcPoint.x = 0;
      }
    if (DestObj->sizlBitmap.cx < SrcPoint.x + ClippedDestRect.right - ClippedDestRect.left)
      {
        ClippedDestRect.right = ClippedDestRect.left + DestObj->sizlBitmap.cx - SrcPoint.x;
      }
    if (SrcPoint.y < 0)
      {
        ClippedDestRect.top -= SrcPoint.y;
        SrcPoint.y = 0;
      }
    if (DestObj->sizlBitmap.cy < SrcPoint.y + ClippedDestRect.bottom - ClippedDestRect.top)
      {
        ClippedDestRect.bottom = ClippedDestRect.top + DestObj->sizlBitmap.cy - SrcPoint.y;
      }
    EnterLeave->TrivialClipObj = EngCreateClip();
    EnterLeave->TrivialClipObj->iDComplexity = DC_TRIVIAL;
    if (ClippedDestRect.left < (*OutputObj)->sizlBitmap.cx &&
        0 <= ClippedDestRect.right &&
        SrcPoint.x < DestObj->sizlBitmap.cx &&
        ClippedDestRect.top <= (*OutputObj)->sizlBitmap.cy &&
        0 <= ClippedDestRect.bottom &&
        SrcPoint.y < DestObj->sizlBitmap.cy &&
        ! GDIDEVFUNCS(DestObj).CopyBits(
                                        *OutputObj, DestObj,
                                        EnterLeave->TrivialClipObj, NULL,
                                        &ClippedDestRect, &SrcPoint))
      {
      EngDeleteClip(EnterLeave->TrivialClipObj);
      EngFreeMem((*OutputObj)->pvBits);
      EngUnlockSurface(*OutputObj);
      EngDeleteSurface((HSURF)EnterLeave->OutputBitmap);
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

  if (NULL != *OutputObj
      && 0 != (((BITMAPOBJ*) *OutputObj)->flHooks & HOOK_SYNCHRONIZE))
    {
      if (NULL != GDIDEVFUNCS(*OutputObj).SynchronizeSurface)
        {
          GDIDEVFUNCS(*OutputObj).SynchronizeSurface(*OutputObj, DestRect, 0);
        }
      else if (STYPE_BITMAP == (*OutputObj)->iType
               && NULL != GDIDEVFUNCS(*OutputObj).Synchronize)
        {
          GDIDEVFUNCS(*OutputObj).Synchronize((*OutputObj)->dhpdev, DestRect);
        }
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
  BOOL Result = TRUE;

  if (EnterLeave->OutputObj != EnterLeave->DestObj && NULL != EnterLeave->OutputObj)
    {
    if (! EnterLeave->ReadOnly)
      {
      SrcPoint.x = 0;
      SrcPoint.y = 0;
      if (EnterLeave->DestRect.left < 0)
        {
          SrcPoint.x = - EnterLeave->DestRect.left;
          EnterLeave->DestRect.left = 0;
        }
      if (EnterLeave->DestObj->sizlBitmap.cx < EnterLeave->DestRect.right)
        {
          EnterLeave->DestRect.right = EnterLeave->DestObj->sizlBitmap.cx;
        }
      if (EnterLeave->DestRect.top < 0)
        {
          SrcPoint.y = - EnterLeave->DestRect.top;
          EnterLeave->DestRect.top = 0;
        }
      if (EnterLeave->DestObj->sizlBitmap.cy < EnterLeave->DestRect.bottom)
        {
          EnterLeave->DestRect.bottom = EnterLeave->DestObj->sizlBitmap.cy;
        }
      if (SrcPoint.x < EnterLeave->OutputObj->sizlBitmap.cx &&
          EnterLeave->DestRect.left <= EnterLeave->DestRect.right &&
          EnterLeave->DestRect.left < EnterLeave->DestObj->sizlBitmap.cx &&
          SrcPoint.y < EnterLeave->OutputObj->sizlBitmap.cy &&
          EnterLeave->DestRect.top <= EnterLeave->DestRect.bottom &&
          EnterLeave->DestRect.top < EnterLeave->DestObj->sizlBitmap.cy)
        {
          Result = GDIDEVFUNCS(EnterLeave->DestObj).CopyBits(
                                                 EnterLeave->DestObj,
                                                 EnterLeave->OutputObj,
                                                 EnterLeave->TrivialClipObj, NULL,
                                                 &EnterLeave->DestRect, &SrcPoint);
        }
      else
        {
          Result = TRUE;
        }
      }
    EngFreeMem(EnterLeave->OutputObj->pvBits);
    EngUnlockSurface(EnterLeave->OutputObj);
    EngDeleteSurface((HSURF)EnterLeave->OutputBitmap);
    EngDeleteClip(EnterLeave->TrivialClipObj);
    }
  else
    {
    Result = TRUE;
    }

  return Result;
}

HANDLE STDCALL
EngGetCurrentProcessId(VOID)
{
  /* http://www.osr.com/ddk/graphics/gdifncs_5ovb.htm */
  return PsGetCurrentProcessId();
}

HANDLE STDCALL
EngGetCurrentThreadId(VOID)
{
  /* http://www.osr.com/ddk/graphics/gdifncs_25rb.htm */
  return PsGetCurrentThreadId();
}

HANDLE STDCALL
EngGetProcessHandle(VOID)
{
  /* http://www.osr.com/ddk/graphics/gdifncs_3tif.htm
     In Windows 2000 and later, the EngGetProcessHandle function always returns NULL.
     FIXME - what does NT4 return? */
  return NULL;
}

/* EOF */
