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
/* $Id: cliprgn.c,v 1.17 2003/06/28 08:39:18 gvg Exp $ */

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/dc.h>
#include <win32k/region.h>
#include <win32k/cliprgn.h>
#include <win32k/coord.h>
#include <include/error.h>
#include "../eng/clip.h"

#define NDEBUG
#include <win32k/debug1.h>

VOID FASTCALL
CLIPPING_UpdateGCRegion(DC* Dc)
{
  HRGN Combined;
  PROSRGNDATA CombinedRegion;

#ifndef TODO
  if (Dc->w.hGCClipRgn == NULL)
    {
      Dc->w.hGCClipRgn = W32kCreateRectRgn(0, 0, 0, 0);
    }

  if (Dc->w.hClipRgn == NULL)
    {
      W32kCombineRgn(Dc->w.hGCClipRgn, Dc->w.hVisRgn, 0, RGN_COPY);
    }
  else
    {
      W32kCombineRgn(Dc->w.hGCClipRgn, Dc->w.hClipRgn, Dc->w.hVisRgn,
		     RGN_AND);
    }
#endif

  Combined = W32kCreateRectRgn(0, 0, 0, 0);
  ASSERT(NULL != Combined);

  if (Dc->w.hClipRgn == NULL)
    {
      W32kCombineRgn(Combined, Dc->w.hVisRgn, 0, RGN_COPY);
    }
  else
    {
      W32kCombineRgn(Combined, Dc->w.hClipRgn, Dc->w.hVisRgn,
		     RGN_AND);
    }

  CombinedRegion = RGNDATA_LockRgn(Combined);
  ASSERT(NULL != CombinedRegion);

  if (NULL != Dc->CombinedClip)
    {
      IntEngDeleteClipRegion(Dc->CombinedClip);
    }

  Dc->CombinedClip = IntEngCreateClipRegion(CombinedRegion->rdh.nCount,
                                            CombinedRegion->Buffer,
                                            CombinedRegion->rdh.rcBound);
  ASSERT(NULL != Dc->CombinedClip);

  RGNDATA_UnlockRgn(Combined);
  W32kDeleteObject(Combined);
}

HRGN WINAPI SaveVisRgn(HDC hdc)
{
  HRGN copy;
  PROSRGNDATA obj, copyObj;
  PDC dc = DC_HandleToPtr(hdc);

  if (!dc) return 0;

  obj = RGNDATA_LockRgn(dc->w.hVisRgn);

  if(!(copy = W32kCreateRectRgn(0, 0, 0, 0)))
  {
    RGNDATA_UnlockRgn(dc->w.hVisRgn);
    DC_ReleasePtr(hdc);
    return 0;
  }
  W32kCombineRgn(copy, dc->w.hVisRgn, 0, RGN_COPY);
  copyObj = RGNDATA_LockRgn(copy);
/*  copyObj->header.hNext = obj->header.hNext;
  header.hNext = copy; */

  return copy;
}

INT STDCALL
W32kSelectVisRgn(HDC hdc, HRGN hrgn)
{
  int retval;
  DC *dc;

  if (!hrgn)
  	return ERROR;
  if (!(dc = DC_HandleToPtr(hdc)))
  	return ERROR;

  dc->w.flags &= ~DC_DIRTY;

  retval = W32kCombineRgn(dc->w.hVisRgn, hrgn, 0, RGN_COPY);
  CLIPPING_UpdateGCRegion(dc);
  DC_ReleasePtr( hdc );

  return retval;
}

int STDCALL W32kExcludeClipRect(HDC  hDC,
                         int  LeftRect,
                         int  TopRect,
                         int  RightRect,
                         int  BottomRect)
{
  UNIMPLEMENTED;
}

int STDCALL W32kExtSelectClipRgn(HDC  hDC,
                          HRGN  hrgn,
                          int  fnMode)
{
  UNIMPLEMENTED;
}

int STDCALL W32kGetClipBox(HDC  hDC,
			   LPRECT  rc)
{
  int retval;
  DC *dc;

  if (!(dc = DC_HandleToPtr(hDC)))
  	return ERROR;
  retval = UnsafeW32kGetRgnBox(dc->w.hGCClipRgn, rc);
  rc->left -= dc->w.DCOrgX;
  rc->right -= dc->w.DCOrgX;
  rc->top -= dc->w.DCOrgY;
  rc->bottom -= dc->w.DCOrgY;

  DC_ReleasePtr( hDC );
  W32kDPtoLP(hDC, (LPPOINT)rc, 2);
  return(retval);
}

int STDCALL W32kGetMetaRgn(HDC  hDC,
                    HRGN  hrgn)
{
  UNIMPLEMENTED;
}

int STDCALL W32kIntersectClipRect(HDC  hDC,
                           int  LeftRect,
                           int  TopRect,
                           int  RightRect,
                           int  BottomRect)
{
  UNIMPLEMENTED;
}

int STDCALL W32kOffsetClipRgn(HDC  hDC,
                       int  XOffset,
                       int  YOffset)
{
  UNIMPLEMENTED;
}

BOOL STDCALL W32kPtVisible(HDC  hDC,
                    int  X,
                    int  Y)
{
  UNIMPLEMENTED;
}

BOOL STDCALL W32kRectVisible(HDC  hDC,
                      CONST PRECT  rc)
{
  UNIMPLEMENTED;
}

BOOL STDCALL W32kSelectClipPath(HDC  hDC,
                         int  Mode)
{
  UNIMPLEMENTED;
}

int STDCALL W32kSelectClipRgn(HDC  hDC,
                              HRGN hRgn)
{
  int Type;
  PDC dc;
  HRGN Copy;

  dc = DC_HandleToPtr(hDC);
  if (NULL == dc)
    {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return ERROR;
    }

  if (NULL != hRgn)
    {
      Copy = W32kCreateRectRgn(0, 0, 0, 0);
      if (NULL == Copy)
	{
	  return ERROR;
	}
      Type = W32kCombineRgn(Copy, hRgn, 0, RGN_COPY);
      if (ERROR == Type)
	{
	  W32kDeleteObject(Copy);
	  return ERROR;
	}
      W32kOffsetRgn(Copy, dc->w.DCOrgX, dc->w.DCOrgY);
    }
  else
    {
      Copy = NULL;
    }

  if (NULL != dc->w.hClipRgn)
    {
      W32kDeleteObject(dc->w.hClipRgn);
    }
  dc->w.hClipRgn = Copy;
  CLIPPING_UpdateGCRegion(dc);

  return ERROR;
}

int STDCALL W32kSetMetaRgn(HDC  hDC)
{
  UNIMPLEMENTED;
}


/* EOF */
