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
/* $Id: cliprgn.c,v 1.40 2004/05/26 18:49:06 weiden Exp $ */
#include <w32k.h>

int FASTCALL
CLIPPING_UpdateGCRegion(DC* Dc)
{
   PROSRGNDATA CombinedRegion;

   if (Dc->w.hGCClipRgn == NULL)
      Dc->w.hGCClipRgn = NtGdiCreateRectRgn(0, 0, 0, 0);

   if (Dc->w.hClipRgn == NULL)
      NtGdiCombineRgn(Dc->w.hGCClipRgn, Dc->w.hVisRgn, 0, RGN_COPY);
   else
      NtGdiCombineRgn(Dc->w.hGCClipRgn, Dc->w.hClipRgn, Dc->w.hVisRgn, RGN_AND);
   NtGdiOffsetRgn(Dc->w.hGCClipRgn, Dc->w.DCOrgX, Dc->w.DCOrgY);

   CombinedRegion = RGNDATA_LockRgn(Dc->w.hGCClipRgn);
   ASSERT(CombinedRegion != NULL);

   if (Dc->CombinedClip != NULL)
      IntEngDeleteClipRegion(Dc->CombinedClip);

   Dc->CombinedClip = IntEngCreateClipRegion(
      CombinedRegion->rdh.nCount,
      (PRECTL)CombinedRegion->Buffer,
      (PRECTL)&CombinedRegion->rdh.rcBound);
   ASSERT(Dc->CombinedClip != NULL);

   RGNDATA_UnlockRgn(Dc->w.hGCClipRgn);
   return NtGdiOffsetRgn(Dc->w.hGCClipRgn, -Dc->w.DCOrgX, -Dc->w.DCOrgY);
}

HRGN WINAPI SaveVisRgn(HDC hdc)
{
  HRGN copy;
  PROSRGNDATA obj, copyObj;
  PDC dc = DC_LockDc(hdc);

  if (!dc) return 0;

  obj = RGNDATA_LockRgn(dc->w.hVisRgn);

  if(!(copy = NtGdiCreateRectRgn(0, 0, 0, 0)))
  {
    RGNDATA_UnlockRgn(dc->w.hVisRgn);
    DC_UnlockDc(hdc);
    return 0;
  }
  NtGdiCombineRgn(copy, dc->w.hVisRgn, 0, RGN_COPY);
  copyObj = RGNDATA_LockRgn(copy);
/*  copyObj->header.hNext = obj->header.hNext;
  header.hNext = copy; */

  return copy;
}

INT STDCALL
NtGdiSelectVisRgn(HDC hdc, HRGN hrgn)
{
  int retval;
  DC *dc;

  if (!hrgn)
  {
  	SetLastWin32Error(ERROR_INVALID_PARAMETER);
  	return ERROR;
  }
  if (!(dc = DC_LockDc(hdc)))
  {
  	SetLastWin32Error(ERROR_INVALID_HANDLE);
  	return ERROR;
  }

  dc->w.flags &= ~DC_DIRTY;

  if (dc->w.hVisRgn == NULL)
    {
      dc->w.hVisRgn = NtGdiCreateRectRgn(0, 0, 0, 0);
      GDIOBJ_CopyOwnership(hdc, dc->w.hVisRgn);
    }

  retval = NtGdiCombineRgn(dc->w.hVisRgn, hrgn, 0, RGN_COPY);
  CLIPPING_UpdateGCRegion(dc);
  DC_UnlockDc( hdc );

  return retval;
}

int STDCALL NtGdiExtSelectClipRgn(HDC  hDC,
                          HRGN  hrgn,
                          int  fnMode)
{
  int retval;
  DC *dc;

  if (!(dc = DC_LockDc(hDC)))
  {
  	SetLastWin32Error(ERROR_INVALID_HANDLE);
  	return ERROR;
  }
  
//  dc->w.flags &= ~DC_DIRTY;
  
  if (!hrgn)
  {
    if (fnMode == RGN_COPY)
    {
      if (dc->w.hClipRgn != NULL)
      {
        NtGdiDeleteObject(dc->w.hClipRgn);
        dc->w.hClipRgn = NULL;
        retval = NULLREGION;
      }
    }
    else
    {
      DC_UnlockDc( hDC );
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return ERROR;
    }
  }
  else
  {
    if (!dc->w.hClipRgn)
    {
      PROSRGNDATA Rgn;
      RECT rect;
      if((Rgn = RGNDATA_LockRgn(dc->w.hVisRgn)))
      {
        UnsafeIntGetRgnBox(Rgn, &rect);
        RGNDATA_UnlockRgn(dc->w.hVisRgn);
        dc->w.hClipRgn = UnsafeIntCreateRectRgnIndirect(&rect);
      }
      else
      {
        dc->w.hClipRgn = NtGdiCreateRectRgn(0, 0, 0, 0);
      }
    }
    if(fnMode == RGN_COPY)
    {
      NtGdiCombineRgn(dc->w.hClipRgn, hrgn, 0, fnMode);
    }
    else
      NtGdiCombineRgn(dc->w.hClipRgn, dc->w.hClipRgn, hrgn, fnMode);
  }

  retval = CLIPPING_UpdateGCRegion(dc);
  DC_UnlockDc( hDC );

  return retval;
}

INT FASTCALL
IntGdiGetClipBox(HDC hDC, LPRECT rc)
{
   PROSRGNDATA Rgn;
   INT retval;
   PDC dc;

   if (!(dc = DC_LockDc(hDC)))
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return ERROR;
   }

   if (!(Rgn = RGNDATA_LockRgn(dc->w.hGCClipRgn)))
   {
      DC_UnlockDc( hDC );
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return ERROR;
   }
   retval = UnsafeIntGetRgnBox(Rgn, rc);
   RGNDATA_UnlockRgn(dc->w.hGCClipRgn);
   IntDPtoLP(dc, (LPPOINT)rc, 2);
   DC_UnlockDc(hDC);

   return retval;
}

