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

   if((CombinedRegion = RGNDATA_LockRgn(Dc->w.hGCClipRgn)))
   {
     if (Dc->CombinedClip != NULL)
        IntEngDeleteClipRegion(Dc->CombinedClip);

     Dc->CombinedClip = IntEngCreateClipRegion(
        CombinedRegion->rdh.nCount,
        (PRECTL)CombinedRegion->Buffer,
        (PRECTL)&CombinedRegion->rdh.rcBound);

     RGNDATA_UnlockRgn(CombinedRegion);
   }

   if ( NULL == Dc->CombinedClip )
   {
	   DPRINT1("IntEngCreateClipRegion() failed\n");
	   return ERROR;
   }

   return NtGdiOffsetRgn(Dc->w.hGCClipRgn, -Dc->w.DCOrgX, -Dc->w.DCOrgY);
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
      GDIOBJ_CopyOwnership(GdiHandleTable, hdc, dc->w.hVisRgn);
    }
  else
    {
      NtGdiOffsetRgn(dc->w.hVisRgn, dc->w.DCOrgX, dc->w.DCOrgY);
    }

  retval = NtGdiCombineRgn(dc->w.hVisRgn, hrgn, 0, RGN_COPY);
  if ( retval != ERROR )
    {
      NtGdiOffsetRgn(dc->w.hVisRgn, -dc->w.DCOrgX, -dc->w.DCOrgY);
      CLIPPING_UpdateGCRegion(dc);
    }
  DC_UnlockDc(dc);

  return retval;
}


int STDCALL IntGdiExtSelectClipRgn(PDC dc, 
                                HRGN hrgn, 
                                int fnMode)
{
  int retval;
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
        RGNDATA_UnlockRgn(Rgn);
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

  retval = IntGdiExtSelectClipRgn ( dc, hrgn, fnMode );

  DC_UnlockDc(dc);
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
      DC_UnlockDc(dc);
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return ERROR;
   }
   retval = UnsafeIntGetRgnBox(Rgn, rc);
   RGNDATA_UnlockRgn(Rgn);
   IntDPtoLP(dc, (LPPOINT)rc, 2);
   DC_UnlockDc(dc);

   return retval;
}

int STDCALL NtGdiGetClipBox(HDC  hDC,
			   LPRECT  rc)
{
  int Ret;
  NTSTATUS Status = STATUS_SUCCESS;
  RECT Saferect;

  Ret = IntGdiGetClipBox(hDC, &Saferect);

  _SEH_TRY
  {
    ProbeForWrite(rc,
                  sizeof(RECT),
                  1);
    *rc = Saferect;
  }
  _SEH_HANDLE
  {
    Status = _SEH_GetExceptionCode();
  }
  _SEH_END;

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
  return 0;
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
   else
   {
      if (!dc->w.hClipRgn)
      {
         dc->w.hClipRgn = NtGdiCreateRectRgn(0, 0, 0, 0);
         NtGdiCombineRgn(dc->w.hClipRgn, dc->w.hVisRgn, NewRgn, RGN_DIFF);
         Result = SIMPLEREGION;
      }
      else
      {
         Result = NtGdiCombineRgn(dc->w.hClipRgn, dc->w.hClipRgn, NewRgn, RGN_DIFF);
      }
      NtGdiDeleteObject(NewRgn);
   }
   if (Result != ERROR)
      CLIPPING_UpdateGCRegion(dc);

   DC_UnlockDc(dc);

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

   DC_UnlockDc(dc);

   return Result;
}

int STDCALL NtGdiOffsetClipRgn(HDC  hDC,
                       int  XOffset,
                       int  YOffset)
{
  INT Result;
  DC *dc;

  if(!(dc = DC_LockDc(hDC)))
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return ERROR;
  }

  if(dc->w.hClipRgn != NULL)
  {
    Result = NtGdiOffsetRgn(dc->w.hClipRgn,
                            XOffset,
                            YOffset);
    CLIPPING_UpdateGCRegion(dc);
  }
  else
  {
    Result = NULLREGION;
  }
  
  DC_UnlockDc(dc);
  return Result;
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
  DC_UnlockDc(dc);

  return (rgn ? NtGdiPtInRegion(rgn, X, Y) : FALSE);
}

BOOL STDCALL NtGdiRectVisible(HDC  hDC,
                      CONST PRECT  UnsafeRect)
{
   NTSTATUS Status = STATUS_SUCCESS;
   PROSRGNDATA Rgn;
   PDC dc = DC_LockDc(hDC);
   BOOL Result = FALSE;
   RECT Rect;

   if (!dc)
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
   }

   _SEH_TRY
   {
      ProbeForRead(UnsafeRect,
                   sizeof(RECT),
                   1);
      Rect = *UnsafeRect;
   }
   _SEH_HANDLE
   {
      Status = _SEH_GetExceptionCode();
   }
   _SEH_END;

   if(!NT_SUCCESS(Status))
   {
      DC_UnlockDc(dc);
      SetLastNtError(Status);
      return FALSE;
   }

   if (dc->w.hGCClipRgn)
   {
      if((Rgn = (PROSRGNDATA)RGNDATA_LockRgn(dc->w.hGCClipRgn)))
      {
         IntLPtoDP(dc, (LPPOINT)&Rect, 2);
         Result = UnsafeIntRectInRegion(Rgn, &Rect);
         RGNDATA_UnlockRgn(Rgn);
      }
   }
   DC_UnlockDc(dc);

   return Result;
}

INT STDCALL
NtGdiSelectClipRgn(HDC hDC, HRGN hRgn)
{
   return NtGdiExtSelectClipRgn(hDC, hRgn, RGN_COPY);
}

int STDCALL NtGdiSetMetaRgn(HDC  hDC)
{
  UNIMPLEMENTED;
  return 0;
}

/* EOF */
