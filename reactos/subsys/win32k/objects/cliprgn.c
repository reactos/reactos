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
/* $Id: cliprgn.c,v 1.15 2003/05/18 17:16:17 ea Exp $ */

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/dc.h>
#include <win32k/region.h>
#include <win32k/cliprgn.h>
#include <win32k/coord.h>

#define NDEBUG
#include <win32k/debug1.h>

VOID FASTCALL
CLIPPING_UpdateGCRegion(DC* Dc)
{
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
                       HRGN  hrgn)
{
  UNIMPLEMENTED;
}

int STDCALL W32kSetMetaRgn(HDC  hDC)
{
  UNIMPLEMENTED;
}


/* EOF */