int STDCALL NtGdiGetClipBox(HDC  hDC,
			   LPRECT  rc)
{
  int Ret;
  NTSTATUS Status;
  RECT Saferect;
  
  Ret = IntGdiGetClipBox(hDC, &Saferect);
  
  Status = MmCopyToCaller(rc, &Saferect, sizeof(RECT));
  if(!NT_SUCCESS(Status))
  {
  
    SetLastNtError(Status);
    return ERROR;
  }
  
  return Ret;
}

int STDCALL NtGdiGetMetaRgn(HDC  hDC,
                    HRGN  hrgn)
{
  UNIMPLEMENTED;
}

int STDCALL NtGdiExcludeClipRect(HDC  hDC,
                         int  LeftRect,
                         int  TopRect,
                         int  RightRect,
                         int  BottomRect)
{
   INT Result;
   RECT Rect;
   HRGN NewRgn;
   PDC dc = DC_LockDc(hDC);

   if (!dc)
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return ERROR;
   }

   Rect.left = LeftRect;
   Rect.top = TopRect;
   Rect.right = RightRect;
   Rect.bottom = BottomRect;

   IntLPtoDP(dc, (LPPOINT)&Rect, 2);

   NewRgn = UnsafeIntCreateRectRgnIndirect(&Rect);
   if (!NewRgn)
   {
      Result = ERROR;
   }
   else if (!dc->w.hClipRgn)
   {
      dc->w.hClipRgn = NewRgn;
      Result = SIMPLEREGION;
   }
   else
   {
      Result = NtGdiCombineRgn(dc->w.hClipRgn, dc->w.hClipRgn, NewRgn, RGN_DIFF);
      NtGdiDeleteObject(NewRgn);
   }
   if (Result != ERROR)
      CLIPPING_UpdateGCRegion(dc);

   DC_UnlockDc(hDC);

   return Result;
}

int STDCALL NtGdiIntersectClipRect(HDC  hDC,
                           int  LeftRect,
                           int  TopRect,
                           int  RightRect,
                           int  BottomRect)
{
   INT Result;
   RECT Rect;
   HRGN NewRgn;
   PDC dc = DC_LockDc(hDC);

   DPRINT("NtGdiIntersectClipRect(%x, %d,%d-%d,%d)\n",
      hDC, LeftRect, TopRect, RightRect, BottomRect);

   if (!dc)
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return ERROR;
   }

   Rect.left = LeftRect;
   Rect.top = TopRect;
   Rect.right = RightRect;
   Rect.bottom = BottomRect;

   IntLPtoDP(dc, (LPPOINT)&Rect, 2);

   NewRgn = UnsafeIntCreateRectRgnIndirect(&Rect);
   if (!NewRgn)
   {
      Result = ERROR;
   }
   else if (!dc->w.hClipRgn)
   {
      dc->w.hClipRgn = NewRgn;
      Result = SIMPLEREGION;
   }
   else
   {
      Result = NtGdiCombineRgn(dc->w.hClipRgn, dc->w.hClipRgn, NewRgn, RGN_AND);
      NtGdiDeleteObject(NewRgn);
   }
   if (Result != ERROR)
      CLIPPING_UpdateGCRegion(dc);

   DC_UnlockDc(hDC);

   return Result;
}

int STDCALL NtGdiOffsetClipRgn(HDC  hDC,
                       int  XOffset,
                       int  YOffset)
{
  UNIMPLEMENTED;
}

BOOL STDCALL NtGdiPtVisible(HDC  hDC,
                    int  X,
                    int  Y)
{
  HRGN rgn;
  DC *dc;
  
  if(!(dc = DC_LockDc(hDC)))
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  
  rgn = dc->w.hGCClipRgn;
  DC_UnlockDc(hDC);
  
  return (rgn ? NtGdiPtInRegion(rgn, X, Y) : FALSE);
}

BOOL STDCALL NtGdiRectVisible(HDC  hDC,
                      CONST PRECT  UnsafeRect)
{
   NTSTATUS Status;
   PROSRGNDATA Rgn;
   PDC dc = DC_LockDc(hDC);
   BOOL Result = FALSE;
   RECT Rect;

   if (!dc)
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
   }

   Status = MmCopyFromCaller(&Rect, UnsafeRect, sizeof(RECT));
   if(!NT_SUCCESS(Status))
   {
      DC_UnlockDc(hDC);
      return FALSE;
   }

   if (dc->w.hGCClipRgn)
   {
      if((Rgn = (PROSRGNDATA)RGNDATA_LockRgn(dc->w.hGCClipRgn)))
      {
         IntLPtoDP(dc, (LPPOINT)&Rect, 2);
         Result = UnsafeIntRectInRegion(Rgn, &Rect);
         RGNDATA_UnlockRgn(dc->w.hGCClipRgn);
      }
   }
   DC_UnlockDc(hDC);

   return Result;
}

BOOL STDCALL NtGdiSelectClipPath(HDC  hDC,
                         int  Mode)
{
  UNIMPLEMENTED;
}

INT STDCALL
NtGdiSelectClipRgn(HDC hDC, HRGN hRgn)
{
   return NtGdiExtSelectClipRgn(hDC, hRgn, RGN_COPY);
}

int STDCALL NtGdiSetMetaRgn(HDC  hDC)
{
  UNIMPLEMENTED;
}

/* EOF */
